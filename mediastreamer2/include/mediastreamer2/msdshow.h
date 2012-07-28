#ifndef msdshow_h
#define msdshow_h
#include <mediastreamer2/mscommon.h>

#include <initguid.h>
#include <ocidl.h>

#if !defined(INITGUID) || (defined(INITGUID) && defined(__cplusplus))
#define GUID_EXT EXTERN_C 
#else
#define GUID_EXT
#endif

GUID_EXT const GUID CLSID_VideoInputDeviceCategory;
GUID_EXT const GUID CLSID_SystemDeviceEnum;
GUID_EXT const GUID CLSID_FilterGraph;
GUID_EXT const GUID CLSID_SampleGrabber;
GUID_EXT const GUID CLSID_NullRenderer;
GUID_EXT const GUID CLSID_VfwCapture;
GUID_EXT const GUID IID_IGraphBuilder;
GUID_EXT const GUID IID_IBaseFilter;
GUID_EXT const GUID IID_ICreateDevEnum;
GUID_EXT const GUID IID_IEnumFilters;
GUID_EXT const GUID IID_IEnumPins;
GUID_EXT const GUID IID_IMediaSample;
GUID_EXT const GUID IID_IMediaFilter;
GUID_EXT const GUID IID_IPin;
GUID_EXT const GUID IID_IMediaSample;
GUID_EXT const GUID IID_IMediaFilter;
GUID_EXT const GUID IID_IPin;
GUID_EXT const GUID IID_ISampleGrabber;
GUID_EXT const GUID IID_ISampleGrabberCB;
GUID_EXT const GUID IID_IMediaEvent;
GUID_EXT const GUID IID_IMediaControl;
GUID_EXT const GUID IID_IMemInputPin;
GUID_EXT const GUID IID_IAMStreamConfig;
GUID_EXT const GUID IID_IVideoProcAmp;
GUID_EXT const GUID MEDIATYPE_Video;
GUID_EXT const GUID MEDIASUBTYPE_I420;
GUID_EXT const GUID MEDIASUBTYPE_YV12;
GUID_EXT const GUID MEDIASUBTYPE_IYUV;
GUID_EXT const GUID MEDIASUBTYPE_YUYV;
GUID_EXT const GUID MEDIASUBTYPE_YUY2;
GUID_EXT const GUID MEDIASUBTYPE_UYVY;
GUID_EXT const GUID MEDIASUBTYPE_RGB24;
GUID_EXT const GUID CLSID_PushSourceDesktop;

#undef CINTERFACE
#undef INTERFACE

template <typename _ComT>
class ComPtr{
	private:
		_ComT *mPtr;
	public:
		int coCreateInstance(REFCLSID clsid, REFIID iid){
			HRESULT res;
			res=::CoCreateInstance(clsid,NULL,CLSCTX_INPROC,iid,
				(void**)&mPtr);
			return (res==S_OK) ? 0 : -1;
		}
		ComPtr() : mPtr(0){
		}
		~ComPtr(){
			reset();
		}
		ComPtr(const ComPtr<_ComT>& other) : mPtr(other.mPtr){
			if (mPtr) mPtr->AddRef();
		}
		ComPtr<_ComT> & operator=(const ComPtr<_ComT> &other){
			if (other.mPtr)
				other.mPtr->AddRef();
			if (mPtr)
				mPtr->Release();
			mPtr=other.mPtr;
			return *this;
		}
		bool operator==(const ComPtr<_ComT> &other){
			return other.mPtr==mPtr;
		}
		bool operator!=(const ComPtr<_ComT> &other){
			return other.mPtr!=mPtr;
		}
		_ComT *get(){
			return mPtr;
		}
		_ComT **operator&(){
			return &mPtr;
		}
		_ComT * operator->(){
			return mPtr;
		}
		void reset(){
			if (mPtr){
				mPtr->Release();
				mPtr=0;
			}
		}
};


typedef LONGLONG REFERENCE_TIME;

