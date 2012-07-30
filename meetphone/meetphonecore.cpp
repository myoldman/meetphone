#include "StdAfx.h"
#include "support.h"
#include "meetphonecore.h"
#include "meetphonemainDlg.h"
#include "meetphonemeet.h"
#include "meetphone.h"


HANDLE log_mutex = CreateMutex(NULL, FALSE, NULL);
CList<MeetphoneOutputLog* ,MeetphoneOutputLog*&> log_queue;

void CALLBACK MeetphoneIterate(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	static BOOL in_iterate=FALSE;
	if (in_iterate) return;
	in_iterate=TRUE;
	linphone_core_iterate(theApp.GetCore());
	in_iterate=FALSE;
}

void CALLBACK MeetphoneLog(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	WaitForSingleObject(log_mutex, INFINITE);
	while(!log_queue.IsEmpty()) 
	{
		MeetphoneOutputLog *log_item = log_queue.RemoveHead();
		if(theApp.m_pLogWnd != NULL) 
		{
			CRichEditCtrl *log_edit = (CRichEditCtrl*)theApp.m_pLogWnd->GetDlgItem(IDC_LOG);
			int nLength = log_edit->GetWindowTextLength();
			log_edit->SetSel(nLength, nLength);
			log_edit->ReplaceSel(log_item->msg);
			if(log_item->lev == ORTP_WARNING || log_item->lev == ORTP_ERROR) 
			{
				int lineCount = log_edit->GetLineCount();
				int lineStart,lineEnd;
				CHARFORMAT cFmt;
				cFmt.cbSize = sizeof(CHARFORMAT);
				cFmt.crTextColor = RGB(250,128,0);
				if(log_item->lev == ORTP_ERROR)
					cFmt.crTextColor = RGB(255,0,0);
				cFmt.dwEffects = 0;
				cFmt.dwMask = CFM_COLOR;


				lineStart = log_edit->LineIndex(lineCount - 2);//取第一行的第一个字符的索引
				lineEnd = log_edit->LineIndex(lineCount - 1) - 1;//取第一行的最后一个字符的索引——用第二行的第一个索引减1来实现
				log_edit->SetSel(lineStart,lineEnd);//选取第一行字符
				log_edit->SetSelectionCharFormat(cFmt);//设置颜色
			}
			log_edit->PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
		}
		delete log_item;

	}
	ReleaseMutex(log_mutex);
}

void MeetphoneLogHandler(OrtpLogLevel lev, const char *fmt, va_list args)
{
	const char *lname="undef";
	char *msg;
	char *msg_str;

	switch(lev){
			case ORTP_DEBUG:
				lname="DEBUG";
				break;
			case ORTP_MESSAGE:
				lname="MESSAGE";
				break;
			case ORTP_WARNING:
				lname="WARNING";
				break;
			case ORTP_ERROR:
				lname="ERROR";
				break;
			case ORTP_FATAL:
				lname="FATAL";
				break;
			default:
				lname = ("Bad level !");
	}


	msg=ortp_strdup_vprintf(fmt,args);

	msg_str = ortp_strdup_printf("%s: %s \r\n",lname,msg);

	CString wMsgStr(msg_str);
	OutputDebugString(wMsgStr);

	WaitForSingleObject(log_mutex, INFINITE);
	MeetphoneOutputLog *lgl= new MeetphoneOutputLog;
	//memset(lgl, 0, sizeof(MeetphoneOutputLog));
	lgl->lev=lev;
	lgl->msg=wMsgStr;
	lgl->length = wMsgStr.GetLength();
	log_queue.AddTail(lgl);
	ReleaseMutex(log_mutex);

	ms_free(msg);
	ms_free(msg_str);
}

void MeetphoneRegistrationStateChanged(LinphoneCore *lc, LinphoneProxyConfig *cfg, 
									   LinphoneRegistrationState rs, const char *msg)
{
	LinphoneProxyConfig *cfg1 = NULL;
	int ret = 0;
	switch (rs){
		case LinphoneRegistrationOk:
			ret = linphone_core_get_default_proxy(lc, &cfg1);
			if (cfg){
				if(ret < 0 && cfg1 == NULL) {
					theApp.m_pMainWnd->ShowWindow(SW_HIDE);
					linphone_core_set_default_proxy(lc,cfg);
					const char *login_name = meetphone_get_ui_config ("login_username",NULL);
					const char *admin_number = meetphone_get_ui_config ("admin_number","10000");
					if(login_name != NULL && strcasecmp(login_name, admin_number) == 0) {
						linphone_core_set_admin(lc, TRUE);
					} else {
						linphone_core_set_admin(lc, FALSE);
					}
					MeetphoneShowMainDlg();
				}
			}
			break;
		case LinphoneRegistrationFailed:
			linphone_core_remove_proxy_config(lc,cfg);
			break;
		default:
			break;
	}

}

