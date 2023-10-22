#ifndef __DATA_SAVE_H__
#define __DATA_SAVE_H__

void *data_save_create(const char *name, const char *file_path, int port);
void data_save_destroy(void *_hdl);
int data_save_request(void *_hdl, void *data, int size, int timeout_ms);
int data_save_flush(void *_hdl, int timeout_ms);

#endif /*__DATA_SAVE_H__*/