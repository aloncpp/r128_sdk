/*
********************************************************************************************************************
*                                              usb host driver
*
*                              (c) Copyright 2007-2010, javen.China
*                                       All Rights Reserved
*
* File Name     : usb_msc.c
*
* Author        : javen
*
* Version       : 2.0
*
* Date          : 2010.03.02
*
* Description   :
*
* History       :
*
********************************************************************************************************************
*/


#include "usb_gen_hcd.h"
#include "usb_core_base.h"
#include "mod_devicetable.h"

#include "usbh_disk_info.h"
#include "usbh_buff_manager.h"
#include "usbh_disk_remove_time.h"

#include "usb_msc_i.h"
#include "mscTransport.h"
#include "mscProtocol.h"
#include "LunMgr.h"

#define MSC_DRV_NAME	"Mass Storage Class"
#define MSC_DRV_AUTHOR	"Host Driver Author"

static struct usb_driver mscDrv; /*  */
static __u32 mscDev_id_array;		/* 记录了mscDev的编号 */

static struct usb_device_id msc_match_table[] = {
	//--1--CB
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_RBC
		, USBMSC_INTERFACE_PROTOCOL_CBT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_8020
		, USBMSC_INTERFACE_PROTOCOL_CBT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_QIC
		, USBMSC_INTERFACE_PROTOCOL_CBT
		, 0 //driver_info;
	},

	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_UFI
		, USBMSC_INTERFACE_PROTOCOL_CBT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_8070
		, USBMSC_INTERFACE_PROTOCOL_CBT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_SCSI
		, USBMSC_INTERFACE_PROTOCOL_CBT
		, 0 //driver_info;
	},
	//--2--CBI
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_RBC
		, USBMSC_INTERFACE_PROTOCOL_CBIT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_8020
		, USBMSC_INTERFACE_PROTOCOL_CBIT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_QIC
		, USBMSC_INTERFACE_PROTOCOL_CBIT
		, 0 //driver_info;
	},

	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_UFI
		, USBMSC_INTERFACE_PROTOCOL_CBIT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_8070
		, USBMSC_INTERFACE_PROTOCOL_CBIT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_SCSI
		, USBMSC_INTERFACE_PROTOCOL_CBIT
		, 0 //driver_info;
	},
	//--3--BO
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_RBC
		, USBMSC_INTERFACE_PROTOCOL_BOT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_8020
		, USBMSC_INTERFACE_PROTOCOL_BOT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_QIC
		, USBMSC_INTERFACE_PROTOCOL_BOT
		, 0 //driver_info;
	},

	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_UFI
		, USBMSC_INTERFACE_PROTOCOL_BOT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_8070
		, USBMSC_INTERFACE_PROTOCOL_BOT
		, 0 //driver_info;
	},
	{
		USB_DEVICE_ID_MATCH_INT_INFO    //match_flags;
		, 0, 0, 0, 0, 0, 0, 0
		//bInterfaceClass;bInterfaceSubClass;bInterfaceProtocol
		, USB_CLASS_MASS_STORAGE
		, USBMSC_SUBCLASS_SCSI
		, USBMSC_INTERFACE_PROTOCOL_BOT
		, 0 //driver_info;
	},
	{
		0
		, 0, 0, 0, 0, 0, 0, 0
		, 0
		, 0
		, 0

		, 0
	},

	{
		0  //match_flags;

		, 0 //idVendor;
		, 0 //idProduct;
		, 0 //bcdDevice_lo;
		, 0 //bcdDevice_hi;

		//Used for device class matches
		, 0 //bDeviceClass;
		, 0 //bDeviceSubClass;
		, 0 //bDeviceProtocol;

		//Used for interface class matches
		, 0 //bInterfaceClass;
		, 0 //bInterfaceSubClass;
		, 0 //bInterfaceProtocol;

		//not matched against
		, 0 //driver_info;
	}/* Terminating entry */
};

/*
*******************************************************************************
*                     get_mscDev_id
*
* Description:
*    给新插入的设备分配ID
*
* Parameters:
*    void
*
* Return value:
*    返回分配的ID
*
* note:
*    无
*
*******************************************************************************
*/
static unsigned int get_mscDev_id(void)
{
	unsigned int bit = 0;
	bit = find_next_zero_bit((const volatile u32 *)&mscDev_id_array, (1 * 32), 0);

	if (bit > 32) {
		hal_log_err("ERR: find_next_zero_bit failed");
		return 31;
	}

	usb_set_bit(bit, (volatile uint32_t *)&mscDev_id_array);
	return bit;
}

/*
*******************************************************************************
*                     free_mscDev_id
*
* Description:
*    释放分配的ID
*
* Parameters:
*    bit : input.  mscDev的ID号
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static int free_mscDev_id(unsigned int bit)
{
	if (usb_test_bit(bit, (volatile uint32_t *)&mscDev_id_array) == 0) {
		hal_log_err("ERR: free_host_id: invalid bit(%d)", bit);
		return -1;
	}

	/* 回收该位 */
	usb_clear_bit(bit, (volatile uint32_t *)&mscDev_id_array);
	return 0;
}

