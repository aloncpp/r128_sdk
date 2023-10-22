/**
  utils/arecord_test.c

  This simple test application take one output stream. Stores the output in another
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

#include "arecord_test.h"


#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
#include <adb_forward.h>
#endif

static int g_rpd_verbose = 0;
static int g_rpd_forward_port = 0;
static int g_are_bits = 0;
static int g_are_loop_count = 0;
static int g_are_run_msec = 0;
static int g_are_rate = 0;
static int g_are_channels = 0;
static int g_are_cap = 0;

static char are_filename[32];
static FILE *infile = NULL;

static OMX_U32 error_value = 0u;

OMX_CALLBACKTYPE arecord_untunnel_callbacks = {
	.EventHandler = audiorecordEventHandler,
	.EmptyBufferDone = audiorecordEmptyBufferDone,
	.FillBufferDone = audiorecordFillBufferDone,
};

OMX_CALLBACKTYPE arecord_callbacks = {
	.EventHandler = audiorecordEventHandler,
	.EmptyBufferDone = audiorecordEmptyBufferDone,
	.FillBufferDone = arecordFillBufferDone,
};

OMX_CALLBACKTYPE dump_callbacks = {
	.EventHandler = dumpEventHandler,
	.EmptyBufferDone = dumpEmptyBufferDone,
	.FillBufferDone = dumpFillBufferDone,
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
OMX_ERRORTYPE audiorecordEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	arePrivateType *private = (arePrivateType *)pAppData;
	if (hComponent == private->handle)
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
			omx_sem_up(private->eventSem);
		} else  if (Data1 == OMX_CommandPortEnable){
			printf("%s CmdComplete OMX_CommandPortEnable\n", name);
			omx_sem_up(private->eventSem);
		} else if (Data1 == OMX_CommandPortDisable) {
			printf("%s CmdComplete OMX_CommandPortDisable\n", name);
			omx_sem_up(private->eventSem);
		}
	} else if(eEvent == OMX_EventBufferFlag) {
		if ((int)Data2 == OMX_BUFFERFLAG_EOS) {
			printf("%s BufferFlag OMX_BUFFERFLAG_EOS\n", name);
			if (hComponent == private->handle) {
				printf("end of tunnel");
				omx_sem_up(private->eofSem);
			}
		} else
			printf("%s OMX_EventBufferFlag %lx", name, Data2);
	} else if (eEvent == OMX_EventError) {
		error_value = Data1;
		printf("Receive error event. value:%lx", error_value);
		omx_sem_up(private->eventSem);
	} else {
		printf("Param1 is %i\n", (int)Data1);
		printf("Param2 is %i\n", (int)Data2);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE audiorecordEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {
  
  printf("In the %s callback from the port %i\n", __func__, (int)pBuffer->nInputPortIndex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE audiorecordFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	int ret;
	arePrivateType *private = (arePrivateType *)pAppData;

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
	err = OMX_FillThisBuffer(private->handle, pBuffer);
	if (err != OMX_ErrorNone)
		printf("OMX_FillThisBuffer err: %x", err);

  return err;
}


OMX_ERRORTYPE arecordFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	arePrivateType *private = (arePrivateType *)pAppData;

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
		if (g_rpd_forward_port > 0)
			adb_forward_send(g_rpd_forward_port, pBuffer->pBuffer, pBuffer->nFilledLen);
		else {
			printf("[%s] line:%d g_rpd_forward_port %d err \n", __func__, __LINE__, g_rpd_forward_port);
		}
#endif
	}

	/* Output data to standard output */
	pBuffer->nFilledLen = 0;
	err = OMX_FillThisBuffer(private->handle, pBuffer);
	if (err != OMX_ErrorNone)
		printf("OMX_FillThisBuffer err: %x", err);

  return err;
}

/* Callbacks implementation */
OMX_ERRORTYPE dumpEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	arePrivateType *private = (arePrivateType *)pAppData;
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

OMX_ERRORTYPE dumpEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

  printf("In the %s callback from the port %i\n", __func__, (int)pBuffer->nInputPortIndex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE dumpFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	arePrivateType *private = (arePrivateType *)pAppData;

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

	/* Output data to standard output */
	pBuffer->nFilledLen = 0;
	err = OMX_FillThisBuffer(private->dump_handle, pBuffer);
	if (err != OMX_ErrorNone)
	  printf("OMX_FillThisBuffer err: %x", err);

  return err;
}

