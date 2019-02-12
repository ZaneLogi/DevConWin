// Last Modified Date: 11/05/2007

#include "StdAfx.h"

#include <initguid.h>

#include "DevMgr.h"
#include <newdev.h>

#include "StrUtil.h"

#pragma comment( lib, "setupapi.lib" )

class CIdEntry
{
private:
	std::basic_string<TCHAR>	m_strText;	// string looking for
	LPCTSTR						m_pszWild;	// first wild character if any
	bool						m_bInstanceId;

public:
	bool IsInstanceId() const
	{
		return m_bInstanceId;
	}

	bool HasWildCard() const
	{
		return m_pszWild != NULL;
	}

	LPCTSTR GetRule() const
	{
		return m_strText.c_str();
	}

	CIdEntry( LPCTSTR pszId = _T("") )
	{
		Init(pszId);
	}

	void Init(LPCTSTR pszId)
	{
		ASSERT(pszId);

		if ( pszId[0] == INSTANCEID_PREFIX_CHAR ) {
			m_bInstanceId = TRUE;
			pszId = CharNext(pszId);
		}
		else {
			m_bInstanceId = FALSE;
		}
		if ( pszId[0] == QUOTE_PREFIX_CHAR ) {
			//
			// prefix to treat rest of string literally
			//
			m_strText = CharNext(pszId);
			m_pszWild = NULL;
		}
		else {
			//
			// see if any wild characters exist
			//
			m_strText = pszId;
			m_pszWild = _tcschr( m_strText.c_str(), WILD_CHAR );
		}
	}

	bool WildCardMatch( LPCTSTR pszItem)
	{
		//
		// before attempting anything else
		// try and compare everything up to first wild
		//
		if ( !m_pszWild ) {
			return _tcsicmp( pszItem, m_strText.c_str() ) ? false : true;
		}
		if ( _tcsnicmp( pszItem, m_strText.c_str(), m_pszWild - m_strText.c_str() ) != 0 ) {
			return false;
		}

		LPCTSTR pszWildMark = m_pszWild;
		LPCTSTR pszScanItem = pszItem + (m_pszWild - m_strText.c_str());

		for ( ; pszWildMark[0]; )
		{
			//
			// if we get here, we're either at or past a wildcard
			//
			if ( pszWildMark[0] == WILD_CHAR ) {
				//
				// so skip wild chars
				//
				pszWildMark = CharNext(pszWildMark);
				continue;
			}
			//
			// find next wild-card
			//
			LPCTSTR pszNextWild = _tcschr(pszWildMark,WILD_CHAR);
			size_t matchlen;

			if (pszNextWild) {
				//
				// substring
				//
				matchlen = pszNextWild - pszWildMark;
			}
			else {
				//
				// last portion of match
				//
				size_t scanlen = lstrlen(pszScanItem);
				matchlen = lstrlen(pszWildMark);
				if ( scanlen < matchlen ) {
					return false;
				}
				return _tcsicmp( pszScanItem + scanlen - matchlen, pszWildMark) ? false : true;
			}

			if ( _istalpha( pszWildMark[0]) ) {
				//
				// scan for either lower or uppercase version of first character
				//
				TCHAR u = _totupper(pszWildMark[0]);
				TCHAR l = _totlower(pszWildMark[0]);
				while ( pszScanItem[0] && pszScanItem[0] != u && pszScanItem[0] != l ) {
					pszScanItem = CharNext(pszScanItem);
				}
				if ( !pszScanItem[0] ) {
					//
					// ran out of string
					//
					return false;
				}
			}
			else {
				//
				// scan for first character (no case)
				//
				pszScanItem = _tcschr(pszScanItem,pszWildMark[0]);
				if ( !pszScanItem ) {
					//
					// ran out of string
					//
					return false;
				}
			}
			//
			// try and match the sub-string at wildMark against scanItem
			//
			if ( _tcsnicmp(pszScanItem, pszWildMark, matchlen) != 0 ) {
				//
				// nope, try again
				//
				pszScanItem = CharNext(pszScanItem);
				continue;
			}
			//
			// substring matched
			//
			pszScanItem += matchlen;
			pszWildMark += matchlen;
		}
		return (pszWildMark[0] ? false : true);
	}

	bool WildCompareHwIds( CMultiSzString& Array )
	{
		LPCTSTR psz = Array.GetFirst();
		while ( psz ) {
			if ( WildCardMatch(psz) ) {
				return true;
			}
			psz = Array.GetNext();
		}
		return false;
	}

};

CDevMgr::CDevMgr(LPCTSTR pszMachine)
{
	if ( pszMachine )
	{
		int nLen = lstrlen(pszMachine);
		m_pszMachine = new TCHAR[nLen+1];
		lstrcpy( m_pszMachine, pszMachine );
	}
	else
	{
		m_pszMachine = NULL;
	}
}

CDevMgr::~CDevMgr(void)
{
	if ( m_pszMachine )
	{
		delete m_pszMachine;
	}
}

int CDevMgr::EnumerateDevices( CDeviceSet& DeviceSet, DWORD dwFlags, LPCTSTR pszClassName, LPCTSTR pszRule1, ... )
{
	std::vector<LPCTSTR> RuleList;
	va_list marker;
	va_start( marker, pszClassName );
	LPCTSTR psz;
	while ( (psz = va_arg(marker,LPCTSTR)) != NULL )
	{
		RuleList.push_back(psz);
	}
	va_end(marker);

	int nNumIdRule = (int)RuleList.size();
	if ( nNumIdRule == 0 )
	{
		RuleList.push_back(NULL); // prevent to fail to access &RuleList[0]
	}

	return EnumerateDevices( DeviceSet, dwFlags, pszClassName, nNumIdRule, &RuleList[0] );
}

