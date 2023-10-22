/**
  utils/aeq_test.c

  This simple test application take one input stream. Put the input to eq.

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

#include <aw_list.h>
#include <hal_mutex.h>
#include <hal_mem.h>
#include <rpbuf.h>
#include <console.h>
#include <ringbuffer.h>

#include <hal_time.h>

#include "rpbuf_common_interface.h"

#include "aeq_test.h"

static long g_aeq_len = 0;
static int  g_aeq_ctrl_id = 0;
static int  g_aeq_play = 0;
static int  g_aeq_enable = 0;

//static unsigned long g_recv_time = 0;
//static unsigned long g_send_time = 0;
//static unsigned long g_empty_time = 0;

#define AEQ_EV_DATA_GET (1 << 0)
#define AEQ_IN_EV_DATA_GET (2 << 0)
#define AEQ_EV_DATA_SET (3 << 0)

static void rpbuf_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printf("buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}

static int rpbuf_aeq_config_cb(struct rpbuf_buffer *buffer,
		void *data, int data_len, void *priv)
{
	aeqPrivateType *private = (aeqPrivateType *)priv;

	printf("buffer \"%s\" received data (addr: %p, offset: %d, len: %d):\n", \
			rpbuf_buffer_name(buffer), rpbuf_buffer_va(buffer),
			data - rpbuf_buffer_va(buffer), data_len);

	if (data_len != sizeof(eq_remote_prms_t)) {
		printf("buffer \"%s\" received data len err! (offset: %d, len: %d):\n", \
		rpbuf_buffer_name(buffer), data - rpbuf_buffer_va(buffer), data_len);
		return 0;
	}

	memcpy(&private->eq_prms[0], data, data_len);

	memset(data, 0, data_len);

	omx_sem_up(private->aeq_start);
	return 0;
}

static int rpbuf_aeq_config_reset_cb(struct rpbuf_buffer *buffer,
		void *data, int data_len, void *priv)
{
	aeqPrivateType *private = (aeqPrivateType *)priv;

	printf("buffer \"%s\" received data (addr: %p, offset: %d, len: %d):\n", \
			rpbuf_buffer_name(buffer), rpbuf_buffer_va(buffer),
			data - rpbuf_buffer_va(buffer), data_len);

	if (data_len != sizeof(eq_remote_prms_t)) {
		printf("buffer \"%s\" received data len err! (offset: %d, len: %d):\n", \
		rpbuf_buffer_name(buffer), data - rpbuf_buffer_va(buffer), data_len);
		return 0;
	}

	memcpy(&private->eq_prms[1], data, data_len);

	memset(data, 0, data_len);

	omx_sem_up(private->aeq_reset);
	return 0;
}

static int rpbuf_aeq_in_cb(struct rpbuf_buffer *buffer,
		void *data, int data_len, void *priv)
{
	aeqPrivateType *private = (aeqPrivateType *)priv;
//	OMX_BUFFERHEADERTYPE *pBuffer = NULL;
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
	if (data_len != private->aeq_test->aeq_len) {
		printf("buffer \"%s\" received data len err! (offset: %d, len: %d):\n", \
		rpbuf_buffer_name(buffer), data - rpbuf_buffer_va(buffer), data_len);
		ret = -1;
		return ret;
	}

	write_size = data_len;

	while (write_size > 0) {

		size = hal_ringbuffer_put(private->aeq_rb, data + offset, write_size);
		if (size < 0) {
			printf("ring buf put err %d\n", ret);
			goto exit;
		}

		if (size == write_size)
			break;

		offset += size;
		write_size -= size;

		ret = hal_event_wait(private->event, AEQ_IN_EV_DATA_GET,
			HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		if (!ret) {
			ret = -1;
			printf("aec in cb wait data timeout:%d", timeout);
			break;
		}
	}

	hal_event_set_bits(private->event, AEQ_EV_DATA_SET);

	memset(data, 0, data_len);


exit:
	return ret;
}

static int rpbuf_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	aeqPrivateType *private = (aeqPrivateType *)priv;

	printf("buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));
	if (!strncmp(rpbuf_buffer_name(buffer), private->aeq_test->aeq_arg.aeq_in_name, sizeof(private->aeq_test->aeq_arg.aeq_in_name)))
	{
		printf("buffer \"%s\": recv thread will exit\n", rpbuf_buffer_name(buffer));
		
		private->aeq_eof = OMX_TRUE;
	}
	return 0;
}

static const struct rpbuf_buffer_cbs rpbuf_aeq_config_cbs = {
	.available_cb 	= rpbuf_available_cb,
	.rx_cb 			= rpbuf_aeq_config_cb,
	.destroyed_cb 	= rpbuf_destroyed_cb,
};

static const struct rpbuf_buffer_cbs rpbuf_aeq_config_reset_cbs = {
	.available_cb 	= rpbuf_available_cb,
	.rx_cb 			= rpbuf_aeq_config_reset_cb,
	.destroyed_cb 	= rpbuf_destroyed_cb,
};

static const struct rpbuf_buffer_cbs rpbuf_aeq_in_cbs = {
	.available_cb 	= rpbuf_available_cb,
	.rx_cb 			= rpbuf_aeq_in_cb,
	.destroyed_cb 	= rpbuf_destroyed_cb,
};

static const struct rpbuf_buffer_cbs rpbuf_aeq_out_cbs = {
	.available_cb 	= rpbuf_available_cb,
	.rx_cb 			= NULL,
	.destroyed_cb 	= rpbuf_destroyed_cb,
};

/* Application private date: should go in the component field (segs...) */
static OMX_U32 error_value = 0u;

