// DevConWin.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CDevConWinApp:
// See DevConWin.cpp for the implementation of this class
//

class CDevConWinApp : public CWinApp
{
public:
	CDevConWinApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CDevConWinApp theApp;