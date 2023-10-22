
#ifndef _PM_DEBUG_H_
#define _PM_DEBUG_H_

#include "stdio.h"


#ifdef CONFIG_COMPONENTS_PM_CORE_M33
#define CORE_NAME  "m33"
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_DSP
#define CORE_NAME  "dsp"
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_RISCV
#define CORE_NAME  "rv."
#endif

enum {
	PM_DEBUG_ID_AMPRPC = 0,
	PM_DEBUG_ID_RISCV,
	PM_DEBUG_ID_DSP,
	PM_DEBUG_ID_M33,

	PM_DEBUG_ID_BASE,
	PM_DEBUG_ID_WAKELOCK,
	PM_DEBUG_ID_SUSPEND,
	PM_DEBUG_ID_TASK,

	PM_DEBUG_ID_NOTIFY,
	PM_DEBUG_ID_SUBSYS,
	PM_DEBUG_ID_DEVOPS,
	PM_DEBUG_ID_PLATOPS,

	PM_DEBUG_ID_WAKESRC,
	PM_DEBUG_ID_WAKERES,
	PM_DEBUG_ID_WAKECNT,
	PM_DEBUG_ID_SYSCORE,

	PM_DEBUG_ID_OTHERS,
};

#define PM_DEBUG_INVALID	(0)

#define PM_DEBUG_AMPRPC		(0x1 << PM_DEBUG_ID_AMPRPC)
#define PM_DEBUG_RISCV		(0x1 << PM_DEBUG_ID_RISCV)
#define PM_DEBUG_DSP		(0x1 << PM_DEBUG_ID_DSP)
#define PM_DEBUG_M33		(0x1 << PM_DEBUG_ID_M33)
#define PM_DEBUG_BASE		(0x1 << PM_DEBUG_ID_BASE)
#define PM_DEBUG_WAKELOCK	(0x1 << PM_DEBUG_ID_WAKELOCK)
#define PM_DEBUG_SUSPEND	(0x1 << PM_DEBUG_ID_SUSPEND)
#define PM_DEBUG_TASK		(0x1 << PM_DEBUG_ID_TASK)
#define PM_DEBUG_NOTIFY		(0x1 << PM_DEBUG_ID_NOTIFY)
#define PM_DEBUG_SUBSYS		(0x1 << PM_DEBUG_ID_SUBSYS)
#define PM_DEBUG_DEVOPS		(0x1 << PM_DEBUG_ID_DEVOPS)
#define PM_DEBUG_PLATOPS	(0x1 << PM_DEBUG_ID_PLATOPS)
#define PM_DEBUG_WAKESRC	(0x1 << PM_DEBUG_ID_WAKESRC)
#define PM_DEBUG_WAKERES	(0x1 << PM_DEBUG_ID_WAKERES)
#define PM_DEBUG_WAKECNT	(0x1 << PM_DEBUG_ID_WAKECNT)
#define PM_DEBUG_SYSCORE	(0x1 << PM_DEBUG_ID_SYSCORE)
#define PM_DEBUG_OTHERS		(0x1 << PM_DEBUG_ID_OTHERS)

#ifndef PM_DEBUG_MODULE
#define PM_DEBUG_MODULE		PM_DEBUG_OTHERS
#endif

#ifndef PM_DEBUG_ENABLE
#define PM_DEBUG_ENABLE		0xffffffffU
#endif


#define PM_DEBUG_ERR		0x1
#define PM_DEBUG_LOG		0x2
#define PM_DEBUG_WARN		0x4
#define PM_DEBUG_INF		0x8
#define PM_DEBUG_DBG		0x10
#define PM_DEBUG_ABORT		0x20
#define PM_DEBUG_TRACE		0x40

#ifndef PM_DEBUG_MASK
#define PM_DEBUG_MASK		0xffffff67U
#endif

#define pm_raw(fmt, arg...)	do { if (PM_DEBUG_ENABLE & PM_DEBUG_MODULE) printf(fmt, ##arg); } while(0)

#if (PM_DEBUG_MASK & PM_DEBUG_ERR)
#define pm_err(fmt,arg...)  pm_raw("[E(" CORE_NAME ")] " fmt, ##arg)
#define pm_invalid()\
	pm_err("args is invild, at %s:%d.\n", __func__, __LINE__);
