// DevConWinDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "DevMgr.h"
#include "afxwin.h"
#include "CustomDrawTreeCtrl.h"

enum DEVICE_STATUS
{
	DEVICE_STATUS_UNKNOWN,
	DEVICE_STATUS_DISABLED,
	DEVICE_STATUS_HAS_PROBLEM,
	DEVICE_STATUS_DRIVER_ERROR,
	DEVICE_STATUS_RUNNING,
	DEVICE_STATUS_STOPPED,
	DEVICE_STATUS_MAX,
};

// tree item flag
#define TIF_MASK				0xF0000000
#define TIF_UNDEFINED			0x00000000
#define TIF_CLASS				0x10000000
#define TIF_DEVICE				0x20000000
#define TIF_USBHUB				0x30000000
#define TIF_USBDEVICE			0x40000000

#define TIF_CLASS_INDEX_MASK	0x00000FFF	// 0 - 4095 classes in one machine
#define TIF_CLASS_EXPANDED_MASK	0x00001000	// 0: expanded, 1: not expanded

#define TIF_DEVICE_STATUS_MASK	0x0F000000
#define TIF_DEVICE_CLASS_MASK	0x00FFF000
#define TIF_DEVICE_INDEX_MASK	0x00000FFF	// 0 - 4095 devices in one class

#define TIF_USBDEVICE_CONNECT_INDEX_MASK	0x000000FF
#define TIF_USBDEVICE_CONNECT_STATUS_MASK	0x0000FF00

typedef VOID (WINAPI *PHANDLE_MESSAGES_CALLBACK)(LPCTSTR lpszMsg, LPVOID lpContext );
typedef PHANDLE_MESSAGES_CALLBACK LPHANDLE_MESSAGES_CALLBACK;

class CDeviceTree : public CCustomDrawTreeCtrl
{
	DECLARE_DYNAMIC(CDeviceTree)

	static CImageList sm_ilDeviceIcons;
	static CImageList sm_ilUsbViewIcons;

	static int AddSysImage( int index, CImageList& il );

	static int PrepareImages( int index );

	COLORREF m_acrDevStatus[DEVICE_STATUS_MAX];
	std::vector<HDEVNOTIFY> m_vDevNotify;

	LPHANDLE_MESSAGES_CALLBACK m_pfnMsgCb;
	LPVOID m_pMsgCbContext;

public:
	CDeviceTree();
	~CDeviceTree();

	void SetMsgCb( LPHANDLE_MESSAGES_CALLBACK pfn, LPVOID pContext );

	void Clear();
	void PopulateDevices();
	void PopulateUsbDevices();

	DEVICE_STATUS GetDeviceStatus(const CDevice& dev,CString& strStatus);
	void RefreshClass( HTREEITEM htiNode, const CDeviceSet& ds, int nClassIndex );
	void RefreshDevice( HTREEITEM htiNode, const CDevice& dev );

	void DeleteChildItems( HTREEITEM htiNode );

	afx_msg BOOL OnDeviceChange(UINT nEventType,DWORD_PTR dwData);
	afx_msg void OnItemExpanded(NMHDR *pNMHDR, LRESULT *pResult);

protected:
	DECLARE_MESSAGE_MAP()

	virtual bool IsNotifyItemDraw() { return true; }
	virtual COLORREF TextColorForItem(HTREEITEM htiItem, UINT nState, LPARAM lParam)
	{
		DWORD_PTR dwptr = GetItemData(htiItem);
		DWORD dwType = ((DWORD)dwptr & TIF_MASK);
		switch (dwType) {
		case TIF_CLASS:
			return RGB(0,128,192);
		case TIF_DEVICE:
			{
				DWORD dwStatus = ((DWORD)dwptr&TIF_DEVICE_STATUS_MASK)>>24;
				return m_acrDevStatus[dwStatus];
			}
			break;
		default:
			return GetSysColor(COLOR_WINDOWTEXT);
		}
	}
	virtual COLORREF BkColorForItem(HTREEITEM htiItem, UINT nState, LPARAM lParam)
	{
		if ( (nState & CDIS_SELECTED) == CDIS_SELECTED )
		{
			return RGB( 240, 240, 240 );
		}
		else
			return RGB(255,255,255);//GetSysColor(COLOR_WINDOW);
	}
};


// CDevConWinDlg dialog
class CDevConWinDlg : public CDialog
{
// Construction
public:
	CDevConWinDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_DEVCONWIN_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CDeviceTree	m_tcMgr;

	afx_msg void OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnEnable();
	afx_msg void OnBnClickedBtnDisable();
	afx_msg void OnBnClickedBtnFilter();
	afx_msg void OnBnClickedBtnDescriptors();
	afx_msg void OnBnClickedBtnRefresh();

	CComboBox m_cbMsg;
	void ShowMsg( LPCTSTR lpszMsg, ... );

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

	CToolTipCtrl m_tip;
	afx_msg BOOL OnToolTipNotify( UINT id, NMHDR * pNMHDR, LRESULT * pResult );
	afx_msg void OnCbnDropdownCbMsg();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	BOOL m_bViewType;
	afx_msg void OnBnClickedBtnViewType( UINT id );
};
