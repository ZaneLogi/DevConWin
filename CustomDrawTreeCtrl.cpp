// CustomDrawTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "CustomDrawTreeCtrl.h"


// CCustomDrawTreeCtrl

IMPLEMENT_DYNAMIC(CCustomDrawTreeCtrl, CTreeCtrl)
CCustomDrawTreeCtrl::CCustomDrawTreeCtrl()
{
}

CCustomDrawTreeCtrl::~CCustomDrawTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CCustomDrawTreeCtrl, CTreeCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()



// CCustomDrawTreeCtrl message handlers

void CCustomDrawTreeCtrl::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMTVCUSTOMDRAW* pNMTVCUSTOMDRAW = (NMTVCUSTOMDRAW*)pNMHDR;

	HDC  hdc = pNMTVCUSTOMDRAW->nmcd.hdc;
	CDC* pDC = NULL;

	HTREEITEM htiItem = (HTREEITEM)pNMTVCUSTOMDRAW->nmcd.dwItemSpec;
	UINT      nState  = pNMTVCUSTOMDRAW->nmcd.uItemState;
	LPARAM    lParam  = pNMTVCUSTOMDRAW->nmcd.lItemlParam;

	bool bNotifyPostPaint = false;
	bool bNotifyItemDraw = false;
	bool bSkipDefault = false;
	bool bNewFont = false;

	switch (pNMTVCUSTOMDRAW->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		{
			m_pOldItemFont = NULL;

			bNotifyPostPaint = IsNotifyPostPaint();
			bNotifyItemDraw = IsNotifyItemDraw();
  
			if (IsDraw()) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				CRect r(pNMTVCUSTOMDRAW->nmcd.rc);

				if (OnDraw(pDC,r)) {
					bSkipDefault = true;
				}
			}
		}
		break;
	case CDDS_ITEMPREPAINT:
		{
			m_pOldItemFont = NULL;

			bNotifyPostPaint = IsNotifyItemPostPaint(htiItem,nState,lParam);

			pNMTVCUSTOMDRAW->clrText = TextColorForItem(htiItem,nState,lParam);
			pNMTVCUSTOMDRAW->clrTextBk = BkColorForItem(htiItem,nState,lParam);

			// set up a different font to use, if any
			CFont* pNewFont = FontForItem(htiItem,nState,lParam);
			if (pNewFont) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				m_pOldItemFont = pDC->SelectObject(pNewFont);
				bNotifyPostPaint = true; // need to restore font
			}

			if (IsItemDraw(htiItem,nState,lParam)) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				if (OnItemDraw(pDC,htiItem,nState,lParam)) {
					bSkipDefault = true;
				}
			}
		}
		break;
	case CDDS_ITEMPOSTPAINT:
		{
			if (m_pOldItemFont) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				pDC->SelectObject(m_pOldItemFont);
				m_pOldItemFont = NULL;
			}

			// do we want to do any extra drawing?
			if (IsItemPostDraw()) {
				if (! pDC) pDC = CDC::FromHandle(hdc);
				OnItemPostDraw(pDC,htiItem,nState,lParam);
			}
		}
		break;
	case CDDS_POSTPAINT:
		{
			if (IsPostDraw()) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				CRect r(pNMTVCUSTOMDRAW->nmcd.rc);
				OnPostDraw(pDC,r);
			}
		}
		break;
	}

	ASSERT(CDRF_DODEFAULT==0);
	*pResult = 0;

	if (bNotifyPostPaint) {
		*pResult |= CDRF_NOTIFYPOSTPAINT;
	}

	if (bNotifyItemDraw) {
		*pResult |= CDRF_NOTIFYITEMDRAW;
	}

	if (bNewFont) {
		*pResult |= CDRF_NEWFONT;
	}

	if (bSkipDefault) {
		*pResult |= CDRF_SKIPDEFAULT;
	}

	if (*pResult == 0) {
		// redundant as CDRF_DODEFAULT==0 anyway
		// but shouldn't depend on this in our code
		*pResult = CDRF_DODEFAULT;
	}
}
