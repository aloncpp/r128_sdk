/**
  utils/arender_test.c

  This simple test application take one input stream. Put the input to playback.

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

#include "arender_test.h"

static int g_rpd_verbose = 0;
static int g_are_bits = 0;
static int g_are_run_msec = 0;
static int g_are_loop_count = 0;
static int g_are_rate = 0;
static int g_are_channels = 0;
static int g_are_play = 0;
static int g_arender_forward_port = 0;

static char filename[32];

/* Application private date: should go in the component field (segs...) */
static OMX_U32 error_value = 0u;

OMX_CALLBACKTYPE arender_untunnel_callbacks = {
	.EventHandler = audiorenderEventHandler,
	.EmptyBufferDone = audiorenderEmptyBufferDone,
	.FillBufferDone = audiorenderFillBufferDone,
};

OMX_CALLBACKTYPE arecord_tunnel_callbacks = {
	.EventHandler = arecordEventHandler,
	.EmptyBufferDone = NULL,
	.FillBufferDone = NULL,
};

OMX_CALLBACKTYPE dump_tunnel_callbacks = {
	.EventHandler = adumpEventHandler,
	.EmptyBufferDone = NULL,
	.FillBufferDone = NULL,
};

OMX_CALLBACKTYPE arender_tunnel_callbacks = {
	.EventHandler = audiorenderEventHandler,
	.EmptyBufferDone = NULL,
	.FillBufferDone = NULL,
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
OMX_ERRORTYPE adumpEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	arenderPrivateType *private = (arenderPrivateType *)pAppData;
	if (hComponent == private->dump_handle)
		name = "dump";

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
			omx_sem_up(private->dump_eventSem);
		} else  if (Data1 == OMX_CommandPortEnable){
			printf("%s CmdComplete OMX_CommandPortEnable\n", name);
			omx_sem_up(private->dump_eventSem);
		} else if (Data1 == OMX_CommandPortDisable) {
			printf("%s CmdComplete OMX_CommandPortDisable\n", name);
			omx_sem_up(private->dump_eventSem);
		}
	} else if(eEvent == OMX_EventBufferFlag) {
		if ((int)Data2 == OMX_BUFFERFLAG_EOS) {
			printf("%s BufferFlag OMX_BUFFERFLAG_EOS\n", name);
			if (hComponent == private->dump_handle) {
				printf("end of tunnel");
				omx_sem_up(private->eofSem);
			}
		} else
			printf("%s OMX_EventBufferFlag %lx", name, Data2);
	} else if (eEvent == OMX_EventError) {
		error_value = Data1;
		printf("Receive error event. value:%lx", error_value);
		omx_sem_up(private->dump_eventSem);
	} else {
		printf("Param1 is %i\n", (int)Data1);
		printf("Param2 is %i\n", (int)Data2);
	}

	return OMX_ErrorNone;
}

/* Callbacks implementation */
OMX_ERRORTYPE arecordEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	arenderPrivateType *private = (arenderPrivateType *)pAppData;
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


/* Callbacks implementation */
OMX_ERRORTYPE audiorenderEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	arenderPrivateType *private = (arenderPrivateType *)pAppData;
	if (hComponent == private->arender_handle)
		name = "audiorender";

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
			omx_sem_up(private->arender_eventSem);
		} else  if (Data1 == OMX_CommandPortEnable){
			printf("%s CmdComplete OMX_CommandPortEnable\n", name);
			omx_sem_up(private->arender_eventSem);
		} else if (Data1 == OMX_CommandPortDisable) {
			printf("%s CmdComplete OMX_CommandPortDisable\n", name);
			omx_sem_up(private->arender_eventSem);
		}
	} else if(eEvent == OMX_EventBufferFlag) {
		if ((int)Data2 == OMX_BUFFERFLAG_EOS) {
			printf("%s BufferFlag OMX_BUFFERFLAG_EOS\n", name);
			if (hComponent == private->arender_handle) {
				printf("end of tunnel");
				omx_sem_up(private->eofSem);
			}
		} else
			printf("%s OMX_EventBufferFlag %lx", name, Data2);
	} else if (eEvent == OMX_EventError) {
		error_value = Data1;
		printf("Receive error event. value:%lx", error_value);
		omx_sem_up(private->arender_eventSem);
	} else {
		printf("Param1 is %i\n", (int)Data1);
		printf("Param2 is %i\n", (int)Data2);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE audiorenderEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	arenderPrivateType *private = (arenderPrivateType *)pAppData;

	if (pBuffer == NULL || pAppData == NULL) {
		printf("err: buffer header is null");
		return OMX_ErrorBadParameter;
	}

	printf("empty buffer done>> %p, %lu, input:%lu output:%lu", pBuffer->pBuffer, \
	pBuffer->nFlags, pBuffer->nInputPortIndex, pBuffer->nOutputPortIndex);

	err = queue(private->BufferQueue, pBuffer);
	if (err != OMX_ErrorNone)
		printf("queue buffer err: %d", err);

	return err;

}

