#pragma once

// Last Modified Date: 11/05/2007

#include <setupapi.h>
#include <tchar.h>
#include <assert.h>

class CMultiSzString
{
	LPTSTR		m_pData;
	DWORD		m_dwBufferSize;
	DWORD		m_dwNumChars;
	const int	m_nAlignedBytes;

	mutable LPCTSTR	m_pCur;

	void Clear()
	{
		if ( m_pData )
			delete [] m_pData;
		m_pData  = NULL;
		m_dwBufferSize = 0;
		m_dwNumChars = 0;
		m_pCur   = NULL;
	}

	DWORD ValidateBufferSize( DWORD dw )
	{
		return ( dw + ( m_nAlignedBytes-1) ) & ~(m_nAlignedBytes-1);
	}

public:
	CMultiSzString(void) : m_pData(NULL), m_dwBufferSize(0), m_dwNumChars(0), m_nAlignedBytes(256), m_pCur(NULL)
	{
		assert( (m_nAlignedBytes & ( m_nAlignedBytes - 1 )) == 0 ); // must be power of 2
	}

	CMultiSzString( LPCTSTR psz ) : m_dwBufferSize(0), m_dwNumChars(0), m_nAlignedBytes(256), m_pCur(NULL)
	{
		assert( (m_nAlignedBytes & ( m_nAlignedBytes - 1 )) == 0 ); // must be power of 2

		LPCTSTR p = psz;
		do
		{
            m_dwNumChars += (DWORD)_tcsclen(p) + 1;
			p = p + m_dwNumChars;
		} while (*p);

		m_dwNumChars++; // add the last terminator '\0'

		m_dwBufferSize = ValidateBufferSize( m_dwNumChars * sizeof(TCHAR) );

		m_pData = new TCHAR[m_dwBufferSize/sizeof(TCHAR)];
		if ( m_pData )
			memcpy( m_pData, psz, m_dwNumChars * sizeof(TCHAR) );
	}

	~CMultiSzString(void)
	{
		Clear();
	}

	CMultiSzString( const CMultiSzString& x ) : m_nAlignedBytes(x.m_nAlignedBytes)
	{
		m_pCur = NULL;
		if ( x.m_pData == NULL || x.m_dwBufferSize == 0 )
		{
			m_pData = NULL;
			m_dwBufferSize = 0;
			m_dwNumChars = 0;
			m_pCur = NULL;
		}
		else {
			m_pData = new TCHAR[x.m_dwBufferSize/sizeof(TCHAR)];
			if ( m_pData ) {
				memcpy( m_pData, x.m_pData, x.m_dwBufferSize );
				m_dwBufferSize = x.m_dwBufferSize;
				m_dwNumChars = x.m_dwNumChars;
			}
			else {
				// failed to allocate memory
				m_dwBufferSize = 0;
				m_dwNumChars = 0;
			}
		}
	}

	CMultiSzString& operator = ( const CMultiSzString& x )
	{
		m_pCur = NULL;
		if ( x.m_pData == NULL || x.m_dwBufferSize == 0 )
		{
			Clear();
		}
		else
		{
			if ( m_pData && m_dwBufferSize >= x.m_dwBufferSize )
			{
				memcpy( m_pData, x.m_pData, x.m_dwBufferSize );
				m_dwBufferSize = x.m_dwBufferSize;
				m_dwNumChars = x.m_dwNumChars;
			}
			else
			{
				Clear();
				m_pData = new TCHAR[x.m_dwBufferSize/sizeof(TCHAR)];
				if ( m_pData )
				{
					memcpy( m_pData, x.m_pData, x.m_dwBufferSize );
					m_dwBufferSize = x.m_dwBufferSize;
					m_dwNumChars = x.m_dwNumChars;
				}
				else
				{
					// failed to allocate memory
					m_dwBufferSize = 0;
					m_dwNumChars = 0;
				}
			}
		}
		return *this;
	}

	LPCTSTR GetFirst() const
	{
		if ( m_pData ) {
			m_pCur = m_pData;
			return GetNext();
		}
		else {
			m_pCur = NULL;
			return NULL;
		}
	}

	LPCTSTR GetNext() const
	{
		LPCTSTR p = m_pCur;
		if ( m_pCur ) {
			if ( *m_pCur != 0 )
				m_pCur = m_pCur + _tcsclen(m_pCur) + 1;
			else
				p = NULL;
		}
		return p;
	}

