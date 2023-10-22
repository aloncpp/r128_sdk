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
#define TAG	"AudioHWEQDRC"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_hw.h"
#include "AudioBase.h"

#include "local_audio_hw.h"

#include "snd_sunxi_alg_cfg.h"


#define MAX_MAIN_REG_NUM (202)

#define MAX_POST_REG_NUM (28)


#define MAIN_EQ_REG_MIN 	0x100
#define MAIN_EQ_REG_MAX		(0x310 + 0x14 * (20 - 1))
#define POST_EQ_REG_MIN 	0x500
#define POST_EQ_REG_MAX		(0x514 + 0x14 * (5 - 1))

#define AW_EQUAL_CONFIG_NUMBER 2

#define LINE_STR_BUF_LEN_MAX 32

#define MAX_DRC_3_BAND_REG_NUM (78)

#define MAX_DRC_1_BAND_REG_NUM (26)


#define DRC1B_REG_MIN 	0x600
#define DRC1B_REG_MAX	0x660
#define DRC3B_REG_MIN 	0x700
#define DRC3B_REG_MAX	0x834

#define AW_DRC_CONFIG_NUMBER 2

static int verbose = 1;

static struct eq_attr *g_ea = NULL;

static struct drc_attr *g_da = NULL;

/* equalizer parameters */
typedef struct
{
    /* reg address */
    unsigned int reg;
    /* reg value */
    unsigned int reg_val;
} reg_core_prms_t;

typedef struct
{
	/* eq parameters for num of dac reg*/
	int reg_num;
	/* eq parameters for dac reg*/
	reg_core_prms_t *reg_prms;
}reg_prms_t;

struct eq_attr {
	audio_hw_t *ahw;
	char *config_file[AW_EQUAL_CONFIG_NUMBER];
	reg_prms_t eq_reg_prms[AW_EQUAL_CONFIG_NUMBER];
};

struct drc_attr {
	audio_hw_t *ahw;
	char *config_file[AW_DRC_CONFIG_NUMBER];
	reg_prms_t drc_reg_prms[AW_DRC_CONFIG_NUMBER];
};

#if defined(CONFIG_ARCH_SUN20IW2)

static void set_reg_prms(reg_prms_t *reg_prms, struct alg_cfg_reg_domain *domain)
{
	int i;
    for (i = 0; i < reg_prms->reg_num; ++i) {
        reg_core_prms_t *reg_core_prms = reg_prms->reg_prms;
		sunxi_alg_cfg_set(reg_core_prms[i].reg, reg_core_prms[i].reg_val, domain);
    }
}

static int parse_config_to_reg_prms(const char *config_file, reg_prms_t *reg_prms)
{
    int ret = 0;
	unsigned long result;
	const char *line_tmp;
    FILE *fp = NULL;
    char line_str[LINE_STR_BUF_LEN_MAX];
	reg_core_prms_t *reg_core_prms = reg_prms->reg_prms;
    int index = 0;

    if (!config_file || !reg_prms) {
        _err("Invalid config_file or reg_prms");
        ret = -1;
        goto out;
    }

    fp = fopen(config_file, "r");
    if (!fp) {
        _info("Failed to open %s (%d)", config_file, errno);
		ret = -1;
		goto out;
	}

    while (!feof(fp) && fgets(line_str, sizeof(line_str), fp)) {

        if (line_str[0] == '[' && strstr(line_str, "[0x") && strstr(line_str, "] = 0x")) {

			/* get reg */
			line_tmp = line_str + 1;
			result = strtoul(line_tmp, NULL, 16);
			if (result) {
				reg_core_prms[index].reg = (unsigned int)result;
			}

			/* get reg val */
			line_tmp = line_str + 11;
			result = strtoul(line_tmp, NULL, 16);
			if (result) {
				reg_core_prms[index].reg_val = (unsigned int)result;
			}

			++index;
        }

    }

	reg_prms->reg_num = index;

	if (reg_prms->reg_num > MAX_POST_REG_NUM && reg_prms->reg_num > MAX_MAIN_REG_NUM) {
        _err("Invalid reg_num %d", reg_prms->reg_num);
        ret = -1;
		goto out;
    }

    ret = 0;

out:
	if (fp) {
		fclose(fp);
		fp = NULL;
	}
    return ret;
}

