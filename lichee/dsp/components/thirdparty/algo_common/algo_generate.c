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
#include "algo_generate.h"

extern struct arpaf_priv *arpaf_priv;

#ifdef AW_ARPAF_COMPONENT_SIMULATOR
int arpaf_pcm_data_simulator_fill(struct snd_soc_dsp_native_component *native_component,
			void *buffer, unsigned int size, unsigned int offset)
{
	int i = 0;

	for (i = 0; i < size; i++)
	{
		*((char *)buffer + i) = (i + offset)%256;
	}
	return 0;
}

int arpaf_pcm_data_simulator_check(struct snd_soc_dsp_native_component *native_component,
			void *buffer, unsigned int size, unsigned int offset)
{
	struct snd_soc_dsp_component *soc_component = &native_component->soc_component;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_component->params;
	int i = 0;
	for (i = 0; i < size; i++)
	{
		if (*((char *)buffer + i) != (i + offset)%256) {
			printf("%s format:%d, frames:%d check data:0x%x != 0x%x failed.\n",
						__func__, pcm_params->format, i, (i + offset)%256,
						*((char *)buffer + i));
			break;
		}
	}
	return 0;
}
#endif

/*
 * 重采样算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_resample_create(void **handle, void *native_component)
{
	struct snd_soc_dsp_native_component *component = NULL;
	/* only for demo */
	component = rpaf_malloc(sizeof(struct snd_soc_dsp_native_component));
	memcpy(component, native_component, sizeof(struct snd_soc_dsp_native_component));
	*handle = component;
	awrpaf_debug("\n");

	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_resample_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
	/* 从input_buffer获得数据并输出到output_buffer上，因为两者的buffer一定不一样 */
	struct snd_soc_dsp_native_component *native_component = handle;

	UNUSED(native_component);

#ifdef AW_ARPAF_COMPONENT_SIMULATOR
	arpaf_pcm_data_simulator_fill(native_component, input_buffer, *input_size,
					SND_DSP_COMPONENT_RESAMPLE);
