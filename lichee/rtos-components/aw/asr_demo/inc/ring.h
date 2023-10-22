#ifndef __RING_H__
#define __RING_H__

struct ring_debug_info_t {
    int port;
    const char *file_path;
};

void *ring_create(unsigned int size, const char *name, struct ring_debug_info_t *info);
void ring_destroy(void *_hdl);
int ring_send_data(void *_hdl, void *_data, int _size);
int ring_recv_data(void *_hdl, void *_data, int _size);
int ring_recv_drop(void *_hdl);
int ring_recv_done(void *_hdl);

#endif /*__RING_H__*/