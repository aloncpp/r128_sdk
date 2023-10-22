#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined(CONFIG_PROJECT_R328S1_EVB1)
#include "r328s1/evb1/board_config.h"
#elif defined(CONFIG_PROJECT_R528S3_EVB3)
#include "r528s3/evb3_demo/board_config.h"
#elif defined(CONFIG_PROJECT_R528S3_EVB3_NAND)
#include "r528s3/evb3_nand/board_config.h"
#else
#warning check board_config.h
#define WL_WAKE_AP_PIN	GPIO_PE5
#define WL_REG_ON_PIN	GPIO_PE6
#define CONFIG_SDC_ID	1
#endif

#endif /* __CONFIG_H__ */
