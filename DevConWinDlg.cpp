// DevConWinDlg.cpp : implementation file
//
#include "stdafx.h"
#include "DevConWin.h"
#include "DevConWinDlg.h"
#include "dbt.h"

#include "FilterSettingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib,"Version.lib")

static TCHAR szVer[512];

LPCTSTR GetVersionString(HINSTANCE hInstance)
{
	TCHAR	szFullPath[512];
	DWORD	dwVerInfoSize;
	DWORD   dwHandle;
	LPSTR   lpstrVffInfo;
	LPVOID  lpVersion;
	UINT    uVersionLen;

	GetModuleFileName(hInstance, szFullPath, sizeof(szFullPath));
	dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwHandle);
	if (dwVerInfoSize) {
		VS_FIXEDFILEINFO vsVer;
		lpstrVffInfo = new char[dwVerInfoSize];
		GetFileVersionInfo (szFullPath, 0, dwVerInfoSize, lpstrVffInfo);
		//Query product version
		VerQueryValue (lpstrVffInfo, TEXT("\\"), &lpVersion, &uVersionLen);
		memcpy((LPVOID)&vsVer, lpVersion, uVersionLen);
		wsprintf ( szVer, _T("%d.%02d.%03d.%d"),
			HIWORD (vsVer.dwProductVersionMS),
			LOWORD (vsVer.dwProductVersionMS),
			HIWORD (vsVer.dwProductVersionLS),
			LOWORD (vsVer.dwProductVersionLS));
		delete lpstrVffInfo;
	}

	return szVer;
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog()
	{
		GetDlgItem(IDC_VERSION)->SetWindowText( GetVersionString(NULL) );
		return CDialog::OnInitDialog();
	}
// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


/*
Routine Description:
    Registers for notification of changes in the device interfaces for
    the specified interface class GUID.

Parameters:
    InterfaceClassGuid - The interface class GUID for the device
        interfaces.

    hDevNotify - Receives the device notification handle. On failure,
        this value is NULL.

Return Value:
    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.
*/
BOOL DoRegisterDeviceInterface
(
	HWND hWnd,
	GUID InterfaceClassGuid,
	HDEVNOTIFY *hDevNotify
)
{
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	TCHAR szMsg[80];

	ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
	NotificationFilter.dbcc_size =
		sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = InterfaceClassGuid;

	*hDevNotify = RegisterDeviceNotification( hWnd,
		&NotificationFilter,
		DEVICE_NOTIFY_WINDOW_HANDLE );

	if(!*hDevNotify)
	{
		wsprintf(szMsg, _T("RegisterDeviceNotification failed: %d\n"),
			GetLastError());
		MessageBox(hWnd, szMsg, _T("Registration"), MB_OK);
		return FALSE;
	}

	return TRUE;
}

/*------------------------------------------------------------------
   FirstDriveFromMask (unitmask)

   Description
     Finds the first valid drive letter from a mask of drive letters.
     The mask must be in the format bit 0 = A, bit 1 = B, bit 3 = C,
     etc. A valid drive letter is defined when the corresponding bit
     is set to 1.

   Returns the first drive letter that was found.
--------------------------------------------------------------------*/

char FirstDriveFromMask (ULONG unitmask)
{
   char i;

   for (i = 0; i < 26; ++i)
   {
      if (unitmask & 0x1)
         break;
      unitmask = unitmask >> 1;
   }

   return (i + 'A');
}

inline LPCTSTR CreateGuidString( LPTSTR pszBuf, const GUID& guid )
{
	wsprintf( pszBuf, _T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
		guid.Data1,
		guid.Data2,
		guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3], guid.Data4[4],
		guid.Data4[5], guid.Data4[6], guid.Data4[7] );
	return pszBuf;
}

inline HTREEITEM GetTreeItem( const CTreeCtrl& tc, HTREEITEM htiParent, int nIndex )
{
	HTREEITEM htiNext = tc.GetChildItem(htiParent);

	while ( nIndex-- > 0 && htiNext )
	{
		htiNext = tc.GetNextItem(htiNext, TVGN_NEXT);
	}

	return htiNext;
}

// CDevConWinDlg dialog



CDevConWinDlg::CDevConWinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDevConWinDlg::IDD, pParent)
	, m_bViewType(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDevConWinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_tcMgr);
	DDX_Control(pDX, IDC_CB_MSG, m_cbMsg);
	DDX_Radio(pDX, IDC_DEVICE_VIEW, m_bViewType);
}

BEGIN_MESSAGE_MAP(CDevConWinDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, OnTvnSelchangedTree1)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnToolTipNotify )
	ON_CBN_DROPDOWN(IDC_CB_MSG, OnCbnDropdownCbMsg)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_DEVICE_VIEW, IDC_USB_VIEW, OnBnClickedBtnViewType)
	ON_BN_CLICKED(IDC_BTN_ENABLE, OnBnClickedBtnEnable)
	ON_BN_CLICKED(IDC_BTN_DISABLE, OnBnClickedBtnDisable)
	ON_BN_CLICKED(IDC_BTN_FILTER, OnBnClickedBtnFilter)
	ON_BN_CLICKED(IDC_BTN_USB_DESCRIPTOR, OnBnClickedBtnDescriptors)
	ON_BN_CLICKED(IDC_BTN_REFRESH, OnBnClickedBtnRefresh)
END_MESSAGE_MAP()


// CDevConWinDlg message handlers

void CDevConWinDlg::ShowMsg(LPCTSTR lpszMsg,...)
{
	const int BUF_LEN = 511;
	int nBuf;
	TCHAR szBuffer[BUF_LEN+1];

	va_list args;
	va_start(args, lpszMsg);
	nBuf = _vsntprintf_s( szBuffer, BUF_LEN, lpszMsg, args );
	if ( nBuf < 0 ) {
		nBuf = BUF_LEN;
	}
	szBuffer[nBuf] = '\0';
	va_end(args);

	TCHAR achTime[64];
    GetTimeFormat( NULL, TIME_FORCE24HOURFORMAT, NULL, _T("hh':'mm':'ss"), achTime, sizeof(achTime) );

	TCHAR achBuf[MAX_PATH];
	wsprintf( achBuf, _T("%s) %s"), achTime, szBuffer );

	int nIdx = m_cbMsg.AddString( achBuf );
	m_cbMsg.SetCurSel(nIdx);
}

void CDevConWinDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}

void CDevConWinDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

VOID WINAPI DeviceTreeMsgCb( LPCTSTR lpszMsg, LPVOID lpContext )
{
	CDevConWinDlg* pDlg = (CDevConWinDlg*)lpContext;
	pDlg->ShowMsg( lpszMsg );
}

