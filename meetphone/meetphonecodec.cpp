// meetphonecodec.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonecodec.h"


// Cmeetphonecodec 对话框

IMPLEMENT_DYNAMIC(Cmeetphonecodec, CDialog)

const char *get_codec_color(LinphoneCore *lc, PayloadType *pt){
	const char *color;
	if (linphone_core_check_payload_type_usability(lc,pt)) color="blue";
	else color="red";
	if (!linphone_core_payload_type_enabled(lc,pt)) {
		color="grey";
	}
	return color;
}

int payload_compare(const void *a, const void *b)
{
	PayloadType *fa=(PayloadType*)a;
	PayloadType *fb=(PayloadType*)b;
	if (strcmp(fa->mime_type,fb->mime_type) == 0 && fa->clock_rate == fb->clock_rate) return 0;
	return 1;
}

Cmeetphonecodec::Cmeetphonecodec(CWnd* pParent /*=NULL*/)
: CDialog(Cmeetphonecodec::IDD, pParent)
{

}

Cmeetphonecodec::~Cmeetphonecodec()
{
}

void Cmeetphonecodec::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CODEC_VIEW, m_hComboCodecView);
	DDX_Control(pDX, IDC_LIST_CODEC, m_hListCodec);
	DDX_Control(pDX, IDC_EDIT_DOWNLOAD_BW, m_hEditDownloadBw);
	DDX_Control(pDX, IDC_EDIT_UPLOAD_BW, m_hEditUploadBw);
	DDX_Control(pDX, IDC_CHECK_ADAPTIVE_RATE, m_hCheckAdaptiveRate);
	DDX_Control(pDX, IDC_SPIN_DOWNLOAD_BW, m_hSpinDownloadBw);
	DDX_Control(pDX, IDC_SPIN_UPLOAD_BW, m_hSpinUploadBw);
}


BEGIN_MESSAGE_MAP(Cmeetphonecodec, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_CODEC_VIEW, &Cmeetphonecodec::OnCbnSelchangeComboCodecView)
	ON_BN_CLICKED(IDC_BUTTON_UP, &Cmeetphonecodec::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &Cmeetphonecodec::OnBnClickedButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_ENABLE, &Cmeetphonecodec::OnBnClickedButtonEnable)
	ON_BN_CLICKED(IDC_BUTTON_DISABLE, &Cmeetphonecodec::OnBnClickedButtonDisable)
	ON_EN_CHANGE(IDC_EDIT_DOWNLOAD_BW, &Cmeetphonecodec::OnEnChangeEditDownloadBw)
	ON_EN_CHANGE(IDC_EDIT_UPLOAD_BW, &Cmeetphonecodec::OnEnChangeEditUploadBw)
	ON_BN_CLICKED(IDC_CHECK_ADAPTIVE_RATE, &Cmeetphonecodec::OnBnClickedCheckAdaptiveRate)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_CODEC, &Cmeetphonecodec::OnNMCustomdrawListCodec)
END_MESSAGE_MAP()


// Cmeetphonecodec 消息处理程序

void Cmeetphonecodec::OnCancel()
{
	//CDialog::OnCancel();
}

void Cmeetphonecodec::OnOK()
{
	//CDialog::OnOK();
}

BOOL Cmeetphonecodec::OnInitDialog()
{
	CDialog::OnInitDialog();
	LinphoneCore *lc = theApp.GetCore();
	if(lc == NULL)
	{
		return FALSE;
	}
	m_hListCodec.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_hListCodec.InsertColumn(0, L"名称", LVCFMT_LEFT, 40);
	m_hListCodec.InsertColumn(1, L"采样率(Hz)",LVCFMT_LEFT, 70);
	m_hListCodec.InsertColumn(2, L"状态", LVCFMT_LEFT, 40);
	m_hListCodec.InsertColumn(3, L"最小比特率(kbit/s)", LVCFMT_LEFT, 120);
	m_hListCodec.InsertColumn(4, L"参数", LVCFMT_LEFT, 140);
	m_hComboCodecView.SetCurSel(0);
	DrawCodecList(0);
	m_hCheckAdaptiveRate.SetCheck(linphone_core_adaptive_rate_control_enabled(lc));
	m_hSpinDownloadBw.SetRange32(0, 10240);
	m_hSpinDownloadBw.SetPos32(linphone_core_get_download_bandwidth(lc));
	m_hSpinUploadBw.SetRange32(0, 10240);
	m_hSpinUploadBw.SetPos32(linphone_core_get_upload_bandwidth(lc));
	return TRUE;  // return TRUE unless you set the focus to a control
}