#else
#define pm_err(fmt,arg...)  do {} while(0)
#define pm_invalid()        do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_LOG)
#define pm_log(fmt,arg...)  pm_raw("[L(" CORE_NAME ")] " fmt, ##arg)
#else
#define pm_log(fmt,arg...)  do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_WARN)
#define pm_warn(fmt,arg...)  pm_raw("[W(" CORE_NAME ")] " fmt, ##arg)
#define pm_semapbusy(_x)\
	pm_warn("Task Semap" #_x "timeout, at %s:%d.\n", __func__, __LINE__);
#else
#define pm_warn(fmt,arg...)  do {} while(0)
#define pm_semapbusy(_x)	do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_INF)
#define pm_inf(fmt,arg...)  pm_raw("[I(" CORE_NAME ")] " fmt, ##arg)
#else
#define pm_inf(fmt,arg...)  do {} while(0)
#endif


#if (PM_DEBUG_MASK & PM_DEBUG_DBG)
#define pm_dbg(fmt,arg...)  pm_raw("[D(" CORE_NAME ")] " fmt, ##arg)

#define pm_ser_trace()\
	pm_raw("[T(ser)]: %s().\n", __func__);
#define pm_ser_trace1(_x1)\
	pm_raw("[T(ser)]: %s(%d).\n", __func__, _x1);
#define pm_ser_trace2(_x1, _x2)\
	pm_raw("[T(ser)]: %s(%d, %d).\n", __func__, _x1, _x2);
#define pm_ser_trace3(_x1, _x2, _x3)\
	pm_raw("[T(ser)]: %s(%d, %d, %d).\n", __func__, _x1, _x2, _x3);

#define pm_stub_trace()\
	pm_raw("[T(stub)]: %s().\n", __func__);
#define pm_stub_trace1(_x1)\
	pm_raw("[T(stub)]: %s(%d).\n", __func__, _x1);
#define pm_stub_trace2(_x1, _x2)\
	pm_raw("[T(stub)]: %s(%d, %d).\n", __func__, _x1, _x2);
#define pm_stub_trace3(_x1, _x2, _x3)\
	pm_raw("[T(stub)]: %s(%d, %d, %d).\n", __func__, _x1, _x2, _x3);
#else
#define pm_dbg(fmt,arg...)  do {} while(0)

#define pm_ser_trace()			do {} while(0)
#define pm_ser_trace1(_x1)		do {} while(0)
#define pm_ser_trace2(_x1, _x2)		do {} while(0)
#define pm_ser_trace3(_x1, _x2, _x3)	do {} while(0)

#define pm_stub_trace()			do {} while(0)
#define pm_stub_trace1(_x1)		do {} while(0)
#define pm_stub_trace2(_x1, _x2)	do {} while(0)
#define pm_stub_trace3(_x1, _x2, _x3)	do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_ABORT)
#define pm_abort(_cont)     do { if (_cont) {pm_raw("[PM_ABORT(" CORE_NAME ")] at %s:%d\n", __func__, __LINE__); while (1);} } while(0)
#else
#define pm_abort(_cont)     do {} while(0)
#endif

#if (PM_DEBUG_MASK & PM_DEBUG_TRACE)
#define pm_trace(fmt, arg...)\
	pm_raw("[T(" CORE_NAME ")] at %s:%d" fmt, __func__, __LINE__, ##arg)

#define pm_trace_func(fmt, arg...)\
	pm_raw("[T(" CORE_NAME ")] call %s(" fmt ")" "\n", __func__, ##arg)

#define pm_trace_ret(_func, _ret)\
	pm_raw("[T(" CORE_NAME ")] call %s return %d(0x%x)" "\n", #_func, _ret, _ret)

#define pm_trace_info(_str1)\
	pm_trace(" %s\n", (_str1)?(_str1):"null")

#define pm_devops_start_trace(_x1)\
	pm_raw("[T("CORE_NAME")(devops)]: calling %s ...\n", _x1);
#define pm_devops_end_trace(_x1, _x2, _x3)\
	pm_raw("[T("CORE_NAME")(devops)]: call %s return %d after %" PRIu64 " usec.\n", _x1, _x2, _x3);

#define pm_stage(fmt, arg...)	do { printf("[STAGE(" CORE_NAME ")] PM: " fmt, ##arg); } while(0)
#else
#define pm_trace(fmt, arg...)		do {} while(0)
#define pm_trace_func(fmt, arg...)	do {} while(0)
#define pm_trace_ret(_func, _ret)	do {} while(0)
#define pm_trace_info(_str1)		do {} while(0)

#define pm_devops_start_trace(_x1)		do {} while(0)
#define pm_devops_end_trace(_x1, _x2, _x3)	do {} while(0)
#define pm_stage(fmt, arg...)			do {} while(0)
#endif

#endif /* _PM_DEBUG_H_ */


