#pragma once

#include <vector>

struct DEVICELISTENTRY
{
	std::basic_string<TCHAR>	InstanceId;
	std::basic_string<TCHAR>	DevicePath;
	std::basic_string<TCHAR>	FriendlyName;
};

class CDeviceList
{
	std::vector<DEVICELISTENTRY> m_List;

public:
	CDeviceList(void);
	~CDeviceList(void);

	bool Create( const GUID& guidInterface );

	DWORD DeviceCount() const
	{
		return (DWORD)m_List.size();
	}

	const DEVICELISTENTRY* GetDevice( DWORD index ) const
	{
		if ( index < m_List.size() )
			return &m_List[index];

		return NULL;		
	}

};
