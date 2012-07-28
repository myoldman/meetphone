#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msvideo.h"

#include "layouts.h"

#define SCALE_FACTOR 4.0f
#define SELVIEW_POS_INACTIVE -100.0
#define LOCAL_BORDER_SIZE 2
#define MAX_CHILD_WINDOWS 12
#define BAR_HEIGHT 25

#include <Vfw.h>

typedef struct Yuv2RgbCtx{
	uint8_t *rgb;
	size_t rgblen;
	MSVideoSize dsize;
	MSVideoSize ssize;
	MSScalerContext *sws;
}Yuv2RgbCtx;

static void yuv2rgb_init(Yuv2RgbCtx *ctx){
	ctx->rgb=NULL;
	ctx->rgblen=0;
	ctx->dsize.width=0;
	ctx->dsize.height=0;
	ctx->ssize.width=0;
	ctx->ssize.height=0;
	ctx->sws=NULL;
}

static void yuv2rgb_uninit(Yuv2RgbCtx *ctx){
	if (ctx->rgb){
		ms_free(ctx->rgb);
		ctx->rgb=NULL;
		ctx->rgblen=0;
	}
	if (ctx->sws){
		ms_scaler_context_free(ctx->sws);
		ctx->sws=NULL;
	}
	ctx->dsize.width=0;
	ctx->dsize.height=0;
	ctx->ssize.width=0;
	ctx->ssize.height=0;
}

static void yuv2rgb_prepare(Yuv2RgbCtx *ctx, MSVideoSize src, MSVideoSize dst){
	if (ctx->sws!=NULL) yuv2rgb_uninit(ctx);
	ctx->sws=ms_scaler_create_context(src.width,src.height,MS_YUV420P,
			dst.width,dst.height, MS_RGB24_REV,
			MS_SCALER_METHOD_BILINEAR);
	ctx->dsize=dst;
	ctx->ssize=src;
	ctx->rgblen=dst.width*dst.height*3;
	ctx->rgb=(uint8_t*)ms_malloc0(ctx->rgblen+dst.width);
}


/*
 this function resizes the original pictures to the destination size and converts to rgb.
 It takes care of reallocating a new SwsContext and rgb buffer if the source/destination sizes have 
 changed.
*/
static bool_t yuv2rgb_process(Yuv2RgbCtx *ctx, MSPicture *src, MSVideoSize dstsize, bool_t mirroring){
	MSVideoSize srcsize;
	int size_change = FALSE;
	srcsize.width=src->w;
	srcsize.height=src->h;
	if (!ms_video_size_equal(dstsize,ctx->dsize) || !ms_video_size_equal(srcsize,ctx->ssize)){	
		ms_message("dst size is %dx%d",dstsize.width, dstsize.height);
		yuv2rgb_prepare(ctx,srcsize,dstsize);
		size_change = TRUE;
	}
	{
		int rgb_stride=-dstsize.width*3;
		uint8_t *p;

		p=ctx->rgb+(dstsize.width*3*(dstsize.height-1));
		if (ms_scaler_process(ctx->sws,src->planes,src->strides, &p, &rgb_stride)<0){
			ms_error("Error in 420->rgb ms_scaler_process().");
		}
		if (mirroring) rgb24_mirror(ctx->rgb,dstsize.width,dstsize.height,dstsize.width*3);
	}
	return size_change;
}

static void yuv2rgb_draw(Yuv2RgbCtx *ctx, HDRAWDIB ddh, HDC hdc, int dstx, int dsty){
	if (ctx->rgb){
		BITMAPINFOHEADER bi;
		memset(&bi,0,sizeof(bi));
		bi.biSize=sizeof(bi);
		bi.biWidth=ctx->dsize.width;
		bi.biHeight=ctx->dsize.height;
		bi.biPlanes=1;
		bi.biBitCount=24;
		bi.biCompression=BI_RGB;
		bi.biSizeImage=ctx->rgblen;

		DrawDibDraw(ddh,hdc,dstx,dsty,-1,-1,&bi,ctx->rgb,
			0,0,ctx->dsize.width,ctx->dsize.height,0);
	}
}

