// meetphonenetwork.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonenetwork.h"


// Cmeetphonenetwork 对话框

IMPLEMENT_DYNAMIC(Cmeetphonenetwork, CDialog)

Cmeetphonenetwork::Cmeetphonenetwork(CWnd* pParent /*=NULL*/)
	: CDialog(Cmeetphonenetwork::IDD, pParent)
{

}

Cmeetphonenetwork::~Cmeetphonenetwork()
{
}

void Cmeetphonenetwork::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN_PROTO_PORT, m_hSpinProtoPort);
	DDX_Control(pDX, IDC_EDIT_PROTO_PORT, m_hEditProtoPort);
	DDX_Control(pDX, IDC_EDIT_VIDEO_RTP_PORT, m_hEditVideoRtpPort);
	DDX_Control(pDX, IDC_SPIN_VIDEO_RTP_PORT, m_hSpinVideoRtpPort);
	DDX_Control(pDX, IDC_SPIN_AUDIO_RTP_PORT, m_hSpinAudioRtpPort);
	DDX_Control(pDX, IDC_SPIN_RTSP_VIDEO_RTP_PORT, m_hSpinRtspVideoRtpPort);
	DDX_Control(pDX, IDC_SPIN_RTSP_AUDIO_RTP_PORT, m_hSpinRtspAudioRtpPort);
	DDX_Control(pDX, IDC_COMBO_PROTOCOL, m_hComboProtocol);
	DDX_Control(pDX, IDC_RADIO_NO_NAT, m_hRadioNoNat);
	DDX_Control(pDX, IDC_EDIT_NAT_ADDRESS, m_hEditNatAddress);
	DDX_Control(pDX, IDC_EDIT_STUN_SERVER, m_hEditStunServer);
}


BEGIN_MESSAGE_MAP(Cmeetphonenetwork, CDialog)
	ON_BN_CLICKED(IDC_TRANSPORT_SETTING, &Cmeetphonenetwork::OnBnClickedTransportSetting)
	ON_CBN_SELCHANGE(IDC_COMBO_PROTOCOL, &Cmeetphonenetwork::OnCbnSelchangeComboProtocol)
	ON_EN_CHANGE(IDC_EDIT_PROTO_PORT, &Cmeetphonenetwork::OnEnChangeEditProtoPort)
	ON_EN_CHANGE(IDC_EDIT_VIDEO_RTP_PORT, &Cmeetphonenetwork::OnEnChangeEditVideoRtpPort)
	ON_EN_CHANGE(IDC_EDIT_AUDIO_RTP_PORT, &Cmeetphonenetwork::OnEnChangeEditAudioRtpPort)
	ON_EN_CHANGE(IDC_EDIT_RTSP_VIDEO_RTP_PORT, &Cmeetphonenetwork::OnEnChangeEditRtspVideoRtpPort)
	ON_EN_CHANGE(IDC_EDIT_RTSP_AUDIO_RTP_PORt, &Cmeetphonenetwork::OnEnChangeEditRtspAudioRtpPort)
	ON_BN_CLICKED(IDC_RADIO_NO_NAT, &Cmeetphonenetwork::OnBnClickedRadioNoNat)
	ON_BN_CLICKED(IDC_RADIO_USE_NAT, &Cmeetphonenetwork::OnBnClickedRadioUseNat)
	ON_BN_CLICKED(IDC_RADIO_USE_STUN, &Cmeetphonenetwork::OnBnClickedRadioUseStun)
	ON_EN_CHANGE(IDC_EDIT_NAT_ADDRESS, &Cmeetphonenetwork::OnEnChangeEditNatAddress)
	ON_EN_CHANGE(IDC_EDIT_STUN_SERVER, &Cmeetphonenetwork::OnEnChangeEditStunServer)
//	ON_EN_UPDATE(IDC_EDIT_NAT_ADDRESS, &Cmeetphonenetwork::OnEnUpdateEditNatAddress)
//	ON_EN_UPDATE(IDC_EDIT_STUN_SERVER, &Cmeetphonenetwork::OnEnUpdateEditStunServer)
END_MESSAGE_MAP()


// Cmeetphonenetwork 消息处理程序

void Cmeetphonenetwork::OnBnClickedTransportSetting()
{
	// TODO: 在此添加控件通知处理程序代码
}

void Cmeetphonenetwork::OnCbnSelchangeComboProtocol()
{
	// TODO: 在此添加控件通知处理程序代码
}

