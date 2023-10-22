/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PART'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNER'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PART'S TECHNOLOGY.


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

#include "udc.h"
#include "usb_phy.h"
#include "udc_platform.h"

#ifdef CONFIG_STANDBY
#include <standby/standby.h>
#elif defined(CONFIG_COMPONENTS_PM)
#include "pm_suspend.h"
#include "pm_wakelock.h"
#include "pm_devops.h"
#include <pm_debug.h>
#endif

#ifndef CONFIG_KERNEL_FREERTOS
#define KEY_UDC_IRQ_FLAG	"usbd_irq_flag"
#define KEY_UDC_DRIVER_LEVEL	"usbd_driver_level"
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#endif

/* UDC base address */
static volatile UDC_REGISTER_T *musb = (UDC_REGISTER_T *)SUNXI_USB_OTG_PBASE;
static volatile USBPHY_REGISTER_T *musb_phy = (USBPHY_REGISTER_T *)(SUNXI_USB_OTG_PBASE + USB_PHY_BASE_OFFSET);

#ifdef CONFIG_UDC_USE_DMA
static uint8_t ep_dma_enable[9] = {0};
#endif

sunxi_udc_io_t g_sunxi_udc;
/* UDC private data */
udc_priv_t g_udc;

#if defined(CONFIG_SOC_SUN20IW1) || defined(CONFIG_SOC_SUN20IW2)
/*8K +64byte fifo size */
#define SW_UDC_EPNUMS	5
static udc_fifo_t g_ep_fifo[] = {
	{0,			0,								64,	0}, /* ep0 */
	{1 | USB_DIR_IN,	64,								1024,	1},
	{1 | USB_DIR_OUT,	64 + 1024,							1024,	1},
	{2 | USB_DIR_IN,	64 + 1024 + 1024 ,						1024,	1},
	{2 | USB_DIR_OUT,	64 + 1024 + 1024 + 1024,					1024,	1},
	{3 | USB_DIR_IN,	64 + 1024 + 1024 + 1024 + 1024,					512,	0},
	{3 | USB_DIR_OUT,	64 + 1024 + 1024 + 1024 + 1024 + 512,				512,	0},
	{4 | USB_DIR_IN,	64 + 1024 + 1024 + 1024 + 1024 + 512 + 512,			512,	0},
	{4 | USB_DIR_OUT,	64 + 1024 + 1024 + 1024 + 1024 + 512 + 512 + 512,		512,	0},
	{5 | USB_DIR_IN,	64 + 1024 + 1024 + 1024 + 1024 + 512 + 512 + 512 + 512,		1024,	1},
	{5 | USB_DIR_OUT,	64 + 1024 + 1024 + 1024 + 1024 + 512 + 512 + 512 + 512 + 1024,	1024,	1},
};
#else
/* NOTICE: 4K or 8K fifo size */
#define SW_UDC_EPNUMS	4
static udc_fifo_t g_ep_fifo[] = {
	{0,			0,						64,	0}, /* ep0 */
	{1 | USB_DIR_IN,	64,						512,	0},
	{1 | USB_DIR_OUT,	64 + 512,					512,	0},
	{2 | USB_DIR_IN,	64 + 512 + 512 ,				512,	0},
	{2 | USB_DIR_OUT,	64 + 512 + 512 + 512,				512,	0},
	{3 | USB_DIR_IN,	64 + 512 + 512 + 512 + 512,			512,	0},
	{3 | USB_DIR_OUT,	64 + 512 + 512 + 512 + 512 + 512,		512,	0},
	{4 | USB_DIR_IN,	64 + 512 + 512 + 512 + 512 + 512 + 512,		512,	0},
	{4 | USB_DIR_OUT,	64 + 512 + 512 + 512 + 512 + 512 + 512 + 512,	512,	0},
};
#endif
/**
 * ep_fifo_in[i] = {n} i: the physic ep index, n: ep_fifo's index for the ep
 *
 * eg: ep_fifo_in[2] = {3} ===> ep2_in is in ep_fifo[3]
 *
 * ep3_iso_name and ep4_int_name cannot be tx or rx simultaneously.
 *
 */
static const uint32_t g_ep_fifo_in[] = {0, 1, 3, 5, 7};
static const uint32_t g_ep_fifo_out[] = {0, 2, 4, 6, 8};

#define SW_UDC_ENDPOINTS	ARRAY_SIZE(g_ep_fifo)

/* identify ep0 control request */
static uint8_t g_crq_bRequest;
static uint8_t g_crq_wIndex;

static hal_spinlock_t udc_lock;

void *usbc_get_musb_phy(void)
{
	return (void *)musb_phy;
}

static void usbc_wakeup_clear_change_detect(void)
{
	USB_DRV_ClearBits32(&musb_phy->iscr, USB_ISCR_VBUS_CHANGE_DETECT);
	USB_DRV_ClearBits32(&musb_phy->iscr, USB_ISCR_ID_CHANGE_DETECT);
	USB_DRV_ClearBits32(&musb_phy->iscr, USB_ISCR_DPDM_CHANGE_DETECT);
}

static void usbc_enable_dpdm_pullup(bool enable)
{
	if (enable) {
		USB_DRV_SetBits32(&musb_phy->iscr, USB_ISCR_DPDM_PULLUP_EN);
	} else {
		USB_DRV_ClearBits32(&musb_phy->iscr, USB_ISCR_DPDM_PULLUP_EN);
	}

	usbc_wakeup_clear_change_detect();

	log_udc_dbg("dp dm pull up %s\r\n", enable ? "enabled" : "disabled");
}

static void usbc_enable_id_pullup(bool enable)
{
	if (enable) {
		USB_DRV_SetBits32(&musb_phy->iscr, USB_ISCR_ID_PULLUP_EN);
	} else {
		USB_DRV_ClearBits32(&musb_phy->iscr, USB_ISCR_ID_PULLUP_EN);
	}

	usbc_wakeup_clear_change_detect();

	log_udc_dbg("id pull up %s\r\n", enable ? "enabled" : "disabled");
}

static void usbc_force_id(uint32_t id_type)
{
	USB_DRV_ClearBits32(&musb_phy->iscr, USB_ISCR_FORCE_ID_MASK);
	USB_DRV_SetBits32(&musb_phy->iscr, id_type);

	usbc_wakeup_clear_change_detect();

	log_udc_dbg("force id type: 0x%x\r\n", id_type);
}

static void usbc_force_vbus_valid(uint32_t vbus_type)
{
	USB_DRV_ClearBits32(&musb_phy->iscr, USB_ISCR_FORCE_VBUS_MASK);
	USB_DRV_SetBits32(&musb_phy->iscr, vbus_type);

	usbc_wakeup_clear_change_detect();

	log_udc_dbg("force vbus valid type: 0x%x\r\n", vbus_type);
}

static void usbc_select_bus(udc_io_type_t io_type, udc_ep_type_t ep_type, uint32_t ep_index)
{
	uint32_t reg_val;

	reg_val = USB_DRV_Reg8(&musb->vend0);

	if (io_type == UDC_IO_TYPE_DMA) {
		if (ep_type == UDC_EP_TYPE_TX) {
			reg_val |= ((ep_index - 0x01) << 1) << USB_VEND0_DRQ_SEL; /* drq_sel */
			reg_val |= 0x1 << USB_VEND0_BUS_SEL; /* io_dma */
		} else {
			reg_val |= (((ep_index - 0x01) << 1) + 1) << USB_VEND0_DRQ_SEL; /* drq_sel */
			reg_val |= 0x1 << USB_VEND0_BUS_SEL; /* io_dma */
		}
	} else {
		reg_val &= 0x00; /* clear drq_sel, select pio */
	}

	/*
	 * in SUN8IW5 SUN8IW6 and later ic, FIFO_BUS_SEL bit(bit24 of reg0x40
	 * for host/device) is fixed to 1, the hw guarantee that it's ok for
	 * cpu/inner_dma/outer_dma transfer.
	 */
#if !defined(CONFIG_SOC_SUN20IW1) && !defined(CONFIG_SOC_SUN20IW2)
	reg_val |= 0x1 << USB_VEND0_BUS_SEL;
#endif
	USB_DRV_WriteReg8(&musb->vend0, reg_val);
}

static void usbc_phy_set_ctl(bool set)
{
	/* NOTICE: 40nm platform is different */
#if defined(CONFIG_ARCH_SUN20IW2)
	/* USB BIAS enable, set 0 if usb disable to save power at sleep */
	USB_DRV_SetBits32(SUNXI_GPRCM_BASE + USB_BIAS_CTRL, USB_BIAS_CTRL_EN);
	/* TODO: 40nm platform's phyctrl register is the same as 28nm */
	if (set) {
		USB_DRV_ClearBits32(&musb_phy->phyctrl28nm, USB_PHYCTL40NM_SIDDQ);
	} else {
		USB_DRV_SetBits32(&musb_phy->phyctrl28nm, USB_PHYCTL40NM_SIDDQ);
	}
#else
	if (set) {
		USB_DRV_SetBits32(&musb_phy->phyctrl28nm, USB_PHYCTL28NM_VBUSVLDEXT);
		USB_DRV_ClearBits32(&musb_phy->phyctrl28nm, USB_PHYCTL28NM_SIDDQ);
	} else {
		USB_DRV_SetBits32(&musb_phy->phyctrl28nm, USB_PHYCTL28NM_SIDDQ);
	}
#endif
	log_udc_dbg("phy %s ctl\r\n", set ? "set" : "clear");
}

static void usbc_phy_otg_sel(bool otg_sel)
{
	if (otg_sel) {
		USB_DRV_SetBits32(&musb_phy->physel, USB_PHYSEL_OTG_SEL);
	} else {
		USB_DRV_ClearBits32(&musb_phy->physel, USB_PHYSEL_OTG_SEL);
	}
}

static uint32_t usbc_get_active_ep(void)
{
	return USB_DRV_Reg8(&musb->index);
}

static void usbc_select_active_ep(uint8_t ep_index)
{
	USB_DRV_WriteReg8(&musb->index, ep_index);
}

static void usbc_udc_disable(void)
{
	log_udc_dbg("udc disable\r\n");

	/* disable all interrupts */
	USB_DRV_WriteReg8(&musb->intrusbe, 0);
	USB_DRV_WriteReg8(&musb->intrtxe, 0);
	USB_DRV_WriteReg8(&musb->intrrxe, 0);

	/* clear the interrupt registers */
	USB_DRV_WriteReg(&musb->intrtx, 0xffff);
	USB_DRV_WriteReg(&musb->intrrx, 0xffff);
	USB_DRV_WriteReg8(&musb->intrusb, 0xff);

	/* clear soft connect */
	USB_DRV_ClearBits8(&musb->power, USB_POWER_SOFTCONN);
}

