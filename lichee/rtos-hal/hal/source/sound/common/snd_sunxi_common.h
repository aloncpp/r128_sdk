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
#ifndef __SND_SUNXI_COMMON_H
#define __SND_SUNXI_COMMON_H

/* for reg debug */
#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct audio_reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};

struct sunxi_pa_config {
	uint32_t pa_msleep_time;
	uint32_t gpio;
	uint32_t mul_sel;
	uint32_t pull;
	uint32_t drv_level;
	uint32_t data;
};

/* EX:
 * static struct audio_reg_label reg_labels[] = {
 * 	REG_LABEL(SUNXI_REG_0),
 * 	REG_LABEL(SUNXI_REG_1),
 * 	REG_LABEL(SUNXI_REG_n),
 * 	REG_LABEL_END,
 * };
 */
int snd_sunxi_save_reg(struct audio_reg_label *reg_labels, void *data,
		       unsigned int (*snd_read_func)(void *data, unsigned int reg));
int snd_sunxi_echo_reg(struct audio_reg_label *reg_labels, void *data,
		       void (*snd_write_func)(void *data, unsigned int reg, unsigned int val));
int snd_sunxi_pa_enable(struct sunxi_pa_config *pa_cfg);
void snd_sunxi_pa_disable(struct sunxi_pa_config *pa_cfg);
int snd_sunxi_pa_init(struct sunxi_pa_config *pa_cfg,
		      struct sunxi_pa_config *default_pa_cfg,
		      char *secname);
#endif /* __SND_SUNXI_COMMON_H */
