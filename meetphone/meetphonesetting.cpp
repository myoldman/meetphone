// meetphonesetting.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonesetting.h"


// Cmeetphonesetting 对话框

IMPLEMENT_DYNAMIC(Cmeetphonesetting, CDialog)

Cmeetphonesetting::Cmeetphonesetting(CWnd* pParent /*=NULL*/)
: CDialog(Cmeetphonesetting::IDD, pParent)
{

}

Cmeetphonesetting::~Cmeetphonesetting()
{
}

void Cmeetphonesetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTING_TAB, m_hTab);
}


BEGIN_MESSAGE_MAP(Cmeetphonesetting, CDialog)
	ON_BN_CLICKED(IDOK, &Cmeetphonesetting::OnBnClickedOk)
	ON_NOTIFY(TCN_SELCHANGE, IDC_SETTING_TAB, &Cmeetphonesetting::OnTcnSelchangeSettingTab)
END_MESSAGE_MAP()


// Cmeetphonesetting 消息处理程序

void Cmeetphonesetting::OnBnClickedOk()
{
	OnOK();
}

void Cmeetphonesetting::OnTcnSelchangeSettingTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	for(int i = 0; i < 3; i++)
		m_pDialog[i]->ShowWindow(SW_HIDE);
	m_pDialog[m_hTab.GetCurSel()]->ShowWindow(SW_SHOW);
	*pResult = 0;
}

BOOL Cmeetphonesetting::OnInitDialog()
{
	CDialog::OnInitDialog();
	//EnableThemeDialogTexture( m_hWnd, ETDT_ENABLETAB  ); 
	m_hTab.InsertItem(0, L"网络设置");
	m_hTab.InsertItem(1, L"音视频设置");
	m_hTab.InsertItem(2, L"编码器");

	CRect rec;
	m_hTab.GetClientRect(&rec);//获得TAB控件的坐标

	//定位选项卡页的位置，这里可以根据情况自己调节偏移量
	rec.bottom -= 2;
	rec.left += 2;
	rec.top += 22;
	rec.right -= 2;

	//创建子页面
	m_hNetwork.Create(IDD_MEETPHONE_NETWORK, &m_hTab);
	m_hMultimedia.Create(IDD_MEETPHONE_MULTIMEDIA, &m_hTab);
	m_hCodec.Create(IDD_MEETPHONE_CODEC, &m_hTab);
	//将子页面移动到指定的位置
	m_hNetwork.MoveWindow(&rec);
	m_hMultimedia.MoveWindow(&rec);
	m_hCodec.MoveWindow(&rec);
	//显示子页面
	m_hNetwork.ShowWindow(SW_SHOW);
	m_hMultimedia.ShowWindow(SW_HIDE);
	m_hCodec.ShowWindow(SW_HIDE);
	m_pDialog[0] = &m_hNetwork;
	m_pDialog[1] = &m_hMultimedia;
	m_pDialog[2] = &m_hCodec;
	return TRUE;  // return TRUE unless you set the focus to a control
}