#endif
	memcpy(output_buffer, input_buffer, *input_size);
	*output_size = *input_size;

	awrpaf_debug("input_buffer:%p, size;%u, output_buffer:%p, size:%u\n",
				input_buffer, *input_size, output_buffer, *output_size);

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_resample_release(void *handle)
{
	if (handle)
		rpaf_free(handle);
	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_resample = {
	.create = snd_dsp_component_resample_create,
	.process = snd_dsp_component_resample_process,
	.release = snd_dsp_component_resample_release,
};

/*
 * agc算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_agc_create(void **handle, void *native_component)
{
	struct snd_soc_dsp_native_component *component = NULL;
	/* only for demo */
	component = rpaf_malloc(sizeof(struct snd_soc_dsp_native_component));
	memcpy(component, native_component, sizeof(struct snd_soc_dsp_native_component));
	*handle = component;
	awrpaf_debug("\n");

	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_agc_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
	/* for test */
	struct snd_soc_dsp_native_component *native_component = handle;

	UNUSED(native_component);
#ifdef AW_ARPAF_COMPONENT_SIMULATOR
	arpaf_pcm_data_simulator_fill(native_component, input_buffer, *input_size,
					SND_DSP_COMPONENT_AGC);
#endif

	memcpy(output_buffer, input_buffer, *input_size);
	*output_size = *input_size;

//	awrpaf_debug("\n");

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_agc_release(void *handle)
{
	if (handle)
		rpaf_free(handle);
	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_agc = {
	.create = snd_dsp_component_agc_create,
	.process = snd_dsp_component_agc_process,
	.release = snd_dsp_component_agc_release,
};

/*
 * eq算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_aec_create(void **handle, void *native_component)
{
	struct snd_soc_dsp_native_component *component = NULL;
	/* only for demo */
	component = rpaf_malloc(sizeof(struct snd_soc_dsp_native_component));
	memcpy(component, native_component, sizeof(struct snd_soc_dsp_native_component));
	*handle = component;
	awrpaf_debug("\n");
	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_aec_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
	struct snd_soc_dsp_native_component *native_component = handle;

	UNUSED(native_component);

#ifdef AW_ARPAF_COMPONENT_SIMULATOR
	arpaf_pcm_data_simulator_fill(native_component, input_buffer, *input_size,
					SND_DSP_COMPONENT_AEC);
#endif

	memcpy(output_buffer, input_buffer, *input_size);
	*output_size = *input_size;

//	awrpaf_debug("\n");

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_aec_release(void *handle)
{
	if (handle)
		rpaf_free(handle);
	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_aec = {
	.create = snd_dsp_component_aec_create,
	.process = snd_dsp_component_aec_process,
	.release = snd_dsp_component_aec_release,
};

/*
 * ns算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_ns_create(void **handle, void *native_component)
{
	struct snd_soc_dsp_native_component *component = NULL;
	/* only for demo */
	component = rpaf_malloc(sizeof(struct snd_soc_dsp_native_component));
	memcpy(component, native_component, sizeof(struct snd_soc_dsp_native_component));
	*handle = component;
	awrpaf_debug("\n");

	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_ns_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
	struct snd_soc_dsp_native_component *native_component = handle;

	UNUSED(native_component);

#ifdef AW_ARPAF_COMPONENT_SIMULATOR
	arpaf_pcm_data_simulator_fill(native_component, input_buffer, *input_size,
					SND_DSP_COMPONENT_NS);
#endif

	memcpy(output_buffer, input_buffer, *input_size);
	*output_size = *input_size;

//	awrpaf_debug("\n");

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_ns_release(void *handle)
{
	if (handle)
		rpaf_free(handle);

	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_ns = {
	.create = snd_dsp_component_ns_create,
	.process = snd_dsp_component_ns_process,
	.release = snd_dsp_component_ns_release,
};

/*
 * eq算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_eq_create(void **handle, void *native_component)
{
	struct snd_soc_dsp_native_component *component = NULL;
	/* only for demo */
	component = rpaf_malloc(sizeof(struct snd_soc_dsp_native_component));
	memcpy(component, native_component, sizeof(struct snd_soc_dsp_native_component));
	*handle = component;

	awrpaf_debug("\n");

	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_eq_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
	struct snd_soc_dsp_native_component *native_component = handle;

	UNUSED(native_component);

#ifdef AW_ARPAF_COMPONENT_SIMULATOR
	arpaf_pcm_data_simulator_fill(native_component, input_buffer, *input_size,
					SND_DSP_COMPONENT_EQ);
#endif

	memcpy(output_buffer, input_buffer, *input_size);
	*output_size = *input_size;

	awrpaf_debug("input_buffer:%p, size;%u, output_buffer:%p, size:%u\n",
				input_buffer, *input_size, output_buffer, *output_size);

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_eq_release(void *handle)
{
	if (handle)
		rpaf_free(handle);

	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_eq = {
	.create = snd_dsp_component_eq_create,
	.process = snd_dsp_component_eq_process,
	.release = snd_dsp_component_eq_release,
};

/*
 * DRC算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_drc_create(void **handle, void *native_component)
{
	struct snd_soc_dsp_native_component *component = NULL;
	/* only for demo */
	component = rpaf_malloc(sizeof(struct snd_soc_dsp_native_component));
	memcpy(component, native_component, sizeof(struct snd_soc_dsp_native_component));
	*handle = component;
	awrpaf_debug("\n");

	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_drc_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
	struct snd_soc_dsp_native_component *native_component = handle;

	UNUSED(native_component);

#ifdef AW_ARPAF_COMPONENT_SIMULATOR
	arpaf_pcm_data_simulator_fill(native_component, input_buffer, *input_size,
					SND_DSP_COMPONENT_DRC);
#endif

	memcpy(output_buffer, input_buffer, *input_size);
	*output_size = *input_size;

	awrpaf_debug("input_buffer:%p, size;%u, output_buffer:%p, size:%u\n",
				input_buffer, *input_size, output_buffer, *output_size);

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_drc_release(void *handle)
{
	if (handle)
		rpaf_free(handle);

	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_drc = {
	.create = snd_dsp_component_drc_create,
	.process = snd_dsp_component_drc_process,
	.release = snd_dsp_component_drc_release,
};

/*
 * 混响算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_reverb_create(void **handle, void *native_component)
{
	awrpaf_debug("\n");

	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_reverb_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
	struct snd_soc_dsp_native_component *native_component = handle;

	UNUSED(native_component);

#ifdef AW_ARPAF_COMPONENT_SIMULATOR
	arpaf_pcm_data_simulator_fill(native_component, input_buffer, *input_size,
					SND_DSP_COMPONENT_DRC);
#endif

	memcpy(output_buffer, input_buffer, *input_size);
	*output_size = *input_size;

	awrpaf_debug("input_buffer:%p, size;%u, output_buffer:%p, size:%u\n",
				input_buffer, *input_size, output_buffer, *output_size);

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_reverb_release(void *handle)
{
	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_reverb = {
	.create = snd_dsp_component_reverb_create,
	.process = snd_dsp_component_reverb_process,
	.release = snd_dsp_component_reverb_release,
};

/*
 * 解码算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_decodec_create(void **handle, void *native_component)
{
	awrpaf_debug("\n");

	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_decodec_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
	/* 从input_buffer获得数据并输出到output_buffer上，因为两者的buffer一定不一样 */
//	awrpaf_debug("\n");

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_decodec_release(void *handle)
{
	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_decodec = {
	.create = snd_dsp_component_decodec_create,
	.process = snd_dsp_component_decodec_process,
	.release = snd_dsp_component_decodec_release,
};

/*
 * 其它算法
 */
/* 分配自身算法需要的结构空间变量和配置参数 */
int snd_dsp_component_other_create(void **handle, void *native_component)
{
	awrpaf_debug("\n");

	return 0;
}

/* 实现该算法处理input_buffer, 最后填写output_buffer */
int snd_dsp_component_other_process(void *handle, void *input_buffer,
			unsigned int * const input_size, void *output_buffer,
			unsigned int * const output_size)
{
//	awrpaf_debug("\n");

	return 0;
}

/* 算法资源释放 */
int snd_dsp_component_other_release(void *handle)
{
	awrpaf_debug("\n");

	return 0;
}

struct snd_dsp_hal_component_process_driver snd_dsp_component_other = {
	.create = snd_dsp_component_other_create,
	.process = snd_dsp_component_other_process,
	.release = snd_dsp_component_other_release,
};

/* 可以在DSP启动流程中添加该函数，进行算法组件的安装 */
int algo_generate_install(void)
{
	/* 设置算法通用配置，系统启动后也可以通过CPUX进行修改 */
	snd_dsp_hal_component_install(&snd_dsp_component_resample, SND_DSP_COMPONENT_RESAMPLE);
	snd_dsp_hal_component_install(&snd_dsp_component_agc, SND_DSP_COMPONENT_AGC);
	snd_dsp_hal_component_install(&snd_dsp_component_aec, SND_DSP_COMPONENT_AEC);
	snd_dsp_hal_component_install(&snd_dsp_component_ns, SND_DSP_COMPONENT_NS);
	snd_dsp_hal_component_install(&snd_dsp_component_decodec, SND_DSP_COMPONENT_DECODEC);
	snd_dsp_hal_component_install(&snd_dsp_component_drc, SND_DSP_COMPONENT_DRC);
	snd_dsp_hal_component_install(&snd_dsp_component_reverb, SND_DSP_COMPONENT_REVERB);
	snd_dsp_hal_component_install(&snd_dsp_component_other, SND_DSP_COMPONENT_OTHER);
	snd_dsp_hal_component_install(&snd_dsp_component_eq, SND_DSP_COMPONENT_EQ);
	return 0;
}
