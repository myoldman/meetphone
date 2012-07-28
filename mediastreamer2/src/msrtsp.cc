#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/mswebcam.h"
#include "mediastreamer2/msrtsp.h"

#include "liveMedia/liveMedia.hh"
#include "BasicUsageEnvironment/BasicUsageEnvironment.hh"
#ifdef WIN32
typedef unsigned int in_addr_t;
#endif


#define MAX_RTP_SIZE	1500
#define payload_type_set_number(pt,n)	(pt)->user_data=(void*)((long)n);
#define payload_type_get_number(pt)		((int)(long)(pt)->user_data)
#define RTSP_TIME_OUT 60

class RTSPClientLinphone : public RTSPClient
{
public:
    RTSPClientLinphone( UsageEnvironment& env, char const* rtspURL, int verbosityLevel,
                   char const* applicationName, portNumBits tunnelOverHTTPPortNum) :
                   RTSPClient( env, rtspURL, verbosityLevel, applicationName,
                   tunnelOverHTTPPortNum )
    {
		event_rtsp = 0;
		i_live555_ret = 0;
		b_error = false;
		p_sdp = NULL;
    }
	char event_rtsp;
	int i_live555_ret;
	bool b_error;
    char *p_sdp;
};

struct RtspReceiverData {
	RtpSession *session;
	int local_port;
	int dest_port;
	int payload_type;
	int clock_rate;
	char *dest_addr;
	char *rtsp_url;
	char *recv_fmtp;
	char *mime_type;
	bool_t starting;
	RTSPClientLinphone *rtspClient;
	MediaSession *rtspMediaSession;
	UsageEnvironment* env;
	TaskScheduler* scheduler;
};

typedef struct RtspReceiverData RtspReceiverData;

/*****************************************************************************
 *
 *****************************************************************************/
static void TaskInterruptRTSP( void *p_private )
{
    RTSPClientLinphone *client_linphone = (RTSPClientLinphone*)p_private;

    /* Avoid lock */
    client_linphone->event_rtsp = 0xff;
}

static void default_live555_callback( RTSPClient* client, int result_code, char* result_string )
{
    RTSPClientLinphone *client_linphone = static_cast<RTSPClientLinphone *> ( client );
    delete []result_string;
    client_linphone->i_live555_ret = result_code;
    client_linphone->b_error = client_linphone->i_live555_ret != 0;
    client_linphone->event_rtsp = 1;
}

/* return true if the RTSP command succeeded */
static bool wait_Live555_response( RTSPClient* client, TaskScheduler* scheduler, int i_timeout = 0 /* ms */ )
{
    TaskToken task;
    RTSPClientLinphone *client_linphone = static_cast<RTSPClientLinphone *> ( client );
    client_linphone->event_rtsp = 0;
    if( i_timeout > 0 )
    {
        /* Create a task that will be called if we wait more than timeout ms */
        task = scheduler->scheduleDelayedTask( i_timeout * 1000000,
                                                      (TaskFunc*)TaskInterruptRTSP,
                                                      client_linphone );
    }
    client_linphone->event_rtsp = 0;
    client_linphone->b_error = true;
    client_linphone->i_live555_ret = 0;
    scheduler->doEventLoop( &client_linphone->event_rtsp );
    //here, if b_error is true and i_live555_ret = 0 we didn't receive a response
    if( i_timeout > 0 )
    {
        /* remove the task */
        scheduler->unscheduleDelayedTask( task );
    }
    return !client_linphone->b_error;
}

static void continueAfterDESCRIBE( RTSPClient* client, int result_code,
                                   char* result_string )
{
    RTSPClientLinphone *client_linphone = static_cast<RTSPClientLinphone *> (client);
    client_linphone->i_live555_ret = result_code;
    if ( result_code == 0 )
    {
        char* sdpDescription = result_string;
        free( client_linphone->p_sdp );
        client_linphone->p_sdp = NULL;
        if( sdpDescription )
        {
            client_linphone->p_sdp = _strdup( sdpDescription );
            client_linphone->b_error = false;
        }
    }
    else
        client_linphone->b_error = true;
    delete[] result_string;
    client_linphone->event_rtsp = 1;
}

