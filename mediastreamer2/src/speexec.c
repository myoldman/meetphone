/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2010  Belledonne Communications SARL 
Author: Simon Morlat <simon.morlat@linphone.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#if defined(HAVE_CONFIG_H)
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>
#include "ortp/b64.h"

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#ifdef WIN32
#include <malloc.h> /* for alloca */
#endif

typedef struct _AudioFlowController{
	int target_samples;
	int total_samples;
	int current_pos;
	int current_dropped;
}AudioFlowController;

void audio_flow_controller_init(AudioFlowController *ctl){
	ctl->target_samples=0;
	ctl->total_samples=0;
	ctl->current_pos=0;
	ctl->current_dropped=0;
}

void audio_flow_controller_set_target(AudioFlowController *ctl, int samples_to_drop, int total_samples){
	ctl->target_samples=samples_to_drop;
	ctl->total_samples=total_samples;
	ctl->current_pos=0;
	ctl->current_dropped=0;
}

static void discard_well_choosed_samples(mblk_t *m, int nsamples, int todrop){
	int i;
	int16_t *samples=(int16_t*)m->b_rptr;
	int min_diff=32768;
	int pos=0;

	
#ifdef TWO_SAMPLES_CRITERIA
	for(i=0;i<nsamples-1;++i){
		int tmp=abs((int)samples[i]- (int)samples[i+1]);
#else
	for(i=0;i<nsamples-2;++i){
		int tmp=abs((int)samples[i]- (int)samples[i+1])+abs((int)samples[i+1]- (int)samples[i+2]);
#endif
		if (tmp<=min_diff){
			pos=i;
			min_diff=tmp;
		}
	}
	/*ms_message("min_diff=%i at pos %i",min_diff, pos);*/
#ifdef TWO_SAMPLES_CRITERIA
	memmove(samples+pos,samples+pos+1,(nsamples-pos-1)*2);
#else
	memmove(samples+pos+1,samples+pos+2,(nsamples-pos-2)*2);
#endif
	
	todrop--;
	m->b_wptr-=2;
	nsamples--;
	if (todrop>0){
		/*repeat the same process again*/
		discard_well_choosed_samples(m,nsamples,todrop);
	}
}

mblk_t * audio_flow_controller_process(AudioFlowController *ctl, mblk_t *m){
	if (ctl->total_samples>0 && ctl->target_samples>0){
		int nsamples=(m->b_wptr-m->b_rptr)/2;
		if (ctl->target_samples*16>ctl->total_samples){
			ms_warning("Too many samples to drop, dropping entire frames");
			m->b_wptr=m->b_rptr;
			ctl->current_pos+=nsamples;
		}else{
			int th_dropped;
			int todrop;
	
			ctl->current_pos+=nsamples;
			th_dropped=(ctl->target_samples*ctl->current_pos)/ctl->total_samples;
			todrop=th_dropped-ctl->current_dropped;
			if (todrop>0){
				if (todrop>nsamples) todrop=nsamples;
				discard_well_choosed_samples(m,nsamples,todrop);
				/*ms_message("th_dropped=%i, current_dropped=%i, %i samples dropped.",th_dropped,ctl->current_dropped,todrop);*/
				ctl->current_dropped+=todrop;
			}
		}
		if (ctl->current_pos>=ctl->total_samples) ctl->target_samples=0;/*stop discarding*/
	}
	return m;
}


//#define EC_DUMP 1
#ifdef ANDROID
#define EC_DUMP_PREFIX "/sdcard"
#else
#define EC_DUMP_PREFIX "/dynamic/tests"
#endif

static const float smooth_factor=0.05f;
static const int framesize=64;
static const int flow_control_interval_ms=5000;


typedef struct SpeexECState{
	SpeexEchoState *ecstate;
	SpeexPreprocessState *den;
	MSBufferizer delayed_ref;
	MSBufferizer ref;
	MSBufferizer echo;
	int framesize;
	int filterlength;
	int samplerate;
	int delay_ms;
	int tail_length_ms;
	int nominal_ref_samples;
	int min_ref_samples;
	AudioFlowController afc;
	char *state_str;
#ifdef EC_DUMP
	FILE *echofile;
	FILE *reffile;
	FILE *cleanfile;
#endif
	bool_t echostarted;
	bool_t bypass_mode;
	bool_t using_zeroes;
}SpeexECState;

static void speex_ec_init(MSFilter *f){
	SpeexECState *s=(SpeexECState *)ms_new(SpeexECState,1);

	s->samplerate=8000;
	ms_bufferizer_init(&s->delayed_ref);
	ms_bufferizer_init(&s->echo);
	ms_bufferizer_init(&s->ref);
	s->delay_ms=0;
	s->tail_length_ms=250;
	s->ecstate=NULL;
	s->framesize=framesize;
	s->den = NULL;
	s->state_str=NULL;
	s->using_zeroes=FALSE;
	s->echostarted=FALSE;
	s->bypass_mode=FALSE;

#ifdef EC_DUMP
	{
		char *fname=ms_strdup_printf("%s/msspeexec-%p-echo.raw", EC_DUMP_PREFIX,f);
		s->echofile=fopen(fname,"w");
		ms_free(fname);
		fname=ms_strdup_printf("%s/msspeexec-%p-ref.raw", EC_DUMP_PREFIX,f);
		s->reffile=fopen(fname,"w");
		ms_free(fname);
		fname=ms_strdup_printf("%s/msspeexec-%p-clean.raw", EC_DUMP_PREFIX,f);
		s->cleanfile=fopen(fname,"w");
		ms_free(fname);
	}
#endif
	
	f->data=s;
}

static void speex_ec_uninit(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	if (s->state_str) ms_free(s->state_str);
	ms_bufferizer_uninit(&s->delayed_ref);
#ifdef EC_DUMP
	if (s->echofile)
		fclose(s->echofile);
	if (s->reffile)
		fclose(s->reffile);
#endif
	ms_free(s);
}

#ifdef SPEEX_ECHO_GET_BLOB

static void apply_config(SpeexECState *s){
	if (s->state_str!=NULL){
		size_t buflen=strlen(s->state_str);
		uint8_t *buffer=alloca(buflen);
		SpeexEchoStateBlob *blob;
		if ((buflen=b64_decode(s->state_str,strlen(s->state_str),buffer,buflen))<=0){
			ms_error("Could not decode base64 %s",s->state_str);
			return;
		}
		blob=speex_echo_state_blob_new_from_memory(buffer,buflen);
		if (blob==NULL){
			ms_error("Could not create blob from config string");
			return;
		}
		if (speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_BLOB, blob)!=0){
			ms_error("Could not apply speex echo blob !");
		}
		speex_echo_state_blob_free(blob);
		ms_message("speex echo state restored.");
	}	
}

