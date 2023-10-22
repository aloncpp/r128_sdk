/**
  utils/asr_test.c

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

#include "asr_test.h"


#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
#include <adb_forward.h>
#endif

static int g_asr_rpd_verbose = 0;
static int g_asr_rpd_forward_port = 0;
static int g_asr_bits = 0;
static int g_asr_loop_count = 0;
static int g_asr_run_msec = 0;
static int g_asr_rate = 0;
static int g_asr_channels = 0;
static int g_asr_dump = 0;
static int g_asr_enable = 0;
static int g_asr_dump_merge = 0;

volatile int rpdata_send_flag = 0;

static char asr_dumpfilename[32];
static char asr_outfilename[32];

static FILE *asrdumpfile = NULL;
static FILE *asroutfile = NULL;

static OMX_U32 error_value = 0u;

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
static int print_time = 0;
static long write_size = 0;
static long write_dumpsize = 0;

#endif

OMX_CALLBACKTYPE asr_tunnel_callbacks = {
	.EventHandler = audioasrEventHandler,
	.EmptyBufferDone = audioasrEmptyBufferDone,
	.FillBufferDone = audioasrFillBufferDone,
};

OMX_CALLBACKTYPE asr_arecord_tunnel_callbacks = {
	.EventHandler = asrarecordEventHandler,
	.EmptyBufferDone = NULL,
	.FillBufferDone = NULL,
};

OMX_CALLBACKTYPE asr_untunnel_callbacks = {
	.EventHandler = audioasrEventHandler,
	.EmptyBufferDone = audioasrEmptyBufferDone,
	.FillBufferDone = asrFillBufferDone,
};

OMX_CALLBACKTYPE asr_dump_callbacks = {
	.EventHandler = asrdumpEventHandler,
	.EmptyBufferDone = NULL,
	.FillBufferDone = NULL,
};

void rpdata_init_param(void)
{
	g_asr_loop_count = -1;
	g_asr_run_msec = 20;
	g_asr_rate = ASR_TEST_RATE;
	g_asr_channels = ASR_TEST_CHANNELS;
	g_asr_bits = ASR_TEST_BIT_WIDTH;
}
void rpdata_deinit_param(void)
{
	g_asr_loop_count = 0;
	g_asr_run_msec = 0;  //ms, total time g_asr_run_msec * g_asr_loop_count = 20s
	g_asr_rate = 0;
	g_asr_channels = 0;
	g_asr_bits = 0;
}

void rpdata_enable_asr(void)
{
	g_asr_enable = 1;
}

void rpdata_disable_asr(void)
{
	g_asr_enable = 0;
}

void rpdata_enable_dump(void)
{
	g_asr_dump = 1;
}

void rpdata_disable_dump(void)
{
	g_asr_dump = 0;
}

void rpdata_enable_dump_merge(void)
{
	g_asr_dump_merge = 1;
}

void rpdata_disable_dump_merge(void)
{
	g_asr_dump_merge = 0;
}

static void setHeader(OMX_PTR header, OMX_U32 size) {
  OMX_VERSIONTYPE* ver = (OMX_VERSIONTYPE*)(header + sizeof(OMX_U32));
  *((OMX_U32*)header) = size;

  ver->s.nVersionMajor = VERSIONMAJOR;
  ver->s.nVersionMinor = VERSIONMINOR;
  ver->s.nRevision = VERSIONREVISION;
  ver->s.nStep = VERSIONSTEP;
}

/* Callbacks implementation */
OMX_ERRORTYPE asrarecordEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	asrPrivateType *private = (asrPrivateType *)pAppData;
	if (hComponent == private->arecord_handle)
		name = "asraudiorecord";

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
OMX_ERRORTYPE audioasrEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	asrPrivateType *private = (asrPrivateType *)pAppData;
	if (hComponent == private->asr_handle)
		name = "audioasr";

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
			omx_sem_up(private->asr_eventSem);
		} else  if (Data1 == OMX_CommandPortEnable){
			printf("%s CmdComplete OMX_CommandPortEnable\n", name);
			omx_sem_up(private->asr_eventSem);
		} else if (Data1 == OMX_CommandPortDisable) {
			printf("%s CmdComplete OMX_CommandPortDisable\n", name);
			omx_sem_up(private->asr_eventSem);
		}
	} else if(eEvent == OMX_EventBufferFlag) {
		if ((int)Data2 == OMX_BUFFERFLAG_EOS) {
			printf("%s BufferFlag OMX_BUFFERFLAG_EOS\n", name);
			if (hComponent == private->asr_handle) {
				printf("end of tunnel");
				omx_sem_up(private->eofSem);
			}
		} else
			printf("%s OMX_EventBufferFlag %lx", name, Data2);
	} else if (eEvent == OMX_EventError) {
		error_value = Data1;
		printf("Receive error event. value:%lx", error_value);
		omx_sem_up(private->asr_eventSem);
	} else {
		printf("Param1 is %i\n", (int)Data1);
		printf("Param2 is %i\n", (int)Data2);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE audioasrEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

  printf("In the %s callback from the port %i\n", __func__, (int)pBuffer->nInputPortIndex);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE audioasrFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	int ret;
	asrPrivateType *private = (asrPrivateType *)pAppData;
/*
	printf("In the %s callback. Got buflen %i  offset %i for buffer at 0x%p\n",
                          __func__, (int)pBuffer->nFilledLen, (int)pBuffer->nOffset, pBuffer);
*/
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
	err = OMX_FillThisBuffer(private->asr_handle, pBuffer);
	if (err != OMX_ErrorNone)
		printf("OMX_FillThisBuffer err: %x", err);

  return err;
}