OMX_CALLBACKTYPE aeq_untunnel_callbacks = {
	.EventHandler = AudioEqualizerEventHandler,
	.EmptyBufferDone = AudioEqualizerEmptyBufferDone,
	.FillBufferDone = AudioEqualizerFillBufferDone,
};

OMX_CALLBACKTYPE aeq_tunnel_callbacks = {
	.EventHandler = AudioEqualizerEventHandler,
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
OMX_ERRORTYPE AudioEqualizerEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

	char *name;

	aeqPrivateType *private = (aeqPrivateType *)pAppData;
	if (hComponent == private->aeq_handle)
		name = "AudioEqualizer";

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
			omx_sem_up(private->aeq_eventSem);
		} else  if (Data1 == OMX_CommandPortEnable){
			printf("%s CmdComplete OMX_CommandPortEnable\n", name);
			omx_sem_up(private->aeq_eventSem);
		} else if (Data1 == OMX_CommandPortDisable) {
			printf("%s CmdComplete OMX_CommandPortDisable\n", name);
			omx_sem_up(private->aeq_eventSem);
		}
	} else if(eEvent == OMX_EventBufferFlag) {
		if ((int)Data2 == OMX_BUFFERFLAG_EOS) {
			printf("%s BufferFlag OMX_BUFFERFLAG_EOS\n", name);
			if (hComponent == private->aeq_handle) {
				printf("end of tunnel");
				omx_sem_up(private->eofSem);
			}
		} else
			printf("%s OMX_EventBufferFlag %lx", name, Data2);
	} else if (eEvent == OMX_EventError) {
		error_value = Data1;
		printf("Receive error event. value:%lx", error_value);
		omx_sem_up(private->aeq_eventSem);
	} else {
		printf("Param1 is %i\n", (int)Data1);
		printf("Param2 is %i\n", (int)Data2);
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioEqualizerEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	aeqPrivateType *private = (aeqPrivateType *)pAppData;

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

OMX_ERRORTYPE AudioEqualizerFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

	OMX_ERRORTYPE err;
	int ret;
	int write_size = 0;
	int size = 0;
	int offset = 0;
	aeqPrivateType *private = (aeqPrivateType *)pAppData;
	int timeout =  1000;

/*
	printf("In the %s callback. Got buflen %i for buffer at 0x%p time %ld delta time %ld\n",
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

		size = hal_ringbuffer_put(private->aeq_out_rb, pBuffer->pBuffer + pBuffer->nOffset + offset, write_size);
		if (size < 0) {
			printf("ring buf put err %d\n", ret);
			break;
		}

		if (size == write_size)
			break;

		offset += size;
		write_size -= size;

		ret = hal_event_wait(private->send_event, AEQ_EV_DATA_GET,
			HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		if (!ret) {
			ret = -1;
			printf("wait data timeout:%d", timeout);
			break;
		}
	}

	hal_event_set_bits(private->send_event, AEQ_EV_DATA_SET);
/*
	if(pBuffer->nFilledLen > 0) {
		ret = rpbuf_common_transmit(private->aeq_test->aeq_arg.recv_name, (void *)pBuffer->pBuffer, pBuffer->nFilledLen, pBuffer->nOffset);
		if (ret < 0) {
			printf("rpbuf_common_transmit (with data) failed\n");
		}
	}
*/
	/* Output data to standard output */
	pBuffer->nFilledLen = 0;
	err = OMX_FillThisBuffer(hComponent, pBuffer);
	if (err != OMX_ErrorNone)
		printf("OMX_FillThisBuffer err: %x", err);

	return err;
}

