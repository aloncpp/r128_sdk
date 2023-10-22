/*
*********************************************************************************************************
*                                                    MELIS
*                                    the Easy Portable/Player Develop Kits
*                                                 WEBCAM Driver
*
*                                    (c) Copyright 2012-2016, Kingvan.Tong China
*                                             All Rights Reserved
*
* File    : drv_webcam.h
* By      : Kingvan
* Version : v1.0
* Date    : 2012-9-27
* Descript:
* Update  : date                auther         ver     notes
*           2012-9-27 11:09:13  Kingvan.Tong   2.0     build the file.
*********************************************************************************************************
*/

#include "drv_webcam_i.h"

extern __s32 WEBCAM_DEV_NODE_Init_Part1(void);	// WEBCAM_DEV_NODE的初始化的第一部分
// extern __s32 WEBCAM_DEV_NODE_Init_Part2(__u32 webcam_channle,__u32 system);
extern __s32 WEBCAM_DEV_NODE_Exit(void);

#ifdef CONFIG_OS_MELIS
extern __hdle DEV_WEBCAM_Open(void *open_arg, __u32 mode);
extern __s32 DEV_WEBCAM_Close(__hdle hwebcam);
extern __u32 DEV_WEBCAM_Read(void *pdata, __u32 size, __u32 n, __hdle hPower);
extern __u32 DEV_WEBCAM_Write(const void *pdata, __u32 size, __u32 n, __hdle hPower);
extern __s32 DEV_WEBCAM_Ioctrl(__hdle hPower, __u32 cmd, __s32 aux, void *pbuffer);
#elif defined(CONFIG_KERNEL_FREERTOS)
extern int DEV_WEBCAM_Open(struct devfs_node *node);
extern int DEV_WEBCAM_Close(struct devfs_node *node);
extern ssize_t DEV_WEBCAM_Read(struct devfs_node *node, uint32_t addr, uint32_t size, void *data);
extern ssize_t DEV_WEBCAM_Write(struct devfs_node *node, uint32_t addr, uint32_t size, const void *data);
extern int DEV_WEBCAM_Ioctrl(struct devfs_node *node, int cmd, void *pbuffer);
#endif
static __webcam_drv_t webcam_drv;

__s32 DRV_WEBCAM_MInit(UVCDev_t *UVCDev)
{
#ifdef CONFIG_OS_MELIS
	extern void *esDEV_DevReg(const char *classname, const char *name,
				  struct devfs_node *pDevOp, void *pOpenArg);
#endif
	hal_log_info("DRV_WEBCAM_MInit\n");

	// WEBCAM设备节点的初始化，此时这些设备均未注册
	memset(&webcam_drv, 0, sizeof(__webcam_drv_t));
	if (EPDK_OK == WEBCAM_DEV_NODE_Init_Part1()) {
#ifdef CONFIG_OS_MELIS
		webcam_drv.webcam_dev_entry.Open = DEV_WEBCAM_Open;
		webcam_drv.webcam_dev_entry.Close = DEV_WEBCAM_Close;
		webcam_drv.webcam_dev_entry.Read = DEV_WEBCAM_Read;
		webcam_drv.webcam_dev_entry.Write = DEV_WEBCAM_Write;
		webcam_drv.webcam_dev_entry.Ioctl = DEV_WEBCAM_Ioctrl;
#elif defined(CONFIG_KERNEL_FREERTOS)
		webcam_drv.webcam_dev_entry.open = DEV_WEBCAM_Open;
		webcam_drv.webcam_dev_entry.close = DEV_WEBCAM_Close;
		webcam_drv.webcam_dev_entry.read = DEV_WEBCAM_Read;
		webcam_drv.webcam_dev_entry.write = DEV_WEBCAM_Write;
		webcam_drv.webcam_dev_entry.ioctl = DEV_WEBCAM_Ioctrl;
		webcam_drv.webcam_dev_entry.name = "video";
		webcam_drv.webcam_dev_entry.alias = "WEBCAM";
		webcam_drv.webcam_dev_entry.private = &webcam_drv;
		webcam_drv.webcam_dev_entry.size = 1;
#endif
	} else {
		goto _err0;
	}
	webcam_drv.used = 0;

	if (webcam_drv.used) {
		hal_log_info("webcam already used!\n");
		return EPDK_FAIL;
	}
#ifdef CONFIG_OS_MELIS
	webcam_drv.hReg_WebcamDevHdl = (__hdle)esDEV_DevReg("UVC", "WEBCAM", &webcam_drv.webcam_dev_entry, 0);
#elif defined(CONFIG_KERNEL_FREERTOS)
	if (devfs_add_node(&(webcam_drv.webcam_dev_entry))) {
			hal_log_err("ERR: webcam_drv device register failed.");
			return EPDK_FAIL;
	}
	webcam_drv.hReg_WebcamDevHdl = &(webcam_drv.webcam_dev_entry);
	webcam_drv.UVCDev = UVCDev;
#endif
	if (!webcam_drv.hReg_WebcamDevHdl) {
		hal_log_info("user webcam registered Error!\n");
		return EPDK_FAIL;
	}
	hal_log_info("drv webcam reg ok!\n");
//	ret = WEBCAM_DEV_NODE_Init_Part2(aux,(__s32)pbuffer);
//
//	if(ret != EPDK_OK)
//	{
//		return ret;
//	}
//	webcam_drv.used = 1;
	hal_log_info("webcam drv plugin ok!\n");
	return EPDK_OK;
_err0:
	WEBCAM_DEV_NODE_Exit();
	memset(&webcam_drv, 0, sizeof(__webcam_drv_t));
	return EPDK_FAIL;
}

