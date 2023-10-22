/**
  utils/rpbuf_aec_test.c

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <aw_list.h>
#include <hal_mutex.h>
#include <hal_mem.h>
#include <rpbuf.h>
#include <console.h>

#include <hal_time.h>
#include <ringbuffer.h>
#include <hal_event.h>

#include "rpbuf_common_interface.h"

#include "rpbuf_aec_test.h"

static long g_aec_len = 0;
static int  g_aec_ctrl_id = 0;
static int  g_aec_play = 0;
static int  g_aec_enable = 0;
//static unsigned long g_recv_time = 0;
//static unsigned long g_send_time = 0;
//static unsigned long g_empty_time = 0;

#define AEC_EV_DATA_GET (1 << 0)
#define AEC_IN_EV_DATA_GET (2 << 0)
#define AEC_EV_DATA_SET (3 << 0)

static int pcm_s16le_split_to_plane(aecPrivateType* priv, short *dst, short *src, unsigned int len)
{
	aecPrivateType *private = (aecPrivateType *)priv;
	uint32_t rate = 0;
	uint32_t channels = 0;
	uint8_t bitwidth = 0;
	uint32_t BytesPerSample = 0;
	short nSampleCount = 0;
	int i = 0;

	channels = private->aec_prms.chan;
	bitwidth = private->aec_prms.bits_per_sample;
	rate     = private->aec_prms.sampling_rate;

	if (channels == 0 || bitwidth== 0 || rate == 0) {
		printf("input param error, rate %d, ch %d, width %d\n", \
				rate, channels, bitwidth);
		return -1;
	}

	BytesPerSample = bitwidth * channels / 8;

	nSampleCount = len / BytesPerSample;

	for (i = 0; i < nSampleCount; ++i) {

		dst[i] = src[2 * i];

		dst[i + nSampleCount] = src[2 * i + 1];
	}

	return 0;
}

static void rpbuf_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printf("buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}

static int rpbuf_config_rx_cb(struct rpbuf_buffer *buffer,
		void *data, int data_len, void *priv)
{
	aecPrivateType *private = (aecPrivateType *)priv;

	printf("buffer \"%s\" received data (addr: %p, offset: %d, len: %d):\n", \
			rpbuf_buffer_name(buffer), rpbuf_buffer_va(buffer),
			data - rpbuf_buffer_va(buffer), data_len);

	if (data_len != sizeof(aec_prms_t)) {
		printf("buffer \"%s\" received data len err! (offset: %d, len: %d):\n", \
		rpbuf_buffer_name(buffer), data - rpbuf_buffer_va(buffer), data_len);
		return 0;
	}

	memcpy(&private->aec_prms, data, data_len);

	memset(data, 0, data_len);

	omx_sem_up(private->aec_start);
	return 0;
}


static int rpbuf_aec_in_rx_cb(struct rpbuf_buffer *buffer,
		void *data, int data_len, void *priv)
{
	aecPrivateType *private = (aecPrivateType *)priv;
	int ret = 0;
	int write_size = 0;
	int size = -1;
	int offset = 0;
	int timeout =  1000;
/*
	printf("buffer \"%s\" received data (addr: %p, offset: %d, len: %d) time: %ld delta time: %ld\n", \
			rpbuf_buffer_name(buffer), rpbuf_buffer_va(buffer),
			data - rpbuf_buffer_va(buffer), data_len, OSTICK_TO_MS(hal_tick_get()), OSTICK_TO_MS(hal_tick_get()) - g_recv_time);

	g_recv_time = OSTICK_TO_MS(hal_tick_get());
*/
	if (data_len != private->aec_test->aec_len) {
		printf("buffer \"%s\" received data len err! (offset: %d, len: %d):\n", \
		rpbuf_buffer_name(buffer), data - rpbuf_buffer_va(buffer), data_len);
		ret = -1;
		return ret;
	}

	write_size = data_len;

	while (write_size > 0) {

		size = hal_ringbuffer_put(private->aec_rb, data + offset, write_size);
		if (size < 0) {
			printf("ring buf put err %d\n", ret);
			goto exit;
		}

		if (size == write_size)
			break;

		offset += size;
		write_size -= size;

		ret = hal_event_wait(private->event, AEC_IN_EV_DATA_GET,
			HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		if (!ret) {
			ret = -1;
			printf("aec in cb wait data timeout:%d", timeout);
			break;
		}
	}

	hal_event_set_bits(private->event, AEC_EV_DATA_SET);

	memset(data, 0, data_len);

	ret = rpbuf_common_transmit(private->aec_test->aec_arg.aec_in_recv_name, (void *)data, data_len, 0);
	if (ret < 0) {
		printf("rpbuf_common_transmit (with data) failed\n");
	}


exit:
	return ret;
}

