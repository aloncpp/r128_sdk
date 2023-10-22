#ifndef __AW_OTA_H__
#define __AW_OTA_H__

void otaprogressBar(long cur_size, long total_size);
int load_para(char* para);
int update_from_flash(char* path);
int save_para(char* para);
int clear_para_after_end(void);

#endif
