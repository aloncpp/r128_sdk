/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
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

#include "gadget.h"
#include "audio.h"

#define SPEAKER_ENABLE 1
#define MIC_ENABLE 1

#define USE_FEATURE_UNIT

#define F_AUDIO_AC_INTERFACE		0
#define F_AUDIO_AS_OUT_INTERFACE	1
#define F_AUDIO_AS_IN_INTERFACE		2
/* Number of streaming interfaces */
#if (!SPEAKER_ENABLE && !MIC_ENABLE)
#error "spk or mic enable at least"
#elif (SPEAKER_ENABLE & MIC_ENABLE)
#define F_AUDIO_NUM_INTERFACES		2
#else
#define F_AUDIO_NUM_INTERFACES		1
#endif


#define UAC1_OUT_EP_MAX_PACKET_SIZE (200)

static int generic_set_cmd(struct usb_audio_control *con, u8 cmd, int value);
static int generic_get_cmd(struct usb_audio_control *con, u8 cmd);

struct f_uac1 {
	int g_audio;
	struct list_head cs;
	u8 ac_intf, as_in_intf, as_out_intf;
	u8 ac_alt, as_in_alt, as_out_alt;

	u8 set_cmd;
	int8_t set_ep;
	int prate;
	int crate;
	struct usb_audio_control *set_con;

} g_uac1 = {0};

static const uint16_t g_str_lang_id[] = {
	0x0304, 0x0409
};

static const uint16_t g_str_manufacturer[] = {
	0x0314, 'A', 'l', 'l', 'w', 'i', 'n', 'n', 'e', 'r'
};

static const uint16_t g_str_product[] = {
	0x030a, 'T', 'i', 'n', 'a',
};

static const uint16_t g_str_serialnumber[] = {
	0x0312, '2', '0', '0', '8', '0', '4', '1', '1'
};

static const uint16_t g_str_config[] = {
	0x0302,
};

static const uint16_t g_str_interface[] = {
	0x031a, 'A', 'C', ' ', 'I', 'n', 't', 'e', 'r', 'f', 'a', 'c', 'e'
};

#if SPEAKER_ENABLE
static const uint16_t g_str_usb_out_it[] = {
	0x0330, 'P', 'l', 'a', 'y', 'b', 'a', 'c', 'k', ' ', 'I', 'n', 'p', 'u', 't', ' ', 't', \
		'e', 'r', 'm', 'i', 'n', 'a', 'l'
};

static const uint16_t g_str_usb_out_it_ch_names[] = {
	0x0324, 'P', 'l', 'a', 'y', 'b', 'a', 'c', 'k', ' ', 'C', 'h', 'a', 'n', 'n', 'e', 'l', \
		's'
};

#ifdef USE_FEATURE_UNIT
static const uint16_t g_str_feat_desc0[] = {
	0x032c, 'V', 'o', 'l', 'u', 'm', 'e', ' ', 'c', 'o', 'n', 't', 'r', 'o', 'l', ' ', '&', \
		' ', 'm', 'u', 't', 'e'
};
#endif
static const uint16_t g_str_io_out_ot[] = {
	0x0332, 'P', 'l', 'a', 'y', 'b', 'a', 'c', 'k', ' ', 'O', 'u', 't', 'p', 'u', 't', ' ', \
		't', 'e', 'r', 'm', 'i', 'n', 'a', 'l'
};
#endif

#if MIC_ENABLE
static const uint16_t g_str_io_in_it[] = {
	0x032e, 'C', 'a', 'p', 't', 'u', 'r', 'e', ' ', 'I', 'n', 'p', 'u', 't', ' ', 't', 'e', \
		'r', 'm', 'i', 'n', 'a', 'l'
};

static const uint16_t g_str_io_in_it_ch_names[] = {
	0x0322, 'C', 'a', 'p', 't', 'u', 'r', 'e', ' ', 'C', 'h', 'a', 'n', 'n', 'e', 'l', 's'
};

static const uint16_t g_str_usb_in_ot[] = {
	0x0330, 'C', 'a', 'p', 't', 'u', 'r', 'e', ' ', 'O', 'u', 't', 'p', 'u', 't', ' ', 't', \
		'e', 'r', 'm', 'i', 'n', 'a', 'l'
};
#endif

#if SPEAKER_ENABLE
static const uint16_t g_str_as_out_if_alt0[] = {
	0x0324, 'P', 'l', 'a', 'y', 'b', 'a', 'c', 'k', ' ', 'I', 'n', 'a', 'c', 't', 'i', 'v', \
		'e'
};

