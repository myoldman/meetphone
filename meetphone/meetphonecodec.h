#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// Cmeetphonecodec 对话框

class Cmeetphonecodec : public CDialog
{
	DECLARE_DYNAMIC(Cmeetphonecodec)

public:
	Cmeetphonecodec(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~Cmeetphonecodec();

// 对话框数据
	enum { IDD = IDD_MEETPHONE_CODEC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	virtual void OnOK();
public:
	virtual BOOL OnInitDialog();
private:
	void DrawCodecList(int type);
	void MoveCodec(int dir);
	void CodecEnable(BOOL enable);
public:
	afx_msg void OnCbnSelchangeComboCodecView();
	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonEnable();
	afx_msg void OnBnClickedButtonDisable();
	afx_msg void OnEnChangeEditDownloadBw();
	afx_msg void OnEnChangeEditUploadBw();
	afx_msg void OnBnClickedCheckAdaptiveRate();

	afx_msg void OnNMCustomdrawListCodec(NMHDR *pNMHDR, LRESULT *pResult);
private:
	CEdit m_hEditDownloadBw;
	CEdit m_hEditUploadBw;
	CButton m_hCheckAdaptiveRate;
	CSpinButtonCtrl m_hSpinDownloadBw;
	CSpinButtonCtrl m_hSpinUploadBw;
	CComboBox m_hComboCodecView;
	CListCtrl m_hListCodec;

};