/*
*******************************************************************************
*                     mscDevInit
*
* Description:
*
*
* Parameters:
*    mscDev  ： input. msc设备信息
*    intf    :  input. USB接口
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static int mscDevInit(__mscDev_t *mscDev, struct usb_interface *intf)
{
	int ret = 0;
	__u8 err = 0;

	if (mscDev == NULL) {
		hal_log_err("ERR: invalid argment");
		return -1;
	}

	mscDev->pusb_dev = interface_to_usbdev(intf);

	if (mscDev->pusb_dev == NULL) {
		hal_log_err("ERR: invalid argment");
		return -1;
	}

	mscDev->pusb_intf = intf;
	mscDev->InterfaceNo = intf->cur_altsetting->desc.bInterfaceNumber;
	/* Store our private data in the interface */
	usb_set_intfdata(intf, (void *)mscDev);
	mscDev->mscDevNo = get_mscDev_id();
	USB_INIT_LIST_HEAD(&(mscDev->cmd_list));
	/* alloc urb */
	mscDev->CurrentUrb = usb_alloc_urb(0);

	if (mscDev->CurrentUrb == NULL) {
		hal_log_err("ERR: hal_sem_create scan_lock failed");
		return USB_ERR_ALLOC_URB_FAILED;
	}

	/* 一把钥匙 */
	mscDev->scan_lock = hal_sem_create(1);

	if (mscDev->scan_lock == NULL) {
		hal_log_err("ERR: hal_sem_create scan_lock failed");
		ret = USB_ERR_CREATE_SIME_FAILED;
		goto error0;
	}

	mscDev->DevLock = hal_sem_create(1);

	if (mscDev->DevLock == NULL) {
		hal_log_err("ERR: hal_sem_create DevLock failed");
		ret = USB_ERR_CREATE_SIME_FAILED;
		goto error1;
	}

	/* 线程同步 */
	mscDev->ThreadSemi = hal_sem_create(0);

	if (mscDev->ThreadSemi == NULL) {
		hal_log_err("ERR: hal_sem_create dev_lock failed");
		ret = USB_ERR_CREATE_SIME_FAILED;
		goto error2;
	}

	mscDev->notify_complete = hal_sem_create(0);

	if (mscDev->notify_complete == NULL) {
		hal_log_err("ERR: hal_sem_create notify_complete failed");
		ret = USB_ERR_CREATE_SIME_FAILED;
		goto error3;
	}

	mscDev->CtrlReq = (struct usb_ctrlrequest *)usb_buffer_alloc(mscDev->pusb_dev,
								     sizeof(struct usb_ctrlrequest),
								     0,
								     NULL);

	if (mscDev->CtrlReq == NULL) {
		hal_log_err("ERR: usb_buffer_alloc CtrlReq failed");
		ret = USB_ERR_MALLOC_FAILED;
		goto error4;
	}

	mscDev->iobuf = (__u8 *)usb_buffer_alloc(mscDev->pusb_dev, MSC_IOBUF_SIZE, 0, NULL);

	if (mscDev->iobuf == NULL) {
		hal_log_err("ERR: usb_buffer_alloc iobuf failed");
		ret = USB_ERR_MALLOC_FAILED;
		goto error5;
	}

	mscDev->UrbWait = hal_sem_create(0);

	if (mscDev->UrbWait == NULL) {
		hal_log_err("ERR: hal_sem_create UrbWait failed");
		ret = USB_ERR_CREATE_SIME_FAILED;
		goto error6;
	}

	return USB_ERR_SUCCESS;
error6:
	usb_buffer_free(mscDev->pusb_dev, MSC_IOBUF_SIZE, (void *)(mscDev->iobuf), 0);
error5:
	usb_buffer_free(mscDev->pusb_dev, sizeof(*mscDev->CtrlReq), (void *)(mscDev->CtrlReq), 0);
error4:
	hal_sem_delete(mscDev->notify_complete);
error3:
	hal_sem_delete(mscDev->ThreadSemi);
error2:
	hal_sem_delete(mscDev->DevLock);
error1:
	hal_sem_delete(mscDev->scan_lock);
error0:
	usb_free_urb(mscDev->CurrentUrb);
	return ret;
}