#if 0
    IEnumPins : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next(
            /* [in] */ ULONG cPins,
            /* [size_is][out] */
            __out_ecount_part(cPins, *pcFetched)  IPin **ppPins,
            /* [out] */
            __out_opt  ULONG *pcFetched) = 0;

        virtual HRESULT STDMETHODCALLTYPE Skip(
            /* [in] */ ULONG cPins) = 0;

        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE Clone(
            /* [out] */
            __out  IEnumPins **ppEnum) = 0;

    };
#endif

#ifndef DECLARE_ENUMERATOR_
#define DECLARE_ENUMERATOR_(I,T) \
    DECLARE_INTERFACE_(I,IUnknown) \
    { \
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE; \
        STDMETHOD_(ULONG,AddRef)(THIS) PURE; \
        STDMETHOD_(ULONG,Release)(THIS) PURE; \
		STDMETHOD(Next)(THIS_ ULONG,T*,ULONG*) PURE;\
        STDMETHOD(Skip)(THIS_ ULONG) PURE; \
        STDMETHOD(Reset)(THIS) PURE; \
        STDMETHOD(Clone)(THIS_ I**) PURE; \
    }
#endif


typedef struct tagVIDEOINFOHEADER {
  RECT rcSource;
  RECT rcTarget;
  DWORD dwBitRate;
  DWORD dwBitErrorRate;
  REFERENCE_TIME AvgTimePerFrame;
  BITMAPINFOHEADER bmiHeader;
} VIDEOINFOHEADER;

typedef struct _AMMediaType {
  GUID majortype;
  GUID subtype;
  BOOL bFixedSizeSamples;
  BOOL bTemporalCompression;
  ULONG lSampleSize;
  GUID formattype;
  IUnknown *pUnk;
  ULONG cbFormat;
  BYTE *pbFormat;
} AM_MEDIA_TYPE;

DECLARE_ENUMERATOR_(IEnumMediaTypes,AM_MEDIA_TYPE*);

typedef struct _VIDEO_STREAM_CONFIG_CAPS
{
  GUID guid;
  ULONG VideoStandard;
  SIZE InputSize;
  SIZE MinCroppingSize;
  SIZE MaxCroppingSize;
  int CropGranularityX;
  int CropGranularityY;
  int CropAlignX;
  int CropAlignY;
  SIZE MinOutputSize;
  SIZE MaxOutputSize;
  int OutputGranularityX;
  int OutputGranularityY;
  int StretchTapsX;
  int StretchTapsY;
  int ShrinkTapsX;
  int ShrinkTapsY;
  LONGLONG MinFrameInterval;
  LONGLONG MaxFrameInterval;
  LONG MinBitsPerSecond;
  LONG MaxBitsPerSecond;
} VIDEO_STREAM_CONFIG_CAPS;

typedef LONGLONG REFERENCE_TIME;

typedef interface IBaseFilter IBaseFilter;
typedef interface IReferenceClock IReferenceClock;
typedef interface IFilterGraph IFilterGraph;

typedef enum _FilterState {
  State_Stopped,
  State_Paused,
  State_Running
} FILTER_STATE;

#define MAX_FILTER_NAME 128
typedef struct _FilterInfo {
  WCHAR achName[MAX_FILTER_NAME]; 
  IFilterGraph *pGraph;
} FILTER_INFO;

typedef enum _PinDirection {
  PINDIR_INPUT,
  PINDIR_OUTPUT
} PIN_DIRECTION;

#define MAX_PIN_NAME 128
typedef struct _PinInfo {
  IBaseFilter *pFilter;
  PIN_DIRECTION dir;
  WCHAR achName[MAX_PIN_NAME];
} PIN_INFO;

