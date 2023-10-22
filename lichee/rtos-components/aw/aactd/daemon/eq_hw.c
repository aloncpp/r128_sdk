#include "eq_hw.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "aactd/common.h"
#include "aactd/communicate.h"
#include "local.h"
#include "snd_sunxi_alg_cfg.h"

static struct aactd_com_eq_hw_data local_data;

int eq_hw_local_init(void)
{
    int ret;

    local_data.reg_num = 0;
    local_data.reg_args = malloc(com_buf_len_max);
    if (!local_data.reg_args) {
        aactd_error("No memory\n");
        ret = -ENOMEM;
        goto err_out;
    }

    return 0;

free_local_data:
    free(local_data.reg_args);
    local_data.reg_args = NULL;
err_out:
    return ret;
}

int eq_hw_local_release(void)
{
    if (local_data.reg_args) {
        free(local_data.reg_args);
        local_data.reg_args = NULL;
    }
    return 0;
}

static int eq_hw_data_set(const struct aactd_com_eq_hw_data *data)
{
    int ret = -1;
	int i;
	struct alg_cfg_reg_domain *domain;
	enum SUNXI_ALG_CFG_DOMAIN domain_sel;
	int reg_main_cnt = 0;
	int reg_post_cnt = 0;


	for (i = 0; i < data->reg_num; ++i) {

		if (data->reg_args[i].offset >= MAIN_EQ_REG_MIN &&
			data->reg_args[i].offset <= MAIN_EQ_REG_MAX) {
			reg_main_cnt++;
		} else if (data->reg_args[i].offset >= POST_EQ_REG_MIN &&
			data->reg_args[i].offset <= POST_EQ_REG_MAX) {
			reg_post_cnt++;
		} else {
			aactd_info("reg: 0x%04lx", (long)data->reg_args[i].offset);
			continue;
		}
	}

	if (reg_main_cnt == data->reg_num)
		domain_sel = SUNXI_ALG_CFG_DOMAIN_MEQ;
	else if (reg_post_cnt == data->reg_num)
		domain_sel = SUNXI_ALG_CFG_DOMAIN_PEQ;
	else {
		aactd_error("reg_main_cnt %d is not equal to reg_num %d", reg_main_cnt, data->reg_num);
		aactd_error("reg_post_cnt %d is not equal to reg_num %d", reg_post_cnt, data->reg_num);
		goto err;
	}


	ret = sunxi_alg_cfg_domain_get(&domain, domain_sel);
	if (ret) {
		aactd_error("alg_cfg domain get failed");
		goto err;
	}

    for (i = 0; i < data->reg_num; ++i) {
		sunxi_alg_cfg_set(data->reg_args[i].offset, data->reg_args[i].value, domain);
    }

err:

    return ret;
}

int eq_hw_write_com_to_local(const struct aactd_com *com)
{
    int ret;

    ret = aactd_com_eq_hw_buf_to_data(com->data, &local_data);
    if (ret < 0) {
        aactd_error("aactd_com_drc_hw_buf_to_data failed\n");
        goto out;
    }

    ret = eq_hw_data_set(&local_data);
    if (ret < 0) {
        aactd_error("Failed to set EQ data\n");
        goto out;
    }

    ret = 0;
out:
    return ret;
}