static const uint16_t g_str_as_out_if_alt1[] = {
	0x0320, 'P', 'l', 'a', 'y', 'b', 'a', 'c', 'k', ' ', 'A', 'c', 't', 'i', 'v', 'e'
};
#endif

#if MIC_ENABLE
static const uint16_t g_str_as_in_if_alt0[] = {
	0x0322, 'C', 'a', 'p', 't', 'u', 'r', 'e', ' ', 'I', 'n', 'a', 'c', 't', 'i', 'v', 'e'
};

static const uint16_t g_str_as_in_if_alt1[] = {
	0x031e, 'C', 'a', 'p', 't', 'u', 'r', 'e', ' ', 'A', 'c', 't', 'i', 'v', 'e'
};
#endif

enum {
	STR_AC_IF = USB_GADGET_INTERFACE_IDX,
#if SPEAKER_ENABLE
	STR_USB_OUT_IT,
	STR_USB_OUT_IT_CH_NAMES,
#ifdef USE_FEATURE_UNIT
	STR_FEAT_DESC_0,
#endif
	STR_IO_OUT_OT,
#endif
#if MIC_ENABLE
	STR_IO_IN_IT,
	STR_IO_IN_IT_CH_NAMES,
	STR_USB_IN_OT,
#endif
#if SPEAKER_ENABLE
	STR_AS_OUT_IF_ALT0,
	STR_AS_OUT_IF_ALT1,
#endif
#if MIC_ENABLE
	STR_AS_IN_IF_ALT0,
	STR_AS_IN_IF_ALT1,
#endif
	STR_UAC_MAX,
};

static const uint16_t *uac_string_desc[STR_UAC_MAX + 1] = {
	g_str_lang_id,
	g_str_manufacturer,
	g_str_product,
	g_str_serialnumber,
	g_str_config,
	g_str_interface,
#if SPEAKER_ENABLE
	g_str_usb_out_it,
	g_str_usb_out_it_ch_names,
#ifdef USE_FEATURE_UNIT
	g_str_feat_desc0,
#endif
	g_str_io_out_ot,
#endif
#if MIC_ENABLE
	g_str_io_in_it,
	g_str_io_in_it_ch_names,
	g_str_usb_in_ot,
#endif
#if SPEAKER_ENABLE
	g_str_as_out_if_alt0,
	g_str_as_out_if_alt1,
#endif
#if MIC_ENABLE
	g_str_as_in_if_alt0,
	g_str_as_in_if_alt1,
#endif
	NULL,
};

static struct usb_device_descriptor uac_device_desc = {
	.bLength            = USB_DT_DEVICE_SIZE,
	.bDescriptorType    = USB_DT_DEVICE,
	.bcdUSB             = 0x0200,
	.bDeviceClass       = 0x0,
	.bDeviceSubClass    = 0,
	.bDeviceProtocol    = 0,
	.bMaxPacketSize0    = 64,
	.idVendor           = 0x1d61,
	.idProduct          = 0x0101,
	.bcdDevice          = 0x0409,
	.iManufacturer      = USB_GADGET_MANUFACTURER_IDX,
	.iProduct           = USB_GADGET_PRODUCT_IDX,
	.iSerialNumber      = USB_GADGET_SERIAL_IDX,
	.bNumConfigurations = 1
};

static struct usb_config_descriptor uac_config_desc = {
	.bLength         = USB_DT_CONFIG_SIZE,
	.bDescriptorType = USB_DT_CONFIG,
	.wTotalLength    = 0,
	.bNumInterfaces      = 3,
	.bConfigurationValue = 1,
	.iConfiguration      = USB_GADGET_CONFIG_IDX,
	.bmAttributes        = 0xc0,
	.bMaxPower           = 0xfa                    /* 500mA */
};


static struct usb_interface_assoc_descriptor ac_interface_association_desc = {
	.bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
        .bDescriptorType =  USB_DT_INTERFACE_ASSOCIATION,
        .bFirstInterface = 0,
        .bInterfaceCount = F_AUDIO_NUM_INTERFACES+1,
        .bFunctionClass = USB_CLASS_AUDIO,//audio
        .bFunctionSubClass = 0x00,//
        .bFunctionProtocol = UAC_VERSION_1,
        .iFunction = STR_AC_IF,
};

static struct usb_interface_descriptor ac_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOCONTROL,
	.iInterface = STR_AC_IF,
};

DECLARE_UAC_AC_HEADER_DESCRIPTOR(2);