static int rpbuf_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	aecPrivateType *private = (aecPrivateType *)priv;

	printf("buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));
	if (!strncmp(rpbuf_buffer_name(buffer), private->aec_test->aec_arg.aec_in_name, sizeof(private->aec_test->aec_arg.aec_in_name)))
	{
		printf("buffer \"%s\": recv thread will exit\n", rpbuf_buffer_name(buffer));

		private->aec_eof = OMX_TRUE;
	}
	return 0;
}

static const struct rpbuf_buffer_cbs rpbuf_config_cbs = {
	.available_cb 	= rpbuf_available_cb,
	.rx_cb 			= rpbuf_config_rx_cb,
	.destroyed_cb 	= rpbuf_destroyed_cb,
};

static const struct rpbuf_buffer_cbs rpbuf_aec_in_cbs = {
	.available_cb 	= rpbuf_available_cb,
	.rx_cb 			= rpbuf_aec_in_rx_cb,
	.destroyed_cb 	= rpbuf_destroyed_cb,
};

static const struct rpbuf_buffer_cbs rpbuf_aec_out_cbs = {
	.available_cb 	= rpbuf_available_cb,
	.rx_cb 			= NULL,
	.destroyed_cb 	= rpbuf_destroyed_cb,
};

/* Application private date: should go in the component field (segs...) */
static OMX_U32 error_value = 0u;

OMX_CALLBACKTYPE aec_untunnel_callbacks = {
	.EventHandler = AudioEchoCancelEventHandler,
	.EmptyBufferDone = AudioEchoCancelEmptyBufferDone,
	.FillBufferDone = AudioEchoCancelFillBufferDone,
};

OMX_CALLBACKTYPE aec_tunnel_callbacks = {
	.EventHandler = AudioEchoCancelEventHandler,
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
OMX_ERRORTYPE AudioEchoCancelEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	aecPrivateType *private = (aecPrivateType *)pAppData;
	if (hComponent == private->aec_handle)
		name = "AudioEchoCancel";

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

OMX_ERRORTYPE AudioEchoCancelEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	aecPrivateType *private = (aecPrivateType *)pAppData;

	if (pBuffer == NULL || pAppData == NULL) {
		printf("err: buffer header is null");
		return OMX_ErrorBadParameter;
	}
/*
	printf("empty buffer done>> %p, %lu, input:%lu output:%lu time %ld delta time %ld\n", pBuffer->pBuffer, \
	pBuffer->nFlags, pBuffer->nInputPortIndex, pBuffer->nOutputPortIndex, OSTICK_TO_MS(hal_tick_get()) ,OSTICK_TO_MS(hal_tick_get()) - g_empty_time);

	g_empty_time= OSTICK_TO_MS(hal_tick_get());
*/
	err = queue(private->BufferQueue, pBuffer);
	if (err != OMX_ErrorNone)
		printf("queue buffer err: %d", err);

	return err;

}

OMX_ERRORTYPE AudioEchoCancelFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	int ret;
	int write_size = 0;
	int size = 0;
	int offset = 0;
	aecPrivateType *private = (aecPrivateType *)pAppData;
	int timeout =  1000;

	/*printf("In the %s callback. Got buflen %i for buffer at 0x%p time %ld delta time %ld\n",
                          __func__, (int)pBuffer->nFilledLen, pBuffer, OSTICK_TO_MS(hal_tick_get()), OSTICK_TO_MS(hal_tick_get()) - g_send_time);
	g_send_time = OSTICK_TO_MS(hal_tick_get());
	*/
	if (pBuffer == NULL || pAppData == NULL) {
		printf("err: buffer header is null");
		return OMX_ErrorBadParameter;
	}

	if (pBuffer->nFilledLen == 0) {
		printf("Ouch! In %s: no data in the output buffer!\n", __func__);
		return OMX_ErrorNone;
	}

	write_size = pBuffer->nFilledLen;

	while (write_size > 0) {

		size = hal_ringbuffer_put(private->aec_out_rb, pBuffer->pBuffer + pBuffer->nOffset + offset, write_size);
		if (size < 0) {
			printf("ring buf put err %d\n", ret);
			break;
		}

		if (size == write_size)
			break;

		offset += size;
		write_size -= size;

		ret = hal_event_wait(private->send_event, AEC_EV_DATA_GET,
			HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		if (!ret) {
			ret = -1;
			printf("wait data timeout:%d", timeout);
			break;
		}
	}

	hal_event_set_bits(private->send_event, AEC_EV_DATA_SET);

	/* Output data to standard output */
	pBuffer->nFilledLen = 0;
	err = OMX_FillThisBuffer(hComponent, pBuffer);
	if (err != OMX_ErrorNone)
		printf("OMX_FillThisBuffer err: %x\n", err);

	return err;
}

