#include <hal_reset.h>
#include <hal_atomic.h>
#include <hal_osal.h>
#include <hal_waitqueue.h>
#include "hal_clk.h"
#include "hal_timer.h"
#include "hal_log.h"
#include "hal_mutex.h"
#include "FreeRTOS/_os_mutex.h"

#include "cj_board_cfg.h"
#include "platform_csi.h"
#include "csi/hal_csi_jpeg.h"
#include "csi_camera/csi.h"
#include "jpeg/hal_jpeg.h"
#ifdef CONFIG_COMPONENTS_PM
#include "pm_devops.h"
#endif

#define MAX_BUF_NUM 5

typedef struct {
	struct csi_jpeg_mem csi_mem[MAX_BUF_NUM];
	unsigned char csi_mem_count;
	struct list_head csi_active;
	struct list_head csi_done;
	bool csi_stream;
//jpeg
	struct csi_jpeg_mem jpeg_mem[MAX_BUF_NUM];
	unsigned char jpeg_mem_count;
	struct list_head jpeg_active;
	struct list_head jpeg_done;

	hal_mutex_t lock;
	hal_spinlock_t slock;
	hal_waitqueue_head_t csi_waitqueue;
	hal_waitqueue_head_t jpeg_waitqueue;

	hal_clk_t mbus_csi_clk;
	hal_clk_t bus_csi_jpeg_clk;
	hal_clk_t csi_jpeg_clk;
	hal_clk_t csi_jpeg_clk_src;
	struct reset_control *csi_jpeg_reset;

	struct csi_jpeg_fmt output_fmt;
} csi_jpeg_private;

static csi_jpeg_private *gcsi_jpeg_priv;

static csi_jpeg_private *csi_jpeg_getpriv()
{
	return gcsi_jpeg_priv;
}

static void csi_jpeg_setpriv(csi_jpeg_private *csi_jpeg_priv)
{
	gcsi_jpeg_priv = csi_jpeg_priv;
}

static int sunxi_csi_clk_init(void)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_reset_id_t reset_id = RST_CSI_JPE;
	hal_clk_id_t bus_clk_id = CLK_BUS_CSI_JPE;
	hal_clk_id_t dev_clk_id = CLK_CSI_JPE;
	hal_clk_id_t mbus_clk_id = CLK_MBUS_CSI;

	csi_jpeg_priv->csi_jpeg_reset = hal_reset_control_get(reset_type, reset_id);
	if (hal_reset_control_deassert(csi_jpeg_priv->csi_jpeg_reset)) {
		cj_err("csi reset deassert failed!\n");
		return -1;
	}

	csi_jpeg_priv->mbus_csi_clk = hal_clock_get(clk_type, mbus_clk_id);
	if (hal_clock_enable(csi_jpeg_priv->mbus_csi_clk)) {
		cj_err("csi clk enable mbus csi clk failed!\n");
		return -1;
	}

	csi_jpeg_priv->bus_csi_jpeg_clk = hal_clock_get(clk_type, bus_clk_id);
	if (hal_clock_enable(csi_jpeg_priv->bus_csi_jpeg_clk)) {
		cj_err("csi clk enable bus csi jpe failed!\n");
		return -1;
	}

	csi_jpeg_priv->csi_jpeg_clk = hal_clock_get(clk_type, dev_clk_id);
	csi_jpeg_priv->csi_jpeg_clk_src = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_DEVICE);
	hal_clk_set_parent(csi_jpeg_priv->csi_jpeg_clk, csi_jpeg_priv->csi_jpeg_clk_src);
	hal_clk_set_rate(csi_jpeg_priv->csi_jpeg_clk, CSI_JPEG_CLK);
	cj_dbg("set CSI_JPEG_CLK rate = %u\n", CSI_JPEG_CLK);
	cj_dbg("get CSI_JPEG_CLK rate = %u\n", hal_clk_get_rate(csi_jpeg_priv->csi_jpeg_clk));
	if (hal_clock_enable(csi_jpeg_priv->csi_jpeg_clk)) {
		cj_err("csi clk enable devclk failed!");
		return -1;
	}
	cj_dbg("sunxi_csi_clk_init ok!");

	return 0;
}