#define UAC_DT_AC_HEADER_LENGTH	UAC_DT_AC_HEADER_SIZE(F_AUDIO_NUM_INTERFACES)
/* 2 input terminals and 2 output terminals */
#ifdef USE_FEATURE_UNIT
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH \
	+ F_AUDIO_NUM_INTERFACES*UAC_DT_INPUT_TERMINAL_SIZE + \
	F_AUDIO_NUM_INTERFACES*UAC_DT_OUTPUT_TERMINAL_SIZE + UAC_DT_FEATURE_UNIT_SIZE(0))
#else
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH \
	+ F_AUDIO_NUM_INTERFACES*UAC_DT_INPUT_TERMINAL_SIZE + \
	F_AUDIO_NUM_INTERFACES*UAC_DT_OUTPUT_TERMINAL_SIZE)
#endif

static struct uac1_ac_header_descriptor_2 ac_header_desc = {
	.bLength =		UAC_DT_AC_HEADER_LENGTH,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_HEADER,
	.bcdADC =		cpu_to_le16(0x0100),
	.wTotalLength =		cpu_to_le16(UAC_DT_TOTAL_LENGTH),
	.bInCollection =	F_AUDIO_NUM_INTERFACES,
	.baInterfaceNr = {
	/* Interface number of the AudioStream interfaces */
		[0] =		1,
		[1] =		2,
	}
};

#define USB_OUT_IT_ID	1
static struct uac_input_terminal_descriptor usb_out_it_desc = {
	.bLength =		UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_INPUT_TERMINAL,
	.bTerminalID =		USB_OUT_IT_ID,
	.wTerminalType =	cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal =	0,
	.wChannelConfig =	cpu_to_le16(0x3),
	.iTerminal = STR_USB_OUT_IT,
	.iChannelNames = STR_USB_OUT_IT_CH_NAMES,
};

#if SPEAKER_ENABLE
#ifdef USE_FEATURE_UNIT
#define FEATURE_UNIT_ID 2
DECLARE_UAC_FEATURE_UNIT_DESCRIPTOR(0);
static struct uac_feature_unit_descriptor_0 feature_unit_desc = {
	.bLength =		UAC_DT_FEATURE_UNIT_SIZE(0),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FEATURE_UNIT,
	.bUnitID =		FEATURE_UNIT_ID,
	.bSourceID =		USB_OUT_IT_ID,
	.bControlSize	=	2,
	.bmaControls[0] =	(UAC_FU_MUTE | UAC_FU_VOLUME),
	.iFeature = STR_FEAT_DESC_0,
};

static struct usb_audio_control mute_control = {
	.list = LIST_HEAD_INIT(mute_control.list),
	.name = "Mute Control",
	.type = UAC_FU_MUTE,
	/* Todo: add real Mute control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control volume_control = {
	.list = LIST_HEAD_INIT(volume_control.list),
	.name = "Volume Control",
	.type = UAC_FU_VOLUME,
	/* Todo: add real Volume control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control_selector feature_unit = {
	.list = LIST_HEAD_INIT(feature_unit.list),
	.id = FEATURE_UNIT_ID,
	.name = "Mute & Volume Control",
	.type = UAC_FEATURE_UNIT,
	.desc = (struct usb_descriptor_header *)&feature_unit_desc,
};
#endif /* USE_FEATURE_UNIT */
#endif /* SPEAKER_ENABLE */

#define IO_OUT_OT_ID	3
static struct uac1_output_terminal_descriptor io_out_ot_desc = {
	.bLength		= UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_OUTPUT_TERMINAL,
	.bTerminalID		= IO_OUT_OT_ID,
	.wTerminalType		= cpu_to_le16(UAC_OUTPUT_TERMINAL_SPEAKER),
#ifdef USE_FEATURE_UNIT
	.bAssocTerminal		= USB_OUT_IT_ID,
	.bSourceID		= FEATURE_UNIT_ID,
#else
	.bAssocTerminal		= USB_OUT_IT_ID,
	.bSourceID		= USB_OUT_IT_ID,
#endif
	.iTerminal = STR_IO_OUT_OT,
};

#define IO_IN_IT_ID	4
static struct uac_input_terminal_descriptor io_in_it_desc = {
	.bLength		= UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_INPUT_TERMINAL,
	.bTerminalID		= IO_IN_IT_ID,
	.wTerminalType		= cpu_to_le16(UAC_INPUT_TERMINAL_MICROPHONE),
	.bAssocTerminal		= 0,
	.wChannelConfig		= cpu_to_le16(0x3),
	.iTerminal		= STR_IO_IN_IT,
	.iChannelNames		= STR_IO_IN_IT_CH_NAMES,
};

#if MIC_ENABLE
#define USB_IN_OT_ID	5
static struct uac1_output_terminal_descriptor usb_in_ot_desc = {
	.bLength =		UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_OUTPUT_TERMINAL,
	.bTerminalID =		USB_IN_OT_ID,
	.wTerminalType =	cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal =	IO_IN_IT_ID,
	.bSourceID =		IO_IN_IT_ID,
	.iTerminal = STR_USB_IN_OT,
};
#endif



/* B.4.1  Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_out_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.iInterface = STR_AS_OUT_IF_ALT0,
};

static struct usb_interface_descriptor as_out_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.iInterface = STR_AS_OUT_IF_ALT1,
};

#if MIC_ENABLE
static struct usb_interface_descriptor as_in_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.iInterface = STR_AS_IN_IF_ALT0,
};

static struct usb_interface_descriptor as_in_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.iInterface = STR_AS_IN_IF_ALT1,
};
#endif

#if SPEAKER_ENABLE
/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_out_header_desc = {
	.bLength =		UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink =	USB_OUT_IT_ID,
	.bDelay =		1,
	.wFormatTag =		cpu_to_le16(UAC_FORMAT_TYPE_I_PCM),
};
#endif

#if MIC_ENABLE
static struct uac1_as_header_descriptor as_in_header_desc = {
	.bLength =		UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink =	USB_IN_OT_ID,
	.bDelay =		1,
	.wFormatTag =		cpu_to_le16(UAC_FORMAT_TYPE_I_PCM),
};
#endif

DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(1);
#if SPEAKER_ENABLE
static struct uac_format_type_i_discrete_descriptor_1 as_out_type_i_desc = {
	.bLength =		UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		1,
};

/* Standard ISO OUT Endpoint Descriptor */
static struct usb_endpoint_descriptor as_out_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	0x3 | USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize	=	cpu_to_le16(UAC1_OUT_EP_MAX_PACKET_SIZE),
	.bInterval =		4,
};

