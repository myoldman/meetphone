#pragma once


// Cmeetphonepassword 对话框

class Cmeetphonepassword : public CDialog
{
	DECLARE_DYNAMIC(Cmeetphonepassword)

public:
	Cmeetphonepassword(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~Cmeetphonepassword();

// 对话框数据
	enum { IDD = IDD_MEETPHONE_PASSWORD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_sUsername;
	CString m_sPassword;
};
