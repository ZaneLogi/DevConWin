// FilterSettingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DevConWin.h"
#include "FilterSettingDlg.h"


// CFilterSettingDlg dialog

IMPLEMENT_DYNAMIC(CFilterSettingDlg, CDialog)

CFilterSettingDlg::CFilterSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilterSettingDlg::IDD, pParent)
{

}

CFilterSettingDlg::~CFilterSettingDlg()
{
}

void CFilterSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_LIST, m_tabList);
	DDX_Control(pDX, IDC_LIST1, m_lbFilters);
	DDX_Control(pDX, IDC_BTN_ADD, m_btnAdd);
	DDX_Control(pDX, IDC_BTN_REMOVE, m_btnRemove);
	DDX_Control(pDX, IDC_EC_NAME, m_ecName);
}


BEGIN_MESSAGE_MAP(CFilterSettingDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_LIST, &CFilterSettingDlg::OnTcnSelchangeTabList)
	ON_LBN_SELCHANGE(IDC_LIST1, &CFilterSettingDlg::OnLbnSelchangeList1)
	ON_EN_CHANGE(IDC_EC_NAME, &CFilterSettingDlg::OnEnChangeEcName)
	ON_BN_CLICKED(IDC_BTN_ADD, &CFilterSettingDlg::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDC_BTN_REMOVE, &CFilterSettingDlg::OnBnClickedBtnRemove)
END_MESSAGE_MAP()


// CFilterSettingDlg message handlers

BOOL CFilterSettingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	m_tabList.InsertItem( 0, _T("Lower Filters") );
	m_tabList.InsertItem( 1, _T("Upper Filters") );

	m_btnAdd.EnableWindow(FALSE);
	m_btnRemove.EnableWindow(FALSE);

	UpdateListBox( &m_mstrLowerFilters );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CFilterSettingDlg::OnTcnSelchangeTabList(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;

	int nTabSel = m_tabList.GetCurSel();
	if ( nTabSel < 0 )
		return;

	UpdateListBox( (nTabSel == 0)
		? &m_mstrLowerFilters
		: &m_mstrUpperFilters );

}

void CFilterSettingDlg::OnLbnSelchangeList1()
{
	// TODO: Add your control notification handler code here
	int nSel = m_lbFilters.GetCurSel();
	CString s;
	m_lbFilters.GetText(nSel,s);
	m_ecName.SetWindowText(s);
	m_btnAdd.EnableWindow();
	m_btnRemove.EnableWindow();
}

void CFilterSettingDlg::OnEnChangeEcName()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	m_btnAdd.EnableWindow( m_ecName.GetWindowTextLength() > 0 );
}

void CFilterSettingDlg::OnBnClickedBtnAdd()
{
	// TODO: Add your control notification handler code here
	int nTabSel = m_tabList.GetCurSel();
	if ( nTabSel < 0 )
		return;

	CString s;
	m_ecName.GetWindowText(s);

	CMultiSzString* pMStr = (nTabSel == 0)
		? &m_mstrLowerFilters
		: &m_mstrUpperFilters;

	pMStr->Append( s );

	m_lbFilters.AddString(s);
}

void CFilterSettingDlg::OnBnClickedBtnRemove()
{
	// TODO: Add your control notification handler code here
	m_btnRemove.EnableWindow(FALSE);
	m_ecName.SetWindowText(_T(""));

	int nTabSel = m_tabList.GetCurSel();
	if ( nTabSel < 0 )
		return;

	int nLbSel = m_lbFilters.GetCurSel();
	if ( nLbSel < 0 )
		return;

	CString s;
	m_lbFilters.GetText( nLbSel, s );

	CMultiSzString* pMStr = (nTabSel == 0)
		? &m_mstrLowerFilters
		: &m_mstrUpperFilters;

	pMStr->Remove( s );

	m_lbFilters.DeleteString(nLbSel);
}

void CFilterSettingDlg::UpdateListBox( CMultiSzString* pMStr )
{
	m_lbFilters.ResetContent();

	LPCTSTR p = pMStr->GetFirst();
	while ( p )
	{
		m_lbFilters.AddString(p);
		p = pMStr->GetNext();
	}
}
