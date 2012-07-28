#pragma once
#include "afxwin.h"
#include "PictureEx.h"


// Cmeetphonemember 对话框

class Cmeetphonemember : public CDialog
{
	DECLARE_DYNAMIC(Cmeetphonemember)

public:
	Cmeetphonemember(CWnd* pParent = NULL);
	virtual ~Cmeetphonemember();

// 对话框数据
	enum { IDD = IDD_MEETPHONE_MEMBER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg LONG OnMemberStopLoadingMsg(WPARAM wP,LPARAM lP);
	CString m_sMemberName;
private:
	CStatic m_hMemberName;
	CPictureEx m_hLoading;
};