__s32 DRV_WEBCAM_MExit(void)
{
	hal_log_info("DRV_WEBCAM_MExit\n");
	WEBCAM_DEV_NODE_Exit();
	memset(&webcam_drv, 0, sizeof(__webcam_drv_t));
	return EPDK_OK;
}

//__mp *DRV_WEBCAM_MOpen(__u32 mid, __u32 mod)
//{
//	hal_log_info("DRV_WEBCAM_MOpen\n");
//	webcam_drv.mid = mid;

//	return (__mp *)&webcam_drv;
//}

//__s32 DRV_WEBCAM_MClose(__mp *mp)
//{
//	hal_log_info("DRV_WEBCAM_MClose\n");
//
//	return EPDK_OK;
//}

//__u32 DRV_WEBCAM_MRead(void *pdata, __u32 size, __u32 n, __mp *mp)
//{
//    return EPDK_OK;
//}

//__u32 DRV_WEBCAM_MWrite(const void *pdata, __u32 size, __u32 n, __mp *mp)
//{
//    return EPDK_OK;
//}

//__s32 DRV_WEBCAM_MIoctrl(__mp *mp, __u32 cmd, __s32 aux, void *pbuffer)
//{
//    __s32   ret;

//	switch(cmd)
//    {
//        case DRV_CMD_PLUGIN:
//        {
//			if(webcam_drv.used)
//            {
//                hal_log_info("webcam already used!\n");
//                return EPDK_FAIL;
//            }
//            webcam_drv.hReg_WebcamDevHdl = esDEV_DevReg("UVC", "WEBCAM", &webcam_drv.webcam_dev_entry, 0);
//        	if(!webcam_drv.hReg_WebcamDevHdl)
//        	{
//                hal_log_info("user webcam registered Error!\n");
//        	    return EPDK_FAIL;
//        	}
//            hal_log_info("drv webcam reg ok!\n");
//
//            ret = WEBCAM_DEV_NODE_Init_Part2(aux,(__s32)pbuffer);
//
//            if(ret != EPDK_OK)
//            {
//                return ret;
//            }
//        	webcam_drv.used = 1;
//
//        	hal_log_info("webcam drv plugin ok!\n");
//
//        	return EPDK_OK;
//        }

//        case DRV_CMD_PLUGOUT:
//        {
//			hal_log_info("webcam DRV_CMD_PLUGOUT!\n");
//			if(webcam_drv.used == 1)
//            {
//            	__s32 ret ;
//                if(!webcam_drv.hReg_WebcamDevHdl)
//                {
//                    hal_log_info("Dev not plugin!\n");
//                    return EPDK_FAIL;
//                }
//
//                ret = esDEV_DevUnreg(webcam_drv.hReg_WebcamDevHdl);
//				hal_log_info("ret=%d\n",ret);
//                webcam_drv.hReg_WebcamDevHdl = NULL;
//                WEBCAM_DEV_NODE_Exit();
//            }
//
//            webcam_drv.used = 0;
//            return EPDK_OK;
//        }

//        case DRV_CMD_GET_STATUS:
//        {
//			hal_log_info("DRV_CMD_GET_STATUS\n");
//			if(webcam_drv.used)
//            {
//                return DRV_STA_BUSY;
//            }
//            else
//            {
//                return DRV_STA_FREE;
//            }
//        }
//		default:
//            return EPDK_FAIL;
//    }
//}
