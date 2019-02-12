#pragma once

class CLVLogFile
{
public:
	static CLVLogFile sm_Log;

	void Init(LPCTSTR pszFileName);
	void OutputString(LPCTSTR lpszFormat, ... );

private:
	FILE*	m_fhLog;

	CLVLogFile();
	~CLVLogFile();

};

#define LVT_DebugInit(filename) CLVLogFile::sm_Log.Init(filename)

#define LVT_DbgPrint( _x_ ) CLVLogFile::sm_Log.OutputString _x_
