/**
  utils/aec_test.c

  This simple test application take one input stream/s. passes
  these streams to an audio echo cancel component and stores the output in another
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <rpdata.h>
#include <console.h>

#include <hal_time.h>
#include <hal_mutex.h>
#include <hal_sem.h>

#include "aec_test.h"


#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
#include <adb_forward.h>
#endif


static int g_aec_verbose = 0;
static int g_aec_forward_port = 0;
static int g_aec_bits = 0;
static int g_aec_loop_count = 0;
static int g_aec_run_msec = 0;
static int g_aec_rate = 0;
static int g_aec_channels = 0;
static int g_aec_cap = 0;

static OMX_U32 error_value = 0u;

static OMX_CALLBACKTYPE arecord_callbacks = {
	.EventHandler = audiorecEventHandler,
	.EmptyBufferDone = audiorecEmptyBufferDone,
	.FillBufferDone = audiorecFillBufferDone,
};

static OMX_CALLBACKTYPE aec_callbacks = {
	.EventHandler = audioechocancelEventHandler,
	.EmptyBufferDone = audioechocancelEmptyBufferDone,
	.FillBufferDone = aechocancelFillBufferDone,
};

static OMX_CALLBACKTYPE aecho_cancel_callbacks = {
	.EventHandler = audioechocancelEventHandler,
	.EmptyBufferDone = audioechocancelEmptyBufferDone,
	.FillBufferDone = audioechocancelFillBufferDone,
};

static void setHeader(OMX_PTR header, OMX_U32 size) {
  OMX_VERSIONTYPE* ver = (OMX_VERSIONTYPE*)(header + sizeof(OMX_U32));
  *((OMX_U32*)header) = size;

  ver->s.nVersionMajor = VERSIONMAJOR;
  ver->s.nVersionMinor = VERSIONMINOR;
  ver->s.nRevision = VERSIONREVISION;
  ver->s.nStep = VERSIONSTEP;
}

/* Callbacks implementation */
OMX_ERRORTYPE audiorecEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	aecPrivateType *private = (aecPrivateType *)pAppData;
	if (hComponent == private->arecord_handle)
		name = "audiorecord";

	printf("In the %s callback\n", __func__);
	if(eEvent == OMX_EventCmdComplete) {
		if (Data1 == OMX_CommandStateSet) {
			switch ((int)Data2) {
				case OMX_StateInvalid:
					printf("%s StateSet OMX_StateInvalid\n", name);
					break;
				case OMX_StateLoaded:
					printf("%s StateSet OMX_StateLoaded\n", name);
					break;
				case OMX_StateIdle:
					printf("%s StateSet OMX_StateIdle\n", name);
					break;
				case OMX_StateExecuting:
					printf("%s StateSet OMX_StateExecuting\n", name);
					break;
				case OMX_StatePause:
					printf("%s StateSet OMX_StatePause\n", name);
					break;
				case OMX_StateWaitForResources:
					printf("%s StateSet WaitForResources\n", name);
					break;
				default:
					printf("%s StateSet unkown state\n", name);
				break;
			}
			omx_sem_up(private->arecord_eventSem);
		} else  if (Data1 == OMX_CommandPortEnable){
			printf("%s CmdComplete OMX_CommandPortEnable\n", name);
			omx_sem_up(private->arecord_eventSem);
		} else if (Data1 == OMX_CommandPortDisable) {
			printf("%s CmdComplete OMX_CommandPortDisable\n", name);
			omx_sem_up(private->arecord_eventSem);
		}
	} else if(eEvent == OMX_EventBufferFlag) {
		if ((int)Data2 == OMX_BUFFERFLAG_EOS) {
			printf("%s BufferFlag OMX_BUFFERFLAG_EOS\n", name);
			if (hComponent == private->arecord_handle) {
				printf("end of tunnel");
				omx_sem_up(private->eofSem);
			}
		} else
			printf("%s OMX_EventBufferFlag %lx", name, Data2);
	} else if (eEvent == OMX_EventError) {
		error_value = Data1;
		printf("Receive error event. value:%lx", error_value);
		omx_sem_up(private->arecord_eventSem);
	} else {
		printf("Param1 is %i\n", (int)Data1);
		printf("Param2 is %i\n", (int)Data2);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE audiorecEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

  printf("In the %s callback from the port %i\n", __func__, (int)pBuffer->nInputPortIndex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE audiorecFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	printf("In the %s callback. Got buflen %i for buffer at 0x%p\n",
                          __func__, (int)pBuffer->nFilledLen, pBuffer);

	return OMX_ErrorNone;
}

/* Callbacks implementation */
OMX_ERRORTYPE audioechocancelEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	aecPrivateType *private = (aecPrivateType *)pAppData;
	if (hComponent == private->aec_handle)
		name = "audioechocancel";

	printf("In the %s callback\n", __func__);
	if(eEvent == OMX_EventCmdComplete) {
		if (Data1 == OMX_CommandStateSet) {
			switch ((int)Data2) {
				case OMX_StateInvalid:
					printf("%s StateSet OMX_StateInvalid\n", name);
					break;
				case OMX_StateLoaded:
					printf("%s StateSet OMX_StateLoaded\n", name);
					break;
				case OMX_StateIdle:
					printf("%s StateSet OMX_StateIdle\n", name);
					break;
				case OMX_StateExecuting:
					printf("%s StateSet OMX_StateExecuting\n", name);
					break;
				case OMX_StatePause:
					printf("%s StateSet OMX_StatePause\n", name);
					break;
				case OMX_StateWaitForResources:
					printf("%s StateSet WaitForResources\n", name);
					break;
				default:
					printf("%s StateSet unkown state\n", name);
				break;
			}
			omx_sem_up(private->aec_eventSem);
		} else  if (Data1 == OMX_CommandPortEnable){
			printf("%s CmdComplete OMX_CommandPortEnable\n", name);
			omx_sem_up(private->aec_eventSem);
		} else if (Data1 == OMX_CommandPortDisable) {
			printf("%s CmdComplete OMX_CommandPortDisable\n", name);
			omx_sem_up(private->aec_eventSem);
		}
	} else if(eEvent == OMX_EventBufferFlag) {
		if ((int)Data2 == OMX_BUFFERFLAG_EOS) {
			printf("%s BufferFlag OMX_BUFFERFLAG_EOS\n", name);
			if (hComponent == private->aec_handle) {
				printf("end of tunnel");
				omx_sem_up(private->eofSem);
			}
		} else
			printf("%s OMX_EventBufferFlag %lx", name, Data2);
	} else if (eEvent == OMX_EventError) {
		error_value = Data1;
		printf("Receive error event. value:%lx", error_value);
		omx_sem_up(private->aec_eventSem);
	} else {
		printf("Param1 is %i\n", (int)Data1);
		printf("Param2 is %i\n", (int)Data2);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE audioechocancelEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	printf("In the %s callback from the port %i\n", __func__, (int)pBuffer->nInputPortIndex);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE audioechocancelFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	int ret;
	aecPrivateType *private = (aecPrivateType *)pAppData;

	printf("In the %s callback. Got buflen %i for buffer at 0x%p\n",
                          __func__, (int)pBuffer->nFilledLen, pBuffer);

	if (pBuffer == NULL || pAppData == NULL) {
	  printf("err: buffer header is null");
	  return OMX_ErrorBadParameter;
	}

	if (pBuffer->nFilledLen == 0) {
		printf("Ouch! In %s: no data in the output buffer!\n", __func__);
		return OMX_ErrorNone;
	}

	if(pBuffer->nFilledLen > 0) {
		if (rpdata_is_connect(private->rpd) == 0) {
			memcpy(private->rpd_buffer, pBuffer->pBuffer, pBuffer->nFilledLen);
			ret = rpdata_send(private->rpd, 0, pBuffer->nFilledLen);
			if (ret != 0) {
				printf("[%s] line:%d \n", __func__, __LINE__);
			}
		}
		else {
			printf("[%s] line:%d rpdata is not connected \n", __func__, __LINE__);
		}
	}

	/* Output data to standard output */
	pBuffer->nFilledLen = 0;
	err = OMX_FillThisBuffer(hComponent, pBuffer);
	if (err != OMX_ErrorNone)
		printf("OMX_FillThisBuffer err: %x", err);

  return err;
}


OMX_ERRORTYPE aechocancelFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	aecPrivateType *private = (aecPrivateType *)pAppData;

	printf("In the %s callback. Got buflen %i for buffer at 0x%p\n",
                          __func__, (int)pBuffer->nFilledLen, pBuffer);

	if (pBuffer == NULL || pAppData == NULL) {
	  printf("err: buffer header is null");
	  return OMX_ErrorBadParameter;
	}

	if (pBuffer->nFilledLen == 0) {
		printf("Ouch! In %s: no data in the output buffer!\n", __func__);
		return OMX_ErrorNone;
	}

	if(pBuffer->nFilledLen > 0) {
#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
		if (g_aec_forward_port > 0)
			adb_forward_send(g_aec_forward_port, pBuffer->pBuffer, pBuffer->nFilledLen);
		else {
			printf("[%s] line:%d g_rpd_forward_port %d err \n", __func__, __LINE__, g_aec_forward_port);
		}
#endif
	}

	/* Output data to standard output */
	pBuffer->nFilledLen = 0;
	err = OMX_FillThisBuffer(private->aec_handle, pBuffer);
	if (err != OMX_ErrorNone)
		printf("OMX_FillThisBuffer err: %x", err);

  return err;
}