/*
*******************************************************************************
*                     mscDevFree
*
* Description:
*
*
* Parameters:
*    mscDev  ： input. msc设备信息
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void mscDevFree(__mscDev_t *mscDev)
{
	__u8 err = 0;

	if (mscDev->ThreadSemi) {
		hal_sem_delete(mscDev->ThreadSemi);
	}

	if (mscDev->scan_lock) {
		hal_sem_delete(mscDev->scan_lock);
	}

	if (mscDev->DevLock) {
		hal_sem_delete(mscDev->DevLock);
	}

	if (mscDev->notify_complete) {
		hal_sem_delete(mscDev->notify_complete);
	}

	if (mscDev->UrbWait) {
		hal_sem_delete(mscDev->UrbWait);
	}

	if (mscDev->CtrlReq) {
		usb_buffer_free(mscDev->pusb_dev, sizeof(*mscDev->CtrlReq), (void *)(mscDev->CtrlReq), 0);
	}

	if (mscDev->iobuf) {
		usb_buffer_free(mscDev->pusb_dev, MSC_IOBUF_SIZE, (void *)(mscDev->iobuf), 0);
	}

	usb_free_urb(mscDev->CurrentUrb);
	/* Remove our private data from the interface */
	usb_set_intfdata(mscDev->pusb_intf, NULL);
	free_mscDev_id(mscDev->mscDevNo);
}

/*
*******************************************************************************
*                     SetMscDevState
*
* Description:
*
*
* Parameters:
*
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void SetMscDevState(__mscDev_t *mscDev, mscDev_state_t state)
{
	unsigned long cpu_sr = 0;
	// ENTER_CRITICAL(cup_sr);
	cpu_sr = hal_interrupt_disable_irqsave();
	mscDev->state = state;
	hal_interrupt_enable_irqrestore(cpu_sr);
	// EXIT_CRITICAL(cup_sr);
}

/*
*******************************************************************************
*                     mscDevOnline
*
* Description:
*
*
* Parameters:
*
*
* Return value:
*    真/假
*
* note:
*    无
*
*******************************************************************************
*/
unsigned int mscDevOnline(__mscDev_t *mscDev)
{
	return (mscDev->state == MSC_DEV_ONLINE);
}