static void usbc_udc_enable(void)
{
	log_udc_dbg("udc enable\r\n");

	/* config usb transfer type, default: bulk transfer */
	USB_DRV_ClearBits8(&musb->power, USB_POWER_ISOUPDATE);

	/* config usb gadget speed, default: high speed */
	USB_DRV_SetBits8(&musb->power, USB_POWER_HSENAB);

	/* enable usb bus interrupt */
	USB_DRV_SetBits8(&musb->intrusbe, USB_INTRUSB_SUSPEND
						| USB_INTRUSB_RESUME
						| USB_INTRUSB_RESET);

	/* enable ep0 interrupt */
	USB_DRV_SetBits(&musb->intrtxe, USB_INTRE_EPEN << 0);

	/* set soft connect */
	USB_DRV_SetBits8(&musb->power, USB_POWER_SOFTCONN);

	// krhino_spin_lock_init(&g_udc.lock);
}

/* mask the useless irq, save disconect, reset, resume, suspend */
static uint32_t usbc_filtrate_irq(uint32_t usb_irq)
{
	uint32_t irq = usb_irq;

	irq &= ~(USB_INTRUSBE_VBUSERROR
		| USB_INTRUSBE_SESSREQ
		| USB_INTRUSBE_CONN
		| USB_INTRUSBE_SOF);

	USB_DRV_ClearBits8(&musb->intrusb, USB_INTRUSBE_VBUSERROR
						| USB_INTRUSBE_SESSREQ
						| USB_INTRUSBE_CONN
						| USB_INTRUSBE_SOF);

	return irq;
}

static void *usbc_select_fifo(uint32_t ep_index)
{
	uint32_t offset;

	offset = 0x0 + (ep_index << 2);

	return (void *)((char *)&musb->fifo0 + offset);
}

static uint32_t usbc_read_packet(void *fifo, uint32_t cnt, void *buf)
{
	uint8_t *buf8 = NULL;
	uint32_t len = 0, i32 = 0, i8 = 0, *buf32 = NULL;

	if (cnt == 0)
		return cnt;

	/* adjust data */
	buf32 = buf;
	len = cnt;

	i32 = len >> 2;
	i8 = len & 0x03;

	/* deal with 4 byte part */
	while (i32--) {
		*buf32++ = USB_DRV_Reg32(fifo);
	}

	/* deal with not 4 byte part */
	buf8 = (uint8_t *)buf32;
	while (i8--) {
		*buf8++ = USB_DRV_Reg8(fifo);
	}

	return len;
}

static void usbc_write_packet(void *fifo, uint32_t cnt, void *buf)
{
	uint8_t *buf8 = NULL;
	uint32_t len = 0, i32 = 0, i8 = 0, *buf32 = NULL;

	/* adjust data */
	buf32 = buf;
	len = cnt;

	i32 = len >> 2;
	i8 = len & 0x03;

	/* deal with 4 byte part */
	while (i32--) {
		USB_DRV_WriteReg32(fifo, *buf32++);
	}

	/* deal with not 4 byte part */
	buf8 = (uint8_t *)buf32;
	while (i8--) {
		USB_DRV_WriteReg8(fifo, *buf8++);
	}
}

static void usbc_ep_config_default(uint8_t is_in)
{
	/* NOTICE: must already select active ep */

	if (is_in) {
		/* clear tx csr */
		USB_DRV_WriteReg(&musb->txcsr, 0x00);
		/* clear tx ep max packet */
		USB_DRV_WriteReg(&musb->txmap, 0x00);
		/* flush fifo */
		USB_DRV_WriteReg(&musb->txcsr, USB_TXCSR_CLRDATATOG | USB_TXCSR_FLUSHFIFO);
	} else {
		/* clear rx csr */
		USB_DRV_WriteReg(&musb->rxcsr, 0x00);
		/* clear rx ep max packet */
		USB_DRV_WriteReg(&musb->rxmap, 0x00);
		/* flush fifo */
		USB_DRV_WriteReg(&musb->rxcsr, USB_RXCSR_CLRDATATOG | USB_RXCSR_FLUSHFIFO);
	}
}

static void usbc_ep_config(uint32_t ts_type, uint16_t maxpacket,
			uint8_t is_double_fifo, uint8_t is_in)
{
	uint32_t reg_val = 0, temp = 0;

	/* NOTICE: must already select active ep */

	if (is_in) {
		/* config tx csr */
		reg_val = USB_TXCSR_MODE | USB_TXCSR_CLRDATATOG | USB_TXCSR_FLUSHFIFO;
		USB_DRV_WriteReg(&musb->txcsr, reg_val);
		if (is_double_fifo) /* config twice */
			USB_DRV_WriteReg(&musb->txcsr, reg_val);

		/* config tx ep max packet */
		reg_val = USB_DRV_Reg(&musb->txmap);
		temp = maxpacket & USB_TXMAXP_MAXPAYLOAD_MASK;
		reg_val |= temp;
		USB_DRV_WriteReg(&musb->txmap, reg_val);

		/* config tx ep transfer type */
		switch (ts_type) {
		case USB_ENDPOINT_XFER_ISOC:
			USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_ISO);
			break;
		case USB_ENDPOINT_XFER_INT:
		case USB_ENDPOINT_XFER_BULK:
			USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_ISO);
			break;
		default:
			USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_ISO);
			break;
		}
	} else {
		/* config rx csr */
		reg_val = USB_RXCSR_CLRDATATOG | USB_RXCSR_FLUSHFIFO;
		USB_DRV_WriteReg(&musb->rxcsr, reg_val);
		if (is_double_fifo) /* config twice */
			USB_DRV_WriteReg(&musb->rxcsr, reg_val);

		/* config rx ep max packet */
		reg_val = USB_DRV_Reg(&musb->rxmap);
		temp = maxpacket & USB_RXMAXP_MAXPAYLOAD_MASK;
		reg_val |= temp;
		USB_DRV_WriteReg(&musb->rxmap, reg_val);

		/* config rx ep transfer type */
		switch (ts_type) {
		case USB_ENDPOINT_XFER_ISOC:
			USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_ISO);
			break;
		case USB_ENDPOINT_XFER_INT:
			USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_DISNYET);
			break;
		case USB_ENDPOINT_XFER_BULK:
			USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_ISO);
			break;
		default:
			USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_ISO);
			break;
		}
	}
}

static void usbc_ep_fifo_config(uint32_t fifo_addr, uint32_t fifo_size,
			uint8_t is_double_fifo, uint8_t is_in)
{
	uint32_t temp = 0, size = 0, addr = 0;

	/* NOTICE: must already select active ep */

	/* 512 align */
	size = 0;
	temp = fifo_size + 511;
	temp &= ~511;
	temp >>= 3;
	temp >>= 1;
	while (temp) {
		size++;
		temp >>= 1;
	}

	/* caculate addr */
	addr = fifo_addr >> 3;

	if (is_in) {
		/* config fifo addr */
		USB_DRV_WriteReg(&musb->txfifoadd, addr);
		/* config fifo size */
		USB_DRV_WriteReg8(&musb->txfifosz, (size & USB_FIFOSZ_SIZE_MASK));
		if (is_double_fifo)
			USB_DRV_SetBits8(&musb->txfifosz, USB_FIFOSZ_DPB);
	} else {
		/* config fifo addr */
		USB_DRV_WriteReg(&musb->rxfifoadd, addr);
		/* config fifo size */
		USB_DRV_WriteReg8(&musb->rxfifosz, (size & USB_FIFOSZ_SIZE_MASK));
		if (is_double_fifo)
			USB_DRV_SetBits8(&musb->rxfifosz, USB_FIFOSZ_DPB);
	}
}

static void usbc_ep_intr_enable(uint8_t ep_idx, uint8_t is_in)
{
	/* NOTICE: must already select active ep */

	if (is_in) {
		USB_DRV_SetBits(&musb->intrtxe, USB_INTRE_EPEN << ep_idx);
	} else {
		USB_DRV_SetBits(&musb->intrrxe, USB_INTRE_EPEN << ep_idx);
	}
}

static void usbc_ep_intr_disable(uint8_t ep_idx, uint8_t is_in)
{
	/* NOTICE: must already select active ep */

	if (is_in) {
		USB_DRV_ClearBits(&musb->intrtxe, USB_INTRE_EPEN << ep_idx);
		USB_DRV_SetBits(&musb->intrtx, 0x1 << ep_idx);
	} else {
		USB_DRV_ClearBits(&musb->intrrxe, USB_INTRE_EPEN << ep_idx);
		USB_DRV_SetBits(&musb->intrrx, 0x1 << ep_idx);
	}
}

static udc_errno_t crq_get_status(struct usb_ctrlrequest *crq)
{
	void *fifo = NULL;
	uint16_t status = 0;
	uint8_t buf[8] = {0}, ep_idx = crq->wIndex & 0x7f, is_in = crq->wIndex & USB_DIR_IN;

	switch (crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			buf[0] = 0x00;
			buf[1] = 0x00;
			break;

		case USB_RECIP_DEVICE:
			buf[0] = 0x01;
			buf[1] = 0x00;
			break;

		case USB_RECIP_ENDPOINT:
			if (crq->wLength > 2)
				return UDC_ERRNO_CMD_INVALID;

			if (ep_idx == 0) {
				status = USB_DRV_Reg(&musb->txcsr) & USB_CSR0_SENDSTALL;
			} else {
				if (is_in) {
					status = USB_DRV_Reg(&musb->txcsr) & USB_TXCSR_SENDSTALL;
				} else {
					status = USB_DRV_Reg(&musb->rxcsr) & USB_RXCSR_SENDSTALL;
				}
			}

			status = status ? 1 : 0;
			if (status) {
				buf[0] = 0x01;
				buf[1] = 0x00;
			} else {
				buf[0] = 0x00;
				buf[1] = 0x00;
			}
			break;

		default:
			return UDC_ERRNO_CMD_INVALID;
	}

	/* need udelay(5)? */
	USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY);
	fifo = usbc_select_fifo(0);
	usbc_write_packet(fifo, crq->wLength, buf);
	USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_TXPKTRDY | USB_CSR0_DATAEND);

	return UDC_ERRNO_SUCCESS;
}

static uint32_t pio_read_fifo(udc_ep_t *ep,void* buf,uint32_t len)
{
	void *fifo = NULL;
	uint32_t idx, ret;

	idx = ep->ep_addr & 0x7f;
	fifo = usbc_select_fifo(idx);

	ret = usbc_read_packet(fifo, len, buf);

	// printf("cpu_rx: (ep%d, len:%d, actual:%d) max:%d \n", idx,
	// 	ep->data_len, ep->data_actual, ep->maxpacket);

	return ret;
}