static void aec_demo_usage(void)
{
	printf("Usgae: aec_demo [option]\n");
	printf("-h,          rpdata help\n");
	printf("-m,          mode, 0-send; 1-recv;\n");
	printf("-t,          type, type name\n");
	printf("-n,          name, id name\n");
	printf("             (type + name) specify unique data xfer\n");
	printf("-d,          dir, remote processor, 1-cm33;2-c906;3-dsp\n");
	printf("\n");
	printf("DSP -> RV\n");
	printf("rpccli dsp rpdata_audio -m 0 -d 2 -t DSPtoRVAudio -n RVrecvDSPsend\n");
	printf("rpdata_audio -m 1 -d 3 -t DSPtoRVAudio -n RVrecvDSPsend\n");
	printf("\n");
}

typedef struct  rpdata_arg_aec{
	char type[32];
	char name[32];
	int dir;
}rpdata_arg_aec;

typedef struct  aec_test_data {
	int loop_count;
	uint32_t rate;
	uint8_t channels;
	uint8_t bits;
	int8_t msec;
	rpdata_arg_aec *targ;
}aec_test_data;

static int get_port_index(OMX_COMPONENTTYPE *comp, OMX_DIRTYPE dir,
	OMX_PORTDOMAINTYPE domain, int start)
{
	int i;
	int ret = -1;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	for (i = start; i < OMX_PORT_NUMBER_SUPPORTED; i++) {
		port_def.nPortIndex = i;
		ret = OMX_GetParameter(comp, OMX_IndexParamPortDefinition, &port_def);
		if (ret == OMX_ErrorNone && port_def.eDir == dir
			&& port_def.eDomain == domain) {
			ret = i;
			break;
		}
	}
	printf("index:%d\n", i);
	if (i == OMX_PORT_NUMBER_SUPPORTED)
		printf("can not get port, dir:%d, domain:%d, start:%d",
			dir, domain, start);
	return ret;
}

