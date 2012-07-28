// meetphonertspaddr.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonertspaddr.h"


// Cmeetphonertspaddr 对话框

IMPLEMENT_DYNAMIC(Cmeetphonertspaddr, CDialog)

Cmeetphonertspaddr::Cmeetphonertspaddr(CWnd* pParent /*=NULL*/)
	: CDialog(Cmeetphonertspaddr::IDD, pParent)
	, m_sRtspAddr(_T(""))
{

}

Cmeetphonertspaddr::~Cmeetphonertspaddr()
{
}

void Cmeetphonertspaddr::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_RTSP, m_sRtspAddr);
}


BEGIN_MESSAGE_MAP(Cmeetphonertspaddr, CDialog)
	ON_BN_CLICKED(IDOK, &Cmeetphonertspaddr::OnBnClickedOk)
END_MESSAGE_MAP()


// Cmeetphonertspaddr 消息处理程序

void Cmeetphonertspaddr::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	OnOK();
}