static void continueAfterOPTIONS( RTSPClient* client, int result_code,
                                  char* result_string )
{
    RTSPClientLinphone *client_linphone = static_cast<RTSPClientLinphone *> (client);
    client_linphone->i_live555_ret = result_code;
    if ( result_code != 0 )
    {
        client_linphone->b_error = true;
        client_linphone->event_rtsp = 1;
    }
    else
    {
        client_linphone->sendDescribeCommand( continueAfterDESCRIBE );
    }
    delete[] result_string;
}

MSList *ms_rtsp_connect(const char *rtsp_url, int video_port, int audio_port, char *dest_addr, void **remote_rtsp_client, void **remove_rtsp_mediasession)
{
	char* username;
	char* password;
	NetAddress destAddress;
	portNumBits urlPortNum;
	char const* urlSuffix;
	UsageEnvironment* env;
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);
	if(!RTSPClient::parseRTSPURL(*env, rtsp_url, username, password, destAddress, urlPortNum, &urlSuffix)){
		return NULL;
		delete scheduler;
	}

	struct in_addr destinationAddr; 
	destinationAddr.s_addr = *(in_addr_t*)destAddress.data();
	char* temp_addr = inet_ntoa(destinationAddr);
	strcpy(dest_addr, temp_addr);

	RTSPClientLinphone *linphoneRtspClient= new RTSPClientLinphone( *env, rtsp_url, 1, "Linphone", 0);

	linphoneRtspClient->sendOptionsCommand( &continueAfterOPTIONS, NULL );

	if( !wait_Live555_response( linphoneRtspClient, scheduler, RTSP_TIME_OUT ) )
    {
        int i_code = linphoneRtspClient->i_live555_ret;

        if( i_code == 0 )
            ms_error( "connection timeout" );
         else
            ms_error( "connection error %d", i_code );
        RTSPClient::close( linphoneRtspClient );
		return NULL;
    }

	MediaSession *remoteRtspMediaSession = MediaSession::createNew(*env, linphoneRtspClient->p_sdp);

	MediaSubsessionIterator iter(*remoteRtspMediaSession);
	MediaSubsession *subsession;
	MSList *l=NULL;
	while ((subsession = iter.next()) != NULL) {
		ms_message("Send setup message %d %s \n", subsession->clientPortNum(), subsession->codecName());
		if(strcmp(subsession->mediumName(), "video") == 0 &&
            		(strcmp(subsession->codecName(), "H264") == 0)) {
			subsession->setClientPortNum(video_port);
		}

		if(strcmp(subsession->mediumName(), "audio") == 0 &&  
            		(strcmp(subsession->codecName(), "PCMA") == 0)) {
			subsession->setClientPortNum(audio_port);
		}
		
		if(strcmp(subsession->mediumName(), "audio") == 0 &&  
            		(strcmp(subsession->codecName(), "PCMU") == 0)) {
			subsession->setClientPortNum(audio_port);
		}
	
		if (subsession->clientPortNum() == 0) continue; // port # was not set  

		linphoneRtspClient->sendSetupCommand(*subsession, default_live555_callback, False, False, False);
		if (!wait_Live555_response( linphoneRtspClient, scheduler, RTSP_TIME_OUT )) {
			*env << "Failed to setup " << subsession->mediumName()
			<< "/" << subsession->codecName()
			<< " subsession: " << env->getResultMsg() << "\n";
		} else {  
			*env << "Setup " << subsession->mediumName()
			<< "/" << subsession->codecName() << " " << subsession->serverPortNum
			<< " subsession (client ports " << subsession->clientPortNum()
			<< "-" << subsession->clientPortNum()+1 << subsession->rtpPayloadFormat() << ")\n";
			MSRtspSubSession *rtsp_sub = (MSRtspSubSession *)ms_new0(MSRtspSubSession, 1);
			strncpy(rtsp_sub->addr,dest_addr,sizeof(rtsp_sub->addr));
			strncpy(rtsp_sub->mime_type,subsession->codecName(),sizeof(rtsp_sub->mime_type));
			rtsp_sub->server_port=subsession->serverPortNum == 0 ? 65500 : subsession->serverPortNum;
			rtsp_sub->payload_type = subsession->rtpPayloadFormat();
			rtsp_sub->clock_rate = subsession->rtpTimestampFrequency();
			if(strcmp(subsession->mediumName(), "video") == 0 &&
            		(strcmp(subsession->codecName(), "H264") == 0)) {
				const char *propset  = subsession->fmtp_spropparametersets();
				const char *format = (const char *)"sprop-parameter-sets=%s";
				sprintf(rtsp_sub->recv_fmtp, format, propset);
			}
			l=ms_list_append(l,rtsp_sub);
		}
	}
	
	*remote_rtsp_client = linphoneRtspClient;
	*remove_rtsp_mediasession = remoteRtspMediaSession;
	return l;
}