static OMX_ERRORTYPE alloc_aec_buffer(aecPrivateType *priv, OMX_S32 num, OMX_S32 size)
{
	OMX_S32 i = 0;
	OMX_S32 port_index = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int port_aec_out = -1;

	port_aec_out = get_port_index(priv->aec_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_aec_out < 0) {
		printf("Error in get port index, port_aec_out %d\n", port_aec_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}
	port_index = port_aec_out;

	priv->buffer = malloc(num * sizeof(OMX_BUFFERHEADERTYPE *));
	if (NULL == priv->buffer)
		return OMX_ErrorBadParameter;
	for (i = 0; i < num; i++) {
		priv->buffer[i] = NULL;
		ret = OMX_AllocateBuffer(priv->aec_handle, &priv->buffer[i],
				port_index, priv, size);
		printf("AllocateBuffer %p on port %ld\n", priv->buffer[i], port_index);
		if (ret != OMX_ErrorNone) {
			printf("Error on AllocateBuffer %p on port %ld\n",
				&priv->buffer[i], port_index);
			break;
		}
	}

	return ret;
}

static void free_aec_buffer(aecPrivateType *priv)
{
	OMX_S32 i = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int port_aec_out = -1;
	OMX_S32 port_index = 0;

	port_aec_out = get_port_index(priv->aec_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_aec_out < 0) {
		printf("Error in get port index, port_aec_out %d\n", port_aec_out);
		return;
	}
	port_index = port_aec_out;

	if (priv->buffer) {
		for (i = 0; i < priv->buf_num; i++) {
			if (priv->buffer[i]) {
				ret = OMX_FreeBuffer(priv->aec_handle,
						port_index,
						priv->buffer[i]);
				if (ret != OMX_ErrorNone)
					printf("port %ld ,freebuffer:%ld failed",
						port_index, i);
			}
			priv->buffer[i] = NULL;
		}
		free(priv->buffer);
		priv->buffer = NULL;
	}
}

static int tunnel_config_arecord_component(aecPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	OMX_PORT_PARAM_TYPE sParam;
	int port_are_out = -1;

	if (g_aec_bits == 0 || g_aec_channels == 0 || g_aec_rate == 0 || g_aec_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = g_aec_bits / 8 * g_aec_channels;
	uint32_t len = frame_bytes * g_aec_rate / 100; /* 10ms */
	len = (len / 10) * g_aec_run_msec;

	port_are_out = get_port_index(priv->arecord_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_are_out < 0) {
		printf("Error in get port index, port_are_out %d\n", port_are_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}

	memset(&priv->arecord_port_para, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->arecord_port_para.nBufferCountActual = DEFAULT_AEC_BUF_CNT;
	priv->arecord_port_para.bBuffersContiguous = 1;
	priv->arecord_port_para.eDomain = OMX_PortDomainAudio;
	priv->arecord_port_para.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->arecord_port_para.nPortIndex = port_are_out;
	priv->arecord_port_para.nBufferSize = len;

	setHeader(&priv->arecord_port_para, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->arecord_handle, OMX_IndexParamPortDefinition,
			&priv->arecord_port_para);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_aec_channels;
	audio_params.nBitPerSample = g_aec_bits;
	audio_params.nSamplingRate = g_aec_rate;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->arecord_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio record params error!");
		return ret;
	}

	/** Get the number of ports */
	setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
	ret = OMX_GetParameter(priv->arecord_handle, OMX_IndexParamAudioInit, &sParam);
	if(ret != OMX_ErrorNone){
		printf("Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		return ret;
	}
	printf("Audio record has %d ports\n",(int)sParam.nPorts);

	return ret;
}

static int tunnel_config_aec_component(aecPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	OMX_PORT_PARAM_TYPE sParam;
	int port_aec_in = -1;
	int port_aec_out = -1;

	if (g_aec_bits == 0 || g_aec_channels == 0 || g_aec_rate == 0 || g_aec_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = g_aec_bits / 8 * g_aec_channels;
	uint32_t len = frame_bytes * g_aec_rate / 100; /* 10ms */
	len = (len / 10) * g_aec_run_msec;


	port_aec_in = get_port_index(priv->aec_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	port_aec_out = get_port_index(priv->aec_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_aec_in < 0 || port_aec_out < 0) {
		printf("Error in get port index, port_aec_in %d port_aec_out %d\n", port_aec_in, port_aec_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}

	memset(&priv->aec_port_para[port_aec_in], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

	/* set input port */
	priv->buf_num = DEFAULT_AEC_BUF_CNT;
	priv->aec_port_para[port_aec_in].nBufferCountActual = priv->buf_num;
	priv->aec_port_para[port_aec_in].bBuffersContiguous = 1;
	priv->aec_port_para[port_aec_in].eDomain = OMX_PortDomainAudio;
	priv->aec_port_para[port_aec_in].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->aec_port_para[port_aec_in].nPortIndex = port_aec_in;
	priv->aec_port_para[port_aec_in].nBufferSize = len;

	setHeader(&priv->aec_port_para[port_aec_in], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->aec_handle, OMX_IndexParamPortDefinition,
			&priv->aec_port_para[port_aec_in]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_aec_channels;
	audio_params.nBitPerSample = g_aec_bits;
	audio_params.nSamplingRate = g_aec_rate;
	audio_params.nPortIndex = port_aec_in;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->aec_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio echo cancel params error!");
		return ret;
	}


	/* set output port */
	memset(&priv->aec_port_para[port_aec_out], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->buf_num = DEFAULT_AEC_BUF_CNT;
	priv->aec_port_para[port_aec_out].nBufferCountActual = priv->buf_num;
	priv->aec_port_para[port_aec_out].bBuffersContiguous = 1;
	priv->aec_port_para[port_aec_out].eDomain = OMX_PortDomainAudio;
	priv->aec_port_para[port_aec_out].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->aec_port_para[port_aec_out].nPortIndex = port_aec_out;
	priv->aec_port_para[port_aec_out].nBufferSize = len;

	setHeader(&priv->aec_port_para[port_aec_out], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->aec_handle, OMX_IndexParamPortDefinition,
			&priv->aec_port_para[port_aec_out]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_aec_channels;
	audio_params.nBitPerSample = g_aec_bits;
	audio_params.nSamplingRate = g_aec_rate;
	audio_params.nPortIndex = port_aec_out;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->aec_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio echo cancel params error!");
		return ret;
	}

	/** Get the number of ports */
	setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
	ret = OMX_GetParameter(priv->aec_handle, OMX_IndexParamAudioInit, &sParam);
	if(ret != OMX_ErrorNone){
		printf("Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		return ret;
	}
	printf("Audio echo cancel has %d ports\n",(int)sParam.nPorts);

	return ret;
}

static void rpdata_aec_send(void *arg)
{
	aec_test_data *aec_test = (aec_test_data *)arg;
	rpdata_arg_aec targ;
	rpdata_t *rpd;
	void *buffer = NULL;
	aecPrivateType *priv = NULL;
	int ret = -1;
	int i = 0;
	int port_src_out = -1;
	int port_filter_in = -1;
	int port_filter_out = -1;
	int rate = aec_test->rate;
	int bits = aec_test->bits;
	int channels = aec_test->channels;
	int frame_bytes = bits / 8 * channels;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
	len = (len / 10) * g_aec_run_msec;
#if 0
	uint32_t rpdata_len = bits / 8 * 1 * rate / 100; /* data after processing(aec, mono ch) */
#else
	uint32_t rpdata_len = len ;
#endif
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;

	memcpy(&targ, aec_test->targ, sizeof(rpdata_arg_aec));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

	rpd = rpdata_create(targ.dir, targ.type, targ.name, rpdata_len);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	/* Initialize application private data */
	priv = malloc(sizeof(aecPrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	memset(priv, 0 ,sizeof(aecPrivateType));

	priv->arecord_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->arecord_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->arecord_eventSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	priv->aec_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->aec_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->aec_eventSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	priv->eofSem = malloc(sizeof(omx_sem_t));
	if (!priv->eofSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->eofSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	priv->rpd = rpd;
	priv->rpd_buffer = buffer;

	 /* 1. get component handle */
	err = OMX_GetHandle(&priv->arecord_handle, "OMX.audio.record", priv, &arecord_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio record OMX_GetHandle failed\n");
		goto exit;
	}

	err = OMX_GetHandle(&priv->aec_handle, "OMX.audio.echocancel", priv, &aecho_cancel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio echocancel OMX_GetHandle failed\n");
		goto exit;
	}

	/* 2. config component */
	err = tunnel_config_arecord_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in tunnel_config_arecord_component\n");
		goto exit;
	}
	err = tunnel_config_aec_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in tunnel_config_aec_component\n");
		goto exit;
	}

	port_src_out = get_port_index(priv->arecord_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	port_filter_in = get_port_index(priv->aec_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	if (port_src_out < 0 || port_filter_in < 0) {
		printf("Error in get port index, port_src_out %d port_filter_in %d\n", port_src_out, port_filter_in);
		goto exit;
	}

	/* 3. set tunnel */
	err = OMX_SetupTunnel(priv->arecord_handle, port_src_out, priv->aec_handle, port_filter_in);
	if(err != OMX_ErrorNone) {
		printf("Set up Tunnel between audio record & audio echo cancel Failed\n");
		goto exit;
	}

	/* 4. set component stat to idle */
	err = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	err = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->arecord_eventSem);

	port_filter_out = get_port_index(priv->aec_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_filter_out < 0) {
		printf("Error in get port index, port_filter_out %d\n", port_filter_out);
		goto exit;
	}

	port_def.nPortIndex = port_filter_out;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->aec_handle, OMX_IndexParamPortDefinition, &port_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}
	if ((priv->aec_port_para[port_filter_out].nBufferSize != port_def.nBufferSize) ||
		(priv->aec_port_para[port_filter_out].nBufferCountActual != port_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize %ld nBufferCountActual %ld\n", \
			priv->aec_port_para[port_filter_out].nBufferSize, priv->aec_port_para[port_filter_out].nBufferCountActual , \
			port_def.nBufferSize, port_def.nBufferCountActual);
		goto exit;
	}

	err = alloc_aec_buffer(priv, port_def.nBufferCountActual, port_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}
	omx_sem_down(priv->aec_eventSem);

	/* 5. set component stat to executing */
	err = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->arecord_eventSem);

	err = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->aec_eventSem);

	rpdata_wait_connect(rpd);

	/* 6. send buffer to source component queue */
	for (i = 0; i < priv->buf_num; i++) {
		err = OMX_FillThisBuffer(priv->aec_handle, priv->buffer[i]);
		printf("OMX_FillThisBuffer %p on port %lu\n", priv->buffer[i],
			priv->buffer[i]->nOutputPortIndex);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_FillThisBuffer\n");
			goto exit;
		}
	}

	printf("audio echo cancel test start\n");

	while (aec_test->loop_count--) {
		hal_msleep(g_aec_run_msec);
	}

	printf("audio echo cancel test end\n");

	/* 7. set component stat to idle */
	ret = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->arecord_eventSem);

	ret = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->aec_eventSem);

	/* 8. set component stat to loaded */
	ret = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}


	ret = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}

	omx_sem_down(priv->arecord_eventSem);

	/* 9. free buffer */
	free_aec_buffer(priv);

	omx_sem_down(priv->aec_eventSem);

exit:
	printf("rpdata auto send test finish start\n");

	free_aec_buffer(priv);

	if (priv->arecord_eventSem) {
		omx_sem_deinit(priv->arecord_eventSem);
		free(priv->arecord_eventSem);
	}
	if (priv->aec_eventSem) {
		omx_sem_deinit(priv->aec_eventSem);
		free(priv->aec_eventSem);
	}
	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
	}
	if (priv->arecord_handle) {
		OMX_FreeHandle(priv->arecord_handle);
		priv->arecord_handle = NULL;
	}
	if (priv->aec_handle) {
		OMX_FreeHandle(priv->aec_handle);
		priv->aec_handle = NULL;
	}
	if (priv->rpd) {
		rpdata_destroy(priv->rpd);
		priv->rpd_buffer = NULL;
	}

	OMX_Deinit();

	if (priv) {
		free(priv);
		priv = NULL;
	}

	if (aec_test) {
		free(aec_test);
		aec_test = NULL;
	}

	g_aec_forward_port = 0;
	printf("rpdata audio echo cancel send test finish end\n");
	vTaskDelete(NULL);
}


static int do_rpdata_aec_send(struct rpdata_arg_aec *targ)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	aec_test_data *aec_test;


	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	aec_test = malloc(sizeof(aec_test_data));
	if (aec_test) {
		aec_test->targ = targ;
		aec_test->rate = g_aec_rate;
		aec_test->channels = g_aec_channels;
		aec_test->bits = g_aec_bits;
		aec_test->loop_count = g_aec_loop_count;
		aec_test->msec = g_aec_run_msec;
	} else {
		printf("malloc are_test failed\n");
		return -1;
	}

	xTaskCreate(rpdata_aec_send, "rpd_aec_send", 4096, aec_test,
			configAPPLICATION_OMX_PRIORITY, &handle);
	hal_msleep(500);
	return 0;
}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
static int rpd_audio_echo_cancel_forward_cb(rpdata_t *rpd, void *data, uint32_t data_len)
{
	int ret;

	if (!g_aec_forward_port)
		return 0;

	ret = adb_forward_send(g_aec_forward_port, data, data_len);
	/*printf("[%s] line:%d ret=%d, data_len=%d\n", __func__, __LINE__, ret, data_len);*/
	return 0;
}

struct rpdata_cbs rpd_audio_echo_cancel_forward_cbs = {
	.recv_cb = rpd_audio_echo_cancel_forward_cb,
};
#endif

static int rpd_audio_echo_cancel_cb(rpdata_t *rpd, void *data, uint32_t data_len)
{
	static int count = 0;

#if 1
	int i;
	for (i = 0; i < data_len; i++) {
		uint8_t *c = data + i;
		if (*c  != RPDATA_AUDIO_CHECK_CHAR) {
			printf("data check failed, expected 0x%x but 0x%x\n",
				RPDATA_AUDIO_CHECK_CHAR, *c);
			return 0;
		}
	}
#endif
	if (!g_aec_verbose)
		return 0;
	if (count++ % g_aec_verbose == 0)
		printf("audio data check ok(print interval %d)\n", g_aec_verbose);
	return 0;
}

struct rpdata_cbs rpd_audio_echo_cancel_cbs = {
	.recv_cb = rpd_audio_echo_cancel_cb,
};

static void rpdata_aec_recv(void *arg)
{
	rpdata_arg_aec targ;
	rpdata_t *rpd;
	void *buffer = NULL;


	memcpy(&targ, arg, sizeof(rpdata_arg_aec));
	printf("dir:%d, type:%s, name:%s\n", targ.dir, targ.type, targ.name);

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if (!g_aec_forward_port) {
		printf("[%s] line:%d g_rpd_forward_port=%d\n", __func__, __LINE__, g_aec_forward_port);
		goto exit;
	}
	if (adb_forward_create_with_rawdata(g_aec_forward_port) < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
#endif

	rpd = rpdata_connect(targ.dir, targ.type, targ.name);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

#if 1

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	rpdata_set_recv_cb(rpd, &rpd_audio_echo_cancel_forward_cbs);
#else
	rpdata_set_recv_cb(rpd, &rpd_audio_echo_cancel_cbs);
#endif
	while (g_aec_loop_count--) {
		hal_msleep(g_aec_run_msec + 2);
	}
#else
	void *recv_buf = NULL;
	int len = 0;
	int ret = 0;

	rpdata_set_recv_ringbuffer(rpd, 4 * rpdata_buffer_len(rpd));

	len = rpdata_buffer_len(rpd);
	if (len <= 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	recv_buf = malloc(len);
	if (!recv_buf) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	while (1) {
		ret = rpdata_recv(rpd, recv_buf, len, 10000);
		if (ret <= 0) {
			printf("rpdata recv timeout \n");
			goto exit;
		}
#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
		ret = adb_forward_send(g_aec_forward_port, recv_buf, len);
		if (ret != 0) {
			printf("adb_forward_send err ret%d \n", ret);
			goto exit;
		}
#endif
	}
	if (recv_buf)
		free(recv_buf);
#endif

exit:
	if (rpd)
		rpdata_destroy(rpd);

	printf("rpdata audio echo cancel recv test finish\n");
	vTaskDelete(NULL);
}

static int do_rpdata_aec_recv(rpdata_arg_aec *targ)
{
	TaskHandle_t handle;

	xTaskCreate(rpdata_aec_recv, "rpd_aec_recv", 1024, targ,
			configAPPLICATION_OMX_PRIORITY, &handle);
	hal_msleep(500);
	return 0;
}

static int aec_check_dir(int dir)
{
	switch (dir) {
	case RPDATA_DIR_CM33:
	case RPDATA_DIR_RV:
	case RPDATA_DIR_DSP:
		return 0;
	default:
		return -1;
	}
}

static void audio_echocancel_task(void *arg)
{
	aec_test_data *aec_test = (aec_test_data *)arg;
	aecPrivateType *priv = NULL;
	int ret = -1;
	int i = 0;
	int port_src_out = -1;
	int port_filter_in = -1;
	int port_filter_out = -1;
	int rate = aec_test->rate;
	int bits = aec_test->bits;
	int channels = aec_test->channels;
	int frame_bytes = bits / 8 * channels;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
	len = (len / 10) * g_aec_run_msec;

	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;

	/* Initialize application private data */
	priv = malloc(sizeof(aecPrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	memset(priv, 0 ,sizeof(aecPrivateType));

	priv->arecord_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->arecord_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->arecord_eventSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	priv->aec_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->aec_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->aec_eventSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	priv->eofSem = malloc(sizeof(omx_sem_t));
	if (!priv->eofSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->eofSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}


	 /* 1. get component handle */
	err = OMX_GetHandle(&priv->arecord_handle, "OMX.audio.record", priv, &arecord_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio record OMX_GetHandle failed\n");
		goto exit;
	}

	err = OMX_GetHandle(&priv->aec_handle, "OMX.audio.echocancel", priv, &aec_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio echocancel OMX_GetHandle failed\n");
		goto exit;
	}

	/* 2. config component */
	err = tunnel_config_arecord_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in tunnel_config_arecord_component\n");
		goto exit;
	}
	err = tunnel_config_aec_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in tunnel_config_aec_component\n");
		goto exit;
	}

	port_src_out = get_port_index(priv->arecord_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	port_filter_in = get_port_index(priv->aec_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	if (port_src_out < 0 || port_filter_in < 0) {
		printf("Error in get port index, port_src_out %d port_filter_in %d\n", port_src_out, port_filter_in);
		goto exit;
	}

	/* 3. set tunnel */
	err = OMX_SetupTunnel(priv->arecord_handle, port_src_out, priv->aec_handle, port_filter_in);
	if(err != OMX_ErrorNone) {
		printf("Set up Tunnel between audio record & audio echo cancel Failed\n");
		goto exit;
	}

	/* 4. set component stat to idle */
	err = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	err = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->arecord_eventSem);

	port_filter_out = get_port_index(priv->aec_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_filter_out < 0) {
		printf("Error in get port index, port_filter_out %d\n", port_filter_out);
		goto exit;
	}

	port_def.nPortIndex = port_filter_out;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->aec_handle, OMX_IndexParamPortDefinition, &port_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}
	if ((priv->aec_port_para[port_filter_out].nBufferSize != port_def.nBufferSize) ||
		(priv->aec_port_para[port_filter_out].nBufferCountActual != port_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize %ld nBufferCountActual %ld\n", \
			priv->aec_port_para[port_filter_out].nBufferSize, priv->aec_port_para[port_filter_out].nBufferCountActual , \
			port_def.nBufferSize, port_def.nBufferCountActual);
		goto exit;
	}

	err = alloc_aec_buffer(priv, port_def.nBufferCountActual, port_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}
	omx_sem_down(priv->aec_eventSem);

	/* 5. set component stat to executing */
	err = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->arecord_eventSem);

	err = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->aec_eventSem);

	/* 6. send buffer to source component queue */
	for (i = 0; i < priv->buf_num; i++) {
		err = OMX_FillThisBuffer(priv->aec_handle, priv->buffer[i]);
		printf("OMX_FillThisBuffer %p on port %lu\n", priv->buffer[i],
			priv->buffer[i]->nOutputPortIndex);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_FillThisBuffer\n");
			goto exit;
		}
	}

	printf("audio echo cancel test start\n");

	while (aec_test->loop_count--) {
		hal_msleep(g_aec_run_msec);
	}

	printf("audio echo cancel test end\n");

	/* 7. set component stat to idle */
	ret = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->arecord_eventSem);

	ret = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->aec_eventSem);

	/* 8. set component stat to loaded */
	ret = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}


	ret = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}

	omx_sem_down(priv->arecord_eventSem);

	/* 9. free buffer */
	free_aec_buffer(priv);

	omx_sem_down(priv->aec_eventSem);


