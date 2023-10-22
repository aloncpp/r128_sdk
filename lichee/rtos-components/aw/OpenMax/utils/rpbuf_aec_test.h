/**
  utils/rpbuf_aec_test.h

  This simple test application take one input stream. Put the input to aec.

* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef __RPBUF_AUDIO_ECHO_CANCEL_H__
#define __RPBUF_AUDIO_ECHO_CANCEL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Audio.h>
#include "OMX_Base.h"
#include "echo_cancellation.h"


/** Specification version*/
#define VERSIONMAJOR    1
#define VERSIONMINOR    1
#define VERSIONREVISION 0
#define VERSIONSTEP     0

#define AEC_RPBUF_BUFFER_PROCESS_LENGTH_DEFAULT 640  //byte

#define AEC_RPBUF_BUFFER_LENGTH_DEFAULT 2560  //byte

#define DEFAULT_AECHO_CANCEL_BUF_CNT	4

#define AEC_OMX_PORT_NUMBER_SUPPORTED 2

typedef struct  rpbuf_arg_aec{
	char config_name[32];  // config aec
	char aec_in_name[32];  // aec in
	char aec_in_recv_name[32];  // aec in recv
	char aec_out_name[32];  //aec out
}rpbuf_arg_aec;

typedef struct  aec_test_data {
	int aec_len;
	int aec_ctrl_id;
	OMX_AUDIO_ECHOCANTYPE aec_enable;
	rpbuf_arg_aec aec_arg;
}aec_test_data;

typedef struct
{
	/* sampling rate */
	int sampling_rate;
	/* bits_per_sample */
	int bits_per_sample;
	/* channel num */
	int chan;
	/* aec parameters*/
	AecConfig aec_config;
}aec_prms_t;

/* Application's private data */
typedef struct aecPrivateType{
	int port_filter_in;
	int port_filter_out;
	OMX_BOOL aec_eof;
	OMX_BUFFERHEADERTYPE **bufferin;
	OMX_BUFFERHEADERTYPE **bufferout;
	OMX_PARAM_PORTDEFINITIONTYPE aec_port_para[AEC_OMX_PORT_NUMBER_SUPPORTED];
	hal_ringbuffer_t aec_rb;
	hal_ringbuffer_t aec_out_rb;
	hal_event_t event;
	hal_event_t send_event;
	omx_sem_t* aec_start;
	omx_sem_t* aec_eventSem;
	omx_sem_t* eofSem;
	queue_t* BufferQueue;
	aec_test_data *aec_test;
	OMX_HANDLETYPE aec_handle;
	aec_prms_t aec_prms;
}aecPrivateType;

/* Callback prototypes */
OMX_ERRORTYPE AudioEchoCancelEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData);

OMX_ERRORTYPE AudioEchoCancelEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE AudioEchoCancelFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer);


#endif