static void fetch_config(SpeexECState *s){
	SpeexEchoStateBlob *blob=NULL;
	char *txt;
	size_t txt_len;

	if (s->ecstate==NULL) return;
	
	if (speex_echo_ctl(s->ecstate, SPEEX_ECHO_GET_BLOB, &blob)!=0){
		ms_error("Could not retrieve speex echo blob !");
		return;
	}
	txt_len=(speex_echo_state_blob_get_size(blob)*4)+1;
	txt=ms_malloc0(txt_len);
	if (b64_encode(speex_echo_state_blob_get_data(blob),speex_echo_state_blob_get_size(blob),
			txt,txt_len)==0){
		ms_error("Base64 encoding failed.");
		ms_free(txt);
		return;
	}
	speex_echo_state_blob_free(blob);
	if (s->state_str) ms_free(s->state_str);
	s->state_str=txt;
}

#endif

static void speex_ec_preprocess(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	int delay_samples=0;
	mblk_t *m;

	s->echostarted=FALSE;
	s->filterlength=(s->tail_length_ms*s->samplerate)/1000;
	delay_samples=s->delay_ms*s->samplerate/1000;
	ms_message("Initializing speex echo canceler with framesize=%i, filterlength=%i, delay_samples=%i",
		s->framesize,s->filterlength,delay_samples);
	
	s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
	s->den = speex_preprocess_state_init(s->framesize, s->samplerate);
	speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_SAMPLING_RATE, &s->samplerate);
	speex_preprocess_ctl(s->den, SPEEX_PREPROCESS_SET_ECHO_STATE, s->ecstate);
	/* fill with zeroes for the time of the delay*/
	m=allocb(delay_samples*2,0);
	m->b_wptr+=delay_samples*2;
	ms_bufferizer_put (&s->delayed_ref,m);
	s->min_ref_samples=-1;
	s->nominal_ref_samples=delay_samples;
	audio_flow_controller_init(&s->afc);