#define INTERFACE IPin
DECLARE_INTERFACE_(IPin,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Connect)(THIS_ IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(ReceiveConnection)(THIS_ IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(Disconnect)(THIS) PURE;
  STDMETHOD(ConnectedTo)(THIS_ IPin**) PURE;
  STDMETHOD(ConnectionMediaType)(THIS_ AM_MEDIA_TYPE*) PURE;
  STDMETHOD(QueryPinInfo)(THIS_ PIN_INFO*) PURE;
  STDMETHOD(QueryDirection)(THIS_ PIN_DIRECTION*) PURE;
};
#undef INTERFACE

DECLARE_ENUMERATOR_(IEnumPins,IPin*);

typedef struct _AllocatorProperties {
  long cBuffers;
  long cbBuffer;
  long cbAlign;
  long cbPrefix;
} ALLOCATOR_PROPERTIES;

typedef LONG_PTR OAEVENT;

#define INTERFACE IMediaEvent
DECLARE_INTERFACE_(IMediaEvent,IDispatch)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(GetEventHandle)(THIS_ OAEVENT*) PURE;
  STDMETHOD(GetEvent)(THIS_ long*,LONG_PTR,LONG_PTR,long) PURE;
  STDMETHOD(WaitForCompletion)(THIS_ long,long*) PURE;
  STDMETHOD(CancelDefaultHandling)(THIS_ long) PURE;
  STDMETHOD(RestoreDefaultHandling)(THIS_ long) PURE;
  STDMETHOD(FreeEventParams)(THIS_ long,LONG_PTR,LONG_PTR) PURE;
};
#undef INTERFACE

typedef long OAFilterState;

#define INTERFACE IMediaControl
DECLARE_INTERFACE_(IMediaControl,IDispatch)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Run)(THIS) PURE;
  STDMETHOD(Pause)(THIS) PURE;
  STDMETHOD(Stop)(THIS) PURE;
  STDMETHOD(GetState)(THIS_ LONG,OAFilterState*) PURE;
  STDMETHOD(RenderFile)(THIS_ BSTR) PURE;
  STDMETHOD(AddSourceFilter)(THIS_ BSTR,IDispatch**) PURE;
  STDMETHOD(get_FilterCollection)(THIS_ IDispatch**) PURE;
  STDMETHOD(get_RegFilterCollection)(THIS_ IDispatch**) PURE;
  STDMETHOD(StopWhenReady)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IVideoProcAmp
DECLARE_INTERFACE_(IVideoProcAmp,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IAMStreamConfig
DECLARE_INTERFACE_(IAMStreamConfig,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SetFormat)(THIS_ AM_MEDIA_TYPE*) PURE;
  STDMETHOD(GetFormat)(THIS_ AM_MEDIA_TYPE**) PURE;
  STDMETHOD(GetNumberOfCapabilities)(THIS_ int*,int*) PURE;
  STDMETHOD(GetStreamCaps)(THIS_ int,AM_MEDIA_TYPE**,BYTE*) PURE;
};
#undef INTERFACE

#define INTERFACE IMediaFilter
DECLARE_INTERFACE_(IMediaFilter,IPersist)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Stop)(THIS) PURE;
  STDMETHOD(Pause)(THIS) PURE;
  STDMETHOD(Run)(THIS_ REFERENCE_TIME) PURE;
  STDMETHOD(GetState)(THIS_ DWORD,FILTER_STATE*) PURE;
  STDMETHOD(SetSyncSource)(THIS_ IReferenceClock*) PURE;
  STDMETHOD(GetSyncSource)(THIS_ IReferenceClock**) PURE;
};
#undef INTERFACE

#define INTERFACE IBaseFilter
DECLARE_INTERFACE_(IBaseFilter,IMediaFilter)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(EnumPins)(THIS_ IEnumPins**) PURE;
  STDMETHOD(FindPin)(THIS_ LPCWSTR,IPin**) PURE;
  STDMETHOD(QueryFilterInfo)(THIS_ FILTER_INFO*) PURE;
  STDMETHOD(JoinFilterGraph)(THIS_ IFilterGraph*,LPCWSTR) PURE;
  STDMETHOD(QueryVendorInfo)(THIS_ LPWSTR*) PURE;
};
#undef INTERFACE

DECLARE_ENUMERATOR_(IEnumFilters,IBaseFilter*);