static void print_reg_prms(const reg_prms_t *prms)
{
    int i;
    for (i = 0; i < prms->reg_num; ++i) {
        reg_core_prms_t *reg_core_prms = prms->reg_prms;
        _debug(" [reg %02i] reg: 0x%04lx, reg_val: 0x%08lx ",
                i + 1, (long)reg_core_prms[i].reg, (long)reg_core_prms[i].reg_val);
    }
}

#endif


static int eq_reg_init()
{
	struct eq_attr *ea = g_ea;
    const char *config_file[AW_EQUAL_CONFIG_NUMBER];
    int err = -1;
	int i = 0;

	if (!ea)
		return 0;


#if defined(CONFIG_ARCH_SUN20IW2)

	config_file[0] = "/data/R128EQ-20band.conf";
	config_file[1] = "/data/R128EQ-5band.conf";


	for (i = 0; i < AW_EQUAL_CONFIG_NUMBER; ++i) {

		if (config_file[i]) {
			ea->config_file[i] = (char *)as_alloc(strlen(config_file[i]) + 1);
			if (!ea->config_file[i]) {
				_err("Failed to allocate memory for config_file");
				goto error;
			}

			strncpy(ea->config_file[i], config_file[i], strlen(config_file[i]) + 1);

			if (i == 0) {
				ea->eq_reg_prms[i].reg_prms = (reg_core_prms_t *)as_alloc(sizeof(reg_core_prms_t) * MAX_MAIN_REG_NUM);
				if (!ea->eq_reg_prms[i].reg_prms) {
					_err("Failed to allocate memory for eq_reg_prms");
					goto error;
				}
			}
			else if (i == 1) {
				ea->eq_reg_prms[i].reg_prms = (reg_core_prms_t *)as_alloc(sizeof(reg_core_prms_t) * MAX_POST_REG_NUM);
				if (!ea->eq_reg_prms[i].reg_prms) {
					_err("Failed to allocate memory for eq_reg_prms");
					goto error;
				}
			}

		}
	}
#endif

	return 0;

error:

	if (ea) {
		for (i = 0; i < AW_EQUAL_CONFIG_NUMBER; ++i) {
			if (ea->config_file[i]) {
				as_free(ea->config_file[i]);
				ea->config_file[i] = NULL;
			}
			if (ea->eq_reg_prms[i].reg_prms) {
				as_free(ea->eq_reg_prms[i].reg_prms);
				ea->eq_reg_prms[i].reg_prms = NULL;
			}
		}
		as_free(ea);
		ea = NULL;
	}
	return err;

}

static int eq_hw_update_reg()
{
	struct eq_attr *ea = g_ea;
	//as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	int ret = -1;

	_debug("");

	if (!ea)
		return 0;


#if defined(CONFIG_ARCH_SUN20IW2)

	struct alg_cfg_reg_domain *domain;
	enum SUNXI_ALG_CFG_DOMAIN domain_sel;
	int i,j;
	int cfg_fail_num = 0;
	int reg_main_cnt = 0;
	int reg_post_cnt = 0;

	for (i = 0; i < AW_EQUAL_CONFIG_NUMBER; ++i) {

		ret = parse_config_to_reg_prms(ea->config_file[i], &ea->eq_reg_prms[i]);
		if (ret < 0) {
		   _info("parse_config_to_eq_prms failed");
		   cfg_fail_num++;
		   if (cfg_fail_num == AW_EQUAL_CONFIG_NUMBER)
		   		goto err;
		   if (i == AW_EQUAL_CONFIG_NUMBER - 1)
		   		goto out;

		   continue;
		}

		reg_core_prms_t *reg_core_prms = ea->eq_reg_prms[i].reg_prms;

		if (verbose) {
			_info("Parse from config file %s", ea->config_file[i]);
			print_reg_prms(&ea->eq_reg_prms[i]);
		}

		for (j = 0; j < ea->eq_reg_prms[i].reg_num; ++j) {

			if (reg_core_prms[j].reg >= MAIN_EQ_REG_MIN &&
				reg_core_prms[j].reg <= MAIN_EQ_REG_MAX) {
				reg_main_cnt++;
			} else if (reg_core_prms[j].reg >= POST_EQ_REG_MIN &&
				reg_core_prms[j].reg <= POST_EQ_REG_MAX) {
				reg_post_cnt++;
			} else {
				_info("reg: 0x%04lx", (long)reg_core_prms[j].reg);
				continue;
			}
		}

		if (reg_main_cnt == ea->eq_reg_prms[i].reg_num)
			domain_sel = SUNXI_ALG_CFG_DOMAIN_MEQ;
		else if (reg_post_cnt == ea->eq_reg_prms[i].reg_num)
			domain_sel = SUNXI_ALG_CFG_DOMAIN_PEQ;
		else {
			_err("reg_main_cnt %d is not equal to reg_num %d", reg_main_cnt, ea->eq_reg_prms[i].reg_num);
			_err("reg_post_cnt %d is not equal to reg_num %d", reg_post_cnt, ea->eq_reg_prms[i].reg_num);
			goto err;
		}

		ret = sunxi_alg_cfg_domain_get(&domain, domain_sel);
		if (ret) {
			_err("alg_cfg domain get failed");
			goto err;
		}

		set_reg_prms(&ea->eq_reg_prms[i], domain);

	}

#endif

out:

	_info("create hw eq");

	return 0;
err:

	for (i = 0; i < AW_EQUAL_CONFIG_NUMBER; ++i) {
		if (ea->config_file[i]) {
			as_free(ea->config_file[i]);
			ea->config_file[i] = NULL;
		}
		if (ea->eq_reg_prms[i].reg_prms) {
			as_free(ea->eq_reg_prms[i].reg_prms);
			ea->eq_reg_prms[i].reg_prms = NULL;
		}
	}


	return ret;

}
int eq_hw_init(void)
{
	struct eq_attr *ea;
	int ret = -1;

	_debug("");

	if (!g_ea) {
		ea = as_alloc(sizeof(struct eq_attr));
		if (!ea) {
			_err("no memory");
			goto err;
		}
		g_ea = ea;
	}

	ret = eq_reg_init();
	if (ret != 0) {
		_err("eq reg init failed");
		goto err;
	}

	ret = eq_hw_update_reg();
	if (ret != 0) {
		_err("eq_ahw_update_reg failed");
		goto err;
	}

	_info("");

	return 0;

err:
	return ret;

}

