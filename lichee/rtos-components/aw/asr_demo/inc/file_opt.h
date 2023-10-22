#ifndef __FILE_OPT_H__
#define __FILE_OPT_H__

int load_data_from_file(const char *file_path, unsigned char *data, int limit_size);
int save_data_to_file(const char *file_path, const unsigned char *data, int size);

#endif /* __FILE_OPT_H__ */