void ms_rtsp_play(void *remote_rtsp_client, void *remove_rtsp_mediasession)
{
	RTSPClientLinphone *remoteRtspClient = (RTSPClientLinphone *)remote_rtsp_client;
	MediaSession *remoteRtspMediaSession = (MediaSession *)remove_rtsp_mediasession;
	if(remoteRtspMediaSession != NULL) {
		remoteRtspClient->sendPlayCommand( *remoteRtspMediaSession, default_live555_callback );
	}
}

void ms_rtsp_disconnect(void *remote_rtsp_client, void *remove_rtsp_mediasession)
{
	
	RTSPClientLinphone *remoteRtspClient = (RTSPClientLinphone *)remote_rtsp_client;
	MediaSession *remoteRtspMediaSession = (MediaSession *)remove_rtsp_mediasession;
	if(remoteRtspClient != NULL && remoteRtspClient->p_sdp != NULL) {
		delete[] remoteRtspClient->p_sdp;
	}
	if(remoteRtspClient != NULL && remoteRtspMediaSession != NULL){
		remoteRtspClient->sendTeardownCommand(*remoteRtspMediaSession, NULL);
	}
	if(remoteRtspMediaSession != NULL)
		Medium::close(remoteRtspMediaSession);
	if(remoteRtspClient != NULL)
		Medium::close(remoteRtspClient);
}

static void disable_checksums(ortp_socket_t sock){
#if defined(DISABLE_CHECKSUMS) && defined(SO_NO_CHECK)
	int option=1;
	if (setsockopt(sock,SOL_SOCKET,SO_NO_CHECK,&option,sizeof(option))==-1){
		ms_warning("Could not disable udp checksum: %s",strerror(errno));
	}
#endif
}

static RtpSession * create_duplex_rtpsession( int locport, bool_t ipv6){
	RtpSession *rtpr;
	rtpr=rtp_session_new(RTP_SESSION_SENDRECV);
	rtp_session_set_recv_buf_size(rtpr,MAX_RTP_SIZE);
	rtp_session_set_scheduling_mode(rtpr,0);
	rtp_session_set_blocking_mode(rtpr,0);
	const int socket_buf_size=1000000;
	rtp_session_set_rtp_socket_recv_buffer_size(rtpr,socket_buf_size);
	rtp_session_set_rtp_socket_send_buffer_size(rtpr,socket_buf_size);
	rtp_session_enable_adaptive_jitter_compensation(rtpr,TRUE);
	rtp_session_set_symmetric_rtp(rtpr,TRUE);
	rtp_session_set_local_addr(rtpr,ipv6 ? "::" : "0.0.0.0",locport);
	//rtp_session_signal_connect(rtpr,"timestamp_jump",(RtpCallback)rtp_session_resync,(long)NULL);
	//rtp_session_signal_connect(rtpr,"ssrc_changed",(RtpCallback)rtp_session_resync,(long)NULL);
	rtp_session_set_ssrc_changed_threshold(rtpr,0);
	rtp_session_set_rtcp_report_interval(rtpr,2500); /*at the beginning of the session send more reports*/
	disable_checksums(rtp_session_get_rtp_socket(rtpr));
	return rtpr;
}