#ifdef SPEEX_ECHO_GET_BLOB
	apply_config(s);
#else
	if (s->state_str) ms_warning("This version of speex doesn't support echo canceller restoration state. Rebuild speex and mediatreamer2 if you want to use this feature.");
#endif
}

/*	inputs[0]= reference signal from far end (sent to soundcard)
 *	inputs[1]= near speech & echo signal	(read from soundcard)
 *	outputs[0]=  is a copy of inputs[0] to be sent to soundcard
 *	outputs[1]=  near end speech, echo removed - towards far end
*/
static void speex_ec_process(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	int nbytes=s->framesize*2;
	mblk_t *refm;
	uint8_t *ref,*echo;
	
	if (s->bypass_mode) {
		while((refm=ms_queue_get(f->inputs[0]))!=NULL){
			ms_queue_put(f->outputs[0],refm);
		}
		while((refm=ms_queue_get(f->inputs[1]))!=NULL){
			ms_queue_put(f->outputs[1],refm);
		}
		return;
	}
	
	if (f->inputs[0]!=NULL){
		if (s->echostarted){
			while((refm=ms_queue_get(f->inputs[0]))!=NULL){
				mblk_t *cp=dupmsg(audio_flow_controller_process(&s->afc,refm));
				ms_bufferizer_put(&s->delayed_ref,cp);
				ms_bufferizer_put(&s->ref,refm);
			}
		}else{
			ms_warning("Getting reference signal but no echo to synchronize on.");
			ms_queue_flush(f->inputs[0]);
		}
	}

	ms_bufferizer_put_from_queue(&s->echo,f->inputs[1]);
	
	ref=(uint8_t*)alloca(nbytes);
	echo=(uint8_t*)alloca(nbytes);
	while (ms_bufferizer_read(&s->echo,echo,nbytes)>=nbytes){
		mblk_t *oecho=allocb(nbytes,0);
		int avail;
		int avail_samples;

		if (!s->echostarted) s->echostarted=TRUE;
		if ((avail=ms_bufferizer_get_avail(&s->delayed_ref))<((s->nominal_ref_samples*2)+nbytes)){
			/*we don't have enough to read in a reference signal buffer, inject silence instead*/
			refm=allocb(nbytes,0);
			memset(refm->b_wptr,0,nbytes);
			refm->b_wptr+=nbytes;
			ms_bufferizer_put(&s->delayed_ref,refm);
			ms_queue_put(f->outputs[0],dupmsg(refm));
			if (!s->using_zeroes){
				ms_warning("Not enough ref samples, using zeroes");
				s->using_zeroes=TRUE;
			}
		}else{
			if (s->using_zeroes){
				ms_message("Samples are back.");
				s->using_zeroes=FALSE;
			}
			/* read from our no-delay buffer and output */
			refm=allocb(nbytes,0);
			if (ms_bufferizer_read(&s->ref,refm->b_wptr,nbytes)==0){
				ms_fatal("Should never happen");
			}
			refm->b_wptr+=nbytes;
			ms_queue_put(f->outputs[0],refm);
		}

		/*now read a valid buffer of delayed ref samples*/
		if (ms_bufferizer_read(&s->delayed_ref,ref,nbytes)==0){
			ms_fatal("Should never happen");
		}
		avail-=nbytes;
		avail_samples=avail/2;
		if (avail_samples<s->min_ref_samples || s->min_ref_samples==-1){
			s->min_ref_samples=avail_samples;
		}
		
#ifdef EC_DUMP
		if (s->reffile)
			fwrite(ref,nbytes,1,s->reffile);
		if (s->echofile)
			fwrite(echo,nbytes,1,s->echofile);
#endif
		speex_echo_cancellation(s->ecstate,(short*)echo,(short*)ref,(short*)oecho->b_wptr);
		speex_preprocess_run(s->den, (short*)oecho->b_wptr);
#ifdef EC_DUMP
		if (s->cleanfile)
			fwrite(oecho->b_wptr,nbytes,1,s->cleanfile);
#endif
		oecho->b_wptr+=nbytes;
		ms_queue_put(f->outputs[1],oecho);
	}

	/*verify our ref buffer does not become too big, meaning that we are receiving more samples than we are sending*/
	if (f->ticker->time % flow_control_interval_ms == 0 && s->min_ref_samples!=-1){
		int diff=s->min_ref_samples-s->nominal_ref_samples;
		if (diff>(nbytes/2)){
			int purge=diff-(nbytes/2);
			ms_warning("echo canceller: we are accumulating too much reference signal, need to throw out %i samples",purge);
			audio_flow_controller_set_target(&s->afc,purge,(flow_control_interval_ms*s->samplerate)/1000);
		}
		s->min_ref_samples=-1;
	}
}