/*
*******************************************************************************
*                     GetDeviceInfo
*
* Description:
*
*
* Parameters:
*
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void GetDeviceInfo(__mscDev_t *mscDev, unsigned int index)
{
	struct usb_interface_descriptor *idesc = &(mscDev->pusb_intf->cur_altsetting->desc);
	mscDev->SubClass = idesc->bInterfaceSubClass;
	mscDev->Protocol = idesc->bInterfaceProtocol;
	return;
}

/*
*******************************************************************************
*                     GetTransport
*
* Description:
*    获得msc设备的传输方式
*
* Parameters:
*    mscDev  ： input. msc设备信息
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static int GetTransport(__mscDev_t *mscDev)
{
	//	printf("%s %d %s Protocol:%x\n", __FILE__, __LINE__, __func__, mscDev->Protocol);
	switch (mscDev->Protocol) {
	case USBMSC_INTERFACE_PROTOCOL_BOT:
		mscDev->transport_name = "BulkOnly";
		mscDev->ResetRecovery = mscResetRecovery;
		mscDev->Transport = mscBoTransport;
		mscDev->StopTransport = mscBoStopTransport;
		break;

	default:
		hal_log_info("ERR: not support transport Protocol(%x)", mscDev->Protocol);
		mscDev->transport_name = "unkown";
		mscDev->ResetRecovery = NULL;
		mscDev->Transport = NULL;
		mscDev->StopTransport = NULL;
		return USB_ERR_UNKOWN_ERROR;
	}

	return USB_ERR_SUCCESS;
}

/*
*******************************************************************************
*                     GetTransport
*
* Description:
*    获得msc设备的传输方式
*
* Parameters:
*    mscDev  ： input. msc设备信息
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static int GetProtocol(__mscDev_t *mscDev)
{
	switch (mscDev->SubClass) {
	case USBMSC_SUBCLASS_RBC:
		mscDev->Protocol_name = "Reduced Block Commands (RBC)";
		mscDev->ProtocolHandler = mscScsiCommand;
		break;

	case USBMSC_SUBCLASS_8020:
		mscDev->Protocol_name = "8020i";
		mscDev->ProtocolHandler = mscATAPICommand;
		break;

	case USBMSC_SUBCLASS_QIC:
		mscDev->Protocol_name = "QIC-157";
		mscDev->ProtocolHandler = mscQIC157Command;
		break;

	case USBMSC_SUBCLASS_UFI:
		mscDev->Protocol_name = "Uniform Floppy Interface (UFI)";
		mscDev->ProtocolHandler = mscUFICommand;
		break;

	case USBMSC_SUBCLASS_8070:
		mscDev->Protocol_name = "8070i";
		mscDev->ProtocolHandler = mscQIC157Command;
		break;

	case USBMSC_SUBCLASS_SCSI:
		mscDev->Protocol_name = "Reduced Block Commands (RBC)";
		mscDev->ProtocolHandler = mscScsiCommand;
		break;

	default:
		hal_log_info("ERR: not support transport SubClass(%x)", mscDev->SubClass);
		return USB_ERR_UNKOWN_ERROR;
	}

	return USB_ERR_SUCCESS;
}

/*
*******************************************************************************
*                     CreatePipes
*
* Description:
*    创建传输管道
*
* Parameters:
*    mscDev  ： input. msc设备信息
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static int CreatePipes(__mscDev_t *mscDev)
{
	struct usb_host_interface *altsetting = mscDev->pusb_intf->cur_altsetting;
	struct usb_endpoint_descriptor *ep = NULL;
	struct usb_endpoint_descriptor *ep_in = NULL;
	struct usb_endpoint_descriptor *ep_out = NULL;
	struct usb_endpoint_descriptor *ep_int = NULL;
	int i = 0;

	/*
	 * Find the endpoints we need.
	 * We are expecting a minimum of 2 endpoints - in and out (bulk).
	 * An optional interrupt is OK (necessary for CBI protocol).
	 * We will ignore any others.
	 */
	for (i = 0; i < altsetting->desc.bNumEndpoints; i++) {
		ep = &altsetting->endpoint[i].desc;

		/* Is it a BULK endpoint? */
		if ((ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
			/* BULK in or out? */
			if (ep->bEndpointAddress & USB_DIR_IN) {
				ep_in = ep;
			} else {
				ep_out = ep;
			}
		}
		/* Is it an interrupt endpoint? */
		else if ((ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
			ep_int = ep;
		}
	}

	if (!ep_in || !ep_out || (mscDev->Protocol == USBMSC_INTERFACE_PROTOCOL_CBIT && !ep_int)) {
		hal_log_err("Endpoint sanity check failed! Rejecting dev.");
		return USB_ERR_UNKOWN_ERROR;
	}

	/* Calculate and store the pipe values */
	mscDev->CtrlOut = usb_sndctrlpipe(mscDev->pusb_dev, 0);
	mscDev->CtrlIn = usb_rcvctrlpipe(mscDev->pusb_dev, 0);
	mscDev->BulkOut = usb_sndbulkpipe(mscDev->pusb_dev,
					  ep_out->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	mscDev->BulkIn = usb_rcvbulkpipe(mscDev->pusb_dev,
					 ep_in->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);

	if (ep_int) {
		mscDev->IntIn = usb_rcvintpipe(mscDev->pusb_dev,
					       ep_int->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
		mscDev->EpInterval = ep_int->bInterval;
	}

	return USB_ERR_SUCCESS;
}

static void mscLun_Inquiry_free(__mscLun_t *mscLun)
{
	if (mscLun == NULL) {
		hal_log_err("ERR: input error");
		return;
	}

	if (mscLun->Inquiry == NULL) {
		hal_log_err("ERR: input error");
		return;
	}

#if ENABLE_DMA_ALLOC_COHERENT
	hal_free_coherent(mscLun->Inquiry);
#else
	hal_free_align(mscLun->Inquiry);
#endif
}

/*
*******************************************************************************
*                     mscDevAdd
*
* Description:
*
*
* Parameters:
*    mscDev  ： input. msc设备信息
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void mscDevAdd(__mscDev_t *mscDev)
{
	unsigned int i = 0;
	int ret = 0;

	if (mscDev == NULL) {
		printf("ERR: mscDevAdd input error");
		return;
	}

	//    printf("mscDev->MaxLun=%d", mscDev->MaxLun);

	/* probe lun */
	for (i = 0; i < mscDev->MaxLun; i++) {
		__mscLun_t *mscLun = NULL;
		mscLun = mscLun_alloc();

		if (mscLun == NULL) {
			hal_log_err("ERR: MscLun_alloc failed");
			return;
		}
#if ENABLE_DMA_ALLOC_COHERENT
		mscLun->Inquiry = hal_malloc_coherent(SCSI_MAX_INQUERY_LEN);
#else
		mscLun->Inquiry = hal_malloc_align(SCSI_MAX_INQUERY_LEN, CACHELINE_LEN);
#endif

		if (mscLun->Inquiry == NULL) {
			hal_log_err("ERR: mscLun->Inquiry_alloc failed");
			return;
		}

		memset(mscLun->Inquiry, 0, SCSI_MAX_INQUERY_LEN);
		mscLun->mscDev = mscDev;
		mscLun->LunNo = i;
		mscLun->DiskSubClass = mscDev->SubClass;
		mscLun->RemoveAble = 1; /* 默认就是可移动设备 */
		mscDev->Lun[i] = mscLun;
		hal_log_info("begin mscLunAdd\n");
		ret = mscLunAdd(mscLun);
		hal_log_info("end mscLunAdd\n");

		if (ret != USB_ERR_SUCCESS) {
			hal_log_err("ERR: probe Lun(%d) failed", i);
			continue;
		}

		/* 如果设备是非移动设备, 就delay */
		if (mscLun->RemoveAble == 0) {
			mscDev->SuspendTime = 10000; /* 10s */
		} else {
			mscDev->SuspendTime = 0;
		}

		usbh_disk_save_time(mscDev->SuspendTime);
	}

	return;
}

/*
*******************************************************************************
*                     mscDevDel
*
* Description:
*
*
* Parameters:
*    mscDev  ： input. msc设备信息
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void mscDevDel(__mscDev_t *mscDev)
{
	unsigned int i = 0;

	if (mscDev == NULL) {
		hal_log_err("ERR: mscDevDel input error");
		return;
	}

	/* remove lun */
	for (i = 0; i < mscDev->MaxLun; i++) {
		__mscLun_t *mscLun = NULL;
		mscLun = mscDev->Lun[i];

		if (mscLun == NULL) {
			hal_log_err("ERR: mscLun == NULL");
			return;
		}
		mscLunDel(mscLun);
		mscLun_Inquiry_free(mscLun);
		mscLun_free(mscLun);
		usbh_disk_del_time(mscDev->SuspendTime);
	}

	return;
}

/*
*******************************************************************************
*                     MediaChangeThread
*
* Description:
*    介质检测线程
*
* Parameters:
*    p_arg  ： input. 即mscDev, msc设备信息
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void MediaChangeThread(void *p_arg)
{
	__mscDev_t *mscDev = (__mscDev_t *)p_arg;
	hal_sem_post(mscDev->notify_complete);
	while (1) {
		unsigned int i = 0;
		//--<1>--杀死线程
		// TryTo//KillThreadSelf("MediaChangeThread");

		for (i = 0; i < mscDev->MaxLun; i++) {
			__mscLun_t *mscLun = NULL;
			mscLun = mscDev->Lun[i];

			if (mscLun == NULL) {
				hal_log_err("ERR: MediaChangeThread: mscLun is NULL");
				continue;
			}

			if (mscLun->MediaChange) {
				mscLun->MediaChange(mscLun);
			}
		}

		/* delay 1.25s */
		hal_msleep(1250);
	}
}

