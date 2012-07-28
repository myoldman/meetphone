// TabCtrlEx.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "tabctrlex.h"

// CTabCtrlEx

CTabCtrlEx::CTabCtrlEx() 
{ 
	m_crSelColour           =   RGB(0,0,255); 
	m_crUnselColour       =   RGB(50,50,50); 
} 

CTabCtrlEx::~CTabCtrlEx() 
{ 
} 

BEGIN_MESSAGE_MAP(CTabCtrlEx,   CTabCtrl) 
	//{{AFX_MSG_MAP(CTabCtrlEx) 
	ON_WM_CREATE() 
	//}}AFX_MSG_MAP 
END_MESSAGE_MAP() 

///////////////////////////////////////////////////////////////////////////// 
//   CTabCtrlEx   message   handlers 

int   CTabCtrlEx::OnCreate(LPCREATESTRUCT   lpCreateStruct)   
{ 
	if   (CTabCtrl::OnCreate(lpCreateStruct)   ==   -1)   return   -1; 
	ModifyStyle(0,   TCS_OWNERDRAWFIXED); 

	return   0; 
} 

void   CTabCtrlEx::PreSubclassWindow()   
{ 
	CTabCtrl::PreSubclassWindow(); 
	ModifyStyle(0,   TCS_OWNERDRAWFIXED); 
} 

void   CTabCtrlEx::DrawItem(LPDRAWITEMSTRUCT   lpDrawItemStruct)   
{ 
	CRect   rect   =   lpDrawItemStruct-> rcItem; 
	int   nTabIndex   =   lpDrawItemStruct-> itemID; 
	if   (nTabIndex   <   0)   return; 
	BOOL   bSelected   =   (nTabIndex   ==   GetCurSel()); 

	WCHAR   label[64]; 
	TC_ITEM   tci; 
	tci.mask   =   TCIF_TEXT|TCIF_IMAGE; 
	tci.pszText   =   label;           
	tci.cchTextMax   =   63;         
	if   (!GetItem(nTabIndex,   &tci   ))   return; 

	CDC*   pDC   =   CDC::FromHandle(lpDrawItemStruct-> hDC); 
	if   (!pDC)   return; 
	int   nSavedDC   =   pDC-> SaveDC(); 

	//   For   some   bizarre   reason   the   rcItem   you   get   extends   above   the   actual 
	//   drawing   area.   We   have   to   workaround   this   "feature ". 
	rect.top   +=   ::GetSystemMetrics(SM_CYEDGE); 

	pDC-> SetBkMode(TRANSPARENT); 
	pDC-> FillSolidRect(rect,   ::GetSysColor(COLOR_BTNFACE)); 

	//   Draw   image 
	CImageList*   pImageList   =   GetImageList();
	
	if(pImageList != NULL && tci.iImage >= 0)
	{ 

		rect.left   +=   pDC-> GetTextExtent(_T( "   ")).cx; //   Margin 

		//   Get   height   of   image   so   we   
		IMAGEINFO   info; 
		pImageList-> GetImageInfo(tci.iImage,   &info); 
		CRect   ImageRect(info.rcImage); 
		int   nYpos   =   rect.top; 

		pImageList-> Draw(pDC,   tci.iImage,   CPoint(rect.left,   nYpos),   ILD_TRANSPARENT); 
		rect.left   +=   ImageRect.Width(); 
	} 
	//设置背景颜色： 
	//pDC-> SetBkColor(RGB(255,0,0,));//将背景色设置成红色 
	if   (bSelected)   { 

		pDC-> SetTextColor(m_crSelColour); 
		rect.top   -=   ::GetSystemMetrics(SM_CYEDGE); 
		pDC-> DrawText(label,   rect,   DT_SINGLELINE|DT_VCENTER|DT_CENTER); 
	}   else   { 
		pDC-> SetTextColor(m_crUnselColour); 
		pDC-> DrawText(label,   rect,   DT_SINGLELINE|DT_BOTTOM|DT_CENTER); 
	} 

	pDC-> RestoreDC(nSavedDC); 
	
} 

///////////////////////////////////////////////////////////////////////////// 
//   CTabCtrlEx   operations 

void   CTabCtrlEx::SetColours(COLORREF   bSelColour,   COLORREF   bUnselColour) 
{ 
	m_crSelColour   =   bSelColour; 
	m_crUnselColour   =   bUnselColour; 
	Invalidate(); 
}


// CTabCtrlEx 消息处理程序