void Cmeetphonecodec::DrawCodecList(int type)
{
	LinphoneCore *lc = theApp.GetCore();
	const MSList *elem;
	const MSList *list;
	int pos_codec = 0;
	if (type==0) list=linphone_core_get_audio_codecs(lc);
	else list=linphone_core_get_video_codecs(lc);
	m_hListCodec.DeleteAllItems();
	for(elem=list; elem!=NULL; elem=elem->next){
		char *status;
		int rate;
		float bitrate;
		const char *color;
		const char *params=NULL;
		struct _PayloadType *pt=(struct _PayloadType *)elem->data;
		color=get_codec_color(lc,pt);
		if (linphone_core_payload_type_enabled(lc,pt)) status=_("Enabled");
		else {
			status=_("Disabled");
		}
		bitrate=(float)(payload_type_get_bitrate(pt)/1000.0);
		rate=payload_type_get_rate(pt);
		if (pt->recv_fmtp!=NULL) params=pt->recv_fmtp;
		LV_ITEM item;
		CString mimetype(payload_type_get_mime(pt));
		item.mask = LVIF_TEXT;
		item.iItem = pos_codec;
		item.iSubItem = 0;
		item.pszText = mimetype.GetBuffer();
		m_hListCodec.InsertItem(&item);

		WCHAR w_rate[8] = {0};
		wsprintf(w_rate, L"%d", rate);
		m_hListCodec.SetItemText(pos_codec, 1, w_rate);

		CString strStatus(status);
		m_hListCodec.SetItemText(pos_codec, 2, strStatus);

		CString strBitrate;
		strBitrate.Format(_T("%f"), bitrate);
		m_hListCodec.SetItemText(pos_codec, 3, strBitrate);

		if(params != NULL)
		{
			CString strParams(params);
			m_hListCodec.SetItemText(pos_codec, 4, strParams);
		}
		pos_codec++;
	}

}

void Cmeetphonecodec::OnCbnSelchangeComboCodecView()
{
	int nIndex = m_hComboCodecView.GetCurSel();
	if(nIndex != CB_ERR)
	{
		DrawCodecList(nIndex);
	}
}


void Cmeetphonecodec::OnBnClickedButtonUp()
{
	MoveCodec(1);
}

void Cmeetphonecodec::OnBnClickedButtonDown()
{
	MoveCodec(-1);
}

void Cmeetphonecodec::MoveCodec(int dir)
{
	LinphoneCore *lc=theApp.GetCore();
	POSITION pos = m_hListCodec.GetFirstSelectedItemPosition();
	if(pos == NULL)
	{
		return;
	}
	int nItem = m_hListCodec.GetNextSelectedItem(pos);
	int type = m_hComboCodecView.GetCurSel();
	int count = m_hListCodec.GetItemCount();
	PayloadType *pt;
	MSList *sel_elem,*before;
	MSList *codec_list;
	CString name = m_hListCodec.GetItemText(nItem, 0);
	CString rate = m_hListCodec.GetItemText(nItem, 1);
	if (type==1)//video
		codec_list=ms_list_copy(linphone_core_get_video_codecs(lc));
	else codec_list=ms_list_copy(linphone_core_get_audio_codecs(lc));
	PayloadType temp;
	CW2A ansiName(name);
	temp.mime_type = ansiName;
	temp.clock_rate = atoi(CW2A(rate));
	sel_elem=ms_list_find_custom(codec_list,payload_compare, &temp);
	if(sel_elem == NULL)
	{
		return;
	}
	pt = (PayloadType *)sel_elem->data;
	if (dir>0) {
		if (sel_elem->prev) before=sel_elem->prev;
		else before=sel_elem;
		codec_list=ms_list_insert(codec_list,before,pt);
		nItem--;
		if(nItem < 0)
			nItem = count;
	}
	else{
		if (sel_elem->next) before=sel_elem->next->next;
		else before=sel_elem;
		codec_list=ms_list_insert(codec_list,before,pt);
		nItem++;
		if(nItem > count)
			nItem = count;
	}
	codec_list=ms_list_remove_link(codec_list,sel_elem);
	if (type==1)//video
		linphone_core_set_video_codecs(lc,codec_list);
	else linphone_core_set_audio_codecs(lc,codec_list);
	DrawCodecList(type);
	m_hListCodec.RedrawWindow();
	m_hListCodec.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED ,LVIS_SELECTED | LVIS_FOCUSED );
}