int eq_hw_destroy(void)
{

	struct eq_attr *ea = g_ea;
	int i = 0;

	if (!ea)
		return 0;

	for (i = 0; i < AW_EQUAL_CONFIG_NUMBER; ++i) {
		if (ea->config_file[i]) {
			as_free(ea->config_file[i]);
			ea->config_file[i] = NULL;
		}
		if (ea->eq_reg_prms[i].reg_prms) {
			as_free(ea->eq_reg_prms[i].reg_prms);
			ea->eq_reg_prms[i].reg_prms = NULL;
		}
	}

	as_free(ea);

	g_ea = NULL;

	return 0;
}


static int drc_reg_init()
{
	struct drc_attr *da = g_da;
    const char *config_file[AW_DRC_CONFIG_NUMBER];
    int err = -1;
	int i = 0;

	if (!da)
		return 0;

#if defined(CONFIG_ARCH_SUN20IW2)

	config_file[0] = "/data/DRC3.conf";
	config_file[1] = "/data/DACDRC.conf";


	for (i = 0; i < AW_DRC_CONFIG_NUMBER; ++i) {

		if (config_file[i]) {
			da->config_file[i] = (char *)as_alloc(strlen(config_file[i]) + 1);
			if (!da->config_file[i]) {
				_err("Failed to allocate memory for config_file");
				goto error;
			}

			strncpy(da->config_file[i], config_file[i], strlen(config_file[i]) + 1);

			if (i == 0) {
				da->drc_reg_prms[i].reg_prms = (reg_core_prms_t *)as_alloc(sizeof(reg_core_prms_t) * MAX_DRC_3_BAND_REG_NUM);
				if (!da->drc_reg_prms[i].reg_prms) {
					_err("Failed to allocate memory for drc_reg_prms");
					goto error;
				}
			}
			else if (i == 1) {
				da->drc_reg_prms[i].reg_prms = (reg_core_prms_t *)as_alloc(sizeof(reg_core_prms_t) * MAX_DRC_1_BAND_REG_NUM);
				if (!da->drc_reg_prms[i].reg_prms) {
					_err("Failed to allocate memory for drc_reg_prms");
					goto error;
				}
			}

		}
	}
#endif

	return 0;

error:

	if (da) {
		for (i = 0; i < AW_DRC_CONFIG_NUMBER; ++i) {
			if (da->config_file[i]) {
				as_free(da->config_file[i]);
				da->config_file[i] = NULL;
			}
			if (da->drc_reg_prms[i].reg_prms) {
				as_free(da->drc_reg_prms[i].reg_prms);
				da->drc_reg_prms[i].reg_prms = NULL;
			}
		}
		as_free(da);
		da = NULL;
	}
	return err;

}

