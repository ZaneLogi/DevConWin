#include "StdAfx.h"
#include "DeviceList.h"
#include <setupapi.h>

CDeviceList::CDeviceList(void)
{
}

CDeviceList::~CDeviceList(void)
{
}

bool CDeviceList::Create( const GUID& guidInterface )
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(
		&guidInterface,
		NULL,
		NULL,
		DIGCF_PRESENT|DIGCF_INTERFACEDEVICE);
	if ( hDevInfo == INVALID_HANDLE_VALUE )
		return false;

	SP_INTERFACE_DEVICE_DATA idd;
	idd.cbSize = sizeof(idd);

	for(DWORD devIndex = 0;
		SetupDiEnumDeviceInterfaces( hDevInfo, NULL, &guidInterface, devIndex, &idd);
		++devIndex)
	{
		TCHAR tmp[256];
		DEVICELISTENTRY e;

		//
		// Device Path
		//
		DWORD needed = 0;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &idd, NULL, 0, &needed, NULL);
		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
			continue;

		PSP_INTERFACE_DEVICE_DETAIL_DATA detail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(needed);
		detail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
		
		SP_DEVINFO_DATA did = {sizeof(SP_DEVINFO_DATA)};
		if ( !SetupDiGetDeviceInterfaceDetail(hDevInfo, &idd, detail, needed, NULL, &did) )
		{
			free( (PVOID)detail );
			continue;
		}
		else
		{
			e.DevicePath = detail->DevicePath;
			free((PVOID)detail);
		}

		//
		// Device Instance ID
		//
		if ( SetupDiGetDeviceInstanceId( hDevInfo, &did, tmp, sizeof(tmp), NULL ) )
		{
			e.InstanceId = tmp;		
		}

		//
		// Friendly Name
		//
		if (SetupDiGetDeviceRegistryProperty(hDevInfo, &did, SPDRP_FRIENDLYNAME, NULL, (PBYTE) tmp, sizeof(tmp), NULL))
			e.FriendlyName = tmp;
		else if (SetupDiGetDeviceRegistryProperty(hDevInfo, &did, SPDRP_DEVICEDESC, NULL, (PBYTE) tmp, sizeof(tmp), NULL))
			e.FriendlyName = tmp;

		m_List.push_back(e);
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	
	return true;
}