static void arecord_demo_usage(void)
{
	printf("Usgae: aec_demo [option]\n");
	printf("-h,          rpdata help\n");
	printf("-m,          mode, 0-send; 1-recv;\n");
	printf("-t,          type, type name\n");
	printf("-n,          name, id name\n");
	printf("             (type + name) specify unique data xfer\n");
	printf("-d,          dir, remote processor, 1-cm33;2-c906;3-dsp\n");
	printf("-l,          set loopcount for test\n");
	printf("-s,          set time(ms) for test\n");
	printf("-r,          set sample rate for test\n");
	printf("-c,          set channel num for test\n");
	printf("-b,          set bit width for test\n");
	printf("-g,          test rv audio record\n");
	printf("\n");
	printf("DSP -> RV\n");
	printf("rpccli dsp rpdata_audio -m 0 -d 2 -t DSPtoRVAudio -n RVrecvDSPsend\n");
	printf("rpdata_audio -m 1 -d 3 -t DSPtoRVAudio -n RVrecvDSPsend\n");
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

static OMX_ERRORTYPE alloc_buffer(arePrivateType *priv, OMX_S32 num, OMX_S32 size)
{
	OMX_S32 i = 0;
	OMX_S32 port_index = AUDIO_OUTPUT_PORT_INDEX;
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	priv->buffer = malloc(num * sizeof(OMX_BUFFERHEADERTYPE *));
	if (NULL == priv->buffer)
		return OMX_ErrorBadParameter;
	for (i = 0; i < num; i++) {
		priv->buffer[i] = NULL;
		ret = OMX_AllocateBuffer(priv->handle, &priv->buffer[i],
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

static OMX_ERRORTYPE alloc_dump_buffer(arePrivateType *priv, OMX_S32 num, OMX_S32 size)
{
	OMX_S32 i = 0;
	OMX_S32 port_index = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int port_dump_out = -1;

	port_dump_out = get_port_index(priv->dump_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_dump_out < 0) {
		printf("Error in get port index, port_aec_out %d\n", port_dump_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}
	port_index = port_dump_out;

	priv->buffer = malloc(num * sizeof(OMX_BUFFERHEADERTYPE *));
	if (NULL == priv->buffer)
		return OMX_ErrorBadParameter;
	for (i = 0; i < num; i++) {
		priv->buffer[i] = NULL;
		ret = OMX_AllocateBuffer(priv->dump_handle, &priv->buffer[i],
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

static void free_dump_buffer(arePrivateType *priv)
{
	OMX_S32 i = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int port_dump_out = -1;
	OMX_S32 port_index = 0;

	port_dump_out = get_port_index(priv->dump_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_dump_out < 0) {
		printf("Error in get port index, port_aec_out %d\n", port_dump_out);
		return;
	}
	port_index = port_dump_out;

	if (priv->buffer) {
		for (i = 0; i < priv->buf_num; i++) {
			if (priv->buffer[i]) {
				ret = OMX_FreeBuffer(priv->dump_handle,
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

static void free_buffer(arePrivateType *priv)
{
	OMX_S32 i = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (priv->buffer) {
		for (i = 0; i < priv->buf_num; i++) {
			if (priv->buffer[i]) {
				ret = OMX_FreeBuffer(priv->handle,
						AUDIO_OUTPUT_PORT_INDEX,
						priv->buffer[i]);
				if (ret != OMX_ErrorNone)
					printf("port %d ,freebuffer:%ld failed",
						AUDIO_OUTPUT_PORT_INDEX, i);
			}
			priv->buffer[i] = NULL;
		}
		free(priv->buffer);
		priv->buffer = NULL;
	}
}

static int config_dump_component(arePrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_OTHER_PARAM_DUMPTYPE dump_params;
	OMX_PORT_PARAM_TYPE sParam;
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
	priv->dump_port_para[port_dump_in].nBufferCountActual = DEFAULT_AREC_BUF_CNT;
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
	strncpy((char *)dump_params.nRpdataType, priv->are_test->targ.type, sizeof(priv->are_test->targ.type));
	strncpy((char *)dump_params.nRpdataName, priv->are_test->targ.name, sizeof(priv->are_test->targ.name));

#else
	if (g_rpd_forward_port)
		dump_params.nForwardPort = g_rpd_forward_port;
	else
		printf("g_rpd_forward_port is 0\n");

	if (priv->are_test->filename)
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

	/* set output port */
	port_dump_out = get_port_index(priv->dump_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_dump_out < 0) {
		printf("Error in get port index, port_dump_out %d\n", port_dump_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}
	memset(&priv->dump_port_para[port_dump_out], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->dump_port_para[port_dump_out].nBufferCountActual = DEFAULT_AREC_BUF_CNT;
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

static int config_source_component(arePrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	OMX_PORT_PARAM_TYPE sParam;

	if (g_are_bits == 0 || g_are_channels == 0 || g_are_rate == 0 || g_are_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = g_are_bits / 8 * g_are_channels;
	uint32_t len = frame_bytes * g_are_rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;

	memset(&priv->port_para, 0, sizeof(priv->port_para));
	priv->buf_num = DEFAULT_AREC_BUF_CNT;
	priv->port_para.nBufferCountActual = priv->buf_num;
	priv->port_para.bBuffersContiguous = 1;
	priv->port_para.eDomain = OMX_PortDomainAudio;
	priv->port_para.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->port_para.nBufferSize = len;

	setHeader(&priv->port_para, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->handle, OMX_IndexParamPortDefinition,
			&priv->port_para);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_are_channels;
	audio_params.nBitPerSample = g_are_bits;
	audio_params.nSamplingRate = g_are_rate;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio record params error!");
		return ret;
	}

	/** Get the number of ports */
	setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
	ret = OMX_GetParameter(priv->handle, OMX_IndexParamAudioInit, &sParam);
	if(ret != OMX_ErrorNone){
		printf("Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		return ret;
	}
	printf("Audio record has %d ports\n",(int)sParam.nPorts);

	return ret;
}

static void rpdata_arecord_send(void *arg)
{
	are_test_data *are_test = (are_test_data *)arg;
	rpdata_t *rpd;
	void *buffer = NULL;
	int ret = -1;
	arePrivateType* priv;
	int i = 0;
	int rate = are_test->rate;
	int bits = are_test->bits;
	int channels = are_test->channels;
	int frame_bytes = bits / 8 * channels;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;
#if 0
	uint32_t rpdata_len = bits / 8 * 1 * rate / 100; /* data after processing(aec, mono ch) */
#else
	uint32_t rpdata_len = len ;
#endif
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;

	printf("dir:%d, type:%s, name:%s\n",
		are_test->targ.dir, are_test->targ.type, are_test->targ.name);

	rpd = rpdata_create(are_test->targ.dir, are_test->targ.type, are_test->targ.name, rpdata_len);
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
	priv = malloc(sizeof(arePrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	priv->eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->eventSem, 0);
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
	err = OMX_GetHandle(&priv->handle, "OMX.audio.record", priv, &arecord_untunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio record OMX_GetHandle failed\n");
		goto exit;
	}

	/* 2. config component */
	err = config_source_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in config_source_component\n");
		goto exit;
	}

	/* 3. set component stat to idle */
	err = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	port_def.nPortIndex = AUDIO_OUTPUT_PORT_INDEX;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->handle, OMX_IndexParamPortDefinition, &port_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}
	if ((priv->port_para.nBufferSize != port_def.nBufferSize) ||
		(priv->port_para.nBufferCountActual != port_def.nBufferCountActual)) {
		printf("Error when getting OMX_PORT_PARAM_TYPE");
		goto exit;
	}

	err = alloc_buffer(priv, port_def.nBufferCountActual, port_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}
	omx_sem_down(priv->eventSem);


	/* 4. set component stat to executing */
	err = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->eventSem);

	rpdata_wait_connect(rpd);

	/* 5. send buffer to source component queue */
	for (i = 0; i < priv->buf_num; i++) {
		err = OMX_FillThisBuffer(priv->handle, priv->buffer[i]);
		printf("OMX_FillThisBuffer %p on port %lu\n", priv->buffer[i],
			priv->buffer[i]->nOutputPortIndex);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_FillThisBuffer\n");
			goto exit;
		}
	}

	printf("audio record test start\n");

	while (are_test->loop_count--) {
		hal_msleep(g_are_run_msec);
	}

	printf("audio record test end\n");

	/* 6. set component stat to idle */
	ret = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->eventSem);

	/* 7. free buffer */
	free_buffer(priv);

	/* 8. set component stat to loaded */
	ret = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}
	omx_sem_down(priv->eventSem);

exit:

	free_buffer(priv);

	if (priv->eventSem) {
		omx_sem_deinit(priv->eventSem);
		free(priv->eventSem);
	}
	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
	}
	if (priv->handle) {
		OMX_FreeHandle(priv->handle);
		priv->handle = NULL;
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
	if (are_test) {
		free(are_test);
		are_test = NULL;
	}
	g_rpd_forward_port = 0;
	printf("rpdata audio record send test finish\n");
	vTaskDelete(NULL);
}

static int do_rpdata_arecord_send(rpdata_arg_arecord *targ)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	are_test_data *are_test;

	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	are_test = malloc(sizeof(are_test_data));
	if (are_test) {
		memset(are_test, 0 , sizeof(are_test_data));
		memcpy(&are_test->targ, targ, sizeof(rpdata_arg_arecord));
		are_test->rate = g_are_rate;
		are_test->channels = g_are_channels;
		are_test->bits = g_are_bits;
		are_test->loop_count = g_are_loop_count;
		are_test->msec = g_are_run_msec;
	} else {
		printf("malloc are_test failed\n");
		return -1;
	}

	xTaskCreate(rpdata_arecord_send, "rpd_arecord_send", 2048, are_test,
			configAPPLICATION_OMX_PRIORITY, &handle);
	hal_msleep(500);
	return 0;
}
#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
static int rpd_audio_record_forward_cb(rpdata_t *rpd, void *data, unsigned int data_len)
{
	int ret;
	
	if (!infile && !g_rpd_forward_port) {
		printf("[%s] line:%d infile is null and g_rpd_forward_port is 0\n", __func__, __LINE__);
		return 0;
	}

	if (g_rpd_forward_port)
		ret = adb_forward_send(g_rpd_forward_port, data, data_len);

	if (infile) {
		ret = fwrite(data, 1, data_len, infile);
		if (ret != data_len)
			printf("[%s] line:%d fwrite err ! ret=%d, data_len=%d\n", __func__, __LINE__, ret, data_len);
	}

	/*printf("[%s] line:%d ret=%d, data_len=%d\n", __func__, __LINE__, ret, data_len);*/
	return 0;
}

struct rpdata_cbs rpd_audio_record_forward_cbs = {
	.recv_cb = rpd_audio_record_forward_cb,
};
#endif

static int rpd_audio_record_recv_cb(rpdata_t *rpd, void *data, uint32_t data_len)
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
	if (!g_rpd_verbose)
		return 0;
	if (count++ % g_rpd_verbose == 0)
		printf("audio data check ok(print interval %d)\n", g_rpd_verbose);
	return 0;
}

struct rpdata_cbs rpd_audio_record_cbs = {
	.recv_cb = rpd_audio_record_recv_cb,
};

static void rpdata_arecord_recv(void *arg)
{
	rpdata_arg_arecord targ;
	rpdata_t *rpd = NULL;
	void *buffer = NULL;

	if (!strlen(are_filename) && !g_rpd_forward_port) {
		printf("[%s] line:%d infile and g_rpd_forward_port is both null\n", __func__, __LINE__);
		goto exit;
	}

	memcpy(&targ, arg, sizeof(rpdata_arg_arecord));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if(g_rpd_forward_port) {
		if (adb_forward_create_with_rawdata(g_rpd_forward_port) < 0) {
			printf("[%s] line:%d \n", __func__, __LINE__);
			goto exit;
		}
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

	if (strlen(are_filename)) {
		infile = fopen(are_filename, "wb");
		if (!infile) {
			printf("open file %s error!\n", are_filename);
			goto exit;
		}
	}

#if 1

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	rpdata_set_recv_cb(rpd, &rpd_audio_record_forward_cbs);
#else
	rpdata_set_recv_cb(rpd, &rpd_audio_record_cbs);
#endif
	while (g_are_loop_count--) {
		hal_msleep(g_are_run_msec + 2);
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
		ret = adb_forward_send(g_rpd_forward_port, recv_buf, len);
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

	if (infile)
		fflush(infile);

	if (infile)
		fclose(infile);
	
	infile = NULL;

	memset(are_filename, 0 , sizeof(are_filename));

	if (rpd)
		rpdata_destroy(rpd);

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if (g_rpd_forward_port > 0)
		adb_forward_end(g_rpd_forward_port);
#endif
	g_rpd_forward_port = 0;

	printf("rpdata audio record recv test finish\n");
	vTaskDelete(NULL);
}

static int do_rpdata_arecord_recv(rpdata_arg_arecord *targ)
{
	TaskHandle_t handle;

	xTaskCreate(rpdata_arecord_recv, "rpd_arecord_recv", 1024, targ,
			configAPPLICATION_OMX_PRIORITY, &handle);
	hal_msleep(500);
	return 0;
}

static int arecord_check_dir(int dir)
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

static void arecord_task(void *arg)
{
	are_test_data *are_test = (are_test_data *)arg;
	int ret = -1;
	arePrivateType* priv;
	int i = 0;
	int rate = are_test->rate;
	int bits = are_test->bits;
	int channels = are_test->channels;
	int frame_bytes = bits / 8 * channels;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;

	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if (adb_forward_create_with_rawdata(g_rpd_forward_port) < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
#endif

	/* Initialize application private data */
	priv = malloc(sizeof(arePrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	priv->eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->eventSem, 0);
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
	err = OMX_GetHandle(&priv->handle, "OMX.audio.record", priv, &arecord_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio record OMX_GetHandle failed\n");
		goto exit;
	}

	/* 2. config component */
	err = config_source_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in config_source_component\n");
		goto exit;
	}

	/* 3. set component stat to idle */
	err = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	port_def.nPortIndex = AUDIO_OUTPUT_PORT_INDEX;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->handle, OMX_IndexParamPortDefinition, &port_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}
	if ((priv->port_para.nBufferSize != port_def.nBufferSize) ||
		(priv->port_para.nBufferCountActual != port_def.nBufferCountActual)) {
		printf("Error when getting OMX_PORT_PARAM_TYPE");
		goto exit;
	}

	err = alloc_buffer(priv, port_def.nBufferCountActual, port_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}
	omx_sem_down(priv->eventSem);


	/* 4. set component stat to executing */
	err = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->eventSem);

	/* 5. send buffer to source component queue */
	for (i = 0; i < priv->buf_num; i++) {
		err = OMX_FillThisBuffer(priv->handle, priv->buffer[i]);
		printf("OMX_FillThisBuffer %p on port %lu\n", priv->buffer[i],
			priv->buffer[i]->nOutputPortIndex);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_FillThisBuffer\n");
			goto exit;
		}
	}

	printf("audio record test start\n");

	while (are_test->loop_count--) {
		hal_msleep(g_are_run_msec);
	}

	printf("audio record test end\n");

	/* 6. set component stat to idle */
	ret = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->eventSem);

	/* 7. free buffer */
	free_buffer(priv);

	/* 8. set component stat to loaded */
	ret = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}
	omx_sem_down(priv->eventSem);

exit:

	free_buffer(priv);

	if (priv->eventSem) {
		omx_sem_deinit(priv->eventSem);
		free(priv->eventSem);
	}
	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
	}
	if (priv->handle) {
		OMX_FreeHandle(priv->handle);
		priv->handle = NULL;
	}
	
#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if (g_rpd_forward_port > 0)
		adb_forward_end(g_rpd_forward_port);
#endif

	OMX_Deinit();

	if (priv) {
		free(priv);
		priv = NULL;
	}
	if (are_test) {
		free(are_test);
		are_test = NULL;
	}
	g_are_cap = 0;
	g_rpd_forward_port = 0;
	printf("arecord task test finish\n");
	vTaskDelete(NULL);
}

static int audio_record_task_create(struct are_test_data *data)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	are_test_data *are_test;

	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	are_test = malloc(sizeof(are_test_data));
	if (!are_test) 
	{
		printf("malloc are_test failed\n");
		return -1;
	}
	memset(are_test, 0 , sizeof(are_test_data));

	if (!data) {
		are_test->rate = g_are_rate;
		are_test->channels = g_are_channels;
		are_test->bits = g_are_bits;
		are_test->loop_count = g_are_loop_count;
		are_test->msec = g_are_run_msec;
	} else {
		memcpy(are_test, data, sizeof(struct are_test_data));
	}

	xTaskCreate(arecord_task, "arecord_task", 2048, are_test,
			configAPPLICATION_OMX_PRIORITY,
			&handle);
	return 0;
}

static void arecord_dump_task(void *arg)
{
	are_test_data *are_test = (are_test_data *)arg;
	int ret = -1;
	int i = 0;
	arePrivateType* priv;
	int rate = are_test->rate;
	int bits = are_test->bits;
	int channels = are_test->channels;
	int frame_bytes = bits / 8 * channels;
	int port_src_out = -1;
	int port_filter_in = -1;
	int port_filter_out = -1;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
	len = (len / 10) * g_are_run_msec;

	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;

	/* Initialize application private data */
	priv = malloc(sizeof(arePrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	priv->eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->eventSem, 0);
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
	err = OMX_GetHandle(&priv->handle, "OMX.audio.record", priv, &arecord_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio record OMX_GetHandle failed\n");
		goto exit;
	}

	err = OMX_GetHandle(&priv->dump_handle, "OMX.dump", priv, &dump_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Dump OMX_GetHandle failed\n");
		goto exit;
	}

	/* 2. config component */
	err = config_source_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in config_source_component\n");
		goto exit;
	}
	err = config_dump_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in untunnel_config_dump_component\n");
		goto exit;
	}

	port_src_out = get_port_index(priv->handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	port_filter_in = get_port_index(priv->dump_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	if (port_src_out < 0 || port_filter_in < 0) {
		printf("Error in get port index, port_src_out %d port_filter_in %d\n", port_src_out, port_filter_in);
		goto exit;
	}

	/* 3. set tunnel */
	err = OMX_SetupTunnel(priv->handle, port_src_out, priv->dump_handle, port_filter_in);
	if(err != OMX_ErrorNone) {
		printf("Set up Tunnel between audio record & audio echo cancel Failed\n");
		goto exit;
	}

	/* 4. set component stat to idle */
	err = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	err = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->eventSem);

	port_filter_out = get_port_index(priv->dump_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_filter_out < 0) {
		printf("Error in get port index, port_filter_out %d\n", port_filter_out);
		goto exit;
	}

	port_def.nPortIndex = port_filter_out;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->dump_handle, OMX_IndexParamPortDefinition, &port_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}
	if ((priv->dump_port_para[port_filter_out].nBufferSize != port_def.nBufferSize) ||
		(priv->dump_port_para[port_filter_out].nBufferCountActual != port_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize %ld nBufferCountActual %ld\n", \
			priv->dump_port_para[port_filter_out].nBufferSize, priv->dump_port_para[port_filter_out].nBufferCountActual , \
			port_def.nBufferSize, port_def.nBufferCountActual);
		goto exit;
	}

	err = alloc_dump_buffer(priv, port_def.nBufferCountActual, port_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}

	omx_sem_down(priv->dump_eventSem);

	/* 5. set component stat to executing */
	err = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->eventSem);

	err = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->dump_eventSem);

	/* 6. send buffer to dump component queue */
	for (i = 0; i < priv->buf_num; i++) {
		err = OMX_FillThisBuffer(priv->dump_handle, priv->buffer[i]);
		printf("OMX_FillThisBuffer %p on port %lu\n", priv->buffer[i],
			priv->buffer[i]->nOutputPortIndex);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_FillThisBuffer\n");
			goto exit;
		}
	}

	printf("audio record test start\n");

	while (are_test->loop_count--) {
		hal_msleep(g_are_run_msec);
	}

	printf("audio record test end\n");

	/* 7. set component stat to idle */
	ret = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->eventSem);

	ret = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->dump_eventSem);

	/* 8. set component stat to loaded */
	ret = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}

	ret = OMX_SendCommand(priv->handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}
	omx_sem_down(priv->eventSem);

	/* 9. free buffer */
	free_dump_buffer(priv);

	omx_sem_down(priv->dump_eventSem);


exit:

	printf("arecord  dump task test finish start\n");
	if (priv->buffer)
		free_dump_buffer(priv);

	if (priv->eventSem) {
		omx_sem_deinit(priv->eventSem);
		free(priv->eventSem);
	}

	if (priv->dump_eventSem) {
		omx_sem_deinit(priv->dump_eventSem);
		free(priv->dump_eventSem);
	}

	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
	}
	if (priv->handle) {
		OMX_FreeHandle(priv->handle);
		priv->handle = NULL;
	}
	if (priv->dump_handle) {
		OMX_FreeHandle(priv->dump_handle);
		priv->dump_handle = NULL;
	}

	OMX_Deinit();

	if (priv) {
		free(priv);
		priv = NULL;
	}
	if (are_test) {
		free(are_test);
		are_test = NULL;
	}
	g_are_cap = 0;
	memset(are_filename, 0 , sizeof(are_filename));
	printf("arecord  dump task test finish end\n");
	vTaskDelete(NULL);
}

