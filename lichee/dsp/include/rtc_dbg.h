#ifndef __RTC_DEBUG_H
#define __RTC_DEBUG_H

#include <stdint.h>
#include "platform.h"
#include "hal_prcm.h"

/* standby progress record */
/* macro record */
#define REC_SSTANDBY     (0xf1f10000)
#define REC_NSTANDBY     (0xf2f20000)
#define REC_ESTANDBY     (0xf3f30000)
#define REC_FAKEPOWEROFF (0xf4f40000)
#define REC_TSTANDBY     (0xf5f50000)
#define REC_AUDIO        (0xf6f60000)

/* micro record */
#define REC_COPY_DONE     (0x0000)
#define REC_ENTER         (0x1000)
#define REC_BEFORE_INIT   (0x2000)
#define REC_ENTER_INIT    (0x3000)
#define REC_AFTER_INIT    (0x4000)
#define REC_WAIT_WAKEUP   (0x5000)
#define REC_BEFORE_EXIT   (0x6000)
#define REC_ENTER_EXIT    (0x7000)
#define REC_AFTER_EXIT    (0x8000)
#define REC_DRAM_DBG      (0x9000)
#define REC_SHUTDOWN      (0xa000)
#define REC_HOTPULG       (0xb000)
#define REC_CPUX_RESUME   (0xc000)
#define REC_DEBUG         (0xd000)
#define REC_CPUS_LOOP     (0xe000)
#define REC_CPUS_WAKEUP   (0xf000)

/* audio debug micro */
#define REC_AUDIO_CORE       (0x0000)
#define REC_AUDIO_PCM        (0x1000)
#define REC_AUDIO_MIXER      (0x2000)
#define REC_AUDIO_DEBUG      (0x3000)
#define REC_AUDIO_COMPONENT  (0x4000)
#define REC_AUDIO_COMMON     (0x5000)
#define REC_AUDIO_STANDBY    (0x6000)

#endif
