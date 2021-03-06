// meetphonecreatemeet.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonecreatemeet.h"
#include <json/json.h>
#include "support.h"
#include "private.h"

// Cmeetphonecreatemeet 对话框

IMPLEMENT_DYNAMIC(Cmeetphonecreatemeet, CDialog)

Cmeetphonecreatemeet::Cmeetphonecreatemeet(CWnd* pParent /*=NULL*/)
: CDialog(Cmeetphonecreatemeet::IDD, pParent)
, m_sEditName(_T(""))
, m_sComboMixer(_T(""))
{

}

Cmeetphonecreatemeet::~Cmeetphonecreatemeet()
{
}

void Cmeetphonecreatemeet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_CONF_NAME, m_hEditName);
	DDX_Control(pDX, IDC_COMBO_MIXER, m_hComboMixer);
	DDX_Control(pDX, IDC_LIST_MEMBER, m_hListMember);
	DDX_Text(pDX, IDC_EDIT_CONF_NAME, m_sEditName);
	DDX_CBString(pDX, IDC_COMBO_MIXER, m_sComboMixer);
}


BEGIN_MESSAGE_MAP(Cmeetphonecreatemeet, CDialog)
	ON_BN_CLICKED(IDOK, &Cmeetphonecreatemeet::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &Cmeetphonecreatemeet::OnBnClickedCancel)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_MEMBER, &Cmeetphonecreatemeet::OnLvnDeleteitemListMember)
	ON_NOTIFY(LVN_ITEMCHANGING, IDC_LIST_MEMBER, &Cmeetphonecreatemeet::OnLvnItemchangingListMember)
END_MESSAGE_MAP()


// Cmeetphonecreatemeet 消息处理程序

BOOL Cmeetphonecreatemeet::OnInitDialog()
{
	CDialog::OnInitDialog();
	LinphoneCore *lc = theApp.GetCore();
	LinphoneProxyConfig *cfg;
	linphone_core_get_default_proxy(lc, &cfg);
	m_hActionImage.Create(16, 16, ILC_COLOR32,  4, 4);
	load_png_to_imagelist(m_hActionImage,CString("res/webphone_16.png"));
	m_hListMember.SetImageList(&m_hActionImage, LVSIL_SMALL);
	m_hListMember.SetExtendedStyle(LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES );
	m_hListMember.InsertColumn(0, L"名称", LVCFMT_LEFT, 120);
	m_hListMember.InsertColumn(1, L"状态", LVCFMT_LEFT, 120);
	CString restMethod("/mcuWeb/controller/getMediaMixer");
	Json::Value response;
	if(http_get_request(restMethod, response)) 
	{
		for ( unsigned int index = 0; index < response.size(); ++index ) 
		{
			const char* strName = response[index]["name"].asCString();
			const char *url = response[index]["url"].asCString();
			CString mixer;
			mixer.Format(_T("%s@%s"), CString(strName) , CString(url));
			m_hComboMixer.InsertString(index, mixer); 
		}
		m_hComboMixer.SetCurSel(0);
	}

	restMethod = "/mcuWeb/controller/getSipEndPoint";
	response.clear();
	if(http_get_request(restMethod, response)) 
	{
		m_ListMember.resize(response.size());
		for ( unsigned int index = 0; index < response.size(); ++index ) 
		{
			const char* strName = response[index]["name"].asCString();
			const char *state = response[index]["state"].asCString();
			int id  = response[index]["id"].asInt();		
			BOOL visiable = FALSE;
			if(state != NULL && strcmp(state, "IDLE") == 0)
				visiable = TRUE;
			if(state != NULL)
				state = _(state);
			CString cStrName;
			convert_utf8_to_unicode(strName, cStrName);
			m_hListMember.InsertItem(index,  cStrName);
			if(cfg != NULL && id == atoi(sal_op_get_userid(cfg->op)) )
			{
				state = "创建人";
				visiable = FALSE;
			}
			m_hListMember.SetItemText(index, 1, CString(state));
			if(!visiable)
			{
				m_hListMember.SetItemState(index, INDEXTOSTATEIMAGEMASK(0) | LVIS_CUT, LVIS_STATEIMAGEMASK | LVIS_CUT);
				m_hListMember.SetItemState(index, LVIS_CUT, LVIS_CUT);
			}
			else
			{
				m_ListMember[index] = id;
			}
			
		}
	}


	return TRUE;
}

void Cmeetphonecreatemeet::OnBnClickedOk()
{
	CString name;
	m_hEditName.GetWindowText(name);
	if(name.IsEmpty())
	{
		AfxMessageBox(L"请输入会议名称");
		return;
	}

	CString mixer;
	m_hComboMixer.GetWindowText(mixer);
	if(mixer.IsEmpty())
	{
		AfxMessageBox(L"请选择媒体服务器");
		return;
	}

	LinphoneCore *lc = theApp.GetCore();
	LinphoneProxyConfig *cfg;
	linphone_core_get_default_proxy(lc, &cfg);
	m_StrFormData.Format(_T("name=%s&mixerId=%s&admin=%d&selected_id=sip_%d"), name, mixer, atoi(sal_op_get_userid(cfg->op)), atoi(sal_op_get_userid(cfg->op)));
	for(int i=0; i<m_hListMember.GetItemCount(); i++)
	{
		int memberId = m_ListMember[i];
		if(m_hListMember.GetCheck(i) && memberId)
		{
			CString temp;
			temp.Format(_T("&selected_id=sip_%d"), memberId);
			m_StrFormData += temp;
		}
	}
	OnOK();
}

void Cmeetphonecreatemeet::OnBnClickedCancel()
{
	OnCancel();
}

void Cmeetphonecreatemeet::OnLvnDeleteitemListMember(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
}

void Cmeetphonecreatemeet::OnLvnItemchangingListMember(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	unsigned state = (pNMLV->uOldState ^ pNMLV->uNewState) & LVIS_STATEIMAGEMASK;
	if(INDEXTOSTATEIMAGEMASK(0) == state) {
		*pResult = true;
		return;
	} 
	*pResult = 0;
}
