#include <stdio.h>
#include <string.h>

#include "sensor_helper.h"

twi_status_t sensor_twi_init(twi_port_t port, unsigned char sccb_id)
{
	twi_status_t ret = 0;
	ret = hal_twi_init(port);

	hal_twi_control(port, I2C_SLAVE, &sccb_id);

	if(ret != TWI_STATUS_OK)
	{
	       csi_err("init i2c err ret = %d\n", ret);
	}
	csi_dbg("%s line: %d sccb_id = 0x%x", __func__, __LINE__, sccb_id);
	return TWI_STATUS_OK;
}

twi_status_t sensor_twi_exit(twi_port_t port)
{
	twi_status_t ret = 0;

	ret = hal_twi_uninit(port);
	if (ret != TWI_STATUS_OK) {
		csi_err("deinit i2c err ret=%d.\n", ret);
		return ret;
	}
	csi_dbg("%s line: %d", __func__, __LINE__);

	return TWI_STATUS_OK;
}

static int cci_write_a8_d8(sensor_private *sensor_priv, unsigned char addr,
		    unsigned char value)
{
	twi_msg_t msg;
	twi_status_t ret;
	uint8_t buf[2];

	buf[0] = addr;
	buf[1] = value;
	msg.flags = 0;
	msg.addr =  sensor_priv->slave_addr;
	msg.len = 2;
	msg.buf = buf;

	ret = hal_twi_control(sensor_priv->twi_port, I2C_RDWR, &msg);
	if (ret != TWI_STATUS_OK) {
		csi_err("[%s:%d]\n", __func__, __LINE__);
		csi_err("write_a8_d8 error = %d [REG-0x%02x]\n", ret, addr);
		return ret;
	}

	return TWI_STATUS_OK;
}

static int cci_read_a8_d8(sensor_private *sensor_priv, unsigned char addr,
		    unsigned char *value)
{
	twi_status_t ret;

	ret = hal_twi_read(sensor_priv->twi_port, addr, value, 1);
	if (ret != TWI_STATUS_OK) {
		csi_err("read_a8_d8 error = %d [REG-0x%02x]\n", ret, addr);
		return ret;
	}

	return TWI_STATUS_OK;
}

static int cci_write_a16_d8(sensor_private *sensor_priv, unsigned short addr,
		    unsigned char value)
{
	twi_msg_t msg;
	twi_status_t ret;
	uint8_t buf[3];

	buf[0] = (addr >> 8) & 0xff;
	buf[1] = addr & 0xff;
	buf[2] = value;
	msg.flags = 0;
	msg.addr =  sensor_priv->slave_addr;
	msg.len = 3;
	msg.buf = buf;

	ret = hal_twi_control(sensor_priv->twi_port, I2C_RDWR, &msg);
	if (ret != TWI_STATUS_OK) {
		csi_err("[%s:%d]\n", __func__, __LINE__);
		csi_err("write_a16_d8 error = %d [REG-0x%02x]\n", ret, addr);
		return ret;
	}

	return TWI_STATUS_OK;
}

static int cci_read_a16_d8(sensor_private *sensor_priv, unsigned short addr,
		    unsigned char *value)
{
	twi_msg_t msg[2];
	twi_status_t ret;
	uint8_t buf[3];
	uint8_t num = 2;

	return -4;

	buf[0] = (addr >> 8) & 0xff;
	buf[1] = addr & 0xff;
	buf[2] = 0xee;

	msg[0].addr = sensor_priv->slave_addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = &buf[0];

	msg[1].addr = sensor_priv->slave_addr;
	msg[1].flags = TWI_M_RD;
	msg[1].len = 1;
	msg[1].buf = &buf[2];

	//hal_twi_xfer not support for other modules
	//ret = hal_twi_xfer(sensor_priv->twi_port, msg, num);
	if (ret != TWI_STATUS_OK) {
		csi_err("read_a8_d8 error = %d [REG-0x%02x]\n", ret, addr);
		return ret;
	} else {
		*value = buf[2];
	}

	return TWI_STATUS_OK;
}