static void rpbuf_aec_demo_usage(void)
{
	printf("\n");
	printf("USAGE:\n");
	printf("  rpbuf_aec_test [OPTIONS]\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("  -g          : rpbuf config_name (default: rpbuf_aec_config)\n");
	printf("  -i          : rpbuf aec_in_name (default: rpbuf_aec_in)\n");
	printf("  -o          : rpbuf aec_out_name (default: rpbuf_aec_out)\n");
	printf("  -e          : enable aduio echo cancel	 	(default: 0)\n");
	printf("  -l          : test audio echo cancel(1: test aec, 2: add dump)\n");
	printf("  -I ID       : specify rpbuf ctrl ID (default: 0)\n");
	printf("  -L LENGTH   : specify buffer length (default: %d bytes)\n",
			AEC_RPBUF_BUFFER_LENGTH_DEFAULT);
	printf("\n");
	printf("e.g.\n");
	printf("  rpbuf_aec_test -I 0 -L 1280 -l 1 -e 1 \n");
	printf("  rpbuf_aec_test -I 0 -L 1280 -l 2 -e 1 -g \"rp_aec_config\" -i \"rp_aec_in\" -o \"rp_aec_out\" \n");
	printf("\n");
}

static int get_port_index(OMX_COMPONENTTYPE *comp, OMX_DIRTYPE dir,
	OMX_PORTDOMAINTYPE domain, int start)
{
	int i;
	int ret = -1;
	OMX_PARAM_PORTDEFINITIONTYPE port_def;
	setHeader(&port_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	for (i = start; i < AEC_OMX_PORT_NUMBER_SUPPORTED; i++) {
		port_def.nPortIndex = i;
		ret = OMX_GetParameter(comp, OMX_IndexParamPortDefinition, &port_def);
		if (ret == OMX_ErrorNone && port_def.eDir == dir
			&& port_def.eDomain == domain) {
			ret = i;
			break;
		}
	}
	printf("index:%d\n", i);
	if (i == AEC_OMX_PORT_NUMBER_SUPPORTED)
		printf("can not get port, dir:%d, domain:%d, start:%d",
			dir, domain, start);
	return ret;
}

static OMX_ERRORTYPE alloc_aec_buffer(aecPrivateType *priv, int port_index, OMX_S32 num, OMX_S32 size)
{
	OMX_S32 i = 0;
	OMX_BUFFERHEADERTYPE **buffer;
	OMX_ERRORTYPE ret = OMX_ErrorNone;


	buffer = malloc(num * sizeof(OMX_BUFFERHEADERTYPE *));
	if (NULL == buffer)
		return OMX_ErrorBadParameter;

	if (port_index == priv->port_filter_in)
	{
		priv->bufferin = buffer;
	}

	if (port_index == priv->port_filter_out)
	{
		priv->bufferout = buffer;
	}

	for (i = 0; i < num; i++) {

		buffer[i] = NULL;
		ret = OMX_AllocateBuffer(priv->aec_handle, &buffer[i],
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

static int free_aec_buffer(aecPrivateType *priv, int port_index, OMX_S32 num)
{
	OMX_S32 i = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE **buffer;


	if (priv->aec_handle) {


		if (port_index == priv->port_filter_in)
		{
			buffer = priv->bufferin;
		}

		if (port_index == priv->port_filter_out)
		{
			buffer = priv->bufferout;
		}


		if (buffer)
		{
			for (i = 0; i < num; i++) {
				if (buffer[i]) {
					ret = OMX_FreeBuffer(priv->aec_handle,
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

static void print_aec_prms(const aec_prms_t *prms)
{

    printf("  sampling_rate: %d, bits_per_sample: %d, chan: %d\n",
            prms->sampling_rate, prms->bits_per_sample, prms->chan);

    printf("  nlpMode: %d, skewMode: %d, metricsMode: %d delay_logging %d\n",
            prms->aec_config.nlpMode, prms->aec_config.skewMode, prms->aec_config.metricsMode, prms->aec_config.delay_logging);

}


static int untunnel_config_aec_component(aecPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_PORT_PARAM_TYPE sParam;
	OMX_OTHER_PARAM_AECTYPE sAEcParams;
	OMX_AUDIO_PARAM_PCMMODETYPE sAudioParams;
	OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE sAecConfig;

	memset(&priv->aec_port_para[priv->port_filter_in], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	/* set input port */
	priv->aec_port_para[priv->port_filter_in].nBufferCountActual = DEFAULT_AECHO_CANCEL_BUF_CNT;
	priv->aec_port_para[priv->port_filter_in].bBuffersContiguous = 1;
	priv->aec_port_para[priv->port_filter_in].eDomain = OMX_PortDomainAudio;
	priv->aec_port_para[priv->port_filter_in].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->aec_port_para[priv->port_filter_in].nPortIndex = priv->port_filter_in;
	priv->aec_port_para[priv->port_filter_in].nBufferSize = AEC_RPBUF_BUFFER_PROCESS_LENGTH_DEFAULT;

	setHeader(&priv->aec_port_para[priv->port_filter_in], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->aec_handle, OMX_IndexParamPortDefinition,
			&priv->aec_port_para[priv->port_filter_in]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	/* set input port aec param */
	memset(&sAEcParams, 0, sizeof(OMX_OTHER_PARAM_AECTYPE));
	sAEcParams.nPortIndex = priv->port_filter_in;
	sAEcParams.pAecprm = &priv->aec_prms.aec_config;

	setHeader(&sAEcParams, sizeof(OMX_OTHER_PARAM_AECTYPE));
	ret = OMX_SetParameter(priv->aec_handle, OMX_IndexVendorParamAEC,
			&sAEcParams);
	if (ret) {
		printf("set aec params error!");
		return ret;
	}

	/* set input port pcm param */
	memset(&sAudioParams, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	sAudioParams.nPortIndex = priv->port_filter_in;
	sAudioParams.nChannels = priv->aec_prms.chan;
	sAudioParams.nBitPerSample = priv->aec_prms.bits_per_sample;
	sAudioParams.nSamplingRate = priv->aec_prms.sampling_rate;

	setHeader(&sAudioParams, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->aec_handle, OMX_IndexParamAudioPcm,
			&sAudioParams);
	if (ret) {
		printf("set audio params error!");
		return ret;
	}

	printf("Audio param chan %d, bits_per_sample %d, sampling_rate %d eq enable %d\n", \
		priv->aec_prms.chan, priv->aec_prms.bits_per_sample, priv->aec_prms.sampling_rate, priv->aec_test->aec_enable);

	print_aec_prms((aec_prms_t*)&priv->aec_prms);

	/* set input port aec config */
	memset(&sAecConfig, 0, sizeof(OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE));
	sAecConfig.nPortIndex = priv->port_filter_in;
	sAecConfig.eEchoCancelation = priv->aec_test->aec_enable;
	ret = OMX_SetConfig(priv->aec_handle, OMX_IndexConfigAudioEchoCancelation,
			&sAecConfig);
	if (ret) {
		printf("set aec_config error!");
		return ret;
	}

	/* set output port */
	memset(&priv->aec_port_para[priv->port_filter_out], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->aec_port_para[priv->port_filter_out].nBufferCountActual = DEFAULT_AECHO_CANCEL_BUF_CNT;
	priv->aec_port_para[priv->port_filter_out].bBuffersContiguous = 1;
	priv->aec_port_para[priv->port_filter_out].eDomain = OMX_PortDomainAudio;
	priv->aec_port_para[priv->port_filter_out].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->aec_port_para[priv->port_filter_out].nPortIndex = priv->port_filter_out;
	priv->aec_port_para[priv->port_filter_out].nBufferSize = AEC_RPBUF_BUFFER_PROCESS_LENGTH_DEFAULT / 2;

	setHeader(&priv->aec_port_para[priv->port_filter_out], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->aec_handle, OMX_IndexParamPortDefinition,
			&priv->aec_port_para[priv->port_filter_out]);
	if (ret) {
		printf("set port params error!");
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

static void aec_send_thread(void *params)
{
	aecPrivateType *private = (aecPrivateType *)params;
	unsigned char *data = NULL;
	int ret = OMX_ErrorNone;
	int read_bytes = 0;
	unsigned int data_len = private->aec_test->aec_len / 2;
	int timeout = 300;
	int read_size = 0;
	int read_len = 0;
	int offset = 0;
	int cnt = 0;

	data = malloc(data_len);
	if (!data) {
		printf("malloc len :%d failed\n", data_len);
		goto exit;
	}

	while (1) {


		read_size = data_len;
		offset = 0;
		read_len = 0;
		cnt = 0;

		while (read_size > 0) {

			read_bytes = hal_ringbuffer_get(private->aec_out_rb, data + offset, read_size, 1000);
			if(read_bytes < 0) {
				printf("read err:%d\n", read_bytes);
				goto exit;
			}

			read_len += read_bytes;

			if (read_bytes == read_size)
				break;

			if (read_bytes == 0) {
				cnt++;
				if (cnt > 4) {
					cnt = 0;
					break;
				}
			}

			offset += read_bytes;
			read_size -= read_bytes;

			ret = hal_event_wait(private->send_event, AEC_EV_DATA_SET,
				HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
			if (!ret) {
				ret = -1;
				printf("send wait data timeout:%d\n", timeout);
				continue;
			}
		}


		hal_event_set_bits(private->send_event, AEC_EV_DATA_GET);

		if (read_len == 0 && private->aec_eof == OMX_TRUE) {
			printf("exit aec send loop!\n");
			break;
		}

		if (data_len != read_len)
		{
			printf("send data len %d is not equal to read_bytes %d\n", data_len, read_len);
			goto exit;
		}

		if (read_len > 0) {
			ret = rpbuf_common_transmit(private->aec_test->aec_arg.aec_out_name, data, data_len, 0);
			if (ret < 0) {
				printf("rpbuf_common_transmit (with data) failed\n");
			}
		}
/*
		printf("send to remote. buflen %i  time %ld delta time %ld\n",
	                  (int)read_len, OSTICK_TO_MS(hal_tick_get()), OSTICK_TO_MS(hal_tick_get()) - g_send_time);

		g_send_time = OSTICK_TO_MS(hal_tick_get());
*/
	}

exit:

	if (data) ;
		free(data);
	data = NULL;

	printf("end of aec send thread\n");
	vTaskDelete(NULL);

}

static void aec_process_thread(void *params)
{
	OMX_BUFFERHEADERTYPE *pBuffer = NULL;
	aecPrivateType *private = (aecPrivateType *)params;
	unsigned char *data = NULL;
	int ret = OMX_ErrorNone;
	int read_bytes = 0;
	unsigned int data_len = AEC_RPBUF_BUFFER_PROCESS_LENGTH_DEFAULT;
	OMX_BOOL eof = OMX_FALSE;
	int timeout = 300;
	int cnt = 0;
	int read_size = 0;
	int offset = 0;
	int read_len = 0;

	data = malloc(data_len);
	if (!data) {
		printf("malloc len :%d failed\n", data_len);
		goto exit;
	}

	while (1) {

		if (eof) {
			printf("exit aec loop!\n");
			break;
		}

		pBuffer = NULL;

		while (pBuffer == NULL) {
			/* dequeue one buffer */
			pBuffer = dequeue(private->BufferQueue);
			if (pBuffer == NULL) {
				printf("queue num is %d\n", getquenelem(private->BufferQueue));
				hal_msleep(10);
				continue;
			}
		}

		read_size = data_len;
		offset = 0;
		read_len = 0;
		cnt = 0;

		while (read_size > 0) {

			read_bytes = hal_ringbuffer_get(private->aec_rb, data + offset, read_size, 1000);
			if(read_bytes < 0) {
				printf("read err:%d\n", read_bytes);
				goto exit;
			}

			read_len += read_bytes;

			if (read_bytes == read_size)
				break;

			if (read_bytes == 0) {
				cnt++;
				if (cnt > 3) {
					cnt = 0;
					break;
				}
			}

			offset += read_bytes;
			read_size -= read_bytes;

			ret = hal_event_wait(private->event, AEC_EV_DATA_SET,
				HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
			if (!ret) {
				ret = -1;
				printf("wait data timeout:%d\n", timeout);
				continue;
			}
		}

		hal_event_set_bits(private->event, AEC_IN_EV_DATA_GET);

		if (read_len > 0 && data_len != read_len)
		{
			printf("data len %d is not equal to read_bytes %d\n", data_len, read_len);
			goto exit;
		}

		if (read_len) {
			ret = pcm_s16le_split_to_plane(private, (short *)pBuffer->pBuffer, (short *)data, read_len);
			if (ret != OMX_ErrorNone) {
				printf("pcm_s16le_split_to_plane error!\n");
				goto exit;
			}
		}

		if (read_len == 0 && private->aec_eof == OMX_TRUE) {
			eof = OMX_TRUE;
			pBuffer->nFilledLen = 0;
			pBuffer->nOffset = 0;
			pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
		}
		else {
			pBuffer->nFilledLen = read_len;
			pBuffer->nOffset = 0;
			pBuffer->nFlags = 0;
		}

		if (read_len || (private->aec_eof == OMX_TRUE)) {
			pBuffer->nInputPortIndex = private->port_filter_in;
			ret = OMX_EmptyThisBuffer(private->aec_handle, pBuffer);
			if (ret != OMX_ErrorNone) {
				printf("empty this buffer error!\n");
				goto exit;
			}
		}
		else {
			ret = queue(private->BufferQueue, pBuffer);
			if (ret != OMX_ErrorNone){
				printf("queue buffer err: %d", ret);
				goto exit;
			}
		}
	}
exit:

	if (data) ;
		free(data);
	data = NULL;

	printf("end of aec process thread\n");
	vTaskDelete(NULL);
}

static void aec_task(void *arg)
{
	aec_test_data *aec_test = (aec_test_data *)arg;
	int ret = -1;
	int i = 0;
	TaskHandle_t handle;

	aecPrivateType *priv = NULL;

	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE portin_def;
	OMX_PARAM_PORTDEFINITIONTYPE portout_def;

	/* Initialize application private data */
	priv = malloc(sizeof(aecPrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	memset(priv, 0 ,sizeof(aecPrivateType));

	/* init buffer queue */
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

	/* init aeq_eventSem */
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

	/* init aeq_start */
	priv->aec_start = malloc(sizeof(omx_sem_t));
	if (!priv->aec_start) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->aec_start, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}


	/* init eofSem */
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

	priv->aec_test = aec_test;

	priv->aec_rb = hal_ringbuffer_init(aec_test->aec_len * 4); // 40 * 4 ms
	if (!priv->aec_rb) {
		printf("create ringbuffer err");
		goto exit;
	}

	priv->aec_out_rb = hal_ringbuffer_init(aec_test->aec_len); // 20 ms
	if (!priv->aec_out_rb) {
		printf("create ringbuffer err");
		goto exit;
	}

	priv->event = hal_event_create();
	if (!priv->event) {
		printf("create event err");
		goto exit;
	}

	priv->send_event = hal_event_create();
	if (!priv->send_event) {
		printf("create event err");
		goto exit;
	}

	ret = rpbuf_common_init();
	if (ret < 0) {
		printf("rpbuf_common_init failed\n");
		goto exit;
	}

	ret = rpbuf_common_create(aec_test->aec_ctrl_id, aec_test->aec_arg.config_name, sizeof(aec_prms_t), 0, &rpbuf_config_cbs, priv);
	if (ret < 0) {
		printf("rpbuf_create for name %s (len: %d) failed\n", aec_test->aec_arg.config_name, sizeof(aec_prms_t));
		goto exit;
	}

	ret = rpbuf_common_create(aec_test->aec_ctrl_id, aec_test->aec_arg.aec_in_name, aec_test->aec_len, 0, &rpbuf_aec_in_cbs, priv);
	if (ret < 0) {
		 printf("rpbuf_create for name %s (len: %d) failed\n", aec_test->aec_arg.aec_in_name, aec_test->aec_len);
		 goto exit;
	}

	ret = rpbuf_common_create(aec_test->aec_ctrl_id, aec_test->aec_arg.aec_in_recv_name, aec_test->aec_len, 0, &rpbuf_aec_out_cbs, priv);
	if (ret < 0) {
		 printf("rpbuf_create for name %s (len: %d) failed\n", aec_test->aec_arg.aec_in_recv_name, aec_test->aec_len);
		 goto exit;
	}

	ret = rpbuf_common_create(aec_test->aec_ctrl_id, aec_test->aec_arg.aec_out_name, aec_test->aec_len / 2, 0, &rpbuf_aec_out_cbs, priv);
	if (ret < 0) {
		 printf("rpbuf_create for name %s (len: %d) failed\n", aec_test->aec_arg.aec_out_name, aec_test->aec_len / 2);
		 goto exit;
	}

	omx_sem_down(priv->aec_start);

	/* 1. get component handle */
	err = OMX_GetHandle(&priv->aec_handle, "OMX.audio.echocancel", priv, &aec_untunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio echocancel OMX_GetHandle failed\n");
		goto exit;
	}

	priv->port_filter_in = get_port_index(priv->aec_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	priv->port_filter_out = get_port_index(priv->aec_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (priv->port_filter_in < 0 || priv->port_filter_out < 0) {
		printf("Error in get port index, port_filter_in %d port_filter_out %d\n", priv->port_filter_in, priv->port_filter_out);
		ret = OMX_ErrorBadPortIndex;
		goto exit;
	}

	/* 2. config component */
	err = untunnel_config_aec_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in untunnel config aec component\n");
		goto exit;
	}

	/* 3. set component stat to idle */
	err = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	portin_def.nPortIndex = priv->port_filter_in;
	setHeader(&portin_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->aec_handle, OMX_IndexParamPortDefinition, &portin_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}

	if ((priv->aec_port_para[priv->port_filter_in].nBufferSize != portin_def.nBufferSize) ||
		(priv->aec_port_para[priv->port_filter_in].nBufferCountActual != portin_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize %ld nBufferCountActual %ld\n", \
			priv->aec_port_para[priv->port_filter_in].nBufferSize, priv->aec_port_para[priv->port_filter_in].nBufferCountActual , \
			portin_def.nBufferSize, portin_def.nBufferCountActual);
		goto exit;
	}
	err = alloc_aec_buffer(priv, priv->port_filter_in, portin_def.nBufferCountActual, portin_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}

	for (i = 0; i < portin_def.nBufferCountActual; i++) {
		ret = queue(priv->BufferQueue, priv->bufferin[i]);
		if (ret != 0) {
			printf("queue buffer %d error!\n", i);
			goto exit;
		}
	}
	printf("queue num is %d\n", getquenelem(priv->BufferQueue));

	portout_def.nPortIndex = priv->port_filter_out;
	setHeader(&portout_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->aec_handle, OMX_IndexParamPortDefinition, &portout_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}

	if ((priv->aec_port_para[priv->port_filter_out].nBufferSize != portout_def.nBufferSize) ||
		(priv->aec_port_para[priv->port_filter_out].nBufferCountActual != portout_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize %ld nBufferCountActual %ld\n", \
			priv->aec_port_para[priv->port_filter_out].nBufferSize, priv->aec_port_para[priv->port_filter_out].nBufferCountActual , \
			portout_def.nBufferSize, portout_def.nBufferCountActual);
		goto exit;
	}

	err = alloc_aec_buffer(priv, priv->port_filter_out, portout_def.nBufferCountActual, portout_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}
	omx_sem_down(priv->aec_eventSem);

	/* 4. set component stat to executing */
	err = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->aec_eventSem);

	/* 5. send buffer to source component queue */
	for (i = 0; i < portout_def.nBufferCountActual; i++) {
		err = OMX_FillThisBuffer(priv->aec_handle, priv->bufferout[i]);
		printf("OMX_FillThisBuffer %p on port %lu\n", priv->bufferout[i],
			priv->bufferout[i]->nOutputPortIndex);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_FillThisBuffer\n");
			goto exit;
		}
	}

	printf("audio echo cancel test wait for eos\n");

	ret = rpbuf_common_transmit(priv->aec_test->aec_arg.config_name, (void *)&priv->aec_prms, sizeof(aec_prms_t), 0);
	if (ret < 0) {
		printf("rpbuf_common_transmit config name (with data) failed\n");
	}

	xTaskCreate(aec_process_thread, "aec_process_thread", 2048, (void *)priv,
			configAPPLICATION_OMX_PRIORITY,
			&handle);

	xTaskCreate(aec_send_thread, "aec_send_thread", 2048, (void *)priv,
			configAPPLICATION_OMX_PRIORITY,
			&handle);


	omx_sem_down(priv->eofSem);

	printf("audio echo cancel test get eos signal\n");

	/* 6. set component stat to idle */
	ret = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->aec_eventSem);

	/* 7. free buffer */
	free_aec_buffer(priv, priv->port_filter_in, portin_def.nBufferCountActual);
	free_aec_buffer(priv, priv->port_filter_out, portout_def.nBufferCountActual);

	/* 8. set component stat to loaded */
	ret = OMX_SendCommand(priv->aec_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}
	omx_sem_down(priv->aec_eventSem);

exit:
	printf("audio echo cancel task exit\n");

	free_aec_buffer(priv, priv->port_filter_in, portin_def.nBufferCountActual);
	free_aec_buffer(priv, priv->port_filter_out, portout_def.nBufferCountActual);

	ret = rpbuf_common_destroy(aec_test->aec_arg.config_name);
	if (ret < 0) {
		printf("rpbuf_destroy for name %s (len: %d) failed\n", aec_test->aec_arg.config_name, sizeof(aec_prms_t));
	}
	ret = rpbuf_common_destroy(aec_test->aec_arg.aec_in_name);
	if (ret < 0) {
		printf("rpbuf_destroy for name %s (len: %d) failed\n", aec_test->aec_arg.aec_in_name, aec_test->aec_len);
	}
	ret = rpbuf_common_destroy(aec_test->aec_arg.aec_in_recv_name);
	if (ret < 0) {
		printf("rpbuf_destroy for name %s (len: %d) failed\n", aec_test->aec_arg.aec_in_recv_name, aec_test->aec_len);
	}
	ret = rpbuf_common_destroy(aec_test->aec_arg.aec_out_name);
	if (ret < 0) {
		printf("rpbuf_destroy for name %s (len: %d) failed\n", aec_test->aec_arg.aec_out_name, aec_test->aec_len / 2);
	}

	rpbuf_common_deinit();

	if (priv->aec_rb) {
		hal_ringbuffer_release(priv->aec_rb);
		priv->aec_rb = NULL;
	}

	if (priv->aec_out_rb) {
		hal_ringbuffer_release(priv->aec_out_rb);
		priv->aec_out_rb = NULL;
	}

	if (priv->event) {
		hal_event_delete(priv->event);
		priv->event = NULL;
	}

	if (priv->send_event) {
		hal_event_delete(priv->send_event);
		priv->send_event = NULL;
	}

	if (priv->aec_eventSem) {
		omx_sem_deinit(priv->aec_eventSem);
		free(priv->aec_eventSem);
		priv->aec_eventSem = NULL;
	}

	if (priv->aec_start) {
		omx_sem_deinit(priv->aec_start);
		free(priv->aec_start);
		priv->aec_start = NULL;
	}


	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
		priv->eofSem = NULL;
	}
	if (priv->aec_handle) {
		OMX_FreeHandle(priv->aec_handle);
		priv->aec_handle = NULL;
	}
	if (priv->BufferQueue) {
		queue_deinit(priv->BufferQueue);
		free(priv->BufferQueue);
		priv->BufferQueue = NULL;
	}

	OMX_Deinit();

	priv->aec_eof = OMX_FALSE;

	if (priv) {
		free(priv);
		priv = NULL;
	}
	if (aec_test) {
		free(aec_test);
		aec_test = NULL;
	}
	g_aec_play = 0;

	printf("aec task test finish\n");
	vTaskDelete(NULL);
}

static int audio_echocancel_task_create(rpbuf_arg_aec *targ)
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
	if (aec_test)
	{
		memset(aec_test, 0 , sizeof(aec_test_data));
		aec_test->aec_len = g_aec_len;
		aec_test->aec_ctrl_id = g_aec_ctrl_id;
		aec_test->aec_enable = g_aec_enable;
		memcpy(&aec_test->aec_arg, targ, sizeof(rpbuf_arg_aec));
	}
	else
	{
		printf("malloc aec test failed\n");
		return -1;
	}


	xTaskCreate(aec_task, "aec_task", 2048, aec_test,
			configAPPLICATION_OMX_PRIORITY,
			&handle);
	return 0;
}

static int cmd_rpbuf_aec_test(int argc, char *argv[])
{
	int c;
	int ret = 0;

	rpbuf_arg_aec targ = {
		.config_name = "rpbuf_aec_config",
		.aec_in_name = "rpbuf_aec_in",
		.aec_in_recv_name = "rpbuf_aec_in_recv",
		.aec_out_name  = "rpbuf_aec_out",
	};

	g_aec_len = AEC_RPBUF_BUFFER_LENGTH_DEFAULT;
	g_aec_ctrl_id = 0;
	g_aec_enable = 0;

	optind = 0;
	while ((c = getopt(argc, argv, "hg:i:o:e:l:I:L:")) != -1) {
		switch (c) {
		case 'h':
			ret = 0;
			goto usage;
		case 'g':
			strncpy(targ.config_name, optarg, sizeof(targ.config_name));
			break;
		case 'i':
			strncpy(targ.aec_in_name, optarg, sizeof(targ.aec_in_name));
			break;
		case 'o':
			strncpy(targ.aec_out_name, optarg, sizeof(targ.aec_out_name));
			break;
		case 'e':
			g_aec_enable = atoi(optarg);
			break;
		case 'l':
			g_aec_play = atoi(optarg);
			break;
		case 'I':
			g_aec_ctrl_id = atoi(optarg);
			break;
		case 'L':
			g_aec_len = strtol(optarg, NULL, 0);
			if (g_aec_len == -ERANGE) {
				printf("Invalid length arg.\r\n");
				goto usage;
			}
			break;
		default:
			printf("Invalid option: -%c\n", c);
			ret = -1;
			goto usage;
		}
	}

	printf("g_aec_play %d \n", g_aec_play);

	if (g_aec_play == 0)
		goto usage;

	printf("test start\n");

	switch (g_aec_play) {

		case 1:
			audio_echocancel_task_create(&targ);
			break;
		case 2:
			//audio_record_dump_render_task_create(NULL);
			break;
		default:
			printf("unknown 'l' command\n");
			break;
	}

	return 0;
usage:
	rpbuf_aec_demo_usage();
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpbuf_aec_test, rpbuf_aec_test, audio echo cancel test);