OMX_ERRORTYPE asrFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	asrPrivateType *private = (asrPrivateType *)pAppData;

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
		if (g_asr_rpd_forward_port > 0)
			adb_forward_send(g_asr_rpd_forward_port, pBuffer->pBuffer, pBuffer->nFilledLen);
		else {
			printf("[%s] line:%d g_asr_rpd_forward_port %d err \n", __func__, __LINE__, g_asr_rpd_forward_port);
		}
#endif
	}

	/* Output data to standard output */
	pBuffer->nFilledLen = 0;
	err = OMX_FillThisBuffer(private->asr_handle, pBuffer);
	if (err != OMX_ErrorNone)
		printf("OMX_FillThisBuffer err: %x", err);

  return err;
}

/* Callbacks implementation */
OMX_ERRORTYPE asrdumpEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	asrPrivateType *private = (asrPrivateType *)pAppData;
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

static void asr_demo_usage(void)
{
	printf("Usgae: asr_demo [option]\n");
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
	printf("-g,          test rv audio asr\n");
	printf("-e,          enable/disable audio asr\n");
	printf("\n");
	printf("DSP -> RV\n");
	printf("rpccli dsp arecord_test -m 0 -d 2 -t DSPtoRVAre -n RVrecvDSPsend -l 1000\n");
	printf("asr_test -m 1 -d 3 -t DSPtoRVAsr -n RVrecvDSPsend -f 20226\n");
	printf("\n");
}

static int get_port_index(OMX_COMPONENTTYPE *comp, OMX_DIRTYPE dir,
	OMX_PORTDOMAINTYPE domain, int start)
{
	int i;
	int ret = -1;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	for (i = start; i < OMX_ASR_PORT_NUMBER_SUPPORTED; i++) {
		port_def.nPortIndex = i;
		ret = OMX_GetParameter(comp, OMX_IndexParamPortDefinition, &port_def);
		if (ret == OMX_ErrorNone && port_def.eDir == dir
			&& port_def.eDomain == domain) {
			ret = i;
			break;
		}
	}
	printf("index:%d\n", i);
	if (i == OMX_ASR_PORT_NUMBER_SUPPORTED)
		printf("can not get port, dir:%d, domain:%d, start:%d",
			dir, domain, start);
	return ret;
}

static OMX_ERRORTYPE alloc_asr_buffer(asrPrivateType *priv, int port_index, OMX_S32 num, OMX_S32 size)
{
	OMX_S32 i = 0;
	OMX_BUFFERHEADERTYPE **buffer;
	OMX_ERRORTYPE ret = OMX_ErrorNone;


	buffer = malloc(num * sizeof(OMX_BUFFERHEADERTYPE *));
	if (NULL == buffer)
		return OMX_ErrorBadParameter;

	if (port_index == priv->port_asr_in)
	{
		priv->bufferin = buffer;
	}

	if (port_index == priv->port_asr_out)
	{
		priv->bufferout = buffer;
	}

	for (i = 0; i < num; i++) {

		buffer[i] = NULL;
		ret = OMX_AllocateBuffer(priv->asr_handle, &buffer[i],
				port_index, priv, size);
		printf("AllocateBuffer %p on port %d\n", buffer[i], port_index);
		if (ret != OMX_ErrorNone) {
			printf("Error on AllocateBuffer %p on port %d\n",
				&buffer[i], port_index);
			break;
		}

	}

	return ret;
}

static int free_asr_buffer(asrPrivateType *priv, int port_index, OMX_S32 num)
{
	OMX_S32 i = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE **buffer;


	if (priv->asr_handle) {


		if (port_index == priv->port_asr_in)
		{
			buffer = priv->bufferin;
		}

		if (port_index == priv->port_asr_out)
		{
			buffer = priv->bufferout;
		}


		if (buffer)
		{
			for (i = 0; i < num; i++) {
				if (buffer[i]) {
					ret = OMX_FreeBuffer(priv->asr_handle,
							port_index,
							buffer[i]);
					if (ret != OMX_ErrorNone)
						printf("port %d ,freebuffer:%ld failed",
							port_index, i);
				}
				buffer[i] = NULL;

			}
			free(buffer);
			buffer = NULL;
		}
	}
	return ret;
}