/*
*******************************************************************************
*                     mscDevScanThread
*
* Description:
*    mscDev扫描
*
* Parameters:
*    p_arg  ： input. 即mscDev, msc设备信息
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void mscDevScanThread(void *p_arg)
{
	__mscDev_t *mscDev = (__mscDev_t *)p_arg;
	mscDevAdd(mscDev);
	hal_sem_post(mscDev->notify_complete);

#ifdef CONFIG_KERNEL_FREERTOS  // melis exit auto
	/*safe exit*/
	while (1) {
		if (!hal_thread_stop(mscDev->DevScanThreadId))
			break;
		hal_msleep(3);
	}
#endif
}

/*
*******************************************************************************
*                     QueueCmnd
*
* Description:
*    发送命令到Lun
*
* Parameters:
*    mscLun  	： input. 命令对应的Lun
*    scsi_cmnd 	:  input. 命令
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
int mscDevQueueCmnd(__mscLun_t *mscLun, __ScsiCmnd_t *ScsiCmnd)
{
	__mscDev_t *mscDev = NULL;
	unsigned long sr = 0;
	int val = 0;

	if (ScsiCmnd == NULL) {
		hal_log_err("ERR: invalid argment");
		return USB_ERR_BAD_ARGUMENTS;
	}

	if (mscLun == NULL) {
		hal_log_err("ERR: MscTransport: input error, mscLun = 0x%x, ScsiCmnd = %x", mscLun, ScsiCmnd);
		ScsiCmnd->Result = USB_STOR_TRANSPORT_ERROR;
		return USB_ERR_BAD_ARGUMENTS;
	}

	// DMSG_MSC_TEST("mscDevQueueCmnd: cmnd = %x", ((__u8 *)(ScsiCmnd->cmnd.CommandBlock))[0]);
	mscDev = mscLun->mscDev;

	if (mscDev == NULL) {
		hal_log_err("ERR: MscTransport: input error, mscDev == NULL");
		ScsiCmnd->Result = USB_STOR_TRANSPORT_ERROR;
		return USB_ERR_BAD_ARGUMENTS;
	}

	/* if mscDev is Busy, then all cmd to cmd_list */
	if (!mscDevOnline(mscDev)) {
		hal_log_err("ERR: mscDevice is %s",
			    (mscDev->state == MSC_DEV_OFFLINE) ? "offline" : "died");
		ScsiCmnd->Result = USB_STOR_TRANSPORT_ERROR;
		return USB_ERR_IO_DEVICE_OFFLINE;
	}

	/* add command to cmd_list */
	sr = hal_interrupt_disable_irqsave();
	list_head_malloc_and_add((void *)ScsiCmnd, &(mscDev->cmd_list));
	hal_interrupt_enable_irqrestore(sr);
	/* wakeup mscDev main thread */
	if (!hal_sem_getvalue(mscDev->ThreadSemi, &val) && val == 0) {
		hal_sem_post(mscDev->ThreadSemi);
	}
	return USB_ERR_SUCCESS;
}

