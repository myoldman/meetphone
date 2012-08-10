#pragma once
#include "afxwin.h"
#include "meetphonemember.h"
#include "afxcmn.h"
#include "listctrlex.h"

// Cmeetphonemeet 对话框

class Cmeetphonemeet : public CDialog
{
	DECLARE_DYNAMIC(Cmeetphonemeet)

public:
	Cmeetphonemeet(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~Cmeetphonemeet();

// 对话框数据
	enum { IDD = IDD_MEETPHONE_MEET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg LONG OnMemberMaximizeMsg(WPARAM wP,LPARAM lP);
	afx_msg LONG OnMemberRestoreMsg(WPARAM wP,LPARAM lP);
	afx_msg LONG OnMemberReloadMsg(WPARAM wP,LPARAM lP);
	afx_msg LRESULT OnMemberAdd(WPARAM wP,LPARAM lP);
	afx_msg LRESULT OnMemberDelete(WPARAM wP,LPARAM lP);
	afx_msg LRESULT OnMemberPreviewHwnd(WPARAM wP,LPARAM lP);
private:
	void InitMemberList();
	BOOL ReloadMemberList();
	HWND AddMeetMember(CString &memberName);
	HWND DeleteMeetMember(CString &memberName);

private:
	CList<Cmeetphonemember*, Cmeetphonemember*&> m_ListMember;
	CListCtrlEx m_hListMember;
	CStatic m_hStaticLocal;
	Cmeetphonemember *m_hLocalView;
	CImageList  m_hActionImage;
public:
	CString m_sConfUID;
	std::string m_sConfDID;
	int m_iPartId;
	afx_msg void OnLvnDeleteitemListMember(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickListMember(NMHDR *pNMHDR, LRESULT *pResult);
};
