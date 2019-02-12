#include "StdAfx.h"
#include "lvlogfile.h"

CLVLogFile CLVLogFile::sm_Log;

CLVLogFile::CLVLogFile()
{
	m_fhLog = NULL;
}

CLVLogFile::~CLVLogFile()
{
	if ( m_fhLog ) {
		fclose(m_fhLog);
		m_fhLog = NULL;
	}
}

void CLVLogFile::Init(LPCTSTR pszFileName)
{
	m_fhLog = _tfopen( pszFileName, _T("a+") );
}

void CLVLogFile::OutputString(LPCTSTR lpszFormat, ... )
{
	const int MAXSIZE = 512;
	TCHAR buf[MAXSIZE];
	va_list args;
	va_start(args, lpszFormat);
	int nSize = _vsntprintf( buf, MAXSIZE-1, lpszFormat, args );
	va_end(args);

	if ( nSize >= 0 && m_fhLog )
	{
        buf[nSize] = _T('\0');
		_fputts(buf, m_fhLog);
		fflush(m_fhLog);
	}

}
