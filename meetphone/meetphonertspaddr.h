#pragma once


// Cmeetphonertspaddr 对话框

class Cmeetphonertspaddr : public CDialog
{
	DECLARE_DYNAMIC(Cmeetphonertspaddr)

public:
	Cmeetphonertspaddr(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~Cmeetphonertspaddr();

// 对话框数据
	enum { IDD = IDD_MEETPHONE_RTSP_ADDR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CString m_sRtspAddr;
};
