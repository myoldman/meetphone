#pragma once
#ifndef meetphonecore_h
#define meetphonecore_h

#include "private.h"
#include "linphonecore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MeetphoneOutputLog{
	OrtpLogLevel lev;
	CString msg;
	int length;
}MeetphoneOutputLog;

void CALLBACK MeetphoneIterate(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);
void CALLBACK MeetphoneLog(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);
void MeetphoneLogHandler(OrtpLogLevel lev, const char *fmt, va_list args);
void MeetphoneRegistrationStateChanged(LinphoneCore *lc, LinphoneProxyConfig *cfg, 
											  LinphoneRegistrationState rs, const char *msg);
void MeetphoneCallStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cs, const char *msg);
void MeetphoneDisplayStatus(LinphoneCore *lc, const char *status);
void MeetphoneShowMainDlg();
void MeetphoneShowMeetDlg(LinphoneCall *call);

#ifdef __cplusplus
} 
#endif

#endif