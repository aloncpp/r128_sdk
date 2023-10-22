#ifndef _ENV_H_
#define _ENV_H_

#ifndef CONFIG_COMPONENTS_ENV_FILE_SUPPORT
#define O_RDONLY    (00000000)
#define O_WRONLY    (00000001)
#define O_RDWR      (00000002)
#endif

#define CUR_ENVSIZE   (CONFIG_COMPONENTS_ENV_SIZE)
#define ENV_SIZE      usable_envsize

struct env_image_single {
	uint32_t crc;		/* CRC32 over data bytes    */
	char data[];
};

struct env_image_redundant {
	uint32_t crc;		/* CRC32 over data bytes    */
	unsigned char flags;	/* active or obsolete */
	char data[];
};

enum flag_scheme {
	FLAG_NONE,
	FLAG_BOOLEAN,
	FLAG_INCREMENTAL,
};

struct environment {
	void *image;
	uint32_t *crc;
	unsigned char *flags;
	char *data;
	enum flag_scheme flag_scheme;
};

static int flash_io(int mode);

#endif