/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_iso_out_desc = {
	.bLength =		UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes =		1,
	.bLockDelayUnits =	1,
	.wLockDelay =		cpu_to_le16(1),
};
#endif

#if MIC_ENABLE
static struct uac_format_type_i_discrete_descriptor_1 as_in_type_i_desc = {
	.bLength =		UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		1,
};

/* Standard ISO OUT Endpoint Descriptor */
static struct usb_endpoint_descriptor as_in_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	0x3 | USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_SYNC_ASYNC
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize	=	cpu_to_le16(UAC1_OUT_EP_MAX_PACKET_SIZE),
	.bInterval =		4,
};

/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_iso_in_desc = {
	.bLength =		UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes =		1,
	.bLockDelayUnits =	0,
	.wLockDelay =		0,
};
#endif

static struct usb_descriptor_header *f_audio_desc[] = {
	(struct usb_descriptor_header *)&uac_config_desc,
	(struct usb_descriptor_header *)&ac_interface_association_desc,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc,
#if SPEAKER_ENABLE
	(struct usb_descriptor_header *)&usb_out_it_desc,
#ifdef USE_FEATURE_UNIT
	(struct usb_descriptor_header *)&feature_unit_desc,
#endif
	(struct usb_descriptor_header *)&io_out_ot_desc,
#endif
#if MIC_ENABLE
	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,
#endif
#if SPEAKER_ENABLE
	(struct usb_descriptor_header *)&as_out_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_out_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_out_header_desc,

	(struct usb_descriptor_header *)&as_out_type_i_desc,

	(struct usb_descriptor_header *)&as_out_ep_desc,
	(struct usb_descriptor_header *)&as_iso_out_desc,
#endif
#if MIC_ENABLE
	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_header_desc,

	(struct usb_descriptor_header *)&as_in_type_i_desc,

	(struct usb_descriptor_header *)&as_in_ep_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
#endif
	NULL,
};

__attribute__((weak)) int u_audio_init(int stream, int rate, int ch, int bits)
{

}

__attribute__((weak)) int u_audio_start_capture(void *arg)
{

}

__attribute__((weak)) int u_audio_stop_capture(void *arg)
{

}

__attribute__((weak)) int u_audio_start_playback(void *arg)
{

}

__attribute__((weak)) int u_audio_stop_playback(void *arg)
{

}


#if SPEAKER_ENABLE
#define DEFAULT_VOLUME_CUR	(0xffc0)
#define DEFAULT_VOLUME_MIN	(0xcd00)
#define DEFAULT_VOLUME_MAX	(0xffff)
#define DEFAULT_VOLUME_RES	(131)

static int dB_convert(int value)
{
	int index = 0;
	short db_value = 0;

	value &= 0xffff;
	db_value = value - DEFAULT_VOLUME_MIN;
	index = db_value / DEFAULT_VOLUME_RES;

	return index;
}

