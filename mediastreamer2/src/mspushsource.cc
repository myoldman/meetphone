#include <mediastreamer2/mswebcam.h>
#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msticker.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msdshow.h>

static ComPtr< IPin > getPin( IBaseFilter *filter, PIN_DIRECTION direction, int num )
{
	ComPtr< IPin > retVal;
	ComPtr< IEnumPins > enumPins;
	if (filter->EnumPins( &enumPins )!=S_OK){
		ms_error("Error getting pin enumerator" );
		return retVal;
	}
	ULONG found=0;
	ComPtr< IPin > pin;
	while ( enumPins->Next( 1, &pin, &found ) == S_OK && found > 0) {
		found=0; //reset for next loop
		PIN_DIRECTION pinDirection = (PIN_DIRECTION)( -1 );
		pin->QueryDirection( &pinDirection );
		if ( pinDirection == direction ) {
			if ( num == 0 ) {
				retVal = pin;
				break;
			};
			num--;
		};
	};
	return retVal;
}


class PushSourceCapture : public ISampleGrabberCB{
public:
	PushSourceCapture(){
		qinit(&_rq);
		ms_mutex_init(&_mutex,NULL);
		_vsize.width=MS_VIDEO_SIZE_CIF_W;
		_vsize.height=MS_VIDEO_SIZE_CIF_H;
		_fps=15;
		_start_time=0;
		_frame_count=0;
		_pixfmt=MS_YUV420P;
		_ready=false;
		m_refCount=1;
	}
	virtual ~PushSourceCapture(){
		if (_ready) stopAndClean();
		flushq(&_rq,0);
		ms_mutex_destroy(&_mutex);
	}
	STDMETHODIMP QueryInterface( REFIID riid, void **ppv );
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP SampleCB(double,IMediaSample*);
	STDMETHODIMP BufferCB(double,BYTE*,long);
	int startDshowGraph();
	void stopAndClean();
	mblk_t *readFrame(){
		mblk_t *ret=NULL;
		ms_mutex_lock(&_mutex);
		ret=getq(&_rq);
		ms_mutex_unlock(&_mutex);
		return ret;
	}
	bool isTimeToSend(uint64_t ticker_time);
	MSVideoSize getVSize(){
		if (!_ready) createDshowGraph(); /* so that _vsize is updated according to hardware capabilities*/
		return _vsize;
	}
	void setVSize(MSVideoSize vsize){
		_vsize=vsize;
	}
	void setFps(float fps){
		_fps=fps;
	}
	MSPixFmt getPixFmt(){
		if (!_ready) createDshowGraph(); /* so that _pixfmt is updated*/
		return _pixfmt;
	}
protected:
  	long m_refCount;
private:
	int createDshowGraph();
	int	selectBestFormat(ComPtr<IAMStreamConfig> streamConfig, int count);
	int _devid;
	MSVideoSize _vsize;
	queue_t _rq;
	ms_mutex_t _mutex;
	float _fps;
	float _start_time;
	int _frame_count;
	MSPixFmt _pixfmt;
	ComPtr< IGraphBuilder > _graphBuilder;
	ComPtr< IBaseFilter > _source;
	ComPtr< IBaseFilter > _nullRenderer;
	ComPtr< IBaseFilter > _grabberBase;
	ComPtr< IBaseFilter > _pushdesktopBase;//add for desktop capture
	ComPtr< IMediaControl > _mediaControl;
	ComPtr< IMediaEvent > _mediaEvent;
	bool _ready;
};


STDMETHODIMP PushSourceCapture::QueryInterface(REFIID riid, void **ppv)
{
  HRESULT retval;
  if ( ppv == NULL ) return E_POINTER;
  /*
  if ( riid == IID_IUnknown ) {
    *ppv = static_cast< IUnknown * >( this );
    AddRef();
    retval = S_OK;
  } else if ( riid == IID_ISampleGrabberCB ) {
    *ppv = static_cast< ISampleGrabberCB * >( this );
    AddRef();
    retval = S_OK;
    } else */ {
    retval = E_NOINTERFACE;
  };
  return retval;
};

STDMETHODIMP_(ULONG) PushSourceCapture::AddRef(){
	m_refCount++;
	return m_refCount;
}

STDMETHODIMP_(ULONG) PushSourceCapture::Release()
{
  ms_message("PushSourceCapture::Release");
  if ( !InterlockedDecrement( &m_refCount ) ) {
		int refcnt=m_refCount;
		delete this;
		return refcnt;
	}
  return m_refCount;
}

static void dummy(void*p){
}