static void speex_ec_postprocess(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;

	ms_bufferizer_flush (&s->delayed_ref);
	ms_bufferizer_flush (&s->echo);
	ms_bufferizer_flush (&s->ref);
	if (s->ecstate!=NULL){
		speex_echo_state_destroy(s->ecstate);
		s->ecstate=NULL;
	}
	if (s->den!=NULL){
		speex_preprocess_state_destroy(s->den);
		s->den=NULL;
	}
}

static int speex_ec_set_sr(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->samplerate = *(int*)arg;
	return 0;
}

static int speex_ec_set_framesize(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->framesize = *(int*)arg;
	return 0;
}

static int speex_ec_set_delay(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->delay_ms = *(int*)arg;
	return 0;
}

static int speex_ec_set_tail_length(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->tail_length_ms=*(int*)arg;
	return 0;
}
static int speex_ec_set_bypass_mode(MSFilter *f, void *arg) {
	SpeexECState *s=(SpeexECState*)f->data;
	s->bypass_mode=*(bool_t*)arg;
	ms_message("set EC bypass mode to [%i]",s->bypass_mode);
	return 0;
}
static int speex_ec_get_bypass_mode(MSFilter *f, void *arg) {
	SpeexECState *s=(SpeexECState*)f->data;
	*(bool_t*)arg=s->bypass_mode;
	return 0;
}

static int speex_ec_set_state(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->state_str=ms_strdup((const char*)arg);
	return 0;
}

static int speex_ec_get_state(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
#ifdef SPEEX_ECHO_GET_BLOB
	fetch_config(s);
#endif
	*(char**)arg=s->state_str;
	return 0;
}

static MSFilterMethod speex_ec_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE		,	speex_ec_set_sr 		},
	{	MS_ECHO_CANCELLER_SET_TAIL_LENGTH	,	speex_ec_set_tail_length	},
	{	MS_ECHO_CANCELLER_SET_DELAY		,	speex_ec_set_delay		},
	{	MS_ECHO_CANCELLER_SET_FRAMESIZE		,	speex_ec_set_framesize		},
	{	MS_ECHO_CANCELLER_SET_BYPASS_MODE	,	speex_ec_set_bypass_mode	},
	{	MS_ECHO_CANCELLER_GET_BYPASS_MODE	,	speex_ec_get_bypass_mode	},
	{	MS_ECHO_CANCELLER_GET_STATE_STRING	,	speex_ec_get_state		},
	{	MS_ECHO_CANCELLER_SET_STATE_STRING	,	speex_ec_set_state		}
};

#ifdef _MSC_VER

MSFilterDesc ms_speex_ec_desc={
	MS_SPEEX_EC_ID,
	"MSSpeexEC",
	N_("Echo canceller using speex library"),
	MS_FILTER_OTHER,
	NULL,
	2,
	2,
	speex_ec_init,
	speex_ec_preprocess,
	speex_ec_process,
	speex_ec_postprocess,
	speex_ec_uninit,
	speex_ec_methods
};

#else

MSFilterDesc ms_speex_ec_desc={
	.id=MS_SPEEX_EC_ID,
	.name="MSSpeexEC",
	.text=N_("Echo canceller using speex library"),
	.category=MS_FILTER_OTHER,
	.ninputs=2,
	.noutputs=2,
	.init=speex_ec_init,
	.preprocess=speex_ec_preprocess,
	.process=speex_ec_process,
	.postprocess=speex_ec_postprocess,
	.uninit=speex_ec_uninit,
	.methods=speex_ec_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_speex_ec_desc)