exit:
	if (priv->buffer)
		free_aec_buffer(priv);

	if (priv->arecord_eventSem) {
		omx_sem_deinit(priv->arecord_eventSem);
		free(priv->arecord_eventSem);
	}
	if (priv->aec_eventSem) {
		omx_sem_deinit(priv->aec_eventSem);
		free(priv->aec_eventSem);
	}
	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
	}
	if (priv->arecord_handle) {
		OMX_FreeHandle(priv->arecord_handle);
		priv->arecord_handle = NULL;
	}
	if (priv->aec_handle) {
		OMX_FreeHandle(priv->aec_handle);
		priv->aec_handle = NULL;
	}

	if (priv) {
		free(priv);
		priv = NULL;
	}
	if (aec_test) {
		free(aec_test);
		aec_test = NULL;
	}
	g_aec_cap = 0;
	g_aec_forward_port = 0;
	printf("audio echocancel task finish\n");
	vTaskDelete(NULL);
}


static int audio_echocancel_task_create(struct aec_test_data *data)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	aec_test_data *aec_test;

	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	aec_test = malloc(sizeof(aec_test_data));
	if (!aec_test)
	{
		printf("malloc aec_test failed\n");
		return -1;
	}

	if (!data) {
		aec_test->rate = g_aec_rate;
		aec_test->channels = g_aec_channels;
		aec_test->bits = g_aec_bits;
		aec_test->loop_count = g_aec_loop_count;
		aec_test->msec = g_aec_run_msec;
	} else {
		memcpy(aec_test, data, sizeof(struct aec_test_data));
	}

	xTaskCreate(audio_echocancel_task, "aechocancel_task", 4096, aec_test,
			configAPPLICATION_OMX_PRIORITY,
			&handle);
	return 0;
}

