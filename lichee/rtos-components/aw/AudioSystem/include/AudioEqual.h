
#ifndef __AUDIOEQUAL_H
#define __AUDIOEQUAL_H


#include <hal_queue.h>

#define AW_EQUAL_CONFIG_NUMBER 2

struct AudioEqualProcessItem {
	void *data;
	int len;
};

struct AudioEqual {
	/* set from ae */
	hal_queue_t ae_queue;
	uint8_t keep_alive;
	int verbose_level;
	void *priv;

};

typedef struct AudioEqual tAudioEqual;

int AudioEqualThreadAddAE(tAudioEqual *ae);

int AudioEqualThreadRemoveAE(tAudioEqual *ae);

int AudioEqualThreadInit(void);

int AudioEqualThreadDeInit(void);


#endif
