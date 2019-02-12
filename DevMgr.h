#pragma once

// Last Modified Date: 11/05/2007

#include <setupapi.h>
#include <cfgmgr32.h>
#include <regstr.h>

#include <usbiodef.h>
#include <winioctl.h>

#undef _MP
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#pragma warning( disable : 4200 )
#include <usbioctl.h>
#pragma warning( default : 4200 )
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400

#include <vector>
#include <string>

#include "MultiSzString.h"

#define INSTANCEID_PREFIX_CHAR TEXT('@') // character used to prefix instance ID's
#define CLASS_PREFIX_CHAR      TEXT('=') // character used to prefix class name
#define WILD_CHAR              TEXT('*') // wild character
#define QUOTE_PREFIX_CHAR      TEXT('\'') // prefix character to ignore wild characters
#define SPLIT_COMMAND_SEP      TEXT(":=") // whole word, indicates end of id's

//
// UpdateDriverForPlugAndPlayDevices
//
typedef BOOL (WINAPI *UpdateDriverForPlugAndPlayDevicesProto)(HWND hwndParent,
                                                         LPCTSTR HardwareId,
                                                         LPCTSTR FullInfPath,
                                                         DWORD InstallFlags,
                                                         PBOOL bRebootRequired OPTIONAL
                                                         );
typedef BOOL (WINAPI *SetupSetNonInteractiveModeProto)(IN BOOL NonInteractiveFlag
                                                      );


#ifdef _UNICODE
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesW"
#else
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesA"
#endif
#define SETUPSETNONINTERACTIVEMODE "SetupSetNonInteractiveMode"

//
// exit codes
//
#define EXIT_OK      (0)
#define EXIT_REBOOT  (1)
#define EXIT_FAIL    (2)
#define EXIT_USAGE   (3)

typedef struct CLASS_INFO
{
	GUID	guid;
	TCHAR	name[MAX_CLASS_NAME_LEN];
	TCHAR	desc[LINE_LEN];
	INT		miniIconIndex;
	INT		devType;

	CMultiSzString	upperFilters;
	CMultiSzString	lowerFilters;
} CLASSINFO;

typedef struct RESOURCE_DESCRIPTOR
{
	RESOURCEID			resID; // ResType_Mem, ResType_IO, ResType_DMA, ResType_IRQ
	std::vector<BYTE>	resData;
} RESOURCE_DESCRIPTOR;

typedef std::vector<CLASS_INFO>		ClassList;
typedef std::basic_string<TCHAR>	tstring;
typedef std::vector<tstring>		tstringArray;

inline int GetClassIndex( const ClassList& cl, const GUID& guid )
{
	int i = 0;
	for ( ClassList::const_iterator itr = cl.begin(); itr != cl.end(); ++itr, ++i )
	{
		if ( itr->guid == guid )
		{
			return i;
		}
	}

	return -1;
}

inline void GetErrorString( DWORD dwErrCode, LPTSTR pString, int nMaxCount )
{
	LPTSTR pMsgBuf;
	::FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&pMsgBuf,
		0, NULL );

	_tcscpy_s( pString, nMaxCount, pMsgBuf );
	pString[nMaxCount-1] = _T('\0');

	LocalFree(pMsgBuf);
}

class CDevice
{
	HDEVINFO		m_hDevInfo;
	int				m_nDevIndex;
	SP_DEVINFO_DATA	m_DevInfoData;
	tstring			m_strID;
	tstring			m_strDesc;
	tstring			m_strDevicePath;

	void GetInformation()
	{
		if ( !GetStringProperty( SPDRP_FRIENDLYNAME, m_strDesc ) )
		{
			GetStringProperty( SPDRP_DEVICEDESC, m_strDesc );
		}
	}

	bool SetPropertyChange( DWORD dwStageChange, bool& bNeedReboot )
	{
		bNeedReboot = false;

		SP_PROPCHANGE_PARAMS pcp;
		switch ( dwStageChange ) {
		case  DICS_ENABLE:
			//
			// enable both on global and config-specific profile
			// do global first and see if that succeeded in enabling the device
			// (global enable doesn't mark reboot required if device is still
			// disabled on current config whereas vice-versa isn't true)
			//
			pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
			pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
			pcp.StateChange = DICS_ENABLE;
			pcp.Scope = DICS_FLAG_GLOBAL;
			pcp.HwProfile = 0;
			//
			// don't worry if this fails, we'll get an error when we try config-specific.
			if ( SetupDiSetClassInstallParams( m_hDevInfo, &m_DevInfoData, &pcp.ClassInstallHeader, sizeof(pcp) ) )
			{
				SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,m_hDevInfo,&m_DevInfoData);
			}
			//
			// now enable on config-specific
			//
			pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
			pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
			pcp.StateChange = DICS_ENABLE;
			pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
			pcp.HwProfile = 0;
			break;
		default:
			//
			// operate on config-specific profile
			//
			pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
			pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
			pcp.StateChange = dwStageChange;
			pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
			pcp.HwProfile = 0;
			break;
		}