static int sunxi_csi_clk_exit(void)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();

	if (hal_reset_control_assert(csi_jpeg_priv->csi_jpeg_reset)) {
		cj_err("csi reset assert failed!\n");
		return -1;
	}
	hal_reset_control_put(csi_jpeg_priv->csi_jpeg_reset);

	if (hal_clock_disable(csi_jpeg_priv->bus_csi_jpeg_clk)) {
		cj_err("csi clk disable bus csi jpe failed!\n");
		return -1;
	}

	if (hal_clock_disable(csi_jpeg_priv->csi_jpeg_clk)) {
		cj_err("csi clk disable devclk failed!");
		return -1;
	}

	return 0;
}

void hal_csi_sensor_get_sizes(unsigned int *width, unsigned int *height)
{
	struct sensor_win_size *sensor_sizes;
	sensor_sizes = &SENSOR_FUNC_WIN_SIZE;
	*width = sensor_sizes->width;
	*height = sensor_sizes->height;
}

void hal_csi_jpeg_set_fmt(struct csi_jpeg_fmt *intput_fmt)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	struct csi_fmt csi_output_fmt;

	memcpy(&csi_jpeg_priv->output_fmt, intput_fmt, sizeof(struct csi_jpeg_fmt));

	csi_output_fmt.width = intput_fmt->width;
	csi_output_fmt.height = intput_fmt->height;
	hal_csi_set_fmt(&csi_output_fmt);

//jpeg
	struct jpeg_fmt jpeg_output_fmt;

	if (intput_fmt->scale) {
		jpeg_output_fmt.width = intput_fmt->width / 2;
		jpeg_output_fmt.height = intput_fmt->height / 2;

	} else {
		jpeg_output_fmt.width = intput_fmt->width;
		jpeg_output_fmt.height = intput_fmt->height;
	}
	jpeg_output_fmt.line_mode = intput_fmt->line_mode;
	jpeg_output_fmt.output_mode = intput_fmt->output_mode;
	hal_jpeg_set_fmt(&jpeg_output_fmt);

}

int hal_csi_jpeg_reqbuf(unsigned int count)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	unsigned int size;
	unsigned int jpeg_size;
	unsigned int csi_size;
	int i;

	if (count < 2) {
		count = 3;
		cj_warn("%s:csi buffer count must morn than 2, set to 3!\n");
	}
	csi_jpeg_priv->csi_mem_count = count;
	cj_dbg("[%s:%d] count = %d\n", __func__,__LINE__, count);

	if (csi_jpeg_priv->output_fmt.line_mode == ONLINE_MODE)
		csi_size = 0;
	else
		csi_size = csi_jpeg_priv->output_fmt.width * csi_jpeg_priv->output_fmt.height * 3 / 2;  // yuv420
	jpeg_size = JPEG_OUTPUT_BUFF_SIZE;//may be can set to JPEG_OUTPUT_BUFF_SIZE
	if (csi_jpeg_priv->output_fmt.output_mode == PIX_FMT_OUT_NV12)
		size = csi_size + 16;
	else
		size = csi_size + jpeg_size + JPEG_HEADER_LEN + 1040; //1024+16
	cj_print("csi_size = %d, jpeg_size = %d, total size = %d\n", csi_size, jpeg_size, size);

	hal_mutex_lock(csi_jpeg_priv->lock);
	for (i = 0; i < count; i++) {
		csi_jpeg_priv->csi_mem[i].index = i;
		csi_jpeg_priv->csi_mem[i].buf.size = csi_size;
#if ((CONFIG_ARCH_HAVE_DCACHE & 0xF) != 0)
		csi_jpeg_priv->csi_mem[i].buf.addr = (void *)ALIGN_16B((uint32_t)hal_malloc_coherent(size));  // align with 0x10 ALIGN_16B
#else
		csi_jpeg_priv->csi_mem[i].buf.addr = (void *)ALIGN_16B((uint32_t)malloc(size));  // align with 0x10 ALIGN_16B
#endif
		if (csi_jpeg_priv->csi_mem[i].buf.addr == NULL) {
			cj_err("csi malloc size :%d failed\n", size);
			while (i-- > 0) {
#if ((CONFIG_ARCH_HAVE_DCACHE & 0xF) != 0)
				hal_free_coherent(csi_jpeg_priv->csi_mem[i].buf.addr);
#else
				free(csi_jpeg_priv->csi_mem[i].buf.addr);
#endif
			}
			return -1;
		}

		memset(csi_jpeg_priv->csi_mem[i].buf.addr, 0, size);
		cj_dbg("i = %d addr = 0x%08x\n", i, csi_jpeg_priv->csi_mem[i].buf.addr);
		list_add_tail(&csi_jpeg_priv->csi_mem[i].list, &csi_jpeg_priv->csi_active);
	}

	if (csi_jpeg_priv->output_fmt.output_mode == PIX_FMT_OUT_NV12)
		goto set_addr;

