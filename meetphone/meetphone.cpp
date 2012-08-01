
// meetphone.cpp : 定义应用程序的类行为。
//
#include "stdafx.h"
#include "meetphone.h"
#include "meetphoneDlg.h"
#include "meetphonecore.h"
#include "meetphonesetting.h"
#include "support.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CmeetphoneApp

BEGIN_MESSAGE_MAP(CmeetphoneApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
	ON_COMMAND(ID_MENU_DEBUG, &CmeetphoneApp::OnMenuDebug)
	ON_COMMAND(ID_MENU_ABOUT, &CmeetphoneApp::OnMenuAbout)
	ON_COMMAND(ID_SETTING, &CmeetphoneApp::OnSetting)
	ON_COMMAND(ID_LOGOUT, &CmeetphoneApp::OnLogout)
END_MESSAGE_MAP()


// CmeetphoneApp 构造

CmeetphoneApp::CmeetphoneApp()
{
	m_pCore = NULL;
	m_pMeetWnd = NULL;
	m_pLogWnd = NULL;
	m_pSettingWnd = NULL;
	m_pMeetingWnd = NULL;
	m_ConfigFile = NULL;
	m_SecretsFile = NULL;
	m_nTimerID = 0;
	m_nLogTimerID = 0;
}


// 唯一的一个 CmeetphoneApp 对象

CmeetphoneApp theApp;

char *CmeetphoneApp::GetConfigFile(const char *filename)
{
	char *config_file = new char[MAX_PATH];
	memset(config_file, 0 , MAX_PATH);
	if (filename==NULL) filename=CONFIG_FILE;
	if (_access(CONFIG_FILE,0)==0){
		snprintf(config_file,MAX_PATH,"%s",filename);
	}else{
		const char *appdata=getenv("APPDATA");
		if (appdata){
			snprintf(config_file, MAX_PATH,"%s\\%s",appdata,MEETPHONE_CONFIG_DIR);
			CString wPath(config_file);
			CreateDirectory(wPath,NULL);
			snprintf(config_file,MAX_PATH,"%s\\%s\\%s",appdata,MEETPHONE_CONFIG_DIR,filename);
		}
	}
	return config_file;
}

char *CmeetphoneApp::GetFactoryConfigFile() 
{
	memset(m_FactoryConfigFile, 0 , sizeof(m_FactoryConfigFile));
	if (_access(FACTORY_CONFIG_FILE,0)==0){
		snprintf(m_FactoryConfigFile,sizeof(m_FactoryConfigFile),
						 "%s",FACTORY_CONFIG_FILE);
	} else {
		TCHAR *basename = wcsrchr(m_WorkingDir, '\\');
		if (basename != NULL) {
			basename ++;
			*basename = '\0';
			snprintf(m_FactoryConfigFile, sizeof(m_FactoryConfigFile),
								 "%s\\..\\%s", m_WorkingDir, FACTORY_CONFIG_FILE);
		}
	}
	return m_FactoryConfigFile;
}

BOOL CmeetphoneApp::InitCoreapi() 
{
	m_ConfigFile = GetConfigFile(NULL);
	m_SecretsFile = GetConfigFile(SECRETS_FILE);
	GetFactoryConfigFile();
#ifdef ENABLE_NLS
	char tmp[128];
	snprintf(tmp,sizeof(tmp),"LANG=%s","zh_CN");
	_putenv(tmp);
	setlocale(LC_ALL, "");
	void *p;
	p=bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	if (p==NULL) 
		ms_error("bindtextdomain failed");
	//bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#else
	ms_message("NLS disabled.\n");
#endif
	linphone_core_enable_logs_with_cb(MeetphoneLogHandler);
	LinphoneCoreVTable vtable={0};
	vtable.call_state_changed=MeetphoneCallStateChanged;
	vtable.registration_state_changed=MeetphoneRegistrationStateChanged;
	//vtable.notify_presence_recv=linphone_gtk_notify_recv;
	vtable.notify_recv=MeetphoneNotifyRecvConf;
	//vtable.new_subscription_request=linphone_gtk_new_unknown_subscriber;
	//vtable.auth_info_requested=linphone_gtk_auth_info_requested;
	vtable.display_status=MeetphoneDisplayStatus;
	//vtable.display_message=linphone_gtk_display_message;
	//vtable.display_warning=linphone_gtk_display_warning;
	//vtable.display_url=linphone_gtk_display_url;
	//vtable.call_log_updated=linphone_gtk_call_log_updated;
	//vtable.text_received=linphone_gtk_text_received;
	//vtable.refer_received=linphone_gtk_refer_received;
	//vtable.buddy_info_updated=linphone_gtk_buddy_info_updated;
	//vtable.call_encryption_changed=linphone_gtk_call_encryption_changed;
	linphone_core_set_user_agent("Meetphone", "1.0");
	m_pCore = linphone_core_new(&vtable,m_ConfigFile,m_FactoryConfigFile,NULL);
	//linphone_core_set_waiting_callback(m_pCore,NULL,NULL);
	//linphone_core_set_zrtp_secrets_file(m_pCore,m_SecretsFile);
	m_nTimerID = SetTimer(NULL,NULL,30,MeetphoneIterate);
	m_nLogTimerID = SetTimer(NULL,NULL,30,MeetphoneLog);
	if (linphone_core_video_enabled(m_pCore)){
		linphone_core_use_preview_window(m_pCore, TRUE);
		linphone_core_enable_video_preview(m_pCore,meetphone_get_ui_config_int("videoselfview",
	    	VIDEOSELFVIEW_DEFAULT));
	}
	return TRUE;
}

// CmeetphoneApp 初始化

BOOL CmeetphoneApp::InitInstance()
{
	HANDLE   hMutex; 
	hMutex   =   CreateMutex(NULL,   false, _T(APP_NAME) );
	if   (GetLastError()==ERROR_ALREADY_EXISTS) 
	{ 
		MessageBox(NULL, L"程序已经运行了", L"程序已经运行了",MB_OK);
		CloseHandle(hMutex);
		return   FALSE   ; 
	}

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	AfxInitRichEdit();
	// Initialize GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
 
	CWinAppEx::InitInstance();

	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));
	memset(m_WorkingDir, 0, MAX_PATH);
	GetCurrentDirectory(sizeof(m_WorkingDir) - 1, m_WorkingDir);
	
	m_pLogWnd = new CDialog(IDD_MEETPHONE_LOG);
	m_pLogWnd->Create(IDD_MEETPHONE_LOG,NULL);

	InitCoreapi();

	m_pSettingWnd = new Cmeetphonesetting(NULL);
	m_pSettingWnd->Create(IDD_MEETPHONE_SETTING,NULL);

	CmeetphoneDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

	if(m_ConfigFile)
		delete[] m_ConfigFile;
	if(m_SecretsFile)
		delete[] m_SecretsFile;
	if(m_nTimerID > 0)
		KillTimer(NULL,m_nTimerID);
	if(m_nLogTimerID > 0)
		KillTimer(NULL,m_nLogTimerID);
	CloseHandle(hMutex);

	linphone_core_destroy(m_pCore);
	MeetphoneLog(0,0,0,0);

	m_pLogWnd->DestroyWindow();
	delete m_pLogWnd;
	
	m_pSettingWnd->DestroyWindow();
	delete m_pSettingWnd;
	
	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	return FALSE;
}

void CmeetphoneApp::OnMenuDebug()
{
	if(m_pLogWnd != NULL)
	{
		m_pLogWnd->ShowWindow(SW_SHOWNORMAL);
		m_pLogWnd->SetActiveWindow();
	}
}

void CmeetphoneApp::OnMenuAbout()
{
	CDialog dlgAbout(IDD_ABOUTBOX);
	dlgAbout.DoModal();
}

void CmeetphoneApp::OnSetting()
{
	if(m_pSettingWnd != NULL)
	{
		m_pSettingWnd->ShowWindow(SW_SHOWNORMAL);
		m_pSettingWnd->SetActiveWindow();
	}
}

void CmeetphoneApp::OnLogout()
{
	if(m_pMeetWnd != NULL)
	{
		m_pMeetWnd->DestroyWindow();
	}
}