BOOL Cmeetphonenetwork::OnInitDialog()
{
	CDialog::OnInitDialog();
	LinphoneCore *lc = theApp.GetCore();
	LCSipTransports tr;
	LinphoneFirewallPolicy pol;
	const char *tmp;
	if(lc == NULL)
	{
		return FALSE;
	}
	m_hSpinProtoPort.SetRange32(1024, 60000);
	m_hSpinProtoPort.SetPos32(linphone_core_get_sip_port(lc));
	m_hSpinVideoRtpPort.SetRange32(1024, 60000);
	m_hSpinVideoRtpPort.SetPos32(linphone_core_get_video_port(lc));
	m_hSpinAudioRtpPort.SetRange32(1024, 60000);
	m_hSpinAudioRtpPort.SetPos32(linphone_core_get_audio_port(lc));
	m_hSpinRtspVideoRtpPort.SetRange32(1024, 60000);
	m_hSpinRtspVideoRtpPort.SetPos32(linphone_core_get_video_rtsp_port(lc));
	m_hSpinRtspAudioRtpPort.SetRange32(1024, 60000);
	m_hSpinRtspAudioRtpPort.SetPos32(linphone_core_get_audio_rtsp_port(lc));
	linphone_core_get_sip_transports(lc,&tr);
    if (tr.tcp_port > 0) {
		m_hSpinProtoPort.SetPos32(tr.tcp_port);
		m_hComboProtocol.SetCurSel(1);
    }
    else if (tr.tls_port > 0) {
        m_hSpinProtoPort.SetPos32(tr.tls_port);
		m_hComboProtocol.SetCurSel(2);
    }
    else {
		m_hSpinProtoPort.SetPos32(tr.udp_port);
		m_hComboProtocol.SetCurSel(0);
    }
	tmp=linphone_core_get_nat_address(lc);
	if (tmp) m_hEditNatAddress.SetWindowText(CString(tmp));
	tmp=linphone_core_get_stun_server(lc);
	if (tmp) m_hEditStunServer.SetWindowText(CString(tmp));

	pol=linphone_core_get_firewall_policy(lc);
	switch(pol){
		case LinphonePolicyNoFirewall:
			((CButton*)GetDlgItem(IDC_RADIO_NO_NAT))->SetCheck(TRUE);
		break;
		case LinphonePolicyUseNatAddress:
			((CButton*)GetDlgItem(IDC_RADIO_USE_NAT))->SetCheck(TRUE);
		break;
		case LinphonePolicyUseStun:
			((CButton*)GetDlgItem(IDC_RADIO_USE_STUN))->SetCheck(TRUE);
		break;
	}

	return TRUE;
}

void Cmeetphonenetwork::OnEnChangeEditProtoPort()
{
	LinphoneCore *lc = theApp.GetCore();
	if(m_hSpinProtoPort.m_hWnd != NULL)
	{
		LCSipTransports tr;
		linphone_core_get_sip_transports(lc,&tr);
		int protocol = m_hComboProtocol.GetCurSel();
		if (protocol == 1) {
			tr.tcp_port = m_hSpinProtoPort.GetPos32();
			tr.udp_port = 0;
			tr.tls_port = 0;
		}
		else if ( protocol == 0 ) {
			tr.udp_port = m_hSpinProtoPort.GetPos32();
        	tr.tcp_port = 0;
			tr.tls_port = 0;
		}
		else if (protocol == 2){
			tr.udp_port = 0;
        	tr.tcp_port = 0;
			tr.tls_port = m_hSpinProtoPort.GetPos32();
		}
		linphone_core_set_sip_transports(lc,&tr);
	}
}

void Cmeetphonenetwork::OnCancel()
{
	//CDialog::OnCancel();
}

void Cmeetphonenetwork::OnOK()
{
	//CDialog::OnOK();
}

void Cmeetphonenetwork::OnEnChangeEditVideoRtpPort()
{
	LinphoneCore *lc = theApp.GetCore();
	if(m_hSpinVideoRtpPort.m_hWnd != NULL)
		linphone_core_set_video_port(lc, m_hSpinVideoRtpPort.GetPos32());
}

void Cmeetphonenetwork::OnEnChangeEditAudioRtpPort()
{
	LinphoneCore *lc = theApp.GetCore();
	if(m_hSpinAudioRtpPort.m_hWnd != NULL)
		linphone_core_set_audio_port(lc, m_hSpinAudioRtpPort.GetPos32());
}

void Cmeetphonenetwork::OnEnChangeEditRtspVideoRtpPort()
{
	LinphoneCore *lc = theApp.GetCore();
	if(m_hSpinRtspVideoRtpPort.m_hWnd != NULL)
		linphone_core_set_video_rtsp_port(lc, m_hSpinRtspVideoRtpPort.GetPos32());
}

void Cmeetphonenetwork::OnEnChangeEditRtspAudioRtpPort()
{
	LinphoneCore *lc = theApp.GetCore();
	if(m_hSpinRtspAudioRtpPort.m_hWnd != NULL)
		linphone_core_set_audio_rtsp_port(lc, m_hSpinRtspAudioRtpPort.GetPos32());
}


void Cmeetphonenetwork::OnBnClickedRadioNoNat()
{
	LinphoneCore *lc = theApp.GetCore();
	linphone_core_set_firewall_policy(lc,LinphonePolicyNoFirewall);
}

void Cmeetphonenetwork::OnBnClickedRadioUseNat()
{
	LinphoneCore *lc = theApp.GetCore();
	linphone_core_set_firewall_policy(lc,LinphonePolicyUseNatAddress);
}

void Cmeetphonenetwork::OnBnClickedRadioUseStun()
{
	LinphoneCore *lc = theApp.GetCore();
	linphone_core_set_firewall_policy(lc,LinphonePolicyUseStun);
}

void Cmeetphonenetwork::OnEnChangeEditNatAddress()
{
	LinphoneCore *lc = theApp.GetCore();
	CString tmp;
	m_hEditNatAddress.GetWindowText(tmp);
	CW2A szServer(tmp);
	linphone_core_set_nat_address(lc,szServer);
}

void Cmeetphonenetwork::OnEnChangeEditStunServer()
{
	LinphoneCore *lc = theApp.GetCore();
	CString tmp;
	m_hEditStunServer.GetWindowText(tmp);
	CW2A szServer(tmp);
	linphone_core_set_stun_server(lc,szServer);
}