jpeg_reqbuf:
	csi_jpeg_priv->jpeg_mem_count = count;
	cj_dbg("[%s:%d], count=%d\n", __func__,__LINE__, count);

	for (i = 0; i < count; i++) {
		csi_jpeg_priv->jpeg_mem[i].index = i;
		csi_jpeg_priv->jpeg_mem[i].buf.size = jpeg_size;

		if (csi_jpeg_priv->output_fmt.line_mode == ONLINE_MODE) {
			csi_jpeg_priv->jpeg_mem[i].buf.addr = (void *)ALIGN_1K((uint32_t)csi_jpeg_priv->csi_mem[i].buf.addr + JPEG_HEADER_LEN);
		} else {
			csi_jpeg_priv->jpeg_mem[i].buf.addr = (void *)ALIGN_1K((uint32_t)csi_jpeg_priv->csi_mem[i].buf.addr + JPEG_HEADER_LEN
												    + csi_jpeg_priv->csi_mem[i].buf.size); //after yuv data
		}
		list_add_tail(&csi_jpeg_priv->jpeg_mem[i].list, &csi_jpeg_priv->jpeg_active);
	}

set_addr:
	hal_ve_rst_ctl_release();
	hal_jpeg_clk_en();
	if (csi_jpeg_priv->output_fmt.output_mode == PIX_FMT_OUT_NV12)
		hal_csi_set_addr(csi_jpeg_priv->csi_mem[0].buf.addr);
	else
		hal_csi_jpeg_set_addr(&(csi_jpeg_priv->csi_mem), &(csi_jpeg_priv->jpeg_mem),
				csi_jpeg_priv->jpeg_mem_count); // for first frame arrive

	hal_mutex_unlock(csi_jpeg_priv->lock);

	return 0;
}


int hal_csi_jpeg_freebuf(void)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	struct csi_jpeg_mem *csi_mem;
	struct csi_jpeg_mem *jpeg_mem;
	int i;

	if (csi_jpeg_priv->csi_stream) {
		cj_err("please stream off csi and jpef frist\n");
		return -1;
	}

	hal_mutex_lock(csi_jpeg_priv->lock);
	while (!list_empty(&csi_jpeg_priv->csi_done)) {
		csi_mem = list_entry(csi_jpeg_priv->csi_done.next, struct csi_jpeg_mem, list);
		list_del(&csi_mem->list);
		cj_dbg("csi buf%d delect from done queue\n", csi_mem->index);
	}
	while (!list_empty(&csi_jpeg_priv->csi_active)) {
		csi_mem = list_entry(csi_jpeg_priv->csi_active.next, struct csi_jpeg_mem, list);
		list_del(&csi_mem->list);
		cj_dbg("csi buf%d delect from active queue\n", csi_mem->index);
	}

	for (i = 0; i < csi_jpeg_priv->csi_mem_count; i++) {
#if ((CONFIG_ARCH_HAVE_DCACHE & 0xF) != 0)
		hal_free_coherent(csi_jpeg_priv->csi_mem[i].buf.addr);
#else
		free(csi_jpeg_priv->csi_mem[i].buf.addr);
#endif
	}

jpeg_freebuf:
	while (!list_empty(&csi_jpeg_priv->jpeg_done)) {
		jpeg_mem = list_entry(csi_jpeg_priv->jpeg_done.next, struct csi_jpeg_mem, list);
		list_del(&jpeg_mem->list);
		cj_dbg("jpeg buf%d delect from done queue\n", jpeg_mem->index);
	}
	while (!list_empty(&csi_jpeg_priv->jpeg_active)) {
		jpeg_mem = list_entry(csi_jpeg_priv->jpeg_active.next, struct csi_jpeg_mem, list);
		list_del(&jpeg_mem->list);
		cj_dbg("jpeg buf%d delect from active queue\n", jpeg_mem->index);
	}
	hal_mutex_unlock(csi_jpeg_priv->lock);

	return 0;
}