static void aeq_demo_usage(void)
{
	printf("\n");
	printf("USAGE:\n");
	printf("  aeq_test [OPTIONS]\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("  -s          : rpbuf start name (default: rpbuf_eq_start)\n");
	printf("  -d          : rpbuf reset name (default: rpbuf_eq_reset)\n");
	printf("  -p          : rpbuf send name (default: rpbuf_eq_send)\n");
	printf("  -g          : rpbuf recv name (default: rpbuf_eq_recv)\n");
	printf("  -e          : enable equalizer	 	(default: 0)\n");
	printf("  -l          : test audio render(1: test eq, 2: add dump)\n");
	printf("  -I ID       : specify rpbuf ctrl ID (default: 0)\n");
	printf("  -L LENGTH   : specify buffer length (default: %d bytes)\n",
			EQ_RPBUF_BUFFER_LENGTH_DEFAULT);
	printf("\n");
	printf("e.g.\n");
	printf("  aeq_test -I 0 -L 4096 -l 1 -e 1 \n");
	printf("  aeq_test -I 0 -L 4096 -l 2 -e 1 -s \"rp_eq_start\" -d \"rp_eq_reset\" -p \"rp_eq_send\" -g \"rp_eq_recv\" \n");
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

static OMX_ERRORTYPE alloc_aeq_buffer(aeqPrivateType *priv, int port_index, OMX_S32 num, OMX_S32 size)
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
		ret = OMX_AllocateBuffer(priv->aeq_handle, &buffer[i],
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

static int free_aeq_buffer(aeqPrivateType *priv, int port_index, OMX_S32 num)
{
	OMX_S32 i = 0;
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE **buffer;


	if (priv->aeq_handle) {


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
					ret = OMX_FreeBuffer(priv->aeq_handle,
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

static void print_aeq_prms(const eq_prms_t *prms)
{
    int i;
    for (i = 0; i < prms->biq_num; ++i) {
        const eq_core_prms_t *core_prms = &prms->core_prms[i];
        printf(" [Biquad%02i] type: %i, freq: %d, gain: %d, Q: %.2f\n",
                i + 1, core_prms->type, core_prms->fc, core_prms->G, core_prms->Q);
    }
}

static int untunnel_config_aeq_component(aeqPrivateType *priv)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_PORT_PARAM_TYPE sParam;
	OMX_OTHER_PARAM_EQTYPE sEqParams;
	OMX_AUDIO_PARAM_PCMMODETYPE sAudioParams;
	OMX_AUDIO_CONFIG_EQUALIZERTYPE sEqConfig;

	memset(&priv->aeq_port_para[priv->port_filter_in], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	/* set input port */
	priv->aeq_port_para[priv->port_filter_in].nBufferCountActual = DEFAULT_AEQUALIZER_BUF_CNT;
	priv->aeq_port_para[priv->port_filter_in].bBuffersContiguous = 1;
	priv->aeq_port_para[priv->port_filter_in].eDomain = OMX_PortDomainAudio;
	priv->aeq_port_para[priv->port_filter_in].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->aeq_port_para[priv->port_filter_in].nPortIndex = priv->port_filter_in;
	priv->aeq_port_para[priv->port_filter_in].nBufferSize = priv->aeq_test->aeq_len;

	setHeader(&priv->aeq_port_para[priv->port_filter_in], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->aeq_handle, OMX_IndexParamPortDefinition,
			&priv->aeq_port_para[priv->port_filter_in]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	/* set input port eq param */
	memset(&sEqParams, 0, sizeof(OMX_OTHER_PARAM_EQTYPE));
	sEqParams.nPortIndex = priv->port_filter_in;
	sEqParams.pEqprm = &priv->eq_prms[0];

	setHeader(&sEqParams, sizeof(OMX_OTHER_PARAM_EQTYPE));
	ret = OMX_SetParameter(priv->aeq_handle, OMX_IndexVendorParamEQ,
			&sEqParams);
	if (ret) {
		printf("set eq params error!");
		return ret;
	}

	/* set input port pcm param */
	memset(&sAudioParams, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	sAudioParams.nPortIndex = priv->port_filter_in;
	sAudioParams.nChannels = priv->eq_prms[0].chan;
	sAudioParams.nBitPerSample = priv->eq_prms[0].bits_per_sample;
	sAudioParams.nSamplingRate = priv->eq_prms[0].sampling_rate;

	setHeader(&sAudioParams, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	ret = OMX_SetParameter(priv->aeq_handle, OMX_IndexParamAudioPcm,
			&sAudioParams);
	if (ret) {
		printf("set audio params error!");
		return ret;
	}

	printf("Audio param chan %d, bits_per_sample %d, sampling_rate %d eq enable %d\n", \
		priv->eq_prms[0].chan, priv->eq_prms[0].bits_per_sample, priv->eq_prms[0].sampling_rate, priv->aeq_test->aeq_enable);

	print_aeq_prms((eq_prms_t*)&priv->eq_prms[0]);

	/* set input port eq config */
	memset(&sEqConfig, 0, sizeof(OMX_AUDIO_CONFIG_EQUALIZERTYPE));
	sEqConfig.nPortIndex = priv->port_filter_in;
	sEqConfig.bEnable = priv->aeq_test->aeq_enable;
	ret = OMX_SetConfig(priv->aeq_handle, OMX_IndexConfigAudioEqualizer,
			&sEqConfig);
	if (ret) {
		printf("set eq_config error!");
		return ret;
	}

	/* set output port */
	memset(&priv->aeq_port_para[priv->port_filter_out], 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	priv->aeq_port_para[priv->port_filter_out].nBufferCountActual = DEFAULT_AEQUALIZER_BUF_CNT;
	priv->aeq_port_para[priv->port_filter_out].bBuffersContiguous = 1;
	priv->aeq_port_para[priv->port_filter_out].eDomain = OMX_PortDomainAudio;
	priv->aeq_port_para[priv->port_filter_out].format.audio.eEncoding = OMX_AUDIO_CodingPCM;
	priv->aeq_port_para[priv->port_filter_out].nPortIndex = priv->port_filter_out;
	priv->aeq_port_para[priv->port_filter_out].nBufferSize = priv->aeq_test->aeq_len;

	setHeader(&priv->aeq_port_para[priv->port_filter_out], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	ret = OMX_SetParameter(priv->aeq_handle, OMX_IndexParamPortDefinition,
			&priv->aeq_port_para[priv->port_filter_out]);
	if (ret) {
		printf("set port params error!");
		return ret;
	}

	/** Get the number of ports */
	setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
	ret = OMX_GetParameter(priv->aeq_handle, OMX_IndexParamAudioInit, &sParam);
	if(ret != OMX_ErrorNone){
		printf("Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		return ret;
	}
	printf("Audio equalizer has %d ports\n",(int)sParam.nPorts);

	return ret;
}

static void aeq_send_thread(void *params)
{
	aeqPrivateType *private = (aeqPrivateType *)params;
	unsigned char *data = NULL;
	int ret = OMX_ErrorNone;
	int read_bytes = 0;
	unsigned int data_len = private->aeq_test->aeq_len;
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

			read_bytes = hal_ringbuffer_get(private->aeq_out_rb, data + offset, read_size, 1000);
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

			ret = hal_event_wait(private->send_event, AEQ_EV_DATA_SET,
				HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
			if (!ret) {
				ret = -1;
				printf("send wait data timeout:%d\n", timeout);
				continue;
			}
		}


		hal_event_set_bits(private->send_event, AEQ_EV_DATA_GET);

		if (read_len == 0 && private->aeq_eof == OMX_TRUE) {
			printf("exit aec send loop!\n");
			break;
		}

		if (data_len != read_len)
		{
			printf("send data len %d is not equal to read_bytes %d\n", data_len, read_len);
			goto exit;
		}

		if (read_len > 0) {
			ret = rpbuf_common_transmit(private->aeq_test->aeq_arg.aeq_out_name, data, data_len, 0);
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

static void aeq_process_thread(void *params)
{
	OMX_BUFFERHEADERTYPE *pBuffer = NULL;
	aeqPrivateType *private = (aeqPrivateType *)params;
	int ret = OMX_ErrorNone;
	int read_bytes = 0;
	unsigned int data_len = private->aeq_test->aeq_len;
	OMX_BOOL eof = OMX_FALSE;
	int timeout = 300;
	int cnt = 0;
	int read_size = 0;
	int offset = 0;
	int read_len = 0;


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

		if (pBuffer->nAllocLen != data_len) {
			printf("data len %d is not equal to nAllocLen %lu\n", data_len, pBuffer->nAllocLen);
			break;
		}


		read_size = data_len;
		offset = 0;
		read_len = 0;
		cnt = 0;

		while (read_size > 0) {

			read_bytes = hal_ringbuffer_get(private->aeq_rb, pBuffer->pBuffer + offset, read_size, 1000);
			if(read_bytes < 0) {
				printf("read err:%d\n", read_bytes);
				goto exit;
			}

			read_len += read_bytes;

			if (read_bytes == read_size)
				break;

			if (read_bytes == 0) {
				cnt++;
				if (cnt > 5) {
					cnt = 0;
					break;
				}
			}

			offset += read_bytes;
			read_size -= read_bytes;

			ret = hal_event_wait(private->event, AEQ_EV_DATA_SET,
				HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
			if (!ret) {
				ret = -1;
				printf("wait data timeout:%d\n", timeout);
				continue;
			}
		}

		hal_event_set_bits(private->event, AEQ_IN_EV_DATA_GET);

		if (read_len > 0 && data_len != read_len)
		{
			printf("data len %d is not equal to read_bytes %d\n", data_len, read_len);
			goto exit;
		}

		if (read_len == 0 && private->aeq_eof == OMX_TRUE) {
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

		if (read_len || (private->aeq_eof == OMX_TRUE)) {
			pBuffer->nInputPortIndex = private->port_filter_in;
			ret = OMX_EmptyThisBuffer(private->aeq_handle, pBuffer);
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


	printf("end of aec process thread\n");
	vTaskDelete(NULL);
}

static void aeq_task(void *arg)
{
	aeq_test_data *aeq_test = (aeq_test_data *)arg;
	int ret = -1;
	int i = 0;
	TaskHandle_t handle;
	TaskHandle_t recv_handle;
	aeqPrivateType *priv = NULL;

	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE portin_def;
	OMX_PARAM_PORTDEFINITIONTYPE portout_def;

	/* Initialize application private data */
	priv = malloc(sizeof(aeqPrivateType));
	if (!priv) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}
	memset(priv, 0 ,sizeof(aeqPrivateType));

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
	priv->aeq_eventSem = malloc(sizeof(omx_sem_t));
	if (!priv->aeq_eventSem) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->aeq_eventSem, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	/* init aeq_start */
	priv->aeq_start = malloc(sizeof(omx_sem_t));
	if (!priv->aeq_start) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->aeq_start, 0);
	if (ret < 0) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	/* init aeq_reset */
	priv->aeq_reset = malloc(sizeof(omx_sem_t));
	if (!priv->aeq_reset) {
		printf("[%s] line:%d \n", __func__, __LINE__);
		goto exit;
	}

	ret = omx_sem_init(priv->aeq_reset, 0);
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

	priv->aeq_test = aeq_test;

	priv->aeq_rb = hal_ringbuffer_init(aeq_test->aeq_len * 4); // 40 * 4 ms
	if (!priv->aeq_rb) {
		printf("create ringbuffer err");
		goto exit;
	}

	priv->aeq_out_rb = hal_ringbuffer_init(aeq_test->aeq_len * 2); // 40 * 2 ms
	if (!priv->aeq_out_rb) {
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

	ret = rpbuf_common_create(aeq_test->aeq_ctrl_id, aeq_test->aeq_arg.aeq_config_name, sizeof(eq_remote_prms_t), 0, &rpbuf_aeq_config_cbs, priv);
	if (ret < 0) {
		printf("rpbuf_create for name %s (len: %d) failed\n", aeq_test->aeq_arg.aeq_config_name, sizeof(eq_remote_prms_t));
		goto exit;
	}

	ret = rpbuf_common_create(aeq_test->aeq_ctrl_id, aeq_test->aeq_arg.aeq_reset_config_name, sizeof(eq_remote_prms_t), 0, &rpbuf_aeq_config_reset_cbs, priv);
	if (ret < 0) {
		 printf("rpbuf_create for name %s (len: %d) failed\n", aeq_test->aeq_arg.aeq_reset_config_name, sizeof(eq_remote_prms_t));
		 goto exit;
	}

	ret = rpbuf_common_create(aeq_test->aeq_ctrl_id, aeq_test->aeq_arg.aeq_in_name, aeq_test->aeq_len, 1, &rpbuf_aeq_in_cbs, priv);
	if (ret < 0) {
		 printf("rpbuf_create for name %s (len: %d) failed\n", aeq_test->aeq_arg.aeq_in_name, aeq_test->aeq_len);
		 goto exit;
	}

	ret = rpbuf_common_create(aeq_test->aeq_ctrl_id, aeq_test->aeq_arg.aeq_out_name, aeq_test->aeq_len, 0, &rpbuf_aeq_out_cbs, priv);
	if (ret < 0) {
		 printf("rpbuf_create for name %s (len: %d) failed\n", aeq_test->aeq_arg.aeq_out_name, aeq_test->aeq_len);
		 goto exit;
	}

	omx_sem_down(priv->aeq_start);


	/* 1. get component handle */
	err = OMX_GetHandle(&priv->aeq_handle, "OMX.audio.equalizer", priv, &aeq_untunnel_callbacks);
	if(err != OMX_ErrorNone) {
		printf("Audio equalizer OMX_GetHandle failed\n");
		goto exit;
	}

	priv->port_filter_in = get_port_index(priv->aeq_handle, OMX_DirInput, OMX_PortDomainAudio, 0);
	priv->port_filter_out = get_port_index(priv->aeq_handle, OMX_DirOutput, OMX_PortDomainAudio, 0);
	if (priv->port_filter_in < 0 || priv->port_filter_out < 0) {
		printf("Error in get port index, port_filter_in %d port_filter_out %d\n", priv->port_filter_in, priv->port_filter_out);
		ret = OMX_ErrorBadPortIndex;
		goto exit;
	}

	/* 2. config component */
	err = untunnel_config_aeq_component(priv);
	if(err != OMX_ErrorNone){
		printf("Error in untunnel config aeq component\n");
		goto exit;
	}

	/* 3. set component stat to idle */
	err = OMX_SendCommand(priv->aeq_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}

	portin_def.nPortIndex = priv->port_filter_in;
	setHeader(&portin_def, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	err = OMX_GetParameter(priv->aeq_handle, OMX_IndexParamPortDefinition, &portin_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}

	if ((priv->aeq_port_para[priv->port_filter_in].nBufferSize != portin_def.nBufferSize) ||
		(priv->aeq_port_para[priv->port_filter_in].nBufferCountActual != portin_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize %ld nBufferCountActual %ld\n", \
			priv->aeq_port_para[priv->port_filter_in].nBufferSize, priv->aeq_port_para[priv->port_filter_in].nBufferCountActual , \
			portin_def.nBufferSize, portin_def.nBufferCountActual);
		goto exit;
	}
	err = alloc_aeq_buffer(priv, priv->port_filter_in, portin_def.nBufferCountActual, portin_def.nBufferSize);
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
	err = OMX_GetParameter(priv->aeq_handle, OMX_IndexParamPortDefinition, &portout_def);
	if (err != OMX_ErrorNone) {
		printf("Error when getting OMX_PORT_PARAM_TYPE,%x\n", err);
		goto exit;
	}

	if ((priv->aeq_port_para[priv->port_filter_out].nBufferSize != portout_def.nBufferSize) ||
		(priv->aeq_port_para[priv->port_filter_out].nBufferCountActual != portout_def.nBufferCountActual)) {
		printf("Error port nBufferSize %ld port nBufferCountActual %ld nBufferSize %ld nBufferCountActual %ld\n", \
			priv->aeq_port_para[priv->port_filter_out].nBufferSize, priv->aeq_port_para[priv->port_filter_out].nBufferCountActual , \
			portout_def.nBufferSize, portout_def.nBufferCountActual);
		goto exit;
	}

	err = alloc_aeq_buffer(priv, priv->port_filter_out, portout_def.nBufferCountActual, portout_def.nBufferSize);
	if (err != OMX_ErrorNone) {
		printf("Error when alloc_buffer,%x\n", err);
		goto exit;
	}
	omx_sem_down(priv->aeq_eventSem);

	/* 4. set component stat to executing */
	err = OMX_SendCommand(priv->aeq_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateExecuting\n");
		goto exit;
	}
	/* Wait for commands to complete */
	omx_sem_down(priv->aeq_eventSem);

	/* 5. send buffer to source component queue */
	for (i = 0; i < portout_def.nBufferCountActual; i++) {
		err = OMX_FillThisBuffer(priv->aeq_handle, priv->bufferout[i]);
		printf("OMX_FillThisBuffer %p on port %lu\n", priv->bufferout[i],
			priv->bufferout[i]->nOutputPortIndex);
		if(err != OMX_ErrorNone){
			printf("Error in send OMX_FillThisBuffer\n");
			goto exit;
		}
	}

	ret = rpbuf_common_transmit(priv->aeq_test->aeq_arg.aeq_config_name, (void *)&priv->eq_prms[0], sizeof(eq_remote_prms_t), 0);
	if (ret < 0) {
		printf("rpbuf_common_transmit config name (with data) failed\n");
	}

	xTaskCreate(aeq_process_thread, "aeq_process_thread", 2048, (void *)priv,
			configAPPLICATION_OMX_PRIORITY,
			&handle);

	xTaskCreate(aeq_send_thread, "aeq_send_thread", 2048, (void *)priv,
			configAPPLICATION_OMX_PRIORITY,
			&recv_handle);

	printf("audio eq test wait for eos\n");

	omx_sem_down(priv->eofSem);

	printf("audio eq test get eos signal\n");

	/* 6. set component stat to idle */
	ret = OMX_SendCommand(priv->aeq_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateIdle\n");
		goto exit;
	}
	omx_sem_down(priv->aeq_eventSem);

	/* 7. free buffer */
	free_aeq_buffer(priv, priv->port_filter_in, portin_def.nBufferCountActual);
	free_aeq_buffer(priv, priv->port_filter_out, portout_def.nBufferCountActual);

	/* 8. set component stat to loaded */
	ret = OMX_SendCommand(priv->aeq_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
	if(err != OMX_ErrorNone){
		printf("Error in send OMX_CommandStateSet OMX_StateLoaded\n");
		goto exit;
	}
	omx_sem_down(priv->aeq_eventSem);

exit:
	printf("audio eq task exit\n");

	free_aeq_buffer(priv, priv->port_filter_in, portin_def.nBufferCountActual);
	free_aeq_buffer(priv, priv->port_filter_out, portout_def.nBufferCountActual);

	ret = rpbuf_common_destroy(aeq_test->aeq_arg.aeq_config_name);
	if (ret < 0) {
		printf("rpbuf_destroy for name %s (len: %d) failed\n", aeq_test->aeq_arg.aeq_config_name, sizeof(eq_prms_t));
	}
	ret = rpbuf_common_destroy(aeq_test->aeq_arg.aeq_reset_config_name);
	if (ret < 0) {
		printf("rpbuf_destroy for name %s (len: %d) failed\n", aeq_test->aeq_arg.aeq_reset_config_name, sizeof(eq_prms_t));
	}
	ret = rpbuf_common_destroy(aeq_test->aeq_arg.aeq_in_name);
	if (ret < 0) {
		printf("rpbuf_destroy for name %s (len: %d) failed\n", aeq_test->aeq_arg.aeq_in_name, aeq_test->aeq_len);
	}
	ret = rpbuf_common_destroy(aeq_test->aeq_arg.aeq_out_name);
	if (ret < 0) {
		printf("rpbuf_destroy for name %s (len: %d) failed\n", aeq_test->aeq_arg.aeq_out_name, aeq_test->aeq_len);
	}

	rpbuf_common_deinit();

	if (priv->aeq_rb) {
		hal_ringbuffer_release(priv->aeq_rb);
		priv->aeq_rb = NULL;
	}

	if (priv->aeq_out_rb) {
		hal_ringbuffer_release(priv->aeq_out_rb);
		priv->aeq_out_rb = NULL;
	}

	if (priv->send_event) {
		hal_event_delete(priv->send_event);
		priv->send_event = NULL;
	}

	if (priv->event) {
		hal_event_delete(priv->event);
		priv->event = NULL;
	}

	if (priv->aeq_eventSem) {
		omx_sem_deinit(priv->aeq_eventSem);
		free(priv->aeq_eventSem);
		priv->aeq_eventSem = NULL;
	}

	if (priv->aeq_start) {
		omx_sem_deinit(priv->aeq_start);
		free(priv->aeq_start);
		priv->aeq_start = NULL;
	}

	if (priv->aeq_reset) {
		omx_sem_deinit(priv->aeq_reset);
		free(priv->aeq_reset);
		priv->aeq_reset = NULL;
	}

	if (priv->eofSem) {
		omx_sem_deinit(priv->eofSem);
		free(priv->eofSem);
		priv->eofSem = NULL;
	}
	if (priv->aeq_handle) {
		OMX_FreeHandle(priv->aeq_handle);
		priv->aeq_handle = NULL;
	}
	if (priv->BufferQueue) {
		queue_deinit(priv->BufferQueue);
		free(priv->BufferQueue);
		priv->BufferQueue = NULL;
	}

	OMX_Deinit();

	if (priv) {
		free(priv);
		priv = NULL;
	}
	if (aeq_test) {
		free(aeq_test);
		aeq_test = NULL;
	}
	g_aeq_play = 0;
	printf("aeq task test finish\n");
	vTaskDelete(NULL);
}

static int audio_equalizer_task_create(rpbuf_arg_aeq *targ)
{
	TaskHandle_t handle;
	OMX_ERRORTYPE err;
	aeq_test_data *aeq_test;

	err = OMX_Init();
	if(err != OMX_ErrorNone) {
		printf("OMX_Init() failed\n");
		return -1;
	}

	aeq_test = malloc(sizeof(aeq_test_data));
	if (aeq_test)
	{
		memset(aeq_test, 0 , sizeof(aeq_test_data));
		aeq_test->aeq_len = g_aeq_len;
		aeq_test->aeq_ctrl_id = g_aeq_ctrl_id;
		aeq_test->aeq_enable = g_aeq_enable;
		memcpy(&aeq_test->aeq_arg, targ, sizeof(rpbuf_arg_aeq));
	}
	else
	{
		printf("malloc aeq_test failed\n");
		return -1;
	}


	xTaskCreate(aeq_task, "aeq_task", 2048, aeq_test,
			configAPPLICATION_OMX_PRIORITY,
			&handle);
	return 0;
}

static int cmd_aeq_test(int argc, char *argv[])
{
	int c;
	int ret = 0;

	rpbuf_arg_aeq targ = {
		.aeq_config_name =       "rpbuf_eq_config",
		.aeq_reset_config_name = "rpbuf_eq_reset",
		.aeq_in_name  =          "rpbuf_eq_in",
		.aeq_out_name =          "rpbuf_eq_out",
	};

	g_aeq_len = EQ_RPBUF_BUFFER_LENGTH_DEFAULT;
	g_aeq_ctrl_id = 0;
	g_aeq_enable = 0;

	optind = 0;
	while ((c = getopt(argc, argv, "hs:d:p:g:e:l:I:L:")) != -1) {
		switch (c) {
		case 'h':
			ret = 0;
			goto usage;
		case 's':
			strncpy(targ.aeq_config_name, optarg, sizeof(targ.aeq_config_name));
			break;
		case 'd':
			strncpy(targ.aeq_reset_config_name, optarg, sizeof(targ.aeq_reset_config_name));
			break;
		case 'p':
			strncpy(targ.aeq_in_name, optarg, sizeof(targ.aeq_in_name));
			break;
		case 'g':
			strncpy(targ.aeq_out_name, optarg, sizeof(targ.aeq_out_name));
			break;
		case 'e':
			g_aeq_enable = atoi(optarg);
			break;
		case 'l':
			g_aeq_play = atoi(optarg);
			break;
		case 'I':
			g_aeq_ctrl_id = atoi(optarg);
			break;
		case 'L':
			g_aeq_len = strtol(optarg, NULL, 0);
			if (g_aeq_len == -ERANGE) {
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

	printf("g_aeq_play %d \n", g_aeq_play);

	if (g_aeq_play == 0)
		goto usage;

	printf("test start\n");

	switch (g_aeq_play) {

		case 1:
			audio_equalizer_task_create(&targ);
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
	aeq_demo_usage();
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_aeq_test, aeq_test, eq test);

