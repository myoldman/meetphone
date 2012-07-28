// meetphonemultimedia.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonemultimedia.h"
#include "meetphonertspaddr.h"


// Cmeetphonemultimedia 对话框

IMPLEMENT_DYNAMIC(Cmeetphonemultimedia, CDialog)

Cmeetphonemultimedia::Cmeetphonemultimedia(CWnd* pParent /*=NULL*/)
: CDialog(Cmeetphonemultimedia::IDD, pParent)
{

}

Cmeetphonemultimedia::~Cmeetphonemultimedia()
{
}

void Cmeetphonemultimedia::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PLAYBACK, m_hComboPlayBack);
	DDX_Control(pDX, IDC_COMBO_RING, m_hComboRing);
	DDX_Control(pDX, IDC_COMBO_CAPTURE, m_hComboCapture);
	DDX_Control(pDX, IDC_EDIT_RING, m_hEditRing);
	DDX_Control(pDX, IDC_CHECK_ECHOCANCEL, m_hCheckEchoCancel);
	DDX_Control(pDX, IDC_BUTTON_BROWSE, m_hButtonBrowse);
	DDX_Control(pDX, IDC_BUTTON_PLAY, m_hButtonPlay);
	DDX_Control(pDX, IDC_COMBO_WEBCAMS, m_hComboWebcams);
	DDX_Control(pDX, IDC_COMBO_VIDEO_SIZE, m_hComboVideoSize);
}


BEGIN_MESSAGE_MAP(Cmeetphonemultimedia, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &Cmeetphonemultimedia::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &Cmeetphonemultimedia::OnBnClickedButtonBrowse)
	ON_CBN_SELCHANGE(IDC_COMBO_WEBCAMS, &Cmeetphonemultimedia::OnCbnSelchangeComboWebcams)
	ON_CBN_SELCHANGE(IDC_COMBO_VIDEO_SIZE, &Cmeetphonemultimedia::OnCbnSelchangeComboVideoSize)
	ON_BN_CLICKED(IDC_CHECK_ECHOCANCEL, &Cmeetphonemultimedia::OnBnClickedCheckEchocancel)
END_MESSAGE_MAP()

static void meetphone_end_of_ring(LinphoneCore *lc, void *user_data){
	CButton *play_button = (CButton*)user_data;
	play_button->EnableWindow(TRUE);
}

// Cmeetphonemultimedia 消息处理程序

void Cmeetphonemultimedia::OnBnClickedButtonPlay()
{
	LinphoneCore *lc = theApp.GetCore();
	if (linphone_core_preview_ring(lc,
		linphone_core_get_ring(lc),
		meetphone_end_of_ring,
		&m_hButtonPlay)==0){
			m_hButtonPlay.EnableWindow(FALSE);
	}
}


BOOL Cmeetphonemultimedia::OnInitDialog()
{
	CDialog::OnInitDialog();
	LinphoneCore *lc = theApp.GetCore();
	int pos_capture = 0;
	int pos_playback = 0;
	int pos_video = 0;
	int pos_size = 0;
	const char *ringer;
	const char *video_device;
	const char **video_devices;
	const char **sound_devices;
	char vsize_def[256];
	if(lc == NULL)
	{
		return FALSE;
	}
	const char *playback_device = linphone_core_get_playback_device(lc);
	const char *ringer_device = linphone_core_get_ringer_device(lc);
	const char *capture_device = linphone_core_get_capture_device(lc);
	const MSVideoSizeDef *def=linphone_core_get_supported_video_sizes(lc);
	MSVideoSize cur=linphone_core_get_preferred_video_size(lc);

	sound_devices=linphone_core_get_sound_devices(lc);
	video_devices = linphone_core_get_video_devices(lc);
	video_device = linphone_core_get_video_device(lc);
	for(;*sound_devices != NULL; ++sound_devices)
	{
		if(linphone_core_sound_device_can_playback(lc,*sound_devices))
		{
			m_hComboPlayBack.InsertString(pos_playback, CString(*sound_devices));
			m_hComboRing.InsertString(pos_playback, CString(*sound_devices));
			if (strcmp(playback_device,*sound_devices)==0)
				m_hComboPlayBack.SetCurSel(pos_playback);
			if (strcmp(ringer_device,*sound_devices)==0)
				m_hComboRing.SetCurSel(pos_playback);
			pos_playback++;
		}

		if(linphone_core_sound_device_can_capture(lc,*sound_devices))
		{
			m_hComboCapture.InsertString(pos_capture, CString(*sound_devices));
			if (strcmp(capture_device,*sound_devices)==0)
				m_hComboCapture.SetCurSel(pos_capture);
			pos_capture++;
		}

	}
	m_hCheckEchoCancel.SetCheck(linphone_core_echo_cancellation_enabled(lc));
	ringer = linphone_core_get_ring(lc);
	m_hEditRing.SetWindowText(CString(ringer));

	for(;*video_devices != NULL; ++video_devices)
	{
		m_hComboWebcams.InsertString(pos_video, CString(*video_devices));
		if (strcmp(video_device,*video_devices)==0) {
			m_hComboWebcams.SetCurSel(pos_video);
			m_iLastWebcam = pos_video;
		}
		pos_video++;
	}

	for(int i=0;def->name!=NULL;++def,++i){
		snprintf(vsize_def,sizeof(vsize_def),"%s (%ix%i)",def->name,def->vsize.width,def->vsize.height);
		m_hComboVideoSize.InsertString(pos_size, CString(vsize_def));
		if (cur.width==def->vsize.width && cur.height==def->vsize.height)
		{
			m_hComboVideoSize.SetCurSel(pos_size);
		}
		pos_size++;
	}


	return TRUE;
}