static int generic_set_cmd(struct usb_audio_control *con, u8 cmd, int value)
{
	struct f_uac1 *uac1 = NULL;
	struct list_head *cs_list = NULL;

	/* TODO */
	cs_list = feature_unit.list.next;
	uac1 = container_of(cs_list, struct f_uac1, cs);

	switch (cmd) {
	case UAC__CUR:
		con->data[cmd] = value;
		/*g_audio_notify(audio, con->type, dB_convert(value));*/
		break;
	case UAC__MIN:
	case UAC__MAX:
		gadget_err("unknown cmd:%d\n", cmd);
		break;
	case UAC__RES:
		break;
	default:
		break;
	}

	return 0;
}

static int generic_get_cmd(struct usb_audio_control *con, u8 cmd)
{
	return con->data[cmd];
}

static int control_selector_init(struct f_uac1 *uac1)
{
	INIT_LIST_HEAD(&uac1->cs);
	list_add(&feature_unit.list, &uac1->cs);

	INIT_LIST_HEAD(&feature_unit.control);
	list_add(&mute_control.list, &feature_unit.control);
	list_add(&volume_control.list, &feature_unit.control);

	volume_control.data[UAC__CUR] = DEFAULT_VOLUME_CUR;
	volume_control.data[UAC__MIN] = DEFAULT_VOLUME_MIN;
	volume_control.data[UAC__MAX] = DEFAULT_VOLUME_MAX;
	volume_control.data[UAC__RES] = DEFAULT_VOLUME_RES;
}

#endif


static int audio_set_alt(uint32_t intf, uint32_t alt)
{
	struct f_uac1 *uac1 = &g_uac1;
	int ret = 0;

	if (alt > 1) {
		gadget_err("unknown alt:%d", alt);
		return -EINVAL;
	}

	if (intf == uac1->ac_intf) {
		if (alt) {
			gadget_err("unknown alt:%d with ac intf", alt);
			return -EINVAL;
		}
		return 0;
	}

#if 0
	printf("[%s] line:%d intf:%u, as_out_intf:%u, as_in_intf:%u\n", __func__, __LINE__,
				intf, uac1->as_out_intf, uac1->as_in_intf);
	printf("[%s] line:%d alt:%u, as_out_alt:%u, as_in_alt:%u\n", __func__, __LINE__,
				alt, uac1->as_out_alt, uac1->as_in_alt);
#endif
	if (intf == uac1->as_out_intf) {
		uac1->as_out_alt = alt;
		if (alt)
			ret = u_audio_start_capture(&uac1->g_audio);
		else
			u_audio_stop_capture(&uac1->g_audio);
	} else if (intf == uac1->as_in_intf) {
		uac1->as_in_alt = alt;
		if (alt)
			ret = u_audio_start_playback(&uac1->g_audio);
		else
			u_audio_stop_playback(&uac1->g_audio);
	} else {
		gadget_err("unknown intf:%d", intf);
		return -EINVAL;
	}
	return ret;
}

static int uac_standard_req(struct usb_ctrlrequest *crq)
{
	unsigned short w_index = le16_to_cpu(crq->wIndex);
	unsigned short w_value = le16_to_cpu(crq->wValue);
	unsigned short w_length = le16_to_cpu(crq->wLength);

	/*gadget_err("standard req:%u, type:0x%x", crq->bRequest, crq->bRequestType);*/
	/*gadget_err("index:0x%x, value:0x%x, len:0x%x", w_index, w_value, w_length);*/
	switch (crq->bRequest) {
	case USB_REQ_SET_CONFIGURATION:
		/* init ISOC ep */
		/*printf("[%s] line:%d enable EP\n", __func__, __LINE__);*/
		hal_udc_ep_enable(as_in_ep_desc.bEndpointAddress,
				  as_in_ep_desc.wMaxPacketSize,
				  as_in_ep_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK);
		hal_udc_ep_enable(as_out_ep_desc.bEndpointAddress,
				  as_out_ep_desc.wMaxPacketSize,
				  as_out_ep_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK);
		break;
	case USB_REQ_SET_INTERFACE:
		/*printf("index=%d, value=%d\n", w_index, w_value);*/
		audio_set_alt(w_index, w_value);
		break;
	default:
		break;
	}
	return 0;
}

