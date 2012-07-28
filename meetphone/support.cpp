#include "StdAfx.h"
#include "lpconfig.h"
#include "meetphone.h"
#include "support.h"
#include <afxinet.h>

#ifdef __cplusplus
extern "C" {
#endif

	void meetphone_get_conf_server(CString& confServer)
	{
		char *conf_server = ortp_strdup(meetphone_get_ui_config("login_server", "192.168.1.77"));
		char *port = NULL;
		port=strchr(conf_server, ':');
		if(port != NULL) {
			*port++='\0';		
		}
		confServer = conf_server;
		ortp_free(conf_server);
	}

	int meetphone_get_json_port()
	{
		return meetphone_get_ui_config_int("login_port",9080);
	}

	const char *meetphone_get_ui_config(const char *key, const char *def){
		LinphoneCore *lc = theApp.GetCore();
		if (lc){
			LpConfig *cfg=linphone_core_get_config(lc);
			return lp_config_get_string(cfg,"GtkUi",key,def);
		}else{
			ms_error ("Cannot read config, no core created yet.");
			return NULL;
		}
	}

	int meetphone_get_ui_config_int(const char *key, int def){
		LpConfig *cfg=linphone_core_get_config(theApp.GetCore());
		return lp_config_get_int(cfg,"GtkUi",key,def);
	}

	void meetphone_set_ui_config_int(const char *key , int val){
		LpConfig *cfg=linphone_core_get_config(theApp.GetCore());
		lp_config_set_int(cfg,"GtkUi",key,val);
	}

	void meetphone_set_ui_config(const char *key , const char * val){
		LpConfig *cfg=linphone_core_get_config(theApp.GetCore());
		lp_config_set_string(cfg,"GtkUi",key,val);
	}

	void convert_utf8_to_unicode(const char *utf8, CString &unicode)
	{
		int unicodeLen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
		WCHAR *pUnicode = new WCHAR[unicodeLen + 1];
		memset(pUnicode,0,(unicodeLen+1)*sizeof(wchar_t));

		MultiByteToWideChar(CP_UTF8,0,utf8,-1, pUnicode, unicodeLen);
		unicode = pUnicode;
		delete []pUnicode;
	}

	BOOL http_get_request(CString &restMethod, Json::Value &response )
	{
		CInternetSession session;
		CString confServer;
		BOOL ret = FALSE;
		LinphoneCore *lc = theApp.GetCore();

		session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 1000 * 20);
		session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
		session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 1);
			
		meetphone_get_conf_server(confServer);
		LinphoneProxyConfig *cfg;
		linphone_core_get_default_proxy(lc, &cfg);

		CHttpConnection* pConnection = session.GetHttpConnection(confServer,(INTERNET_PORT)meetphone_get_json_port());
		CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET, restMethod, 0,1,0,0,INTERNET_FLAG_DONT_CACHE);
		pFile->SendRequest();
		DWORD dwRet;
		pFile->QueryInfoStatusCode(dwRet);
		if(dwRet != HTTP_STATUS_OK)
		{
			CString errText;
			errText.Format(L"GET出错，错误码：%d", dwRet);
			AfxMessageBox(errText);
		} else {
			int len = (int)pFile->GetLength();
			char strBuff[1025] = {0};
			std::string strHtml; //是string 不是CString
			while ((pFile->Read((void*)strBuff, 1024)) > 0)
			{
				strHtml += strBuff;
			}

			Json::Reader reader;
			Json::Value json_object;
			if (reader.parse(strHtml, response) && response.isArray()){
				ret = TRUE;
			}
		}
		session.Close();
		pFile->Close(); 
		delete pFile;
		return ret;
	}

	BOOL http_post_request(CString &restMethod, CString &formData, Json::Value &response )
	{
		CInternetSession session;
		CString confServer;
		BOOL ret = FALSE;
		LinphoneCore *lc = theApp.GetCore();

		session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 1000 * 20);
		session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
		session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 1000);
			
		meetphone_get_conf_server(confServer);
		LinphoneProxyConfig *cfg;
		linphone_core_get_default_proxy(lc, &cfg);
		CString  strHeaders    = _T("Content-Type: application/x-www-form-urlencoded");
		CHttpConnection* pConnection = session.GetHttpConnection(confServer,(INTERNET_PORT)meetphone_get_json_port());
		CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, restMethod, 0,1,0,0,INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT);
		 char strUtf8Req[512];  
		 memset( strUtf8Req, 0, 512);
		WideCharToMultiByte(CP_UTF8, 0, formData, -1, strUtf8Req, 512, NULL, NULL );
		pFile->SendRequest(strHeaders,0,(LPVOID)strUtf8Req,strlen(strUtf8Req));
		DWORD dwRet;
		pFile->QueryInfoStatusCode(dwRet);
		if(dwRet != HTTP_STATUS_OK && dwRet != HTTP_STATUS_REDIRECT)
		{
			CString errText;
			errText.Format(L"POST出错，错误码：%d", dwRet);
			AfxMessageBox(errText);
		} else {
			int len = (int)pFile->GetLength();
			char strBuff[1025] = {0};
			std::string strHtml; //是string 不是CString
			while ((pFile->Read((void*)strBuff, 1024)) > 0)
			{
				strHtml += strBuff;
			}

			Json::Reader reader;
			Json::Value json_object;
			if (reader.parse(strHtml, response) && response.isArray()){
				ret = TRUE;
			}
		}
		session.Close();
		pFile->Close(); 
		delete pFile;
		return ret;
	}

#ifdef __cplusplus
} 
#endif