static int cci_write(sensor_private *sensor_priv, unsigned short addr, unsigned short value)
{
	int ret;

	if (sensor_priv->addr_width == TWI_BITS_16 && sensor_priv->data_width == TWI_BITS_8)
		ret = cci_write_a16_d8(sensor_priv, addr, value);
	else if (sensor_priv->addr_width == TWI_BITS_8 && sensor_priv->data_width == TWI_BITS_8)
		ret = cci_write_a8_d8(sensor_priv, addr, value);
	else {
		csi_err("%s:Now not support addr_width/%d and data_width/%d\n",
				__func__, sensor_priv->addr_width, sensor_priv->data_width);
		ret = -1;
	}

	return ret;
}

static int cci_read(sensor_private *sensor_priv, unsigned short addr, unsigned short *value)
{
	int ret;
	*value = 0;

	if (sensor_priv->addr_width == TWI_BITS_16 && sensor_priv->data_width == TWI_BITS_8)
		ret = cci_read_a16_d8(sensor_priv, addr, (unsigned char *)value);
	else if (sensor_priv->addr_width == TWI_BITS_8 && sensor_priv->data_width == TWI_BITS_8)
		ret = cci_read_a8_d8(sensor_priv, addr, (unsigned char *)value);
	else {
		csi_err("%s:Now not support addr_width/%d and data_width/%d\n",
				__func__, sensor_priv->addr_width, sensor_priv->data_width);
		ret = -1;
	}

	return ret;
}

int sensor_read(sensor_private *sensor_priv, unsigned short reg, unsigned short *value)
{
	twi_status_t ret = 0;
	int cnt = 0;

	ret = cci_read(sensor_priv, reg, value);
	while ((ret != TWI_STATUS_OK) && (cnt < 2)) {
		ret = cci_read(sensor_priv, reg, value);
		cnt++;
	}
	if (cnt > 0)
		csi_err("%s sensor read retry = %d\n", sensor_priv->name, cnt);

	return ret;
}

int sensor_write(sensor_private *sensor_priv, unsigned short reg, unsigned short value)
{
	twi_status_t ret = 0;
	int cnt = 0;

	ret = cci_write(sensor_priv, reg, value);
	while ((ret != TWI_STATUS_OK) && (cnt < 2)) {
		ret = cci_write(sensor_priv, reg, value);
		cnt++;
	}
	if (cnt > 0)
		csi_err("%s sensor write retry = %d\n", sensor_priv->name, cnt);

	return ret;
}

int sensor_write_array(sensor_private *sensor_priv, struct regval_list *regs, int array_size)
{
	twi_status_t ret = 0;
	int i = 0;
	//u32 rval = 0;

	csi_dbg("regs array size is %d\n", array_size);

	if (!regs)
		return -1;

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			hal_msleep(regs->data);
		} else {
			hal_msleep(2);  // for stable, if not, twi will fail probabilistic
			ret = sensor_write(sensor_priv, regs->addr, regs->data);
			if (ret != TWI_STATUS_OK) {
				csi_err("%s sensor write array error!\n",
					sensor_priv->name);
				return ret;
			}
			//sensor_read(sensor_priv, regs->addr, &rval);
			//csi_dbg("addr0x%x write data is 0x%x, read data is 0x%x\n", regs->addr, regs->data, rval);
		}
		i++;
		regs++;
	}
	return 0;
}

void csi_set_mclk_rate(sensor_private *sensor_priv, unsigned int clk_rate)
{
	sensor_priv->mclk_rate = clk_rate;
}

int csi_set_mclk(sensor_private *sensor_priv, unsigned int on)
{
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_id_t mclk_id = CLK_CSI_MCLK;

	if (on) {
		sensor_priv->mclk = hal_clock_get(HAL_SUNXI_CCU, CLK_CSI_MCLK);
		sensor_priv->mclk_src = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_DEVICE);
		hal_clk_set_parent(sensor_priv->mclk , sensor_priv->mclk_src);

		csi_dbg("set mclk_rate = %u\n", sensor_priv->mclk_rate);
		if (sensor_priv->mclk_rate)
			hal_clk_set_rate(sensor_priv->mclk, sensor_priv->mclk_rate);
		else {
			hal_clk_set_rate(sensor_priv->mclk, 24*1000*1000);
			csi_warn("mclk set to default 24M!!");
		}

		csi_dbg("get mclk_rate = %u\n", hal_clk_get_rate(sensor_priv->mclk));
		if (hal_clock_enable(sensor_priv->mclk)) {
			csi_err("csi clk enable mclk failed!\n");
			return -1;
		}
	} else {
		hal_clock_disable(sensor_priv->mclk);
	}

	return 0;
}