BOOL CDevConWinDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	ShowMsg( _T("Welcome...") );

	CString strCaption;
	GetWindowText(strCaption);
	strCaption += L" ";
	strCaption += GetVersionString(AfxGetApp()->m_hInstance);
	SetWindowText(strCaption);

	ShowMsg(strCaption);

	EnableToolTips();
	m_tip.Create(this);

	m_tcMgr.SetMsgCb(DeviceTreeMsgCb,this);
	OnBnClickedBtnRefresh();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CDevConWinDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	m_tip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CDevConWinDlg::OnToolTipNotify( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
{
	TOOLTIPTEXT*  pTTT  = (TOOLTIPTEXT *)pNMHDR;
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	UINT_PTR nID = pNMHDR->idFrom;
	if (pTTT->uFlags & TTF_IDISHWND) {
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
		if(nID) {
			CString s;
			if ( s.LoadString((int)nID) ) {
                pTTT->lpszText = MAKEINTRESOURCE(nID);
				pTTT->hinst = AfxGetResourceHandle();
			}
			else if ( nID == IDC_CB_MSG )
			{
				static CString strCbMsg;
				int nLen = m_cbMsg.GetWindowTextLength();
				LPTSTR psz = strCbMsg.GetBufferSetLength(nLen);
                m_cbMsg.GetWindowText( psz, nLen );
				pTTT->lpszText = psz;
				pTTT->hinst = NULL;
				strCbMsg.ReleaseBuffer();
			}
			else {
				GetDlgItem((int)nID)->GetWindowText( pTTT->szText, 80 );
			}
			return(TRUE);
		}
	}
	return(FALSE);
}

void CDevConWinDlg::OnCbnDropdownCbMsg()
{
	// Reset the dropped width
	int nNumEntries = m_cbMsg.GetCount();
    int nWidth = 0;
    CString str;

    CClientDC dc(this);
    int nSave = dc.SaveDC();
    dc.SelectObject(GetFont());

    int nScrollWidth = ::GetSystemMetrics(SM_CXVSCROLL);
    for (int i = 0; i < nNumEntries; i++)
    {
		m_cbMsg.GetLBText(i, str);
        int nLength = dc.GetTextExtent(str).cx + nScrollWidth;
        nWidth = max(nWidth, nLength);
    }

    // Add margin space to the calculations
    nWidth += dc.GetTextExtent(_T("0")).cx;

    dc.RestoreDC(nSave);
    m_cbMsg.SetDroppedWidth(nWidth);
}

void CDevConWinDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDevConWinDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDevConWinDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//
// CDeviceTree
//
CImageList CDeviceTree::sm_ilDeviceIcons;
CImageList CDeviceTree::sm_ilUsbViewIcons;

int CDeviceTree::AddSysImage( int index, CImageList& il )
{
	int cxSmIcon = GetSystemMetrics(SM_CXSMICON), cySmIcon = GetSystemMetrics(SM_CYSMICON);

	CDC dc;
	dc.CreateCompatibleDC(NULL);

	CBitmap bmp;
	BOOL b = bmp.CreateBitmap( cxSmIcon, cySmIcon, 1, 32, NULL );
	CBitmap* pOldBitmap = (CBitmap*)dc.SelectObject(&bmp);
	SetupDiDrawMiniIcon( dc.GetSafeHdc(), CRect(0,0,cxSmIcon,cySmIcon), index, DMI_USERECT );
	dc.SelectObject(pOldBitmap);

    return il.Add( &bmp, GetSysColor(COLOR_WINDOW) );
}

int CDeviceTree::PrepareImages( int nIndex )
{
	if ( sm_ilDeviceIcons.GetSafeHandle() == NULL )
	{
		CDevMgr dm;
		const ClassList& cl = dm.GetClasses();

		int cxSmIcon = GetSystemMetrics(SM_CXSMICON), cySmIcon = GetSystemMetrics(SM_CYSMICON);
		BOOL b = sm_ilDeviceIcons.Create( cxSmIcon, cySmIcon, ILC_COLOR32|ILC_MASK, 0, (int)cl.size() + 2 );

		HICON hIcon;

		hIcon = LoadIcon( NULL, IDI_INFORMATION );
		sm_ilDeviceIcons.Add(hIcon);

		AddSysImage( 0, sm_ilDeviceIcons );

		for ( unsigned i = 0; i < cl.size(); i++ )
		{
			AddSysImage(cl[i].miniIconIndex,sm_ilDeviceIcons);
		}
	}

	if ( sm_ilUsbViewIcons.GetSafeHandle() == NULL )
	{
		int cxSmIcon = GetSystemMetrics(SM_CXSMICON), cySmIcon = GetSystemMetrics(SM_CYSMICON);
		BOOL b = sm_ilUsbViewIcons.Create( cxSmIcon, cySmIcon, ILC_COLOR32|ILC_MASK, 0, 8 );

		HICON hIcon;
		hIcon = LoadIcon( NULL, IDI_INFORMATION ); // 0
		sm_ilUsbViewIcons.Add(hIcon);

		hIcon = AfxGetApp()->LoadIcon(IDI_MONITOR); // 1
		sm_ilUsbViewIcons.Add(hIcon);

		hIcon = AfxGetApp()->LoadIcon(IDI_USB); // 2
		sm_ilUsbViewIcons.Add(hIcon);

		hIcon = AfxGetApp()->LoadIcon(IDI_HUB); // 3
		sm_ilUsbViewIcons.Add(hIcon);

		hIcon = AfxGetApp()->LoadIcon(IDI_PORT); // 4
		sm_ilUsbViewIcons.Add(hIcon);

		hIcon = AfxGetApp()->LoadIcon(IDI_BANG); // 5
		sm_ilUsbViewIcons.Add(hIcon);
	}

	return (nIndex == 0) ? sm_ilDeviceIcons.GetImageCount() : sm_ilUsbViewIcons.GetImageCount();
}

IMPLEMENT_DYNAMIC(CDeviceTree, CCustomDrawTreeCtrl)

CDeviceTree::CDeviceTree() : m_pfnMsgCb(NULL), m_pMsgCbContext(NULL)
{
	m_acrDevStatus[DEVICE_STATUS_UNKNOWN]      = RGB(192,192,  0);
	m_acrDevStatus[DEVICE_STATUS_DISABLED]     = RGB(128,128,128);
	m_acrDevStatus[DEVICE_STATUS_HAS_PROBLEM]  = RGB(255, 64, 64);
	m_acrDevStatus[DEVICE_STATUS_DRIVER_ERROR] = RGB(255, 64, 64);
	m_acrDevStatus[DEVICE_STATUS_RUNNING]      = RGB(  0,192,  0);
	m_acrDevStatus[DEVICE_STATUS_STOPPED]      = RGB( 64,128, 64);
}

CDeviceTree::~CDeviceTree()
{
	Clear();
}

BEGIN_MESSAGE_MAP(CDeviceTree, CCustomDrawTreeCtrl)
	ON_WM_DEVICECHANGE()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemExpanded)
END_MESSAGE_MAP()

void CDeviceTree::SetMsgCb( LPHANDLE_MESSAGES_CALLBACK pfn, LPVOID pContext )
{
	m_pfnMsgCb = pfn;
	m_pMsgCbContext = pContext;
}

void CDeviceTree::Clear()
{
	for ( std::vector<HDEVNOTIFY>::iterator i = m_vDevNotify.begin(); i != m_vDevNotify.end(); ++i )
	{
		if ( *i != NULL )
			UnregisterDeviceNotification(*i);
	}
	m_vDevNotify.clear();
}

void CDeviceTree::PopulateDevices()
{
	Clear();

	std::vector<bool> vIsExpanded;

	//
	// try to record the last state of tree items for expanding.
	//
	HTREEITEM htiRoot = GetRootItem();
	if ( htiRoot )
	{
		if ( ItemHasChildren(htiRoot) )
		{
			HTREEITEM hNextItem = GetChildItem(htiRoot);

			while ( hNextItem )
			{
				UINT u = GetItemState( hNextItem, TVIF_STATE );
				vIsExpanded.push_back( u & TVIS_EXPANDED ? true : false );
				hNextItem = GetNextItem(hNextItem, TVGN_NEXT);
			}
		}
	}

	DeleteAllItems();

	CDevMgr dm;
	const ClassList& cl = dm.GetClasses();

	//
	// show message that the exact same one for devcon.exe
	//
	if ( m_pfnMsgCb )
	{
		m_pfnMsgCb( _T("COMMAND: devcon classes\n"), m_pMsgCbContext );

		CString str;
		str.Format(_T("Listing %d setup classes.\n"), cl.size() );
		m_pfnMsgCb( str, m_pMsgCbContext );
	}

	int nImageCount = PrepareImages(0);
	SetImageList( &sm_ilDeviceIcons, TVSIL_NORMAL );

	//
	// populate the classes
	//
	HTREEITEM hRootItem = InsertItem( _T("Local Machine"), 1, 1 );
	SetItemData( hRootItem, TIF_UNDEFINED );
	for ( unsigned i = 0; i < cl.size(); i++ )
	{
		int iImage = nImageCount - (int)cl.size() + i;

		//
		// Insert a class item with its description
		//
		HTREEITEM htc = InsertItem( cl[i].desc, iImage, iImage, hRootItem );
		SetItemData( htc, (TIF_CLASS|TIF_CLASS_EXPANDED_MASK|i));

		//
		// create notification
		//
		HDEVNOTIFY hDevNotify;
		DoRegisterDeviceInterface( GetSafeHwnd(), cl[i].guid, &hDevNotify );
		m_vDevNotify.push_back(hDevNotify); // even it fails, still add it to list.

		//
		// Insert its class name
		//
		TCHAR ach[64], ach1[64];
		wsprintf( ach, _T("Class: %s"), cl[i].name );
		InsertItem( ach, 0, 0, htc );

		//
		// Insert its class guid
		//
		wsprintf( ach, _T("ClassGUID: %s"), CreateGuidString( ach1, cl[i].guid ) );
		InsertItem( ach, 0, 0, htc );

		//
		// Insert its device type
		//
		if ( cl[i].devType >= 0 ) {
            wsprintf( ach, _T("DevType: %08Xh"), cl[i].devType );
			InsertItem( ach, 0, 0, htc );
		}

		//
		// Insert its lower filters & upper filters
		//
		LPCTSTR pFilter;
		pFilter = cl[i].upperFilters.GetFirst();
		while ( pFilter ) {
			wsprintf( ach, _T("Upper Filter: %s"), pFilter );
			InsertItem( ach, 0, 0, htc );
			pFilter = cl[i].upperFilters.GetNext();
		}
		pFilter = cl[i].lowerFilters.GetFirst();
		while ( pFilter ) {
			wsprintf( ach, _T("Lower Filter: %s"), pFilter );
			InsertItem( ach, 0, 0, htc );
			pFilter = cl[i].lowerFilters.GetNext();
		}

	}

	Expand( hRootItem, TVE_EXPAND );

	if ( cl.size() == vIsExpanded.size() )
	{
		HTREEITEM hNextItem = GetChildItem(hRootItem);

		int i = 0;
		while ( hNextItem )
		{
			if ( vIsExpanded[i] ) {
				Expand( hNextItem, TVE_EXPAND );
			}
			i++;
			hNextItem = GetNextItem(hNextItem, TVGN_NEXT);
		}
	}
}