struct csi_jpeg_mem *hal_csi_dqbuf(unsigned int timeout_msec)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	struct csi_jpeg_mem *csi_mem;
	int ret;

	if (csi_jpeg_priv->output_fmt.line_mode == ONLINE_MODE) {
		cj_err("online mode, there is no yuv data to dequeue!\n");
		return NULL;
	}

	cj_dbg("hal_csi_dqbuf timeout_msec = %d", timeout_msec);
	ret = hal_wait_event_timeout(csi_jpeg_priv->csi_waitqueue, list_empty(&csi_jpeg_priv->csi_done) == 0, timeout_msec);
	if (ret <= 0) {
		cj_err("%s:csi dqbuf timeout %d msec!\n", __func__, timeout_msec);
		return NULL;
	}

	if (list_empty(&csi_jpeg_priv->csi_done)) {
		cj_err("%s:csi done queue is empty\n", __func__);
		return NULL;
	}

	hal_mutex_lock(csi_jpeg_priv->lock);
	csi_mem = list_entry(csi_jpeg_priv->csi_done.next, struct csi_jpeg_mem, list);

	cj_dbg("%s line: %d addr = 0x%08x size = %d\n", __func__, __LINE__,
				csi_mem->buf.addr, csi_mem->buf.size);
	cj_dbg("csi buf%d dqueue\n", csi_mem->index);

	hal_mutex_unlock(csi_jpeg_priv->lock);

	return csi_mem;
}

void hal_csi_qbuf(void)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	struct csi_jpeg_mem *csi_mem;
	uint32_t cpu_sr;

	if (csi_jpeg_priv->output_fmt.line_mode == ONLINE_MODE) {
		cj_err("online mode, there is no yuv data to queue!\n");
		return;
	}

	hal_mutex_lock(csi_jpeg_priv->lock);
	if (list_empty(&csi_jpeg_priv->csi_done)) {
		cj_err("%s:csi done queue is empty\n", __func__);
		return;
	}

	cpu_sr = hal_spin_lock_irqsave(&csi_jpeg_priv->slock);

	csi_mem = list_entry(csi_jpeg_priv->csi_done.next, struct csi_jpeg_mem, list);
	list_move_tail(&csi_mem->list, &csi_jpeg_priv->csi_active);

	cj_dbg("csi buf%d queue\n", csi_mem->index);

	hal_spin_unlock_irqrestore(&csi_jpeg_priv->slock, cpu_sr);
	hal_mutex_unlock(csi_jpeg_priv->lock);
}

//jpeg
struct csi_jpeg_mem *hal_jpeg_dqbuf(unsigned int timeout_msec)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	struct csi_jpeg_mem *jpeg_mem;
	int ret;

	ret = hal_wait_event_timeout(csi_jpeg_priv->jpeg_waitqueue, list_empty(&csi_jpeg_priv->jpeg_done) == 0, timeout_msec);
	if (ret <= 0) {
		cj_err("%s:jpeg wait buffer timeout %d msec!\n", __func__, timeout_msec);
		return NULL;
	}

	if (list_empty(&csi_jpeg_priv->jpeg_done)) {
		cj_err("%s:jpeg done queue is empty\n", __func__);
		return NULL;
	}

	hal_mutex_lock(csi_jpeg_priv->lock);
	jpeg_mem = list_entry(csi_jpeg_priv->jpeg_done.next, struct csi_jpeg_mem, list);

	cj_dbg("jpeg buf%d dqueue\n", jpeg_mem->index);

	hal_mutex_unlock(csi_jpeg_priv->lock);

	return jpeg_mem;
}

void hal_jpeg_qbuf(void)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	struct csi_jpeg_mem *jpeg_mem;
	uint32_t cpu_sr;

	hal_mutex_lock(csi_jpeg_priv->lock);
	if (list_empty(&csi_jpeg_priv->jpeg_done)) {
		cj_err("%s:jpeg done queue is empty\n", __func__);
		return;
	}

	cpu_sr = hal_spin_lock_irqsave(&csi_jpeg_priv->slock);

	jpeg_mem = list_entry(csi_jpeg_priv->jpeg_done.next, struct csi_jpeg_mem, list);

	list_move_tail(&jpeg_mem->list, &csi_jpeg_priv->jpeg_active);

	cj_dbg("jpeg buf%d queue\n", jpeg_mem->index);

	hal_spin_unlock_irqrestore(&csi_jpeg_priv->slock, cpu_sr);
	hal_mutex_unlock(csi_jpeg_priv->lock);

}

