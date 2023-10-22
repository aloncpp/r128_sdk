#ifndef __VSPRINTF_H_
#define __VSPRINTF_H_

int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);

#endif