void MeetphoneNotifyRecvConf(LinphoneCore *lc, LinphoneCall *call, const char *from, const char *ev)
{
	if(theApp.m_pMeetingWnd != NULL)
		theApp.m_pMeetingWnd->PostMessage(WM_MEMBER_RELOAD);
}

void MeetphoneCallStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cs, const char *msg)
{
	unsigned long id = 0;
	switch(cs){
		case LinphoneCallOutgoingInit:
			break;
		case LinphoneCallOutgoingProgress:
			break;
		case LinphoneCallStreamsRunning:
			break;
		case LinphoneCallPaused:
			break;
		case LinphoneCallError:
			break;
		case LinphoneCallEnd:
			if(theApp.m_pMeetingWnd != NULL)
				theApp.m_pMeetingWnd->DestroyWindow();
			break;
		case LinphoneCallIncomingReceived:
			linphone_core_accept_call(lc,call);
			break;
		case LinphoneCallResuming:
			break;
		case LinphoneCallPausing:
		case LinphoneCallPausedByRemote:
			break;
		case LinphoneCallConnected:
			MeetphoneShowMeetDlg(call);
			if(theApp.m_pMeetWnd != NULL)
				theApp.m_pMeetWnd->DestroyWindow();
			break;
		case LinphoneCallReleased:
			if(theApp.m_pMeetWnd != NULL)
				theApp.m_pMeetWnd->PostMessage(WM_RELOAD_CONEFENCE);
			break;
		default:
			break;
	}
}

void MeetphoneDisplayStatus(LinphoneCore *lc, const char *status)
{
	if(theApp.m_pMainWnd != NULL && theApp.m_pMainWnd->IsWindowEnabled())
		theApp.m_pMainWnd->PostMessage(WM_UPDATE_STATUS, (WPARAM)_strdup(status));
}

void MeetphoneShowMainDlg()
{
	LinphoneProxyConfig *cfg = NULL;
	LinphoneCore *lc = theApp.GetCore();
	linphone_core_get_default_proxy(lc, &cfg);
	CString userName;
	CString wTitle;
	CString wFormat(_("Hello %s"));
	const char *username = sal_op_get_username(cfg->op);
	convert_utf8_to_unicode(username, userName);
	wTitle.Format(wFormat, userName);	

	CmeetphonemainDlg *dlg=new CmeetphonemainDlg(theApp.m_pMainWnd);
	dlg->Create(IDD_MEETPHONE_MAIN,theApp.m_pMainWnd);
	dlg->ModifyStyleEx(WS_EX_TOOLWINDOW, WS_EX_APPWINDOW);
	dlg->SetWindowText(wTitle);
	dlg->ShowWindow(SW_SHOWNORMAL);
	dlg->SetActiveWindow();
	theApp.m_pMeetWnd = dlg;
}

void MeetphoneShowMeetDlg(LinphoneCall *call)
{
	LinphoneCore *lc = theApp.GetCore();
	Cmeetphonemeet *dlg=new Cmeetphonemeet(theApp.m_pMainWnd);
	CString memberName;
	const char *confUID = sal_op_get_confuid(call->op);
	const char *username = sal_op_get_username(call->op);
	if(confUID != NULL)
	{
		dlg->m_sConfUID = confUID;
	}
	if(username != NULL)
	{
		convert_utf8_to_unicode(username, memberName);
	}
	else 
	{
		memberName = _T("未知");
	}
	dlg->Create(IDD_MEETPHONE_MEET,theApp.m_pMainWnd);
	dlg->ModifyStyleEx(WS_EX_TOOLWINDOW, WS_EX_APPWINDOW);
	dlg->SetWindowText(L"会议");
	dlg->ShowWindow(SW_SHOWNORMAL);
	dlg->SetActiveWindow();
	HWND hMemberWnd = (HWND)dlg->SendMessage(WM_MEMBER_ADD, (WPARAM)&memberName);
	call->videostream->window_id = (unsigned long)hMemberWnd;
	HWND hPreviewWnd = (HWND)dlg->SendMessage(WM_MEMBER_PREVIEW_HWND, (WPARAM)&memberName);
	linphone_core_set_native_preview_window_id(lc,(unsigned long)hPreviewWnd);
	theApp.m_pMeetingWnd = dlg;
}
