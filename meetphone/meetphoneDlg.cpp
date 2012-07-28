
// meetphoneDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphoneDlg.h"
#include "meetphonemainDlg.h"
#include "meetphonemeet.h"
#include "support.h"
#include "private.h"
#include "meetphonecore.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const int statusbars = 1;
static UINT indicators[] = {
	IDS_STATUS
};

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CmeetphoneDlg 对话框

CmeetphoneDlg::CmeetphoneDlg(CWnd* pParent /*=NULL*/)
: CDialog(CmeetphoneDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CmeetphoneDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_MAIN, m_hLoginStatic);
	DDX_Control(pDX, IDC_LOGIN_SERVER, m_hLoginServer);
	DDX_Control(pDX, IDC_LOGIN_USER, m_hLoginUser);
}

BEGIN_MESSAGE_MAP(CmeetphoneDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CmeetphoneDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CmeetphoneDlg::OnBnClickedCancel)
	ON_MESSAGE(WM_DELETE_DLG,OnDelDlgMsg)
	ON_MESSAGE(WM_DELETE_MEETDLG,OnDelMeetDlgMsg)
	ON_MESSAGE(WM_UPDATE_STATUS, OnUpdateStatus)
END_MESSAGE_MAP()


// CmeetphoneDlg 消息处理程序

BOOL CmeetphoneDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	CImage login_image;
	login_image.Load(_T("res/login_image.png"));
	if(!login_image.IsNull()) 
	{
		RECT rect;
		int width = login_image.GetWidth();
		int height = login_image.GetHeight();
		HBITMAP hBmp = login_image.Detach();
		GetClientRect(&rect);
		m_hLoginStatic.SetBitmap(hBmp);
		m_hLoginStatic.SetWindowPos(NULL,   
			(rect.right - rect.left)/2 - width/2,   
			0,   
			width,   
			height,   
			SWP_NOACTIVATE | SWP_NOZORDER); 
	}

	CRect Rect;
	GetClientRect(&Rect);                           //获取客户区域
	m_StatusBar.Create(this);
	m_StatusBar.SetIndicators(indicators,statusbars);
	m_StatusBar.MoveWindow(0,Rect.bottom-20,Rect.right,20);             //设置状态栏位置
	CTime time=CTime::GetCurrentTime();             //获取当前时间
	CString str=time.Format("%H:%M:%S");             //时间格式化为字符串
	for(int n=0;n<statusbars;n++)
	{
		m_StatusBar.SetPaneInfo(n,indicators[n],0,Rect.right/statusbars); //设置面板宽度
	}
	m_StatusBar.SetPaneText(0,L"就绪");
	//m_StatusBar.SetPaneText(1,CString("时间:") + str);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,0);	
	SetTimer(1,1000,NULL); //设置定时器

	const char *login_name = meetphone_get_ui_config ("login_username",NULL);
	const char *login_server = meetphone_get_ui_config ("login_server",NULL);
	if(login_server)
		m_hLoginServer.SetWindowText(CString(login_server));
	if(login_name)
		m_hLoginUser.SetWindowText(CString(login_name));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CmeetphoneDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CmeetphoneDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

void CmeetphoneDlg::OnTimer(UINT nIDEvent)
{
	CTime time=CTime::GetCurrentTime();             //获取当前时间
	CString str=time.Format("%H:%M:%S");             //时间格式化为字符串
	//m_StatusBar.SetPaneText(1,CString("时间:") + str); //设置状态栏第3个面板的显示字符
	CDialog::OnTimer(nIDEvent);
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CmeetphoneDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CmeetphoneDlg::OnBnClickedOk()
{
	CString userName;
	CString loginServer;
	CString identity;
	LinphoneCore *lc=theApp.GetCore();
	LinphoneProxyConfig *cfg;
	cfg=linphone_proxy_config_new();
	cfg->lc = lc;
	cfg->reg_sendregister = 1;
	cfg->commit=FALSE;
	cfg->expires = 600;
	m_hLoginServer.GetWindowText(loginServer);
	m_hLoginUser.GetWindowText(userName);
	if(userName.IsEmpty() || loginServer.IsEmpty()) {
		return;
	}
	CW2A ansiUserName(userName);
	CW2A ansiLoginServer(loginServer);
	meetphone_set_ui_config("login_username", ansiUserName);
	meetphone_set_ui_config("login_server", ansiLoginServer);
	identity.Format(L"sip:%s@%s", userName, loginServer);
	linphone_proxy_config_set_identity(cfg,CW2A(identity));
	linphone_proxy_config_set_server_addr(cfg,ansiLoginServer);
	linphone_core_add_proxy_config(lc,cfg); /*add proxy config to linphone core*/
	linphone_proxy_config_update(cfg);
	//OnOK();
}


void CmeetphoneDlg::OnBnClickedCancel()
{
	OnCancel();
}

LONG CmeetphoneDlg::OnDelDlgMsg(WPARAM wP,LPARAM lP)
{
	delete (CmeetphonemainDlg*)wP;
	theApp.m_pMeetWnd = NULL;

	if(theApp.m_pMeetingWnd == NULL)
	{
		LinphoneCore *lc=theApp.GetCore();
		LinphoneProxyConfig *cfg=NULL;
		linphone_core_get_default_proxy(lc,&cfg);
		if (cfg){
			linphone_core_remove_proxy_config(lc,cfg);
		}
		this->ShowWindow(SW_SHOWNORMAL);
	}
	return 0;
}

LONG CmeetphoneDlg::OnDelMeetDlgMsg(WPARAM wP, LPARAM lP)
{
	delete (Cmeetphonemeet*)wP;
	theApp.m_pMeetingWnd = NULL;
	MeetphoneShowMainDlg();
	return 0;
}


LONG CmeetphoneDlg::OnUpdateStatus(WPARAM wP,LPARAM lP)
{
	const char *message = (const char*)wP;
	m_StatusBar.SetPaneText(0, CString(message));
	delete[] message;
	return 0;
}