typedef struct
{
	CDeviceTree*	ptcDevice;
	HTREEITEM		htiNode;
} EnumUsbCbParams, *PEnumUsbCbParams;

DWORD WINAPI EnumUsbHubCallback( HANDLE hUsbHub, PUSB_NODE_INFORMATION pUsbNodeInfo, LPVOID pContext );

DWORD WINAPI EnumUsbHubPortCallback(
	LPCTSTR pszHubPortName,
	LPCTSTR pszDriverKeyName,
	LPCTSTR pszExtHubDevicePath,
	PUSB_NODE_CONNECTION_INFORMATION_EX pUsbNodeConnectInfoEx,
	LPVOID pContext )
{
	PEnumUsbCbParams pParams = (PEnumUsbCbParams)pContext;

	HTREEITEM hti = pParams->ptcDevice->GetChildItem(pParams->htiNode); // the first child item should have usb hub handle (see EnumUsbHubCallback)
	ASSERT(hti);

	CString strHubHandle = pParams->ptcDevice->GetItemText(hti);

	HANDLE hUsbHub;
	_stscanf_s( strHubHandle.GetBuffer(), _T("[%x]"), &hUsbHub );


	//
	// it's an external hub connected to this hub port.
	//
	if ( pszExtHubDevicePath )
	{
		CString str;
		str.Format( _T("%s=%s"), pszHubPortName, pszExtHubDevicePath );
		hti = pParams->ptcDevice->InsertItem( str, 3, 3, pParams->htiNode );
		pParams->ptcDevice->SetItemData(hti, TIF_USBHUB);

        EnumUsbCbParams params;
		params.ptcDevice = pParams->ptcDevice;
		params.htiNode = hti;

		EnumerateUsbHub( pszExtHubDevicePath, EnumUsbHubCallback, &params );
	}
	else
	{
		hti = pParams->ptcDevice->InsertItem( pszHubPortName, 4, 4, pParams->htiNode );
		ASSERT( pUsbNodeConnectInfoEx->ConnectionIndex <= 0xFF );
		ASSERT( pUsbNodeConnectInfoEx->ConnectionStatus <= 0xFF );
		pParams->ptcDevice->SetItemData(hti,
			TIF_USBDEVICE |
			(BYTE)(pUsbNodeConnectInfoEx->ConnectionIndex&0xFF) |
			(BYTE)(pUsbNodeConnectInfoEx->ConnectionStatus) << 8
			);

		CString str;
		if ( pUsbNodeConnectInfoEx->ConnectionStatus != NoDeviceConnected )
		{
			str.Format( _T("Driver Key: %s"), pszDriverKeyName );
			pParams->ptcDevice->InsertItem( str, hti );

			HTREEITEM htidd = pParams->ptcDevice->InsertItem( _T("Device Descriptor"), hti );
			str.Format( _T("bcdUSB: %04Xh"), pUsbNodeConnectInfoEx->DeviceDescriptor.bcdUSB );
			pParams->ptcDevice->InsertItem( str, htidd );
			str.Format( _T("bDeviceClass: %02Xh"), pUsbNodeConnectInfoEx->DeviceDescriptor.bDeviceClass );
			pParams->ptcDevice->InsertItem( str, htidd );
			str.Format( _T("bDeviceSubClass: %02Xh"), pUsbNodeConnectInfoEx->DeviceDescriptor.bDeviceSubClass );
			pParams->ptcDevice->InsertItem( str, htidd );
			str.Format( _T("bDeviceProtocol: %02Xh"), pUsbNodeConnectInfoEx->DeviceDescriptor.bDeviceProtocol );
			pParams->ptcDevice->InsertItem( str, htidd );
			str.Format( _T("bMaxPacketSize0: %02Xh"), pUsbNodeConnectInfoEx->DeviceDescriptor.bMaxPacketSize0 );
			pParams->ptcDevice->InsertItem( str, htidd );
			str.Format( _T("idVendor: %04Xh"), pUsbNodeConnectInfoEx->DeviceDescriptor.idVendor );
			pParams->ptcDevice->InsertItem( str, htidd );
			str.Format( _T("idProduct: %04Xh"), pUsbNodeConnectInfoEx->DeviceDescriptor.idProduct );
			pParams->ptcDevice->InsertItem( str, htidd );
			str.Format( _T("bcdDevice: %04Xh"), pUsbNodeConnectInfoEx->DeviceDescriptor.bcdDevice );
			pParams->ptcDevice->InsertItem( str, htidd );

			USB_DESCRIPTOR configDesc;
			GetUsbDeviceConfigDescriptor( hUsbHub, pUsbNodeConnectInfoEx->ConnectionIndex, 0, &configDesc );

			if ( configDesc.size() >= sizeof(USB_CONFIGURATION_DESCRIPTOR) )
			{
				HTREEITEM hticd = pParams->ptcDevice->InsertItem( _T("Configuration Descriptor"), hti );
				PUSB_CONFIGURATION_DESCRIPTOR pCD = (PUSB_CONFIGURATION_DESCRIPTOR)&configDesc[0];
				str.Format( _T("wTotalLength: %04Xh"), pCD->wTotalLength );
				pParams->ptcDevice->InsertItem( str, hticd );
				str.Format( _T("bNumInterfaces: %02Xh"), pCD->bNumInterfaces );
				pParams->ptcDevice->InsertItem( str, hticd );
				str.Format( _T("bConfigurationValue: %02Xh"), pCD->bConfigurationValue );
				pParams->ptcDevice->InsertItem( str, hticd );
				str.Format( _T("iConfiguration: %02Xh"), pCD->iConfiguration );
				pParams->ptcDevice->InsertItem( str, hticd );
				str.Format( _T("bmAttributes: %02Xh"), pCD->bmAttributes );
				pParams->ptcDevice->InsertItem( str, hticd );
				str.Format( _T("MaxPower: %02Xh"), pCD->MaxPower );
				pParams->ptcDevice->InsertItem( str, hticd );
			}
		}
	}

	return 1;
}

DWORD WINAPI EnumUsbHubCallback( HANDLE hUsbHub, PUSB_NODE_INFORMATION pUsbNodeInfo, LPVOID pContext )
{
	PEnumUsbCbParams pParams = (PEnumUsbCbParams)pContext;

	CString str;
	str.Format( _T("[%08x]"), hUsbHub );
	HTREEITEM hti = pParams->ptcDevice->InsertItem( str, 0, 0, pParams->htiNode );
	pParams->ptcDevice->SetItemData(hti,TIF_UNDEFINED);

	USHORT wHubChar = pUsbNodeInfo->u.HubInformation.HubDescriptor.wHubCharacteristics;

	switch (wHubChar & 0x0003) {
	case 0x0000:
		pParams->ptcDevice->InsertItem( _T("Power switching: Ganged"), 0, 0, hti );
		break;
	case 0x0001:
		pParams->ptcDevice->InsertItem( _T("Power switching: Individual"), 0, 0, hti );
		break;
	case 0x0002:
	case 0x0003:
		pParams->ptcDevice->InsertItem( _T("Power switching: None"), 0, 0, hti );
		break;
	}

	switch (wHubChar & 0x0004) {
	case 0x0000:
		pParams->ptcDevice->InsertItem( _T("Compound device: No"), 0, 0, hti );
		break;
	case 0x0004:
		pParams->ptcDevice->InsertItem( _T("Compound device: Yes"), 0, 0, hti );
		break;
	}

	switch (wHubChar & 0x0018) {
	case 0x0000:
		pParams->ptcDevice->InsertItem( _T("Over-current Protection: Global"), 0, 0, hti );
		break;
	case 0x0008:
		pParams->ptcDevice->InsertItem( _T("Over-current Protection: Individual"), 0, 0, hti );
		break;
	case 0x0010:
	case 0x0018:
		pParams->ptcDevice->InsertItem( _T("No Over-current Protection (Bus Power Only)"), 0, 0, hti );
		break;
	}

	EnumUsbCbParams params;
	params.ptcDevice = pParams->ptcDevice;
	params.htiNode = pParams->htiNode;

	EnumerateUsbHubPorts( hUsbHub, EnumUsbHubPortCallback, &params );

	pParams->ptcDevice->Expand( params.htiNode, TVE_EXPAND );

	return 1;
}

DWORD WINAPI EnumUsbHcdCallback( LPCTSTR pszHostName, LPCTSTR pszRootHubDevicePath, LPVOID pContext )
{
	PEnumUsbCbParams pParams = (PEnumUsbCbParams)pContext;
	HTREEITEM htiHcd = pParams->ptcDevice->InsertItem( pszHostName, 2, 2, pParams->htiNode );
	pParams->ptcDevice->SetItemData(htiHcd, TIF_UNDEFINED);

	CString str;
	str.Format( _T("RootHub=%s"), pszRootHubDevicePath );
	HTREEITEM hti = pParams->ptcDevice->InsertItem( str, 2, 2, htiHcd );
	pParams->ptcDevice->SetItemData(hti, TIF_USBHUB);

	EnumUsbCbParams params;
	params.ptcDevice = pParams->ptcDevice;
	params.htiNode = hti;

	EnumerateUsbHub( pszRootHubDevicePath, EnumUsbHubCallback, &params );

	pParams->ptcDevice->Expand( hti, TVE_EXPAND );
	pParams->ptcDevice->Expand( htiHcd, TVE_EXPAND );

	return 1;
}