#define INTERFACE IFilterGraph
DECLARE_INTERFACE_(IFilterGraph,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(AddFilter)(THIS_ IBaseFilter*,LPCWSTR) PURE;
  STDMETHOD(RemoveFilter)(THIS_ IBaseFilter*) PURE;
  STDMETHOD(EnumFilters)(THIS_ IEnumFilters**) PURE;
  STDMETHOD(FindFilterByName)(THIS_ LPCWSTR,IBaseFilter**) PURE;
  STDMETHOD(ConnectDirect)(THIS_ IPin*,IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(Reconnect)(THIS_ IPin*) PURE;
  STDMETHOD(Disconnect)(THIS_ IPin*) PURE;
  STDMETHOD(SetDefaultSyncSource)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IGraphBuilder
DECLARE_INTERFACE_(IGraphBuilder,IFilterGraph)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Connect)(THIS_ IPin*,IPin*) PURE;
  STDMETHOD(Render)(THIS_ IPin*) PURE;
  STDMETHOD(RenderFile)(THIS_ LPCWSTR,LPCWSTR) PURE;
  STDMETHOD(AddSourceFilter)(THIS_ LPCWSTR,LPCWSTR,IBaseFilter**) PURE;
  STDMETHOD(SetLogFile)(THIS_ DWORD_PTR) PURE;
  STDMETHOD(Abort)(THIS) PURE;
  STDMETHOD(ShouldOperationContinue)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE ICreateDevEnum
DECLARE_INTERFACE_(ICreateDevEnum,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(CreateClassEnumerator)(THIS_ REFIID,IEnumMoniker**,DWORD) PURE;
};
#undef INTERFACE

#define INTERFACE IMediaSample
DECLARE_INTERFACE_(IMediaSample,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(GetPointer)(THIS_ BYTE **) PURE;
  STDMETHOD_(long, GetSize)(THIS) PURE;
};

#undef INTERFACE

#define INTERFACE IMemAllocator
DECLARE_INTERFACE_(IMemAllocator,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SetProperties)(THIS_ ALLOCATOR_PROPERTIES*,ALLOCATOR_PROPERTIES*) PURE;
  STDMETHOD(GetProperties)(THIS_ ALLOCATOR_PROPERTIES*) PURE;
  STDMETHOD(Commit)(THIS) PURE;
  STDMETHOD(Decommit)(THIS) PURE;
  STDMETHOD(GetBuffer)(THIS_ IMediaSample **,REFERENCE_TIME*,REFERENCE_TIME*,DWORD) PURE;
  STDMETHOD(ReleaseBuffer)(THIS_ IMediaSample*) PURE;
};
#undef INTERFACE

#define INTERFACE IMemInputPin
DECLARE_INTERFACE_(IMemInputPin,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(GetAllocator)(THIS_ IMemAllocator**) PURE;
  STDMETHOD(NotifyAllocator)(THIS_ IMemAllocator*,BOOL) PURE;
  STDMETHOD(GetAllocatorRequirements)(THIS_ ALLOCATOR_PROPERTIES*) PURE;
  STDMETHOD(Receive)(THIS_ IMediaSample*) PURE;
  STDMETHOD(ReceiveMultiple)(THIS_ IMediaSample**,LONG,LONG*) PURE;
  STDMETHOD(ReceiveCanBlock)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE ISampleGrabberCB
DECLARE_INTERFACE_(ISampleGrabberCB,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SampleCB)(THIS_ double,IMediaSample*) PURE;
  STDMETHOD(BufferCB)(THIS_ double,BYTE*,long) PURE;
};
#undef INTERFACE

#define INTERFACE ISampleGrabber
DECLARE_INTERFACE_(ISampleGrabber,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SetOneShot)(THIS_ BOOL) PURE;
  STDMETHOD(SetMediaType)(THIS_ const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(GetConnectedMediaType)(THIS_ AM_MEDIA_TYPE*) PURE;
  STDMETHOD(SetBufferSamples)(THIS_ BOOL) PURE;
  STDMETHOD(GetCurrentBuffer)(THIS_ long*,long*) PURE;
  STDMETHOD(GetCurrentSample)(THIS_ IMediaSample**) PURE;
  STDMETHOD(SetCallBack)(THIS_ ISampleGrabberCB *,long) PURE;
};
#undef INTERFACE


#endif