STDMETHODIMP PushSourceCapture::SampleCB( double par1 , IMediaSample * sample)
{
	uint8_t *p;
	unsigned int size;
	if (sample->GetPointer(&p)!=S_OK){
		ms_error("error in GetPointer()");
		return S_OK;
	}
	size=sample->GetSize();
	mblk_t *m=esballoc(p,size,0,dummy);
	m->b_wptr+=size;
	ms_mutex_lock(&_mutex);
	putq(&_rq,ms_yuv_buf_alloc_from_buffer(_vsize.width,_vsize.height,m));
	ms_mutex_unlock(&_mutex);
  	return S_OK;
}



STDMETHODIMP PushSourceCapture::BufferCB( double, BYTE *b, long len)
{
	ms_message("PushSourceCapture::BufferCB");
	return S_OK;
}

static void pushsource_init(MSFilter *f){
	PushSourceCapture *s=new PushSourceCapture();
	f->data=s;
}



static void pushsource_uninit(MSFilter *f){
	PushSourceCapture *s=(PushSourceCapture*)f->data;
	s->Release();
}

static char * fourcc_to_char(char *str, uint32_t fcc){
	memcpy(str,&fcc,4);
	str[4]='\0';
	return str;
}

static int find_best_format(ComPtr<IAMStreamConfig> streamConfig, int count,MSVideoSize *requested_size, MSPixFmt requested_fmt ){
	int i;
	MSVideoSize best_found={0,0};
	int best_index=-1;
	char fccstr[5];
	char selected_fcc[5];
	for (i=0; i<count; i++ ) {
		VIDEO_STREAM_CONFIG_CAPS videoConfig;
		AM_MEDIA_TYPE *mediaType;
		if (streamConfig->GetStreamCaps( i, &mediaType,
                                                 (BYTE *)&videoConfig )!=S_OK){
			ms_error( "Error getting stream capabilities");
			return -1;
		}
		if ( mediaType->majortype == MEDIATYPE_Video &&
           mediaType->cbFormat != 0 ) {
			VIDEOINFOHEADER *infoHeader = (VIDEOINFOHEADER*)mediaType->pbFormat;
			ms_message("Seeing format %ix%i %s %i, %i",infoHeader->bmiHeader.biWidth,infoHeader->bmiHeader.biHeight,
					fourcc_to_char(fccstr,infoHeader->bmiHeader.biCompression),MAKEFOURCC('I','4','2','0'), i);
			if (ms_fourcc_to_pix_fmt(infoHeader->bmiHeader.biCompression)==requested_fmt){
				MSVideoSize cur;
				cur.width=infoHeader->bmiHeader.biWidth;
				cur.height=infoHeader->bmiHeader.biHeight;
				ms_message("cur %ix%i, request %ix%i",cur.width, cur.height ,requested_size->width, requested_size->height );
				if (ms_video_size_greater_than(*requested_size,cur)){
					if (ms_video_size_greater_than(cur,best_found)){
						best_found=cur;
						best_index=i;
						fourcc_to_char(selected_fcc,infoHeader->bmiHeader.biCompression);
					}
				}
			}
		}
		if ( mediaType->cbFormat != 0 )
			CoTaskMemFree( (PVOID)mediaType->pbFormat );
		if ( mediaType->pUnk != NULL ) mediaType->pUnk->Release();
			CoTaskMemFree( (PVOID)mediaType );
	}
	if (best_index!=-1) {
		*requested_size=best_found;
		ms_message("Best camera format is %s %ix%i",selected_fcc,best_found.width,best_found.height);
	}
	return best_index;
}

int PushSourceCapture::selectBestFormat(ComPtr<IAMStreamConfig> streamConfig, int count){
	int index;
	_pixfmt=MS_YUV420P;
	index=find_best_format(streamConfig, count, &_vsize, _pixfmt);
	if (index!=-1) goto success;
	_pixfmt=MS_YUY2;
	index=find_best_format(streamConfig, count, &_vsize,_pixfmt);
	if (index!=-1) goto success;
	_pixfmt=MS_YUYV;
	index=find_best_format(streamConfig, count, &_vsize, _pixfmt);
	if (index!=-1) goto success;
	_pixfmt=MS_RGB24;
	index=find_best_format(streamConfig, count, &_vsize, _pixfmt);
	if (index!=-1) {
		_pixfmt=MS_RGB24_REV;
		goto success;
	}
	ms_error("This camera does not support any of our pixel formats.");
	return -1;
	
	success:
	ms_message("user format %d", _pixfmt);
	VIDEO_STREAM_CONFIG_CAPS videoConfig;
	AM_MEDIA_TYPE *mediaType;
	if (streamConfig->GetStreamCaps( index, &mediaType,
                    (BYTE *)&videoConfig )!=S_OK){
		ms_error( "Error getting stream capabilities" );
	}
 	streamConfig->SetFormat( mediaType );
	return 0;
}