static void uac_audio_complete(struct usb_ctrlrequest *crq, void *data, int len)
{
#if 0
	unsigned short w_index = le16_to_cpu(crq->wIndex);
	unsigned short w_value = le16_to_cpu(crq->wValue);
	unsigned short w_length = le16_to_cpu(crq->wLength);
#endif
	/*uint16_t ep = le16_to_cpu(crq->wIndex);*/

	struct f_uac1 *uac1 = &g_uac1;
	uint32_t value = 0;
	int rate = 0;
	u8 *buf = (u8 *)data;

	if (uac1->set_con) {
		memcpy(&value, data, len);
		/*printf("[%s] line:%d value:0x%x, len=%d\n", __func__, __LINE__, value, len);*/
		uac1->set_con->set(uac1->set_con, uac1->set_cmd, le16_to_cpu(value));
		uac1->set_con = NULL;
		return;
	}

	rate = buf[0] | (buf[1] << 8) | (buf[2] << 16);
	/*printf("[%s] line:%d rate=%d\n", __func__, __LINE__, rate);*/

	switch (crq->bRequest) {
	case UAC_SET_CUR:
		if (uac1->set_ep == as_in_ep_desc.bEndpointAddress) {
			uac1->crate = rate;
		} else if (uac1->set_ep == as_out_ep_desc.bEndpointAddress) {\
			uac1->prate = rate;
		}
		uac1->set_ep = 0;
		break;
	case UAC_SET_MIN:
	case UAC_SET_MAX:
	case UAC_SET_RES:
	case UAC_SET_MEM:
	default:
		break;
	}

}

static int audio_set_intf_req(struct usb_ctrlrequest *crq, int *data)
{
	struct f_uac1 *uac1 = &g_uac1;
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;
	int value = -EOPNOTSUPP;
	uint8_t id = ((le16_to_cpu(crq->wIndex) >> 8) & 0xFF);
	uint16_t len = le16_to_cpu(crq->wLength);
	uint16_t w_value = le16_to_cpu(crq->wValue);
	uint8_t con_sel = (w_value >> 8) & 0xFF;
	uint8_t cmd = (crq->bRequest & 0x0F);

	gadget_debug("bRequest:0x%x, w_value:0x%x, len %d",
		crq->bRequest, w_value, len);
	list_for_each_entry(cs, &uac1->cs, list) {
		if (cs->id == id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == con_sel) {
					uac1->set_con = con;
					break;
				}
			}
			break;
		}
	}

	uac1->set_cmd = cmd;

	return len;
}

static int audio_get_intf_req(struct usb_ctrlrequest *crq, int *data)
{
	struct f_uac1 *uac1 = &g_uac1;
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;
	int value = -EOPNOTSUPP;
	uint8_t id = ((le16_to_cpu(crq->wIndex) >> 8) & 0xFF);
	uint16_t w_value = le16_to_cpu(crq->wValue);
	uint8_t con_sel = (w_value >> 8) & 0xFF;
	uint8_t cmd = (crq->bRequest & 0x0F);

	list_for_each_entry(cs, &uac1->cs, list) {
		if (cs->id == id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == con_sel && con->get) {
					value = con->get(con, cmd);
					break;
				}
			}
			break;
		}
	}

	*data = value;
	/*printf("[%s] line:%d value:%d\n", __func__, __LINE__, value);*/

	return sizeof(value);
}

static int audio_set_endpoint_req(struct usb_ctrlrequest *crq, int *data)
{
	struct f_uac1 *uac1 = &g_uac1;
	int value = -EOPNOTSUPP;
	uint16_t ep = le16_to_cpu(crq->wIndex);
	uint16_t len = le16_to_cpu(crq->wLength);
	uint16_t w_value = le16_to_cpu(crq->wValue);


	gadget_debug("bRequest:0x%x, w_value:0x%x, len %d, ep %d",
		crq->bRequest, w_value, len, ep);

	switch (crq->bRequest) {
	case UAC_SET_CUR:
		value = len;
		uac1->set_ep = ep;
		break;
	case UAC_SET_MIN:
		break;
	case UAC_SET_MAX:
		break;
	case UAC_SET_RES:
		break;
	case UAC_SET_MEM:
		break;
	default:
		break;
	}

	return value;
}