int CDevMgr::EnumerateDevices( CDeviceSet& DeviceSet, DWORD dwFlags, LPCTSTR pszClassName, int nNumIdRule, LPCTSTR apszRule[] )
{
	if ( !DeviceSet.Init(m_pszMachine) )
		return EXIT_FAIL;

	int		nFailCode = EXIT_FAIL;
	GUID	guidClass;
	DWORD	dwNumClass = 0;

	HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;

	std::vector<CIdEntry> RuleList(nNumIdRule);

	if ( pszClassName )	{
		// only search devices under the specific class
		if ( !SetupDiClassGuidsFromNameEx( pszClassName, &guidClass, 1, &dwNumClass, m_pszMachine, NULL) &&
			GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			goto final;
		}
		if ( !dwNumClass ) {
			nFailCode = EXIT_OK;
			goto final;
		}
	}

	bool bAll = (nNumIdRule == 0);
	bool bSearch = false;
	for ( int i = 0; i < nNumIdRule; i++ ) {
		RuleList[i].Init(apszRule[i]);
		if ( RuleList[i].HasWildCard() || !RuleList[i].IsInstanceId() ) {
			//
			// anything other than simple InstanceId's require a search
			//
			bSearch = true;
		}
		if ( RuleList[i].GetRule()[0] == WILD_CHAR && RuleList[i].GetRule()[1] == 0 )
		{
			bAll = true;
		}
	}

	if ( bAll || bSearch ) {
		//
		// add all id's to list
		// if there's a class, filter on specified class
		//
		hDevInfo = SetupDiGetClassDevsEx(
			dwNumClass ? &guidClass : NULL,
			NULL,
			NULL,
			dwFlags | (dwNumClass ? 0 : DIGCF_ALLCLASSES),
			NULL,
			m_pszMachine,
			NULL);
	}
	else {
		//
		// blank list, we'll add instance id's by hand
		//
		hDevInfo = SetupDiCreateDeviceInfoListEx(
			dwNumClass ? &guidClass : NULL,
			NULL,
			m_pszMachine,
			NULL);
	}

	if ( hDevInfo == INVALID_HANDLE_VALUE ) {
		goto final;
	}

	for ( int i = 0; i < nNumIdRule; i++) {
		//
		// add explicit instances to list (even if enumerated all,
		// this gets around DIGCF_PRESENT)
		// do this even if wildcards appear to be detected since they
		// might actually be part of the instance ID of a non-present device
		//
		if ( RuleList[i].IsInstanceId() ) {
			SetupDiOpenDeviceInfo( hDevInfo, RuleList[i].GetRule(), NULL, 0, NULL );
		}
	}

	SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
	devInfoListDetail.cbSize = sizeof(devInfoListDetail);
	if ( !SetupDiGetDeviceInfoListDetail( hDevInfo, &devInfoListDetail ) ) {
		goto final;
	}

	//
	// now enumerate them
	//
	if ( bAll ) {
		bSearch = false;
	}

	SP_DEVINFO_DATA DevInfoData;
	DevInfoData.cbSize = sizeof(DevInfoData);
	for ( int nDevIndex = 0; SetupDiEnumDeviceInfo( hDevInfo, nDevIndex, &DevInfoData); nDevIndex++)
	{
		TCHAR devID[MAX_DEVICE_ID_LEN];
		//
		// determine instance ID
		//
		if (CM_Get_Device_ID_Ex(DevInfoData.DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS) {
			devID[0] = TEXT('\0');
		}

		// alternative method
		TCHAR devID2[MAX_DEVICE_ID_LEN];
        BOOL b = SetupDiGetDeviceInstanceId( hDevInfo, &DevInfoData, devID2, MAX_DEVICE_ID_LEN, NULL );

		if ( bSearch ) {
			for ( int i = 0; i < nNumIdRule; i++ ) {
				if ( RuleList[i].IsInstanceId() ) {
					//
					// match on the instance ID
					//
					if ( RuleList[i].WildCardMatch(devID) ) {
						DeviceSet.AddDevice(devID);
					}
				}
				else {
					//
					// determine hardware ID's
					// and search for matches
					//
					CMultiSzString hwIds;
					hwIds.GetDeviceRegistryProperty( hDevInfo, &DevInfoData, SPDRP_HARDWAREID );
					CMultiSzString compatIds;
					compatIds.GetDeviceRegistryProperty( hDevInfo, &DevInfoData, SPDRP_COMPATIBLEIDS );
					if ( RuleList[i].WildCompareHwIds(hwIds) || RuleList[i].WildCompareHwIds(compatIds) ) {
						DeviceSet.AddDevice(devID);
					}
				}
			}
		}
		else {
			DeviceSet.AddDevice(devID);
		}
	}

	nFailCode  = EXIT_OK;

final:

	if ( hDevInfo != INVALID_HANDLE_VALUE )
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
	return nFailCode;
}

int CDevMgr::EnumerateDevices( CDeviceSet& DeviceSet, DWORD dwFlags, const GUID* pInterfaceGuid )
{
	//
	// dwFlags could be DIGCF_PRESENT
	//
	if ( !DeviceSet.Init(m_pszMachine) )
		return EXIT_FAIL;

	HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
	hDevInfo = SetupDiGetClassDevsEx( pInterfaceGuid, NULL, NULL, (dwFlags|DIGCF_DEVICEINTERFACE), NULL, m_pszMachine, NULL );
	if ( hDevInfo == INVALID_HANDLE_VALUE )
		return EXIT_FAIL;

	SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
	devInfoListDetail.cbSize = sizeof(devInfoListDetail);
	if ( !SetupDiGetDeviceInfoListDetail( hDevInfo, &devInfoListDetail ) ) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return EXIT_FAIL;
	}

	SP_DEVINFO_DATA DevInfoData;
	DevInfoData.cbSize = sizeof(DevInfoData);

	for ( int nDevIndex = 0; SetupDiEnumDeviceInfo( hDevInfo, nDevIndex, &DevInfoData); nDevIndex++)
	{
		TCHAR devID[MAX_DEVICE_ID_LEN];
		//
		// determine instance ID
		//
		if (CM_Get_Device_ID_Ex(DevInfoData.DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS)
		{
			// alternative method
			BOOL b = SetupDiGetDeviceInstanceId( hDevInfo, &DevInfoData, devID, MAX_DEVICE_ID_LEN, NULL );
			if ( !b )
			{
                devID[0] = TEXT('\0');
			}
		}

		SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
		DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);

		if ( !SetupDiEnumDeviceInterfaces( hDevInfo,
										NULL,
										pInterfaceGuid,
										nDevIndex,
										&DeviceInterfaceData ) )
		{
			DWORD dwErr = GetLastError();
			TCHAR str[128];
			GetErrorString( dwErr, str, sizeof(str) );

			DeviceSet.AddDevice(devID);
		}
		else
		{
			switch (DeviceInterfaceData.Flags) {
			case SPINT_ACTIVE:		TRACE("The device is active.\n");				break;
			case SPINT_DEFAULT:		TRACE("The device is the default device.\n");	break;
			case SPINT_REMOVED:		TRACE("The device has been removed.\n");		break;
			default:				TRACE("unknown!\n");							break;
			}

			DWORD dwRequiredLength;
			SetupDiGetDeviceInterfaceDetail( hDevInfo,
				&DeviceInterfaceData,
				NULL,
				0,
				&dwRequiredLength,
				NULL );

			PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData;
			pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)new BYTE[dwRequiredLength];
			pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			SetupDiGetDeviceInterfaceDetail( hDevInfo,
				&DeviceInterfaceData,
				pDeviceInterfaceDetailData,
				dwRequiredLength,
				&dwRequiredLength,
				NULL);

			DeviceSet.AddDevice(devID,pDeviceInterfaceDetailData->DevicePath);

			delete pDeviceInterfaceDetailData;
		}

	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return EXIT_OK;
}