void hal_csi_jpeg_s_stream(unsigned int on)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();

	cj_dbg("hal_csi_jpeg_s_stream\n");

	if (on) {
		csi_jpeg_priv->csi_stream = 1;
		hal_jpeg_s_stream(on);
		hal_csi_s_stream(on);
		hal_sensor_s_stream(on);
		if (csi_jpeg_priv->output_fmt.scale)
			hal_csi_jpeg_scale();
		hal_usleep(200);
		csi_capture_start(CSI_CAP_VIDEO);
		if (csi_jpeg_priv->output_fmt.line_mode == ONLINE_MODE)
			jpeg_enc_start();
		hal_usleep(200);
	} else {
		csi_capture_stop();
		hal_csi_s_stream(on);
		hal_sensor_s_stream(on);
		csi_jpeg_priv->csi_stream = 0;
	}

}

static hal_irqreturn_t csi_jpeg_isr(void *data)//CSI_JPEG_IRQHandler pay attention
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();
	jpeg_private *jpeg_priv = jpeg_getpriv();
	jpeg_mpartbuffinfo jpeg_mpart_info;
	unsigned int csi_int_status = csi_ini_get_status();
	unsigned int jpeg_int_status = jpeg_ini_get_status();
	struct csi_jpeg_mem *csi_mem;
	struct csi_jpeg_mem *jpeg_mem;
	uint32_t cpu_sr;
	uint32_t len;

	cpu_sr = hal_spin_lock_irqsave(&csi_jpeg_priv->slock);

	cj_dbg("line: %d csi_int_status = %x jpeg_int_status = %x!",
			__LINE__, csi_int_status, jpeg_int_status);

	if (!csi_jpeg_priv->csi_stream) {
		csi_ini_clear_status(CSI_INT_STA_ALL_PD);
		jpeg_int_clear_status(JPEG_VE_INT_ALL);
		cj_dbg("csi_stream is already closed!");
		goto unlock;
	}

	if (jpeg_int_status & JPEG_FIFO_OVERFLOW) {
		jpeg_int_clear_status(JPEG_FIFO_OVERFLOW);
		cj_dbg("CSI&JPEG fifo overflow int pending!!");
	}

	if (jpeg_int_status & JPEG_INT_ERR || csi_int_status & CSI_INT_INPUT_SIZE_CHG_EN) {
		hal_ve_rst_ctl_reset();
		hal_jpeg_s_stream(1);
		hal_csi_s_stream(1);
		cj_err("CSI&JPEG excption!!");
		goto unlock;
	}

	if (jpeg_int_status & JPEG_CSI_WB_FINISH) {
		jpeg_int_clear_status(JPEG_CSI_WB_FINISH);
	}

	if (csi_int_status & CSI_INT_STA_FRM_END_PD) {

		if (csi_jpeg_priv->output_fmt.line_mode == OFFLINE_MODE) {
			if (jpeg_priv->jpgVeEn && csi_jpeg_priv->output_fmt.output_mode != PIX_FMT_OUT_NV12) {
				jpeg_enc_start();
				goto jpeg_isr;
			}

			if (&csi_jpeg_priv->csi_active == csi_jpeg_priv->csi_active.next->next) {
				cj_dbg("csi only one buffer left int active queue!!");
				goto jpeg_isr;
			}

			csi_mem = list_entry(csi_jpeg_priv->csi_active.next->next, struct csi_jpeg_mem, list);
			//set addr to csi reg
			hal_csi_set_addr(csi_mem->buf.addr);

			csi_mem = list_entry(csi_jpeg_priv->csi_active.next, struct csi_jpeg_mem, list);
			list_move_tail(&csi_mem->list, &csi_jpeg_priv->csi_done);
			hal_wake_up(&csi_jpeg_priv->csi_waitqueue);
		}
	}

