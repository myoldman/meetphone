#pragma once

// CListCtrlEx
class CListCtrlEx : public CListCtrl
{
    DECLARE_DYNAMIC(CListCtrlEx)

public:
    CListCtrlEx();
    virtual ~CListCtrlEx();

    bool IsSelected(int index);
    BOOL SelectDropTarget(int item);
	bool AddHandOverColumn(int column);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnStateChanged(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);

private:
	CList<int,int> m_HandOverList;
	HCURSOR m_hCursor;
};