void Cmeetphonemultimedia::OnOK()
{
	//CDialog::OnOK();
}

void Cmeetphonemultimedia::OnCancel()
{
	//CDialog::OnCancel();
}

void Cmeetphonemultimedia::OnBnClickedButtonBrowse()
{
	LinphoneCore *lc = theApp.GetCore();
	CFileDialog fileDlg(true, L"wav", L"*.wav", OFN_FILEMUSTEXIST| OFN_HIDEREADONLY, L"声音文件(*.wav)|*.wav||", this);
	WCHAR init_path[256] = {0};
	wsprintf(init_path, L"%smeetphone\\rings", theApp.GetWorkingDir());
	fileDlg.m_ofn.lpstrInitialDir = init_path;
	if(fileDlg.DoModal() == IDOK)
	{
		CString fileName;
		fileName = fileDlg.GetPathName();

		// Transcoding.
		CW2A ansiFileName(fileName);
		linphone_core_set_ring(lc,ansiFileName);
		m_hEditRing.SetWindowText(fileName);
	}
}

void Cmeetphonemultimedia::OnCbnSelchangeComboWebcams()
{
	LinphoneCore *lc = theApp.GetCore();
	int nIndex = m_hComboWebcams.GetCurSel(); 
	CString str;
	char* szAnsi = NULL;
	if(nIndex == CB_ERR) 
	{
		return;
	}

	m_hComboWebcams.GetLBText(nIndex,str);
	if(wcsncmp(str.GetString(), L"RTSP", 4) == 0){
		Cmeetphonertspaddr rtspDialog;
		rtspDialog.m_sRtspAddr = str.Mid(6);;
		INT_PTR nResponse = rtspDialog.DoModal();
		if(nResponse == IDOK)
		{
			if(rtspDialog.m_sRtspAddr.GetLength() > 0)
			{
				linphone_core_set_cam_name(lc, CW2A(str) , CW2A(rtspDialog.m_sRtspAddr));
				m_hComboWebcams.SetWindowText(L"RTSP: " + rtspDialog.m_sRtspAddr);
				linphone_core_set_video_device(lc,CW2A(L"RTSP: " + rtspDialog.m_sRtspAddr));
			}
		} else {
			m_hComboWebcams.SetCurSel(m_iLastWebcam);
		}
	} else {
		linphone_core_set_video_device(lc,CW2A(str));
	}
	m_iLastWebcam = nIndex;
}


void Cmeetphonemultimedia::OnCbnSelchangeComboVideoSize()
{
	LinphoneCore *lc = theApp.GetCore();
	int nIndex = m_hComboVideoSize.GetCurSel(); 
	if(nIndex == CB_ERR) 
	{
		return;
	}
	const MSVideoSizeDef *defs=linphone_core_get_supported_video_sizes(lc);
	linphone_core_set_preferred_video_size(lc,defs[nIndex].vsize);
}

void Cmeetphonemultimedia::OnBnClickedCheckEchocancel()
{
	LinphoneCore *lc = theApp.GetCore();
	linphone_core_enable_echo_cancellation(lc, m_hCheckEchoCancel.GetCheck());
}