void Cmeetphonecodec::CodecEnable(BOOL enable)
{
	LinphoneCore *lc=theApp.GetCore();
	POSITION pos = m_hListCodec.GetFirstSelectedItemPosition();
	int type = m_hComboCodecView.GetCurSel();
	if(pos == NULL)
	{
		return;
	}
	const MSList *codec_list;
	int nItem = m_hListCodec.GetNextSelectedItem(pos);
	CString name = m_hListCodec.GetItemText(nItem, 0);
	CString rate = m_hListCodec.GetItemText(nItem, 1);
	if (type==0) codec_list=linphone_core_get_audio_codecs(lc);
	else codec_list=linphone_core_get_video_codecs(lc);
	PayloadType *pt=NULL;
	MSList *sel_elem;
	char *status=NULL;
	PayloadType temp;
	CW2A ansiName(name);
	temp.mime_type = ansiName;
	temp.clock_rate = atoi(CW2A(rate));
	sel_elem=ms_list_find_custom((MSList *)codec_list,payload_compare, &temp);
	if(sel_elem == NULL)
	{
		return;
	}
	pt = (PayloadType *)sel_elem->data;
	linphone_core_enable_payload_type(lc,pt,enable);
	if(enable) {
		status=_("Enabled");
	}else{
		status=_("Disabled");
	}
	CString strStatus(status);
	m_hListCodec.SetItemText(nItem, 2, strStatus);
	m_hListCodec.RedrawWindow();
	m_hListCodec.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED ,LVIS_SELECTED | LVIS_FOCUSED );
}

void Cmeetphonecodec::OnBnClickedButtonEnable()
{
	CodecEnable(TRUE);
}

void Cmeetphonecodec::OnBnClickedButtonDisable()
{
	CodecEnable(FALSE);
}

void Cmeetphonecodec::OnEnChangeEditDownloadBw()
{
	LinphoneCore *lc = theApp.GetCore();
	if(m_hSpinDownloadBw.m_hWnd != NULL)
		linphone_core_set_download_bandwidth(lc, m_hSpinDownloadBw.GetPos32());
}

void Cmeetphonecodec::OnEnChangeEditUploadBw()
{
	LinphoneCore *lc = theApp.GetCore();
	if(m_hSpinUploadBw.m_hWnd != NULL)
		linphone_core_set_upload_bandwidth(lc, m_hSpinUploadBw.GetPos32());
}

void Cmeetphonecodec::OnBnClickedCheckAdaptiveRate()
{
	LinphoneCore *lc = theApp.GetCore();
	linphone_core_enable_adaptive_rate_control(lc,m_hCheckAdaptiveRate.GetCheck());
}

void Cmeetphonecodec::OnNMCustomdrawListCodec(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	NMLVCUSTOMDRAW*   pLVCD   =   (NMLVCUSTOMDRAW*)(pNMHDR); 

	*pResult = CDRF_NEWFONT;
	if(pLVCD->nmcd.dwDrawStage==CDDS_PREPAINT) 
	{ 
		*pResult   =   CDRF_NOTIFYITEMDRAW; 
	} else if (pLVCD-> nmcd.dwDrawStage==CDDS_ITEMPREPAINT)   
	{	
		int nItem = pLVCD->nmcd.dwItemSpec;
		CString   strTemp = m_hListCodec.GetItemText(nItem  ,2); 
		bool  bDBImplFail = false; 
		if   (strTemp == _("Enabled")) 
		{ 
			pLVCD->clrText  =   RGB(0,0,255); 
		} 
		else 
		{
			pLVCD->clrText   =   RGB(128,128,128);
		} 
		*pResult   =   CDRF_DODEFAULT;	
	} 
}