void CDeviceTree::PopulateUsbDevices()
{
	Clear();
	DeleteAllItems();

	int nImageCount = PrepareImages(1);
	SetImageList( &sm_ilUsbViewIcons, TVSIL_NORMAL );

	HTREEITEM hRootItem = InsertItem( _T("Local Machine"), 1, 1 );
	SetItemData( hRootItem, TIF_UNDEFINED );

	EnumUsbCbParams params;
	params.ptcDevice = this;
	params.htiNode = hRootItem;

	CWaitCursor cur;

	EnumerateUsbHostControllers(EnumUsbHcdCallback,&params);

	Expand( hRootItem, TVE_EXPAND );
}

DEVICE_STATUS CDeviceTree::GetDeviceStatus(const CDevice& dev,CString& strStatus)
{
	DEVICE_STATUS DevStatus;
	ULONG status, problem;
	if ( dev.GetStatus(status,problem) )
	{
		if ((status & DN_HAS_PROBLEM) && problem == CM_PROB_DISABLED) {
			strStatus.Format( _T("Status: Device is disabled.") );
			DevStatus = DEVICE_STATUS_DISABLED;
		}
		else if (status & DN_HAS_PROBLEM) {
			strStatus.Format( _T("Status: Device has a problem: %d!."), problem );
			DevStatus = DEVICE_STATUS_HAS_PROBLEM;
		}
		else if (status & DN_PRIVATE_PROBLEM) {
			strStatus.Format( _T("Status: Device has a problem reported by the driver.") );
			DevStatus = DEVICE_STATUS_DRIVER_ERROR;
		}
		else if (status & DN_STARTED) {
			strStatus.Format( _T("Status: Driver is running.") );
			DevStatus = DEVICE_STATUS_RUNNING;
		}
		else {
			strStatus.Format( _T("Status: Device is currently stopped.") );
			DevStatus = DEVICE_STATUS_STOPPED;
		}
	}
	else
	{
		strStatus.Format( _T("Status: Unknown!") );
		DevStatus = DEVICE_STATUS_UNKNOWN;
	}

	return DevStatus;
}

void CDeviceTree::RefreshClass( HTREEITEM htiClassNode, const CDeviceSet& ds, int nClassIndex )
{
	DWORD_PTR dw = GetItemData(htiClassNode);
	DWORD_PTR dwType = (dw&TIF_MASK);
	ASSERT(dwType==TIF_CLASS);

	DWORD_PTR dwExpanded = (dw&TIF_CLASS_EXPANDED_MASK);
	DWORD_PTR dwClassIdx = (dw&TIF_CLASS_INDEX_MASK);

	if ( dwExpanded == TIF_CLASS_EXPANDED_MASK ) {
		// not yet expanded
		SetItemData( htiClassNode, TIF_CLASS|dwClassIdx ); // expanded
	}
	else {
		// the class node has been expanded
		HTREEITEM htiDevice = GetTreeItem( *this, htiClassNode, 2 );
		if ( htiDevice )
			DeleteItem(htiDevice);
	}

	//
	// insert all devices to the tree
	//
	DWORD dwCount = ds.GetDeviceCount();
	if ( dwCount == 0 )
		return;

	// create device node
    HTREEITEM htiDevice = InsertItem(_T("Device"), nClassIndex+2, nClassIndex+2, htiClassNode);

	for ( DWORD c = 0; c < dwCount; c++ ) {
		//
		// insert a device with its description
		//
		HTREEITEM h = InsertItem(ds.GetDevice(c).GetDesc(),nClassIndex+2,nClassIndex+2,htiDevice);

		CString strMsg;
		DEVICE_STATUS DevStatus = GetDeviceStatus( ds.GetDevice(c), strMsg );
		InsertItem(strMsg,0,0,h);

		// Set item data for OnTvnSelchangedTree1()
		// TIF_MASK:               0xF0000000
		// TIF_DEVICE_STATUS_MASK: 0x0F000000
		// TIF_DEVICE_CLASS_MASK:  0x00FFF000
		// TIF_DEVICE_INDEX_MASK:  0x00000FFF
		SetItemData( h,
			TIF_DEVICE |
			(TIF_DEVICE_STATUS_MASK&(DevStatus<<24)) |
			(TIF_DEVICE_CLASS_MASK&(nClassIndex<<12)) |
			(TIF_DEVICE_INDEX_MASK&(c)) );

		RefreshDevice( h, ds.GetDevice(c) );
	}

	Expand(htiDevice,TVE_EXPAND);
	EnsureVisible(htiDevice);
}

