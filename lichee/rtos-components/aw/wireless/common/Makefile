####complie wifimanger common part##########################################################

obj-y += src/api_action/api_action.o

obj-y += src/os/freertos/mutex.o
obj-y += src/os/freertos/queue.o
obj-y += src/os/freertos/sem.o
obj-y += src/os/freertos/thread.o
obj-y += src/os/freertos/sync_notify.o
obj-y += src/os/freertos/ring_buff.o

obj-y += src/tracing/log_backend_hub.o
obj-y += src/tracing/log_queue_block.o
obj-y += src/tracing/log_worker.o
obj-y += src/tracing/log_core.o
obj-y += src/tracing/log_parse_cmd_rtos.o

CFLAGS += -I include/
CFLAGS += -I drivers/rtos-hal/include/hal/
CFLAGS += -I drivers/rtos-hal/include/osal/
CFLAGS += -I include/FreeRTOS_POSIX
CFLAGS += -I components/common/aw/wireless/common/src
CFLAGS += -I components/common/aw/wireless/common/src/api_action
CFLAGS += -I components/common/aw/wireless/common/src/os
CFLAGS += -I components/common/aw/wireless/common/src/os/freertos
CFLAGS += -I components/common/aw/wireless/common/src/tracing
CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/xradio/include

CFLAGS += -DOS_NET_FREERTOS_OS=1
CFLAGS += -D_POSIX_MONOTONIC_CLOCK=1
