#include "StdAfx.h"
#include "support.h"
#include "meetphonecore.h"
#include "meetphonemainDlg.h"
#include "meetphonemeet.h"
#include "meetphonepassword.h"
#include "meetphone.h"


HANDLE log_mutex = CreateMutex(NULL, FALSE, NULL);
CList<MeetphoneOutputLog* ,MeetphoneOutputLog*&> log_queue;
esl_handle_t event_handle = {{0}};
esl_handle_t api_handle = {{0}};
ms_thread_t event_thread;
bool done = false;

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

void MeetphoneNotifyRecvConf(LinphoneCore *lc, LinphoneCall *call, const char *from, const char *ev,int partId)
{
	if(theApp.m_pMeetingWnd != NULL && strcmp(ev, "CONNECTED") == 0) {
		theApp.m_pMeetingWnd->PostMessage(WM_MEMBER_RELOAD);
		if(partId != ((Cmeetphonemeet*)theApp.m_pMeetingWnd)->m_iPartId) {
			LinphoneCore *lc = theApp.GetCore();
			linphone_core_invite_spy(lc, ((Cmeetphonemeet*)theApp.m_pMeetingWnd)->m_sConfDID.c_str(), partId);
		}
	}
}

void MeephoneAuthInfoRequest(LinphoneCore *lc, const char *realm, const char *username)
{
	Cmeetphonepassword password;
	password.m_sUsername = username;
	INT_PTR nResponse = password.DoModal();
	if(nResponse == IDOK) {
		LinphoneAuthInfo *info;
		info=linphone_auth_info_new(username, CW2A(password.m_sUsername), CW2A(password.m_sPassword), NULL,realm);
		linphone_core_add_auth_info(lc,info);
		//todo release info memory
	} else {
		linphone_core_abort_authentication(lc,NULL);
	}
	
}

mblk_t *copyb(mblk_t *mp)
{
	mblk_t *newm;
	int len=(int) (mp->b_wptr-mp->b_rptr);
	newm=allocb(len,BPRI_MED);
	memcpy(newm->b_wptr,mp->b_rptr,len);
	newm->b_wptr+=len;
	return newm;
}