static int drc_hw_update_reg()
{
	struct drc_attr *da = g_da;
	//as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	int ret = -1;

	_debug("");

	if (!da)
		return 0;

#if defined(CONFIG_ARCH_SUN20IW2)

	struct alg_cfg_reg_domain *domain;
	enum SUNXI_ALG_CFG_DOMAIN domain_sel;
	int i,j;
	int cfg_fail_num = 0;
	int reg_1db_cnt = 0;
	int reg_3db_cnt = 0;

	for (i = 0; i < AW_DRC_CONFIG_NUMBER; ++i) {

		ret = parse_config_to_reg_prms(da->config_file[i], &da->drc_reg_prms[i]);
		if (ret < 0) {
		   _info("parse_config_to_reg_prms failed");
		   cfg_fail_num++;
		   if (cfg_fail_num == AW_DRC_CONFIG_NUMBER)
		   		goto err;
		   if (i == AW_DRC_CONFIG_NUMBER - 1)
		   		goto out;

		   continue;
		}

		reg_core_prms_t *reg_core_prms = da->drc_reg_prms[i].reg_prms;

		if (verbose) {
			_info("Parse from config file %s", da->config_file[i]);
			print_reg_prms(&da->drc_reg_prms[i]);
		}

		for (j = 0; j < da->drc_reg_prms[i].reg_num; ++j) {

			if (reg_core_prms[j].reg >= DRC1B_REG_MIN &&
				reg_core_prms[j].reg <= DRC1B_REG_MAX) {
				reg_1db_cnt++;
			} else if (reg_core_prms[j].reg >= DRC3B_REG_MIN &&
				reg_core_prms[j].reg <= DRC3B_REG_MAX) {
				reg_3db_cnt++;
			} else {
				_info("reg: 0x%04lx", (long)reg_core_prms[j].reg);
				continue;
			}
		}

		if (reg_1db_cnt == da->drc_reg_prms[i].reg_num)
			domain_sel = SUNXI_ALG_CFG_DOMAIN_1BDRC;
		else if (reg_3db_cnt == da->drc_reg_prms[i].reg_num)
			domain_sel = SUNXI_ALG_CFG_DOMAIN_3BDRC;
		else {
			_err("reg_1db_cnt %d is not equal to reg_num %d", reg_1db_cnt, da->drc_reg_prms[i].reg_num);
			_err("reg_3db_cnt %d is not equal to reg_num %d", reg_3db_cnt, da->drc_reg_prms[i].reg_num);
			goto err;
		}

		ret = sunxi_alg_cfg_domain_get(&domain, domain_sel);
		if (ret) {
			_err("alg_cfg domain get failed");
			goto err;
		}

		set_reg_prms(&da->drc_reg_prms[i], domain);

	}

#endif

out:

	_info("create hw drc");

	return 0;
err:

	for (i = 0; i < AW_DRC_CONFIG_NUMBER; ++i) {
		if (da->config_file[i]) {
			as_free(da->config_file[i]);
			da->config_file[i] = NULL;
		}
		if (da->drc_reg_prms[i].reg_prms) {
			as_free(da->drc_reg_prms[i].reg_prms);
			da->drc_reg_prms[i].reg_prms = NULL;
		}
	}


	return ret;

}

int drc_hw_init(void)
{
	struct drc_attr *da;
	int ret = -1;

	_debug("");

	if (!g_da) {
		da = as_alloc(sizeof(struct drc_attr));
		if (!da) {
			_err("no memory");
			goto err;
		}
		g_da = da;
	}

	ret = drc_reg_init();
	if (ret != 0) {
		_err("drc reg init failed");
		goto err;
	}

	ret = drc_hw_update_reg();
	if (ret != 0) {
		_err("drc_ahw_update_reg failed");
		goto err;
	}

	_info("");

	return 0;

err:
	return ret;

}

int drc_hw_destroy(void)
{

	struct drc_attr *da = g_da;
	int i = 0;

	if (!da)
		return 0;

	for (i = 0; i < AW_DRC_CONFIG_NUMBER; ++i) {
		if (da->config_file[i]) {
			as_free(da->config_file[i]);
			da->config_file[i] = NULL;
		}
		if (da->drc_reg_prms[i].reg_prms) {
			as_free(da->drc_reg_prms[i].reg_prms);
			da->drc_reg_prms[i].reg_prms = NULL;
		}
	}

	as_free(da);

	g_da = NULL;

	return 0;
}