static int audio_record_dump_create(rpdata_arg_arecord *targ)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	are_test_data *are_test;

	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	are_test = malloc(sizeof(are_test_data));
	if (are_test) {
		memset(are_test, 0 , sizeof(are_test_data));
		memcpy(&are_test->targ, targ, sizeof(rpdata_arg_arecord));
		are_test->rate = g_are_rate;
		are_test->channels = g_are_channels;
		are_test->bits = g_are_bits;
		are_test->loop_count = g_are_loop_count;
		are_test->msec = g_are_run_msec;
		memcpy(are_test->filename, are_filename, sizeof(are_test->filename));
	} else {
		printf("malloc are_test failed\n");
		return -1;
	}

	xTaskCreate(arecord_dump_task, "arecord_dump_task", 4096, are_test,
			configAPPLICATION_OMX_PRIORITY,
			&handle);
	return 0;
}

static int cmd_arecord_test(int argc, char *argv[])
{
	int c, mode = 2;
	rpdata_arg_arecord targ = {
		.type = "RVtoDSPAre",
		.name = "RVrecvDSPsend",
		.dir  = RPDATA_DIR_DSP,
	};

	g_are_loop_count = 1000;
	g_are_run_msec = 20;  //ms, total time g_are_run_msec * g_are_loop_count = 20s
	g_are_rate = TEST_RATE;
	g_are_channels = TEST_CHANNELS;
	g_are_bits = TEST_BIT_WIDTH;

	optind = 0;
	while ((c = getopt(argc, argv, "hm:t:n:d:l:s:r:c:b:v:g:f:p:")) != -1) {
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
			g_are_loop_count = atoi(optarg);
			break;
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
		case 'v':
			g_rpd_verbose = atoi(optarg);
			break;
		case 'g':
			g_are_cap = atoi(optarg);
			break;
		case 'f':
			g_rpd_forward_port = atoi(optarg);
			break;
		case 'p':
			memcpy(are_filename, optarg, sizeof(are_filename));
			break;
		case 'h':
		default:
			goto usage;
		}
	}

	printf("mode %d g_are_cap %d\n", mode, g_are_cap);

	if ((mode != 0 && mode != 1) && (!g_are_cap))
		goto usage;

	if (arecord_check_dir(targ.dir) < 0)
		goto usage;

	printf("test start\n");

	if (mode == 0)
		do_rpdata_arecord_send(&targ);
	else if (mode == 1)
		do_rpdata_arecord_recv(&targ);

	switch (g_are_cap) {
		case 1:
			audio_record_task_create(NULL);
			break;
		case 2:
			audio_record_dump_create(&targ);
			break;
		default:
			printf("unknown 's' command\n");
			break;
	}

	return 0;
usage:
	arecord_demo_usage();
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_arecord_test, arecord_test, omx test);