const ClassList& CDevMgr::GetClasses()
{
	m_Classes.clear();

	DWORD				numGuids = 1;
	std::vector<GUID>	vGuids(1);

	while ( !SetupDiBuildClassInfoListEx( 0, &vGuids[0], numGuids, &numGuids, m_pszMachine, NULL ) )
	{
		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {
			return m_Classes;
		}
		vGuids.resize(numGuids);
	}

	m_Classes.resize(numGuids);
	for ( DWORD index = 0; index < numGuids; index++ )
	{
		m_Classes[index].guid = vGuids[index];
		if ( !SetupDiClassNameFromGuidEx( &vGuids[index], m_Classes[index].name, MAX_CLASS_NAME_LEN, NULL, m_pszMachine, NULL ) )
		{
			lstrcpyn( m_Classes[index].name, TEXT("?"), MAX_CLASS_NAME_LEN );
		}
		if ( !SetupDiGetClassDescriptionEx( &vGuids[index], m_Classes[index].desc, LINE_LEN, NULL, m_pszMachine, NULL ) )
		{
			lstrcpyn( m_Classes[index].desc, m_Classes[index].name, LINE_LEN );
		}
		INT nIconIndex;
		if ( SetupDiLoadClassIcon( &vGuids[index], NULL, &nIconIndex ) )
		{
			m_Classes[index].miniIconIndex = (nIconIndex >= 0 ? nIconIndex : -nIconIndex);
		}
		else
		{
			m_Classes[index].miniIconIndex = 18; // unknown
		}

		//
		// Windows XP and later
		//
		DWORD dwRegDataType;
		DWORD dwDevType;
		DWORD dwRequiredSize;
		BOOL b = SetupDiGetClassRegistryProperty(
					&vGuids[index],
                    SPCRP_DEVTYPE,
					&dwRegDataType,
					(PBYTE)&dwDevType,
					sizeof(dwDevType),
					&dwRequiredSize,
					NULL,
					NULL );
		if ( !b ) {
			m_Classes[index].devType = -1;
		}
		else {
			m_Classes[index].devType = dwDevType;
		}

		// upper & lower filters
		HKEY hClassKey = SetupDiOpenClassRegKeyEx(
							&vGuids[index],
							KEY_READ,
							DIOCR_INSTALLER,
							NULL,
							NULL );

		if ( hClassKey != INVALID_HANDLE_VALUE ) {
            m_Classes[index].upperFilters.GetRegistryValue( hClassKey, REGSTR_VAL_UPPERFILTERS );
			m_Classes[index].lowerFilters.GetRegistryValue( hClassKey, REGSTR_VAL_LOWERFILTERS );
		}
	}

	return m_Classes;
}

BOOL CDevMgr::Reboot()
/*++

Routine Description:

    Attempt to reboot computer

Arguments:

    none

Return Value:

    TRUE if API suceeded

--*/
{
	HANDLE Token;
	TOKEN_PRIVILEGES NewPrivileges;
	LUID Luid;

	//
	// we need to "turn on" reboot privilege
	// if any of this fails, try reboot anyway
	//
	if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
		goto final;
	}

	if(!LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&Luid)) {
		CloseHandle(Token);
		goto final;
	}

	NewPrivileges.PrivilegeCount = 1;
	NewPrivileges.Privileges[0].Luid = Luid;
	NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(
		Token,
		FALSE,
		&NewPrivileges,
		0,
		NULL,
		NULL
		);

	CloseHandle(Token);

final:

	//
	// attempt reboot - inform system that this is planned hardware install
	//
	return ExitWindowsEx(EWX_REBOOT, REASON_PLANNED_FLAG|REASON_HWINSTALL);
}