void CDeviceTree::RefreshDevice( HTREEITEM h, const CDevice& dev )
{
	TCHAR ach[MAX_PATH];
	wsprintf( ach, _T("Instance ID: %s"), dev.GetID() );
	InsertItem(ach,0,0,h);

	tstring str;
	CMultiSzString mstr;

#ifdef UNICODE
#define INFO(x,y) \
	if ( dev.GetStringProperty(y,str) ) \
	{ \
		wsprintf( ach, _T("%s: %s"), L#x, str.c_str() ); \
		InsertItem(ach,0,0,h); \
	}

#define INFO_MZ(x,y) \
	if ( dev.GetMzStringProperty(y,mstr) ) \
	{ \
		LPCTSTR p = mstr.GetFirst(); \
		while ( p ) \
		{ \
			wsprintf( ach, _T("%s: %s"), L#x, p ); \
			InsertItem(ach,0,0,h); \
			p = mstr.GetNext(); \
		} \
	}

#else
#define INFO(x,y) \
	if ( dev.GetStringProperty(y,str) ) \
	{ \
		wsprintf( ach, _T("%s: %s"), #x, str.c_str() ); \
		InsertItem(ach,0,0,h); \
	}

#define INFO_MZ(x,y) \
	if ( dev.GetMzStringProperty(y,mstr) ) \
	{ \
		LPCTSTR p = mstr.GetFirst(); \
		while ( p ) \
		{ \
			wsprintf( ach, _T("%s: %s"), #x, p ); \
			InsertItem(ach,0,0,h); \
			p = mstr.GetNext(); \
		} \
	}

#endif

	INFO(DeviceDesc,SPDRP_DEVICEDESC);
	INFO(Driver,SPDRP_DRIVER);
	INFO(LocationInformation,SPDRP_LOCATION_INFORMATION);
	INFO(Mfg,SPDRP_MFG);
	INFO(Service,SPDRP_SERVICE);
	INFO_MZ(Upper Filter,SPDRP_UPPERFILTERS);
	INFO_MZ(Lower Filter,SPDRP_LOWERFILTERS);

#undef INFO

	ULONG status, problem;
	bool haveConfig;
	std::vector<RESOURCE_DESCRIPTOR> list;
	if ( dev.GetDeviceResources(status, problem, haveConfig, list) )
	{
		if ( !haveConfig ) {
			InsertItem( (status&DN_STARTED)
				? _T("Device is not using any resources.")
				: _T("Device has no resources reserved."), 0, 0, h );
		}
		else {
			HTREEITEM h2 = InsertItem( (status & DN_STARTED)
				? _T("Device is currently using the following resources:")
				: _T("Device has the following resources reserved:"), 0, 0, h );
			std::vector<RESOURCE_DESCRIPTOR>::iterator i = list.begin();
			while ( i != list.end() ) {
				switch ( i->resID ) {
				case ResType_Mem:
					{
						PMEM_RESOURCE pMemData = (PMEM_RESOURCE)&i->resData[0];
						if (pMemData->MEM_Header.MD_Alloc_End-pMemData->MEM_Header.MD_Alloc_Base+1) {
							wsprintf(ach,TEXT("MEM : %08I64x-%08I64x"),pMemData->MEM_Header.MD_Alloc_Base,pMemData->MEM_Header.MD_Alloc_End);
							InsertItem( ach, 0, 0, h2 );
						}
						break;
					}
				case ResType_IO:
					{
						PIO_RESOURCE pIoData = (PIO_RESOURCE)&i->resData[0];
						if(pIoData->IO_Header.IOD_Alloc_End-pIoData->IO_Header.IOD_Alloc_Base+1) {
							wsprintf(ach,TEXT("IO  : %04I64x-%04I64x"),pIoData->IO_Header.IOD_Alloc_Base,pIoData->IO_Header.IOD_Alloc_End);
							InsertItem( ach, 0, 0, h2 );
						}
						break;
					}
				case ResType_DMA:
					{
						PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)&i->resData[0];
						wsprintf(ach,TEXT("DMA : %u"),pDmaData->DMA_Header.DD_Alloc_Chan);
						InsertItem( ach, 0, 0, h2 );
						break;
					}
				case ResType_IRQ:
					{
						PIRQ_RESOURCE  pIrqData = (PIRQ_RESOURCE)&i->resData[0];
						wsprintf(ach,TEXT("IRQ : %u"),pIrqData->IRQ_Header.IRQD_Alloc_Num);
						InsertItem( ach, 0, 0, h2 );
						break;
					}
				}
				++i;
			}
		}
	}
}

void CDeviceTree::DeleteChildItems( HTREEITEM htiNode )
{
	HTREEITEM htiChild = GetChildItem(htiNode);
	HTREEITEM htiNext;
	while ( htiChild )
	{
		htiNext = GetNextItem( htiChild, TVGN_NEXT );
		DeleteItem( htiChild );
		htiChild = htiNext;
	}
}

BOOL CDeviceTree::OnDeviceChange( UINT nEventType, DWORD_PTR dwData )
{
	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)dwData;

	switch (nEventType) {
	case DBT_DEVICEARRIVAL:
		{
			// Check whether a CD or DVD was inserted into a drive.
			PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
			if ( lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME && (lpdbv->dbcv_flags & DBTF_MEDIA) )
			{
				if ( m_pfnMsgCb ) {
					CString str;
					str.Format(_T("Drive %c: Media has arrived.\n"), FirstDriveFromMask(lpdbv->dbcv_unitmask) );
					m_pfnMsgCb( str, m_pMsgCbContext );
				}
			}
			else if ( lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE )
			{
				PDEV_BROADCAST_DEVICEINTERFACE lpdbd = (PDEV_BROADCAST_DEVICEINTERFACE)lpdb;
				if ( m_pfnMsgCb ) {
					CString str;
					str.Format(_T("The device '%s' has been inserted and is now available."), lpdbd->dbcc_name );
					m_pfnMsgCb( str, m_pMsgCbContext );
				}

				CDevMgr dm;
				const ClassList& cl = dm.GetClasses();
				int nClassIndex = GetClassIndex(cl,lpdbd->dbcc_classguid);

				HTREEITEM htiClassNode = GetTreeItem( *this, GetRootItem(), nClassIndex );
				DWORD dw = ((DWORD)GetItemData(htiClassNode)&TIF_CLASS_EXPANDED_MASK);
				if ( dw != TIF_CLASS_EXPANDED_MASK )
				{
					// the class node has been expanded
					CDeviceSet ds;
					dm.EnumerateDevices( ds, 0, cl[nClassIndex].name, NULL );
					RefreshClass( htiClassNode, ds, nClassIndex );
				}
			}
			else
			{
				if ( m_pfnMsgCb ) {
					CString str;
					str.Format(_T("A device has been inserted and is now available."));
					m_pfnMsgCb( str, m_pMsgCbContext );
				}
			}
		}
		break;
	case DBT_DEVICEQUERYREMOVE:
		TRACE("Permission to remove a device is requested. Any application can deny this request and cancel the removal. \n");
		break;
	case DBT_DEVICEQUERYREMOVEFAILED:
		TRACE("Request to remove a device has been canceled. \n");
		break;
	case DBT_DEVICEREMOVEPENDING:
		TRACE("Device is about to be removed. Cannot be denied. \n");
		break;
	case DBT_DEVICEREMOVECOMPLETE:
		{
			// Check whether a CD or DVD was removed from a drive.
			PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
			if ( lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME && ( lpdbv->dbcv_flags & DBTF_MEDIA ) )
			{
				if ( m_pfnMsgCb ) {
					CString str;
					str.Format( _T("Drive %c: Media was removed.\n"), FirstDriveFromMask(lpdbv ->dbcv_unitmask) );
					m_pfnMsgCb( str, m_pMsgCbContext );
				}
			}
			else if ( lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE )
			{
				PDEV_BROADCAST_DEVICEINTERFACE lpdbd = (PDEV_BROADCAST_DEVICEINTERFACE)lpdb;
				if ( m_pfnMsgCb ) {
					CString str;
					str.Format(_T("The device '%s' has been removed."), lpdbd->dbcc_name );
					m_pfnMsgCb( str, m_pMsgCbContext );
				}

				CDevMgr dm;
				const ClassList& cl = dm.GetClasses();
				int nClassIndex = GetClassIndex(cl,lpdbd->dbcc_classguid);

				HTREEITEM htiClassNode = GetTreeItem( *this, GetRootItem(), nClassIndex );
				DWORD dw = ((DWORD)GetItemData(htiClassNode)&TIF_CLASS_EXPANDED_MASK);
				if ( dw != TIF_CLASS_EXPANDED_MASK )
				{
					// the class node has been expanded
					CDeviceSet ds;
					dm.EnumerateDevices( ds, 0, cl[nClassIndex].name, NULL );
					RefreshClass( htiClassNode, ds, nClassIndex );
				}
			}
			else
			{
				if ( m_pfnMsgCb ) {
					CString str;
					str.Format(_T("A device has been removed."));
					m_pfnMsgCb( str, m_pMsgCbContext );
				}
			}
		}
		break;
	case DBT_DEVICETYPESPECIFIC:
		TRACE("Device-specific event. \n");
		break;
	case DBT_CONFIGCHANGED:
		TRACE("Current configuration has changed. \n");
		break;
	case DBT_DEVNODES_CHANGED:
		TRACE(_T("Device node has changed.\n"));
		break;
	default:
		TRACE("Unknown event type %08Xh\n", nEventType);
		break;
	}

	return TRUE;
}

void CDeviceTree::OnItemExpanded(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;

	HTREEITEM htc = pNMTreeView->itemNew.hItem;
	DWORD_PTR dw = GetItemData(htc);
	//
	// Because the ID is set when OnInitDialog() is called.
	// Insert devices under this class and set its item data to be 0 to prevent from calling this function.
	//
	DWORD_PTR dwType = (dw&TIF_MASK);
	DWORD_PTR dwClassIndex = (dw&TIF_CLASS_INDEX_MASK);
	DWORD_PTR dwExpanded = (dw&TIF_CLASS_EXPANDED_MASK);

	if ( dwType == TIF_CLASS && dwExpanded == TIF_CLASS_EXPANDED_MASK )
	{
		CDevMgr dm;
		const ClassList& cl = dm.GetClasses();

		CWaitCursor cur;
		//
		// enumerate devices under the class
		//
        CDeviceSet ds;
		dm.EnumerateDevices( ds, 0, cl[dwClassIndex].name, NULL );

		//
		// Show message that the command is used for devcon.
		//
		if ( m_pfnMsgCb )
		{
			CString str;
			str.Format( _T("COMMAND: devcon listclass %s"), cl[dwClassIndex].name );
			m_pfnMsgCb( str, m_pMsgCbContext );
		}

		RefreshClass(htc,ds,(DWORD)dwClassIndex);
	}
}

void CDevConWinDlg::OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;

	DWORD_PTR dw = m_tcMgr.GetItemData(pNMTreeView->itemNew.hItem);

	// check the item data if this is a device or a class, otherwise disable related buttons
	switch (dw&TIF_MASK) {
	case TIF_DEVICE:
		{
			DWORD dwDevStatus = (DWORD)(dw&TIF_DEVICE_STATUS_MASK)>>24;

			GetDlgItem(IDC_BTN_ENABLE)->EnableWindow(  dwDevStatus == DEVICE_STATUS_DISABLED );
			GetDlgItem(IDC_BTN_DISABLE)->EnableWindow( dwDevStatus == DEVICE_STATUS_RUNNING );

			GetDlgItem(IDC_BTN_FILTER)->EnableWindow();
		}
		break;
		// deadlock
	/*case TIF_CLASS:
		{
			GetDlgItem(IDC_BTN_ENABLE)->EnableWindow();
			GetDlgItem(IDC_BTN_DISABLE)->EnableWindow();
		}
		break;*/
	case TIF_USBHUB:
		{
		}
		break;
	case TIF_USBDEVICE:
		{
			DWORD dwDevStatus = (DWORD)(dw&TIF_USBDEVICE_CONNECT_STATUS_MASK)>>8;
			DWORD dwDevIndex = (DWORD)(dw&TIF_USBDEVICE_CONNECT_INDEX_MASK);

			if ( dwDevStatus != NoDeviceConnected )
			{
				GetDlgItem(IDC_BTN_USB_DESCRIPTOR)->EnableWindow();
			}
		}
		break;
	default:
		{
            GetDlgItem(IDC_BTN_ENABLE)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_DISABLE)->EnableWindow(FALSE);

			GetDlgItem(IDC_BTN_FILTER)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_USB_DESCRIPTOR)->EnableWindow(FALSE);
		}
		break;
	}
}