	bool Append( LPCTSTR pNewString )
	{
		int nNewStringLength = (int)_tcsclen(pNewString) + 1;
		DWORD dwNewBufferSize = ValidateBufferSize( ( m_dwNumChars + 1 + nNewStringLength ) * sizeof(TCHAR) );
		if ( m_dwBufferSize < dwNewBufferSize )
		{
			TCHAR* pNewData = new TCHAR[dwNewBufferSize/sizeof(TCHAR)];
			if ( !pNewData )
				return false;

			if ( m_pData )
			{
				memcpy( pNewData, m_pData, m_dwNumChars * sizeof(TCHAR) );
				delete m_pData;
			}

			m_pData = pNewData;
			m_dwBufferSize = dwNewBufferSize;
		}

		if ( m_dwNumChars > 0 )
		{
			memcpy( &m_pData[m_dwNumChars-1], pNewString, nNewStringLength * sizeof(TCHAR) );
			m_dwNumChars += nNewStringLength;
		}
		else
		{ // new buffer
			memcpy( m_pData, pNewString, nNewStringLength * sizeof(TCHAR) );
			m_dwNumChars = nNewStringLength + 1;
		}

		m_pData[m_dwNumChars-1] = _T('\0');

		return true;
	}

	bool Remove( LPCTSTR pTargetString, bool bCaseSensitive = false )
	{
		LPCTSTR p = GetFirst();
		while ( p )
		{
			if ( bCaseSensitive )
			{
				if ( _tcscmp( p, pTargetString ) == 0 )
					break;
			}
			else
			{
				if ( _tcsicmp( p, pTargetString ) == 0 )
					break;
			}

			p = GetNext();
		}

		if ( p )
		{
			int nTargetStringLength = (int)_tcsclen(pTargetString) + 1;
			int nReplacePos = (int)( p - m_pData );
			int nReplaceWith = nReplacePos + nTargetStringLength;
			int nReplaceLen = m_dwNumChars - nReplacePos - nTargetStringLength;

			memcpy( m_pData + nReplacePos, m_pData + nReplaceWith, nReplaceLen * sizeof(TCHAR) );
			m_dwNumChars -= nTargetStringLength;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool GetDeviceRegistryProperty( HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD dwProp, PDWORD pdwErrorCode = NULL )
	{
		DWORD	dwDataType;
		DWORD	dwReqSize;
		DWORD	dwErrCode;

		Clear();

		while ( !SetupDiGetDeviceRegistryProperty(
			hDevInfo,
			pDevInfoData,
			dwProp,
			&dwDataType,
			(LPBYTE)m_pData,
			m_dwBufferSize,
			&dwReqSize ) )
		{
			dwErrCode = GetLastError();
			if ( dwErrCode != ERROR_INSUFFICIENT_BUFFER || dwDataType != REG_MULTI_SZ || dwReqSize == 0 )
			{
				if ( pdwErrorCode )
					*pdwErrorCode = dwErrCode;

				return false;
			}

			assert(m_pData==NULL);
			m_dwBufferSize = ValidateBufferSize(dwReqSize);
			m_pData = new TCHAR[m_dwBufferSize/sizeof(TCHAR)];
			if ( !m_pData )
			{
				if ( pdwErrorCode )
					*pdwErrorCode = ERROR_OUTOFMEMORY;

				return false;
			}

			m_dwNumChars = dwReqSize/sizeof(TCHAR);
		}

		if ( pdwErrorCode )
			*pdwErrorCode = ERROR_SUCCESS;

		return true;
	}

	bool SetDeviceRegistryProperty( HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD dwProp, PDWORD pdwErrorCode = NULL )
	{
		if ( !m_pData || m_dwNumChars == 0 )
		{
			if ( pdwErrorCode )
				*pdwErrorCode = ERROR_INVALID_DATA;

			return false;
		}
		
		BOOL b = SetupDiSetDeviceRegistryProperty(
			hDevInfo,
			pDevInfoData,
			dwProp,
			(BYTE*)m_pData,
			m_dwNumChars * sizeof(TCHAR) );

		if ( pdwErrorCode )
		{
			*pdwErrorCode = GetLastError();
		}

		return b == TRUE;
	}

	bool GetRegistryValue( HKEY hKey, LPCTSTR pszValName )
	{
		DWORD	dwSize;
		DWORD	dwDataType;

		Clear();

		if ( NO_ERROR != RegQueryValueEx( hKey, pszValName, NULL, &dwDataType, NULL, &dwSize ) )
		{
			return false;
		}

		if ( dwDataType != REG_MULTI_SZ )
		{
			return false;
		}

		m_dwBufferSize = ValidateBufferSize(dwSize);
		m_pData = new TCHAR[m_dwBufferSize/sizeof(TCHAR)];
		if (!m_pData)
		{
			return false;
		}

		if ( NO_ERROR != RegQueryValueEx( hKey, pszValName, NULL, &dwDataType, (LPBYTE)m_pData, &dwSize ) )
		{
			return false;
		}

		return true;
	}

	bool SetRegistryValue( HKEY hKey, LPCTSTR pszValName )
	{
		if ( !m_pData || m_dwNumChars == 0 )
			return false;

		LONG lRet = RegSetValueEx(
			hKey,
			pszValName,
			NULL,
			REG_MULTI_SZ,
			(BYTE*)m_pData,
			m_dwNumChars * sizeof(TCHAR) );

		return lRet == ERROR_SUCCESS;
	}
};