static uint32_t pio_read_fifo_crq(struct usb_ctrlrequest *crq)
{
	uint32_t fifo_count = 0, i = 0;
	void *pout = crq, *fifo = NULL;

	fifo = usbc_select_fifo(0);
	fifo_count = USB_DRV_Reg(&musb->rxcount);

	log_udc_dbg("ep0 fifo count is %d\r\n", fifo_count);
	if (fifo_count != 8) {
		i = 0;

		while (i < 16 && (fifo_count != 8)) {
			fifo_count = USB_DRV_Reg(&musb->rxcount);
			i++;
		}

		if (i >= 16) {
			log_udc_err("ep0 get fifo len failed fifo_count:%d\r\n", fifo_count);
		} else {
			log_udc_err("i:%d\r\n", i);
		}
	}

	return usbc_read_packet(fifo, fifo_count, pout);
}

static int32_t pio_write_fifo(udc_ep_t *ep)
{
	void *fifo = NULL, *buf = NULL;
	uint32_t idx = 0, count = 0, old_ep_idx = 0;
	int is_last = 0;

	idx = ep->ep_addr & 0x7f;
	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(idx);

	if (!ep->pdata) {
		log_udc_dbg("ep data buf is NULL\r\n");
		return UDC_ERRNO_BUF_NULL;
	}

	count = min(ep->data_len - ep->data_actual, ep->maxpacket);
	buf = ep->pdata + ep->data_actual;
	fifo = usbc_select_fifo(idx);

	// printf("cpu_tx: (ep%d, len:%d, actual:%d) max:%d \n", idx,
	// ep->data_len, ep->data_actual, ep->maxpacket);

	usbc_write_packet(fifo, count, buf);
	ep->data_actual += count;

	if (count != ep->maxpacket || ep->data_len == ep->data_actual)
		is_last = 1;

	if (idx) {
		USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_UNDERRUN);
		USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_TXPKTRDY);
	} else {
		USB_DRV_SetBits(&musb->txcsr, USB_CSR0_TXPKTRDY);
		if (is_last) {
			USB_DRV_SetBits(&musb->txcsr, USB_CSR0_DATAEND);
		}
		if (USB_DRV_Reg(&musb->txcsr) & USB_CSR0_RXPKTRDY) {
			USB_DRV_SetBits(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY);
		}
		while (USB_DRV_Reg(&musb->txcsr) & USB_CSR0_TXPKTRDY);
	}

	usbc_select_active_ep(old_ep_idx);
	return is_last;
}
#ifdef CONFIG_UDC_USE_DMA
static void hal_udc_ep_set_dma_enable(uint8_t ep_addr, uint8_t enable)
{
	uint8_t ep_idx = 0, is_in = 0;

	ep_idx = ep_addr & 0x7f;
	is_in = ep_addr & USB_DIR_IN;

	if(ep_idx == 0){
		ep_dma_enable[0] = 0;
		return;
	}

	if (is_in) {
		ep_dma_enable[g_ep_fifo_in[ep_idx]] = enable;
	} else {
		ep_dma_enable[g_ep_fifo_out[ep_idx]] = enable;
	}
}

static void udc_usb_dma_tx_start(uint32_t ep_idx, uint32_t src_addr, uint32_t len)
{
	while (usb_dma_finish(g_udc.dma_tx_chan[ep_idx]));

	usb_dma_clr_irq_status(g_udc.dma_tx_chan[ep_idx]);

	usbc_ep_intr_disable(ep_idx, 1);
	usb_dma_irq_enable(g_udc.dma_tx_chan[ep_idx]);

	hal_dcache_clean((unsigned long)src_addr, len);
	usb_dma_start(g_udc.dma_tx_chan[ep_idx], src_addr, len);
}

static void udc_usb_dma_rx_start(uint32_t ep_idx, uint32_t dest_addr, uint32_t len)
{
	while (usb_dma_finish(g_udc.dma_rx_chan[ep_idx]));

	usb_dma_clr_irq_status(g_udc.dma_rx_chan[ep_idx]);
	usbc_ep_intr_disable(ep_idx, 0);
	usb_dma_irq_enable(g_udc.dma_rx_chan[ep_idx]);

	hal_dcache_clean_invalidate((unsigned long)dest_addr, len);
	usb_dma_start(g_udc.dma_rx_chan[ep_idx], dest_addr, len);
}

static void dma_read_fifo(udc_ep_t *ep,void* buf, uint32_t len)
{
	uint32_t idx;
	idx = ep->ep_addr & 0x7f;

	udc_usb_dma_rx_start(idx, (uint32_t)buf, len);

	// printf("dma_rx: (ep%d, len:%d, actual:%d) max:%d \n", idx,
	// 	ep->data_len, ep->data_actual, ep->maxpacket);
}

static int32_t dma_write_fifo(udc_ep_t *ep)
{
	void *buf;
	uint32_t idx;
	uint32_t count;

	if (!ep->pdata) {
		log_udc_err("ep data buf is NULL\r\n");
		return UDC_ERRNO_BUF_NULL;
	}

	idx = ep->ep_addr & 0x7f;
	count = min(ep->data_len - ep->data_actual, 16*ep->maxpacket);
	count = count - count%ep->maxpacket;
	buf = ep->pdata + ep->data_actual;
	ep->data_actual += count;

	// printf("dma_tx: (ep%d, len:%d, actual:%d) max:%d \n", idx,
	// 	ep->data_len, ep->data_actual, ep->maxpacket);

	udc_usb_dma_tx_start(idx, (uint32_t)buf, count);
	return 0;
}
static void udc_set_config_rx_dma(uint32_t dma_en,uint32_t ep_idx)
{
	uint32_t old_ep_idx;

	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);
	if (dma_en) {
		USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_DMAREQMODE);
		USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_AUTOCLEAR | USB_RXCSR_DMAREQENAB);
		USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_DMAREQMODE);
		USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_DMAREQMODE);
		if (0) {  // g_udc.device_desc.bDeviceProtocol
			USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_ISO);
		}
		usbc_select_bus(UDC_IO_TYPE_DMA, UDC_EP_TYPE_RX, ep_idx);
	} else {
		USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_AUTOCLEAR);
		USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_DMAREQMODE);
		USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_DMAREQENAB);
		if (0) {  // g_udc.device_desc.bDeviceProtocol
			USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_ISO);
		}
		usbc_select_bus(UDC_IO_TYPE_PIO, UDC_EP_TYPE_RX, ep_idx);
	}
	usbc_select_active_ep(old_ep_idx);
}

static void udc_set_config_tx_dma(uint32_t dma_en,uint32_t ep_idx)
{
	uint32_t old_ep_idx;

	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);
	if (dma_en) {
		USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_AUTOSET);
		USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_MODE);
		USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_DMAREQENAB);
		USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_DMAREQMODE);
		if (0) {
			USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_ISO);
		}
		usbc_select_bus(UDC_IO_TYPE_DMA, UDC_EP_TYPE_TX, ep_idx);
	} else {
		USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_AUTOSET);
		USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_DMAREQENAB);
		USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_DMAREQMODE);
		// USB_DRV_ClearBits(&musb->txcsr,USB_TXCSR_MODE);
		if (0) {
			USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_ISO);
		}
		usbc_select_bus(UDC_IO_TYPE_PIO, UDC_EP_TYPE_TX, ep_idx);
	}
	usbc_select_active_ep(old_ep_idx);
}

static void usb_ep_dma_init(uint32_t ep_addr)
{
	uint16_t usb_dma_burst;
	uint32_t usb_dma_config;
	uint8_t old_ep_idx;
	uint8_t ep_idx, is_in, fifo_idx;

	is_in = ep_addr & USB_DIR_IN;
	ep_idx = ep_addr & 0x7f;

	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);

	if (is_in) {
		fifo_idx = g_ep_fifo_in[ep_idx];
	} else {
		fifo_idx = g_ep_fifo_out[ep_idx];
	}

	log_udc_dbg("ep_idx:%d, is_in:%d\n", ep_idx, is_in);
	if (ep_dma_enable[fifo_idx] == 0) {
		log_udc_err("ep_idx:%d, is_in:%d\n", ep_idx, is_in);
		return;
	}
	if (is_in) {
		log_udc_dbg("tx:%d chan=%d\n", ep_addr, g_udc.dma_tx_chan[ep_idx]);
		if (g_udc.dma_tx_chan[ep_idx] == 0xff) {
			g_udc.dma_tx_chan[ep_idx] = usb_dma_request();
		}
		log_udc_dbg("tx:%d chan=%d\n", ep_addr, g_udc.dma_tx_chan[ep_idx]);
		usb_dma_burst = USB_DRV_Reg(&musb->txmap) & USB_TXMAXP_MAXPAYLOAD_MASK;
		usb_dma_config = usb_get_dma_tx_config(usb_dma_burst, ep_idx);
		usb_dma_setup(g_udc.dma_tx_chan[ep_idx], usb_dma_config);
		log_udc_dbg("tx %d:0x%x chan=%d\n", ep_addr, usb_dma_burst, g_udc.dma_tx_chan[ep_idx]);
	} else {
		log_udc_dbg("rx:%d chan=%d\n", ep_addr, g_udc.dma_tx_chan[ep_idx]);
		if (g_udc.dma_rx_chan[ep_idx] == 0xff) {
			g_udc.dma_rx_chan[ep_idx] = usb_dma_request();
		}
		usb_dma_burst = USB_DRV_Reg(&musb->rxmap) & USB_TXMAXP_MAXPAYLOAD_MASK;
		usb_dma_config = usb_get_dma_rx_config(usb_dma_burst, ep_idx);
		usb_dma_setup(g_udc.dma_rx_chan[ep_idx], usb_dma_config);
		log_udc_dbg("rx %d:0x%x chan=%d\n", ep_addr, usb_dma_burst, g_udc.dma_rx_chan[ep_idx]);
	}
	usbc_select_active_ep(old_ep_idx);
}
#endif

static int32_t udc_write_fifo(udc_ep_t *ep, bool in_interrupt)
{
#ifdef CONFIG_UDC_USE_DMA
	int dma_req = 0;
#endif
	int is_last = 0;
	uint32_t idx;
	uint32_t old_ep_idx;

	idx = ep->ep_addr & 0x7f;

	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(idx);
	if (!ep->data_len && !ep->pdata && !idx) {  // send null data
		USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY | USB_CSR0_DATAEND);
		ep->pdata = NULL;
		ep->data_actual = 0;
		g_udc.ep0state = UDC_EP0_IDLE;
		ep->ep_call_finish = 1;
		if (!in_interrupt)
			hal_spin_unlock(&udc_lock);
		g_udc.callback(ep->ep_addr, UDC_EVENT_TX_COMPLETE, NULL, ep->data_len);
		if (!in_interrupt)
			hal_spin_lock(&udc_lock);
		return 0;
	}
#ifdef CONFIG_UDC_USE_DMA
	dma_req = (idx && ((ep->data_len - ep->data_actual) >= ep->maxpacket) && ep_dma_enable[g_ep_fifo_in[idx]] != 0)? 1 : 0;
