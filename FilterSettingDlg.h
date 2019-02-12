#pragma once

#include "MultiSzString.h"
#include "afxcmn.h"
#include "afxwin.h"

// CFilterSettingDlg dialog

class CFilterSettingDlg : public CDialog
{
	DECLARE_DYNAMIC(CFilterSettingDlg)

	void UpdateListBox( CMultiSzString* pMStr );

public:
	CMultiSzString m_mstrUpperFilters;
	CMultiSzString m_mstrLowerFilters;

	CFilterSettingDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFilterSettingDlg();

// Dialog Data
	enum { IDD = IDD_FILTER_SETTING_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CTabCtrl m_tabList;
	CListBox m_lbFilters;

	virtual BOOL OnInitDialog();
	afx_msg void OnTcnSelchangeTabList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLbnSelchangeList1();
	CButton m_btnAdd;
	CButton m_btnRemove;
	CEdit m_ecName;
	afx_msg void OnEnChangeEcName();
	afx_msg void OnBnClickedBtnAdd();
	afx_msg void OnBnClickedBtnRemove();
};
