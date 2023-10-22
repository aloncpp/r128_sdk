#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sunxi_amp.h"

extern sunxi_amp_func_table fsys_table[];
extern sunxi_amp_func_table net_table[];
extern sunxi_amp_func_table DEMO_table[];
extern sunxi_amp_func_table console_table[];
extern sunxi_amp_func_table bt_table[];
extern sunxi_amp_func_table pmofm33_table[];
extern sunxi_amp_func_table pmofrv_table[];
extern sunxi_amp_func_table pmofdsp_table[];
extern sunxi_amp_func_table flashc_table[];
extern sunxi_amp_func_table misc_table[];
extern sunxi_amp_func_table audio_table[];
extern sunxi_amp_func_table rpdata_table[];
extern sunxi_amp_func_table tfm_table[];

sunxi_amp_func_table *func_table[] =
{
#ifdef CONFIG_AMP_FSYS_SERVICE
    (void *)&fsys_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_NET_SERVICE
    (void *)&net_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_BT_SERVICE
    (void *)&bt_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_DEMO_SERVICE
    (void *)&DEMO_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_CONSOLE_SERVICE
    (void *)&console_table,
    (void *)&console_table,
    (void *)&console_table,
#else
    NULL,
    NULL,
    NULL,
#endif
#ifdef CONFIG_AMP_PMOFM33_SERVICE
    (void *)&pmofm33_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_PMOFRV_SERVICE
    (void *)&pmofrv_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_PMOFDSP_SERVICE
    (void *)&pmofdsp_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_FLASHC_SERVICE
    (void *)&flashc_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_MISC_SERVICE
    (void *)&misc_table,
    (void *)&misc_table,
    (void *)&misc_table,
#else
    NULL,
    NULL,
    NULL,
#endif
#ifdef CONFIG_AMP_AUDIO_SERVICE
    (void *)&audio_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_RPDATA_SERVICE
    (void *)&rpdata_table,
#else
    NULL,
#endif
#ifdef CONFIG_AMP_TFM_SERVICE
    (void *)&tfm_table,
#else
    NULL,
#endif
};