OMX_ERRORTYPE audiorenderFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {


	printf("In the %s callback. Got buflen %i for buffer at 0x%p\n",
                          __func__, (int)pBuffer->nFilledLen, pBuffer);

	return OMX_ErrorNone;
}

static void arender_demo_usage(void)
{
	printf("Usgae: arender_demo [option]\n");
	printf("-h,          arender demo help\n");
	printf("-s,          set time(ms) for test\n");
	printf("-r,          set sample rate for test\n");
	printf("-c,          set channel num for test\n");
	printf("-b,          set bit width for test\n");
	printf("-g,          test rv audio render\n");
	printf("-p,          set filename to play\n");
	printf("\n");
	printf("arender_test -g 1 -p /data/test.wav -s 20 -r 16000 -c 2 -b 16 \n");
	printf("\n");
}

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

static OMX_ERRORTYPE alloc_buffer(arenderPrivateType *priv, OMX_S32 num, OMX_S32 size)
{
	OMX_S32 i = 0;
	OMX_S32 port_index = AUDIO_INPUT_PORT_INDEX;
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	priv->buffer = malloc(num * sizeof(OMX_BUFFERHEADERTYPE *));
	if (NULL == priv->buffer)
		return OMX_ErrorBadParameter;
	for (i = 0; i < num; i++) {
		priv->buffer[i] = NULL;
		ret = OMX_AllocateBuffer(priv->arender_handle, &priv->buffer[i],
				port_index, priv, size);
		printf("AllocateBuffer %p on port %ld\n", priv->buffer[i], port_index);
		if (ret != OMX_ErrorNone) {
			printf("Error on AllocateBuffer %p on port %ld\n",
				&priv->buffer[i], port_index);
			break;
		}
		ret = queue(priv->BufferQueue, priv->buffer[i]);
		if (ret != 0) {
			printf("queue buffer %ld error!\n", i);
			break;
		}
	}

	return ret;
}

static void free_buffer(arenderPrivateType *priv)
{
	OMX_S32 i = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (priv->buffer) {
		for (i = 0; i < priv->buf_num; i++) {
			if (priv->buffer[i]) {
				ret = OMX_FreeBuffer(priv->arender_handle,
						AUDIO_INPUT_PORT_INDEX,
						priv->buffer[i]);
				if (ret != OMX_ErrorNone)
					printf("port %d ,freebuffer:%ld failed",
						AUDIO_INPUT_PORT_INDEX, i);
			}
			priv->buffer[i] = NULL;
		}
		free(priv->buffer);
		priv->buffer = NULL;
	}
}