int CDevMgr::Rescan()
/*++

Routine Description:

    RESCAN
    rescan for new devices

Return Value:

    EXIT_xxxx

--*/
{
	//
	// reenumerate from the root of the devnode tree
	// totally CM based
	//
	int failcode = EXIT_FAIL;
	HMACHINE machineHandle = NULL;
	DEVINST devRoot;

	if (m_pszMachine) {
		if (CM_Connect_Machine(m_pszMachine,&machineHandle) != CR_SUCCESS) {
			return failcode;
		}
	}

	if (CM_Locate_DevNode_Ex(&devRoot,NULL,CM_LOCATE_DEVNODE_NORMAL,machineHandle) != CR_SUCCESS) {
		goto final;
	}

	TRACE( TEXT("Scanning for new hardware.\n") );

	if (CM_Reenumerate_DevNode_Ex(devRoot, 0, machineHandle) != CR_SUCCESS) {
		goto final;
	}

	TRACE( TEXT("Scanning completed.\n") );

	failcode = EXIT_OK;

final:
	if (machineHandle) {
		CM_Disconnect_Machine(machineHandle);
	}

	return failcode;
}

int CDevMgr::InstallDriver( LPCTSTR pszInfPath, LPCTSTR pszHwid, bool& bNeedReboot )
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    int failcode = EXIT_FAIL;

	if (m_pszMachine) {
		//
		// must be local machine
		//
		return EXIT_USAGE;
	}

	if (!pszInfPath || !pszHwid ) {
		return EXIT_USAGE;
	}

	//
	// Inf must be a full pathname
	//
    TCHAR achInfPath[MAX_PATH];
	if (GetFullPathName(pszInfPath,MAX_PATH,achInfPath,NULL) >= MAX_PATH) {
		//
		// inf pathname too long
		//
		return EXIT_FAIL;
	}

	//
	// List of hardware ID's must be double zero-terminated
	//
	TCHAR achHwIdList[LINE_LEN+4];
	ZeroMemory(achHwIdList,sizeof(achHwIdList));
	lstrcpyn(achHwIdList,pszHwid,LINE_LEN);

	//
	// Use the INF File to extract the Class GUID.
	//
	GUID guidClass;
	TCHAR achClassName[MAX_CLASS_NAME_LEN];
	if (!SetupDiGetINFClass(achInfPath,&guidClass,achClassName,sizeof(achClassName)/sizeof(achClassName[0]),0))
	{
		goto final;
	}

	//
	// Create the container for the to-be-created Device Information Element.
	//
	DeviceInfoSet = SetupDiCreateDeviceInfoList(&guidClass,0);
	if(DeviceInfoSet == INVALID_HANDLE_VALUE)
	{
		goto final;
	}

	//
	// Now create the element.
	// Use the Class GUID and Name from the INF file.
	//
	SP_DEVINFO_DATA DeviceInfoData;
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
								achClassName,
								&guidClass,
								NULL,
								0,
								DICD_GENERATE_ID,
								&DeviceInfoData))
	{
		goto final;
	}

	//
	// Add the HardwareID to the Device's HardwareID property.
	//
	if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
										&DeviceInfoData,
										SPDRP_HARDWAREID,
										(LPBYTE)achHwIdList,
										(lstrlen(achHwIdList)+1+1)*sizeof(TCHAR)))
	{
		goto final;
	}

	//
	// Transform the registry element into an actual devnode
	// in the PnP HW tree.
	//
	if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
								DeviceInfoSet,
								&DeviceInfoData))
	{
		goto final;
	}

	TRACE( TEXT("Device node created. Install is complete when drivers are updated...\n") );

	//
	// update the driver for the device we just created
	//
	failcode = UpdateDriver( pszInfPath, pszHwid, bNeedReboot);

final:
	if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	}

	return failcode;
}

