
#ifndef MEMLEAK_H
#define MEMLEAK_H

#ifdef __cplusplus
extern "C"{
#endif

void memleak_detect_start(int num, char * thread_name, ...);
void memleak_detect_stop();

#ifdef __cplusplus
}
#endif

#endif  /*MEMLEAK_H*/
