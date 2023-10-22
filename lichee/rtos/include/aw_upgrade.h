#ifndef __AW_UPGRADE_H__
#define __AW_UPGRADE_H__

#define UPGRADE_SLICE_RETRY (2)
#define UPGRADE_SLICE_ACTION_WRITE (1)
#define UPGRADE_SLICE_ACTION_VERIFY (2)
#define CHUNK (16384)

#define UPGRADE_FROM_FILE (1)
#define UPGRADE_FROM_BUFFER (2)

#define DEFAULT_SOURCE_FILE "/data/update/update.bin"

#ifndef CONFIG_COMPONENTS_AW_OTA_V2
int upgrade_slice(char *source_file, char *source_buffer, char *dest_file, int offset, int slice_size);
int get_rtos_to_upgrade(char *target_rtos);
int aw_upgrade_slice(uint8_t* target, uint8_t* buffer, uint32_t offset, uint32_t size, uint32_t flag);
int aw_upgrade_start(uint32_t flag);
int aw_upgrade_end(uint32_t flag);

#else
int ota_upgrade_slice(uint8_t* target, uint8_t* buffer, uint32_t offset, uint32_t size, uint32_t flag);
int ota_init(void);
int ota_end(void);
#endif
#endif
