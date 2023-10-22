#include "udc.h"
#include "usb_phy.h"
#include "udc_platform.h"

void hal_udc_device_desc_init(void *device_desc)
{
	udc_device_desc_init(device_desc);
}

void hal_udc_config_desc_init(void *config_desc, uint32_t len)
{
	udc_config_desc_init(config_desc, len);
}

void hal_udc_string_desc_init(const void *string_desc)
{
	udc_string_desc_init(string_desc);
}

void hal_udc_register_callback(udc_callback_t user_callback)
{
	udc_register_callback(user_callback);
}

void hal_udc_ep_enable(uint8_t ep_addr, uint16_t maxpacket, uint32_t ts_type)
{
	udc_ep_enable(ep_addr, maxpacket, ts_type);
}

void hal_udc_ep_disable(uint8_t ep_addr)
{
	udc_ep_disable(ep_addr);
}

int32_t hal_udc_ep_read(uint8_t ep_addr, void *buf, uint32_t len)
{
	return udc_ep_read(ep_addr, buf, len);
}

int32_t hal_udc_ep_write(uint8_t ep_addr, void *buf, uint32_t len)
{
	return udc_ep_write(ep_addr, buf, len);
}
void hal_udc_ed_test(const char *buf, size_t count)
{
	udc_ed_test(buf, count);
}

void hal_udc_phy_range_show(int usbc_num)
{
	USBPHY_REGISTER_T *musb_phy = usbc_get_musb_phy();
	usb_phy_range_show((unsigned long)musb_phy);
}

void hal_udc_phy_range_set(int usbc_num, int val)
{
	USBPHY_REGISTER_T *musb_phy = usbc_get_musb_phy();
	usb_phy_range_set((unsigned long)musb_phy, val);
}

void hal_udc_ep_set_buf(uint8_t ep_addr, void *buf, uint32_t len)
{
	udc_ep_set_buf(ep_addr, buf, len);
}

int32_t hal_udc_init(void)
{
	return sunxi_udc_init();
}

int32_t hal_udc_deinit(void)
{
	return sunxi_udc_deinit();
}