#endif
	do
	{
#ifdef CONFIG_UDC_USE_DMA
		if (dma_req) {
			udc_set_config_tx_dma(dma_req, idx);
			is_last = dma_write_fifo(ep);
		} else
#endif
		{
			is_last = pio_write_fifo(ep);
		}
		if (ep->data_len == ep->data_actual) {
			ep->pdata = NULL;
			ep->data_actual = 0;
			if (!idx) {//ep0 send finish
				g_udc.ep0state = UDC_EP0_IDLE;
				break;
			}
#ifdef CONFIG_UDC_USE_DMA
			if (!dma_req)
#endif
			{
				ep->ep_call_finish = 1;
				if (!in_interrupt)
					hal_spin_unlock(&udc_lock);
				g_udc.callback(ep->ep_addr, UDC_EVENT_TX_COMPLETE, NULL, ep->data_len);
				if (!in_interrupt)
					hal_spin_lock(&udc_lock);
			}
		}
	} while(!idx);
	usbc_select_active_ep(old_ep_idx);
	return is_last;
}

static int32_t udc_read_fifo(udc_ep_t *ep, bool need_callback, bool in_interrupt)
{
	bool is_last = 0;
	uint32_t dma_req = 0;
	void *buf = NULL;
	uint32_t idx = 0, fifo_count = 0, count = 0, old_ep_idx = 0;

	if (!ep->pdata || (ep->data_len <= ep->data_actual)) {
//		printf("%d E ep data buf is NULL %d %d rxcsr:0x%x count:%d\r\n", __LINE__, ep->data_len, ep->data_actual, USB_DRV_Reg(&musb->rxcsr), USB_DRV_Reg32(&musb->rxcount)&0x00000fff);
		return UDC_ERRNO_BUF_NULL;
	}
	idx = ep->ep_addr & 0x7f;
	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(idx);

retry:
	fifo_count = USB_DRV_Reg32(&musb->rxcount)&0x00000fff;

#ifdef CONFIG_UDC_USE_DMA
	count = ep->data_len - ep->data_actual;
	if (count >= ep->maxpacket && fifo_count == ep->maxpacket) {
		if (count % ep->maxpacket) {
			count = count - count % ep->maxpacket;
		}
	} else if (count > fifo_count) {
		count = fifo_count;
	}

	dma_req = (idx && count >= ep->maxpacket && ep_dma_enable[g_ep_fifo_out[idx]] != 0) ? 1 : 0;
#else
	count = fifo_count;
#endif
	if (fifo_count != (USB_DRV_Reg32(&musb->rxcount) & (0x00000fff))) {
		// printf("%d fifo_count:%d %d r\n", __LINE__, fifo_count,
		//        USB_DRV_Reg32(&musb->rxcount) & 0x00000fff);
		goto retry;
	}
	/* select fifo*/
	buf = ep->pdata + ep->data_actual;
	ep->data_actual += count;
	// printf("%d %d %d count:%d fifo_count:%d 0x%x %d r\n", __LINE__, ep->data_len,
	// ep->data_actual, count, fifo_count, USB_DRV_Reg(&musb->rxcsr),
	// USB_DRV_Reg32(&musb->rxcount)&0x00000fff);
#ifdef CONFIG_UDC_USE_DMA
	if (dma_req) {
		udc_set_config_rx_dma(dma_req, idx);
		dma_read_fifo(ep, buf, count);
	} else
#endif
	{
		pio_read_fifo(ep, buf, count);
	}

	if (idx != 0 && (fifo_count < ep->maxpacket || ep->data_len == ep->data_actual) && !dma_req) {
		is_last = 1;
	} else if (!dma_req) {
		is_last = (ep->data_len <= ep->data_actual) ? 1 : 0;
	}

	if (idx) { /* ep1~4*/
		if (!dma_req) {
			USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_RXPKTRDY | USB_RXCSR_OVERRUN | USB_RXCSR_DATAERROR);
			if (is_last) {
				void *pdata = ep->pdata;
				uint32_t data_actual = ep->data_actual;

				ep->pdata = NULL;
				ep->data_actual = 0;
				ep->data_len = 0;
				/* callback to user */
				if (need_callback && g_udc.callback && !ep->ep_call_finish) {
					if (!in_interrupt)
						hal_spin_unlock(&udc_lock);
					ep->ep_call_finish = 1;
					g_udc.callback(ep->ep_addr, UDC_EVENT_RX_DATA, pdata, data_actual);
					if (!in_interrupt)
						hal_spin_lock(&udc_lock);
				}
			}
		}
	} else { /* ep0 */
		if (is_last) {
			USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY | USB_CSR0_DATAEND);
			if (need_callback && g_udc.callback) {
				if (!in_interrupt)
					hal_spin_unlock(&udc_lock);
				g_udc.callback(ep->ep_addr, UDC_EVENT_RX_DATA, ep->pdata, ep->data_actual);
				if (!in_interrupt)
					hal_spin_lock(&udc_lock);
			}
			ep->pdata = NULL;
			ep->data_len = 0;
			ep->data_actual = 0;
			g_udc.ep0state = UDC_EP0_IDLE;
		} else {
			USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY);
		}
	}
	usbc_select_active_ep(old_ep_idx);
	return count;
}

static void udc_set_halt_ex(uint8_t ep_addr, int value);
static void udc_handle_ep0_idle(void)
{
	uint32_t len;
	uint32_t string_idx;
	uint32_t config_idx;
	struct usb_ctrlrequest *crq = &g_udc.crq;
	udc_ep_t *ep0 = &g_udc.ep0;
	int is_in = 0;
	static uint8_t set_config = 0;
	static uint8_t set_interface = 0;

	/* start control request */
	if (!(USB_DRV_Reg(&musb->txcsr) & USB_CSR0_RXPKTRDY)) {
		log_udc_dbg("ep0 setup data is not ready\r\n");
		return;
	}

	len = pio_read_fifo_crq(crq);
	if (len != sizeof(*crq)) {
		USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY);
		USB_DRV_SetBits(&musb->txcsr, USB_CSR0_SENDSTALL);
		return;
	}

	log_udc_dbg("ep0: bRequest = 0x%x, bRequestType = 0x%x, wValue = 0x%x, "
		    "wIndex = 0x%x, wLength = 0x%x\r\n",
		    crq->bRequest, crq->bRequestType, crq->wValue,
		    crq->wIndex, crq->wLength);

	g_udc.req_std = ((crq->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD);

	if (g_udc.req_std) { /* standard request */
		switch (crq->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			if (crq->bRequestType != USB_DIR_IN) {
				goto stall;
			}

			USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY);
			switch (crq->wValue >> 8) {
			case USB_DT_DEVICE:
				log_udc_dbg("get device descriptor\r\n");

				/* fill device descriptor */
				ep0->pdata = g_udc.device_desc;
				ep0->data_len = min(crq->wLength, (uint16_t)sizeof(*g_udc.device_desc));
				ep0->data_actual = 0;

				g_udc.ep0state = UDC_EP0_IN_DATA_PHASE;
				goto out;
			case USB_DT_CONFIG:
				log_udc_dbg("get configuration descriptor\r\n");

				ep0->pdata = g_udc.config_desc;
				ep0->data_len = min(crq->wLength, g_udc.config_desc_len);
				ep0->data_actual = 0;

				g_udc.ep0state = UDC_EP0_IN_DATA_PHASE;
				goto out;
			case USB_DT_STRING:
				log_udc_dbg("get string descriptor\r\n");

				string_idx = crq->wValue & 0xff;
				if (string_idx > g_udc.string_desc_num) {
					log_udc_err("get string descriptor index overflow\r\n");
					goto stall;
				}
				ep0->pdata = g_udc.string_desc[string_idx];
				ep0->data_len = min(crq->wLength, g_udc.string_desc[string_idx]->bLength);
				ep0->data_actual = 0;

				g_udc.ep0state = UDC_EP0_IN_DATA_PHASE;
				goto out;
			case USB_DT_BOS:
				USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY
								| USB_CSR0_DATAEND);
				g_udc.ep0state = UDC_EP0_IDLE;
				goto stall;
			default:
				/* not support */
				log_udc_err("Get descriptor request not supported\r\n");
				goto stall;
			}
			break;
		case USB_REQ_SET_CONFIGURATION:
			log_udc_dbg("set configuration\r\n");

			/* rx receive over, data end, tx packet ready */
			USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY | USB_CSR0_DATAEND);

			config_idx = crq->wValue & 0xff;
			if (config_idx > g_udc.device_desc->bNumConfigurations) {
				log_udc_err("set configuration index overflow\r\n");
				goto stall;
			}
			set_config = crq->wValue & 0xff;
			/* callback to user to reset configuration, interfaces and endpoints */
			// if (g_udc.callback)
			// 	g_udc.callback(0, UDC_EVENT_RX_STANDARD_REQUEST, crq, sizeof(*crq));
			break;
		case USB_REQ_GET_CONFIGURATION:
			log_udc_info("get configuration\r\n");
			ep0->pdata = &set_config;
			ep0->data_len = min(crq->wLength, 1);
			ep0->data_actual = 0;
			g_udc.ep0state = UDC_EP0_IN_DATA_PHASE;
			break;
		case USB_REQ_SET_INTERFACE:
			log_udc_dbg("set interface\r\n");

			/* rx receive over, data end, tx packet ready */
			USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY | USB_CSR0_DATAEND);
			set_interface = crq->wValue & 0xff;

			/* TODO: callback to user to set altsetting */
			break;
		case USB_REQ_GET_INTERFACE:
			log_udc_dbg("get interface\r\n");

			ep0->pdata = &set_interface;
			ep0->data_len = min(crq->wLength, 1);
			ep0->data_actual = 0;
			g_udc.ep0state = UDC_EP0_IN_DATA_PHASE;
			/* TODO */
			break;
		case USB_REQ_SET_ADDRESS:
			log_udc_dbg("set address\r\n");

			if (crq->bRequestType == USB_RECIP_DEVICE) {
				g_udc.address = crq->wValue & 0x7f;

				/* rx receive over, data end, tx packet ready */
				USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY | USB_CSR0_DATAEND);

				g_udc.ep0state = UDC_EP0_END_XFER;
				g_crq_bRequest = USB_REQ_SET_ADDRESS;

				return;
			}
			break;
		case USB_REQ_GET_STATUS:
			log_udc_dbg("get status\r\n");

			if (crq_get_status(crq) != UDC_ERRNO_SUCCESS)
				goto stall;
			break;
		case USB_REQ_CLEAR_FEATURE:
			printf("clear feature\r\n");
			/* --<1>--data direction must be host to device */
			if (crq->bRequestType & (1 << 7)) {
				log_udc_err("USB_REQ_CLEAR_FEATURE:\n");
				log_udc_err("data is not host to device\n");
				break;
			}

			USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY | USB_CSR0_DATAEND);

			/* --<3>--data stage */
			if (crq->bRequestType == USB_RECIP_DEVICE) {
				/* wValue 0-1 */
				if (crq->wValue) {
					/*dev->devstatus &= ~(1 << USB_DEVICE_REMOTE_WAKEUP);*/
				} else {
					int k = 0;
					for (k = 0; k < SW_UDC_ENDPOINTS; k++) {
						is_in = crq->wIndex & USB_DIR_IN;
						udc_set_halt_ex(g_ep_fifo[k].ep_addr, 0);
					}
				}
			} else if (crq->bRequestType == USB_RECIP_INTERFACE) {
				/* do nothing */
			} else if (crq->bRequestType == USB_RECIP_ENDPOINT) {
				/* --<3>--release the forbidden of ep */
				/* wValue 0-1 */
				if (crq->wValue) {
					/*dev->devstatus &= ~(1 << USB_DEVICE_REMOTE_WAKEUP);*/
				} else {
					int k = 0;
					is_in = crq->wIndex & USB_DIR_IN;
					for (k = 0; k < SW_UDC_ENDPOINTS; k++) {
						if (g_ep_fifo[k].ep_addr == (crq->wIndex & 0xff))
							udc_set_halt_ex(g_ep_fifo[k].ep_addr, 0);
					}
				}
			} else {
				log_udc_dbg("PANIC : nonsupport set feature request. (%d)\r\n", crq->bRequestType);
				goto stall;
			}
			g_udc.ep0state = UDC_EP0_IDLE;
			break;
		case USB_REQ_SET_FEATURE:
			log_udc_dbg("set feature %d\r\n",crq->bRequestType);
			g_udc.ep0state = UDC_EP0_IDLE;

			//--<3>--data stage
			if(crq->bRequestType == USB_RECIP_DEVICE){
				if((crq->wValue == USB_DEVICE_TEST_MODE) ){
					//setup packet receive over
					switch (crq->wIndex){
					case SUNXI_UDC_TEST_J:
							USB_DRV_WriteReg(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY|USB_CSR0_DATAEND);
							g_udc.ep0state = UDC_EP0_END_XFER;
							g_crq_wIndex = TEST_J;
							g_crq_bRequest = USB_REQ_SET_FEATURE;
							return;
					case SUNXI_UDC_TEST_K:
							USB_DRV_WriteReg(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY|USB_CSR0_DATAEND);
							g_udc.ep0state = UDC_EP0_END_XFER;
							g_crq_wIndex = TEST_K;
							g_crq_bRequest = USB_REQ_SET_FEATURE;
							return;
					case SUNXI_UDC_TEST_SE0_NAK:
							USB_DRV_WriteReg(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY|USB_CSR0_DATAEND);
							g_udc.ep0state = UDC_EP0_END_XFER;
							g_crq_wIndex = TEST_SE0_NAK;
							g_crq_bRequest = USB_REQ_SET_FEATURE;
							return;
					case SUNXI_UDC_TEST_PACKET:
							USB_DRV_WriteReg(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY|USB_CSR0_DATAEND);
							g_udc.ep0state = UDC_EP0_END_XFER;
							g_crq_wIndex = TEST_PACKET;
							g_crq_bRequest = USB_REQ_SET_FEATURE;
							return;
					default:
							break;
					}
				}
				USB_DRV_WriteReg(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY|USB_CSR0_DATAEND);
					//g_udc->devstatus |= (1 << USB_DEVICE_REMOTE_WAKEUP);
			}else if(crq->bRequestType == USB_RECIP_INTERFACE){
				//--<2>--token stage over
				USB_DRV_WriteReg(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY|USB_CSR0_DATAEND);
				//do nothing
			} else if(crq->bRequestType == USB_RECIP_ENDPOINT){
				//--<3>--forbidden ep
				USB_DRV_WriteReg(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY|USB_CSR0_DATAEND);
				int k = 0;
				for (k = 0; k < SW_UDC_ENDPOINTS; k++) {
					if (g_ep_fifo[k].ep_addr == (crq->wIndex & 0xff))
						udc_set_halt_ex(g_ep_fifo[k].ep_addr,1);
				}
			} else  {
				log_udc_info("PANIC : nonsupport set feature request. (%d)\n", crq->bRequestType);
				USB_DRV_WriteReg(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY|USB_CSR0_DATAEND);
				goto stall;
			}
			/* TODO */
			break;
		default:
			/* not support */
			USB_DRV_SetBits(&musb->txcsr,USB_CSR0_SERVICEDRXPKTRDY);
			log_udc_err("Standard request not supported\r\n");
			goto stall;
		}

		/* callback to user to handle after then */
		if (g_udc.callback) {
			g_udc.callback(0, UDC_EVENT_RX_STANDARD_REQUEST, crq, sizeof(*crq));
		}
	} else { /* class request */
		/* callback to user to handle specific class requests */
//		printf("%d 0x%08x :%d\n", __LINE__, USB_DRV_Reg32(&musb->txmap), USB_DRV_Reg32(&musb->rxcount)&0x00000fff);
		if (g_udc.callback) {
			g_udc.callback(0, UDC_EVENT_RX_CLASS_REQUEST, crq, sizeof(*crq));
		}
	}