static void rtsp_init(MSFilter * f)
{
	ms_message("rtsp init");
	RtspReceiverData *d = (RtspReceiverData *)ms_new(RtspReceiverData, 1);
	d->starting=FALSE;
	d->session = NULL;
	d->rtspClient = NULL;
	d->rtspMediaSession = NULL;
	d->env = NULL;
	d->scheduler = NULL;
	d->recv_fmtp = NULL;
	d->rtsp_url = NULL;
	d->local_port = 10000;
	d->dest_port = 0;
	d->clock_rate = 0;
	d->payload_type = 0;
	d->mime_type = NULL;
	d->dest_addr = NULL;
	f->data = d;
}

static void rtsp_preprocess(MSFilter * f){
	RtspReceiverData *d = (RtspReceiverData *) f->data;
}

static void rtsp_postprocess(MSFilter * f){
	RtspReceiverData *d = (RtspReceiverData *) f->data;
}

static void rtsp_uninit(MSFilter * f){
	RtspReceiverData *d = (RtspReceiverData *) f->data;
	ms_free(d);
}

static void rtsp_process(MSFilter * f)
{
	RtspReceiverData *d = (RtspReceiverData *) f->data;
	mblk_t *m;
	uint32_t timestamp;
	if (d->session == NULL)
		return;

	if (d->starting){
		
		PayloadType *pt=rtp_profile_get_payload(
			rtp_session_get_profile(d->session),
			rtp_session_get_recv_payload_type(d->session));
		if (pt && pt->type!=PAYLOAD_VIDEO)
			rtp_session_flush_sockets(d->session);
		d->starting=FALSE;
	}

	timestamp = (uint32_t) (f->ticker->time * (d->clock_rate/1000));
	while ((m = rtp_session_recvm_with_ts(d->session, timestamp)) != NULL) {
		mblk_set_timestamp_info(m, rtp_get_timestamp(m));
		mblk_set_marker_info(m, rtp_get_markbit(m));
		mblk_set_cseq(m, rtp_get_seqnumber(m));
		rtp_get_payload(m,&m->b_rptr);
		ms_queue_put(f->outputs[0], m);
		if(f->outputs[1] != NULL) {
			ms_queue_put(f->outputs[1], dupmsg(m));
		}
	}
}