		if ( !SetupDiSetClassInstallParams( m_hDevInfo, &m_DevInfoData, &pcp.ClassInstallHeader,sizeof(pcp))
			|| !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,m_hDevInfo,&m_DevInfoData))
		{
			//
			// failed to invoke DIF_PROPERTYCHANGE
			//
			return false;
		}
		else {
			//
			// see if device needs reboot
			//
			SP_DEVINSTALL_PARAMS devParams;
			devParams.cbSize = sizeof(devParams);
			if ( SetupDiGetDeviceInstallParams( m_hDevInfo, &m_DevInfoData, &devParams)
				&& (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT)))
			{
				bNeedReboot = true;
			}
		}
		return true;
	}


public:
	CDevice( HDEVINFO hDevInfo, int nDevIndex, const SP_DEVINFO_DATA& DevInfoData, LPCTSTR pszID, LPCTSTR pszDevicePath = NULL ) :
		m_hDevInfo(hDevInfo), m_nDevIndex(nDevIndex), m_DevInfoData(DevInfoData), m_strID(pszID),
		m_strDevicePath( pszDevicePath ? pszDevicePath : _T("") )
	{
		GetInformation();
	}

	LPCTSTR GetID() const
	{
		return m_strID.c_str();
	}

	LPCTSTR GetDesc() const
	{
		return m_strDesc.c_str();
	}

	const GUID* GetClassGUID() const
	{
		return &m_DevInfoData.ClassGuid;
	}

	LPCTSTR GetDevicePath() const
	{
		return m_strDevicePath.c_str();
	}

	bool GetStringProperty( DWORD dwProp, tstring& rString ) const
	{
		rString = _T("?");

		DWORD		dwSize		= ( (DWORD)rString.size() + 1 ) * sizeof(TCHAR);
		DWORD		dwDataType	= 0;

		while ( !SetupDiGetDeviceRegistryProperty( m_hDevInfo, (PSP_DEVINFO_DATA)&m_DevInfoData, dwProp, &dwDataType, (LPBYTE)&rString[0], dwSize, &dwSize ) )
		{
			if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER || dwDataType != REG_SZ ) {
				rString = _T("?");
				return false;
			}
			rString.resize((dwSize/sizeof(TCHAR))+1);
		}
		return true;
	}

	bool GetMzStringProperty( DWORD dwProp, CMultiSzString& rMzString ) const
	{
		DWORD dwErrCode;
		rMzString.GetDeviceRegistryProperty( m_hDevInfo, (PSP_DEVINFO_DATA)&m_DevInfoData, dwProp, &dwErrCode );

		return dwErrCode == ERROR_SUCCESS;
	}

	bool SetMzStringProperty( DWORD dwProp, CMultiSzString& rMzString ) const
	{
		DWORD dwErrCode;
		rMzString.SetDeviceRegistryProperty( m_hDevInfo, (PSP_DEVINFO_DATA)&m_DevInfoData, dwProp, &dwErrCode );

		return dwErrCode == ERROR_SUCCESS;
	}

	bool Enable( bool& bNeedRestart )
	{
		return SetPropertyChange( DICS_ENABLE, bNeedRestart );
	}

	bool Disable( bool& bNeedRestart )
	{
		return SetPropertyChange( DICS_DISABLE, bNeedRestart );
	}

	bool Restart( bool& bNeedRestart )
	{
		return SetPropertyChange( DICS_PROPCHANGE, bNeedRestart );
	}

	bool Remove( bool& bNeedRestart )
	{
		bNeedRestart = false;

		// must be local machine as we need to involve class/co installers
		SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
		devInfoListDetail.cbSize = sizeof(devInfoListDetail);
		if ( (!SetupDiGetDeviceInfoListDetail(m_hDevInfo,&devInfoListDetail))
			|| devInfoListDetail.RemoteMachineHandle != NULL )
		{
			return false;
		}

		SP_REMOVEDEVICE_PARAMS rmdParams;
		rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
		rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
		rmdParams.HwProfile = 0;
		if ( !SetupDiSetClassInstallParams( m_hDevInfo, &m_DevInfoData, &rmdParams.ClassInstallHeader, sizeof(rmdParams))
			|| !SetupDiCallClassInstaller(DIF_REMOVE,m_hDevInfo,&m_DevInfoData))
		{
			//
			// failed to invoke DIF_REMOVE
			//
			return false;
		}
		else
		{
			//
			// see if device needs reboot
			//
		    SP_DEVINSTALL_PARAMS devParams;
			devParams.cbSize = sizeof(devParams);
			if ( SetupDiGetDeviceInstallParams(m_hDevInfo,&m_DevInfoData,&devParams)
				&& (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT)))
			{
				//
				// reboot required
				//
				bNeedRestart = true;
			}
		}
		return true;
	}

	bool GetStatus( ULONG& ulDN_status, ULONG& ulCM_PROB_problem ) const
	{
		ulDN_status = 0;
		ulCM_PROB_problem = 0;

		SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
		devInfoListDetail.cbSize = sizeof(devInfoListDetail);
		if ( (!SetupDiGetDeviceInfoListDetail(m_hDevInfo,&devInfoListDetail)) ||
			(CM_Get_DevNode_Status_Ex(&ulDN_status,&ulCM_PROB_problem,m_DevInfoData.DevInst,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS))
		{
			return false;
		}

		//
		// handle off the status/problem codes
		//
		if ((ulDN_status & DN_HAS_PROBLEM) && ulCM_PROB_problem == CM_PROB_DISABLED) {
			// Device is disabled.
		}
		else if (ulDN_status & DN_HAS_PROBLEM) {
			// Device has a problem: %1!02u!. => ulCM_PROB_problem
		}
		else if (ulDN_status & DN_PRIVATE_PROBLEM ) {
			// Device has a problem reported by the driver.
		}
		else if (ulDN_status & DN_STARTED) {
			// Driver is running.
		}
		else {
			// Device is currently stopped.
		}
		return true;
	}

	bool DumpDeviceStack()
	{
		CMultiSzString Filters;
		LPCTSTR pszFilter;
		SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
		//
		// we need machine information
		//
		devInfoListDetail.cbSize = sizeof(devInfoListDetail);
		if ( !SetupDiGetDeviceInfoListDetail(m_hDevInfo,&devInfoListDetail))
		{
			return false;
		}

		//
		// we need device setup class, we can use the GUID in DevInfo
		// note that this GUID is a snapshot, but works fine
		// if DevInfo isn't old
		//

		//
		// class upper/lower filters are in class registry
		//
		HKEY hClassKey = SetupDiOpenClassRegKeyEx(
			&m_DevInfoData.ClassGuid,
			KEY_READ,
			DIOCR_INSTALLER,
			devInfoListDetail.RemoteMachineName,
			NULL );

		if ( hClassKey != INVALID_HANDLE_VALUE ) {
			//
			// dump upper class filters if available
			//
			if ( Filters.GetRegistryValue( hClassKey, REGSTR_VAL_UPPERFILTERS ) )
			{
                LPCTSTR pszFilter = Filters.GetFirst();
				while ( pszFilter ) {
					TRACE( TEXT("\tClassUpperFilter: %s\n"), pszFilter );
					pszFilter = Filters.GetNext();
				}
			}
		}

		if ( Filters.GetDeviceRegistryProperty( m_hDevInfo, &m_DevInfoData, SPDRP_UPPERFILTERS ) )
		{
            pszFilter = Filters.GetFirst();
			while ( pszFilter ) {
				TRACE( TEXT("\tDeviceUpperFilter: %s\n"), pszFilter );
				pszFilter = Filters.GetNext();
			}
		}

		tstring strService;
		GetStringProperty(SPDRP_SERVICE,strService);
		TRACE( TEXT("\tService: %s\n"), strService.c_str() );
		
		if ( hClassKey != INVALID_HANDLE_VALUE ) {
			//
			// dump lower class filters if available
			//
			if ( Filters.GetRegistryValue(hClassKey,REGSTR_VAL_LOWERFILTERS) )
			{
                pszFilter = Filters.GetFirst();
				while ( pszFilter ) {
					TRACE( TEXT("\tClassLowerFilter: %s\n"), pszFilter );
					pszFilter = Filters.GetNext();
				}
			}
			RegCloseKey(hClassKey);
		}

		if ( Filters.GetDeviceRegistryProperty(m_hDevInfo,&m_DevInfoData,SPDRP_LOWERFILTERS) )
		{
            pszFilter = Filters.GetFirst();
			while ( pszFilter ) {
				TRACE( TEXT("\tDeviceLowerFilter: %s\n"), pszFilter );
				pszFilter = Filters.GetNext();
			}
		}

		return true;
	}

	bool DumpDeviceDriverNodes()
	{
		bool bSuccess = false;

		SP_DEVINSTALL_PARAMS deviceInstallParams;
		ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
		deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

		if (!SetupDiGetDeviceInstallParams( m_hDevInfo, &m_DevInfoData, &deviceInstallParams ))
		{
			return false;
		}

		//
		// Set the flags that tell SetupDiBuildDriverInfoList to allow excluded drivers.
		//
		deviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
		
		if (!SetupDiSetDeviceInstallParams( m_hDevInfo, &m_DevInfoData, &deviceInstallParams ))
		{
			return false;
		}

		//
		// Now build a class driver list.
		//
		if (!SetupDiBuildDriverInfoList(m_hDevInfo, &m_DevInfoData, SPDIT_COMPATDRIVER))
		{
			goto final2;
		}

		SP_DRVINFO_DATA driverInfoData;
		ZeroMemory(&driverInfoData, sizeof(driverInfoData));
		driverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

		//
		// Enumerate all of the drivernodes.
		//
		DWORD dwIndex = 0;
		while ( SetupDiEnumDriverInfo( m_hDevInfo, &m_DevInfoData, SPDIT_COMPATDRIVER, dwIndex, &driverInfoData) )
		{
			bSuccess = true;

			TRACE( TEXT("\tDriverNode #%d:\n"), dwIndex );
			
			//
			// get useful driver information
			//
			SP_DRVINFO_DETAIL_DATA driverInfoDetail;
			driverInfoDetail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
			
			if (SetupDiGetDriverInfoDetail(m_hDevInfo,&m_DevInfoData,&driverInfoData,&driverInfoDetail,sizeof(SP_DRVINFO_DETAIL_DATA),NULL) ||
				GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				TRACE( TEXT("\t\tInf file is %s\n"), driverInfoDetail.InfFileName );
				TRACE( TEXT("\t\tInf section is %s\n"), driverInfoDetail.SectionName );
			}

			TRACE( TEXT("\t\tDriver description is %s\n"), driverInfoData.Description );
			TRACE( TEXT("\t\tManufacturer name is %s\n"), driverInfoData.MfgName );
			TRACE( TEXT("\t\tProvider name is %s\n"), driverInfoData.ProviderName );

#if !USE_SP_DRVINFO_DATA_V1 && !(_SETUPAPI_VER < 0x0500)  // use version 1 driver info data structure
			SYSTEMTIME SystemTime;
			TCHAR Buffer[MAX_PATH];
			if (FileTimeToSystemTime(&driverInfoData.DriverDate, &SystemTime)) {
				if (GetDateFormat(LOCALE_USER_DEFAULT,
					DATE_SHORTDATE,
					&SystemTime,
					NULL,
					Buffer,
					sizeof(Buffer)/sizeof(TCHAR) ) != 0)
				{
					TRACE( TEXT("\t\tDriver date is %s\n"), Buffer );
				}
			}

			ULARGE_INTEGER Version;
			Version.QuadPart = driverInfoData.DriverVersion;
			TRACE( TEXT("\t\tDriver version is %d.%d.%d.%d\n"),
				HIWORD(Version.HighPart),
				LOWORD(Version.HighPart),
				HIWORD(Version.LowPart),
				LOWORD(Version.LowPart) );
#endif

			SP_DRVINSTALL_PARAMS driverInstallParams;
			driverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);

			if (SetupDiGetDriverInstallParams(m_hDevInfo,&m_DevInfoData,&driverInfoData,&driverInstallParams))
			{
				TRACE( TEXT("\t\tDriver node rank is %d\n"), driverInstallParams.Rank );
				TRACE( TEXT("\t\tDriver node flags is 0x%08X\n"), driverInstallParams.Flags );

				//
				// Interesting flags to dump
				//
				if (driverInstallParams.Flags & DNF_OLD_INET_DRIVER) {
					TRACE( TEXT("\t\t\tInf came from the Internet\n") );
				}
				if (driverInstallParams.Flags & DNF_BAD_DRIVER) {
					TRACE( TEXT("\t\t\tDriver node is marked as BAD\n") );
				}

#if defined(DNF_INF_IS_SIGNED)
				//
				// DNF_INF_IS_SIGNED is only available since WinXP
				//
				if (driverInstallParams.Flags & DNF_INF_IS_SIGNED) {
					TRACE( TEXT("\t\t\tInf is digitally signed\n") );
				}
#endif

#if defined(DNF_OEM_F6_INF)
				//
				// DNF_OEM_F6_INF is only available since WinXP
				//
				if (driverInstallParams.Flags & DNF_OEM_F6_INF) {
					TRACE( TEXT("\t\t\tInf was installed using F6 during textmode setup\n") );
				}
#endif

#if defined(DNF_BASIC_DRIVER)
				//
				// DNF_BASIC_DRIVER is only available since WinXP
				//
				if (driverInstallParams.Flags & DNF_BASIC_DRIVER) {
					TRACE( TEXT("\t\t\tDriver provides basic functionality if no other signed driver exists.\n") );
				}
#endif
			}
			dwIndex++;
		}

		SetupDiDestroyDriverInfoList(m_hDevInfo,&m_DevInfoData,SPDIT_COMPATDRIVER);

final2:

		if (!bSuccess)
		{
			TRACE( TEXT("\t\tNo DriverNodes found for device.\n") );
		}
		return bSuccess;
	}

	bool DumpDeviceHwIds()
	{
		bool bDisplayed = false;
		CMultiSzString IdArray;

		if ( IdArray.GetDeviceRegistryProperty(m_hDevInfo,&m_DevInfoData,SPDRP_HARDWAREID) ) {
			bDisplayed = true;
			TRACE( TEXT("\tHardware ID's:\n") );
			LPCTSTR psz = IdArray.GetFirst();
			while ( psz ) {
				TRACE( TEXT("\t\t%s\n"), psz );
				psz = IdArray.GetNext();
			}
		}

		if ( IdArray.GetDeviceRegistryProperty(m_hDevInfo,&m_DevInfoData,SPDRP_COMPATIBLEIDS) ) {
			bDisplayed = true;
			TRACE( TEXT("\tCompatible ID's:\n") );
			LPCTSTR psz = IdArray.GetFirst();
			while ( psz ) {
				TRACE( TEXT("\t\t%s\n"), psz );
				psz = IdArray.GetNext();
			}
		}

		if (!bDisplayed) {
			TRACE( TEXT("\tNo hardware/compatible ID's available for this device.\n") );
		}

		return true;
	}

	bool GetDeviceResources( ULONG& status, ULONG& problem, bool& haveConfig, std::vector<RESOURCE_DESCRIPTOR>& list ) const
	{
		list.clear();

		status = 0;
		problem = 0;
		haveConfig = false;

		//
		// see what state the device is in
		//
		SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
		devInfoListDetail.cbSize = sizeof(devInfoListDetail);
		if ((!SetupDiGetDeviceInfoListDetail(m_hDevInfo,&devInfoListDetail)) ||
			(CM_Get_DevNode_Status_Ex(&status,&problem,m_DevInfoData.DevInst,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS))
		{
			return false;
		}

		//
		// see if the device is running and what resources it might be using
		//
		LOG_CONF config = 0;

		if(!(status & DN_HAS_PROBLEM)) {
			//
			// If this device is running, does this devinst have a ALLOC log config?
			//
			if (CM_Get_First_Log_Conf_Ex(&config,
										m_DevInfoData.DevInst,
										ALLOC_LOG_CONF,
										devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS)
			{
				haveConfig = true;
			}
		}

		if (!haveConfig) {
			//
			// If no config so far, does it have a FORCED log config?
			// (note that technically these resources might be used by another device
			// but is useful info to show)
			//
			if (CM_Get_First_Log_Conf_Ex(&config,
										m_DevInfoData.DevInst,
										FORCED_LOG_CONF,
										devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS)
			{
				haveConfig = true;
			}
		}

		if (!haveConfig) {
			//
			// if there's a hardware-disabled problem, boot-config isn't valid
			// otherwise use this if we don't have anything else
			//
			if(!(status & DN_HAS_PROBLEM) || (problem != CM_PROB_HARDWARE_DISABLED)) {
				//
				// Does it have a BOOT log config?
				//
				if (CM_Get_First_Log_Conf_Ex(&config,
											m_DevInfoData.DevInst,
											BOOT_LOG_CONF,
											devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS)
				{
					haveConfig = true;
				}
			}
		}
		
		if (!haveConfig) {
			//
			// if we don't have any configuration, display an apropriate message
			//
			//TRACE( (status & DN_STARTED)
			//	? TEXT("\tDevice is not using any resources.\n")
			//	: TEXT("\tDevice has no resources reserved.\n" ) );
			return true;
		}

		//TRACE( (status & DN_STARTED)
		//	? TEXT("\tDevice is currently using the following resources:\n")
		//	: TEXT("\tDevice has the following resources reserved:\n") );

		RES_DES prevResDes = (RES_DES)config;
		RES_DES resDes = 0;
		RESOURCEID resId = 0;
		ULONG dataSize;

		while (CM_Get_Next_Res_Des_Ex(&resDes,prevResDes,ResType_All,&resId,0,devInfoListDetail.RemoteMachineHandle)==CR_SUCCESS)
		{
			CM_Free_Res_Des_Handle(prevResDes);
			
			prevResDes = resDes;
			if (CM_Get_Res_Des_Data_Size_Ex(&dataSize,resDes,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS || dataSize == 0) {
				continue;
			}

			RESOURCE_DESCRIPTOR rd;
			rd.resID = resId;
			rd.resData.resize(dataSize);

			if (CM_Get_Res_Des_Data_Ex(resDes,&rd.resData[0],dataSize,0,devInfoListDetail.RemoteMachineHandle)==CR_SUCCESS) {
				list.push_back(rd);
			}
		}
		
		CM_Free_Res_Des_Handle(prevResDes);

		return true;
	}

	bool DumpDeviceResources()
	{
		ULONG status = 0;
		ULONG problem = 0;
		//
		// see what state the device is in
		//
		SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
		devInfoListDetail.cbSize = sizeof(devInfoListDetail);
		if ((!SetupDiGetDeviceInfoListDetail(m_hDevInfo,&devInfoListDetail)) ||
			(CM_Get_DevNode_Status_Ex(&status,&problem,m_DevInfoData.DevInst,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS))
		{
			return false;
		}

		//
		// see if the device is running and what resources it might be using
		//
		LOG_CONF config = 0;
		bool haveConfig = false;

		if(!(status & DN_HAS_PROBLEM)) {
			//
			// If this device is running, does this devinst have a ALLOC log config?
			//
			if (CM_Get_First_Log_Conf_Ex(&config,
										m_DevInfoData.DevInst,
										ALLOC_LOG_CONF,
										devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS)
			{
				haveConfig = true;
			}
		}

		if (!haveConfig) {
			//
			// If no config so far, does it have a FORCED log config?
			// (note that technically these resources might be used by another device
			// but is useful info to show)
			//
			if (CM_Get_First_Log_Conf_Ex(&config,
										m_DevInfoData.DevInst,
										FORCED_LOG_CONF,
										devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS)
			{
				haveConfig = true;
			}
		}

		if (!haveConfig) {
			//
			// if there's a hardware-disabled problem, boot-config isn't valid
			// otherwise use this if we don't have anything else
			//
			if(!(status & DN_HAS_PROBLEM) || (problem != CM_PROB_HARDWARE_DISABLED)) {
				//
				// Does it have a BOOT log config?
				//
				if (CM_Get_First_Log_Conf_Ex(&config,
											m_DevInfoData.DevInst,
											BOOT_LOG_CONF,
											devInfoListDetail.RemoteMachineHandle) == CR_SUCCESS)
				{
					haveConfig = true;
				}
			}
		}
		
		if (!haveConfig) {
			//
			// if we don't have any configuration, display an apropriate message
			//
			TRACE( (status & DN_STARTED)
				? TEXT("\tDevice is not using any resources.\n")
				: TEXT("\tDevice has no resources reserved.\n" ) );
			return true;
		}

		TRACE( (status & DN_STARTED)
			? TEXT("\tDevice is currently using the following resources:\n")
			: TEXT("\tDevice has the following resources reserved:\n") );
		
		//
		// dump resources
		//
		DumpDeviceResourcesOfType(m_DevInfoData.DevInst,devInfoListDetail.RemoteMachineHandle,config,ResType_Mem);
		DumpDeviceResourcesOfType(m_DevInfoData.DevInst,devInfoListDetail.RemoteMachineHandle,config,ResType_IO);
		DumpDeviceResourcesOfType(m_DevInfoData.DevInst,devInfoListDetail.RemoteMachineHandle,config,ResType_DMA);
		DumpDeviceResourcesOfType(m_DevInfoData.DevInst,devInfoListDetail.RemoteMachineHandle,config,ResType_IRQ);

		//
		// release handle
		//
		CM_Free_Log_Conf_Handle(config);
		
		return true;
	}

	bool DumpDeviceResourcesOfType(DEVINST DevInst,HMACHINE MachineHandle,LOG_CONF Config,RESOURCEID ReqResId)
	{
		RES_DES prevResDes = (RES_DES)Config;
		RES_DES resDes = 0;
		RESOURCEID resId = ReqResId;
		ULONG dataSize;
		PBYTE resDesData;

		while (CM_Get_Next_Res_Des_Ex(&resDes,prevResDes,ReqResId,&resId,0,MachineHandle)==CR_SUCCESS) {
			if (prevResDes != Config) {
				CM_Free_Res_Des_Handle(prevResDes);
			}
			prevResDes = resDes;
			if (CM_Get_Res_Des_Data_Size_Ex(&dataSize,resDes,0,MachineHandle)!=CR_SUCCESS) {
				continue;
			}
			resDesData = new BYTE[dataSize];
			if (!resDesData) {
				continue;
			}
			if (CM_Get_Res_Des_Data_Ex(resDes,resDesData,dataSize,0,MachineHandle)!=CR_SUCCESS) {
				delete [] resDesData;
				continue;
			}
			switch(resId) {
			case ResType_Mem:
				{
					PMEM_RESOURCE pMemData = (PMEM_RESOURCE)resDesData;
					if (pMemData->MEM_Header.MD_Alloc_End-pMemData->MEM_Header.MD_Alloc_Base+1) {
						TRACE(TEXT("\t\tMEM : %08I64x-%08I64x\n"),pMemData->MEM_Header.MD_Alloc_Base,pMemData->MEM_Header.MD_Alloc_End);
					}
					break;
				}
			case ResType_IO:
				{
					PIO_RESOURCE pIoData = (PIO_RESOURCE)resDesData;
					if(pIoData->IO_Header.IOD_Alloc_End-pIoData->IO_Header.IOD_Alloc_Base+1) {
						TRACE(TEXT("\t\tIO  : %04I64x-%04I64x\n"),pIoData->IO_Header.IOD_Alloc_Base,pIoData->IO_Header.IOD_Alloc_End);
					}
					break;
				}
			case ResType_DMA:
				{
					PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)resDesData;
					TRACE(TEXT("\t\tDMA : %u\n"),pDmaData->DMA_Header.DD_Alloc_Chan);
					break;
				}
			case ResType_IRQ:
				{
					PIRQ_RESOURCE  pIrqData = (PIRQ_RESOURCE)resDesData;
					TRACE(TEXT("\t\tIRQ : %u\n"),pIrqData->IRQ_Header.IRQD_Alloc_Num);
					break;
				}
			}
			delete [] resDesData;
		}
		
		if (prevResDes != Config) {
			CM_Free_Res_Des_Handle(prevResDes);
		}
		
		return true;
	}

};

class CDeviceSet
{
	HDEVINFO				m_hDevInfo;
	std::vector<CDevice>	m_vDevices;

public:
	CDeviceSet() : m_hDevInfo(INVALID_HANDLE_VALUE)
	{
	}

	~CDeviceSet()
	{
		Clear();
	}

	void Clear()
	{
		m_vDevices.clear();

		if ( m_hDevInfo != INVALID_HANDLE_VALUE )
		{
			SetupDiDestroyDeviceInfoList(m_hDevInfo);
		}
	}

	bool Init(LPCTSTR pszMachine)
	{
		Clear();
		m_hDevInfo = SetupDiCreateDeviceInfoListEx( NULL, NULL, pszMachine,	NULL );
		return m_hDevInfo != INVALID_HANDLE_VALUE;
	}

	bool AddDevice(LPCTSTR pszDeviceInstanceId,LPCTSTR pszDevicePath=NULL)
	{
		if ( m_hDevInfo == INVALID_HANDLE_VALUE )
			return false;

		SP_DEVINFO_DATA DevInfoData;
		DevInfoData.cbSize = sizeof(DevInfoData);
		if ( SetupDiOpenDeviceInfo( m_hDevInfo, pszDeviceInstanceId, NULL, 0, &DevInfoData ) )
		{
			int nDevIndex = (int)m_vDevices.size();
            m_vDevices.push_back( CDevice(m_hDevInfo,nDevIndex,DevInfoData,pszDeviceInstanceId,pszDevicePath) );
			//m_vDevices.back().DumpDeviceStack();
			//m_vDevices.back().DumpDeviceDriverNodes();
			//m_vDevices.back().DumpDeviceHwIds();
			//m_vDevices.back().DumpDeviceResources();
			return true;
		}
		return false;
	}

	CDevice& GetDevice(int index)
	{
		return m_vDevices[index];
	}

	const CDevice& GetDevice(int index) const
	{
		return m_vDevices[index];
	}

	DWORD GetDeviceCount() const
	{
		return (DWORD)m_vDevices.size();
	}
};

class CDevMgr
{
protected:
	LPTSTR			m_pszMachine;
	ClassList		m_Classes;

	//LPTSTR GetDeviceStringProperty( HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD dwProp );
	//LPTSTR GetDeviceDescription( HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData );

	//BOOL DumpDeviceWithInfo( HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, LPCTSTR pszInfo );
	//BOOL DumpDevice( HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData );

public:
	static BOOL Reboot();

	int Rescan();
	int InstallDriver( LPCTSTR pszInfPath, LPCTSTR pszHwid, bool& bNeedReboot );
	int UpdateDriver( LPCTSTR pszInfPath, LPCTSTR pszHwid, bool& bNeedReboot );
	int UpdateDriverNI( LPCTSTR pszInfPath, LPCTSTR pszHwid, bool& bNeedReboot );

	CDevMgr( LPCTSTR pszMachine = NULL );
	~CDevMgr(void);

	const ClassList& GetClasses();

	int EnumerateDevices( CDeviceSet& DeviceSet, DWORD dwFlags, LPCTSTR pszClassName, LPCTSTR pszRule1, ... ); // MUST PUT A 'NULL' in the end.
	int EnumerateDevices( CDeviceSet& DeviceSet, DWORD dwFlags, LPCTSTR pszClassName, int nNumIdRule, LPCTSTR apszRule[] );
	int EnumerateDevices( CDeviceSet& DeviceSet, DWORD dwFlags, const GUID* pInterfaceGuid );

};

typedef std::vector<BYTE> USB_DESCRIPTOR;

bool GetUsbDeviceConfigDescriptor( HANDLE hHubDev, ULONG ulConnectionIndex, UCHAR ucDescriptorIndex, USB_DESCRIPTOR* pUsbDesc );


typedef DWORD (WINAPI*  PCBENUMUSBHOST)( LPCTSTR pszHostName, LPCTSTR pszHubDevicePath, LPVOID lpContext );
typedef PCBENUMUSBHOST LPCBENUMUSBHOST;

typedef DWORD (WINAPI* PCBENUMUSBHUB)( HANDLE hUsbHub, PUSB_NODE_INFORMATION pUsbNodeInfo, LPVOID pContext );
typedef PCBENUMUSBHUB LPCBENUMUSBHUB;

typedef DWORD (WINAPI* PCBENUMUSBHUBPORT)( LPCTSTR pszHubPortInfo, LPCTSTR pszDriverKeyName, LPCTSTR pszExtHubDevicePath,
										  PUSB_NODE_CONNECTION_INFORMATION_EX pUsbNodeConnectInfoEx,
										  LPVOID pContext );
typedef PCBENUMUSBHUBPORT LPCBENUMUSBHUBPORT;

bool EnumerateUsbHostControllers( LPCBENUMUSBHOST pCallback, LPVOID lpContext );
bool EnumerateUsbHub( LPCTSTR pszHubDevicePath, LPCBENUMUSBHUB pCallback, LPVOID pContext );
bool EnumerateUsbHubPorts( HANDLE hUsbHub, LPCBENUMUSBHUBPORT pCallback, LPVOID lpContext );