int PushSourceCapture::createDshowGraph(){
	int ret = 0;
	CoInitialize(NULL);
	
	if (_graphBuilder.coCreateInstance( CLSID_FilterGraph, IID_IGraphBuilder )!=0){
		ms_error("Could not create graph builder.");
		return -1;
	}
	
	ret = CoCreateInstance( CLSID_PushSourceDesktop, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&_source );
	if (ret!=S_OK){
    		ms_error("Error creating push source filter %d" ,ret );
    		return -1;
	}

	if (_source.get()==0){
		ms_error("Could not interface with webcam devid=%i",_devid);
		return -1;
	}

 	if (_graphBuilder->AddFilter( _source.get(), L"Source" )!=S_OK){
    		ms_error("Error adding camera source to filter graph" );
    		return -1;
	}
	ComPtr< IPin > sourceOut = getPin( _source.get(), PINDIR_OUTPUT, 0 );
	if (sourceOut.get()==NULL){
		ms_error("Error getting output pin of camera source" );
		return -1;
	}
	
	ComPtr< IAMStreamConfig > streamConfig;
	if (sourceOut->QueryInterface( IID_IAMStreamConfig,
                                  (void **)&streamConfig )!=S_OK){
		ms_error("Error requesting stream configuration API" );
		return -1;
	}
		
	int count, size;
	if (streamConfig->GetNumberOfCapabilities( &count, &size )!=S_OK){
		ms_error("Error getting number of capabilities" );
		return -1;
	}
	if (selectBestFormat(streamConfig,count)!=0){
		return -1;
	}
	streamConfig.reset();

	if (CoCreateInstance( CLSID_SampleGrabber, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&_grabberBase )!=S_OK){
    		ms_error("Error creating sample grabber" );
    		return -1;
	}

	/*
	if (CoCreateInstance( CLSID_PushSourceDesktop, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&_pushdesktopBase )!=S_OK){
    		ms_error("Error creating _pushdesktopBase" );
    		return -1;
	}
	*/
	
	if (_graphBuilder->AddFilter( _grabberBase.get(), L"Grabber" )!=S_OK){
		ms_error("Error adding sample grabber to filter graph");
		return -1;
	}
	ComPtr< ISampleGrabber > sampleGrabber;
	if (_grabberBase->QueryInterface( IID_ISampleGrabber,
                                               (void **)&sampleGrabber )!=S_OK){
		ms_error("Error requesting sample grabber interface");
		return -1;
	}
		
	if (sampleGrabber->SetOneShot( FALSE )!=S_OK){
 		ms_error("Error disabling one-shot mode" );
		return -1;
	}
	if (sampleGrabber->SetBufferSamples( TRUE )!=S_OK){
		ms_error("Error enabling buffer sampling" );
		return -1;
	}
	if (sampleGrabber->SetCallBack(this, 0 )!=S_OK){
		ms_error("Error setting callback interface for grabbing" );
		return -1;
	}
    ComPtr< IPin > grabberIn = getPin( _grabberBase.get(), PINDIR_INPUT, 0 );
    if (grabberIn.get() == NULL){
        ms_error("Error getting input of sample grabber");
		return -1;
	}
    ComPtr< IPin > grabberOut = getPin( _grabberBase.get(), PINDIR_OUTPUT, 0 );
	if (grabberOut.get()==NULL){
		ms_error("Error getting output of sample grabber" );
		return -1;
	}
    if (CoCreateInstance( CLSID_NullRenderer, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&_nullRenderer )!=S_OK){
		ms_error("Error creating Null Renderer" );
		return -1;
	}
	if (_graphBuilder->AddFilter( _nullRenderer.get(), L"Sink" )!=S_OK){
        ms_error("Error adding null renderer to filter graph" );
        return -1;
	}
    ComPtr< IPin > nullIn = getPin( _nullRenderer.get(), PINDIR_INPUT, 0 );
	if (_graphBuilder->Connect( sourceOut.get(), grabberIn.get() )!=S_OK){
    	ms_error("Error connecting source to sample grabber" );
    	return -1;
	}
    if (_graphBuilder->Connect( grabberOut.get(), nullIn.get() )!=S_OK){
        ms_error("Error connecting sample grabber to sink" );
        return -1;
	}
    ms_message("Directshow graph is now ready to run.");

    if (_graphBuilder->QueryInterface( IID_IMediaControl,
                                                (void **)&_mediaControl )!=S_OK){
        ms_error("Error requesting media control interface" );
		return -1;
	}
    if (_graphBuilder->QueryInterface( IID_IMediaEvent,
                                        (void **)&_mediaEvent )!=S_OK){
    	ms_error("Error requesting event interface" );
    	return -1;
	}
	_ready=true;
	return 0;
}

int PushSourceCapture::startDshowGraph(){
	if (!_ready) {
		if (createDshowGraph()!=0) return -1;
	}
	HRESULT r=_mediaControl->Run();
	if (r!=S_OK && r!=S_FALSE){
		ms_error("Error starting graph (%i)",r);
		return -1;
	}
	ms_message("Graph started");
	return 0;
}