/*
*******************************************************************************
*                     mscCmndDone
*
* Description:
*    决策命令的去向，重发还是完成
*
* Parameters:
*    mscLun  		： input. 命令对应的Lun
*    scsi_cmnd 		:  input. 命令
* 	 TransStatus 	:  input. 传输状态
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void mscCmndDone(__mscDev_t *mscDev, __ScsiCmnd_t *ScsiCmnd, int TransStatus)
{
	unsigned int sr = 0;
	unsigned long flag;

	if (ScsiCmnd == NULL) {
		return;
	}

	if (mscDev->state != MSC_DEV_OFFLINE) {
		/* need retry? */
		if (TransStatus == USB_STOR_TRANSPORT_ERROR && (ScsiCmnd->retries < ScsiCmnd->allowed)) {
			if (mscGetDataTransportReason(ScsiCmnd->sc_lun, ScsiCmnd) == USB_ERR_COMMAND_NEED_RETRY) {
				ScsiCmnd->retries++;
				hal_log_err("ERR: do command(%x) failed, need retry, retries = %d, ScsiCmnd->allowed = %d",
					    ((__u8 *)(ScsiCmnd->cmnd.CommandBlock))[0], ScsiCmnd->retries, ScsiCmnd->allowed);
			} else {
				hal_log_err("ERR: mscCmndDone: media is not present");
			}
		}

		//DMSG_MSC_TEST("mscCmndDone: cmnd = %x", ((__u8 *)(ScsiCmnd->cmnd.CommandBlock))[0]);

		/* retries = allowed = 0 ? */
		if (ScsiCmnd->retries > ScsiCmnd->allowed) {
			hal_log_err("ERR: ScsiCmnd(%x) retry %d times, maybe device is died",
				    ((__u8 *)(ScsiCmnd->cmnd.CommandBlock))[0], ScsiCmnd->retries);
			/* 即使命令执行多次失败也不能认为设备坏了, 因为设备可能不支持这个命令 */
		}
	} else {
		hal_log_err("ERR: mscCmndDone: device is offline");
	}

	/* Done command */
	// ENTER_CRITICAL(sr);
	flag = hal_interrupt_disable_irqsave();
	list_head_unlink_and_del(mscDev->cmd_list.next);
	ScsiCmnd->Result = TransStatus;
	hal_interrupt_enable_irqrestore(flag);
	// EXIT_CRITICAL(sr);

	if (ScsiCmnd->Done) {
		ScsiCmnd->Done(ScsiCmnd);
	}

	return;
}

/*
*******************************************************************************
*                     mscMainThread
*
* Description:
*    主线程，完成命令的传输
*
* Parameters:
*    p_arg  ： input. 即mscDev, msc设备信息
*
* Return value:
*    无
*
* note:
*    无
*
*******************************************************************************
*/
static void mscMainThread(void *p_arg)
{
	__mscDev_t *mscDev = (__mscDev_t *)p_arg;
	__ScsiCmnd_t *ScsiCmnd = NULL;
	int TransStatus = 0;
	unsigned long sr = 0;
	hal_sem_post(mscDev->notify_complete);

	while (1) {
		//--<1>--杀死线程
		// TryTo//KillThreadSelf("mscMainThread");
		/* sleep */
		// UsbThreadSleep(mscDev->ThreadSemi);
		// hal_thread_stop(mscDev->MainThreadId);
		hal_sem_wait(mscDev->ThreadSemi);
		/* cmd_list is empty?
		 * mscDevQueueCmnd把cmd加到cmd_list，刚好遇到mscMainTread处理完命令，这样
		 * for循环检测到cmd_list不空就会去执行下一条命令。执行完毕后cmd_list空了。
		 * 此时UsbThreadWakeUp唤醒mscMainTread，但是cmd_list已经空了。
		 */
		if (usb_list_empty(&(mscDev->cmd_list))) {
			hal_log_debug("Wrn: mscDev cmd_list is empty");
			continue;
		}

		for (; !usb_list_empty(&(mscDev->cmd_list));) {
			/* Get command */
			ScsiCmnd = (__ScsiCmnd_t *)mscDev->cmd_list.next->data;

			if (ScsiCmnd == NULL) {
				hal_log_err("Wrn: mscDev cmd_list is not empty, but ScsiCmnd is NULL");
				// ENTER_CRITICAL(sr);
				sr = hal_interrupt_disable_irqsave();
				list_head_unlink_and_del(mscDev->cmd_list.next);
				hal_interrupt_enable_irqrestore(sr);
				// EXIT_CRITICAL(sr);
				continue;
			}

			/* transport */
			if (mscDev->ProtocolHandler) {
				TransStatus = mscDev->ProtocolHandler(mscDev, ScsiCmnd);
			} else {
				hal_log_err("ERR: mscDev ProtocolHandler is NULL");
				ScsiCmnd->Result = USB_STOR_TRANSPORT_ERROR;
				TransStatus = USB_STOR_TRANSPORT_GOOD;
			}

			/* done cmnd */
			mscCmndDone(mscDev, ScsiCmnd, TransStatus);
		}
	}
}

