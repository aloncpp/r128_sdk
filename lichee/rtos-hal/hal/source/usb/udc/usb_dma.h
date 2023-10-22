#ifndef _USB_DMA_H
#define _USB_DMA_H

#define DMA_OTG_TYPE 0

#define USB_DMA_CHANN_NUM 8

#define usb_get_dma_tx_config(dma_burst, ep_no) \
	(((dma_burst & 0x7ff) << 16) | (0x00 << 4) | (ep_no & 0x0f))

#define usb_get_dma_rx_config(dma_burst, ep_no) \
	(((dma_burst & 0x7ff) << 16) | (0x01 << 4) | (ep_no & 0x0f))

struct usb_dma_manage {
	uint32_t chan;
	uint32_t used;
	uint32_t config;
	uint32_t sdram_add;
};

void init_usb_dma(void);

int32_t usb_dma_setup(uint32_t chan, uint32_t config);
int32_t usb_dma_start(uint32_t chan, uint32_t sdram_add, uint32_t len);
void usb_dma_stop(uint32_t chan);

int32_t usb_dma_request(void);
uint32_t usb_dma_finish(uint32_t chan);
int32_t usb_dma_release(uint32_t chan);

void usb_dma_irq_enable(uint32_t chan);
void usb_dma_irq_disable(uint32_t chan);

void usb_dma_clr_irq_status(uint32_t chan);
uint32_t usb_dma_get_irq_status(uint32_t chan);
struct usb_dma_manage *usb_dma_get_ch_info(uint32_t chan);

#endif
