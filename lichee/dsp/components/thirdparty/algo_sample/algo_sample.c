#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <aw_rpaf/common.h>
#include <aw_rpaf/component.h>

int snd_dsp_hal_component_install(struct snd_dsp_hal_component_process_driver *algo_comp,
				enum SND_DSP_COMPONENT_TYPE type);
struct algo_sample_priv {
	unsigned int a;
	unsigned int b;
};

static int algo_sample_create(void **handle, void *native_component)
{
	struct algo_sample_priv *priv;

	/* 申请算法私有资源 */
	priv = malloc(sizeof(struct algo_sample_priv));
	if (!priv)
		return -ENOMEM;

	priv->a = 1;
	priv->b = 2;

	*handle = priv;

	return 0;
}

static int algo_sample_process(void *handle, void *input_buffer, unsigned int * const input_size,
				void *output_buffer, unsigned int * const output_size)
{
	struct algo_sample_priv *priv = handle;

	printf("[%s] line:%d a=%d, b=%d\n", __func__, __LINE__, priv->a, priv->b);
	/* 算法处理, 将结果输出到outpu_buffer中 */
	/*algo_sample_convert();*/

	return 0;
}

static int algo_sample_release(void *handle)
{
	struct algo_sample_priv *priv = handle;

	/* 释放算法私有资源 */
	free(priv);
	return 0;
}

static struct snd_dsp_hal_component_process_driver algo_sample_comp = {
	.create		= algo_sample_create,
	.process	= algo_sample_process,
	.release	= algo_sample_release,
};

/* 可以在DSP启动流程中添加该函数，进行算法组件的安装 */
int algo_sample_install(void)
{
	int ret;
	/* 从SND_DSP_COMPONENT_USER开始可以定义一些特殊的算法 */
	enum SND_DSP_COMPONENT_TYPE type = SND_DSP_COMPONENT_USER;
	struct snd_dsp_component_algo_config *config = &algo_sample_comp.config;

	/* 设置算法通用配置，系统启动后也可以通过CPUX进行修改 */
	/* 设置该算法默认绑定的声卡 */
	snprintf(config->card_name, sizeof(config->card_name), "audiocodec");
	/* 设置该算法默认绑定的声卡对应的音频流 */
	config->stream = SND_PCM_STREAM_CAPTURE;
	/* 设置是否默认使能该算法 */
	config->enable = 1;

	ret = snd_dsp_hal_component_install(&algo_sample_comp, type);

	return ret;
}
#if 1
#include <console.h>
static int cmd_algosample_install(int argc, char *argv[])
{
	if (!algo_sample_install())
		printf("algo sample install.\n");
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_algosample_install, asi, algorithm sampel install);
#endif