/*
*******************************************************************************
*                     mscDevProbe
*
* Description:
*    创建传输管道
*
* Parameters:
*    intf  		： input. USB接口信息
*    table_item :  input. 匹配表
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static int32_t mscDevProbe(struct usb_interface *intf,
			   const struct usb_device_id *table_item)
{
	const s32 match_table_index = table_item - msc_match_table;
	__mscDev_t *mscDev = NULL;
	int ret = 0;
	unsigned char err = 0;

	hal_log_info("mscDevProbe begin");
	if (intf == NULL || table_item == NULL) {
		hal_log_err("ERR: mscDevProbe: input error");
		return -1;
	}

	//----------------------------------------------------------------
	//   初始化mscDev
	//----------------------------------------------------------------

	/* 初始化一个mscDev */
	mscDev = (__mscDev_t *)hal_malloc(sizeof(__mscDev_t));

	if (mscDev == NULL) {
		hal_log_err("ERR: hal_malloc failed");
		ret = -1;
		goto error0;
	}

	memset(mscDev, 0, sizeof(__mscDev_t));
	ret = mscDevInit(mscDev, intf);

	if (ret != USB_ERR_SUCCESS) {
		hal_log_err("ERR: mscDevInit failed");
		ret = -1;
		goto error1;
	}

	GetDeviceInfo(mscDev, match_table_index);
	/* 获得SubClass */
	ret = GetProtocol(mscDev);
	ret = GetProtocol(mscDev);

	if (ret != USB_ERR_SUCCESS) {
		hal_log_err("ERR: GetProtocol failed");
		ret = -1;
		goto error2;
	}

	/* 获得Transport */
	ret = GetTransport(mscDev);

	if (ret != USB_ERR_SUCCESS) {
		hal_log_err("ERR: GetTransport failed");
		ret = -1;
		goto error3;
	}

	/* 创建传输管道 */
	ret = CreatePipes(mscDev);
	if (ret != USB_ERR_SUCCESS) {
		hal_log_err("ERR: CreatePipes failed");
		ret = -1;
		goto error4;
	}

	//----------------------------------------------------------------
	//   识别mscDev
	//----------------------------------------------------------------
	SetMscDevState(mscDev, MSC_DEV_ONLINE);
	/* 获得MaxLun */
	mscDev->MaxLun = mscGetMaxLun(mscDev) + 1;

	if (mscDev->MaxLun > MSC_MAX_LUN) {
		hal_log_err("ERR: MaxLun = %d, mscDrv only support %d Lun", mscDev->MaxLun, MSC_MAX_LUN);
		mscDev->MaxLun = MSC_MAX_LUN;
	}

	/* msc main thread */
	mscDev->MainThreadId = hal_thread_run((void *)mscMainThread,
					   (void *)mscDev,
					   "MscMainThread");

	if (IS_ERR((unsigned long)mscDev->MainThreadId)) {
		hal_log_err("ERR: create MainThreadId failed");
		ret = -1;
		goto error5;
	}

	hal_sem_wait(mscDev->notify_complete);
	/* mscDev scan thread */
	mscDev->DevScanThreadId = hal_thread_run((void *)mscDevScanThread,
					      (void *)mscDev,
					      "mscDevScanThread");

	if (IS_ERR((unsigned long)mscDev->DevScanThreadId)) {
		hal_log_err("ERR: create mscDevScanThread failed");
		ret = -1;
		goto error6;
	}

	hal_sem_wait(mscDev->notify_complete);
	hal_log_info("mscDevScanThread end...");
	/* media change thread */
	mscDev->MediaChangeId = hal_thread_run((void *)MediaChangeThread,
					    (void *)mscDev,
					    "MediaChangeThread");

	if (IS_ERR((unsigned long)mscDev->MediaChangeId)) {
		hal_log_err("ERR: create MediaChangeId failed");
		ret = -1;
		goto error7;
	}

	hal_sem_wait(mscDev->notify_complete);
	return 0;
error7:
	hal_thread_stop(mscDev->DevScanThreadId);
error6:
	hal_thread_stop(mscDev->MainThreadId);
error5:
error4:
error3:
error2:
	mscDevFree(mscDev);
error1:
	hal_free(mscDev);
error0:
	hal_log_err("ERR: USB Mass Storage Driver probe failed");
	return ret;
}