jpeg_isr:
	//pay attention
	if (jpeg_priv->memPartEn && (jpeg_int_status & JPEG_MEM_PART_INT)) {

		len = (jpeg_priv->memPartCnt + 1) * jpeg_priv->memPartSize - jpeg_priv->memPartOffSet;

		jpeg_mpart_info.buff_offset = jpeg_priv->memPartOffSet;
		jpeg_mpart_info.size = len;
		jpeg_mpart_info.tail = 0;
		jpeg_priv->memPartOffSet += len;
		jpeg_priv->memCurSize += len;

		jpeg_priv->memPartCnt++;
		if (jpeg_priv->memPartCnt >= jpeg_priv->memPartNum) {
			jpeg_priv->memPartCnt = 0;
			jpeg_priv->memPartOffSet = 0;
		}

		if (&csi_jpeg_priv->jpeg_active == csi_jpeg_priv->jpeg_active.next->next) {
			cj_dbg("jpeg only one buffer left int active queue!!");
		}

		jpeg_mem = list_entry(csi_jpeg_priv->jpeg_active.next, struct csi_jpeg_mem, list);
		jpeg_mem->mpart_info = jpeg_mpart_info;
		if (csi_jpeg_priv->output_fmt.cb)
			csi_jpeg_priv->output_fmt.cb(jpeg_mem);
		jpeg_mempart_take();
	}

	if (jpeg_int_status & JPEG_ENC_FINISH) {

		uint32_t encode_len = 0;
		encode_len = ((JPEG->HARDWARE_OFFSET - jpeg_priv->jpgOutStmOffset)+7)/8;//unit is bytes
		encode_len = (encode_len+3)/4*4;
		cj_dbg(">>>debug, encode_len = %d[offset:%d]", encode_len, jpeg_priv->jpgOutStmOffset);

		if (jpeg_priv->memPartEn) {

			jpeg_mpart_info.buff_offset = (uint32_t)jpeg_priv->jpgOutStmAddr[jpeg_priv->jpgOutBufId];
			jpeg_mpart_info.size = encode_len;
			jpeg_mpart_info.tail = 1;

			if (encode_len >= jpeg_priv->memCurSize) {
				len = encode_len - jpeg_priv->memCurSize;
				if (len > jpeg_priv->memPartSize)
					len = jpeg_priv->memPartSize;
				jpeg_mpart_info.buff_offset = jpeg_priv->memPartOffSet;
				jpeg_mpart_info.size = len;
				jpeg_priv->memCurSize = 0;

				jpeg_priv->memPartCnt++;
				if (jpeg_priv->memPartCnt >= jpeg_priv->memPartNum)
					jpeg_priv->memPartCnt = 0;
				jpeg_priv->memPartOffSet = jpeg_priv->memPartSize * jpeg_priv->memPartCnt;
				jpeg_priv->memPartOffSet = ALIGN_32B(jpeg_priv->memPartOffSet);
				jpeg_priv->jpgOutStmOffset = jpeg_priv->memPartOffSet * 8;
			}
		}

		if (jpeg_priv->jpgVeEn && csi_jpeg_priv->output_fmt.line_mode == ONLINE_MODE)
			jpeg_enc_start();

		jpeg_mem = list_entry(csi_jpeg_priv->jpeg_active.next->next, struct csi_jpeg_mem, list);
		//set addr to jpeg reg
		hal_jpeg_set_addr(jpeg_mem->index);
		jpeg_priv->jpgOutBufId = jpeg_mem->index;

		if (&csi_jpeg_priv->jpeg_active == csi_jpeg_priv->jpeg_active.next->next) {
			cj_dbg("jpeg only one buffer left int active queue!!");
			goto unlock;
		}

		jpeg_mem = list_entry(csi_jpeg_priv->jpeg_active.next, struct csi_jpeg_mem, list);
		if (jpeg_priv->memPartEn) {
			jpeg_mem->mpart_info = jpeg_mpart_info;
			if (csi_jpeg_priv->output_fmt.cb)
				csi_jpeg_priv->output_fmt.cb(jpeg_mem);
			jpeg_mempart_take();
		} else {
			jpeg_mem->buf.size = encode_len;
			if (csi_jpeg_priv->output_fmt.cb)
				csi_jpeg_priv->output_fmt.cb(jpeg_mem);
		}

		list_move_tail(&jpeg_mem->list, &csi_jpeg_priv->jpeg_done);
		hal_wake_up(&csi_jpeg_priv->jpeg_waitqueue);
	}