static int config_asr_component(asrPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_PORT_PARAM_TYPE sParam;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	OMX_AUDIO_CONFIG_AUTOSPEECHRECOGNITIONTYPE sAsrConfig;

	if (g_asr_bits == 0 || g_asr_channels == 0 || g_asr_rate == 0 || g_asr_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = g_asr_bits / 8 * g_asr_channels;
	uint32_t len = frame_bytes * g_asr_rate / 100; /* 10ms */

	/* set input port */

	memset(&priv->asr_port_para[priv->port_asr_in], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->asr_port_para[priv->port_asr_in].nBufferCountActual = DEFAULT_ASR_BUF_CNT;
	priv->asr_port_para[priv->port_asr_in].bBuffersContiguous = 1;
	priv->asr_port_para[priv->port_asr_in].eDomain = OMX_PortDomainAudio;
	priv->asr_port_para[priv->port_asr_in].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->asr_port_para[priv->port_asr_in].nPortIndex = priv->port_asr_in;
	priv->asr_port_para[priv->port_asr_in].nBufferSize = len;

	setHeader(&priv->asr_port_para[priv->port_asr_in], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->asr_handle, OMX_IndexParamPortDefinition,
			&priv->asr_port_para[priv->port_asr_in]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_asr_channels;
	audio_params.nBitPerSample = g_asr_bits;
	audio_params.nSamplingRate = g_asr_rate;
	audio_params.nPortIndex = priv->port_asr_in;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->asr_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio render params error!");
		return ret;
	}

	/* set input port asr config */
	memset(&sAsrConfig, 0, sizeof(OMX_AUDIO_CONFIG_AUTOSPEECHRECOGNITIONTYPE));
	sAsrConfig.nPortIndex = priv->port_asr_in;
	sAsrConfig.bAutoSpeechRecg = priv->asr_test->asr_enable;
	sAsrConfig.bAsrdump = priv->asr_test->asr_dump;
	ret = OMX_SetConfig(priv->asr_handle, OMX_IndexConfigAudioAutoSpeechRecognition,
			&sAsrConfig);
	if (ret) {
		printf("set asr_config error!");
		return ret;
	}

	/* set output port */
	priv->port_asr_out = get_port_index(priv->asr_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (priv->port_asr_out < 0) {
		printf("Error in get port index, port_asr_out %d\n", priv->port_asr_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}
	memset(&priv->asr_port_para[priv->port_asr_out], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->asr_port_para[priv->port_asr_out].nBufferCountActual = DEFAULT_ASR_BUF_CNT;
	priv->asr_port_para[priv->port_asr_out].bBuffersContiguous = 1;
	priv->asr_port_para[priv->port_asr_out].eDomain = OMX_PortDomainAudio;
	priv->asr_port_para[priv->port_asr_out].format.audio.eEncoding = OMX_AUDIO_CodingPCM;

	if (priv->asr_test->asr_dump)
		priv->asr_port_para[priv->port_asr_out].nBufferSize = (len / g_asr_channels) * 4 + ASR_OUT_OFFSET; // (960 / 3)  * 4 + 12 = 1292 byte
	else
		priv->asr_port_para[priv->port_asr_out].nBufferSize = (len / g_asr_channels) + ASR_OUT_OFFSET; // (960 / 3)  + 12 = 332 byte

	priv->asr_port_para[priv->port_asr_out].nPortIndex = priv->port_asr_out;

	setHeader(&priv->asr_port_para[priv->port_asr_out], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->asr_handle, OMX_IndexParamPortDefinition,
			&priv->asr_port_para[priv->port_asr_out]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	memset(&audio_params, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	audio_params.nChannels = g_asr_channels;
	audio_params.nBitPerSample = g_asr_bits;
	audio_params.nSamplingRate = g_asr_rate;
	audio_params.nPortIndex = priv->port_asr_out;
	setHeader(&audio_params, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->asr_handle, OMX_IndexParamAudioPcm,
			&audio_params);
	if (ret) {
		printf("set audio render params error!");
		return ret;
	}

	/** Get the number of ports */
	setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
	ret = OMX_GetParameter(priv->asr_handle, OMX_IndexParamAudioInit, &sParam);
	if(ret != OMX_ErrorNone){
		printf("Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		return ret;
	}
	printf("asr has %d ports\n",(int)sParam.nPorts);

	return ret;
}

static int config_asr_dump_component(asrPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_OTHER_PARAM_DUMPTYPE dump_params;
	OMX_PORT_PARAM_TYPE sParam;
	int port_dump_in = -1;
	int port_dump_out = -1;

	if (g_asr_bits == 0 || g_asr_channels == 0 || g_asr_rate == 0 || g_asr_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = g_asr_bits / 8 * g_asr_channels;
	uint32_t len = frame_bytes * g_asr_rate / 100;  /* asr need 10ms input */

	/* set input port */
	port_dump_in = get_port_index(priv->dump_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	if (port_dump_in < 0) {
		printf("Error in get port index, port_dump_in %d\n", port_dump_in);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}

	memset(&priv->dump_port_para[port_dump_in], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->dump_port_para[port_dump_in].nBufferCountActual = DEFAULT_ASR_BUF_CNT;
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

	strncpy((char *)dump_params.nRpdataType, priv->asr_test->targ_dump.type, sizeof(priv->asr_test->targ_dump.type));
	strncpy((char *)dump_params.nRpdataName, priv->asr_test->targ_dump.name, sizeof(priv->asr_test->targ_dump.name));

#else
	if (g_asr_rpd_forward_port)
		dump_params.nForwardPort = g_asr_rpd_forward_port;
	else
		printf("g_asr_rpd_forward_port is 0\n");

	if (priv->asr_test->filename)
		memcpy(dump_params.nPathName, priv->asr_test->filename, sizeof(priv->asr_test->filename));
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
	priv->dump_port_para[port_dump_out].nBufferCountActual = DEFAULT_ASR_BUF_CNT;
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

static int config_asr_source_component(asrPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE audio_params;
	OMX_PORT_PARAM_TYPE sParam;
	int port_are_out = -1;

	if (g_asr_bits == 0 || g_asr_channels == 0 || g_asr_rate == 0 || g_asr_run_msec == 0)
		return OMX_ErrorBadParameter;

	int frame_bytes = g_asr_bits / 8 * g_asr_channels;
	uint32_t len = frame_bytes * g_asr_rate / 100;    /* asr need 10ms input */

	port_are_out = get_port_index(priv->arecord_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_are_out < 0) {
		printf("Error in get port index, port_are_out %d\n", port_are_out);
		ret = OMX_ErrorBadPortIndex;
		return ret;
	}

	memset(&priv->arecord_port_para, 0, sizeof(priv->arecord_port_para));
	priv->buf_num = DEFAULT_ASR_BUF_CNT;
	priv->arecord_port_para.nBufferCountActual = priv->buf_num;
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
	audio_params.nChannels = g_asr_channels;
	audio_params.nBitPerSample = g_asr_bits;
	audio_params.nSamplingRate = g_asr_rate;
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

static int rpd_dsp_vad_cb(rpdata_t *rpd, void *data, int data_len)
{
	asrPrivateType *priv = NULL;
	OMX_AUDIO_CONFIG_AUTOSPEECHRECOGNITIONTYPE sAsrConfig;
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	vad_config_param_t vad_config_param = {0};

	priv = (asrPrivateType *)rpdata_get_private_data(rpd);
	if (!priv)
		return -1;

	if (data_len != sizeof(vad_config_param_t)) {
		printf("vad get rpdata len %d err!", data_len);
		return -1;
	}

	memcpy(&vad_config_param, data, data_len);

	printf("dsp recv vad enable flag %d\n", vad_config_param.value);

	/* set input port asr config */
	memset(&sAsrConfig, 0, sizeof(OMX_AUDIO_CONFIG_AUTOSPEECHRECOGNITIONTYPE));
	sAsrConfig.nPortIndex = priv->port_asr_in;
	sAsrConfig.bAutoSpeechRecg = priv->asr_test->asr_enable;
	sAsrConfig.bAsrdump = priv->asr_test->asr_dump;
	sAsrConfig.bAsrVad = vad_config_param.value;
	ret = OMX_SetConfig(priv->asr_handle, OMX_IndexConfigAudioAutoSpeechRecognition,
			&sAsrConfig);
	if (ret) {
		printf("set asr_config error!");
		return ret;
	}
	return 0;
}

struct rpdata_cbs rpd_dsp_vad_cbs = {
	.recv_cb = (recv_cb_t)rpd_dsp_vad_cb,
};

static void rpdata_asr_rec_vad(void *arg)
{
	asrPrivateType *priv= (asrPrivateType *)arg;

	rpdata_arg_asr targ_vad = {
		.type = "RVtoDSPVad",
		.name = "RVtoDSPVadCb",
		.dir  = RPDATA_DIR_RV,
	};
	rpdata_t *rpd = NULL;
	void *buffer = NULL;
	int loopcount = g_asr_loop_count;

	printf("dir:%d, type:%s, name:%s\n",
		targ_vad.dir, targ_vad.type, targ_vad.name);

	rpd = rpdata_connect(targ_vad.dir, targ_vad.type, targ_vad.name);
	if (!rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}


	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}


	rpdata_set_private_data(rpd, priv);

	rpdata_set_recv_cb(rpd, &rpd_dsp_vad_cbs);

	if (loopcount < 0) {
		rpdata_send_flag = 1;
		while(rpdata_send_flag)
			hal_msleep(100);
	}
	else {
		while (loopcount--) {
			hal_msleep(g_asr_run_msec);
		}
	}

exit:

	if (rpd)
		rpdata_destroy(rpd);

	printf("rpdata audio asr vad recv test finish\n");
	vTaskDelete(NULL);
}

void do_rpdata_asr_stop_send(void)
{
	rpdata_send_flag = 0;
}

static void rpdata_asr_send(void *arg)
{
	asr_test_data *asr_test = (asr_test_data *)arg;
	rpdata_t *rpd;
	void *buffer = NULL;
	int ret = -1;
	asrPrivateType* priv;
	int i = 0;
	int rate = asr_test->rate;
	int bits = asr_test->bits;
	int channels = asr_test->channels;
	int frame_bytes = bits / 8 * channels;
	uint32_t len = frame_bytes * rate / 100; /* 10ms */
#if 0
	uint32_t rpdata_len = bits / 8 * 1 * rate / 100; /* data after processing(aec, mono ch) */
#else
	uint32_t rpdata_len;

	if (asr_test->asr_dump)
		rpdata_len = (len / 3) * 4 + ASR_OUT_OFFSET; // 1292
	else
		rpdata_len = len / 3 + ASR_OUT_OFFSET; // 332
#endif
	int port_src_out = -1;
	int port_dump_in = -1;
	int port_dump_out = -1;
	int loopcount = 0;
	TaskHandle_t recv_handle;

	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE portout_def;

	printf("dir:%d, type:%s, name:%s\n",
		asr_test->targ.dir, asr_test->targ.type, asr_test->targ.name);

	rpd = rpdata_create(asr_test->targ.dir, asr_test->targ.type, asr_test->targ.name, rpdata_len);
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
	priv = malloc(sizeof(asrPrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	memset(priv, 0, sizeof(asrPrivateType));

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

	priv->asr_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->asr_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->asr_eventSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	if (g_asr_dump) {
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
	priv->asr_test = asr_test;

	 /* 1. get component arecord_handle */
	err = OMX_GetHandle(&priv->arecord_handle, "OMX.audio.record", priv, &asr_arecord_tunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio record OMX_GetHandle failed\n");
		goto exit;
	}

	if (g_asr_dump) {
		err = OMX_GetHandle(&priv->dump_handle, "OMX.dump", priv, &asr_dump_callbacks);
		if(err != OMX_ErrorNone) {
			printf("Dump OMX_GetHandle failed\n");
			goto exit;
		}
	}

	err = OMX_GetHandle(&priv->asr_handle, "OMX.audio.asr", priv, &asr_tunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio asr OMX_GetHandle failed\n");
		goto exit;
	}

	priv->port_asr_in = get_port_index(priv->asr_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	priv->port_asr_out = get_port_index(priv->asr_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (priv->port_asr_out < 0 || priv->port_asr_in < 0) {
		printf("Error in get port index, port_src_out %d port_filter_in %d\n", priv->port_asr_in, priv->port_asr_out);
		goto exit;
	}

	/* 2. config component */
	err = config_asr_source_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in config_asr_source_component\n");
		goto exit;
	}

	if (g_asr_dump) {
		err = config_asr_dump_component(priv);
		if(err != OMX_ErrorNone){
			printf("Error in config_asr_dump_component\n");
			goto exit;
		}
	}

	err = config_asr_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in config_asr_component\n");
		goto exit;
	}

	/* 3. set tunnel */
	port_src_out = get_port_index(priv->arecord_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (port_src_out < 0){
		printf("Error in get port index, port_src_out %d\n", port_src_out);
		goto exit;
	}


	if (g_asr_dump) {
		port_dump_in = get_port_index(priv->dump_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
		port_dump_out = get_port_index(priv->dump_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
		if (port_dump_in < 0 || port_dump_out < 0) {
			printf("Error in get port index, port_src_in %d, port_src_out %d\n", port_dump_in, port_dump_out);
			goto exit;
		}
		err = OMX_SetupTunnel(priv->arecord_handle, port_src_out, priv->dump_handle, port_dump_in);
		if(err != OMX_ErrorNone) {
			printf("Set up Tunnel between audio record & audio dump Failed\n");
			goto exit;
		}

		err = OMX_SetupTunnel(priv->dump_handle, port_dump_out, priv->asr_handle, priv->port_asr_in);
		if(err != OMX_ErrorNone) {
			printf("Set up Tunnel between dump & audio render Failed\n");
			goto exit;
		}
	}
	else {
		err = OMX_SetupTunnel(priv->arecord_handle, port_src_out, priv->asr_handle, priv->port_asr_in);
		if(err != OMX_ErrorNone) {
			printf("Set up Tunnel between dump & audio render Failed\n");
			goto exit;
		}
	}

	/* 4. set component stat to idle */

	err = OMX_SendCommand(priv->asr_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	if (g_asr_dump) {
		err = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
			goto exit;
		}
	}

	err = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	omx_sem_down(priv->arecord_eventSem);

	if (g_asr_dump)
		omx_sem_down(priv->dump_eventSem);

	portout_def.nPortIndex = priv->port_asr_out;
	setHeader(&portout_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->asr_handle, OMX_IndexParamPortDefinition, &portout_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}
	if ((priv->asr_port_para[priv->port_asr_out].nBufferSize != portout_def.nBufferSize) ||
		(priv->asr_port_para[priv->port_asr_out].nBufferCountActual != portout_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize %ld nBufferCountActual %ld\n", \
					priv->asr_port_para[priv->port_asr_out].nBufferSize, priv->asr_port_para[priv->port_asr_out].nBufferCountActual , \
					portout_def.nBufferSize, portout_def.nBufferCountActual);
		goto exit;
	}

	err = alloc_asr_buffer(priv, priv->port_asr_out, portout_def.nBufferCountActual, portout_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}

	omx_sem_down(priv->asr_eventSem);


	/* 5. set component stat to executing */
	err = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->arecord_eventSem);

	if (g_asr_dump) {
		err = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
			goto exit;
		}
		/* Wait for commands to complete */
		omx_sem_down(priv->dump_eventSem);
	}

	err = OMX_SendCommand(priv->asr_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->asr_eventSem);

	xTaskCreate(rpdata_asr_rec_vad, "rpdata_asr_rec_vad", 1024, priv,
			configAPPLICATION_OMX_HIGH_PRIORITY, &recv_handle);

	rpdata_wait_connect(rpd);

	/* 6. send buffer to source component queue */
	for (i = 0; i < priv->buf_num; i++) {
		err = OMX_FillThisBuffer(priv->asr_handle, priv->bufferout[i]);
		printf("OMX_FillThisBuffer %p on port %lu\n", priv->bufferout[i],
			priv->bufferout[i]->nOutputPortIndex);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_FillThisBuffer\n");
			goto exit;
		}
	}

	loopcount = asr_test->loop_count;

	printf("audio asr test start loopcount %d\n", loopcount);

	if (loopcount < 0) {
		rpdata_send_flag = 1;
		while(rpdata_send_flag)
			hal_msleep(100);
	}
	else {
		while (1) {
			hal_msleep(g_asr_run_msec);
			if (loopcount == 0)
				break;
			if (loopcount < 0)
				continue;
			loopcount--;
		}
	}

	printf("audio asr test end\n");

	/* 7. set component stat to idle */
	ret = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->arecord_eventSem);

	if (g_asr_dump) {
		ret = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
			goto exit;
		}
		omx_sem_down(priv->dump_eventSem);
	}

	ret = OMX_SendCommand(priv->asr_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->asr_eventSem);

	/* 8. free buffer */
	free_asr_buffer(priv, priv->port_asr_out, portout_def.nBufferCountActual);

	/* 9. set component stat to loaded */
	ret = OMX_SendCommand(priv->asr_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}

	if (g_asr_dump) {
		ret = OMX_SendCommand(priv->dump_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
			goto exit;
		}
	}

	ret = OMX_SendCommand(priv->arecord_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}

	omx_sem_down(priv->arecord_eventSem);

	if (g_asr_dump)
		omx_sem_down(priv->dump_eventSem);

	omx_sem_down(priv->asr_eventSem);

exit:

	free_asr_buffer(priv, priv->port_asr_out, portout_def.nBufferCountActual);

	if (priv->arecord_eventSem) {
		omx_sem_deinit(priv->arecord_eventSem);
		free(priv->arecord_eventSem);
	}

	if (g_asr_dump && priv->dump_eventSem) {
		omx_sem_deinit(priv->dump_eventSem);
		free(priv->dump_eventSem);
	}

	if (priv->asr_eventSem) {
		omx_sem_deinit(priv->asr_eventSem);
		free(priv->asr_eventSem);
	}

	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
	}

	if (priv->arecord_handle) {
		OMX_FreeHandle(priv->arecord_handle);
		priv->arecord_handle = NULL;
	}

	if (g_asr_dump && priv->dump_handle) {
		OMX_FreeHandle(priv->dump_handle);
		priv->dump_handle = NULL;
	}

	if (priv->asr_handle) {
		OMX_FreeHandle(priv->asr_handle);
		priv->asr_handle = NULL;
	}

	if (priv->rpd) {
		rpdata_destroy(priv->rpd);
		priv->rpd_buffer = NULL;
	}

	OMX_Deinit();

	if (priv->asr_test) {
		free(priv->asr_test);
		priv->asr_test = NULL;
	}

	if (priv) {
		free(priv);
		priv = NULL;
	}
	g_asr_dump  = 0;
	g_asr_rpd_forward_port = 0;
	printf("rpdata audio asr send test finish\n");
	vTaskDelete(NULL);
}

int do_rpdata_asr_send(rpdata_arg_asr *targ, rpdata_arg_asr *targ_dump)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	asr_test_data *asr_test;

	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	asr_test = malloc(sizeof(asr_test_data));
	if (asr_test) {
		memset(asr_test, 0 , sizeof(asr_test_data));
		memcpy(&asr_test->targ, targ, sizeof(rpdata_arg_asr));
		memcpy(&asr_test->targ_dump, targ_dump, sizeof(rpdata_arg_asr));
		asr_test->rate = g_asr_rate;
		asr_test->channels = g_asr_channels;
		asr_test->bits = g_asr_bits;
		asr_test->loop_count = g_asr_loop_count;
		asr_test->msec = g_asr_run_msec;
		asr_test->asr_enable = g_asr_enable;
		asr_test->asr_dump = g_asr_dump_merge;
	} else {
		printf("malloc asr_test failed\n");
		return -1;
	}

	xTaskCreate(rpdata_asr_send, "rpd_asr_send", 2048, asr_test,
			configAPPLICATION_OMX_HIGH_PRIORITY, &handle);
	hal_msleep(500);
	return 0;
}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
static int rpd_audio_asr_forward_cb(rpdata_t *rpd, void *data, int data_len)
{
	int ret;
	int  *pIntmp = NULL;
	float  *pFloattmp = NULL;
	int frame_bytes = g_asr_bits / 8 * g_asr_channels;
	uint32_t len = frame_bytes * g_asr_rate / 100; /* 10ms */
	asr_out_t *asr_out = (asr_out_t *)rpdata_get_private_data(rpd);

	if (g_asr_dump_merge)
		len = (len / g_asr_channels) * 4;  // 1280byte
	else
		len = len / g_asr_channels;  // 320byte

	if (!asroutfile && !g_asr_rpd_forward_port) {
		//printf("[%s] line:%d infile is null and g_asr_rpd_forward_port is 0\n", __func__, __LINE__);
		return 0;
	}

	pIntmp = (int *)(data);

	asr_out->vad_flag = pIntmp[0];

	asr_out->word_id = pIntmp[1];

	pFloattmp = (float *)((char*)data + sizeof(int) * 2);

	asr_out->confidence = pFloattmp[0];

	memcpy((char *)asr_out->vad_out, (char *)data + ASR_OUT_OFFSET, data_len - ASR_OUT_OFFSET);

	if (g_asr_rpd_forward_port)
		ret = adb_forward_send(g_asr_rpd_forward_port, (char *)data + ASR_OUT_OFFSET , data_len - ASR_OUT_OFFSET);

	if (asroutfile) {
		ret = fwrite((char *)data + ASR_OUT_OFFSET, 1, data_len - ASR_OUT_OFFSET, asroutfile);
		if (ret != data_len - ASR_OUT_OFFSET)
			printf("[%s] line:%d fwrite err ! ret=%d, data_len=%d\n", __func__, __LINE__, ret, data_len);
		write_size += ret;
		if (write_size == len * 20) {
			fflush(asroutfile);
			fsync(fileno(asroutfile));
			write_size = 0;
			print_time++;
		}
	}

	if (print_time && print_time % 10 == 0) {
		printf("[%s] line:%d vad_flag = %d, word_id = %d confidence = %f data_len=%d\n", __func__, __LINE__, \
				asr_out->vad_flag, asr_out->word_id, asr_out->confidence, data_len);
		print_time = 0;
	}
	return 0;
}

struct rpdata_cbs rpd_audio_asr_forward_cbs = {
	.recv_cb = (recv_cb_t)rpd_audio_asr_forward_cb,
};
#endif

static int rpd_audio_asr_recv_cb(rpdata_t *rpd, void *data, uint32_t data_len)
{
	static int count = 0;

#if 1
	int i;
	for (i = 0; i < data_len; i++) {
		uint8_t *c = data + i;
		if (*c  != RPDATA_AUDIO_ASR_CHECK_CHAR) {
			printf("data check failed, expected 0x%x but 0x%x\n",
				RPDATA_AUDIO_ASR_CHECK_CHAR, *c);
			return 0;
		}
	}
#endif
	if (!g_asr_rpd_verbose)
		return 0;
	if (count++ % g_asr_rpd_verbose == 0)
		printf("audio data check ok(print interval %d)\n", g_asr_rpd_verbose);
	return 0;
}

struct rpdata_cbs rpd_audio_asr_cbs = {
	.recv_cb = rpd_audio_asr_recv_cb,
};

static void rpdata_asr_recv(void *arg)
{
	rpdata_arg_asr targ;
	rpdata_t *rpd = NULL;
	void *buffer = NULL;
	rpdata_t *vad_rpd = NULL;
	void *vad_buffer = NULL;
	int loopcount = g_asr_loop_count;
	vad_config_param_t vad_config_param;
	int ret = 0;
	asr_out_t *asr_out = NULL;
	int frame_bytes = g_asr_bits / 8 * g_asr_channels;
	uint32_t len = frame_bytes * g_asr_rate / 100; /* 10ms */

	rpdata_arg_asr targ_vad = {
		.type = "RVtoDSPVad",
		.name = "RVtoDSPVadCb",
		.dir  = RPDATA_DIR_DSP,
	};

	if (!strlen(asr_outfilename) && !g_asr_rpd_forward_port) {
		printf("[%s] line:%d infile and g_asr_rpd_forward_port is both null\n", __func__, __LINE__);
		goto exit;
	}

	memcpy(&targ, arg, sizeof(rpdata_arg_asr));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if(g_asr_rpd_forward_port) {
		if (adb_forward_create_with_rawdata(g_asr_rpd_forward_port) < 0) {
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

	vad_rpd = rpdata_create(targ_vad.dir, targ_vad.type, targ_vad.name, sizeof(vad_config_param_t));
	if (!vad_rpd) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	vad_buffer = rpdata_buffer_addr(vad_rpd);
	if (!vad_buffer) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	if (strlen(asr_outfilename)) {
		asroutfile = fopen(asr_outfilename, "wb");
		if (!asroutfile) {
			printf("open file %s error!\n", asr_outfilename);
			goto exit;
		}
	}

	asr_out = malloc(sizeof(asr_out_t));
	if (!asr_out) {
		printf("no memory!\n");
		goto exit;
	}

	memset(asr_out, 0 ,sizeof(asr_out_t));

	if (g_asr_dump_merge) {
		asr_out->vad_out = malloc((len / g_asr_channels) * 4); // 1280
		if (!asr_out->vad_out) {
			printf("no memory!\n");
			goto exit;
		}
		memset(asr_out->vad_out, 0 ,(len / g_asr_channels) * 4);
	}
	else {
		asr_out->vad_out = malloc(len / g_asr_channels); // 320
		if (!asr_out->vad_out) {
			printf("no memory!\n");
			goto exit;
		}
		memset(asr_out->vad_out, 0 , (len / g_asr_channels));
	}

	rpdata_set_private_data(rpd, asr_out);

#if 1

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	rpdata_set_recv_cb(rpd, &rpd_audio_asr_forward_cbs);
#else
	rpdata_set_recv_cb(rpd, &rpd_audio_asr_cbs);
#endif

	memset(vad_buffer, 0 , sizeof(vad_config_param_t));

	vad_config_param.value = 1;

	printf("send vad enable flag %d\n", vad_config_param.value);

	memcpy(vad_buffer, &vad_config_param, sizeof(vad_config_param_t));

	rpdata_wait_connect(vad_rpd);

	ret = rpdata_send(vad_rpd, 0, sizeof(vad_config_param_t));
	if (ret != 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	if (loopcount < 0) {
		rpdata_send_flag = 1;
		while(rpdata_send_flag)
			hal_msleep(100);
	}
	else {

		while (1) {

			hal_msleep(g_asr_run_msec + 1);

			if (loopcount == 0)
				break;
			if (loopcount < 0)
				continue;

			loopcount--;

			if (loopcount == 50) {
				memset(vad_buffer, 0 , sizeof(vad_config_param_t));

				vad_config_param.value = 0;

				printf("send vad enable flag %d\n", vad_config_param.value);

				memcpy(vad_buffer, &vad_config_param, sizeof(vad_config_param_t));

				rpdata_wait_connect(vad_rpd);

				ret = rpdata_send(vad_rpd, 0, sizeof(vad_config_param_t));
				if (ret != 0) {
					printf("[%s] line:%d \n", __func__, __LINE__);
					goto exit;
				}
			}
		}
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
		ret = adb_forward_send(g_asr_rpd_forward_port, recv_buf, len);
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

	if (asroutfile)
		fflush(asroutfile);

	if (asroutfile)
		fclose(asroutfile);

	asroutfile = NULL;

	memset(asr_outfilename, 0 , sizeof(asr_outfilename));

	if (rpd)
		rpdata_destroy(rpd);

	if (vad_rpd)
		rpdata_destroy(vad_rpd);

	if (asr_out) {
		if (asr_out->vad_out) {
			free(asr_out->vad_out);
			asr_out->vad_out = NULL;
		}
		free(asr_out);
		asr_out = NULL;
	}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if (g_asr_rpd_forward_port > 0)
		adb_forward_end(g_asr_rpd_forward_port);
#endif
	g_asr_rpd_forward_port = 0;

	printf("rpdata audio asr recv test finish\n");
	vTaskDelete(NULL);
}

static int do_rpdata_asr_recv(rpdata_arg_asr *targ)
{
	TaskHandle_t handle;

	xTaskCreate(rpdata_asr_recv, "rpd_asr_recv", 1024, targ,
			configAPPLICATION_OMX_PRIORITY, &handle);
	hal_msleep(500);
	return 0;
}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
static int rpd_audio_asr_dump_forward_cb(rpdata_t *rpd, void *data, int data_len)
{
	int ret;
	int frame_bytes = g_asr_bits / 8 * g_asr_channels;
	uint32_t len = frame_bytes * g_asr_rate / 100; /* 10ms */

	if (!asrdumpfile && !g_asr_rpd_forward_port) {
		printf("[%s] line:%d infile is null and g_asr_rpd_forward_port is 0\n", __func__, __LINE__);
		return 0;
	}

	if (g_asr_rpd_forward_port)
		ret = adb_forward_send(g_asr_rpd_forward_port, data , data_len);

	if (asrdumpfile) {
		ret = fwrite(data, 1, data_len, asrdumpfile);
		if (ret != data_len)
			printf("[%s] line:%d fwrite err ! ret=%d, data_len=%d\n", __func__, __LINE__, ret, data_len);

		write_dumpsize += ret;
		if (write_dumpsize == len * 20) {
			fflush(asrdumpfile);
			fsync(fileno(asrdumpfile));
			write_dumpsize = 0;
		}
	}

	//printf("[%s] line:%d ret=%d, data_len=%d\n", __func__, __LINE__, ret, data_len);

	return 0;
}

struct rpdata_cbs rpd_audio_asr_dump_forward_cbs = {
	.recv_cb = (recv_cb_t)rpd_audio_asr_dump_forward_cb,
};
#endif

static void rpdata_asr_dump_recv(void *arg)
{
	rpdata_arg_asr targ;
	rpdata_t *rpd = NULL;
	void *buffer = NULL;
	int loopcount = g_asr_loop_count;

	if (!strlen(asr_dumpfilename) && !g_asr_rpd_forward_port) {
		printf("[%s] line:%d infile and g_asr_rpd_forward_port is both null\n", __func__, __LINE__);
		goto exit;
	}

	memcpy(&targ, arg, sizeof(rpdata_arg_asr));
	printf("dir:%d, type:%s, name:%s\n",
		targ.dir, targ.type, targ.name);

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if(g_asr_rpd_forward_port) {
		if (adb_forward_create_with_rawdata(g_asr_rpd_forward_port) < 0) {
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

	if (strlen(asr_dumpfilename)) {
		asrdumpfile = fopen(asr_dumpfilename, "wb");
		if (!asrdumpfile) {
			printf("open file %s error!\n", asr_dumpfilename);
			goto exit;
		}
	}

#if 1

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	rpdata_set_recv_cb(rpd, &rpd_audio_asr_dump_forward_cbs);
#else
	rpdata_set_recv_cb(rpd, &rpd_audio_asr_cbs);
#endif

	if (loopcount < 0) {
		rpdata_send_flag = 1;
		while(rpdata_send_flag)
			hal_msleep(100);
	}
	else {
		while (loopcount--) {
			hal_msleep(g_asr_run_msec + 2);
		}
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
		ret = adb_forward_send(g_asr_rpd_forward_port, recv_buf, len);
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

	if (asrdumpfile)
		fflush(asrdumpfile);

	if (asrdumpfile)
		fclose(asrdumpfile);

	asrdumpfile = NULL;

	memset(asr_dumpfilename, 0 , sizeof(asr_dumpfilename));

	if (rpd)
		rpdata_destroy(rpd);

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if (g_asr_rpd_forward_port > 0)
		adb_forward_end(g_asr_rpd_forward_port);
#endif
	g_asr_rpd_forward_port = 0;

	printf("rpdata audio asr dump recv test finish\n");
	vTaskDelete(NULL);
}

static int do_rpdata_asr_dump_recv(rpdata_arg_asr *targ)
{
	TaskHandle_t handle;

	xTaskCreate(rpdata_asr_dump_recv, "rpd_asr_dump_recv", 1024, targ,
			configAPPLICATION_OMX_PRIORITY, &handle);
	hal_msleep(500);
	return 0;
}

static int asr_check_dir(int dir)
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

static int cmd_asr_test(int argc, char *argv[])
{
	int c, mode = 2;
	rpdata_arg_asr targ = {
		.type = "RVtoDSPAsr",
		.name = "RVrecvDSPsend",
		.dir  = RPDATA_DIR_DSP,
	};

	rpdata_arg_asr targ_dump = {
		.type = "RVtoDSPAsrdump",
		.name = "dRVrecvDSPsend",
		.dir  = RPDATA_DIR_DSP,
	};

	g_asr_loop_count = -1;
	g_asr_run_msec = 20;  //ms, total time g_asr_run_msec * g_asr_loop_count = 20s
	g_asr_rate = ASR_TEST_RATE;
	g_asr_channels = ASR_TEST_CHANNELS;
	g_asr_bits = ASR_TEST_BIT_WIDTH;
	g_asr_enable = 0;
	g_asr_dump = 0;
	g_asr_dump_merge = 0;

	optind = 0;
	while ((c = getopt(argc, argv, "hm:t:n:j:k:d:l:s:r:c:b:v:g:e:o:f:p:q:")) != -1) {
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
		case 'j':
			strncpy(targ_dump.type, optarg, sizeof(targ_dump.type));
			break;
		case 'k':
			strncpy(targ_dump.name, optarg, sizeof(targ_dump.name));
			break;
		case 'd':
			targ.dir = atoi(optarg);
			targ_dump.dir = atoi(optarg);
			break;
		case 'l':
			g_asr_loop_count = atoi(optarg);
			break;
		case 's':
			g_asr_run_msec = atoi(optarg);
			break;
		case 'r':
			g_asr_rate = atoi(optarg);
			break;
		case 'c':
			g_asr_channels = atoi(optarg);
			break;
		case 'b':
			g_asr_bits = atoi(optarg);
			break;
		case 'v':
			g_asr_rpd_verbose = atoi(optarg);
			break;
		case 'g':
			g_asr_dump = atoi(optarg);
			break;
		case 'e':
			g_asr_enable = atoi(optarg);
			break;
		case 'o':
			g_asr_dump_merge = atoi(optarg);
			break;
		case 'f':
			g_asr_rpd_forward_port = atoi(optarg);
			break;
		case 'p':
			memcpy(asr_outfilename, optarg, sizeof(asr_outfilename));
			break;
		case 'q':
			memcpy(asr_dumpfilename, optarg, sizeof(asr_dumpfilename));
			break;
		case 'h':
		default:
			goto usage;
		}
	}

	printf("mode %d g_asr_dump %d\n", mode, g_asr_dump);

	if (mode != 0 && mode != 1 && mode != 2 && mode != 3)
		goto usage;

	if (asr_check_dir(targ.dir) < 0)
		goto usage;

	printf("test start\n");

	switch (mode) {
		case 0:
			do_rpdata_asr_send(&targ, &targ_dump);
			break;
		case 1:
			do_rpdata_asr_dump_recv(&targ_dump);
			break;
		case 2:
			do_rpdata_asr_recv(&targ);
			break;
		case 3:
			do_rpdata_asr_stop_send();
			break;
		default:
			printf("unknown 's' command\n");
			break;
	}

	return 0;
usage:
	asr_demo_usage();
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_asr_test, asr_test, omx test);