static int msrtsp_describe(MSFilter *f, void *arg){
	RtspReceiverData *d=(RtspReceiverData*)f->data;
	if (d->rtsp_url) ms_free(d->rtsp_url);
	d->rtsp_url=ms_strdup((char*)arg);

	d->scheduler = BasicTaskScheduler::createNew();
	
	d->env = BasicUsageEnvironment::createNew(*d->scheduler);
	
	char* username;
	char* password;
	NetAddress destAddress;
	portNumBits urlPortNum;
	char const* urlSuffix;
	
	if(!RTSPClient::parseRTSPURL(*d->env, d->rtsp_url, username, password, destAddress, urlPortNum, &urlSuffix)){
		delete d->scheduler;
		return -1;
	}
	
	struct in_addr destinationAddr; 
	destinationAddr.s_addr = *(in_addr_t*)destAddress.data();
	char* temp_dest_addr = inet_ntoa(destinationAddr);
	d->dest_addr = ms_strdup(temp_dest_addr);
	
	d->rtspClient = new RTSPClientLinphone( *d->env, d->rtsp_url, 1, "Linphone", 0);

	d->rtspClient->sendOptionsCommand( &continueAfterOPTIONS, NULL );

	if( !wait_Live555_response(d->rtspClient, d->scheduler, RTSP_TIME_OUT ) )
    {
        int i_code = d->rtspClient->i_live555_ret;

        if( i_code == 0 )
            ms_error( "connection timeout" );
         else
            ms_error( "connection error %d", i_code );
        RTSPClient::close( d->rtspClient );
		d->rtspClient = NULL;
		return -1;
    }
	
	d->rtspMediaSession = MediaSession::createNew(*d->env, d->rtspClient->p_sdp);

	MediaSubsessionIterator iter(*d->rtspMediaSession);
	MediaSubsession *subsession;
		
	
	while ((subsession = iter.next()) != NULL) {
		if(strcmp(subsession->mediumName(), "video") == 0 &&
			(strcmp(subsession->codecName(), "H264") == 0)) {
			subsession->setClientPortNum(d->local_port);
		}
		
		if(strcmp(subsession->mediumName(), "audio") == 0 &&  
            		(strcmp(subsession->codecName(), "PCMA") == 0)) {
			subsession->setClientPortNum(10002);
		}
		
		if(strcmp(subsession->mediumName(), "audio") == 0 &&  
            		(strcmp(subsession->codecName(), "PCMU") == 0)) {
			subsession->setClientPortNum(10002);
		}

		if(strcmp(subsession->mediumName(), "audio") == 0 &&  
            		(_strnicmp(subsession->codecName(), "mpeg4", 5) == 0)) {
			subsession->setClientPortNum(10002);
		}

		if (subsession->clientPortNum() == 0) continue; // port # was not set  
		d->rtspClient->sendSetupCommand(*subsession, default_live555_callback, False, False, False);
		if (!wait_Live555_response( d->rtspClient, d->scheduler, RTSP_TIME_OUT )) {
			*d->env << "Failed to setup " << subsession->mediumName()
			<< "/" << subsession->codecName()
			<< " subsession: " << d->env->getResultMsg() << "\n";
		} else {  
			if(strcmp(subsession->mediumName(), "video") == 0 &&
				(strcmp(subsession->codecName(), "H264") == 0)) {
				d->dest_port = subsession->serverPortNum;
				d->payload_type = subsession->rtpPayloadFormat();
				d->clock_rate = subsession->rtpTimestampFrequency();
				d->mime_type = ms_strdup(subsession->codecName());
				const char *propset  = subsession->fmtp_spropparametersets();
				char recv_fmtp[256] = {0};
				const char *format = (const char *)"sprop-parameter-sets=%s";
				sprintf(recv_fmtp, format, propset);
				d->recv_fmtp = ms_strdup(recv_fmtp);
				//ms_message("h264 propset is %s", recv_fmtp);
			}
		}
	}
	
	JBParameters jbp;
	const int socket_buf_size=2000000;
	d->session = create_duplex_rtpsession(d->local_port,FALSE);
	RtpSession *rtps=d->session;
	RtpProfile *prof=rtp_profile_new("Call profile");
	PayloadType *pt = payload_type_new();
	pt->type=PAYLOAD_VIDEO;
	payload_type_set_number(pt,d->payload_type);
	payload_type_set_flag(pt,PAYLOAD_TYPE_USER_FLAG_0);
	payload_type_set_flag(pt,PAYLOAD_TYPE_USER_FLAG_1);
	payload_type_set_flag(pt,PAYLOAD_TYPE_USER_FLAG_2);
	pt->mime_type = ms_strdup(d->mime_type);
	pt->clock_rate = d->clock_rate;
	payload_type_set_recv_fmtp(pt, d->recv_fmtp);
	rtp_profile_set_payload(prof,d->payload_type,pt);
	rtp_session_set_profile(rtps,prof);
	rtp_session_set_remote_addr_full(rtps,d->dest_addr,d->dest_port,d->dest_port+1);
	rtp_session_set_payload_type(rtps,d->payload_type);
	rtp_session_set_jitter_compensation(rtps,60);
	rtp_session_set_recv_buf_size(rtps,MAX_RTP_SIZE);
	rtp_session_get_jitter_buffer_params(rtps,&jbp);
	jbp.max_packets=1000;//needed for high resolution video
	rtp_session_set_jitter_buffer_params(rtps,&jbp);
	rtp_session_set_rtp_socket_recv_buffer_size(rtps,socket_buf_size);
	rtp_session_set_rtp_socket_send_buffer_size(rtps,socket_buf_size);
	d->rtspClient->sendPlayCommand( *d->rtspMediaSession, default_live555_callback );
	wait_Live555_response(d->rtspClient, d->scheduler, RTSP_TIME_OUT);
	d->starting=TRUE;	
	return 0;
}