out:
	/* finish data stage in just one interrupt */
	switch (g_udc.ep0state) {
	case UDC_EP0_IN_DATA_PHASE:
		if (!(USB_DRV_Reg(&musb->txcsr) & USB_CSR0_TXPKTRDY) &&
			pio_write_fifo(&g_udc.ep0)) {
			/* is last */
			g_udc.ep0state = UDC_EP0_IDLE;
		}
		break;
	case UDC_EP0_OUT_DATA_PHASE:
		if (USB_DRV_Reg(&musb->txcsr) & USB_CSR0_RXPKTRDY) {
			if (udc_read_fifo(&g_udc.ep0, true, true) == 0) {
				/* keep UDC_EP0_OUT_DATA_PHASE */
				return;
			}
		}
		g_udc.ep0state = UDC_EP0_IDLE;
		break;
	default:
		break;
	}

	return;

stall:
	log_udc_dbg("ep0 send stall...\r\n");
	USB_DRV_WriteReg(&musb->txcsr, USB_CSR0_SERVICEDRXPKTRDY);
	USB_DRV_SetBits(&musb->txcsr, USB_CSR0_SENDSTALL);
}

static void udc_handle_ep0(void)
{
	log_udc_dbg("Handling ep0, ep0state is %d\r\n", g_udc.ep0state);

	/* select ep0 */
	usbc_select_active_ep(0);

	/* clear stall status */
	if (USB_DRV_Reg(&musb->txcsr) & USB_CSR0_SENTSTALL) {
		log_udc_err("EP0 Stall\r\n");

		/* clear ep0 stall */
		USB_DRV_ClearBits(&musb->txcsr, USB_CSR0_SENDSTALL);
		USB_DRV_ClearBits(&musb->txcsr, USB_CSR0_SENTSTALL);

		g_udc.ep0state = UDC_EP0_IDLE;
		return;
	}

	/* clear setup end */
	if (USB_DRV_Reg(&musb->txcsr) & USB_CSR0_SETUPEND) {
		log_udc_dbg("EP0 Setup End\r\n");

		/* clear ep0 setup end */
		USB_DRV_SetBits(&musb->txcsr, USB_CSR0_SERVICEDSETUPEND);

		g_udc.ep0state = UDC_EP0_IDLE;
	}

	switch (g_udc.ep0state) {
	case UDC_EP0_IDLE:
		udc_handle_ep0_idle();
		break;
	case UDC_EP0_IN_DATA_PHASE: /* GET_DESCRIPTER etc... */
		log_udc_dbg("ep0 in data phase...\r\n");

		if (!(USB_DRV_Reg(&musb->txcsr) & USB_CSR0_TXPKTRDY))
			udc_write_fifo(&g_udc.ep0, true);
		break;
	case UDC_EP0_OUT_DATA_PHASE: /* SET_DESCRIPTER etc... */
		log_udc_dbg("ep0 out data phase...\r\n");

		if (USB_DRV_Reg(&musb->txcsr) & USB_CSR0_RXPKTRDY)
			udc_read_fifo(&g_udc.ep0, true, true);
		break;
	case UDC_EP0_END_XFER:
		log_udc_dbg("ep0 end xfer, g_crq_bRequest = 0x%x\r\n", g_crq_bRequest);

		switch (g_crq_bRequest) {
		case USB_REQ_SET_ADDRESS:
			/* clear ep0 setup end */
			USB_DRV_SetBits(&musb->txcsr, USB_CSR0_SERVICEDSETUPEND);

			/* set address */
			USB_DRV_WriteReg8(&musb->faddr, g_udc.address);
			log_udc_dbg("Set address: %d\r\n", g_udc.address);
			break;
		case USB_REQ_SET_FEATURE:
			/* TODO: enter test mode */
			break;
		default:
			break;
		}

		g_crq_bRequest = 0;
		g_udc.ep0state = UDC_EP0_IDLE;
		break;
	case UDC_EP0_STALL:
		log_udc_dbg("ep0 stall...\r\n");
		g_udc.ep0state = UDC_EP0_IDLE;
		break;
	}
}

static void udc_handle_ep(uint32_t fifo_idx)
{
	udc_fifo_t fifo_config;
	uint8_t ep_idx;
	uint8_t old_ep_idx;
	uint8_t is_in;

	fifo_config = g_ep_fifo[fifo_idx];

	ep_idx = fifo_config.ep_addr & 0x7f;
	is_in = fifo_config.ep_addr & USB_DIR_IN;

	/* select ep */
	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);

	log_udc_dbg("Handling %s ep%d...\n", is_in ? "tx" : "rx", ep_idx);

	if (is_in) {
		if (USB_DRV_Reg(&musb->txcsr) & USB_TXCSR_SENTSTALL) {
			log_udc_err("tx ep%d is stall\n", ep_idx);
			USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_SENTSTALL | USB_TXCSR_SENDSTALL);
			/* clear data toggle */
			USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_CLRDATATOG);
			goto end;
		}

		if (!(USB_DRV_Reg(&musb->txcsr) & USB_TXCSR_TXPKTRDY)) {
			if (!g_udc.epin[ep_idx - 1].pdata) {
				/* callback to user to fill tx buf */
				if (g_udc.callback && g_udc.epin[ep_idx - 1].data_len
				    && g_udc.epin[ep_idx - 1].ep_call_finish == 0) {
					g_udc.epin[ep_idx - 1].ep_call_finish = 1;
					g_udc.callback(g_udc.epin[ep_idx - 1].ep_addr,
						       UDC_EVENT_TX_COMPLETE, NULL,
						       g_udc.epin[ep_idx - 1].data_len);
				}
			} else {
				udc_write_fifo(&g_udc.epin[ep_idx - 1], true);
			}
		}

		if (USB_DRV_Reg(&musb->txcsr) & USB_TXCSR_UNDERRUN) {
			USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_UNDERRUN);
		}
	} else {
		if (USB_DRV_Reg(&musb->rxcsr) & USB_RXCSR_SENTSTALL) {
			log_udc_err("rx ep%d is stall\n", ep_idx);
			USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_SENTSTALL | USB_RXCSR_SENDSTALL);
			/* clear data toggle */
			USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_CLRDATATOG);

			goto end;
		}

		if (USB_DRV_Reg(&musb->rxcsr) & USB_RXCSR_RXPKTRDY) {
			udc_read_fifo(&g_udc.epout[ep_idx - 1], true, true);
		}

		if (USB_DRV_Reg(&musb->txcsr) & USB_RXCSR_OVERRUN) {
			USB_DRV_ClearBits(&musb->txcsr, USB_RXCSR_OVERRUN);
		}
	}

