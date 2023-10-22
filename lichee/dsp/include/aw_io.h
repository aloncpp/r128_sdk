#ifndef __AW_IO_H__
#define __AW_IO_H__

#if __cplusplus
extern "C" {
#endif

#undef readb
#undef readw
#undef readl
#undef writeb
#undef writew
#undef writel

#define readb(addr)	(*((volatile unsigned char  *)(addr)))
#define readw(addr)	(*((volatile unsigned short *)(addr)))
#define readl(addr)	(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))


#if __cplusplus
}
#endif

#endif /* __AW_IO_H__ */
