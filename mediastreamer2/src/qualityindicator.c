/*
mediastreamer2 library - modular sound and video processing and streaming

 * Copyright (C) 2011  Belledonne Communications, Grenoble, France

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

#include "mediastreamer2/qualityindicator.h"

#include <math.h>

#define RATING_SCALE 5.0
#define WORSE_JITTER 0.2
#define WORSE_RT_PROP 5.0



struct _MSQualityIndicator{
	RtpSession *session;
	int clockrate;
	double sum_ratings;
	float rating;
	float local_rating;
	float remote_rating;
	uint64_t last_packet_count;
	uint32_t last_lost;
	uint32_t last_late;
	int count;
};

MSQualityIndicator *ms_quality_indicator_new(RtpSession *session){
	MSQualityIndicator *qi=ms_new0(MSQualityIndicator,1);
	qi->session=session;
	qi->rating=5.0;
	qi->local_rating=1.0;
	qi->remote_rating=1.0;
	return qi;
}

float ms_quality_indicator_get_rating(MSQualityIndicator *qi){
	return qi->rating;
}

static float inter_jitter_rating(float inter_jitter){
	float tmp=(float)(inter_jitter/WORSE_JITTER);
	if (tmp>1) tmp=1;
	return 1.0f-(0.3f*tmp);
}

static float rt_prop_rating(float rt_prop){
	float tmp=(float)(rt_prop/WORSE_RT_PROP);
	if (tmp>1) tmp=1;
	return 1.0f-(0.7f*tmp);
}

static float loss_rating(float loss){
	/* the exp function allows to have a rating that decrease rapidly at the begining.
	 Indeed even with 10% loss, the quality is significantly affected. It is not good at all.
	 With this formula:
	 5% losses gives a rating of 4/5
	 20% losses gives a rating of 2.2/5
	 80% losses gives a rating of 0.2
	*/
	return expf(-loss*4.0);
}

static float compute_rating(float loss_rate, float inter_jitter, float late_rate, float rt_prop){
	return loss_rating(loss_rate)*inter_jitter_rating(inter_jitter)*loss_rating(late_rate)*rt_prop_rating(rt_prop);
}

static void update_global_rating(MSQualityIndicator *qi){
	qi->rating=(float)(RATING_SCALE*qi->remote_rating*qi->local_rating);
	qi->sum_ratings+=qi->rating;
	qi->count++;
}

void ms_quality_indicator_update_from_feedback(MSQualityIndicator *qi, mblk_t *rtcp){
	const report_block_t *rb=NULL;
	if (rtcp_is_SR(rtcp)){
		rb=rtcp_SR_get_report_block(rtcp,0);
	}else if (rtcp_is_RR(rtcp)){
		rb=rtcp_RR_get_report_block(rtcp,0);
	}else{
		ms_warning("ms_quality_indicator_update_from_feedback(): not a RTCP report");
	}
	if (qi->clockrate==0){
		PayloadType *pt=rtp_profile_get_payload(rtp_session_get_send_profile(qi->session),rtp_session_get_send_payload_type(qi->session));
		if (pt!=NULL) qi->clockrate=pt->clock_rate;
		else return;
	}
	if (rb){
		float loss_rate=(float)report_block_get_fraction_lost(rb)/256.0f;
		float inter_jitter=(float)report_block_get_interarrival_jitter(rb)/(float)qi->clockrate;
		float rt_prop=rtp_session_get_round_trip_propagation(qi->session);
		qi->remote_rating=compute_rating(loss_rate,inter_jitter,0,rt_prop);
		update_global_rating(qi);
	}
}

void ms_quality_indicator_update_local(MSQualityIndicator *qi){
	const rtp_stats_t *stats=rtp_session_get_stats(qi->session);
	int lost,late,recvcnt;
	float loss_rate=0,late_rate=0;

	recvcnt=(int)(stats->packet_recv-qi->last_packet_count);
	if (recvcnt==0){
		ms_message("ms_quality_indicator_update_local(): no packet received since last call");
		return;/* no information usable*/
	}else if (recvcnt<0){
		qi->last_packet_count=stats->packet_recv;
		recvcnt=0; /*should not happen obviously*/
		return;
	}
	
	lost=(int)(stats->cum_packet_loss-qi->last_lost);
	qi->last_lost=(uint32_t)stats->cum_packet_loss;
	late=(int)(stats->outoftime-qi->last_late);
	qi->last_late=(uint32_t)stats->outoftime;
	qi->last_packet_count=stats->packet_recv;

	if (lost<0) lost=0;
	if (late<0) late=0;

	loss_rate=(float)lost/(float)recvcnt;
	late_rate=(float)late/(float)recvcnt;
	
	qi->local_rating=compute_rating(loss_rate,0,late_rate,rtp_session_get_round_trip_propagation(qi->session));
	update_global_rating(qi);
}

float ms_quality_indicator_get_average_rating(MSQualityIndicator *qi){
	if (qi->count==0) return -1; /*no rating available*/
	return (float)(qi->sum_ratings/(double)qi->count);
}

void ms_quality_indicator_destroy(MSQualityIndicator *qi){
	ms_free(qi);
}


