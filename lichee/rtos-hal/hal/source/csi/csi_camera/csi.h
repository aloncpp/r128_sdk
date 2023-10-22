
#include "hal_time.h"
#include "hal_timer.h"

#include "csi_reg/csi_reg.h"
#include "cj_board_cfg.h"
#include "hal_log.h"

#define IS_FLAG(x, y) (((x)&(y)) == y)

#define CSI_DEV_DBG_EN   0
#if (CSI_DEV_DBG_EN == 1)
#define csi_dbg(x, arg...) hal_log_info("[csi_debug]"x, ##arg)
#else
#define csi_dbg(x, arg...)
#endif

#define csi_err(x, arg...)  hal_log_err("[csi_err]"x, ##arg)
#define csi_warn(x, arg...) hal_log_warn("[csi_warn]"x, ##arg)
#define csi_print(x, arg...) hal_log_info("[csi]"x, ##arg)

#define ALIGN_16B(x)    (((x) + (15)) & ~(15))

struct csi_fmt {
	unsigned int width;
	unsigned int height;
};

HAL_Status hal_csi_set_fmt(struct csi_fmt *csi_input_fmt);
//HAL_Status hal_csi_sensor_s_stream(unsigned int on);
HAL_Status hal_csi_s_stream(unsigned int on);
HAL_Status hal_sensor_s_stream(unsigned int on);

int hal_csi_probe(void);
void hal_csi_remove(void);