unlock:
	csi_ini_clear_status(csi_int_status);
	jpeg_int_clear_status(jpeg_int_status);

	hal_spin_unlock_irqrestore(&csi_jpeg_priv->slock, cpu_sr);

	return 0;
}

#ifdef CONFIG_COMPONENTS_PM
static int csi_jpeg_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	cj_print("csi_jpeg_suspend\n");
	hal_csi_jpeg_s_stream(0);
	csi_ini_clear_status(CSI_INT_STA_ALL_PD);
	jpeg_int_clear_status(JPEG_VE_INT_ALL);
	cj_print("csi stream off!!");
	return 0;
}

static int csi_jpeg_resume(struct pm_device *dev, suspend_mode_t mode)
{
	cj_print("csi_jpeg_resume\n");
	csi_ini_clear_status(CSI_INT_STA_ALL_PD);
	jpeg_int_clear_status(JPEG_VE_INT_ALL);
	hal_csi_jpeg_s_stream(1);
	cj_print("csi stream on!");
	return 0;
}

static struct pm_devops csi_jpeg_devops = {
    .suspend = csi_jpeg_suspend,
    .resume = csi_jpeg_resume,
};

static struct pm_device csi_jpeg_pm = {
    .name = "csi_jpeg",
    .ops = &csi_jpeg_devops,
};
#endif

HAL_Status hal_csi_jpeg_probe(void)
{
	csi_jpeg_private *csi_jpeg_priv;
	const char *isr_name = "csi_jpeg_isr";

	csi_jpeg_priv = malloc(sizeof(csi_jpeg_private));
	if (!csi_jpeg_priv) {
		cj_err("csi jpeg malloc faild\n");
		return HAL_ERROR;
	} else {
		memset(csi_jpeg_priv, 0, sizeof(csi_jpeg_private));
		csi_jpeg_setpriv(csi_jpeg_priv);
	}
	csi_jpeg_priv->lock = hal_mutex_create();
	if (!csi_jpeg_priv->lock) {
		cj_err("mutex create fail\n");
		goto emalloc;
	}

	hal_waitqueue_head_init(&csi_jpeg_priv->csi_waitqueue);
//jpeg
	hal_waitqueue_head_init(&csi_jpeg_priv->jpeg_waitqueue);
	INIT_LIST_HEAD(&csi_jpeg_priv->jpeg_active);
	INIT_LIST_HEAD(&csi_jpeg_priv->jpeg_done);
	hal_jpeg_init();

	hal_spin_lock_init(&csi_jpeg_priv->slock);

	INIT_LIST_HEAD(&csi_jpeg_priv->csi_active);
	INIT_LIST_HEAD(&csi_jpeg_priv->csi_done);

	sunxi_csi_clk_init();
	hal_csi_probe();

	/*request csi irq*/
	if (hal_request_irq(CSI_JPEG_IRQn, csi_jpeg_isr, isr_name, NULL) < 0) {
		cj_err("[csi] request irq error\n");
		goto freemutex;
	}
	hal_enable_irq(CSI_JPEG_IRQn);

	cj_print("[csi] probe ok, irq is %d\n", CSI_JPEG_IRQn);

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&csi_jpeg_pm);
#endif

	return HAL_OK;
freemutex:
	hal_mutex_delete(csi_jpeg_priv->lock);

emalloc:
	free(csi_jpeg_priv);
	csi_jpeg_setpriv(NULL);

	return HAL_ERROR;
}

HAL_Status hal_csi_jpeg_remove(void)
{
	csi_jpeg_private *csi_jpeg_priv = csi_jpeg_getpriv();

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&csi_jpeg_pm);
#endif
	hal_disable_irq(CSI_JPEG_IRQn);
	hal_free_irq(CSI_JPEG_IRQn);
	hal_jpeg_deinit();
	hal_csi_remove();
	sunxi_csi_clk_exit();

	hal_spin_lock_deinit(&csi_jpeg_priv->slock);
	hal_waitqueue_head_deinit(&csi_jpeg_priv->csi_waitqueue);
	hal_waitqueue_head_deinit(&csi_jpeg_priv->jpeg_waitqueue);

	hal_mutex_delete(csi_jpeg_priv->lock);

	free(csi_jpeg_priv);
	csi_jpeg_setpriv(NULL);

	return HAL_OK;
}

