#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// Cmeetphonenetwork 对话框

class Cmeetphonenetwork : public CDialog
{
	DECLARE_DYNAMIC(Cmeetphonenetwork)

public:
	Cmeetphonenetwork(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~Cmeetphonenetwork();

// 对话框数据
	enum { IDD = IDD_MEETPHONE_NETWORK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedTransportSetting();
	afx_msg void OnCbnSelchangeComboProtocol();
	afx_msg void OnEnChangeEditProtoPort();
	afx_msg void OnEnChangeEditVideoRtpPort();
	afx_msg void OnEnChangeEditAudioRtpPort();
	afx_msg void OnEnChangeEditRtspVideoRtpPort();
	afx_msg void OnEnChangeEditRtspAudioRtpPort();
	afx_msg void OnBnClickedRadioNoNat();
	afx_msg void OnBnClickedRadioUseNat();
	afx_msg void OnBnClickedRadioUseStun();
	afx_msg void OnEnChangeEditNatAddress();
	afx_msg void OnEnChangeEditStunServer();
protected:
	virtual void OnCancel();
	virtual void OnOK();

private:
	CButton m_hRadioNoNat;
	CEdit m_hEditNatAddress;
	CEdit m_hEditStunServer;
	CEdit m_hEditProtoPort;
	CEdit m_hEditVideoRtpPort;
	CSpinButtonCtrl m_hSpinVideoRtpPort;
	CSpinButtonCtrl m_hSpinAudioRtpPort;
	CSpinButtonCtrl m_hSpinRtspVideoRtpPort;
	CSpinButtonCtrl m_hSpinRtspAudioRtpPort;
	CComboBox m_hComboProtocol;
	CSpinButtonCtrl m_hSpinProtoPort;
};
