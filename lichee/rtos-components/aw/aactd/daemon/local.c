#include "local.h"

#include <stdlib.h>
#include <errno.h>

#include "aactd/common.h"
#include "aactd/communicate.h"
#include "eq_sw.h"
#include "drc_hw.h"
#include "eq_hw.h"

unsigned int com_buf_len_max = COM_BUF_LEN_MAX_DEFAULT;

int verbose_level = -1;

int local_process_init(void)
{
    int ret;

    ret = eq_sw_local_init();
    if (ret < 0) {
        aactd_error("eq_sw_local_init failed\n");
        goto err_out;
    }

    ret = drc_hw_local_init();
    if (ret < 0) {
        aactd_error("drc_hw_local_init failed\n");
        goto release_eq_sw;
    }

    ret = eq_hw_local_init();
    if (ret < 0) {
        aactd_error("eq_hw_local_init failed\n");
        goto release_drc_hw;
    }
    return 0;

release_drc_hw:
    drc_hw_local_release();
release_eq_sw:
    eq_sw_local_release();
err_out:
    return ret;
}

int local_process_release(void)
{
    eq_sw_local_release();
    drc_hw_local_release();
	eq_hw_local_release();
    return 0;
}

int read_com_from_local(struct aactd_com *com)
{
    // TODO: implement this
    aactd_error("Not supported\n");
    return -1;

    switch (com->header.type) {
    case EQ_SW:
        break;
    case DRC_HW:
        break;
	case EQ_HW:
        break;
	case DRC3_HW:
        break;
    default:
        aactd_error("Unknown type\n");
        break;
    }
    return 0;
}

int write_com_to_local(const struct aactd_com *com)
{
    int ret;

    switch (com->header.type) {
    case EQ_SW:
        ret = eq_sw_write_com_to_local(com);
        if (ret < 0) {
            aactd_error("Failed to write EQ_SW com to local\n");
            goto out;
        }
        break;
    case DRC_HW:
        ret = drc_hw_write_com_to_local(com);
        if (ret < 0) {
            aactd_error("Failed to write DRC_HW com to local\n");
            goto out;
        }
        break;
	case EQ_HW:
        ret = eq_hw_write_com_to_local(com);
        if (ret < 0) {
            aactd_error("Failed to write EQ_HW com to local\n");
            goto out;
        }
        break;
	case DRC3_HW:
		#if defined(CONFIG_ARCH_SUN20IW2)
        ret = drc_hw_write_com_to_local(com);
        if (ret < 0) {
            aactd_error("Failed to write DRC3_HW com to local\n");
            goto out;
        }
		#endif
        break;
    default:
        aactd_error("Unknown type\n");
        break;
    }

    ret = 0;
out:
    return ret;
}