end:
	/* restore ep */
	usbc_select_active_ep(old_ep_idx);
}

static hal_irqreturn_t udc_irq_handler(void *dev_id)
{
	unsigned long flags = 0;
	uint32_t old_ep_idx = 0, i = 0;
	uint32_t usb_irq = 0, tx_irq = 0, rx_irq = 0;

	// krhino_spin_lock_irq_save(&g_udc.lock, flags);
	flags = hal_spin_lock_irqsave(&udc_lock);

	/* save index */
	old_ep_idx = USB_DRV_Reg8(&musb->index);

	/* read status registers */
	usb_irq = USB_DRV_Reg8(&musb->intrusb);
	tx_irq = USB_DRV_Reg(&musb->intrtx);
	rx_irq = USB_DRV_Reg(&musb->intrrx);

	usb_irq = usbc_filtrate_irq(usb_irq);

	hal_log_debug("usb_irq: %02x, tx_irq: %02x, rx_irq: %02x\n",
		      usb_irq, tx_irq, rx_irq);

	/* RESET */
	if (usb_irq & USB_INTRUSB_RESET) {
		log_udc_dbg("irq: Reset\r\n");

		/* clear irq pending */
		USB_DRV_WriteReg8(&musb->intrusb, USB_INTRUSB_RESET);

		/* select ep0 */
		usbc_select_active_ep(0);

		/* set default address: 0x0 */
		USB_DRV_WriteReg8(&musb->faddr, 0x00);

		g_udc.address = 0;
		g_udc.ep0state = UDC_EP0_IDLE;
		g_udc.speed = UDC_SPEED_UNKNOWN;

		goto end;
	}

	/* RESUME */
	if (usb_irq & USB_INTRUSB_RESUME) {
		log_udc_dbg("irq: Resume\r\n");

		/* clear irq pending */
		USB_DRV_WriteReg8(&musb->intrusb, USB_INTRUSB_RESUME);

		if (g_udc.speed != UDC_SPEED_UNKNOWN) {
			/* TODO: Resume work */
		}
	}

	/* SUSPEND */
	if (usb_irq & USB_INTRUSB_SUSPEND) {
		log_udc_dbg("irq: Suspend\r\n");

		/* clear irq pending */
		USB_DRV_WriteReg8(&musb->intrusb, USB_INTRUSB_SUSPEND);

		if (g_udc.speed != UDC_SPEED_UNKNOWN) {
			/* TODO: Suspend work */
		}

		g_udc.ep0state = UDC_EP0_IDLE;
	}

	/* EP0 control transfer */
	if (tx_irq & USB_INTRTX_EP0) {
		log_udc_dbg("irq: ep0\r\n");

		/* clear irq pending by setting it to a 1 */
		USB_DRV_WriteReg(&musb->intrtx, USB_INTRTX_EP0);

		if (g_udc.speed == UDC_SPEED_UNKNOWN) {
			if (USB_DRV_Reg8(&musb->power) & USB_POWER_HSMODE) {
				log_udc_dbg("usb enter High-speed mode\r\n");
				g_udc.speed = UDC_SPEED_HIGH;
			} else {
				log_udc_dbg("usb enter Full-speed mode\r\n");
				g_udc.speed = UDC_SPEED_FULL;
			}
		}

		udc_handle_ep0();
	}
	/* rx endpoint data transfers */
	for (i = 1; i <= SW_UDC_EPNUMS; i++) {
		if (rx_irq & (1 << i)) {
			log_udc_dbg("rx irq: ep%d\n", i);

			/* clear irq pending by setting it to a 1 */
			USB_DRV_WriteReg(&musb->intrrx, 0x1 << i);

			udc_handle_ep(g_ep_fifo_out[i]);
		}
	}

	/* tx endpoint data transfers */
	for (i = 1; i <= SW_UDC_EPNUMS; i++) {
		if (tx_irq & (1 << i)) {
			log_udc_dbg("tx irq: ep%d\n", i);
			/* clear irq pending by setting it to a 1 */
			USB_DRV_WriteReg(&musb->intrtx, 0x1 << i);
			udc_handle_ep(g_ep_fifo_in[i]);
		}
	}
#ifdef CONFIG_UDC_USE_DMA
	/* firstly to get data */
	uint32_t dma_irq;
	for (i = 1; i <= SW_UDC_EPNUMS; i++) {
		dma_irq = usb_dma_get_irq_status(g_udc.dma_rx_chan[i]);
		if (dma_irq) {
			uint32_t old_ep_idx = 0;
			old_ep_idx = usbc_get_active_ep();
			usbc_select_active_ep(i);
			usb_dma_irq_disable(g_udc.dma_rx_chan[i]);
			usb_dma_clr_irq_status(g_udc.dma_rx_chan[i]);
			usb_dma_stop(g_udc.dma_rx_chan[i]);
			udc_set_config_rx_dma(0, i);
			usbc_ep_intr_enable(i, 0);
			/* callback to user */
			USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_OVERRUN | USB_RXCSR_DATAERROR);
			if (g_udc.epout[i - 1].data_len == g_udc.epout[i - 1].data_actual) {
				// printf("%d D R d:%d a:%d %08x count:%d\n", __LINE__,
				//        g_udc.epout[i - 1].data_len, g_udc.epout[i - 1].data_actual,
				//        USB_DRV_Reg32(&musb->rxmap), USB_DRV_Reg32(&musb->rxcount));
				void *pdata = g_udc.epout[i - 1].pdata;
				uint32_t data_actual = g_udc.epout[i - 1].data_actual;
				hal_dcache_invalidate((unsigned long)pdata, data_actual);
				g_udc.epout[i - 1].pdata = NULL;
				g_udc.epout[i - 1].data_actual = 0;
				g_udc.epout[i - 1].data_len = 0;
				if (g_udc.callback && !g_udc.epout[i - 1].ep_call_finish) {
					g_udc.epout[i - 1].ep_call_finish = 1;
					g_udc.callback(g_udc.epout[i - 1].ep_addr, UDC_EVENT_RX_DATA, pdata, data_actual);
				}
			} else if (USB_DRV_Reg(&musb->rxcsr) & USB_RXCSR_RXPKTRDY) {
				udc_read_fifo(&g_udc.epout[i - 1], true, false);
			}
			usbc_select_active_ep(old_ep_idx);
		}
	}

	for (i = 1; i <= SW_UDC_EPNUMS; i++) {
		dma_irq = usb_dma_get_irq_status(g_udc.dma_tx_chan[i]);
		if (dma_irq) {
			uint32_t old_ep_idx = 0;
			old_ep_idx = usbc_get_active_ep();
			usbc_select_active_ep(i);
			usb_dma_irq_disable(g_udc.dma_tx_chan[i]);
			usb_dma_clr_irq_status(g_udc.dma_tx_chan[i]);
			while((USB_DRV_Reg(&musb->txcsr) & (USB_TXCSR_TXPKTRDY | USB_TXCSR_FIFONOTEMPTY)));
			usb_dma_stop(g_udc.dma_tx_chan[i]);
			udc_set_config_tx_dma(0, i);
			usbc_ep_intr_enable(i, 1);
			if (g_udc.epin[i - 1].pdata) {
				udc_write_fifo(&g_udc.epin[i - 1], false);
			} else {
				if (g_udc.callback && g_udc.epin[i - 1].ep_call_finish == 0) {
					g_udc.epin[i - 1].ep_call_finish = 1;
					g_udc.callback(g_udc.epin[i - 1].ep_addr, UDC_EVENT_TX_COMPLETE, NULL, g_udc.epin[i - 1].data_len);
				}
			}
			usbc_select_active_ep(old_ep_idx);
		}
	}
#endif
end:
	/* restore ep */
	usbc_select_active_ep(old_ep_idx);

	// krhino_spin_unlock_irq_restore(&g_udc.lock, flags);
	hal_spin_unlock_irqrestore(&udc_lock, flags);

	return HAL_IRQ_OK;
}

void udc_device_desc_init(void *device_desc)
{
	g_udc.device_desc = device_desc;
#ifdef CONFIG_UDC_USE_DMA
	{
		uint32_t i = 0;
		for(i = 0; i < USB_DMA_CHANN_NUM; i++){
			usb_dma_release(g_udc.dma_tx_chan[i]);
			g_udc.dma_tx_chan[i] = 0xff;
			g_udc.dma_rx_chan[i] = 0xff;
		}
	}
#endif
}

void udc_config_desc_init(void *config_desc, uint32_t len)
{
	g_udc.config_desc = config_desc;
	g_udc.config_desc_len = len;
}

void udc_string_desc_init(const void *string_desc)
{
	if (g_udc.string_desc_num >= UDC_MAX_NUM_STRING_DESC) {
		printf("str desc num not enough.\n");
		return;
	}
	g_udc.string_desc[g_udc.string_desc_num]
				= (struct usb_string_descriptor *)string_desc;

	g_udc.string_desc_num++;
}

void udc_register_callback(udc_callback_t user_callback)
{
	g_udc.callback = user_callback;
}

static void ep_info_init(void)
{
	uint32_t i;

	/* ep0 init */
	g_udc.ep0.ep_addr = 0;
	g_udc.ep0.maxpacket = UDC_MAX_PACKET_SIZE_EP0;

	/* epin init */
	for (i = 0; i < UDC_MAX_NUM_EP_TX; i++) {
		g_udc.epin[i].ep_addr = i + 1;
		g_udc.epin[i].maxpacket = UDC_MAX_PACKET_SIZE_EP_BULK;
	}

	/* epout init */
	for (i = 0; i < UDC_MAX_NUM_EP_RX; i++) {
		g_udc.epout[i].ep_addr = i + 1;
		g_udc.epout[i].maxpacket = UDC_MAX_PACKET_SIZE_EP_BULK;
	}
}