void CDevConWinDlg::OnBnClickedBtnEnable()
{
	// TODO: Add your control notification handler code here
	HTREEITEM hti = m_tcMgr.GetSelectedItem();
	DWORD_PTR dwptr = m_tcMgr.GetItemData(hti);

	switch ( (dwptr&TIF_MASK) ) {
	case TIF_DEVICE:
		{
			DWORD dwDevStatus = ((DWORD)dwptr&TIF_DEVICE_STATUS_MASK)>>24;
			ASSERT(dwDevStatus==DEVICE_STATUS_DISABLED);

			DWORD dwClsIdx = ((DWORD)dwptr&TIF_DEVICE_CLASS_MASK)>>12;
			DWORD dwDevIdx = ((DWORD)dwptr&TIF_DEVICE_INDEX_MASK);

			CDevMgr dm;
			const ClassList& cl = dm.GetClasses();
			CDeviceSet ds;
			dm.EnumerateDevices( ds, 0, cl[dwClsIdx].name, NULL );

			CWaitCursor cur;
			CDevice& dev = ds.GetDevice(dwDevIdx);
			bool bNeedRestart;

			//
			// show devcon command
			//
			ShowMsg( _T("COMMAND: devcon enable \"@%s\""), dev.GetID() );

			dev.Enable(bNeedRestart);

			Sleep(1000);
		}
		break;

	case TIF_CLASS:
		{
			DWORD dwClsIdx = (DWORD)dwptr&TIF_CLASS_INDEX_MASK;

			CWaitCursor cur;

			CDevMgr dm;
			const ClassList& cl = dm.GetClasses();
			CDeviceSet ds;
			dm.EnumerateDevices( ds, 0, cl[dwClsIdx].name, NULL );

			//
			// show dev command
			//
			ShowMsg( _T("COMMAND: devcon enable \"=%s\""), cl[dwClsIdx].name );

			bool bNeedRestart;
			for ( DWORD dwDevIdx = 0; dwDevIdx < ds.GetDeviceCount(); dwDevIdx++ )
			{
				CDevice& dev = ds.GetDevice(dwDevIdx);
				dev.Enable(bNeedRestart);
			}

			Sleep(1000);
		}
		break;

	default:
		{
			ASSERT(FALSE);
		}
		break;
	}
}

void CDevConWinDlg::OnBnClickedBtnDisable()
{
	// TODO: Add your control notification handler code here
	HTREEITEM hti = m_tcMgr.GetSelectedItem();
	DWORD_PTR dwptr = m_tcMgr.GetItemData(hti);
	ASSERT( (dwptr&TIF_MASK)==TIF_DEVICE || (dwptr&TIF_MASK)==TIF_CLASS );

	DWORD dwDevStatus = ((DWORD)dwptr&TIF_DEVICE_STATUS_MASK)>>24;
	ASSERT(dwDevStatus==DEVICE_STATUS_RUNNING);

	DWORD dwClsIdx = ((DWORD)dwptr&TIF_DEVICE_CLASS_MASK)>>12;
	DWORD dwDevIdx = ((DWORD)dwptr&TIF_DEVICE_INDEX_MASK);

	CDevMgr dm;
	const ClassList& cl = dm.GetClasses();
	CDeviceSet ds;
	dm.EnumerateDevices( ds, 0, cl[dwClsIdx].name, NULL );

	CWaitCursor cur;
	CDevice& dev = ds.GetDevice(dwDevIdx);
	bool bNeedRestart;

	//
	// show devcon command
	//
	ShowMsg( _T("COMMAND: devcon disable \"@%s\""), dev.GetID() );

	dev.Disable(bNeedRestart);

	Sleep(1000);
}

void CDevConWinDlg::OnBnClickedBtnFilter()
{
	// TODO: Add your control notification handler code here
	HTREEITEM hti = m_tcMgr.GetSelectedItem();
	DWORD_PTR dwptr = m_tcMgr.GetItemData(hti);
	ASSERT( (dwptr&TIF_MASK)==TIF_DEVICE );

	DWORD dwClsIdx = ((DWORD)dwptr&TIF_DEVICE_CLASS_MASK)>>12;
	DWORD dwDevIdx = ((DWORD)dwptr&TIF_DEVICE_INDEX_MASK);

	CDevMgr dm;
	const ClassList& cl = dm.GetClasses();
	CDeviceSet ds;
	dm.EnumerateDevices( ds, 0, cl[dwClsIdx].name, NULL );

	CDevice& dev = ds.GetDevice(dwDevIdx);

	CFilterSettingDlg dlg;
	dev.GetMzStringProperty( SPDRP_LOWERFILTERS, dlg.m_mstrLowerFilters );
	dev.GetMzStringProperty( SPDRP_UPPERFILTERS, dlg.m_mstrUpperFilters );

	if ( dlg.DoModal() == IDOK )
	{
		dev.SetMzStringProperty( SPDRP_LOWERFILTERS, dlg.m_mstrLowerFilters );
		dev.SetMzStringProperty( SPDRP_UPPERFILTERS, dlg.m_mstrUpperFilters );

		m_tcMgr.DeleteChildItems(hti);
		m_tcMgr.RefreshDevice( hti, dev );
	}
}

static HWND ghNotepadWnd = NULL;
static BOOL CALLBACK EnumChildProc(
	__in  HWND hwnd,
	__in  LPARAM lParam )
{
	DWORD dwTarget = (DWORD)lParam;

	DWORD dwProcessId;
	GetWindowThreadProcessId(hwnd,&dwProcessId);

	if ( dwTarget == dwProcessId )
	{
		ghNotepadWnd = hwnd;
		return FALSE;
	}

	return TRUE;
}

