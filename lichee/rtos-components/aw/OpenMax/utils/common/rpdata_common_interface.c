/*
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
#include "FreeRTOS.h"
#include <hal_time.h>
#include <rpdata.h>

#include "OMX_Base.h"
#include "asr_test.h"
#include "asr_demo_config.h"

#include "rpdata_common_interface.h"

#if (defined CONFIG_COMPONENTS_PROCESS_ASR) && (defined CONFIG_XTENSA_HIFI5)

static int rpd_dsp_ctrl(rpdata_t *rpd, void *data, int data_len)
{
	dsp_ctrl_init_t dsp_ctrl_init;

	DSP_CTRL_TYPE dsp_ctrl_type;

	rpdata_arg_asr targ = {
		.type = "DSPtoRVAsr",
		.name = "RVrecvDSPsend",
		.dir  = RPDATA_DIR_RV,
	};

	rpdata_arg_asr targ_dump = {
		.type = "DSPtoRVAsrdump",
		.name = "dRVrecvDSPsend",
		.dir  = RPDATA_DIR_RV,
	};

	if (data_len != sizeof(dsp_ctrl_init_t)) {
		omx_err("dsp ctrl init get rpdata len %d err!", data_len);
		return -1;
	}

	memcpy(&dsp_ctrl_init, data, data_len);

	dsp_ctrl_type = dsp_ctrl_init.cmd;

	switch(dsp_ctrl_type)
	{
		case DSP_START:
		{
			//asr start, alg is not work, must be enable alg.
			rpdata_init_param();
			do_rpdata_asr_send(&targ, &targ_dump);
			break;
		}

		case DSP_STOP:
		{
			//asr stop
			do_rpdata_asr_stop_send();
			rpdata_deinit_param();
			rpdata_disable_asr();
			rpdata_disable_dump();
			rpdata_disable_dump_merge();
			break;
		}
		case DSP_ENABLE_ALG:
		{
			// enable alg.
			rpdata_enable_asr();
			break;
		}
		case DSP_DUMP_MERGE_DATA:
		{
			// enable raw data dump.
			rpdata_enable_dump_merge();
			break;
		}

		case DSP_DUMP_RAW_DATA:
		{
			// enable raw data dump.
			rpdata_enable_dump();
			break;
		}
		default:
		{
			omx_err("dsp ctrl get type %d err!", dsp_ctrl_type);
			break;
		}

	 }
	return 0;
}

#else
static int rpd_dsp_ctrl(rpdata_t *rpd, void *data, int data_len)
{
	return 0;
}


#endif

struct rpdata_cbs rpd_dsp_ctrl_init_cbs = {
	.recv_cb = (recv_cb_t)rpd_dsp_ctrl,
};

void RpdataThread(void *arg)
{

	rpdata_t *rpd = NULL;
	void *buffer = NULL;

	rpdata_arg_t arg_init = {
        .type = RPD_CTL_TYPE,
        .name = RPD_CTL_NAME,
        .dir = RPDATA_DIR_RV,
    };

	omx_debug("start");

	/* process dsp ctrl init */
	rpd = rpdata_connect(arg_init.dir, arg_init.type, arg_init.name);
	if (!rpd) {
		omx_err("rpdata connect failed");
		goto err_rpdata_connect_failed;
	}
	omx_debug("");

	buffer = rpdata_buffer_addr(rpd);
	if (!buffer) {
		omx_err("rpdata buffer addr failed");
		goto err_rpdata_connect_failed;
	}

	rpdata_set_recv_cb(rpd, &rpd_dsp_ctrl_init_cbs);

	while (1) {
		hal_msleep(10000);
	}

err_rpdata_connect_failed:
	if (rpd)
		rpdata_destroy(rpd);

	vTaskDelete(NULL);

	omx_debug("end");

}

int rpdata_ctrl_init(void)
{
	int ret = 0;

	ret = xTaskCreate(RpdataThread, "rpdata-ctrl", 1024, NULL,
			configAPPLICATION_OMX_PRIORITY,
			NULL);
	if (ret != pdPASS) {
		omx_err("task create failed");
		return ret;
	}

	return ret;
}

