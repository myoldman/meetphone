#pragma once
#include "afxwin.h"


// Cmeetphonemultimedia 对话框

class Cmeetphonemultimedia : public CDialog
{
	DECLARE_DYNAMIC(Cmeetphonemultimedia)

public:
	Cmeetphonemultimedia(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~Cmeetphonemultimedia();

// 对话框数据
	enum { IDD = IDD_MEETPHONE_MULTIMEDIA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonPlay();
	virtual BOOL OnInitDialog();
protected:
	virtual void OnOK();
	virtual void OnCancel();
public:
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnCbnSelchangeComboWebcams();
	afx_msg void OnCbnSelchangeComboVideoSize();
	afx_msg void OnBnClickedCheckEchocancel();
private:
	CComboBox m_hComboPlayBack;
	CComboBox m_hComboRing;
	CComboBox m_hComboCapture;
	CEdit m_hEditRing;
	CButton m_hCheckEchoCancel;
	CButton m_hButtonBrowse;
	CButton m_hButtonPlay;
	CComboBox m_hComboWebcams;
	CComboBox m_hComboVideoSize;
	int m_iLastWebcam;
};