static int msrtsp_teardown(MSFilter *f, void *arg) {
	RtspReceiverData *d=(RtspReceiverData*)f->data;
	if(d->session != NULL) {
		rtp_stats_display(rtp_session_get_stats(d->session),"Rtsp Preview session's RTP statistics");
		rtp_session_destroy(d->session);
	}

	if(d->rtspClient != NULL && d->rtspClient->p_sdp != NULL) {
		delete[] d->rtspClient->p_sdp;
	}

	if(d->rtspClient != NULL && d->rtspMediaSession != NULL){
		d->rtspClient->sendTeardownCommand(*d->rtspMediaSession, NULL);
	}
	if(d->rtspMediaSession != NULL)
		Medium::close(d->rtspMediaSession);
	if(d->rtspClient != NULL)
		Medium::close(d->rtspClient);
	
	if(d->rtsp_url != NULL)
		ms_free(d->rtsp_url);
	if(d->dest_addr != NULL)
		ms_free(d->dest_addr);
	if(d->recv_fmtp != NULL)
		ms_free(d->recv_fmtp);
	if(d->mime_type != NULL)
		ms_free(d->mime_type);
	
	d->rtsp_url = NULL;
	d->dest_addr = NULL;
	d->recv_fmtp = NULL;
	d->mime_type = NULL;
	d->rtspMediaSession = NULL;
	d->rtspClient = NULL;
	d->starting=FALSE;
	return 1;
}

static int msrtsp_get_fmtp(MSFilter *f, void *arg){
	RtspReceiverData *s=(RtspReceiverData*)f->data;
	*(char**)arg=s->recv_fmtp;
	return 0;
}

static int msrtsp_get_mimetype(MSFilter *f, void *arg){
	RtspReceiverData *s=(RtspReceiverData*)f->data;
	*(char**)arg=s->mime_type;
	return 0;
}

static MSFilterMethod rtsp_methods[]={
	{	MS_RTSP_GET_FMTP,	msrtsp_get_fmtp },
	{	MS_RTSP_GET_MIME_TYPE,	msrtsp_get_mimetype },
	{	MS_RTSP_DESCRIBE,	msrtsp_describe },
	{	MS_RTSP_TEARDOWN,	msrtsp_teardown },
	{	0			,	NULL}
};

#if defined(_MSC_VER) || defined(__cplusplus)

MSFilterDesc ms_rtsp_capture_desc = {
	MS_RTSP_CAPTURE_ID,
	"MSRtspCature",
	N_("A filter to grab pictures from rtsp based cameras"),
	MS_FILTER_OTHER,
	NULL,
	0,
	2,
	rtsp_init,
	rtsp_preprocess,
	rtsp_process,
	rtsp_postprocess,
	rtsp_uninit,
	rtsp_methods
};

#else

MSFilterDesc ms_rtsp_capture_desc = {
	.id = MS_RTSP_CAPTURE_ID,
	.name = "MSRtspCature",
	.text = N_("A filter to grab pictures from rtsp based cameras"),
	.category = MS_FILTER_OTHER,
	.ninputs = 0,
	.noutputs = 2,
	.init = rtsp_init,
	.preprocess = rtsp_preprocess,
	.process = rtsp_process,
	.postprocess=rtsp_postprocess,
	.uninit = rtsp_uninit,
	.methods = rtsp_methods
};

#endif
static void msrtsp_detect(MSWebCamManager *obj);

static MSFilter *msrtsp_create_reader(MSWebCam *obj){
	MSFilter *f=ms_filter_new(MS_RTSP_CAPTURE_ID);
	return f;
}

static void msrtsp_cam_init(MSWebCam *cam){
}

#ifdef __cplusplus  
extern "C"{  
#endif  
MSWebCamDesc rtsp_card_desc={
	"RTSP",
	&msrtsp_detect,
	&msrtsp_cam_init,
	&msrtsp_create_reader,
	NULL
};

#ifdef __cplusplus  
}  
#endif  


static void msrtsp_detect(MSWebCamManager *obj){
	/* is a Rtsp Camera */
	MSWebCam *cam=ms_web_cam_new(&rtsp_card_desc);
	ms_web_cam_manager_add_cam(obj,cam);
}

MS_FILTER_DESC_EXPORT(ms_rtsp_capture_desc)