static int audio_get_endpoint_req(struct usb_ctrlrequest *crq, int *data)
{
	struct f_uac1 *uac1 = &g_uac1;
	int value = -EOPNOTSUPP;
	uint16_t ep = le16_to_cpu(crq->wIndex);
	uint16_t len = le16_to_cpu(crq->wLength);
	uint16_t w_value = le16_to_cpu(crq->wValue);
	int rate = 0;

	gadget_debug("bRequest:0x%x, w_value:0x%x, len %d, ep %d",
		crq->bRequest, w_value, len, ep);

	if (w_value != (UAC_EP_CS_ATTR_SAMPLE_RATE << 8))
		return value;

	switch (crq->bRequest) {
	case UAC_GET_CUR:
	case UAC_GET_MIN:
	case UAC_GET_MAX:
	case UAC_GET_RES:
		if (ep  == as_in_ep_desc.bEndpointAddress) {
			rate = uac1->crate;
		} else if (ep == as_out_ep_desc.bEndpointAddress) {
			rate = uac1->prate;
		}
		/*printf("[%s] line:%d rate:%d\n", __func__, __LINE__, rate);*/
		*data = rate;
		value = len;
		break;
	case UAC_GET_MEM:
		break;
	default:
		break;
	}

	return value;
}

struct usb_function_driver uac_usb_func;
extern int g_dump_flag;
static int uac_class_req(struct usb_ctrlrequest *crq)
{
	int			value = -EOPNOTSUPP;
	unsigned short w_index = le16_to_cpu(crq->wIndex);
	unsigned short w_value = le16_to_cpu(crq->wValue);
	unsigned short w_length = le16_to_cpu(crq->wLength);
	static int data = 0;
	int is_in = 0;

	/*gadget_err("standard req:0x%x, type:0x%x", crq->bRequest, crq->bRequestType);*/
	/*gadget_err("index:0x%x, value:0x%x, len:0x%x", w_index, w_value, w_length);*/

	if (crq->bRequestType & USB_DIR_IN)
		is_in = USB_DIR_IN;

	data = 0;
	switch (crq->bRequestType) {
	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		value = audio_set_intf_req(crq, &data);
		/*gadget_err("standard req:0x%x, type:0x%x", crq->bRequest, crq->bRequestType);*/
		break;

	case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		value = audio_get_intf_req(crq, &data);
		break;

	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_set_endpoint_req(crq, &data);
		/*gadget_err("standard req:0x%x, type:0x%x", crq->bRequest, crq->bRequestType);*/
		break;

	case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_get_endpoint_req(crq, &data);
		break;

	default:
		gadget_err("invalid control req%02x.%02x v%04x i%04x l%d\n",
			crq->bRequestType, crq->bRequest,
			w_value, w_index, w_length);
	}

	if (value >= 0) {
		/*value = usb_gadget_function_write(0, (char *)&data, value);*/
		/*value = hal_udc_ep_write(fd->ep_addr[ep_idx], (void *)buf, size);*/
		/*g_dump_flag = 1;*/
		int len = min(sizeof(int), w_length);
		/*printf("[%s] line:%d len=%d, data:0x%x\n", __func__, __LINE__, len, data);*/
#if 0
		if (crq->bRequest == 1 && crq->bRequestType == 0x21)
			g_dump_flag = 1;
#endif
		/*value = hal_udc_ep_write(uac_usb_func.ep_addr[0], (void *)&data, len);*/
		hal_udc_ep_set_buf(0 | is_in, (void *)&data, len);
	}

	return value;
}


