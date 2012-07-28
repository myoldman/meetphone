#include "stdafx.h"
#include "ListCtrlEx.h"

// CListCtrlEx
IMPLEMENT_DYNAMIC(CListCtrlEx, CListCtrl)

CListCtrlEx::CListCtrlEx()
{
	m_hCursor = LoadCursor(NULL,   IDC_HAND);
}

CListCtrlEx::~CListCtrlEx()
{
}

BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
    ON_WM_MOUSEMOVE()

    ON_NOTIFY_REFLECT(LVN_ODSTATECHANGED, &CListCtrlEx::OnStateChanged)
END_MESSAGE_MAP()

// CListCtrlEx message handlers

void CListCtrlEx::OnMouseMove(UINT nFlags, CPoint point)
{
	DWORD dwPos=GetMessagePos();
	CPoint point1( LOWORD(dwPos), HIWORD(dwPos));

	ScreenToClient(&point1);


	LVHITTESTINFO lvinfo;
	lvinfo.pt=point;
	lvinfo.flags=LVHT_ABOVE;


	int nItem=SubItemHitTest(&lvinfo);
	if(nItem!=-1  && lvinfo.iSubItem >= 0 && m_HandOverList.Find(lvinfo.iSubItem))
	{
		SetCursor(m_hCursor);
	}

    CListCtrl::OnMouseMove(nFlags, point);
}

bool CListCtrlEx::IsSelected(int index)
{
    return (GetItemState(index, LVIS_SELECTED) & LVIS_SELECTED) != 0;
}

// highlight drop targets sort of like CTreeCtrl
BOOL CListCtrlEx::SelectDropTarget(int item)
{
    static int prevHighlight(-1);
    if (item >= 0 && item < GetItemCount())
    {
        if (item != prevHighlight)
        {
            if (prevHighlight >= 0)
            {
                SetItemState(prevHighlight, 0, LVIS_DROPHILITED); // remove highlight from previous target
                RedrawItems(prevHighlight, prevHighlight);
            }

            prevHighlight = item;
            SetItemState(item, LVIS_DROPHILITED, LVIS_DROPHILITED); // highlight target
            RedrawItems(item, item);

            UpdateWindow();
            return TRUE;
        }
    }
    else
    {
        for (int i(0); i < GetItemCount(); ++i)
            SetItemState(i, 0, LVIS_DROPHILITED); // un-highlight all
        prevHighlight = -1;
    }

    return FALSE;
}

void CListCtrlEx::OnStateChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
// MSDN:
// If a list-view control has the LVS_OWNERDATA style,
// and the user selects a range of items by holding down the SHIFT key and clicking the mouse,
// LVN_ITEMCHANGED notification codes are not sent for each selected or deselected item.
// Instead, you will receive a single LVN_ODSTATECHANGED notification code,
// indicating that a range of items has changed state.

    NMLVODSTATECHANGE* pStateChanged = (NMLVODSTATECHANGE*)pNMHDR;

    // redraw newly selected items
    if (pStateChanged->uNewState == LVIS_SELECTED)
        RedrawItems(pStateChanged->iFrom, pStateChanged->iTo);
}

bool CListCtrlEx::AddHandOverColumn(int column)
{
	m_HandOverList.AddTail(column);
	return true;
}