/*
*******************************************************************************
*                     mscDevSuspend
*
* Description:
*    挂起设备
*
* Parameters:
*    intf  ： input. USB接口信息
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static int32_t mscDevSuspend(struct usb_interface *intf)
{
	__mscDev_t *mscDev = NULL;

	if (intf == NULL) {
		hal_log_err("ERR: mscDevRemove: input error");
		return -1;
	}

	mscDev = (__mscDev_t *)usb_get_intfdata(intf);

	if (mscDev == NULL) {
		hal_log_err("ERR: mscDev = NULL");
		return -1;
	}

	/* suspend interface */
	/* delay for suspend successful */
	hal_log_info("mscDevSuspend: SuspendTime = %dms", mscDev->SuspendTime);
	//  hal_msleep(mscDev->SuspendTime);
	return 0;
}

/*
*******************************************************************************
*                     mscDevRemove
*
* Description:
*    msc设备移除
*
* Parameters:
*    intf  ： input. USB接口信息
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static void mscDevRemove(struct usb_interface *intf)
{
	__mscDev_t *mscDev = NULL;

	if (intf == NULL) {
		hal_log_err("ERR: mscDevRemove: input error");
		return;
	}

	mscDev = (__mscDev_t *)usb_get_intfdata(intf);

	if (mscDev == NULL) {
		hal_log_err("ERR: mscDev = NULL");
		return;
	}

	SetMscDevState(mscDev, MSC_DEV_OFFLINE);
	hal_log_info("mscDevRemove: SetMscDevState MSC_DEV_OFFLINE");
	/* 停止当前传输 */
	mscDev->StopTransport(mscDev);
	/* kill media change thrad */
	hal_thread_stop(mscDev->MediaChangeId);
	/* 断开mscDev上面的所有的Lun */
	mscDevDel(mscDev);
	/* kill main thrad */
	// hal_thread_stop(mscDev->MainThreadId, mscDev->ThreadSemi);
	hal_thread_stop(mscDev->MainThreadId);
	if (mscDev->ThreadSemi) {
		hal_sem_delete(mscDev->ThreadSemi);
		mscDev->ThreadSemi = NULL;
	}
	/* 释放mscDev资源 */
	mscDevFree(mscDev);
	hal_free(mscDev);
	hal_log_info("mscDevRemove complete");
	return;
}

/*
*******************************************************************************
*                     mscDrv_init
*
* Description:
*    msc驱动初始化
*
* Parameters:
*    drv  ： input. msc驱动
*
* Return value:
*    0	：成功
*	!0	：失败
*
* note:
*    无
*
*******************************************************************************
*/
static int mscDrvInit(struct usb_driver *drv)
{
	if (drv == NULL) {
		hal_log_err("ERR: mscDrv_init: input error");
		return -1;
	}

	USB_INIT_LIST_HEAD(&(drv->virt_dev_list));
	drv->func_drv_name = MSC_DRV_NAME;
	drv->func_drv_auther = MSC_DRV_AUTHOR;
	drv->probe = mscDevProbe;
	drv->disconnect = mscDevRemove;
	drv->suspend = mscDevSuspend;
	drv->resume = NULL;
	drv->match_table = msc_match_table;
	return 0;
}

/*
*******************************************************************************
*                     scsi_bus_drv_reg
*
* Description:
*    往scsi总线注册驱动
*
* Parameters:
*    drv : input.  驱动
*
* Return value:
*    EPDK_OK / EPDK_FAIL
*
* note:
*    无
*
*******************************************************************************
*/
int mscInit(void)
{
	int ret = 0;

	memset(&mscDrv, 0, sizeof(struct usb_driver));
	mscDev_id_array = 0;
	init_usbh_buff_manager();
	usbh_disk_time_init();
	/* 根据客户的需求，要求反映disk的状态, 这里就是管理disk的状态 */
	usbh_disk_info_reg();

	if (mscDrvInit(&mscDrv) != 0) {
		hal_log_err("ERR: mscDrv_init failed");
		return -1;
	}

	ret = usb_host_func_drv_reg(&mscDrv);
	if (ret != 0) {
		hal_log_err("ERR: mscInit: Reg driver %s failed", mscDrv.func_drv_name);
		return -1;
	}
	return 0;
}

/*
*******************************************************************************
*                     scsi_bus_drv_reg
*
* Description:
*    往scsi总线注册驱动
*
* Parameters:
*    drv : input.  驱动
*
* Return value:
*    EPDK_OK / EPDK_FAIL
*
* note:
*    无
*
*******************************************************************************
*/
int mscExit(void)
{
	int ret = 0;
	ret = usb_host_func_drv_unreg(&mscDrv);

	if (ret != 0) {
		hal_log_err("ERR: mscExit: UnReg driver %s failed", mscDrv.func_drv_name);
		return -1;
	}

	usbh_disk_info_unreg();
	exit_usbh_buff_manager();
	usbh_disk_time_exit();
	return 0;
}
