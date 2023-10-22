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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>
#include <aw_common.h>
#include <aw_list.h>
#include <hal_time.h>
#include <hal_sem.h>
#include <hal_queue.h>
#include <ringbuffer.h>

#include "rpdata_base.h"
#include <rpdata.h>
#include <rpdata_amp.h>

extern void *amp_align_malloc(int size);
extern void amp_align_free(void *ptr);

#define LOG_COLOR_NONE          "\e[0m"
#define LOG_COLOR_GREEN         "\e[32m"
#define LOG_COLOR_BLUE          "\e[34m"
#define LOG_COLOR_RED           "\e[31m"

static int g_rpd_debug_mask = 0;

#if defined(CONFIG_ARCH_ARM_CORTEX_A7)
#define TAG "A7-RPD"
#elif defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define TAG "M33-RPD"
#elif defined(CONFIG_ARCH_RISCV)
#define TAG "RV-RPD"
#elif defined(CONFIG_ARCH_DSP)
#define TAG "DSP-RPD"
#else
#define TAG "RPD"
#endif


#ifndef unlikely
#define unlikely(x)             __builtin_expect ((x), 0)
#endif
#define rpd_debug(fmt, args...) \
do { \
        if (unlikely(g_rpd_debug_mask)) \
                printf(LOG_COLOR_GREEN "[%s-DBG][%s](%d) " fmt "\n" \
                        LOG_COLOR_NONE, TAG, __func__, __LINE__, ##args); \
} while (0)

#define rpd_info(fmt, args...)     \
    printf(LOG_COLOR_BLUE "[%s-INF][%s](%d) " fmt "\n" LOG_COLOR_NONE, \
                        TAG, __func__, __LINE__, ##args)

#define rpd_err(fmt, args...)      \
    printf(LOG_COLOR_RED "[%s-ERR][%s](%d) " fmt "\n" LOG_COLOR_NONE, \
                        TAG, __func__, __LINE__, ##args)

#define HEXDUMP(ptr, size) \
do { \
	int i; \
	char *p = (char *)ptr; \
	printf("\n"); \
	for (i = 0; i < size; i++) { \
		printf("0x%02x ", *(p+i)); \
		if ((i+1)%16 == 0) \
			printf("\n"); \
	} \
	printf("\n"); \
} while (0)

struct rpdata_type_info {
	char type[RPDATA_NAME_LEN];
	struct list_head list;
	struct list_head head;
	uint8_t type_idx;
	uint64_t id_bitmap;
};

struct _rpdata {
	char name[RPDATA_NAME_LEN];
	struct rpdata_info *info;
	struct rpdata_type_info *tinfo;
	struct list_head list;
	hal_sem_t conn_ack_sem;			/* master wait slave connect */
	hal_sem_t conn_notify_sem;		/* slave wait master notify */
	hal_queue_t recv_queue;
	hal_ringbuffer_t recv_rb;
	hal_sem_t destroy_sem;		/* master wait slave destroy */
	struct rpdata_cbs *cbs;
	void *private_data;
	uint32_t state;
	int8_t dir;
	uint8_t id_idx;
	int8_t use_cnt;
	bool is_master;
};

struct rpd_recv_cb_item {
	void *data;
	uint32_t len;
};

static struct _rpdata_ctrl {
	struct list_head type_head[RPDATA_DIR_MAX];
	uint64_t type_bitmap[RPDATA_DIR_MAX];
} *g_rpd_ctrl = NULL;

#define RPDATA_TYPE_HEAD(dir) (&g_rpd_ctrl->type_head[(dir) - 1])
#define RPDATA_TYPE_BITMAP(dir) (g_rpd_ctrl->type_bitmap[(dir) - 1])

static int rpdata_init(void)
{
	int i;
	if (g_rpd_ctrl != NULL)
		return 0;

	rpd_info("rpdata version:%s", RPDATA_VERSION);
	g_rpd_ctrl = calloc(1, sizeof(struct _rpdata_ctrl));
	if (!g_rpd_ctrl)
		return -1;
	for (i = 1; i <= RPDATA_DIR_MAX; i++) {
		if (i == RPDATA_DIR_SELF)
			continue;
		INIT_LIST_HEAD(RPDATA_TYPE_HEAD(i));
		g_rpd_ctrl->type_bitmap[i - 1] = 0;
	}

	return 0;
}

#define RPDATA_CHECK_INIT() \
{ \
	if (!g_rpd_ctrl) \
		rpdata_init(); \
} while (0)

char *rpdata_dir_to_name(int8_t dir)
{
	if (dir == RPDATA_DIR_CM33)
		return RPDATA_DIR_CM33_NAME;
	else if (dir == RPDATA_DIR_RV)
		return RPDATA_DIR_RV_NAME;
	else if (dir == RPDATA_DIR_DSP)
		return RPDATA_DIR_DSP_NAME;
	return NULL;
}

static inline int type_bit_get_free(int8_t dir)
{
	int i;
	for (i = 0; i < sizeof(RPDATA_TYPE_BITMAP(dir)) * 8; i++) {
		if (!(RPDATA_TYPE_BITMAP(dir) & (1 << i)))
			return i + 1;
	}
	return -1;
}

static inline int type_bit_set(int8_t dir, int ofs)
{
	if (ofs > sizeof(RPDATA_TYPE_BITMAP(dir)) * 8 || ofs <= 0)
		return -1;
	if (RPDATA_TYPE_BITMAP(dir) & (1 << (ofs - 1)))
		return -1;
	RPDATA_TYPE_BITMAP(dir) |= (1 << (ofs - 1));

	return 0;
}

static inline int type_bit_clr(int8_t dir, int ofs)
{
	if (ofs > sizeof(RPDATA_TYPE_BITMAP(dir)) * 8 || ofs <= 0)
		return -1;
	RPDATA_TYPE_BITMAP(dir) &= ~(1 << (ofs - 1));
	return 0;
}

static inline int id_bit_get_free(struct rpdata_type_info *tinfo)
{
	int i;
	for (i = 0; i < sizeof(tinfo->id_bitmap) * 8; i++) {
		typeof(tinfo->id_bitmap) mask = (1 << i);
		if (!(tinfo->id_bitmap & mask))
			return i + 1;
	}
	return -1;
}

static inline int id_bit_set(int ofs, struct rpdata_type_info *tinfo)
{
	typeof(tinfo->id_bitmap) mask;

	if (ofs >= sizeof(tinfo->id_bitmap) * 8 || ofs <= 0)
		return -1;

	mask = 1 << (ofs - 1);
	if (tinfo->id_bitmap & mask)
		return -1;
	tinfo->id_bitmap |= mask;

	return 0;
}

static inline int id_bit_clr(int ofs, struct rpdata_type_info *tinfo)
{
	typeof(tinfo->id_bitmap) mask;

	if (ofs >= sizeof(tinfo->id_bitmap) * 8 || ofs <= 0)
		return -1;

	mask = 1 << (ofs - 1);
	tinfo->id_bitmap &= ~mask;

	return 0;
}

static struct rpdata_type_info *_rpdata_find_type_byidx(int dir, int8_t type_idx)
{
	struct rpdata_type_info *info = NULL;

	list_for_each_entry(info, RPDATA_TYPE_HEAD(dir), list) {
		rpd_debug("type:%s,0x%02x", info->type, info->type_idx);
		if (type_idx == info->type_idx)
			return info;
	}
	return NULL;
}

static rpdata_t *_rpdata_find_byidx(int dir, int8_t type_idx, int8_t id_idx)
{
	rpdata_t *rpd = NULL;
	struct rpdata_type_info *info = NULL;

	info = _rpdata_find_type_byidx(dir, type_idx);
	if (!info)
		return NULL;

	list_for_each_entry(rpd, &info->head, list) {
		rpd_debug("name :%s,0x%02x", rpd->name, rpd->id_idx);
		if (id_idx == rpd->id_idx)
			return rpd;
	}
	return NULL;
}

static struct rpdata_type_info *_rpdata_find_type(int8_t dir, const char *type)
{
	struct rpdata_type_info *info = NULL;

	list_for_each_entry(info, RPDATA_TYPE_HEAD(dir), list) {
		if (!strncmp(info->type, type, RPDATA_NAME_LEN))
			return info;
	}
	return NULL;
}

#if 0
static rpdata_t *_rpdata_find(const char *type, const char *name)
{
	rpdata_t *rpd = NULL;
	struct rpdata_type_info *info = NULL;

	info = _rpdata_find_type(type);
	if (!info)
		return NULL;

	list_for_each_entry(rpd, &info->head, list) {
		if (!strncmp(rpd->info->name, name, RPDATA_NAME_LEN))
			return rpd;
	}
	return NULL;
}
#endif

static int rpdata_default_recv_cb(rpdata_t *rpd, void *data, uint32_t data_len)
{
	struct rpd_recv_cb_item item;
	int ret;

	item.data = data;
	item.len = data_len;

	if (rpd->recv_rb) {
		rpd_debug("ringbuffer:%p, data:%p, len:%" PRIu32, rpd->recv_rb, data, data_len);
		ret = hal_ringbuffer_force_put(rpd->recv_rb, data, data_len);
		if (ret > 0)
			ret = 0;
	} else {
		rpd_debug("recv_queue:%p, data:%p, len:%" PRIu32, rpd->recv_queue, data, data_len);
		ret = hal_queue_send(rpd->recv_queue, &item);
	}

	rpd_debug("");

	return ret;
}

struct rpdata_cbs g_rpdata_default_cbs = {
	.recv_cb = rpdata_default_recv_cb,
};

static rpdata_t *_rpdata_create(int dir, const char *type, const char *name, size_t buf_len, bool is_master)
{
	rpdata_t *rpd = NULL;
	struct rpdata_type_info *tinfo = NULL;
	struct rpdata_info *rpdi = NULL;

	rpd_debug("");
	/* find or create type info */
	tinfo = _rpdata_find_type(dir, type);
	if (tinfo != NULL) {
		rpd_debug("");
		list_for_each_entry(rpd, &tinfo->head, list) {
			if (strncmp(rpd->name, name, RPDATA_NAME_LEN) != 0)
				continue;
			if (!rpd->use_cnt &&
				(rpd->state == RPDATA_STATE_DISCONNECTED ||
				rpd->state == RPDATA_STATE_WAIT_CONNECT)) {
				rpd_debug("rpd exist but use_cnt:0");
				return rpd;
			}
			rpd_info("%s alread exist, state=0x%x, cnt:%d",
					name, rpd->state, rpd->use_cnt);
			if (rpd->dir != dir) {
				rpd_info("set dir %d->%d", rpd->dir, dir);
				rpd->dir = dir;
			}
			goto already_exist;
		}
	} else {
		rpd_debug("");
		tinfo = calloc(1, sizeof(struct rpdata_type_info));
		if (!tinfo) {
			rpd_err("no memory");
			return NULL;
		}
		INIT_LIST_HEAD(&tinfo->list);
		INIT_LIST_HEAD(&tinfo->head);
		strcpy(tinfo->type, type);
		if (is_master) {
			tinfo->type_idx = type_bit_get_free(dir);
			if (type_bit_set(dir, tinfo->type_idx) < 0) {
				rpd_err("type_bit_set failed, 0x%08x%08x, idx=%d",
					(uint32_t)(RPDATA_TYPE_BITMAP(dir) & 0xFFFFFFFFUL),
					(uint32_t)(RPDATA_TYPE_BITMAP(dir) >> 32),
					tinfo->type_idx);
			}
			rpd_info("type set id:%d", tinfo->type_idx);
		}
		list_add(&tinfo->list, RPDATA_TYPE_HEAD(dir));
	}

	rpd_debug("");
	/* create rpd */
	rpd = calloc(1, sizeof(rpdata_t));
	if (!rpd) {
		rpd_err("no memory");
		return NULL;
	}
	strncpy(rpd->name, name, sizeof(rpd->name));
	INIT_LIST_HEAD(&rpd->list);

	rpd->conn_ack_sem = hal_sem_create(0);
	rpd_debug("conn_ack_sem sem init, (rpd:%p, sem:%p)", rpd, rpd->conn_ack_sem);
	rpd->conn_notify_sem = hal_sem_create(0);
	rpd_debug("conn_notify_sem init, (rpd:%p, sem:%p)", rpd, rpd->conn_notify_sem);
	rpd->destroy_sem = hal_sem_create(0);
	rpd_debug("destroy_sem init, (rpd:%p, sem:%p)", rpd, rpd->destroy_sem);

	rpd->dir = dir;
	if (is_master) {
		int id = id_bit_get_free(tinfo);
		if (id < 0) {
			rpd_err("id not enough");
			return NULL;
		}
		rpd->id_idx = id;
		id_bit_set(rpd->id_idx, tinfo);
		rpd_info("set id:%d", rpd->id_idx);
	}

	rpd_debug("");
	/* setup callback */
	rpd->cbs = &g_rpdata_default_cbs;
	rpd->recv_queue = hal_queue_create("RPD_RECV", sizeof(struct rpd_recv_cb_item), 1);
	rpd_debug("name:%s recv_queue:%p\n", rpd->name, rpd->recv_queue);
	rpd->recv_rb = NULL;
	rpd->tinfo = tinfo;
	rpd->state = RPDATA_STATE_IDLE;

	if (is_master) {
		/* rpdata_info create */
		rpdi = amp_align_malloc(sizeof(struct rpdata_info));
		if (!rpdi) {
			rpd_err("no memory");
			return NULL;
		}
		memset(rpdi, 0, sizeof(struct rpdata_info));
		strcpy(rpdi->type, type);
		strcpy(rpdi->name, name);
		rpdi->instance = RPDATA_AMP_INS_VAL(tinfo->type_idx, rpd->id_idx);
		rpdi->len = buf_len;
		rpdi->addr = amp_align_malloc(rpdi->len);
		if (!rpdi->addr) {
			rpd_err("no memory");
			return NULL;
		}
		memset(rpdi->addr, 0, rpdi->len);
		rpd->info = rpdi;
		if (g_rpd_debug_mask)
			HEXDUMP(rpd->info, sizeof(struct rpdata_info));
		rpd->is_master = true;
		rpd_info("alloc rpd addr:%p, len:%u", rpdi->addr, rpdi->len);
	}

	list_add(&rpd->list, &tinfo->head);
	rpd_info("rpd create (%s,%s)(%p)", tinfo->type, rpd->name, rpd);

	return rpd;
already_exist:
#if 0
	if (is_master || rpd->state == RPDATA_STATE_CONNECTED)
		return NULL;
	else
		return rpd;
#else
	return NULL;
#endif
}

static int _rpdata_notify(rpdata_t *rpd)
{
	int cmd, ret;

	cmd = RPDATA_IOCTL_NOTIFY | RPDATA_AMP_MAGIC | RPDATA_AMP_IN | RPDATA_AMP_OUT;
	cmd |= RPDATA_AMP_SDIR(RPDATA_DIR_SELF);
	ret = rpdata_ioctl(rpd->dir, cmd, rpd->info, sizeof(struct rpdata_info));

	return ret;
}

static int _rpdata_connect_ack(rpdata_t *rpd)
{
	int cmd, ret;

	rpd_debug("dir=%d", rpd->dir);
	cmd = RPDATA_IOCTL_CON_ACK | RPDATA_AMP_MAGIC;
	cmd |= RPDATA_AMP_SDIR(RPDATA_DIR_SELF);
	cmd |= RPDATA_AMP_INSTANCE(rpd->info->instance);
	ret = rpdata_ioctl(rpd->dir, cmd, NULL, 0);

	return ret;
}

static int _rpdata_notify_destroy(rpdata_t *rpd)
{
	int cmd, ret;

	cmd = RPDATA_IOCTL_NOTIFY_DESTROY | RPDATA_AMP_MAGIC;
	cmd |= RPDATA_AMP_SDIR(RPDATA_DIR_SELF);
	cmd |= RPDATA_AMP_INSTANCE(rpd->info->instance);
	ret = rpdata_ioctl(rpd->dir, cmd, NULL, 0);

	return ret;
}

static int _rpdata_wait_destroy(rpdata_t *rpd)
{
	int cmd, ret;

	cmd = RPDATA_IOCTL_WAIT_DESTROY | RPDATA_AMP_MAGIC;
	cmd |= RPDATA_AMP_SDIR(RPDATA_DIR_SELF);
	cmd |= RPDATA_AMP_INSTANCE(rpd->info->instance);
	ret = rpdata_ioctl(rpd->dir, cmd, NULL, 0);

	return ret;
}

/*
 * master, alloc buffer and rpdata_info
 * */
rpdata_t *rpdata_create(int dir, const char *type, const char *name, size_t buf_len)
{
	rpdata_t * rpd = NULL;

	RPDATA_CHECK_INIT();

	if (strlen(type) >= RPDATA_NAME_LEN ||
		strlen(name) >= RPDATA_NAME_LEN) {
		rpd_err("unexpected rpdata name len");
		goto err;
	}

	/* check dir */
	rpd = _rpdata_create(dir, type, name, buf_len, true);
	rpd_debug("rpd=%p", rpd);
	if (!rpd) {
		rpd_err("create rpdata failed");
		return NULL;
	}
	rpd->use_cnt++;
	rpd->state = RPDATA_STATE_WAIT_CONNECT;
	_rpdata_notify(rpd);

	return rpd;
err:
	if (rpd) {
		amp_align_free(rpd);
		rpd = NULL;
	}
	return NULL;
}

/*
 * slave, get buffer and rpdata_info from master
 */
rpdata_t *rpdata_tryconnect(int dir, const char *type, const char *name)
{
	rpdata_t * rpd = NULL;

	RPDATA_CHECK_INIT();

	if (strlen(type) >= RPDATA_NAME_LEN ||
		strlen(name) >= RPDATA_NAME_LEN) {
		rpd_err("unexpected rpdata name len");
		return NULL;
	}

	rpd = _rpdata_create(dir, type, name, 0, false);
	if (!rpd) {
		rpd_err("create rpdata failed\n");
		return NULL;
	}
	rpd->use_cnt++;
	rpd_info("state:0x%x\n", rpd->state);
	return rpd;
}

rpdata_t *rpdata_connect_with_cb(int dir, const char *type, const char *name,
				struct rpdata_cbs *cbs)
{
	rpdata_t * rpd = NULL;
	int ret;

	rpd = rpdata_tryconnect(dir, type, name);
	if (!rpd)
		return NULL;

	if (cbs != NULL)
		rpd->cbs = cbs;

	switch (rpd->state) {
	case RPDATA_STATE_WAIT_CONNECT:
	case RPDATA_STATE_CONNECTED:
	case RPDATA_STATE_DISCONNECTED:
		break;
	default:
		/* wait master notify */
		rpd->state = RPDATA_STATE_WAIT_CONNECT;
		rpd_debug("conn_notify_sem wait (rpd:%p, sem:%p)", rpd, rpd->conn_notify_sem);
		ret = hal_sem_wait(rpd->conn_notify_sem);
		if (ret != 0) {
			rpd_err("sem wait error");
		}
		rpd_debug("sem wait finish");
		break;
	}

	ret = _rpdata_connect_ack(rpd);
	if (!ret)
		rpd->state = RPDATA_STATE_CONNECTED;

	return rpd;
}

int rpdata_is_connect(rpdata_t *rpd)
{
	if (rpd->state == RPDATA_STATE_CONNECTED)
		return 0;
	return -1;
}

int rpdata_wait_connect(rpdata_t *rpd)
{
	int ret = 0;

	rpd_debug("");
	if (rpd->state == RPDATA_STATE_CONNECTED)
		return ret;

	if (rpd->is_master) {
		rpd_debug("conn_ack_sem wait (rpd:%p, sem:%p)", rpd, rpd->conn_ack_sem);
		ret = hal_sem_wait(rpd->conn_ack_sem);
		if (!ret) {
			rpd->state = RPDATA_STATE_CONNECTED;
		} else {
			rpd_err("sem wait error");
		}
	} else {
		if (rpd->state == RPDATA_STATE_IDLE) {
			ret = hal_sem_wait(rpd->conn_notify_sem);
			if (ret != 0) {
				rpd_err("sem wait error");
			}
		}
		ret = _rpdata_connect_ack(rpd);
		if (!ret)
			rpd->state = RPDATA_STATE_CONNECTED;
	}

	return ret;
}

int rpdata_send(rpdata_t *rpd, unsigned int offset, unsigned int data_len)
{
	int cmd, ret;

	if (rpd->state != RPDATA_STATE_CONNECTED) {
		rpd_err("rpd(%s) state:0x%x", rpd->name, rpd->state);
		return -1;
	}
	if (rpd->info->len - offset < data_len) {
		rpd_err("len invalid, len:%u,ofs:%u,dlen:%u",
			rpd->info->len, offset, data_len);
		return -1;
	}

	cmd = RPDATA_IOCTL_SEND | RPDATA_AMP_MAGIC | RPDATA_AMP_IN;
	cmd |= RPDATA_AMP_SDIR(RPDATA_DIR_SELF);
	cmd |= RPDATA_AMP_INSTANCE(rpd->info->instance);
	ret = rpdata_ioctl(rpd->dir, cmd, rpd->info->addr + offset, data_len);
	return ret;
}

int rpdata_process(rpdata_t *rpd, unsigned int offset, unsigned int data_len)
{
	int cmd, ret;

	if (rpd->state != RPDATA_STATE_CONNECTED) {
		rpd_err("rpd(%s) state:0x%x", rpd->name, rpd->state);
		return -1;
	}
	if (rpd->info->len - offset < data_len) {
		rpd_err("len invalid, len:%u,ofs:%u,dlen:%u",
			rpd->info->len, offset, data_len);
		return -1;
	}

	cmd = RPDATA_IOCTL_PROCESS | RPDATA_AMP_MAGIC | RPDATA_AMP_IN | RPDATA_AMP_OUT;
	cmd |= RPDATA_AMP_SDIR(RPDATA_DIR_SELF);
	cmd |= RPDATA_AMP_INSTANCE(rpd->info->instance);
	ret = rpdata_ioctl(rpd->dir, cmd, rpd->info->addr + offset, data_len);
	return ret;
}

int rpdata_recv(rpdata_t *rpd, void *buf, int len, int timeout_ms)
{
	int ret;
	struct rpd_recv_cb_item item = {0};

	rpd_debug("");
	if (!rpd->recv_rb) {
		ret = hal_queue_recv(rpd->recv_queue, &item, pdMS_TO_TICKS(timeout_ms));
		rpd_debug("ret = %d", ret);
		if (ret != 0) {
			return ret;
		}
		rpd_debug("item data:%p, len:%" PRIu32 ", info->addr:%p", item.data, item.len, rpd->info->addr);
		if (len > item.len)
			len = item.len;
		memcpy(buf, item.data, len);
	} else
		len = hal_ringbuffer_get(rpd->recv_rb, buf, len, timeout_ms);

	rpd_debug(" return %d.\n", len);
	return len;
}

int rpdata_set_recv_cb(rpdata_t *rpd, struct rpdata_cbs *cbs)
{
	rpd_debug("%s: Set cbs 0x%p\n", rpd->name, cbs);
	if (!rpd)
		return -1;
	rpd->cbs = cbs;
	return 0;
}

int rpdata_set_recv_ringbuffer(rpdata_t *rpd, int size)
{
	rpd_debug("%s: Set Recv Ringbuffer.\n", rpd->name);
	if (rpd->recv_rb) {
		rpd_err("%s ringbuffer alread exist, overwrite it.\n", rpd->name);
		hal_ringbuffer_release(rpd->recv_rb);
	}
	rpd->recv_rb = hal_ringbuffer_init(size);
	if (!rpd->recv_rb) {
		rpd_err("%s ringbuffer alloc failed.\n", rpd->name);
		return -ENOMEM;
	}

	return 0;
}

int rpdata_clear_recv_ringbuffer(rpdata_t *rpd, int size)
{
	rpd_debug("%s: Clear Recv Ringbuffer.\n", rpd->name);
	if (!rpd->recv_rb) {
		rpd_err("%s ringbuffer is not set.\n", rpd->name);
		return 0;
	}

	hal_ringbuffer_release(rpd->recv_rb);
	rpd->recv_rb = NULL;

	return 0;
}

void *rpdata_buffer_addr(rpdata_t *rpd)
{
	return rpd->info->addr;
}

int rpdata_buffer_len(rpdata_t *rpd)
{
	return (int)rpd->info->len;
}

int rpdata_set_private_data(rpdata_t *rpd, void *data)
{
	rpd->private_data = data;
	return 0;
}

void *rpdata_get_private_data(rpdata_t *rpd)
{
	return rpd->private_data;
}

static int _rpdata_realdestroy(rpdata_t *rpd)
{
	struct rpdata_type_info *tinfo;

	tinfo = rpd->tinfo;
	rpd->state = RPDATA_STATE_DESTROY;
	rpd_info("type(%s,0x%02x) name(%s,0x%02x) is %s",
			rpd->name, rpd->id_idx,
			tinfo->type, tinfo->type_idx,
			rpd->is_master ? "master" : "slave");
	hal_sem_delete(rpd->conn_ack_sem);
	hal_sem_delete(rpd->conn_notify_sem);
	hal_sem_delete(rpd->destroy_sem);
{
	/* TODO */
	int count = 0;
	struct rpd_recv_cb_item item = {0};
	while (!hal_is_queue_empty(rpd->recv_queue)) {
		rpd_info("wait queue flush,count:%d", count);
		hal_msleep(2);
		if (count++ > 5) {
			hal_queue_recv(rpd->recv_queue, &item, 1);
			rpd_info("item.data:%p, item.len:%d", item.data, item.len);
		}
	}
	hal_queue_delete(rpd->recv_queue);
	if (rpd->recv_rb)
		hal_ringbuffer_release(rpd->recv_rb);
}

	if (rpd->is_master) {
		amp_align_free(rpd->info->addr);
		rpd->info->addr = NULL;
		amp_align_free(rpd->info);
		rpd->info = NULL;
	}
	list_del(&rpd->list);

	/* tinfo destroy */
	if (tinfo != NULL && list_empty(&tinfo->head)) {
		rpd_debug("tinfo list del");
		type_bit_clr(rpd->dir, tinfo->type_idx);
		list_del(&tinfo->list);
		free(tinfo);
		rpd->tinfo = NULL;
	}

	memset(rpd, 0, sizeof(rpdata_t));
	free(rpd);

	return 0;
}

int rpdata_trydestroy(rpdata_t *rpd)
{
	if (!rpd->is_master)
		return -1;
	rpd->state = RPDATA_STATE_DISCONNECTED;
	return 0;
}

int rpdata_destroy(rpdata_t *rpd)
{
	int ret = 0;

	if (!rpd)
		return -1;

	if (rpd->state == RPDATA_STATE_IDLE)
		goto real_destroy;

	if (rpd->is_master) {
		/*
		 * master, only change state.
		 * real destroy should be performed by IOCTL_WAIT_DESTROY
		 */
		rpd->state = RPDATA_STATE_DISCONNECTED;
		ret = _rpdata_wait_destroy(rpd);
		if (ret != 0) {
			rpd_err("wait destroy failed");
			return -1;
		}
		rpd->use_cnt--;
	} else {
		/*
		 * slave, only change state.
		 * real destroy should be performed by IOCTL_WAIT_DESTROY
		 */
		ret = _rpdata_notify_destroy(rpd);
		if (ret != 0) {
			rpd_err("notify destroy failed");
			return -1;
		}
		if (rpd->state == RPDATA_STATE_DISCONNECTED)
			hal_sem_post(rpd->destroy_sem);
		else
			rpd->state = RPDATA_STATE_DISCONNECTED;
		rpd->use_cnt--;
		return 0;
	}

real_destroy:
	_rpdata_realdestroy(rpd);

	return ret;
}


/*
 * serivce, remote processor
 */
static int amp_service_rpdata_notify(int8_t dir, struct rpdata_info *rpdi)
{
	uint8_t type = RPDATA_AMP_INSTANCE_TO_TYPE(rpdi->instance);
	uint8_t id = RPDATA_AMP_INSTANCE_TO_ID(rpdi->instance);
	rpdata_t *rpd = NULL;
	struct rpdata_type_info *tinfo = NULL;

	rpd_debug("");
	tinfo = _rpdata_find_type(dir, rpdi->type);
	if (!tinfo) {
		rpd_info("type:%s not found, create it", rpdi->type);
		goto create_tinfo;
	}

	rpd_debug("tinfo->type:%s", tinfo->type);
	list_for_each_entry(rpd, &tinfo->head, list) {
		rpd_debug("rpd name:%s", rpd->name);
		if (!strncmp(rpd->name, rpdi->name, RPDATA_NAME_LEN)) {
			rpd_info("found it(%s)", rpd->name);
			goto found_rpd;
		}
	}
	rpd_debug("");

create_tinfo:
	rpd = _rpdata_create(dir, rpdi->type, rpdi->name, 0, false);
found_rpd:
	rpd_debug("rpd=%p", rpd);
	/* update rpd info, and notify */
	rpd->info = rpdi;
	if (g_rpd_debug_mask)
		HEXDUMP(rpd->info, sizeof(struct rpdata_info));
	rpd->id_idx = id;
	if (!tinfo)
		tinfo = _rpdata_find_type(dir, rpdi->type);
	if (!tinfo->type_idx)
		tinfo->type_idx = type;
	rpd_info("set type/id idx: (%d,%d)", tinfo->type_idx, rpd->id_idx);

	/* update bitmap */
	/*
	 * update type_idx in 2 case:
	 * 1. first create tinfo
	 * 2. tinfo->type_idx has value
	 */

	if (type_bit_set(dir, tinfo->type_idx) < 0 &&
		strncmp(tinfo->type, rpdi->type, sizeof(tinfo->type)) != 0) {
		rpd_err("type_bit_set failed, 0x%08x%08x, idx=%d",
			(uint32_t)(RPDATA_TYPE_BITMAP(dir) & 0xFFFFFFFFUL),
			(uint32_t)(RPDATA_TYPE_BITMAP(dir) >> 32),
			tinfo->type_idx);
		return -1;
	}

	if (id_bit_set(rpd->id_idx, tinfo) < 0) {
		rpd_err("id_bit_set failed, 0x%08x%08x, idx=%d",
			(uint32_t)(tinfo->id_bitmap & 0xFFFFFFFFUL),
			(uint32_t)(tinfo->id_bitmap >> 32),
			rpd->id_idx);
		return -1;
	}
	rpd_debug("");

	/* update state */
	if (rpd->state == RPDATA_STATE_WAIT_CONNECT) {
		rpd_debug("conn_notify_sem post (rpd:%p, sem:%p)", rpd, rpd->conn_notify_sem);
		hal_sem_post(rpd->conn_notify_sem);
		rpd->state = RPDATA_STATE_CONNECTED;
	} else if (rpd->state == RPDATA_STATE_IDLE) {
		rpd->state = RPDATA_STATE_WAIT_CONNECT;
	}
	rpd_debug("");

	return 0;
}

static int amp_service_rpdata_send(int8_t dir, int16_t instance, void *data, uint32_t len)
{
	rpdata_t *rpd = NULL;
	int ret = 0;
	int8_t type;
	int8_t id;

	rpd_debug("");
	type = RPDATA_AMP_INSTANCE_TO_TYPE(instance);
	id = RPDATA_AMP_INSTANCE_TO_ID(instance);

	rpd_debug("type=0x%02x, id=0x%02x", type, id);
	rpd = _rpdata_find_byidx(dir, type, id);
	rpd_debug("rpd=%p", rpd);
	if (!rpd)
		return -1;
	rpd_debug("data:%p, len:%u", data, len);
	if (rpd->cbs->recv_cb) {
		ret = rpd->cbs->recv_cb(rpd, data, len);
	}
	rpd_debug("ret=%d", ret);

	return ret;
}

static int amp_service_rpdata_connect_ack(int8_t dir, int16_t instance)
{
	rpdata_t *rpd = NULL;
	int8_t type;
	int8_t id;

	type = RPDATA_AMP_INSTANCE_TO_TYPE(instance);
	id = RPDATA_AMP_INSTANCE_TO_ID(instance);

	rpd = _rpdata_find_byidx(dir, type, id);
	if (!rpd)
		return -1;
	rpd_debug("conn_ack_sem post (rpd:%p, sem:%p)", rpd, rpd->conn_ack_sem);
	hal_sem_post(rpd->conn_ack_sem);
	rpd->state = RPDATA_STATE_CONNECTED;
	return 0;
}

static int amp_service_rpdata_wait_destroy(int8_t dir, int16_t instance)
{
	rpdata_t *rpd = NULL;
	int8_t type;
	int8_t id;

	type = RPDATA_AMP_INSTANCE_TO_TYPE(instance);
	id = RPDATA_AMP_INSTANCE_TO_ID(instance);

	rpd = _rpdata_find_byidx(dir, type, id);
	if (!rpd)
		return -1;

	if (rpd->state != RPDATA_STATE_DISCONNECTED &&
		rpd->state != RPDATA_STATE_WAIT_CONNECT) {
		/* wait slave rpd destroy */
		rpd_debug("wait slave rpd destroy_sem");
		rpd->state = RPDATA_STATE_DISCONNECTED;
		if (hal_sem_wait(rpd->destroy_sem) != 0) {
			rpd_err("sem wait error");
		}
	}
	rpd_debug("destroy slave rpd");
	_rpdata_realdestroy(rpd);
	return 0;
}

static int amp_service_rpdata_notify_destroy(int8_t dir, int16_t instance)
{
	rpdata_t *rpd = NULL;
	int8_t type;
	int8_t id;

	type = RPDATA_AMP_INSTANCE_TO_TYPE(instance);
	id = RPDATA_AMP_INSTANCE_TO_ID(instance);

	rpd = _rpdata_find_byidx(dir, type, id);
	if (!rpd)
		return -1;
	rpd->state = RPDATA_STATE_DISCONNECTED;
	rpd_debug("notify rpd(%p,%s) disconnected", rpd, rpd->name);

	return 0;
}

/* amp rpdata service call this function */
int amp_service_rpdata_ioctl(int cmd, void *data, uint32_t len)
{
	int ret = -1;
	int8_t sdir = RPDATA_AMP_CMD_TO_SDIR(cmd);

	RPDATA_CHECK_INIT();

	rpd_debug("ioctl cmd:0x%08x", RPDATA_AMP_IOCTL_MASK(cmd));
	switch (RPDATA_AMP_IOCTL_MASK(cmd)) {
	/* master start */
	case RPDATA_IOCTL_NOTIFY:
		/*
		 * arg0: rpdata_info pointer
		 * arg1: rpdata_info len
		 */
		if (len != sizeof(struct rpdata_info)) {
			rpd_err("unexpectd len:%d", len);
			break;
		}
		ret = amp_service_rpdata_notify(sdir, data);
		break;
	case RPDATA_IOCTL_WAIT_DESTROY:
		/*
		 * arg0: NULL
		 * arg1: 0
		 */
		ret = amp_service_rpdata_wait_destroy(sdir, RPDATA_AMP_INSTANCE_MASK(cmd));
		break;
	/* slave start */
	case RPDATA_IOCTL_CON_ACK:
		/*
		 * arg0: NULL
		 * arg1: 0
		 */
		ret = amp_service_rpdata_connect_ack(sdir, RPDATA_AMP_INSTANCE_MASK(cmd));
		break;
	case RPDATA_IOCTL_NOTIFY_DESTROY:
		/*
		 * arg0: NULL
		 * arg1: 0
		 */
		ret = amp_service_rpdata_notify_destroy(sdir, RPDATA_AMP_INSTANCE_MASK(cmd));
		break;
	/* master&slave start */
	case RPDATA_IOCTL_SEND:
		/*
		 * arg0: data
		 * arg1: len
		 */
		ret = amp_service_rpdata_send(sdir, RPDATA_AMP_INSTANCE_MASK(cmd), data, len);
		break;
	case RPDATA_IOCTL_PROCESS:
		/*
		 * arg0: data
		 * arg1: len
		 */
		ret = amp_service_rpdata_send(sdir, RPDATA_AMP_INSTANCE_MASK(cmd), data, len);
		break;
	default:
		rpd_err("unknown cmd:0x%x", cmd);
		break;
	}
	return ret;
}

/**********************************************************************/


/*
 * rpdata utils command
 */
#include <console.h>
#include <unistd.h>

static void _do_rpd_list(int8_t dir)
{
	struct rpdata_type_info *tinfo = NULL;
	/*
	 * type
	 *   ├── name
	 *   └── name
	 *
	 */
	printf("%-8s%-24s %s\n", "id", "type+name", "state");
	list_for_each_entry(tinfo, RPDATA_TYPE_HEAD(dir), list) {
		rpdata_t *rpd = NULL;
		printf("0x%02x    %s\n", tinfo->type_idx, tinfo->type);
		list_for_each_entry(rpd, &tinfo->head, list) {
			int is_last = list_is_last(&rpd->list, &tinfo->head);
			printf("  0x%02x     %s %-16s %c%c%c%c\n", rpd->id_idx,
				is_last ? "└───" : "├───", rpd->name,
				rpd->state & 0xff, (rpd->state >> 8) & 0xff,
				(rpd->state >> 16) & 0xff, (rpd->state >> 24) & 0xff);
		}
	}
}

static void do_rpd_list(void)
{
	int i;
	for (i = 1; i <= RPDATA_DIR_MAX; i++) {
		if (i == RPDATA_DIR_SELF)
			continue;
		printf("----------------------------------------\n");
		printf("RPdata %s <--> %s\n", rpdata_dir_to_name(RPDATA_DIR_SELF), rpdata_dir_to_name(i));
		_do_rpd_list(i);
	}
	printf("========================================\n");
}

static void rpd_usage(void)
{
	printf("Usgae: rpd [option]\n");
	printf("-h,        rpd help\n");
	printf("-v,        rpdata version\n");
	printf("-l,        list rpdata\n");
	printf("-d,        debug switch, 0-disable; 1-enable\n");
}

static int cmd_rpd(int argc, char *argv[])
{
	int c;

	RPDATA_CHECK_INIT();

	optind = 0;
	while ((c = getopt(argc, argv, "hvld:")) != -1) {
		switch (c) {
		case 'l':
			do_rpd_list();
			break;
		case 'd':
			g_rpd_debug_mask = atoi(optarg);
			break;
		case 'v':
			printf("rpdata version: %s\n", RPDATA_VERSION);
			return 0;
		case 'h':
		default:
			rpd_usage();
			break;
		}
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rpd, rpd, rpdata utils);

/**********************************************************************/
