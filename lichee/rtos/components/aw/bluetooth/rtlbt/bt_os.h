#ifndef __BT_OS_H__
#define __BT_OS_H__

typedef enum {
    BTOS_OK           = 0,    /* success */
    BTOS_FAIL         = -1,   /* general failure */
    BTOS_E_NOMEM      = -2,   /* out of memory */
    BTOS_E_PARAM      = -3,   /* invalid parameter */
    BTOS_E_TIMEOUT    = -4,   /* operation timeout */
    BTOS_E_ISR        = -5,   /* not allowed in ISR context */
} BTOS_Status;


typedef struct OS_Queue {
    QueueHandle_t   handle;
}BTOS_Queue_t;

typedef uint32_t BTOS_Time_t;

BTOS_Status BTOS_QueueCreate(BTOS_Queue_t *queue, uint32_t queueLen, uint32_t itemSize);
BTOS_Status BTOS_QueueDelete(BTOS_Queue_t *queue);
BTOS_Status BTOS_QueueSend(BTOS_Queue_t *queue, const void *item, BTOS_Time_t waitMS);
BTOS_Status BTOS_QueueReceive(BTOS_Queue_t *queue, void *item, BTOS_Time_t waitMS);
#endif