void CDevConWinDlg::OnBnClickedBtnDescriptors()
{
	USB_DESCRIPTOR configDesc;

	TCHAR achModuleFilePath[MAX_PATH];
	CString strSaveFile;

	TCHAR achSysDir[MAX_PATH];
	CString strCmdLine;

	GetModuleFileName( NULL, achModuleFilePath, sizeof(achModuleFilePath) );
	TCHAR* pSlash = _tcsrchr( achModuleFilePath, _T('\\') );
	*(pSlash+1) = NULL;

	GetSystemDirectory( achSysDir, sizeof(achSysDir) );
	_tcscat_s( achSysDir, _T("\\") );


	//
	// Get configuration descriptor
	//
	HTREEITEM hti = m_tcMgr.GetSelectedItem();
	DWORD_PTR dwptr = m_tcMgr.GetItemData(hti);
	ASSERT( (dwptr&TIF_MASK)==TIF_USBDEVICE );

	DWORD dwConnectIndex = ((DWORD)dwptr & TIF_USBDEVICE_CONNECT_INDEX_MASK);
	DWORD dwConnectStatus = ((DWORD)dwptr & TIF_USBDEVICE_CONNECT_STATUS_MASK) >> 8 ;

	HTREEITEM htiHub = m_tcMgr.GetParentItem(hti);
	dwptr = m_tcMgr.GetItemData(htiHub);
	ASSERT( (dwptr&TIF_MASK)==TIF_USBHUB );

	CString strHubName = m_tcMgr.GetItemText(htiHub);
	int n = strHubName.Find(_T('='));
	strHubName = _T("\\\\.\\") + strHubName.Mid(n+1);

	HANDLE hHubFile = CreateFile(
		strHubName,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (hHubFile == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox( _T("Failed to create Hub Device!!!"), MB_OK );
		return;
	}

	GetUsbDeviceConfigDescriptor( hHubFile, dwConnectIndex, 0, &configDesc );

	CloseHandle(hHubFile);

	//
	// save to file
	//
	strSaveFile = achModuleFilePath;
	strSaveFile += _T("DevConWin.configDesc.bin");

	FILE* fp;
	_tfopen_s( &fp, strSaveFile, _T("wb") );
	fwrite( &configDesc[0], 1, configDesc.size(), fp );
	fclose(fp);

	strSaveFile = achModuleFilePath;
	strSaveFile += _T("DevConWin.configDesc.txt");

	_tfopen_s( &fp, strSaveFile, _T("wt") );
	_ftprintf_s( fp, _T("// %s\n"), strSaveFile );
	_ftprintf_s( fp, _T("// Configuration Descriptor\n{\n") );

	PBYTE pCur = &configDesc[0];
	PBYTE pEnd = pCur + configDesc.size();

	BYTE bInterfaceClass = 0;
	BYTE bInterfaceSubClass = 0;
	BYTE bInterfaceProtocol = 0;

	while ( pCur < pEnd )
	{
		PBYTE pLast = pCur;

		switch ( *(pCur+1) ) {
		case 0x01: // device descriptor
			{
				ASSERT(*pCur == 0x12);
				_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // bLength, Device Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // bcdUSB\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, // bDeviceClass\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bDeviceSubClass\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bDeviceProtocol\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bMaxPacketSize\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // idVendor\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // idProduct\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // bcdDevice\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, // iManufacturer\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // iProduct\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // iSerialNumber\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bNumConfigurations\n"), *pLast++ );
			}
			break;
		case 0x02: // configuration descriptor
			{
				ASSERT(*pCur==0x09);
				_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // bLength, Configuration Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wTotalLength\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, // bNumInterfaces\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bConfigurationValue\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // iConfiguration\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bmAttributes\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bMaxPower\n"), *pLast++ );
			}
			break;
		case 0x04: // interface descriptor
			{
				// http://www.usb.org/developers/defined_class
				ASSERT(*pCur==0x09);
				_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // bLength, Interface Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, // bInterfaceNumber\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bAlternateSetting\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bNumEndpoints\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bInterfaceClass\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bInterfaceSubClass\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bInterfaceProtocol\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // iInterface\n"), *pLast++ );

				bInterfaceClass = *(pCur+5);
				bInterfaceSubClass = *(pCur+6);
				bInterfaceProtocol = *(pCur+7);
			}
			break;
		case 0x05: // endpoint descriptor
			{
				ASSERT(*pCur>=0x07);
				_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // bLength, Endpoint Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, // bEndpointAddress, bit7:direction(IN=1), bit6..4:reserved, bit3..0:endpoint number.\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bmAttributes, bit0..1: 00=control 01=isochronous 10=bulk 11=interrupt\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wMaxPacketSize\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, // bInterval\n"), *pLast++ );

				if( bInterfaceClass == 0x01 &&
					bInterfaceSubClass == 0x02 &&
					bInterfaceProtocol == 0x00 ) // standard AS Isochronous Synch Endpoint Descriptor
				{
					ASSERT(*pCur>=0x09);
					_ftprintf_s( fp, _T("        0x%02X, // bRefresh\n"), *pLast++ );
					_ftprintf_s( fp, _T("        0x%02X, // bSynchAddress\n"), *pLast++ );
				}
			}
			break;
		case 0x0B: // IAD descriptor
			{
				ASSERT(*pCur==0x08);
				_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // bLength, IAD Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, // bFirstInterface\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bInterfaceCount\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bFunctionClass\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bFunctionSubClass\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // bFunctionProtocol\n"), *pLast++ );
				_ftprintf_s( fp, _T("        0x%02X, // iFunction\n"), *pLast++ );
			}
			break;
		case 0x21: // HID
			{
				_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // bLength, HID Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // bcdHID\n"), *pLast, *(pLast+1) ); pLast+=2;
				_ftprintf_s( fp, _T("        0x%02X, // bCountryCode\n"), *pLast++ );
				BYTE bNumberDescriptors = *pLast++;
				_ftprintf_s( fp, _T("        0x%02X, // bNumDescriptors\n"), bNumberDescriptors );
				for ( int n = 0; n < bNumberDescriptors; n++ )
				{
					_ftprintf_s( fp, _T("        0x%02X, // bDescriptorType\n"), *pLast++ );
					_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wDescriptorLength\n"), *pLast, *(pLast+1) );
					pLast+=2;
				}
			}
			break;
		case 0x24: // CS_INTERFACE (class-specific interface descriptor)
			_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // Length, Class-specific Interface Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;

			if( bInterfaceClass == 0x01 && // audio interface class
				bInterfaceSubClass == 0x01 && // audio control interface subclass
				bInterfaceProtocol == 0x00 )
			{
				switch (*pLast) {
				case 0x01: // HEADER
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: HEADER\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // bcdADC\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wTotalLength\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, // bInCollection\n"), *pLast++ );
						const int inCollection = *(pCur+7);
						for ( int i = 1; i <= inCollection; i++ ) {
							_ftprintf_s( fp, _T("        0x%02X, // baInterfaceNr[%d]\n"), *pLast++, i );
						}
					}
					break;
				case 0x02: // INPUT_TERMINAL
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: INPUT_TERMINAL\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTerminalID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wTerminalType\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, // bAssocTerminal\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bNrChannels\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wChannelConfig\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, // iChannelNames\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // iTerminal\n"), *pLast++ );
					}
					break;
				case 0x03: // OUTPUT_TERMINAL
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: OUTPUT_TERMINAL\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTerminalID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wTerminalType\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, // bAssocTerminal\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bSourceID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // iTerminal\n"), *pLast++ );
					}
					break;
				case 0x06: // FEATURE_UNIT
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: FEATURE_UNIT\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bUnitID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bSourceID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bControlSize\n"), *pLast++ );
						const int controlSize = *(pCur+5);
						const int nrChannel = (*pCur - 7)/controlSize;
						for ( int i = 0; i < nrChannel; i++ )
						{
							_ftprintf_s( fp, _T("        ") );
							for ( int j = 0; j < controlSize; j++ )
								_ftprintf_s( fp, _T("0x%02X, "), *pLast++ );
							_ftprintf_s( fp, _T("// bmaControls[%d]\n"), i );
						}
						_ftprintf_s( fp, _T("        0x%02X, // iFeature\n"), *pLast++ );

					}
					break;
				default:
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: unknown\n"), *pLast++ );
					}
					break;
				}
			}
			else if(
				bInterfaceClass == 0x01 && // audio interface class
				bInterfaceSubClass == 0x02 && // audio streaming interface subclass
				bInterfaceProtocol == 0x00 )
			{
				switch (*pLast) {
				case 0x01: // AS_GENERAL
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: AS_GENERAL\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTerminalLink\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bDelay\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wFormatTag\n"), *pLast, *(pLast+1) ); pLast+=2;
					}
					break;
				case 0x02: // FORMAT_TYPE
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: FORMAT_TYPE\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bFormatType\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bNrChannels\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bSubframeSize\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bBitResolution\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bSamFreqType\n"), *pLast++ );
						const int sameFreqType = *(pCur+7);
						if ( sameFreqType == 0 )
						{
							_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, // tLowerSamFreq\n"), *(pLast+0), *(pLast+1), *(pLast+2) ); pLast+=3;
							_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, // tUpperSamFreq\n"), *(pLast+0), *(pLast+1), *(pLast+2) ); pLast+=3;
						}
						else
						{
							for ( int i = 0; i < sameFreqType; i++ )
							{
								_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, // tSamFreq[%d]\n"), *(pLast+0), *(pLast+1), *(pLast+2), i+1 ); pLast+=3;
							}
						}

					}
					break;
				default:
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: unknown\n"), *pLast++ );
					}
					break;
				}
			}
			else if(
				bInterfaceClass == 0x0E && // video interface class
				bInterfaceSubClass == 0x01 && // video control interface subclass
				bInterfaceProtocol == 0x00 )
			{
				switch (*pLast) {
				case 0x01: // VC_HEADER
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VC_HEADER\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // bcdUVC\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wTotalLength\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwClockFrequency\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, // bInCollection\n"), *pLast++ );
						const int inCollection = *(pCur+11);
						for ( int i = 1; i <= inCollection; i++ ) {
							_ftprintf_s( fp, _T("        0x%02X, // baInterfaceNr[%d]\n"), *pLast++, i );
						}
					}
					break;
				case 0x02: // VC_INPUT_TERMINAL
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VC_INPUT_TERMINAL\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTerminalID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wTerminalType\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, // bAssocTerminal\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // iTerminal\n"), *pLast++ );
						const int terminalType = *(WORD*)(pCur+4);
						if ( terminalType == 0x201 ) // ITT_CAMERA
						{
							_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wObjectiveFocalLengthMin\n"), *pLast, *(pLast+1) ); pLast+=2;
							_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wObjectiveFocalLengthMax\n"), *pLast, *(pLast+1) ); pLast+=2;
							_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wOcularFocalLength\n"), *pLast, *(pLast+1) ); pLast+=2;
							_ftprintf_s( fp, _T("        0x%02X, // bControlSize\n"), *pLast++ );
							const int controlSize = *(pCur+14);
							_ftprintf_s( fp, _T("        ") );
							for ( int i = 0; i < controlSize; i++ )
							{
								_ftprintf_s( fp, _T("0x%02X, "), *pLast++ );
							}
							_ftprintf_s( fp, _T("// bmControls\n") );
						}
					}
					break;
				case 0x03: // VC_OUTPUT_TERMINAL
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VC_OUTPUT_TERMINAL\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTerminalID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wTerminalType\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, // bAssocTerminal\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bSourceID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // iTerminal\n"), *pLast++ );
					}
					break;
				case 0x05: // VC_PROCESSING_UNIT
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VC_PROCESSING_UNIT\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bUnitID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bSourceID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wMaxMultiplier\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, // bControlSize\n"), *pLast++ );
						_ftprintf_s( fp, _T("        ") );
						const int controlSize = *(pCur+7);
						for ( int i = 0; i < controlSize; i++ )
						{
							_ftprintf_s( fp, _T("0x%02X, "), *pLast++ );
						}
						_ftprintf_s( fp, _T("// bmControls\n") );
						_ftprintf_s( fp, _T("        0x%02X, // iProcessing\n"), *pLast++ );
					}
					break;
				case 0x06: // VC_EXTENSION_UNIT
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VC_EXTENSION_UNIT\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bUnitID\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // guidExtensionCode part1\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // guidExtensionCode part2\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // guidExtensionCode part3\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // guidExtensionCode part4\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, // bNumControls\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bNrInPins\n"), *pLast++ );
						const int nrInPins = *(pCur+21);
						for ( int i = 0; i < nrInPins; i++ )
						{
							_ftprintf_s( fp, _T("        0x%02X, // baSourceID[%d]\n"), *pLast++, i+1 );
						}
						_ftprintf_s( fp, _T("        0x%02X, // bControlSize\n"), *pLast++ );
						const int controlSize = *(pCur+22+nrInPins);
						_ftprintf_s( fp, _T("        ") );
						for ( int i = 0; i < controlSize; i++ )
						{
							_ftprintf_s( fp, _T("0x%02X, "), *pLast++ );
						}
						_ftprintf_s( fp, _T("// bmControls\n") );
						_ftprintf_s( fp, _T("        0x%02X, // iExtension\n"), *pLast++ );
					}
					break;
				default:
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: unknown\n"), *pLast++ );
					}
					break;
				}
			}
			else if(
				bInterfaceClass == 0x0E && // video interface class
				bInterfaceSubClass == 0x02 && // video streaming interface subclass
				bInterfaceProtocol == 0x00 )
			{
				switch (*pLast) {
				case 0x01: // VS_INPUT_HEADER
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VS_INPUT_HEADER\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bNumFormats\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wTotalLength\n"), *pLast, *(pLast+1) ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, // bEndpointAddress\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bmInfo\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTerminalLink\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bStillCaptureMethod\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTriggerSupport\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTriggerUsage\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bControlSize\n"), *pLast++ );

						const int numFormats = *(pCur+3);
						const int controlSize = *(pCur+12);
						ASSERT( 13 + numFormats * controlSize == *pCur );

						for ( int i = 0; i < numFormats; i++ )
						{
							_ftprintf_s( fp, _T("        ") );
							for ( int j = 0; j < controlSize; j++ )
							{
								_ftprintf_s( fp, _T("0x%02X, "), *pLast++ );
							}
							_ftprintf_s( fp, _T("// bmaControls[%d]\n"), i+1 );
						}
					}
					break;
				case 0x04: // VS_FORMAT_UNCOMPRESSED
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VS_FORMAT_UNCOMPRESSED\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bFormatIndex\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bNumFrameDescriptors\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // guidFormat part1\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // guidFormat part2\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // guidFormat part3\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // guidFormat part4\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, // bBitsPerPixel\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bDefaultFrameIndex\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bAspectRatioX\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bAspectRatioY\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bmInterlaceFlags\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bCopyProtect\n"), *pLast++ );
					}
					break;
				case 0x06: // VS_FORMAT_MJPEG
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VS_FORMAT_MJPEG\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bFormatIndex\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bNumFrameDescriptors\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bmFlags\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bDefaultFrameIndex\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bAspectRatioX\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bAspectRatioY\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bmInterlaceFlags\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bCopyProtect\n"), *pLast++ );
					}
					break;
				case 0x05: // VS_FRAME_UNCOMPRESSED
				case 0x07: // VS_FRAME_MJPEG
					{
						if (*pLast==0x05)
							_ftprintf_s( fp, _T("        0x%02X, // subtype: VS_FRAME_UNCOMPRESSED\n"), *pLast++ );
						else
							_ftprintf_s( fp, _T("        0x%02X, // subtype: VS_FRAME_MJPEG\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bFrameIndex\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bmCapabilities\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wWidth %d\n"), *pLast, *(pLast+1), *(WORD*)pLast ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wHeight %d\n"), *pLast, *(pLast+1), *(WORD*)pLast ); pLast+=2;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwMinBitRate\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwMaxBitRate\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwMaxVideoFrameBufferSize\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwDefaultFrameInterval\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						_ftprintf_s( fp, _T("        0x%02X, // bFrameIntervalType\n"), *pLast++ );
						const int frameIntervalType = *(pCur+25);
						if ( frameIntervalType == 0 )
						{
							_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwMinFrameInterval\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
							_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwMaxFrameInterval\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
							_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwFrameIntervalStep\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3) ); pLast+=4;
						}
						else
						{
							for ( int i = 0; i < frameIntervalType; i++ )
							{
								_ftprintf_s( fp, _T("        0x%02X, 0x%02X, 0x%02X, 0x%02X, // dwFrameInterval[%d]\n"), *(pLast+0), *(pLast+1), *(pLast+2), *(pLast+3), i+1 ); pLast+=4;
							}
						}
					}
					break;
				case 0x0D: // VS_COLORFORMAT
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: VS_COLORFORMAT\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bColorPrimaries\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bTransferCharacteristics\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bMatrixCoefficients\n"), *pLast++ );
					}
					break;
				default:
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: unknown\n"), *pLast++ );
					}
					break;
				}
			}
			break;
		case 0x25: // CS_ENDPOINT (class-specific endpoint descriptor)
			_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // Length, Class-specific Endpoint Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;

			if( bInterfaceClass == 0x0E && // video interface class
				bInterfaceSubClass == 0x01 && // video control interface subclass
				bInterfaceProtocol == 0x00 )
			{
				switch (*pLast) {
				case 0x03: // EP_INTERRUPT
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: EP_INTERRUPT\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wMaxTrasnferSize\n"), *pLast, *(pLast+1) ); pLast+=2;
					}
					break;
				default:
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: unknown\n"), *pLast++ );
					}
					break;
				}
			}
			else if (
				bInterfaceClass == 0x01 && // audio interface class
				bInterfaceSubClass == 0x02 && // audio streaming interface subclass
				bInterfaceProtocol == 0x00 )
			{
				switch (*pLast) {
				case 0x01: // EP_GENERAL
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: EP_GENERAL\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bmAttributes\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, // bLockDelayUnits\n"), *pLast++ );
						_ftprintf_s( fp, _T("        0x%02X, 0x%02X, // wLockDelay\n"), *pLast, *(pLast+1) ); pLast+=2;
					}
					break;
				default:
					{
						_ftprintf_s( fp, _T("        0x%02X, // subtype: unknown\n"), *pLast++ );
					}
					break;
				}
			}
			break;
		case 0x22: // REPORT
		case 0x23: // PHYSICAL
		default:
			{
				_ftprintf_s( fp, _T("    0x%02X, 0x%02X, // bLength, Device Descriptor\n"), *pLast, *(pLast+1) ); pLast+=2;
			}
			break;
		}

		LONG_PTR left = *pCur - (pLast - pCur);
		ASSERT(left>=0);
		if ( left > 0 )
		{
			_ftprintf_s( fp, _T("        ") );
			for ( LONG_PTR i = 0; i < left; i++ )
			{
				_ftprintf_s( fp, _T("0x%02X, "), *pLast++ );
			}
			_ftprintf_s( fp, _T("// no parsing...\n") );
		}

		pCur += *pCur;
	}

	ASSERT( pCur == pEnd );

	_ftprintf_s( fp, _T("}\n") );
	fclose(fp);

	//
	// open notepad to see
	//
	strCmdLine = achSysDir;
	strCmdLine += _T("notepad.exe ") + strSaveFile;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	// lpCommandLine
	// The Unicode version of this function, CreateProcessW, can modify the contents of this string.
	// Therefore, this parameter cannot be a pointer to read-only memory (such as a const variable or a literal string).
	// If this parameter is a constant string, the function may cause an access violation.

	BOOL b = ::CreateProcess(
		NULL,									// No module name (use command line)
		strCmdLine.GetBuffer(),					// Command line
		NULL,									// Process handle not inheritable
		NULL,									// Thread handle not inheritable
		FALSE,									// Set handle inheritance to FALSE
		0,										// No creation flags
		NULL,									// Use parent's environment block
		NULL,									// Use parent's starting directory
		&si,									// Pointer to STARTUPINFO structure
		&pi );									// Pointer to PROCESS_INFORMATION structure

	if (!b)
		return;

	// The calling thread can use the WaitForInputIdle function
	// to wait until the new process has finished its initialization
	// and is waiting for user input with no input pending.
	WaitForInputIdle( pi.hProcess, INFINITE );

	// Wait until child process exits.
	//WaitForSingleObject( pi.hProcess, INFINITE );

	// Handles in STARTUPINFO must be closed with CloseHandle when they are no longer needed.
	CloseHandle( si.hStdError );
	CloseHandle( si.hStdInput );
	CloseHandle( si.hStdOutput );

	// Handles in PROCESS_INFORMATION must be closed with CloseHandle when they are no longer needed.
	// Close process and thread handles.
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	// Find the opend notpad
	ghNotepadWnd = NULL;
	CWnd* pDesktopWnd = GetDesktopWindow();
	EnumChildWindows( pDesktopWnd->GetSafeHwnd(), EnumChildProc, pi.dwProcessId );
}

void CDevConWinDlg::OnBnClickedBtnRefresh()
{
	// TODO: Add your control notification handler code here
	UpdateData();
	switch ( m_bViewType ) {
	case 0:
		m_tcMgr.PopulateDevices();
		break;
	case 1:
		m_tcMgr.PopulateUsbDevices();
		break;
	default:
		ASSERT(FALSE);
		break;
	}
}

void CDevConWinDlg::OnBnClickedBtnViewType( UINT id )
{
	switch ( id ) {
	case IDC_DEVICE_VIEW:
		m_tcMgr.PopulateDevices();
		break;
	case IDC_USB_VIEW:
		m_tcMgr.PopulateUsbDevices();
		break;
	default:
		ASSERT(FALSE);
		break;
	}
}