int CDevMgr::UpdateDriver( LPCTSTR pszInfPath, LPCTSTR pszHwid, bool& bNeedReboot )
{
	int failcode = EXIT_FAIL;
	HMODULE newdevMod = NULL;

	if (m_pszMachine) {
		//
		// must be local machine
		//
		return EXIT_USAGE;
	}

	if (!pszInfPath || !pszHwid ) {
		return EXIT_USAGE;
	}

	//
	// Inf must be a full pathname
	//
    TCHAR achInfPath[MAX_PATH];
	if (GetFullPathName(pszInfPath,MAX_PATH,achInfPath,NULL) >= MAX_PATH) {
		//
		// inf pathname too long
		//
		return EXIT_FAIL;
	}

	DWORD flags = 0;
	if (GetFileAttributes(achInfPath)==(DWORD)(-1)) {
		//
		// inf doesn't exist
		//
		return EXIT_FAIL;
	}
	flags |= INSTALLFLAG_FORCE;

	//
	// make use of UpdateDriverForPlugAndPlayDevices
	//
    newdevMod = LoadLibrary(TEXT("newdev.dll"));
	if(!newdevMod) {
		goto final;
	}

	UpdateDriverForPlugAndPlayDevicesProto UpdateFn
		= (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(newdevMod,UPDATEDRIVERFORPLUGANDPLAYDEVICES);
	if(!UpdateFn)
	{
		goto final;
	}

	TRACE( TEXT("Updating drivers for %s from %s.\n"), pszHwid, achInfPath );

	BOOL reboot;
	if (!UpdateFn(NULL,pszHwid,achInfPath,flags,&reboot)) {
        goto final;
    }

	TRACE( TEXT("Drivers updated successfully.\n") );

	bNeedReboot = (reboot == TRUE);
	failcode = bNeedReboot ? EXIT_REBOOT : EXIT_OK;

final:

    if(newdevMod) {
        FreeLibrary(newdevMod);
    }

    return failcode;
}

int CDevMgr::UpdateDriverNI( LPCTSTR pszInfPath, LPCTSTR pszHwid, bool& bNeedReboot )
{
	//
	// turn off interactive mode while doing the update
	//
	HMODULE setupapiMod = NULL;
	SetupSetNonInteractiveModeProto SetNIFn;
	int res;
	BOOL prev;

	setupapiMod = LoadLibrary(TEXT("setupapi.dll"));
	if(!setupapiMod) {
		return UpdateDriver(pszInfPath,pszHwid,bNeedReboot);
	}

	SetNIFn = (SetupSetNonInteractiveModeProto)GetProcAddress(setupapiMod,SETUPSETNONINTERACTIVEMODE);
	if(!SetNIFn)
	{
		FreeLibrary(setupapiMod);
		return UpdateDriver(pszInfPath,pszHwid,bNeedReboot);
	}

	prev = SetNIFn(TRUE);
	res = UpdateDriver(pszInfPath,pszHwid,bNeedReboot);
	SetNIFn(prev);
	FreeLibrary(setupapiMod);
	return res;
}

//
// Get Driver Key from HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{36FC9E60-C465-11CF-8056-444553540000}
// Use DeviceIoControl(IOCTL_GET_HCD_DRIVERKEY_NAME) to ask the driver where is the driver key.
//
static tstring GetUsbHCDDriverKeyName( HANDLE hHCD )
{
	// Get the length of the name of the driver key of the HCD
	USB_HCD_DRIVERKEY_NAME	DriverKeyName;
	PUSB_HCD_DRIVERKEY_NAME	pDriverKeyName = NULL;
	DWORD nBytes;
	tstring strDriverKeyName;

	BOOL success = DeviceIoControl( hHCD,
									IOCTL_GET_HCD_DRIVERKEY_NAME,
									&DriverKeyName,
									sizeof(DriverKeyName),
									&DriverKeyName,
									sizeof(DriverKeyName),
									&nBytes,
									NULL);
	if ( !success ) {
		ASSERT(FALSE);
		goto GetHCDDriverKeyNameError;
	}

	nBytes = DriverKeyName.ActualLength;
	if (nBytes <= sizeof(DriverKeyName) ) {
		ASSERT(FALSE);
		goto GetHCDDriverKeyNameError;
	}

	pDriverKeyName = (PUSB_HCD_DRIVERKEY_NAME)new BYTE[nBytes];
	if (pDriverKeyName == NULL) {
		ASSERT(FALSE);
		goto GetHCDDriverKeyNameError;
	}

	success = DeviceIoControl( hHCD,
							   IOCTL_GET_HCD_DRIVERKEY_NAME,
							   pDriverKeyName,
							   nBytes,
							   pDriverKeyName,
							   nBytes,
							   &nBytes,
							   NULL);
	if (!success) {
		ASSERT(FALSE);
		goto GetHCDDriverKeyNameError;
	}

	ConvertFormat( strDriverKeyName, pDriverKeyName->DriverKeyName );
	delete (PBYTE)pDriverKeyName;
	return strDriverKeyName;

GetHCDDriverKeyNameError:
	if ( pDriverKeyName != NULL ) {
		delete (PBYTE)pDriverKeyName;
		pDriverKeyName = NULL;
	}
	return tstring(_T(""));
}

//
// Get Driver Key from HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\
//
static tstring GetUsbDeviceDriverKeyName( HANDLE hHub, ULONG ulConnectionIndex )
{
	BOOL								success;
	ULONG								nBytes;
	USB_NODE_CONNECTION_DRIVERKEY_NAME	DriverKeyName;
	PUSB_NODE_CONNECTION_DRIVERKEY_NAME	pDriverKeyName = NULL;
	tstring								strDriverKeyName;

	// Get the length of the name of the driver key of the device attached to
	// the specified port.
	//
	DriverKeyName.ConnectionIndex = ulConnectionIndex;
	success = DeviceIoControl( hHub,
							   IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
							   &DriverKeyName,
							   sizeof(DriverKeyName),
							   &DriverKeyName,
							   sizeof(DriverKeyName),
							   &nBytes,
							   NULL);
	if (!success)
	{
		ASSERT(FALSE);
		goto GetDriverKeyNameError;
	}

	// Allocate space to hold the driver key name
	//
	nBytes = DriverKeyName.ActualLength;
	if (nBytes <= sizeof(DriverKeyName))
	{
		ASSERT(FALSE);
		goto GetDriverKeyNameError;
	}

	pDriverKeyName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)new BYTE[nBytes];
	if (pDriverKeyName == NULL)
	{
		ASSERT(FALSE);
		goto GetDriverKeyNameError;
	}

	// Get the name of the driver key of the device attached to
	// the specified port.
	//
	pDriverKeyName->ConnectionIndex = ulConnectionIndex;
	success = DeviceIoControl( hHub,
							   IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
							   pDriverKeyName,
							   nBytes,
							   pDriverKeyName,
							   nBytes,
							   &nBytes,
							   NULL);
	if (!success)
	{
		ASSERT(FALSE);
		goto GetDriverKeyNameError;
	}

	// Convert the driver key name
	//
	ConvertFormat( strDriverKeyName, pDriverKeyName->DriverKeyName );

	// All done, free the uncoverted driver key name and return the
	// converted driver key name
	//
	delete (PBYTE)pDriverKeyName;

	return strDriverKeyName;

GetDriverKeyNameError:
	// There was an error, free anything that was allocated
	//
	if (pDriverKeyName != NULL)
	{
		delete (PBYTE)pDriverKeyName;
		pDriverKeyName = NULL;
	}

	return tstring(_T(""));
}