static int config_filter_component(arenderPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_OTHER_PARAM_DUMPTYPE dump_params;
	OMX_PORT_PARAM_TYPE sParam;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	int port_dump_in = -1;
	int port_dump_out = -1;

	if (g_are_bits == 0 || g_are_channels == 0 || g_are_rate == 0 || g_are_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = g_are_bits / 8 * g_are_channels;
	uint32_t len = frame_bytes * g_are_rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;

	/* set input port */
	port_dump_in = get_port_index(priv->dump_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	if (port_dump_in < 0) {
		printf("Error in get port index, port_dump_in %d\n", port_dump_in);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}

	memset(&priv->dump_port_para[port_dump_in], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->dump_port_para[port_dump_in].nBufferCountActual = DEFAULT_ARENDER_BUF_CNT;
	priv->dump_port_para[port_dump_in].bBuffersContiguous = 1;
	priv->dump_port_para[port_dump_in].eDomain = OMX_PortDomainAudio;
	priv->dump_port_para[port_dump_in].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->dump_port_para[port_dump_in].nPortIndex = port_dump_in;
	priv->dump_port_para[port_dump_in].nBufferSize = len;

	setHeader(&priv->dump_port_para[port_dump_in], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->dump_handle, OMX_IndexParamPortDefinition,
			&priv->dump_port_para[port_dump_in]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	/* set input port dump param */
	memset(&dump_params, 0, sizeof(OMX_OTHER_PARAM_DUMPTYPE));
	dump_params.nPortIndex = port_dump_in;

#if defined(CONFIG_ARCH_DSP)
	//strncpy((char *)dump_params.nRpdataType, priv->are_test->targ.type, sizeof(priv->are_test->targ.type));
	//strncpy((char *)dump_params.nRpdataName, priv->are_test->targ.name, sizeof(priv->are_test->targ.name));

#else
	if (g_arender_forward_port)
		dump_params.nForwardPort = g_arender_forward_port;
	else
		printf("g_rpd_forward_port is 0\n");

	if (strlen(priv->are_test->filename))
		memcpy(dump_params.nPathName, priv->are_test->filename, sizeof(priv->are_test->filename));
	else
		printf("filename is null!\n");
#endif
	setHeader(&dump_params, sizeof(OMX_OTHER_PARAM_DUMPTYPE));
	ret = OMX_SetParameter(priv->dump_handle, OMX_IndexVendorParamDump,
			&dump_params);
	if (ret) {
		printf("set dump params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_are_channels;
	audio_params.nBitPerSample = g_are_bits;
	audio_params.nSamplingRate = g_are_rate;
	audio_params.nPortIndex = port_dump_in;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->dump_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio render params error!");
		return ret;
	}

	/* set output port */
	port_dump_out = get_port_index(priv->dump_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_dump_out < 0) {
		printf("Error in get port index, port_dump_out %d\n", port_dump_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}
	memset(&priv->dump_port_para[port_dump_out], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->dump_port_para[port_dump_out].nBufferCountActual = DEFAULT_ARENDER_BUF_CNT;
	priv->dump_port_para[port_dump_out].bBuffersContiguous = 1;
	priv->dump_port_para[port_dump_out].eDomain = OMX_PortDomainAudio;
	priv->dump_port_para[port_dump_out].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->dump_port_para[port_dump_out].nBufferSize = len;
	priv->dump_port_para[port_dump_out].nPortIndex = port_dump_out;

	setHeader(&priv->dump_port_para[port_dump_out], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->dump_handle, OMX_IndexParamPortDefinition,
			&priv->dump_port_para[port_dump_out]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_are_channels;
	audio_params.nBitPerSample = g_are_bits;
	audio_params.nSamplingRate = g_are_rate;
	audio_params.nPortIndex = port_dump_out;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->dump_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio render params error!");
		return ret;
	}

	/** Get the number of ports */
	setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
	ret = OMX_GetParameter(priv->dump_handle, OMX_IndexParamAudioInit, &sParam);
	if(ret != OMX_ErrorNone){
		printf("Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		return ret;
	}
	printf("Dump has %d ports\n",(int)sParam.nPorts);

	return ret;
}

static int config_source_component(arenderPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	OMX_PORT_PARAM_TYPE sParam;
	int port_are_out = -1;

	if (g_are_bits == 0 || g_are_channels == 0 || g_are_rate == 0 || g_are_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = g_are_bits / 8 * g_are_channels;
	uint32_t len = frame_bytes * g_are_rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;

	port_are_out = get_port_index(priv->arecord_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_are_out < 0) {
		printf("Error in get port index, port_are_out %d\n", port_are_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}

	memset(&priv->arecord_port_para, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->arecord_port_para.nBufferCountActual = DEFAULT_ARENDER_BUF_CNT;
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
	audio_params.nChannels = g_are_channels;
	audio_params.nBitPerSample = g_are_bits;
	audio_params.nSamplingRate = g_are_rate;
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

static int config_sink_component(arenderPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	OMX_PORT_PARAM_TYPE sParam;
	int port_are_in = -1;

	if (g_are_bits == 0 || g_are_channels == 0 || g_are_rate == 0 || g_are_run_msec == 0)
			return OMX_ErrorBadParameter;

	int frame_bytes = g_are_bits / 8 * g_are_channels;
	uint32_t len = frame_bytes * g_are_rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;

	port_are_in = get_port_index(priv->arender_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	if (port_are_in < 0) {
		printf("Error in get port index, port_are_out %d\n", port_are_in);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}

	memset(&priv->arender_port_para, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->buf_num = DEFAULT_ARENDER_BUF_CNT;
	priv->arender_port_para.nBufferCountActual = priv->buf_num;
	priv->arender_port_para.bBuffersContiguous = 1;
	priv->arender_port_para.eDomain = OMX_PortDomainAudio;
	priv->arender_port_para.nPortIndex = port_are_in;
	priv->arender_port_para.nBufferSize = len;

	setHeader(&priv->arender_port_para, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->arender_handle, OMX_IndexParamPortDefinition,
			&priv->arender_port_para);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_are_channels;
	audio_params.nBitPerSample = g_are_bits;
	audio_params.nSamplingRate = g_are_rate;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->arender_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio render params error!");
		return ret;
	}

	/** Get the number of ports */
	setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
	ret = OMX_GetParameter(priv->arender_handle, OMX_IndexParamAudioInit, &sParam);
	if(ret != OMX_ErrorNone){
		printf("Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		return ret;
	}
	printf("Audio render has %d ports\n",(int)sParam.nPorts);

	return ret;
}

static int untunnel_config_sink_component(arenderPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	OMX_PORT_PARAM_TYPE sParam;
	struct WAVE_FORMAT_DEF *wave_fmt;
	wave_fmt = &priv->audio_info.fmt_block.wav_format;

	if (wave_fmt->bits_per_sample == 0 || wave_fmt->channels == 0 || wave_fmt->samples_per_sec == 0 || g_are_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = wave_fmt->bits_per_sample / 8 * wave_fmt->channels;
	uint32_t len = frame_bytes * wave_fmt->samples_per_sec / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;

	printf("frame_bytes %d, bit %d  ch %d sample rate %d\n", \
		frame_bytes ,wave_fmt->bits_per_sample, wave_fmt->channels, wave_fmt->samples_per_sec);

	memset(&priv->arender_port_para, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->buf_num = DEFAULT_ARENDER_BUF_CNT;
	priv->arender_port_para.nBufferCountActual = priv->buf_num;
	priv->arender_port_para.bBuffersContiguous = 1;
	priv->arender_port_para.eDomain = OMX_PortDomainAudio;
	priv->arender_port_para.nBufferSize = len;

	setHeader(&priv->arender_port_para, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->arender_handle, OMX_IndexParamPortDefinition,
			&priv->arender_port_para);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = wave_fmt->channels;
	audio_params.nBitPerSample = wave_fmt->bits_per_sample;
	audio_params.nSamplingRate = wave_fmt->samples_per_sec;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->arender_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio render params error!");
		return ret;
	}

	/** Get the number of ports */
	setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
	ret = OMX_GetParameter(priv->arender_handle, OMX_IndexParamAudioInit, &sParam);
	if(ret != OMX_ErrorNone){
		printf("Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		return ret;
	}
	printf("Audio render has %d ports\n",(int)sParam.nPorts);

	return ret;
}

static void wav_read_thread(void *params)
{
	OMX_BUFFERHEADERTYPE *buffer;
	arenderPrivateType *priv = (arenderPrivateType *)params;
	int rlen;
	int ret = OMX_ErrorNone;
	OMX_BOOL eof = OMX_FALSE;

	while (1) {
		if (eof) {
			printf("exit read loop!\n");
			break;
		}
		/* dequeue one buffer */
		buffer = dequeue(priv->BufferQueue);
		if (buffer == NULL)
			continue;

		/* read wav file */
		rlen = fread(buffer->pBuffer, 1, buffer->nAllocLen, priv->infile);
		printf("read  len %d\n", rlen);
		if (rlen <= 0) {
			printf("read file error rlen %d!\n", rlen);
			eof = OMX_TRUE;
			buffer->nFilledLen = 0;
			buffer->nOffset = 0;
			buffer->nFlags = OMX_BUFFERFLAG_EOS;
		} else {
			buffer->nFilledLen = rlen;
			buffer->nOffset = 0;
			buffer->nFlags = 0;
		}
		buffer->nInputPortIndex = AUDIO_INPUT_PORT_INDEX;
		ret = OMX_EmptyThisBuffer(priv->arender_handle, buffer);
		if (ret != OMX_ErrorNone) {
			printf("empty this buffer error!\n");
			break;
		}
	}
	printf("end of wave read thread\n");
	vTaskDelete(NULL);
}

static void arender_task(void *arg)
{
	arender_test_data *are_test = (arender_test_data *)arg;
	int ret = -1;
	arenderPrivateType *priv = NULL;
	TaskHandle_t handle;
	int rate = are_test->rate;
	int bits = are_test->bits;
	int channels = are_test->channels;
	int frame_bytes = bits / 8 * channels;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;

	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;

	/* Initialize application private data */
	priv = malloc(sizeof(arenderPrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	memset(priv, 0 ,sizeof(arenderPrivateType));

	priv->BufferQueue = malloc(sizeof(queue_t));
	if(priv->BufferQueue == NULL) {
		printf("Insufficient memory in %s\n", __func__);
		goto exit;
	}
	memset(priv->BufferQueue, 0, sizeof(queue_t));

	ret = queue_init(priv->BufferQueue);
	if (ret != 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	priv->infile = fopen(are_test->filename, "rb");
	if (!priv->infile) {
		printf("open file %s error!\n", are_test->filename);
		goto exit;
	}

	if (fread(&(priv->audio_info.header),
		sizeof(struct RIFF_HEADER_DEF), 1, priv->infile) != 1) {
		printf("read header info error");
		goto exit;
	}

	if (fread(&(priv->audio_info.fmt_block),
		sizeof(struct FMT_BLOCK_DEF), 1, priv->infile) != 1) {
		printf("read fmt_block info error");
		goto exit;
	}

	if (fread(&(priv->audio_info.data_block),
		sizeof(struct DATA_BLOCK_DEF), 1, priv->infile) != 1) {
		printf("read data_block info error");
		goto exit;
	}

	priv->arender_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->arender_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->arender_eventSem, 0);
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
	err = OMX_GetHandle(&priv->arender_handle, "OMX.audio.render", priv, &arender_untunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio render OMX_GetHandle failed\n");
		goto exit;
	}

	/* 2. config component */
	err = untunnel_config_sink_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in untunnel_config_source_component\n");
		goto exit;
	}

	/* 3. set component stat to idle */
	err = OMX_SendCommand(priv->arender_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	port_def.nPortIndex = AUDIO_INPUT_PORT_INDEX;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->arender_handle, OMX_IndexParamPortDefinition, &port_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}
	if ((priv->arender_port_para.nBufferSize != port_def.nBufferSize) ||
		(priv->arender_port_para.nBufferCountActual != port_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize%ld nBufferCountActual %ld\n", priv->arender_port_para.nBufferSize, priv->arender_port_para.nBufferCountActual , \
			port_def.nBufferSize, port_def.nBufferCountActual);
		goto exit;
	}

	err = alloc_buffer(priv, port_def.nBufferCountActual, port_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}
	omx_sem_down(priv->arender_eventSem);


	/* 4. set component stat to executing */
	err = OMX_SendCommand(priv->arender_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->arender_eventSem);

	/* start wav read thread */
	xTaskCreate(wav_read_thread, "wav_read_thread", 2048, (void *)priv,
				configAPPLICATION_OMX_PRIORITY,
				&handle);

	//hal_msleep(500);

	printf("audio render test wait for eos\n");

	omx_sem_down(priv->eofSem);

	printf("audio render test get eos signal\n");

	/* 5. set component stat to idle */
	ret = OMX_SendCommand(priv->arender_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->arender_eventSem);

	/* 6. free buffer */
	free_buffer(priv);

	/* 7. set component stat to loaded */
	ret = OMX_SendCommand(priv->arender_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}
	omx_sem_down(priv->arender_eventSem);

exit:

	free_buffer(priv);

	if (priv->infile) {
		fclose(priv->infile);
		priv->infile = NULL;
	}
	if (priv->arender_eventSem) {
		omx_sem_deinit(priv->arender_eventSem);
		free(priv->arender_eventSem);
	}
	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
	}
	if (priv->arender_handle) {
		OMX_FreeHandle(priv->arender_handle);
		priv->arender_handle = NULL;
	}
	if (priv->BufferQueue) {
		queue_deinit(priv->BufferQueue);
		free(priv->BufferQueue);
		priv->BufferQueue = NULL;
	}
	if (priv) {
		free(priv);
		priv = NULL;
	}
	if (are_test) {
		free(are_test);
		are_test = NULL;
	}
	g_are_play = 0;
	memset(filename, 0 , sizeof(filename));
	printf("arender task test finish\n");
	vTaskDelete(NULL);
}

static int audio_render_task_create(struct arender_test_data *data)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	arender_test_data *are_test;

	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	are_test = malloc(sizeof(arender_test_data));
	if (!are_test)
	{
		printf("malloc are_test failed\n");
		return -1;
	}
	memset(are_test, 0 , sizeof(arender_test_data));

	if (!data) {
		are_test->rate = g_are_rate;
		are_test->channels = g_are_channels;
		are_test->bits = g_are_bits;
		are_test->msec = g_are_run_msec;
		memcpy(are_test->filename, filename, sizeof(are_test->filename));
	} else {
		memcpy(are_test, data, sizeof(struct arender_test_data));
	}

	xTaskCreate(arender_task, "arender_task", 2048, are_test,
			configAPPLICATION_OMX_PRIORITY,
			&handle);
	return 0;
}

static void audio_record_dump_render_task(void *arg)
{
	arender_test_data *are_test = (arender_test_data *)arg;
	int ret = -1;
	arenderPrivateType *priv = NULL;
	int rate = are_test->rate;
	int bits = are_test->bits;
	int channels = are_test->channels;
	int frame_bytes = bits / 8 * channels;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;
	int port_src_out = -1;
	int port_filter_in = -1;
	int port_filter_out = -1;
	int port_sink_in = -1;
	OMX_ERRORTYPE err = OMX_ErrorNone;

	/* Initialize application private data */
	priv = malloc(sizeof(arenderPrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	memset(priv, 0 ,sizeof(arenderPrivateType));

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

	priv->arender_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->arender_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->arender_eventSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	priv->dump_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->dump_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->dump_eventSem, 0);
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

	 priv->are_test = are_test;

	 /* 1. get component handle */
	err = OMX_GetHandle(&priv->arecord_handle, "OMX.audio.record", priv, &arecord_tunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio render OMX_GetHandle failed\n");
		goto exit;
	}
	err = OMX_GetHandle(&priv->dump_handle, "OMX.dump", priv, &dump_tunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio render OMX_GetHandle failed\n");
		goto exit;
	}
	err = OMX_GetHandle(&priv->arender_handle, "OMX.audio.render", priv, &arender_tunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio render OMX_GetHandle failed\n");
		goto exit;
	}

	/* 2. config component */
	err = config_source_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in config_source_component\n");
		goto exit;
	}
	err = config_filter_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in config_filter_component\n");
		goto exit;
	}
	err = config_sink_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in config_sink_component\n");
		goto exit;
	}

	/* 3. set tunnel */
	port_src_out = get_port_index(priv->arecord_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	port_filter_in = get_port_index(priv->dump_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	if (port_src_out < 0 || port_filter_in < 0) {
		printf("Error in get port index, port_src_out %d port_filter_in %d\n", port_src_out, port_filter_in);
		goto exit;
	}


	err = OMX_SetupTunnel(priv->arecord_handle, port_src_out, priv->dump_handle, port_filter_in);
	if(err != OMX_ErrorNone) {
		printf("Set up Tunnel between audio record & audio dump Failed\n");
		goto exit;
	}

	port_filter_out = get_port_index(priv->dump_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	port_sink_in = get_port_index(priv->arender_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	if (port_filter_out < 0 || port_sink_in < 0) {
		printf("Error in get port index, port_src_out %d port_filter_in %d\n", port_src_out, port_filter_in);
		goto exit;
	}

	err = OMX_SetupTunnel(priv->dump_handle, port_filter_out, priv->arender_handle, port_sink_in);
	if(err != OMX_ErrorNone) {
		printf("Set up Tunnel between dump & audio render Failed\n");
		goto exit;
	}

	/* 4. set component stat to idle */

	err = OMX_SendCommand(priv->arender_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	err = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
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

	omx_sem_down(priv->dump_eventSem);

	omx_sem_down(priv->arender_eventSem);

	/* 5. set component stat to executing */
	err = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->arecord_eventSem);

	err = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->dump_eventSem);

	err = OMX_SendCommand(priv->arender_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->arender_eventSem);

	printf("audio render test wait for eos\n");

	while (are_test->loop_count--) {
		hal_msleep(g_are_run_msec);
	}

	printf("audio render test get eos signal\n");

	/* 6. set component stat to idle */
	ret = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->arecord_eventSem);

	ret = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->dump_eventSem);

	ret = OMX_SendCommand(priv->arender_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->arender_eventSem);

	/* 7. set component stat to loaded */
	ret = OMX_SendCommand(priv->arender_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}
	ret = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
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

	omx_sem_down(priv->dump_eventSem);

	omx_sem_down(priv->arender_eventSem);

exit:

	printf("audio_record_dump_render_task test finish start\n");

	if (priv->arecord_eventSem) {
		omx_sem_deinit(priv->arecord_eventSem);
		free(priv->arecord_eventSem);
	}

	if (priv->dump_eventSem) {
		omx_sem_deinit(priv->dump_eventSem);
		free(priv->dump_eventSem);
	}

	if (priv->arender_eventSem) {
		omx_sem_deinit(priv->arender_eventSem);
		free(priv->arender_eventSem);
	}

	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
	}
	if (priv->arecord_handle) {
		OMX_FreeHandle(priv->arecord_handle);
		priv->arecord_handle = NULL;
	}

	if (priv->dump_handle) {
		OMX_FreeHandle(priv->dump_handle);
		priv->dump_handle = NULL;
	}
	if (priv->arender_handle) {
		OMX_FreeHandle(priv->arender_handle);
		priv->arender_handle = NULL;
	}
	if (priv) {
		free(priv);
		priv = NULL;
	}
	if (are_test) {
		free(are_test);
		are_test = NULL;
	}
	g_are_play = 0;
	g_arender_forward_port = 0;
	printf("audio_record_dump_render_task test finish end\n");
	vTaskDelete(NULL);
}

static int audio_record_dump_render_task_create(struct arender_test_data *data)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	arender_test_data *are_test;

	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	are_test = malloc(sizeof(arender_test_data));
	if (!are_test)
	{
		printf("malloc are_test failed\n");
		return -1;
	}
	memset(are_test, 0 , sizeof(arender_test_data));

	if (!data) {
		are_test->rate = g_are_rate;
		are_test->channels = g_are_channels;
		are_test->bits = g_are_bits;
		are_test->msec = g_are_run_msec;
		are_test->loop_count = g_are_loop_count;
		memcpy(are_test->filename, filename, sizeof(are_test->filename));
	} else {
		memcpy(are_test, data, sizeof(struct arender_test_data));
	}

	xTaskCreate(audio_record_dump_render_task, "audio_record_dump_render_task", 4096, are_test,
			configAPPLICATION_OMX_PRIORITY,
			&handle);
	return 0;
}

static int cmd_arender_test(int argc, char *argv[])
{
	int c;

	g_are_loop_count = 1000;
	g_are_run_msec = 20;  //ms
	g_are_rate = TEST_RATE;
	g_are_channels = TEST_CHANNELS;
	g_are_bits = TEST_BIT_WIDTH;

	optind = 0;
	while ((c = getopt(argc, argv, "hs:r:c:b:g:p:l:f:v:")) != -1) {
		switch (c) {
		case 's':
			g_are_run_msec = atoi(optarg);
			break;
		case 'r':
			g_are_rate = atoi(optarg);
			break;
		case 'c':
			g_are_channels = atoi(optarg);
			break;
		case 'b':
			g_are_bits = atoi(optarg);
			break;
		case 'g':
			g_are_play = atoi(optarg);
			break;
		case 'p':
			memcpy(filename, optarg, sizeof(filename));
			break;
		case 'l':
			g_are_loop_count = atoi(optarg);
			break;
		case 'f':
			g_arender_forward_port = atoi(optarg);
			break;
		case 'v':
			g_rpd_verbose = atoi(optarg);
			break;
		case 'h':
		default:
			goto usage;
		}
	}

	printf("g_are_play %d filename %s\n", g_are_play, filename);

	if (g_are_play == 0)
		goto usage;

	printf("test start\n");

	switch (g_are_play) {

		case 1:
			audio_render_task_create(NULL);
			break;
		case 2:
			audio_record_dump_render_task_create(NULL);
			break;
		default:
			printf("unknown 's' command\n");
			break;
	}

	return 0;
usage:
	arender_demo_usage();
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_arender_test, arender_test, omx test);