static int cmd_aec_test(int argc, char *argv[])
{
	int c, mode = 2;
	rpdata_arg_aec targ = {
		.type = "RVtoDSPAec",
		.name = "RVrecvDSPsend",
		.dir  = RPDATA_DIR_DSP,
	};

	g_aec_loop_count = 1000;
	g_aec_run_msec = 20;  //ms, total time g_are_run_msec * g_are_loop_count = 20s
	g_aec_rate = TEST_RATE;
	g_aec_channels = TEST_CHANNELS;
	g_aec_bits = TEST_BIT_WIDTH;

	optind = 0;
	while ((c = getopt(argc, argv, "hm:t:n:d:l:s:r:c:b:v:g:f:")) != -1) {
		switch (c) {
		case 'm':
			mode = atoi(optarg);
			break;
		case 't':
			strncpy(targ.type, optarg, sizeof(targ.type));
			break;
		case 'n':
			strncpy(targ.name, optarg, sizeof(targ.name));
			break;
		case 'd':
			targ.dir = atoi(optarg);
			break;
		case 'l':
			g_aec_loop_count = atoi(optarg);
			break;
		case 's':
			g_aec_run_msec = atoi(optarg);
			break;
		case 'r':
			g_aec_rate = atoi(optarg);
			break;
		case 'c':
			g_aec_channels = atoi(optarg);
			break;
		case 'b':
			g_aec_bits = atoi(optarg);
			break;
		case 'v':
			g_aec_verbose = atoi(optarg);
			break;
		case 'g':
			g_aec_cap = atoi(optarg);
			break;
		case 'f':
			g_aec_forward_port = atoi(optarg);
			break;
		case 'h':
		default:
			goto usage;
		}
	}

	printf("mode %d g_are_cap %d\n", mode, g_aec_cap);

	if ((mode != 0 && mode != 1) && (g_aec_cap == 0))
		goto usage;

	if (aec_check_dir(targ.dir) < 0)
		goto usage;

	printf("test start\n");

	if (mode == 0)
		do_rpdata_aec_send(&targ);
	else if (mode == 1)
		do_rpdata_aec_recv(&targ);

	if (g_aec_cap == 1)
		audio_echocancel_task_create(NULL);

	return 0;
usage:
	aec_demo_usage();
	return -1;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_aec_test, aec_test, omx test);