typedef struct _CDDisplay{
	HWND window;
	HWND child[MAX_CHILD_WINDOWS];
	Yuv2RgbCtx childview[MAX_CHILD_WINDOWS];
	HDRAWDIB ddh;
	MSVideoSize wsize; /*the initial requested window size*/
	MSVideoSize vsize[MAX_CHILD_WINDOWS]; /*the video size received for main input*/
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
	bool_t child_need_repaint[MAX_CHILD_WINDOWS];
}CDDisplay;

static LRESULT CALLBACK window_proc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{
	CDDisplay *wd=(CDDisplay*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
	RECT rect;
	GetClientRect(hwnd,&rect);
	switch(uMsg){
		case WM_DESTROY:
			if (wd){
				if(hwnd == wd->window)
					wd->window=NULL;
			}
		break;
		case WM_LBUTTONDBLCLK:
			ms_message("WM_LBUTTONDBLCLK ");
			if(IsZoomed(hwnd)) {
				ShowWindow( hwnd, SW_RESTORE);
			} else {
				ShowWindow( hwnd, SW_MAXIMIZE);
			}
			break;
		case WM_SIZE:
			if (wParam==SIZE_RESTORED){
				int h=(lParam>>16) & 0xffff;
				int w=lParam & 0xffff;
				
				ms_message("Resized to %i,%i",w,h);
				
				if (wd!=NULL){
					wd->need_repaint=TRUE;
					//wd->window_size.width=w;
					//wd->window_size.height=h;
				}else{
					ms_error("Could not retrieve CDDisplay from window !");
				}
				if (hwnd != wd->window) {
					int i = 0;
					for(i = 0; i < MAX_CHILD_WINDOWS; i++) {
						if(wd->child[i]!= hwnd) {
							ShowWindow( wd->child[i], SW_SHOW);
						}
						wd->child_need_repaint[i] = TRUE;
					}
				}
			}
			if (wParam==SIZE_MAXIMIZED && wd != NULL && hwnd != wd->window) {
//				ms_message("%d SIZE_MAXIMIZED", hwnd);
				int i = 0;
				for(i = 0; i < MAX_CHILD_WINDOWS; i++) {
					if(wd->child[i] == hwnd) {
						wd->child_need_repaint[i] = TRUE;
					}
				}
				for(i = 0; i < MAX_CHILD_WINDOWS; i++) {
					if(wd->child[i] != hwnd) {
						ShowWindow( wd->child[i], SW_HIDE);
					}
				}
			}
		break;
		case WM_PAINT:
			if (wd!=NULL){
				if(hwnd == wd->window) {
					wd->need_repaint=TRUE;
				} else {
					int i = 0;
					for(i = 0; i < MAX_CHILD_WINDOWS; i++) {
						if(wd->child[i] == hwnd) {
							wd->child_need_repaint[i] = TRUE;
						}
					}
				}
			}
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

static HWND create_window(int w, int h)
{
	WNDCLASS wc;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HWND hwnd;
	RECT rect;
	wc.style = CS_DBLCLKS ;
	wc.lpfnWndProc = window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = NULL;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = "Video Window";
	
	if(!RegisterClass(&wc))
	{
		/* already registred! */
	}
	rect.left=100;
	rect.top=100;
	rect.right=rect.left+w;
	rect.bottom=rect.top+h;
	if (!AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPCHILDREN /*WS_CAPTION WS_TILED|WS_BORDER*/,FALSE)){
		ms_error("AdjustWindowRect failed.");
	}
	ms_message("AdjustWindowRect: %li,%li %li,%li",rect.left,rect.top,rect.right,rect.bottom);
	hwnd=CreateWindow("Video Window", "Video window", 
		WS_OVERLAPPEDWINDOW /*WS_THICKFRAME*/ | WS_VISIBLE | WS_CLIPCHILDREN |WS_MAXIMIZE,
		CW_USEDEFAULT, CW_USEDEFAULT, rect.right-rect.left,rect.bottom-rect.top,
													NULL, NULL, hInstance, NULL);
	if (hwnd==NULL){
		ms_error("Fail to create video window");
	}
	
	return hwnd;
}

static void cd_display_init(MSFilter  *f){
	CDDisplay *obj=(CDDisplay*)ms_new0(CDDisplay,1);
	obj->wsize.width=MS_VIDEO_SIZE_CIF_W;
	obj->wsize.height=MS_VIDEO_SIZE_CIF_H;
	obj->vsize[0].width=MS_VIDEO_SIZE_CIF_W;
	obj->vsize[0].height=MS_VIDEO_SIZE_CIF_H;
	obj->lsize.width=MS_VIDEO_SIZE_CIF_W;
	obj->lsize.height=MS_VIDEO_SIZE_CIF_H;
	yuv2rgb_init(&obj->mainview);
	yuv2rgb_init(&obj->locview);
	obj->sv_corner=0; /* bottom right*/
	obj->sv_scalefactor=SCALE_FACTOR;
	obj->sv_posx=obj->sv_posy=SELVIEW_POS_INACTIVE;
	obj->background_color[0]=obj->background_color[1]=obj->background_color[2]=0;
	obj->need_repaint=FALSE;
	obj->autofit=TRUE;
	obj->mirroring=FALSE;
	obj->own_window=TRUE;
	f->data=obj;
	int i = 0;
	for(i = 0; i < MAX_CHILD_WINDOWS; i++) {
		yuv2rgb_init(&obj->childview[i]);
	}
}

static void cd_display_prepare(MSFilter *f){
	CDDisplay *cd=(CDDisplay*)f->data;
	if (cd->window==NULL){
		cd->window=create_window(cd->wsize.width,cd->wsize.height);
		SetWindowLong(cd->window,GWL_USERDATA,(long)cd);
	}
	if (cd->ddh==NULL)
		cd->ddh=DrawDibOpen();

	RECT rectfullscreen;
	rectfullscreen.left=0;
	rectfullscreen.top=0;
	rectfullscreen.right=0;
	rectfullscreen.bottom=0;
	GetClientRect(cd->window,&rectfullscreen);

	int denominator_width = (rectfullscreen.right - rectfullscreen.left)/ MS_VIDEO_SIZE_QVGA_W;
	int denominator_heigth = 3;
	int child_width = (rectfullscreen.right - rectfullscreen.left) / denominator_width;
	int child_height = (rectfullscreen.bottom - rectfullscreen.top ) / denominator_heigth;
	//int child_width = (rectfullscreen.right - rectfullscreen.left) / denominator_width;
	//int child_height = (rectfullscreen.bottom - rectfullscreen.top ) / denominator_heigth - BAR_HEIGHT;

	HINSTANCE hInstance = (HINSTANCE)GetWindowLong(cd->window, GWL_HINSTANCE);
	int i = 0;
	for(i = 0; i < MAX_CHILD_WINDOWS; i++) {
		int left_plus = i % denominator_width;
		int top_plus = i / (denominator_heigth + 1);
		RECT rect;
		//rect.left=rectfullscreen.left + (child_width * left_plus) + LOCAL_BORDER_SIZE * (left_plus + 1);
		//rect.top=rectfullscreen.top + (child_height * top_plus) + BAR_HEIGHT * (top_plus + 1); 
		rect.left=rectfullscreen.left + (child_width * left_plus) + LOCAL_BORDER_SIZE * (left_plus + 1);
		rect.top=rectfullscreen.top + (child_height * top_plus) + LOCAL_BORDER_SIZE * (top_plus + 1); 
		rect.right=rect.left+child_width;
		rect.bottom=rect.top+child_height;
		//ms_message("rec is %li %li %li %li", rect.left, rect.top, rect.right, rect.bottom);	
		if (!AdjustWindowRect(&rect,WS_CHILD | WS_VISIBLE |WS_OVERLAPPED | WS_BORDER /*WS_CAPTION WS_TILED|WS_BORDER*/,FALSE)){
		//if (!AdjustWindowRect(&rect,WS_CHILD | WS_VISIBLE |WS_OVERLAPPED | WS_CAPTION |WS_MAXIMIZEBOX /*WS_CAPTION WS_TILED|WS_BORDER*/,FALSE)){
			ms_error("AdjustWindowRect failed.");
		}
		const char* title = N_("Not Connect");
		if(i == 0) {
			title = N_("Local Input");
		}
		//ms_message("rec is %li %li %li %li", rect.left, rect.top, rect.right, rect.bottom);
		cd->child[i] = CreateWindow("Video Window", title,WS_CHILD | WS_VISIBLE |WS_OVERLAPPED | WS_BORDER , 
		rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,cd->window,NULL,hInstance,NULL);
		//cd->child[i] = CreateWindow("Video Window", title,WS_CHILD | WS_VISIBLE |WS_OVERLAPPED | WS_CAPTION | WS_MAXIMIZEBOX | WS_CLIPCHILDREN, 
		//rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,cd->window,NULL,hInstance,NULL);
		SetWindowLong(cd->child[i],GWL_USERDATA,(long)cd);	
	}
	
}

static void cd_display_unprepare(MSFilter *f){
	CDDisplay *cd=(CDDisplay*)f->data;
	if (cd->own_window && cd->window!=NULL){
		DestroyWindow(cd->window);
		cd->window=NULL;
	}
	if (cd->ddh!=NULL){
		DrawDibClose(cd->ddh);
		cd->ddh=NULL;
	}
}

static void cd_display_uninit(MSFilter *f){
	CDDisplay *obj=(CDDisplay*)f->data;
	cd_display_unprepare(f);
	yuv2rgb_uninit(&obj->mainview);
	yuv2rgb_uninit(&obj->locview);
	int i = 0;
	for(i = 0; i < MAX_CHILD_WINDOWS; i++) {
		yuv2rgb_uninit(&obj->childview[i]);
	}
	ms_free(obj);
}

static void cd_display_preprocess(MSFilter *f){
	cd_display_prepare(f);
}


/*
static void draw_local_view_frame(HDC hdc, MSVideoSize wsize, MSRect localrect){
	Rectangle(hdc, localrect.x-LOCAL_BORDER_SIZE, localrect.y-LOCAL_BORDER_SIZE,
		localrect.x+localrect.w+LOCAL_BORDER_SIZE, localrect.y+localrect.h+LOCAL_BORDER_SIZE);
}
*/

/*
* Draws a background, that is the black rectangles at top, bottom or left right sides of the video display.
* It is normally invoked only when a full redraw is needed (notified by Windows).
*/
static void draw_background(HDC hdc, MSVideoSize wsize, MSRect mainrect, int color[3]){
	HBRUSH brush;
	RECT brect;

	brush = CreateSolidBrush(RGB(color[0],color[1],color[2]));
	if (mainrect.x>0){	
		brect.left=0;
		brect.top=0;
		brect.right=mainrect.x;
		brect.bottom=wsize.height;
		FillRect(hdc, &brect, brush);
		brect.left=mainrect.x+mainrect.w;
		brect.top=0;
		brect.right=wsize.width;
		brect.bottom=wsize.height;
		FillRect(hdc, &brect, brush);
	}
	if (mainrect.y>0){
		brect.left=0;
		brect.top=0;
		brect.right=wsize.width;
		brect.bottom=mainrect.y;
		FillRect(hdc, &brect, brush);
		brect.left=0;
		brect.top=mainrect.y+mainrect.h;
		brect.right=wsize.width;
		brect.bottom=wsize.height;
		FillRect(hdc, &brect, brush);
	}
	if (mainrect.w==0 && mainrect.h==0){
		/*no image yet, black everything*/
		brect.left=brect.top=0;
		brect.right=wsize.width;
		brect.bottom=wsize.height;
		FillRect(hdc,&brect,brush);
	}
	DeleteObject(brush);
}

static void draw_window_background(HWND hwnd, int color[3]){
		HDC hdc;
		RECT temp_rect;
		MSRect mainrect;
		MSVideoSize temp_videosize;
		GetClientRect(hwnd,&temp_rect);
		mainrect.w = 0;
		mainrect.h = 0;
		temp_videosize.width = temp_rect.right - temp_rect.left;
		temp_videosize.height = temp_rect.bottom - temp_rect.top;
		hdc=GetDC(hwnd);
		ms_message("draw_window_background %d %dx%d ", (int)hwnd, temp_videosize.width, temp_videosize.height);
		draw_background(hdc,temp_videosize,mainrect, color);
		ReleaseDC(NULL,hdc);	
}

static void cd_display_process(MSFilter *f){
	CDDisplay *obj=(CDDisplay*)f->data;
	RECT rect;
	MSVideoSize wsize; /* the window size*/
	MSRect mainrect;
	MSRect localrect;
	MSPicture mainpic;
	//MSPicture localpic;
	mblk_t *main_im=NULL;
	//bool_t repainted=FALSE;
	int corner=obj->sv_corner;
	float scalefactor=obj->sv_scalefactor;

	if (obj->window==NULL){
		goto end;
	}

	if (GetClientRect(obj->window,&rect)==0
	    || rect.right<=32 || rect.bottom<=32) goto end;

	wsize.width=rect.right;
	wsize.height=rect.bottom;
	obj->wsize=wsize;
	
	/*get most recent message and draw it*/
	if (obj->need_repaint){
		draw_window_background(obj->window, obj->background_color);
		obj->need_repaint=FALSE;
	}

	int i = 0;
	for(i = 0; i < MAX_CHILD_WINDOWS; i ++) {
		if (obj->child_need_repaint[i]) {
			draw_window_background(obj->child[i], obj->background_color);
			obj->child_need_repaint[i] = FALSE;
		}
	}
		
	int y = 0;
	for(y = 0; y < 2; y ++) {
		if (f->inputs[y]!=NULL && (main_im=ms_queue_peek_last(f->inputs[y]))!=NULL) {
			if (ms_yuv_buf_init_from_mblk(&mainpic,main_im)==0){
				if (obj->autofit && (obj->vsize[y].width!=mainpic.w || obj->vsize[y].height!=mainpic.h)
					&& (mainpic.w>wsize.width || mainpic.h>wsize.height)){
//					RECT cur;
					ms_message("Detected video resolution changed, resizing window");
				}
				obj->vsize[y].width=mainpic.w;
				obj->vsize[y].height=mainpic.h;
			}
			if (main_im!=NULL) {
				MSVideoSize vsize;
				MSVideoSize wchildsize;
				HDC hdc3;
				hdc3=GetDC(obj->child[y]);
				RECT rect;
				if (GetClientRect(obj->child[y],&rect)==0
			    	|| rect.right<=32 || rect.bottom<=32) {
					continue;
				}

				wchildsize.width=rect.right - rect.left;
				wchildsize.height=rect.bottom - rect.top;
				
				ms_layout_compute(wchildsize,obj->vsize[y],obj->lsize,corner,scalefactor,&mainrect,&localrect);

				//picture size to convert
				vsize.width=mainrect.w;
				vsize.height=mainrect.h;	
				if (main_im!=NULL) {
					yuv2rgb_process(&obj->childview[y],&mainpic,vsize,obj->mirroring && !mblk_get_precious_flag(main_im) && y == 0);// only local view need mirror
				}
				
				yuv2rgb_draw(&obj->childview[y],obj->ddh,hdc3,mainrect.x,mainrect.y);
				ReleaseDC(NULL,hdc3);
			}
		}
	}

	end:
		
	if (f->inputs[0]!=NULL)
		ms_queue_flush(f->inputs[0]);
	if (f->inputs[1]!=NULL)
		ms_queue_flush(f->inputs[1]);
}

static int get_native_window_id(MSFilter *f, void *data){
	CDDisplay *obj=(CDDisplay*)f->data;
	*(long*)data=(long)obj->window;
	return 0;
}

static int set_native_window_id(MSFilter *f, void *data){
	CDDisplay *obj=(CDDisplay*)f->data;
	obj->window=(HWND)(*(long*)data);
	obj->own_window=FALSE;
	return 0;
}

static int enable_autofit(MSFilter *f, void *data){
	CDDisplay *obj=(CDDisplay*)f->data;
	obj->autofit=*(int*)data;
	return 0;
}

static int enable_mirroring(MSFilter *f, void *data){
	CDDisplay *obj=(CDDisplay*)f->data;
	obj->mirroring=*(int*)data;
	return 0;
}

static int set_corner(MSFilter *f, void *data){
	CDDisplay *obj=(CDDisplay*)f->data;
	obj->sv_corner=*(int*)data;
	obj->need_repaint=TRUE;
	return 0;
}

static int get_vsize(MSFilter *f, void *data){
	CDDisplay *obj=(CDDisplay*)f->data;
	*(MSVideoSize*)data=obj->wsize;
	return 0;
}

static int set_vsize(MSFilter *f, void *data){
	CDDisplay *obj=(CDDisplay*)f->data;
	obj->wsize=*(MSVideoSize*)data;
	return 0;
}

static int set_scalefactor(MSFilter *f,void *arg){
	CDDisplay *obj=(CDDisplay*)f->data;
	ms_filter_lock(f);
	obj->sv_scalefactor = *(float*)arg;
	if (obj->sv_scalefactor<0.5f)
		obj->sv_scalefactor = 0.5f;
	ms_filter_unlock(f);
	return 0;
}

#if 0
static int set_selfview_pos(MSFilter *f,void *arg){
	CDDisplay *s=(CDDisplay*)f->data;
	s->sv_posx=((float*)arg)[0];
	s->sv_posy=((float*)arg)[1];
	s->sv_scalefactor=(float)100.0/((float*)arg)[2];
	return 0;
}

static int get_selfview_pos(MSFilter *f,void *arg){
	CDDisplay *s=(CDDisplay*)f->data;
	((float*)arg)[0]=s->sv_posx;
	((float*)arg)[1]=s->sv_posy;
	((float*)arg)[2]=(float)100.0/s->sv_scalefactor;
	return 0;
}
#endif

static int set_background_color(MSFilter *f,void *arg){
	CDDisplay *s=(CDDisplay*)f->data;
	s->background_color[0]=((int*)arg)[0];
	s->background_color[1]=((int*)arg)[1];
	s->background_color[2]=((int*)arg)[2];
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_GET_VIDEO_SIZE			, get_vsize	},
	{	MS_FILTER_SET_VIDEO_SIZE			, set_vsize	},
	{	MS_VIDEO_DISPLAY_GET_NATIVE_WINDOW_ID, get_native_window_id },
	{	MS_VIDEO_DISPLAY_SET_NATIVE_WINDOW_ID, set_native_window_id },
	{	MS_VIDEO_DISPLAY_ENABLE_AUTOFIT		,	enable_autofit	},
	{	MS_VIDEO_DISPLAY_ENABLE_MIRRORING	,	enable_mirroring},
	{	MS_VIDEO_DISPLAY_SET_LOCAL_VIEW_MODE	, set_corner },
	{	MS_VIDEO_DISPLAY_SET_LOCAL_VIEW_SCALEFACTOR	, set_scalefactor },
	{	MS_VIDEO_DISPLAY_SET_BACKGROUND_COLOR    ,  set_background_color},
	{	0	,NULL}
};


#ifdef _MSC_VER

MSFilterDesc ms_cd_display_desc={
	MS_CASCADE_DISPLAY_ID,
	"MSCascaseDisplay",
	N_("A video display based on windows DrawDib api"),
	MS_FILTER_OTHER,
	NULL,
	2,
	0,
	cd_display_init,
	cd_display_preprocess,
	cd_display_process,
	NULL,
	cd_display_uninit,
	methods
};

#else

MSFilterDesc ms_cd_display_desc={
	.id=MS_CASCADE_DISPLAY_ID,
	.name="MSCascaseDisplay",
	.text=N_("A video display based on windows DrawDib api"),
	.category=MS_FILTER_OTHER,
	.ninputs=2,
	.noutputs=0,
	.init=cd_display_init,
	.preprocess=cd_display_preprocess,
	.process=cd_display_process,
	.uninit=cd_display_uninit,
	.methods=methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_cd_display_desc)