static void udc_set_halt_ex(uint8_t ep_addr, int value)
{
	uint8_t ep_idx;
	uint8_t is_in;
	uint8_t old_ep_idx;

	ep_idx = ep_addr & 0x7f;
	is_in = ep_addr & USB_DIR_IN;
	/* select ep */
	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);

	if (ep_idx == 0) {
		USB_DRV_ClearBits(&musb->txcsr, USB_CSR0_SENDSTALL);
		USB_DRV_ClearBits(&musb->txcsr, USB_CSR0_SENTSTALL);
	} else {
		if (is_in) {
			if (value) {
				USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_SENDSTALL);
			} else {
				USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_SENTSTALL);
				USB_DRV_ClearBits(&musb->txcsr, USB_TXCSR_SENDSTALL);
				USB_DRV_SetBits(&musb->txcsr, USB_TXCSR_CLRDATATOG);
			}
		} else {
			if (value) {
				USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_SENDSTALL);
			} else {
				USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_SENDSTALL);
				USB_DRV_ClearBits(&musb->rxcsr, USB_RXCSR_SENTSTALL);
				USB_DRV_SetBits(&musb->rxcsr, USB_RXCSR_CLRDATATOG);
			}
		}
	}
	usbc_select_active_ep(old_ep_idx);

	return;
}

void udc_ep_enable(uint8_t ep_addr, uint16_t maxpacket, uint32_t ts_type)
{
	uint8_t fifo_idx;
	uint8_t ep_idx;
	uint8_t is_in;
	udc_fifo_t fifo_config;
	uint8_t old_ep_idx;
	uint32_t reg_val;
	uint32_t flags;
	uint32_t fifo_size;

	ep_idx = ep_addr & 0x7f;
	is_in = ep_addr & USB_DIR_IN;

	/* maybe use in irq, can't spinlock */
	/*krhino_spin_lock_irq_save(&g_udc.lock, flags);*/

	/* select ep */
	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);

	if (is_in) {
		fifo_idx = g_ep_fifo_in[ep_idx];
	} else {
		fifo_idx = g_ep_fifo_out[ep_idx];
	}

	fifo_config = g_ep_fifo[fifo_idx];

	log_udc_dbg("ep_addr: 0x%x, maxpacket: %d, ts_type: %d\n",
		    ep_addr, maxpacket, ts_type);
	log_udc_dbg("fifo_idx: %d, fifo.ep_addr: 0x%x, fifo.addr: %d, fifo.size: %d, fifo.dbf: %d\n",
		    fifo_idx, fifo_config.ep_addr, fifo_config.fifo_addr,
		    fifo_config.fifo_size, fifo_config.double_fifo);
	fifo_size = fifo_config.fifo_size;
	if (is_in) {
		g_udc.epin[ep_idx - 1].maxpacket = maxpacket;

		if ((g_udc.epin[ep_idx - 1].maxpacket & 0x7ff) > fifo_config.fifo_size) {
			log_udc_err("err: usbd_ep_enable, fifo size is too small\n");
		} else if (fifo_config.double_fifo) {
			if (((g_udc.epout[ep_idx - 1].maxpacket & 0x7ff) * 2) > fifo_config.fifo_size) {
				log_udc_err("err: usbd_ep_enable, it is double fifo, but fifo size is too small\n");
				g_udc.epin[ep_idx - 1].maxpacket = fifo_config.fifo_size / 2;
			}
			fifo_size = g_udc.epin[ep_idx - 1].maxpacket & 0x7ff;
			g_udc.epin[ep_idx - 1].is_double_fifo = 1;
		}
	} else {
		g_udc.epout[ep_idx - 1].maxpacket = maxpacket;
		if ((g_udc.epout[ep_idx - 1].maxpacket & 0x7ff) > fifo_config.fifo_size) {
			log_udc_err("err: usbd_ep_enable, fifo size is too small\n");
		} else if (fifo_config.double_fifo) {
			if (((g_udc.epout[ep_idx - 1].maxpacket & 0x7ff) * 2)
			    > fifo_config.fifo_size) {
				log_udc_err("err: usbd_ep_enable, it is double fifo, but fifo size is too small\n");
				g_udc.epout[ep_idx - 1].maxpacket = fifo_config.fifo_size / 2;
			}
			fifo_size = g_udc.epout[ep_idx - 1].maxpacket & 0x7ff;
			g_udc.epout[ep_idx - 1].is_double_fifo = 1;
		}
	}

	/* TODO: check validity */
	usbc_ep_config_default(is_in);
	/* set max packet, type, direction, address;
	 * reset fifo counters, enable irq */
	usbc_ep_config(ts_type, maxpacket, fifo_config.double_fifo, is_in);
	usbc_ep_fifo_config(fifo_config.fifo_addr, fifo_size, fifo_config.double_fifo, is_in);
	if (ts_type == USB_ENDPOINT_XFER_ISOC) {
		USB_DRV_SetBits8(&musb->power, USB_POWER_ISOUPDATE);
	}

	/* enable ep interrupt */
	usbc_ep_intr_enable(ep_idx, is_in);
#ifdef CONFIG_UDC_USE_DMA
	hal_udc_ep_set_dma_enable(ep_addr, 1);
	usb_ep_dma_init(ep_addr);
#endif
	/* restore ep */
	usbc_select_active_ep(old_ep_idx);

	/*krhino_spin_unlock_irq_restore(&g_udc.lock, flags);*/
}

void udc_ep_disable(uint8_t ep_addr)
{
	/* TODO */
	uint8_t ep_idx;
	uint8_t is_in;
	uint8_t old_ep_idx;
	ep_idx = ep_addr & 0x7f;
	is_in = ep_addr & USB_DIR_IN;

	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);

	usbc_ep_intr_disable(ep_idx,is_in);
	usbc_ep_config_default(is_in);

	usbc_select_active_ep(old_ep_idx);

#ifdef CONFIG_UDC_USE_DMA
	hal_udc_ep_set_dma_enable(ep_addr, 0);
#endif
}

int32_t udc_ep_read(uint8_t ep_addr, void *buf, uint32_t len)
{
	uint8_t ep_idx;
	uint8_t is_in;
	udc_ep_t *ep;
	uint8_t old_ep_idx;
	int32_t ret = UDC_ERRNO_RX_NOT_READY;
	uint32_t flags;

	ep_idx = ep_addr & 0x7f;
	is_in = ep_addr & USB_DIR_IN;

	flags = hal_spin_lock_irqsave(&udc_lock);
	/* select ep */
	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);

	if (ep_idx == 0) {
		ep = &g_udc.ep0;
	} else if (!is_in) {
		ep = &g_udc.epout[ep_idx - 1];
	} else {
		ret = UDC_ERRNO_EP_INVALID;
		goto end;
	}

	ep->pdata = buf;
	ep->data_len = len;
	ep->data_actual = 0;
	ep->ep_call_finish = 0;
	if ((USB_DRV_Reg(&musb->rxcsr) & USB_RXCSR_RXPKTRDY)
	    || ((USB_DRV_Reg32(&musb->rxcount) & 0x00000fff) != 0)) {
		ret = udc_read_fifo(ep, true, false);
	}

end:
	/* restore ep */
	usbc_select_active_ep(old_ep_idx);

	hal_spin_unlock_irqrestore(&udc_lock, flags);

	return ret;
}

int32_t udc_ep_write(uint8_t ep_addr, void *buf, uint32_t len)
{
	uint8_t ep_idx;
	uint8_t is_in;
	udc_ep_t *ep;
	uint8_t old_ep_idx;
	int32_t ret = UDC_ERRNO_TX_BUSY;
	uint32_t flags;

	ep_idx = ep_addr & 0x7f;
	is_in = ep_addr & USB_DIR_IN;

	//hal_mutex_lock(udc_hal_lock);
	flags = hal_spin_lock_irqsave(&udc_lock);
	/* select ep */
	old_ep_idx = usbc_get_active_ep();
	usbc_select_active_ep(ep_idx);

	if (ep_idx == 0) {
		ep = &g_udc.ep0;
	} else if (is_in) {
		ep = &g_udc.epin[ep_idx - 1];
	} else {
		ret = UDC_ERRNO_EP_INVALID;
		goto end;
	}

	ep->pdata = buf;
	ep->data_len = len;
	ep->data_actual = 0;

	if (ep_idx == 0) {
		if ((USB_DRV_Reg(&musb->txcsr) & USB_CSR0_TXPKTRDY)) {
			goto end;
		}
	} else {
		ep->ep_call_finish = 0;
		if ((USB_DRV_Reg(&musb->txcsr) & USB_TXCSR_TXPKTRDY)) {
			goto end;
		}
	}
	ret = udc_write_fifo(ep, false);

end:
	/* restore ep */
	usbc_select_active_ep(old_ep_idx);
	hal_spin_unlock_irqrestore(&udc_lock, flags);
	return ret;
}

void USBC_EnterMode_Test_J(void)
{
	USB_DRV_SetBits8(&musb->testmode, USB_TESTMODE_TESTSE0NAK);
}

void USBC_EnterMode_Test_K(void)
{
	USB_DRV_SetBits8(&musb->testmode, USB_TESTMODE_TESTJ);
}

void USBC_EnterMode_Test_SE0_NAK(void)
{
	USB_DRV_SetBits8(&musb->testmode, USB_TESTMODE_TESTK);
}

void USBC_EnterMode_TestPacket(void)
{
	USB_DRV_SetBits8(&musb->testmode, USB_TESTMODE_TESTPACKET);
}

void USBC_EnterMode_Idle(void)
{
	USB_DRV_ClearBits8(&musb->testmode, USB_TESTMODE_TESTSE0NAK);
	USB_DRV_ClearBits8(&musb->testmode, USB_TESTMODE_TESTJ);
	USB_DRV_ClearBits8(&musb->testmode, USB_TESTMODE_TESTK);
	USB_DRV_ClearBits8(&musb->testmode, USB_TESTMODE_TESTPACKET);
}

static const unsigned char TestPkt[54] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xEE, 0xEE, 0xEE,
	0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF,
	0xEF, 0xF7, 0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7,
	0xFB, 0xFD, 0x7E, 0x00
};

void udc_ed_test(const char *buf, size_t count)
{
	hal_log_info("otg0_ed_test : %s", buf);
	if (!strncmp(buf, "test_j_state", 12)) {
		USBC_EnterMode_Test_J();
		hal_log_info("test_mode:%s\n", "test_j_state");
	} else if (!strncmp(buf, "test_k_state", 12)) {
		USBC_EnterMode_Test_K();
		hal_log_info("test_mode:%s\n", "test_k_state");
	} else if (!strncmp(buf, "test_se0_nak", 12)) {
		USBC_EnterMode_Test_SE0_NAK();
		hal_log_info("test_mode:%s\n", "test_se0_nak");
	} else if (!strncmp(buf, "test_pack", 9)) {
		void *fifo;
		hal_log_info("test_mode___:%s\n", "test_pack");
		USB_DRV_SetBits32(&musb->power, (0x1 << 0));
		hal_sleep(1);
		fifo = usbc_select_fifo(0);
		usbc_write_packet(fifo, 54, (u32 *)TestPkt);
		USB_DRV_SetBits(&musb->txcsr, (0x1 << 1));
		USBC_EnterMode_TestPacket();
	} else if (!strncmp(buf, "disable_test_mode", 17)) {
		hal_log_info("start disable_test_mode\n");
		USBC_EnterMode_Idle();
	} else {
		hal_log_err("ERR: test_mode Argment is invalid\n");
	}
}

