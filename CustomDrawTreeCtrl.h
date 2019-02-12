#pragma once


// CCustomDrawTreeCtrl

class CCustomDrawTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CCustomDrawTreeCtrl)

public:
	CCustomDrawTreeCtrl();
	virtual ~CCustomDrawTreeCtrl();

	CFont* m_pOldItemFont;

protected:
	afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

	virtual bool IsDraw() { return false; }
	virtual bool IsPostDraw() { return false; }

	virtual bool IsItemDraw(HTREEITEM /*htiItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }
	virtual bool IsItemPostDraw() { return false; }

	virtual bool IsNotifyItemDraw() { return false; }
	virtual bool IsNotifyItemPostPaint(HTREEITEM /*htiItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }
	virtual bool IsNotifyPostPaint() { return false; }

	virtual bool OnDraw(CDC* /*pDC*/, const CRect& /*rc*/) { return false; }
	virtual bool OnPostDraw(CDC* /*pDC*/, const CRect& /*rc*/) { return false; }

	virtual bool OnItemDraw(CDC* /*pDC*/, HTREEITEM /*htiItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }
	virtual bool OnItemPostDraw(CDC* /*pDC*/, HTREEITEM /*htiItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }

	virtual CFont* FontForItem(HTREEITEM /*htiItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return NULL; }
	virtual COLORREF TextColorForItem(HTREEITEM /*htiItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return CLR_DEFAULT; }
	virtual COLORREF BkColorForItem(HTREEITEM /*htiItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return CLR_DEFAULT; }

};