//
// GetUsbRootHubName
//
static tstring GetRootUsbHubDevicePath( HANDLE hHostController )
{
	BOOL                success;
	ULONG               nBytes;
	USB_ROOT_HUB_NAME   RootHubName;
	PUSB_ROOT_HUB_NAME  pRootHubName = NULL;
	tstring				strRootHubDevicePath;

	// Get the length of the name of the Root Hub attached to the
	// Host Controller
	//
	success = DeviceIoControl( hHostController,
							   IOCTL_USB_GET_ROOT_HUB_NAME,
							   0,
							   0,
							   &RootHubName,
							   sizeof(RootHubName),
							   &nBytes,
							   NULL);
	if (!success)
	{
		ASSERT(FALSE);
		goto GetRootHubNameError;
	}

	// Allocate space to hold the Root Hub name
	//
	nBytes = RootHubName.ActualLength;
	pRootHubName = (PUSB_ROOT_HUB_NAME)new BYTE[nBytes];

	if (pRootHubName == NULL)
	{
		ASSERT(FALSE);
		goto GetRootHubNameError;
	}

	// Get the name of the Root Hub attached to the Host Controller
	//
	success = DeviceIoControl( hHostController,
							   IOCTL_USB_GET_ROOT_HUB_NAME,
							   NULL,
							   0,
							   pRootHubName,
							   nBytes,
							   &nBytes,
							   NULL);
	if (!success)
	{
		ASSERT(FALSE);
		goto GetRootHubNameError;
	}

	// Convert the Root Hub name
	//
	ConvertFormat( strRootHubDevicePath, pRootHubName->RootHubName );

	// All done, free the uncoverted Root Hub name and return the
	// converted Root Hub name
	//
	delete (PBYTE)pRootHubName;

    return strRootHubDevicePath;

GetRootHubNameError:
	// There was an error, free anything that was allocated
	//
	if (pRootHubName != NULL)
	{
		delete (PBYTE)pRootHubName;
		pRootHubName = NULL;
	}

	return tstring(_T(""));
}

//
// GetExternalUsbHubDevicePath
//
static tstring GetExternalUsbHubDevicePath( HANDLE hHub, ULONG ulConnectionIndex )
{
	BOOL						success;
	ULONG						nBytes;
	USB_NODE_CONNECTION_NAME	ExtHubName;
	PUSB_NODE_CONNECTION_NAME	pExtHubName = NULL;
	tstring						strExtHubName;

	// Get the length of the name of the external hub attached to the
	// specified port.
	//
	ExtHubName.ConnectionIndex = ulConnectionIndex;
	success = DeviceIoControl( hHub,
							   IOCTL_USB_GET_NODE_CONNECTION_NAME,
							   &ExtHubName,
							   sizeof(ExtHubName),
							   &ExtHubName,
							   sizeof(ExtHubName),
							   &nBytes,
							   NULL);
	if (!success)
	{
		ASSERT(FALSE);
		goto GetExternalHubNameError;
	}

	// Allocate space to hold the external hub name
	//
	nBytes = ExtHubName.ActualLength;
	if (nBytes <= sizeof(ExtHubName))
	{
		ASSERT(FALSE);
		goto GetExternalHubNameError;
	}

	pExtHubName = (PUSB_NODE_CONNECTION_NAME)new BYTE[nBytes];
	if (pExtHubName == NULL)
	{
		ASSERT(FALSE);
		goto GetExternalHubNameError;
	}

	// Get the name of the external hub attached to the specified port
	//
	pExtHubName->ConnectionIndex = ulConnectionIndex;
	success = DeviceIoControl( hHub,
							   IOCTL_USB_GET_NODE_CONNECTION_NAME,
							   pExtHubName,
							   nBytes,
							   pExtHubName,
							   nBytes,
							   &nBytes,
							   NULL);
	if (!success)
	{
		ASSERT(FALSE);
		goto GetExternalHubNameError;
	}

	// Convert the External Hub name
	//
	ConvertFormat( strExtHubName, pExtHubName->NodeName );

	// All done, free the uncoverted external hub name and return the
	// converted external hub name
	//
	delete (PBYTE)pExtHubName;

	return strExtHubName;

GetExternalHubNameError:
	// There was an error, free anything that was allocated
	//
	if (pExtHubName != NULL)
	{
		delete (PBYTE)pExtHubName;
		pExtHubName = NULL;
	}

	return tstring(_T(""));
}