void udc_ep_set_buf(uint8_t ep_addr, void *buf, uint32_t len)
{
	uint8_t ep_idx;
	uint8_t is_in;
	uint32_t flags;

	ep_idx = ep_addr & 0x7f;
	is_in = ep_addr & USB_DIR_IN;

//	hal_mutex_lock(udc_hal_lock);
	flags = hal_spin_lock_irqsave(&udc_lock);

	if (ep_idx == 0) {
		g_udc.ep0.pdata = buf;
		g_udc.ep0.data_len = len;
		g_udc.ep0.data_actual = 0;
		if (is_in)
			g_udc.ep0state = UDC_EP0_IN_DATA_PHASE;
		else
			g_udc.ep0state = UDC_EP0_OUT_DATA_PHASE;
	} else {
		if (is_in) {
			g_udc.epin[ep_idx - 1].pdata = buf;
			g_udc.epin[ep_idx - 1].data_len = len;
		} else {
			g_udc.epout[ep_idx - 1].pdata = buf;
			g_udc.epout[ep_idx - 1].data_len = len;
		}
	}
	hal_spin_unlock_irqrestore(&udc_lock, flags);
//	hal_mutex_unlock(udc_hal_lock);


}

static int32_t udc_init(void)
{
	/* NOTICE: clock and regulator must be ready first */

	ep_info_init();

	/* udc bsp init */
	usbc_enable_dpdm_pullup(true);
	usbc_enable_id_pullup(true);
	usbc_force_id(USB_ISCR_FORCE_ID_HIGH);
	usbc_force_vbus_valid(USB_ISCR_FORCE_VBUS_HIGH);
	usbc_select_bus(UDC_IO_TYPE_PIO, UDC_EP_TYPE_EP0, 0);
	usbc_phy_set_ctl(true);
	usbc_phy_otg_sel(true);

	usb_phy_init((unsigned long)musb_phy, 0);

	/* disable udc before request irq */
	usbc_udc_disable();

	/* request irq */
	if (hal_request_irq(g_sunxi_udc.irq_no, udc_irq_handler, "usb_udc", NULL) < 0) {
		log_udc_err("request irq error\n");
		return -1;
	}

	hal_enable_irq(g_sunxi_udc.irq_no);

	/* udc enable */
	usbc_udc_enable();

	return 0;
}

static int32_t udc_deinit(void)
{
	struct platform_usb_config *otg_table = platform_get_otg_table();

	/*free irq*/
	hal_free_irq(otg_table->irq);

	/* udc bsp deinit */
	usbc_enable_dpdm_pullup(false);
	usbc_enable_id_pullup(false);
	usbc_force_id(USB_ISCR_FORCE_ID_DISABLED);
	usbc_force_vbus_valid(USB_ISCR_FORCE_VBUS_DISABLED);

	memset(&g_udc, 0, sizeof(udc_priv_t));
	return 0;
}

static int32_t udc_open_clk(sunxi_udc_io_t sunxi_udc)
{
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_status_t ret;

	/*open otg clk*/
	sunxi_udc.reset_phy = hal_reset_control_get(reset_type, sunxi_udc.reset_phy_clk);
	ret = hal_reset_control_deassert(sunxi_udc.reset_phy);
	if (ret) {
		hal_log_err("reset phy err!\n");
		return -1;
	}
	hal_reset_control_put(sunxi_udc.reset_phy);

	sunxi_udc.reset_otg = hal_reset_control_get(reset_type, sunxi_udc.reset_otg_clk);
	ret = hal_reset_control_deassert(sunxi_udc.reset_otg);
	if (ret) {
		hal_log_err("reset otg err!\n");
		return -1;
	}
	hal_reset_control_put(sunxi_udc.reset_otg);

	/*open udc clk*/
	sunxi_udc.phy_clk = hal_clock_get(clk_type, sunxi_udc.phy_clk_id);
	ret = hal_clock_enable(sunxi_udc.phy_clk);
	if (ret) {
		hal_log_err("couldn't enable usb_phy_clk!\n");
		return -1;
	}

	sunxi_udc.otg_clk = hal_clock_get(clk_type, sunxi_udc.otg_clk_id);
	ret = hal_clock_enable(sunxi_udc.otg_clk);
	if (ret) {
		hal_log_err("couldn't enable otg_clk!\n");
		return -1;
	}

	return 0;
}

static int32_t udc_close_clk(sunxi_udc_io_t sunxi_udc)
{
	//	int ret;
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_status_t ret;

	sunxi_udc.phy_clk = hal_clock_get(clk_type, sunxi_udc.phy_clk_id);
	ret = hal_clock_disable(sunxi_udc.phy_clk);
	if (ret) {
		hal_log_err("couldn't disable usb_phy_clk!\n");
		return -1;
	}

	sunxi_udc.otg_clk = hal_clock_get(clk_type, sunxi_udc.otg_clk_id);
	ret = hal_clock_disable(sunxi_udc.otg_clk);
	if (ret) {
		hal_log_err("couldn't disable otg_clk!\n");
		return -1;
	}

	sunxi_udc.reset_otg = hal_reset_control_get(reset_type, sunxi_udc.reset_otg_clk);
	ret = hal_reset_control_assert(sunxi_udc.reset_otg);
	if (ret) {
		hal_log_err("reset otg err!\n");
		return -1;
	}
	hal_reset_control_put(sunxi_udc.reset_otg);

	sunxi_udc.reset_phy = hal_reset_control_get(reset_type, sunxi_udc.reset_phy_clk);
	ret = hal_reset_control_assert(sunxi_udc.reset_phy);
	if (ret) {
		hal_log_err("reset phy err!\n");
		return -1;
	}
	hal_reset_control_put(sunxi_udc.reset_phy);

	return 0;
}

static void udc_get_config_param(void)
{
#ifndef CONFIG_KERNEL_FREERTOS
	int ret = -1;
	char udc_name[10] = {0};

	sprintf(udc_name, "usbc0");
	ret = hal_cfg_get_keyvalue(udc_name, KEY_UDC_IRQ_FLAG, (int32_t *)&g_sunxi_udc.irq_flag, 1);
	if (ret) {
		hal_log_err("%s %s fetch error!", udc_name, KEY_UDC_IRQ_FLAG);
		g_sunxi_udc.irq_flag = 0x0;
	}

	ret = hal_cfg_get_keyvalue(udc_name, KEY_UDC_DRIVER_LEVEL,
				  (int32_t *)&g_sunxi_udc.drive_level, 1);
	if (ret) {
		hal_log_err("%s %s fetch error!", udc_name, KEY_UDC_DRIVER_LEVEL);
		g_sunxi_udc.drive_level = 0x8;
	}
	if (g_sunxi_udc.drive_level > 0xf) {
		g_sunxi_udc.drive_level = 0x8;
	}
#else
	g_sunxi_udc.irq_flag = 0;
	g_sunxi_udc.drive_level = 0x8;
#endif
}

#ifdef CONFIG_STANDBY
int udc_suspend(void *timeout)
{
	return 0;
}

int udc_resume(void *timeout)
{
	USB_DRV_ClearBits32(&musb_phy->iscr, USB_ISCR_FORCE_VBUS_MASK);
	USB_DRV_SetBits32(&musb_phy->iscr, USB_ISCR_FORCE_VBUS_HIGH);
	return 0;
}
#elif defined(CONFIG_COMPONENTS_PM)
static int udc_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	pm_inf("[%s] suspend start, mode:%d\n", __func__, mode);
	switch (mode)
	{
		case PM_MODE_SLEEP:
		case PM_MODE_STANDBY:
		case PM_MODE_HIBERNATION:
			break;
		default:
			break;
	}

	/* disable usb controller */
	usbc_udc_disable();

	/* set speed to unknown */
	g_udc.speed = UDC_SPEED_UNKNOWN;

	/* close udc clock */
	udc_close_clk(g_sunxi_udc);

	/* disable phy */
	usbc_phy_set_ctl(false);

	pm_inf("[%s] suspend end\n", __func__);

	return 0;
}

static int udc_resume(struct pm_device *dev, suspend_mode_t mode)
{
	uint32_t ret;
	pm_inf("[%s] resume start, mode:%d\n", __func__, mode);

	/* enable phy */
	usbc_phy_set_ctl(true);

	/* open udc clock */
	ret = udc_open_clk(g_sunxi_udc);
	if (ret)
	{
		log_udc_err("open udc clk failed\n");
		return -1;
	}

	/* udc bsp init */
	usbc_enable_dpdm_pullup(true);
	usbc_enable_id_pullup(true);
	usbc_force_id(USB_ISCR_FORCE_ID_HIGH);
	usbc_force_vbus_valid(USB_ISCR_FORCE_VBUS_HIGH);
	usbc_select_bus(UDC_IO_TYPE_PIO, UDC_EP_TYPE_EP0, 0);
	usbc_phy_set_ctl(true);
	usbc_phy_otg_sel(true);

	usb_phy_init((unsigned long)musb_phy, 0);

	/* udc enable */
	usbc_udc_enable();

	pm_inf("[%s] suspend end\n", __func__);
	return 0;
}

static struct pm_devops udc_devops = {
	.suspend = udc_suspend,
	.resume = udc_resume,
};

static struct pm_device udc_dev = {
	.name = "udc",
	.ops = &udc_devops,
};
#endif

int32_t sunxi_udc_init(void)
{
	uint32_t ret;

	struct platform_usb_config *otg_config = platform_get_otg_table();

	g_sunxi_udc.otg_clk_id		= otg_config->usb_clk;
	g_sunxi_udc.reset_otg_clk	= otg_config->usb_rst;
	g_sunxi_udc.phy_clk_id		= otg_config->phy_clk;
	g_sunxi_udc.reset_phy_clk	= otg_config->phy_rst;

	g_sunxi_udc.irq_no = otg_config->irq;
	/* request gpio */
	// hal_pinmux_set_function();
	ret = udc_open_clk(g_sunxi_udc);
	if (ret) {
		log_udc_err("open udc clk failed\n");
		return -1;
	}
	udc_get_config_param();
	udc_init();
#ifdef CONFIG_STANDBY
	register_pm_dev_notify(udc_suspend, udc_resume, NULL);
#elif defined(CONFIG_COMPONENTS_PM)
	pm_devops_register(&udc_dev);
#endif

	return 0;
}

int32_t sunxi_udc_deinit(void)
{
	udc_deinit();

	udc_close_clk(g_sunxi_udc);
#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&udc_dev);
#endif
	return 0;
}