void PushSourceCapture::stopAndClean(){
	if (_mediaControl.get()!=NULL){
		HRESULT r;
		r=_mediaControl->Stop();
		if (r!=S_OK){
			ms_error("msdscap: Could not stop graph !");
			fflush(NULL);
		}
		
    	_graphBuilder->RemoveFilter(_source.get());
    	_graphBuilder->RemoveFilter(_grabberBase.get());
		_graphBuilder->RemoveFilter(_nullRenderer.get());
	}
	_source.reset();
	_grabberBase.reset();
	_nullRenderer.reset();
	_mediaControl.reset();
	_mediaEvent.reset();
	_graphBuilder.reset();
	CoUninitialize();
	ms_error("PushSourceCapture: Could stop graph !");
	ms_mutex_lock(&_mutex);
	flushq(&_rq,0);
	ms_mutex_unlock(&_mutex);
	_ready=false;
}

bool PushSourceCapture::isTimeToSend(uint64_t ticker_time){
	if (_frame_count==-1){
		_start_time=(float)ticker_time;
		_frame_count=0;
	}
	int cur_frame=(int)(((float)ticker_time-_start_time)*_fps/1000.0);
	if (cur_frame>_frame_count){
		_frame_count++;
		return true;
	}
	return false;
}

static void pushsource_preprocess(MSFilter * obj){
	PushSourceCapture *s=(PushSourceCapture*)obj->data;
	s->startDshowGraph();
}

static void pushsource_postprocess(MSFilter * obj){
	PushSourceCapture *s=(PushSourceCapture*)obj->data;
	s->stopAndClean();
}

static void pushsource_process(MSFilter * obj){
	PushSourceCapture *s=(PushSourceCapture*)obj->data;
	mblk_t *m;
	uint32_t timestamp;

	if (s->isTimeToSend(obj->ticker->time)){
		mblk_t *om=NULL;
		/*keep the most recent frame if several frames have been captured */
		while((m=s->readFrame())!=NULL){
			if (om!=NULL) freemsg(om);
			om=m;
		}
		if (om!=NULL){
			timestamp=(uint32_t)(obj->ticker->time*90);/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			ms_queue_put(obj->outputs[0],om);
		}
	}
}


static int pushsource_set_fps(MSFilter *f, void *arg){
	PushSourceCapture *s=(PushSourceCapture*)f->data;
	s->setFps(*(float*)arg);
	return 0;
}

static int pushsource_get_pix_fmt(MSFilter *f,void *arg){
	PushSourceCapture *s=(PushSourceCapture*)f->data;
	*((MSPixFmt*)arg)=s->getPixFmt();
	return 0;
}

static int pushsource_set_vsize(MSFilter *f, void *arg){
	PushSourceCapture *s=(PushSourceCapture*)f->data;
	s->setVSize(*((MSVideoSize*)arg));
	return 0;
}

static int pushsource_get_vsize(MSFilter *f, void *arg){
	PushSourceCapture *s=(PushSourceCapture*)f->data;
	MSVideoSize *vs=(MSVideoSize*)arg;
	*vs=s->getVSize();
	return 0;
}


static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	pushsource_set_fps	},
	{	MS_FILTER_GET_PIX_FMT	,	pushsource_get_pix_fmt	},
	{	MS_FILTER_SET_VIDEO_SIZE, pushsource_set_vsize	},
	{	MS_FILTER_GET_VIDEO_SIZE, pushsource_get_vsize	},
	{	0								,	NULL			}
};


MSFilterDesc ms_push_source_desc={
	MS_PUSHSOURCE_ID,
	"MSPushsource",
	N_("A webcam based on desktop pushsource."),
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	pushsource_init,
	pushsource_preprocess,
	pushsource_process,
	pushsource_postprocess,
	pushsource_uninit,
	methods
};


static void ms_push_source_detect(MSWebCamManager *obj);
static MSFilter * ms_push_source_create_reader(MSWebCam *obj){
	MSFilter *f=ms_filter_new_from_desc(&ms_push_source_desc);
	PushSourceCapture *s=(PushSourceCapture*)f->data;
	return f;
}


extern "C" MSWebCamDesc ms_push_source_cam_desc={
	"Push Source",
	&ms_push_source_detect,
	NULL,
	&ms_push_source_create_reader,
	NULL
};

static void ms_push_source_detect(MSWebCamManager *obj){
	MSWebCam *cam=ms_web_cam_new(&ms_push_source_cam_desc);
	cam->name=ms_strdup("Desktop Capture");
	ms_web_cam_manager_add_cam(obj,cam);
}

MS_FILTER_DESC_EXPORT(ms_push_source_desc)

