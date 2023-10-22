#include "usb_os_platform.h"
#include "udc_platform.h"
#include "usb_dma.h"

struct usb_dma_manage usb_dma_ch_info[USB_DMA_CHANN_NUM];

int32_t usb_dma_request(void)
{
	uint32_t i;

	for (i = 0; i < USB_DMA_CHANN_NUM; i++) {
		if (!usb_dma_ch_info[i].used) {
			break;
		}
	}

	if (i == USB_DMA_CHANN_NUM) {
		printf("No Valid D DMA Channel to Request!\n");
		return (-1);
	}

	usb_dma_ch_info[i].chan = i;
	usb_dma_ch_info[i].used = 1;
	return i;
}

int32_t usb_dma_setup(uint32_t chan, uint32_t config)
{
	if (chan != usb_dma_ch_info[chan].chan) {
		printf("Wrong Match DMA Channel!\n");
		return (-1);
	}
	usb_dma_ch_info[chan].config = config;
	return 0;
}

void usb_dma_irq_enable(uint32_t chan)
{
	uint32_t reg_value = 0;
	reg_value = USB_DRV_Reg32(SUNXI_USB_OTG_PBASE + 0x500) | (0x01 << chan);
	USB_DRV_WriteReg32(SUNXI_USB_OTG_PBASE + 0x500, reg_value);
}

void usb_dma_irq_disable(uint32_t chan)
{
	uint32_t reg_value = 0;
	reg_value = USB_DRV_Reg32(SUNXI_USB_OTG_PBASE + 0x500);
	reg_value &= ~(0x01 << chan);
	USB_DRV_WriteReg32(SUNXI_USB_OTG_PBASE + 0x500, reg_value);
}

uint32_t usb_dma_get_irq_status(uint32_t chan)
{
	uint32_t reg_value = 0;
	reg_value = USB_DRV_Reg32(SUNXI_USB_OTG_PBASE + 0x504);
	return reg_value & (0x01 << chan);
}

void usb_dma_clr_irq_status(uint32_t chan)
{
	uint32_t reg_val;

	reg_val = USB_DRV_Reg32(SUNXI_USB_OTG_PBASE + 0x504);
	reg_val |= (0x01 << chan);
	USB_DRV_WriteReg32(SUNXI_USB_OTG_PBASE + 0x504, reg_val);
}

int32_t usb_dma_start(uint32_t chan, uint32_t sdram_add, uint32_t len)
{
	USB_DRV_WriteReg32((SUNXI_USB_OTG_PBASE + 0x544 + chan * 0x10), sdram_add);
	USB_DRV_WriteReg32((SUNXI_USB_OTG_PBASE + 0x548 + chan * 0x10), len);  // bc
	USB_DRV_WriteReg32((SUNXI_USB_OTG_PBASE + 0x540 + chan * 0x10),
			   usb_dma_ch_info[chan].config | ((uint32_t)0x01 << 31));

	return 0;
}

uint32_t usb_dma_finish(uint32_t chan)
{
	return (USB_DRV_Reg32(SUNXI_USB_OTG_PBASE + 0x540 + chan * 0x10) & (0x01 << 31));
}

void usb_dma_stop(uint32_t chan)
{
	// USB_DRV_WriteReg32(OTG_BASE+0x540+chan*0x10,
	// USB_DRV_Reg32(OTG_BASE+0x540+chan*0x10)|(0x01<<31));
	uint32_t reg_val;

	reg_val = USB_DRV_Reg32(SUNXI_USB_OTG_PBASE + 0x540 + chan * 0x10);
	reg_val &= ~((uint32_t)0x01 << 31);
	USB_DRV_WriteReg32(SUNXI_USB_OTG_PBASE + 0x540 + chan * 0x10, reg_val);
}

int32_t usb_dma_release(uint32_t chan)
{
	usb_dma_stop(chan);
	usb_dma_irq_disable(chan);
	usb_dma_ch_info[chan].chan = 0;
	usb_dma_ch_info[chan].used = 0;
	usb_dma_ch_info[chan].config = 0;
	usb_dma_ch_info[chan].sdram_add = 0;
	return 0;
}

void init_usb_dma()
{
	memset(usb_dma_ch_info, 0, sizeof(usb_dma_ch_info));
}

struct usb_dma_manage *usb_dma_get_ch_info(uint32_t chan)
{
	return &usb_dma_ch_info[chan];
}