bool GetUsbDeviceConfigDescriptor( HANDLE hHubDev, ULONG ulConnectionIndex, UCHAR ucDescriptorIndex, USB_DESCRIPTOR* pUsbDesc )
{
	BOOL    success;
	ULONG   nBytes;
	ULONG   nBytesReturned;

	UCHAR   configDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) + sizeof(USB_CONFIGURATION_DESCRIPTOR)];

	PUSB_DESCRIPTOR_REQUEST         configDescReq;
	PUSB_CONFIGURATION_DESCRIPTOR   configDesc;

	// Request the Configuration Descriptor the first time using our
	// local buffer, which is just big enough for the Cofiguration
	// Descriptor itself.
	//
	nBytes = sizeof(configDescReqBuf);

	configDescReq = (PUSB_DESCRIPTOR_REQUEST)configDescReqBuf;
	configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(configDescReq+1);

	// Zero fill the entire request structure
	//
	memset(configDescReq, 0, nBytes);

	// Indicate the port from which the descriptor will be requested
	//
	configDescReq->ConnectionIndex = ulConnectionIndex;

	//
	// USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
	// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
	//
	// USBD will automatically initialize these fields:
	//     bmRequest = 0x80
	//     bRequest  = 0x06
	//
	// We must inititialize these fields:
	//     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
	//     wIndex    = Zero (or Language ID for String Descriptors)
	//     wLength   = Length of descriptor buffer
	//
	configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | ucDescriptorIndex;
	configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl( hHubDev,
							   IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
							   configDescReq,
							   nBytes,
							   configDescReq,
							   nBytes,
							   &nBytesReturned,
							   NULL);
	if (!success)
	{
		DWORD dwErr = GetLastError();
		TRACE("!!!ERR: [CUsbDeviceDescriptors GetUsbDeviceConfigDescriptor] hHubDev 0x%08x, ConnectionIndex %d, DescriptorIndex %d\n",
			hHubDev, ulConnectionIndex, ucDescriptorIndex );
		return false;
	}

	if (nBytes != nBytesReturned)
	{
		TRACE("!!!ERR: [CUsbDeviceDescriptors GetUsbDeviceConfigDescriptor] hHubDev 0x%08x, ConnectionIndex %d, DescriptorIndex %d, %d != %d\n",
			hHubDev, ulConnectionIndex, ucDescriptorIndex,
			nBytes, nBytesReturned );
		return false;
	}

	if (configDesc->wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))
	{
		TRACE("!!!ERR: [CUsbDeviceDescriptors GetUsbDeviceConfigDescriptor] hHubDev 0x%08x, ConnectionIndex %d, DescriptorIndex %d, wTotalLength %d too small.\n",
			hHubDev, ulConnectionIndex, ucDescriptorIndex,
			configDesc->wTotalLength );
		return false;
	}

	// Now request the entire Configuration Descriptor using a dynamically
	// allocated buffer which is sized big enough to hold the entire descriptor
	//
	nBytes = sizeof(USB_DESCRIPTOR_REQUEST) + configDesc->wTotalLength;

	configDescReq = (PUSB_DESCRIPTOR_REQUEST)new BYTE[nBytes];

	if (configDescReq == NULL)
	{
		ASSERT(FALSE);
		TRACE("!!!ERR: [CUsbDeviceDescriptors GetUsbDeviceConfigDescriptor] Failed to allocate memory %d bytes.\n",
			nBytes );
		return false;
	}

	configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(configDescReq+1);

	// Indicate the port from which the descriptor will be requested
	//
	configDescReq->ConnectionIndex = ulConnectionIndex;

	//
	// USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
	// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
	//
	// USBD will automatically initialize these fields:
	//     bmRequest = 0x80
	//     bRequest  = 0x06
	//
	// We must inititialize these fields:
	//     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
	//     wIndex    = Zero (or Language ID for String Descriptors)
	//     wLength   = Length of descriptor buffer
	//
	configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | ucDescriptorIndex;
	configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl( hHubDev,
							   IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
							   configDescReq,
							   nBytes,
							   configDescReq,
							   nBytes,
							   &nBytesReturned,
							   NULL);
	if (!success)
	{
		DWORD dwErr = GetLastError();
		TRACE("!!!ERR: [CUsbDeviceDescriptors GetUsbDeviceConfigDescriptor] hHubDev 0x%08x, ConnectionIndex %d, DescriptorIndex %d, Failed to get full descriptor!!!\n",
			hHubDev, ulConnectionIndex, ucDescriptorIndex );
		delete (PBYTE)configDescReq;
		return false;
	}

	if (nBytes != nBytesReturned)
	{
		ASSERT(FALSE);
		delete (PBYTE)configDescReq;
		return false;
	}

	if (configDesc->wTotalLength != (nBytes - sizeof(USB_DESCRIPTOR_REQUEST)))
	{
		ASSERT(FALSE);
		delete (PBYTE)configDescReq;
		return false;
	}

	pUsbDesc->assign( (PBYTE)configDesc, (PBYTE)configDesc + configDesc->wTotalLength );

	delete (PBYTE)configDescReq;

	return true;
}


//
// EnumerateUsbHostControllers
//
bool EnumerateUsbHostControllers( LPCBENUMUSBHOST pCallback, LPVOID lpContext )
{
	CDevMgr dm;
	CDeviceSet dsUHCs, dsUsb;
	dm.EnumerateDevices( dsUHCs, DIGCF_PRESENT, (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER );


	for ( int i = 0; i < (int)dsUHCs.GetDeviceCount(); i++ )
	{
		HANDLE hHCDev = CreateFile( dsUHCs.GetDevice(i).GetDevicePath(),
									GENERIC_WRITE,
									FILE_SHARE_WRITE,
									NULL,
									OPEN_EXISTING,
									0,
									NULL);
		// If the handle is valid, then we've successfully opened a Host
		// Controller.  Display some info about the Host Controller itself,
		// then enumerate the Root Hub attached to the Host Controller.
		//
		if (hHCDev != INVALID_HANDLE_VALUE)
		{
			tstring strUsbRootHubDevicePath = GetRootUsbHubDevicePath(hHCDev);
			DWORD dwRet = pCallback( dsUHCs.GetDevice(i).GetDesc(), strUsbRootHubDevicePath.c_str(), lpContext );

			CloseHandle(hHCDev);

			if (dwRet == 0)
				break;
		}
	}
	return true;
}

bool EnumerateUsbHub( LPCTSTR pszHubDevicePath, LPCBENUMUSBHUB pCallback, LPVOID pContext )
{
	BOOL	success = TRUE;
	HANDLE	hHubDevice = INVALID_HANDLE_VALUE;
	TCHAR	achDeviceName[MAX_PATH];
	ULONG	nBytes;

	USB_NODE_INFORMATION UsbNodeInfo;

	// Try to hub the open device
	//
	_stprintf_s( achDeviceName, _T("\\\\.\\%s"), pszHubDevicePath );
	hHubDevice = CreateFile( achDeviceName,
							 GENERIC_WRITE,
							 FILE_SHARE_WRITE,
							 NULL,
							 OPEN_EXISTING,
							 0,
							 NULL );
	if ( hHubDevice == INVALID_HANDLE_VALUE )
	{
		ASSERT(FALSE);
		success = FALSE;
		goto EnumerateHubError;
	}

	//
	// Now query USBHUB for the USB_NODE_INFORMATION structure for this hub.
	// This will tell us the number of downstream ports to enumerate, among
	// other things.
	//
	success = DeviceIoControl( hHubDevice,
							   IOCTL_USB_GET_NODE_INFORMATION,
							   &UsbNodeInfo,
							   sizeof(UsbNodeInfo),
							   &UsbNodeInfo,
							   sizeof(UsbNodeInfo),
							   &nBytes,
							   NULL);
	if (!success)
	{
		ASSERT(FALSE);
		goto EnumerateHubError;
	}

	pCallback( hHubDevice, &UsbNodeInfo, pContext );

	success = TRUE;

EnumerateHubError:
	//
	// Clean up any stuff that got allocated
	//
	if (hHubDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hHubDevice);
		hHubDevice = INVALID_HANDLE_VALUE;
	}
	return success ? true : false;
}

