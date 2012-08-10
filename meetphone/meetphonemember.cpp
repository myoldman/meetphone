// meetphonemember.cpp : 实现文件
//

#include "stdafx.h"
#include "meetphone.h"
#include "meetphonemember.h"
#include "Vfw.h"

typedef struct Yuv2RgbCtx{
	uint8_t *rgb;
	size_t rgblen;
	MSVideoSize dsize;
	MSVideoSize ssize;
	MSScalerContext *sws;
}Yuv2RgbCtx;

#define UNSIGNIFICANT_VOLUME (-26)
#define SMOOTH 0.15f

typedef struct _DDDisplay{
	HWND window;
	HDRAWDIB ddh;
	MSVideoSize wsize; /*the initial requested window size*/
	MSVideoSize vsize; /*the video size received for main input*/
	MSVideoSize lsize; /*the video size received for local display */
	Yuv2RgbCtx mainview;
	Yuv2RgbCtx locview;
	int sv_corner;
	float sv_scalefactor;
	float sv_posx,sv_posy;
	int background_color[3];
	bool_t need_repaint;
	bool_t autofit;
	bool_t mirroring;
	bool_t own_window;
	char window_title[64];
	uint8_t *volume_png;
	int volume_png_size;
	int volume_png_width;
	int volume_png_height;
	short volume_png_bitcount;
	float frac;
	float last_frac;
}DDDisplay;

// Cmeetphonemember 对话框

IMPLEMENT_DYNAMIC(Cmeetphonemember, CDialog)

Cmeetphonemember::Cmeetphonemember(CWnd* pParent /*=NULL*/)
: CDialog(Cmeetphonemember::IDD, pParent)
{
}

Cmeetphonemember::~Cmeetphonemember()
{
}

void Cmeetphonemember::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_MEMBER_NAME, m_hMemberName);
	DDX_Control(pDX, IDC_LOADING, m_hLoading);
}


BEGIN_MESSAGE_MAP(Cmeetphonemember, CDialog)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_MESSAGE(WM_MEMBER_STOP_LOADING,OnMemberStopLoadingMsg)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// Cmeetphonemember 消息处理程序

BOOL Cmeetphonemember::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (m_hLoading.Load(_T("res/loading.gif")))
		m_hLoading.Draw();
	m_hMemberName.SetWindowText(m_sMemberName);
	SetTimer(1,50,NULL); //设置定时器
	return TRUE;
}

void Cmeetphonemember::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if(IsZoomed()){
		ShowWindow(SW_HIDE);
		m_pParentWnd->PostMessage(WM_MEMBER_RESTORE, (WPARAM)this);
		//Delay restore when parent recv this message to prevent black blank
	} else {
		ShowWindow(SW_HIDE);
		m_pParentWnd->PostMessage(WM_MEMBER_MAXIMIZE, (WPARAM)this);
		ShowWindow(SW_MAXIMIZE);
	}

	CDialog::OnLButtonDblClk(nFlags, point);
}

void Cmeetphonemember::OnSize(UINT nType, int cx, int cy)
{
	if (nType==SIZE_RESTORED || nType==SIZE_MAXIMIZED){
		DDDisplay *wd=(DDDisplay*)GetWindowLongPtr(m_hWnd,GWLP_USERDATA);
		ms_message("Resized to %i,%i",cx,cy);
		if (wd!=NULL){
			wd->need_repaint=TRUE;
		}else{
			ms_error("Could not retrieve CDDisplay from window !");
		}
	}
	CDialog::OnSize(nType, cx, cy);
}

void Cmeetphonemember::OnPaint()
{
	CPaintDC dc(this);
	DDDisplay *wd=(DDDisplay*)GetWindowLongPtr(m_hWnd,GWLP_USERDATA);
	if (wd!=NULL){
		wd->autofit = FALSE;
		CW2A memberName(m_sMemberName);
		strcpy(wd->window_title, memberName);
		if(wd->volume_png == NULL) {
			CBitmap bmp;
			BITMAP bm;
			bmp.LoadBitmap(IDB_VOLUME);
			bmp.GetBitmap( &bm );

			int nbyte = bm.bmBitsPixel / 8;  
			BITMAPINFO bi;    
			bi.bmiHeader.biSize = sizeof(bi.bmiHeader);    
			bi.bmiHeader.biWidth = bm.bmWidth;    
			bi.bmiHeader.biHeight = -bm.bmHeight;    
			bi.bmiHeader.biPlanes = 1;    
			bi.bmiHeader.biBitCount = bm.bmBitsPixel;     
			bi.bmiHeader.biCompression = BI_RGB;     
			bi.bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * nbyte;   
			bi.bmiHeader.biClrUsed = 0;    
			bi.bmiHeader.biClrImportant = 0;
			HDC hdc = ::GetDC(NULL);    
			BYTE* pBits = (BYTE*)new BYTE[bm.bmWidth * bm.bmHeight * nbyte];
			::ZeroMemory(pBits, bm.bmWidth * bm.bmHeight * nbyte);
			if (!::GetDIBits(hdc, bmp, 0, bm.bmHeight, pBits, &bi, DIB_RGB_COLORS))    
			{ 
			}
			wd->volume_png = pBits;
			wd->volume_png_height = bm.bmHeight;
			wd->volume_png_width = bm.bmWidth;
			wd->volume_png_size = bi.bmiHeader.biSizeImage;
			wd->volume_png_bitcount = bm.bmBitsPixel;
			bmp.DeleteObject();
		}
		wd->need_repaint=TRUE;
	}
}

LONG Cmeetphonemember::OnMemberStopLoadingMsg(WPARAM wP,LPARAM lP)
{
	m_hLoading.ShowWindow(SW_HIDE);
	return 0;
}
void Cmeetphonemember::OnTimer(UINT_PTR nIDEvent)
{
	CDialog::OnTimer(nIDEvent);
	DDDisplay *wd=(DDDisplay*)GetWindowLongPtr(m_hWnd,GWLP_USERDATA);
	if (wd!=NULL){
		LinphoneCore *lc = theApp.GetCore();
		LinphoneCall *call = linphone_core_get_current_call(lc);
		if(call == NULL)
			return;
		float volume_db = 0.0f;
		if(m_sMemberName == _T("本地视频"))
		{
			volume_db = linphone_call_get_record_volume(call);
		}
		else
		{
			volume_db = linphone_call_get_play_volume(call);
		}
		float frac=(volume_db-UNSIGNIFICANT_VOLUME)/(float)(-UNSIGNIFICANT_VOLUME+3.0);
		if (frac<0) frac=0;
		if (frac>1.0) frac=1.0;
		if (frac<wd->last_frac){
			frac=(frac*SMOOTH)+(wd->last_frac*(1-SMOOTH));
		}
		wd->last_frac=wd->frac;
		wd->frac=frac;
	}
}
