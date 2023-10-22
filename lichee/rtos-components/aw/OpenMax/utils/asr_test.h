/**
  utils/asr_test.h

  This simple test asr application take one input stream. Stores the output in another
  output file.

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


#ifndef __OMX_AUDIO_ASR_TEST_H__
#define __OMX_AUDIO_ASR_TEST_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Audio.h>
#include "OMX_Base.h"

/** Specification version*/
#define VERSIONMAJOR    1
#define VERSIONMINOR    1
#define VERSIONREVISION 0
#define VERSIONSTEP     0

#define AUDIO_OUTBUF_DEFAULT_SIZE 1920 //16kHz/3ch/16bit-20ms
#define DEFAULT_ASR_BUF_CNT		4
#define ASR_TEST_RATE		16000
#define ASR_TEST_CHANNELS		3
#define ASR_TEST_BIT_WIDTH		16
#define RPDATA_AUDIO_ASR_CHECK_CHAR 	(0xaa)
#define OMX_ASR_PORT_NUMBER_SUPPORTED 2
#define ASR_OUT_OFFSET 12

typedef struct vad_config_param {
	unsigned int value;
} vad_config_param_t;

typedef struct  asr_out{
	int   vad_flag;
	void *vad_out;
	int   word_id;
	float confidence;
}asr_out_t;

typedef struct  rpdata_arg_asr{
	char type[32];
	char name[32];
	int dir;
}rpdata_arg_asr;

typedef struct  asr_test_data {
	int loop_count;
	uint32_t rate,channels;
	rpdata_arg_asr targ;
	rpdata_arg_asr targ_dump;
	uint8_t bits;
	int8_t msec;
	bool asr_enable;
	bool asr_dump;
	char filename[32];
}asr_test_data;

/* Application's private data */
typedef struct asrPrivateType{
	int port_asr_in;
	int port_asr_out;
	rpdata_t *rpd;
	void *rpd_buffer;
	asr_test_data *asr_test;
	OMX_S32 buf_num;
	OMX_BUFFERHEADERTYPE **bufferin;
	OMX_BUFFERHEADERTYPE **bufferout;
	OMX_PARAM_PORTDEFINITIONTYPE arecord_port_para;
	OMX_PARAM_PORTDEFINITIONTYPE dump_port_para[OMX_ASR_PORT_NUMBER_SUPPORTED];
	OMX_PARAM_PORTDEFINITIONTYPE asr_port_para[OMX_ASR_PORT_NUMBER_SUPPORTED];
	omx_sem_t* arecord_eventSem;
	omx_sem_t* dump_eventSem;
	omx_sem_t* asr_eventSem;
	omx_sem_t* eofSem;
	OMX_HANDLETYPE arecord_handle;
	OMX_HANDLETYPE asr_handle;
	OMX_HANDLETYPE dump_handle;
}asrPrivateType;

/* Callback prototypes */
OMX_ERRORTYPE audioasrEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData);

OMX_ERRORTYPE asrarecordEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData);

OMX_ERRORTYPE audioasrEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE audioasrFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE asrFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE asrdumpEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData);

int do_rpdata_asr_send(rpdata_arg_asr *targ, rpdata_arg_asr *targ_dump);
void do_rpdata_asr_stop_send(void);
void rpdata_enable_asr(void);
void rpdata_disable_asr(void);
void rpdata_enable_dump(void);
void rpdata_disable_dump(void);
void rpdata_enable_dump_merge(void);
void rpdata_disable_dump_merge(void);
void rpdata_init_param(void);
void rpdata_deinit_param(void);


#endif