static int uac_desc_init(struct usb_function_driver *fd)
{
	struct f_uac1 *uac1 = &g_uac1;
	int i;
	void *buf = NULL;
	uint16_t *str;
	uint32_t len;
	uint8_t *sam_freq;
	int rate;
	int status = 0;
#define UAC_DEFAULT_PB_CHANNELS		(2)
#define UAC_DEFAULT_PB_CHANNELS_MASK	(0x3)
#define UAC_DEFAULT_PB_RATE		(48000)
#define UAC_DEFAULT_PB_BITS		(16)

#define UAC_DEFAULT_CAP_CHANNELS	(2)
#define UAC_DEFAULT_CAP_CHANNELS_MASK	(0x3)
#define UAC_DEFAULT_CAP_RATE		(16000)
#define UAC_DEFAULT_CAP_BITS		(16)


	memset(uac1, 0, sizeof(struct f_uac1));

#if SPEAKER_ENABLE
	/* device capture */
	u_audio_init(1, UAC_DEFAULT_PB_RATE, UAC_DEFAULT_PB_CHANNELS, UAC_DEFAULT_PB_BITS);
#endif
#if MIC_ENABLE
	/* device playback */
	u_audio_init(0, UAC_DEFAULT_CAP_RATE, UAC_DEFAULT_CAP_CHANNELS, UAC_DEFAULT_CAP_BITS);
#endif


	/* Set channel numbers */
	usb_out_it_desc.bNrChannels = UAC_DEFAULT_PB_CHANNELS;
	usb_out_it_desc.wChannelConfig = UAC_DEFAULT_PB_CHANNELS_MASK;
#if SPEAKER_ENABLE
	/* device playback */
	as_out_type_i_desc.bNrChannels = UAC_DEFAULT_PB_CHANNELS;
	as_out_type_i_desc.bSubframeSize = UAC_DEFAULT_PB_BITS / 8;
	as_out_type_i_desc.bBitResolution = UAC_DEFAULT_PB_BITS;
#endif
	io_in_it_desc.bNrChannels = UAC_DEFAULT_CAP_CHANNELS;
	io_in_it_desc.wChannelConfig = UAC_DEFAULT_CAP_CHANNELS_MASK;
#if MIC_ENABLE
	/* device capture */
	as_in_type_i_desc.bNrChannels = UAC_DEFAULT_CAP_CHANNELS;
	as_in_type_i_desc.bSubframeSize = UAC_DEFAULT_CAP_BITS / 8;
	as_in_type_i_desc.bBitResolution = UAC_DEFAULT_CAP_BITS;
#endif

#if SPEAKER_ENABLE
	/* Set sample rates */
	rate = UAC_DEFAULT_PB_RATE;
	sam_freq = as_out_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
	uac1->prate = rate;
#endif
#if MIC_ENABLE
	rate = UAC_DEFAULT_CAP_RATE;
	sam_freq = as_in_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
	uac1->crate = rate;
#endif

	status = 0;
	ac_interface_desc.bInterfaceNumber = status;
	uac1->ac_intf = status;
	uac1->ac_alt = 0;

	ac_interface_association_desc.bFirstInterface = status;

#if SPEAKER_ENABLE
	status++;
	as_out_interface_alt_0_desc.bInterfaceNumber = status;
	as_out_interface_alt_1_desc.bInterfaceNumber = status;
	ac_header_desc.baInterfaceNr[0] = status;
	uac1->as_out_intf = status;
	uac1->as_out_alt = 0;
#endif
#if MIC_ENABLE
	status++;
	as_in_interface_alt_0_desc.bInterfaceNumber = status;
	as_in_interface_alt_1_desc.bInterfaceNumber = status;
#if !SPEAKER_ENABLE
	ac_header_desc.baInterfaceNr[0] = status;
#else
	ac_header_desc.baInterfaceNr[1] = status;
#endif
#endif
	uac1->as_in_intf = status;
	uac1->as_in_alt = 0;



	control_selector_init(uac1);


	fd->config_desc = usb_copy_config_descriptors(f_audio_desc);
	len = ((struct usb_config_descriptor *)(fd->config_desc))->wTotalLength;

	hal_udc_device_desc_init(&uac_device_desc);
	hal_udc_config_desc_init(fd->config_desc, len);

	i = 0;
	while (uac_string_desc[i] != NULL) {
		str = (uint16_t *)uac_string_desc[i];
		if (i < USB_GADGET_MAX_IDX && fd->strings[i] != NULL)
			str = fd->strings[i];
		hal_udc_string_desc_init(str);
		i++;
		/*printf("i = %d\n", i);*/
	}

	fd->class_req = uac_class_req;
	fd->standard_req = uac_standard_req;
	fd->ep0_rx_complete = uac_audio_complete;

	fd->ep_addr = calloc(3, sizeof(uint8_t));
	if (!fd->ep_addr) {
		gadget_err("no memory.\n");
		goto err;
	}
	fd->ep_addr[0] = 0;
	fd->ep_addr[1] = as_in_ep_desc.bEndpointAddress;
	fd->ep_addr[2] = as_out_ep_desc.bEndpointAddress;

	return 0;
err:
	if (fd->config_desc) {
		free(fd->config_desc);
		fd->config_desc = NULL;
	}

	if (fd->ep_addr) {
		free(fd->ep_addr);
		fd->ep_addr = NULL;
	}

	return 0;
}

static int uac_desc_deinit(struct usb_function_driver *fd)
{
	if (fd->config_desc) {
		free(fd->config_desc);
		fd->config_desc = NULL;
	}

	if (fd->ep_addr) {
		free(fd->ep_addr);
		fd->ep_addr = NULL;
	}

	for (int i = 0; i < USB_GADGET_MAX_IDX; i++) {
		if (fd->strings[i] != NULL) {
			free(fd->strings[i]);
			fd->strings[i] = NULL;
		}
	}

	return 0;
}

struct usb_function_driver uac_usb_func = {
	.name        = "uac",
	.desc_init   = uac_desc_init,
	.desc_deinit = uac_desc_deinit,
};

int usb_gadget_uac_init(void)
{
	return usb_gadget_function_register(&uac_usb_func);
}

int usb_gadget_uac_deinit(void)
{
	return usb_gadget_function_unregister(&uac_usb_func);
}