static LPCTSTR ConnectionStatuses[] =
{
	_T("NoDeviceConnected"),
	_T("DeviceConnected"),
	_T("DeviceFailedEnumeration"),
	_T("DeviceGeneralFailure"),
	_T("DeviceCausedOvercurrent"),
	_T("DeviceNotEnoughPower"),
};

//
// EnumerateUsbHubPorts
//
bool EnumerateUsbHubPorts( HANDLE hUsbHub, LPCBENUMUSBHUBPORT pCallback, LPVOID lpContext )
{
	BOOL								success = true;
	DWORD								nBytes;
	USB_NODE_INFORMATION				UsbNodeInfo;
	PUSB_NODE_CONNECTION_INFORMATION_EX	pUsbNodeConnectInfoEx = NULL;

	CDevMgr dm;
	CDeviceSet dsUsbDevices;

	dm.EnumerateDevices( dsUsbDevices, DIGCF_PRESENT, NULL, _T("USB\\*"), NULL );

	//
	// Now query USBHUB for the USB_NODE_INFORMATION structure for this hub.
	// This will tell us the number of downstream ports to enumerate, among
	// other things.
	//
	success = DeviceIoControl( hUsbHub,
							   IOCTL_USB_GET_NODE_INFORMATION,
							   &UsbNodeInfo,
							   sizeof(USB_NODE_INFORMATION),
							   &UsbNodeInfo,
							   sizeof(USB_NODE_INFORMATION),
							   &nBytes,
							   NULL );
	if (!success)
	{
		ASSERT(FALSE);
		goto EnumerateHubError;
	}

	nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION_EX)+30*sizeof(USB_PIPE_INFO);
	pUsbNodeConnectInfoEx = (USB_NODE_CONNECTION_INFORMATION_EX*)new BYTE[nBytes];
	if ( !pUsbNodeConnectInfoEx )
		goto EnumerateHubError;

	// Loop over all ports of the hub.
	int nNumPorts = UsbNodeInfo.u.HubInformation.HubDescriptor.bNumberOfPorts;

	//
	// Port indices are 1 based, not 0 based.
	//
	for ( int nPort = 1; nPort <= nNumPorts; nPort++ )
	{
		pUsbNodeConnectInfoEx->ConnectionIndex = nPort;
		success = DeviceIoControl( hUsbHub,
								   IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
								   pUsbNodeConnectInfoEx,
								   nBytes,
								   pUsbNodeConnectInfoEx,
								   nBytes,
								   &nBytes,
								   NULL);
		if (!success)
		{
			continue;
		}

		tstring strDriverKeyName;
		// If there is a device connected, get the Device Description
		//
		if (pUsbNodeConnectInfoEx->ConnectionStatus != NoDeviceConnected)
		{
			strDriverKeyName = GetUsbDeviceDriverKeyName( hUsbHub, nPort );
		}

        tstring strDeviceName;
		Format( strDeviceName, _T("[Port%d] %s"),
			pUsbNodeConnectInfoEx->ConnectionIndex,
			ConnectionStatuses[pUsbNodeConnectInfoEx->ConnectionStatus] );

		tstring strDeviceDesc = _T("UNKNOWN");
		if ( strDriverKeyName.length() > 0 )
		{
			int nNumDevs = dsUsbDevices.GetDeviceCount();
			tstring strTestDevDrvKeyName;
			for ( int i = 0; i < nNumDevs; i++ ) {
				bool b = dsUsbDevices.GetDevice(i).GetStringProperty(SPDRP_DRIVER,strTestDevDrvKeyName);
				if ( b && 0 == Compare(strTestDevDrvKeyName.c_str(),strDriverKeyName.c_str()) ) {
					dsUsbDevices.GetDevice(i).GetStringProperty(SPDRP_DEVICEDESC,strDeviceDesc);
					break;
				}
			}

			strDeviceName += _T(" :  ") + strDeviceDesc;
		}

		tstring strExtHubDevicePath;
		if (pUsbNodeConnectInfoEx->DeviceIsHub)
		{
			strExtHubDevicePath = GetExternalUsbHubDevicePath(hUsbHub,nPort);
		}

		pCallback( strDeviceName.c_str(),
			strDriverKeyName.c_str(),
			strExtHubDevicePath.length() > 0 ? strExtHubDevicePath.c_str() : NULL,
			pUsbNodeConnectInfoEx,
			lpContext );
	}

	// anyway, success
	success = TRUE;

EnumerateHubError:
	if ( pUsbNodeConnectInfoEx )
	{
		delete (PBYTE)pUsbNodeConnectInfoEx;
		pUsbNodeConnectInfoEx = NULL;
	}
	return success ? true : false;
}