mblk_t *copymsg(mblk_t *mp)
{
	mblk_t *newm=0,*m;
	m=newm=copyb(mp);
	mp=mp->b_cont;
	while(mp!=NULL){
		m->b_cont=copyb(mp);
		m=m->b_cont;
		mp=mp->b_cont;
	}
	return newm;
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
			if(linphone_call_is_spy(call)) {
				mblk_t *m1, *m2;
				rtp_header_t *rtp;
				m1=allocb(RTP_FIXED_HEADER_SIZE+32,BPRI_MED);
				rtp=(rtp_header_t*)m1->b_rptr;
				rtp->version = 2;
				rtp->markbit=1;
				rtp->padbit = 0;
				rtp->extbit = 0;
				rtp->cc = 0;
				rtp->ssrc = 1233445;
					/* timestamp set later, when packet is sended */
					/*seq number set later, when packet is sended */
					
					/*set the payload type */
				rtp->paytype=99;
					
					/*copy the payload */
				m1->b_wptr+=RTP_FIXED_HEADER_SIZE;
				m2 = copymsg(m1);
				rtp_session_sendm_with_ts(call->videostream->session,m1,0);
				Sleep(50);
				rtp_session_sendm_with_ts(call->videostream->session,m2,0);
			}
			break;
		case LinphoneCallPaused:
			break;
		case LinphoneCallError:
			if(linphone_call_is_spy(call)) {
				
			}
			break;
		case LinphoneCallEnd:
			if(linphone_call_is_spy(call)) {
				CString memberName;
				const char *username = sal_op_get_username(call->op);
				if(username != NULL)
				{
					convert_utf8_to_unicode(username, memberName);
				}
				else 
				{
					memberName = _T("未知");
				}
				if(theApp.m_pMeetingWnd != NULL)
					theApp.m_pMeetingWnd->SendMessage(WM_MEMBER_DELETE, (WPARAM)&memberName);
			} else {
				if(theApp.m_pMeetingWnd != NULL) {
					theApp.m_pMeetingWnd->DestroyWindow();
					theApp.m_pMeetingWnd = NULL;
				}
			}
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
			if(linphone_call_is_spy(call)) {
				MeetphoneAddSpy(call);
			} else {
				MeetphoneShowMeetDlg(call);
				if(theApp.m_pMeetWnd != NULL) {
					theApp.m_pMeetWnd->DestroyWindow();
					theApp.m_pMeetWnd = NULL;
				}
			}
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

void MeetphoneAddSpy(LinphoneCall *call)
{
	CString memberName;
	LinphoneCore *lc = theApp.GetCore();
	const char *username = sal_op_get_username(call->op);
	if(username != NULL)
	{
		convert_utf8_to_unicode(username, memberName);
	}
	else 
	{
		memberName = _T("未知");
	}
	if(theApp.m_pMeetingWnd != NULL) {
		HWND hMemberWnd = (HWND)theApp.m_pMeetingWnd->SendMessage(WM_MEMBER_ADD, (WPARAM)&memberName);
		call->videostream->window_id = (unsigned long)hMemberWnd;
	} else {
		linphone_core_terminate_call(lc, call);
	}
}

void MeetphoneShowMeetDlg(LinphoneCall *call)
{
	LinphoneCore *lc = theApp.GetCore();
	Cmeetphonemeet *dlg=new Cmeetphonemeet(theApp.m_pMainWnd);
	CString memberName;
	CString restMethod;
	const char *confUID = sal_op_get_confuid(call->op);
	const char *username = sal_op_get_username(call->op);
	const char *partid = sal_op_get_userid(call->op);
	const char *confDID = NULL;
	if(call->dir == LinphoneCallOutgoing) {
		dlg->m_sConfDID = sal_op_get_to(call->op);
	} else {
		LinphoneAddress *from = linphone_address_new(sal_op_get_from(call->op));
		if(from != NULL) {
			dlg->m_sConfDID = linphone_address_get_username(from);
			linphone_address_destroy(from);
		} else {
			dlg->m_sConfDID = sal_op_get_from(call->op);
		}
	}
	if(confUID != NULL)
	{
		dlg->m_sConfUID = confUID;
		dlg->m_iPartId = atoi(partid);
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


	restMethod.Format(_T("/mcuWeb/controller/getConferenceMember?confid=%s"), dlg->m_sConfUID);
	Json::Value response;
	if(http_get_request(restMethod, response)) 
	{
		for ( unsigned int index = 0; index < response.size(); ++index ) 
		{
			const char *state = response[index]["state"].asCString();
			int memberId = response[index]["id"].asInt();
			if(state != NULL && strcasecmp("CONNECTED", state) == 0 && atoi(partid) != memberId) {
				linphone_core_invite_spy(lc, confDID, memberId);
			}
		}
	}
}

static void * MeetphoneEslEventProcess(void *p)
{
	esl_status_t status = ESL_FAIL;
	while((status = esl_recv_timed(&event_handle, 1000)) != ESL_FAIL) {
		if (done) {
			break;
		} else if (status == ESL_SUCCESS) {
			ms_message("esl event %s recv", event_handle.last_event->body);
		}
	}
	ms_thread_exit(NULL);
	return NULL;
}

void MeetphoneEslInit()
{
	esl_connect(&api_handle, "202.109.211.109", 8021, NULL, "ClueCon");
	esl_connect(&event_handle, "202.109.211.109", 8021, NULL, "ClueCon");
	esl_events(&event_handle, ESL_EVENT_TYPE_PLAIN, "CUSTOM conference::maintenance");
	ms_thread_create(&event_thread,NULL,MeetphoneEslEventProcess,NULL);
}

void MeetphoneEslUninit()
{
	if(event_thread != 0)
	{
		done = true;
		ms_thread_join(event_thread, NULL);
	}
}