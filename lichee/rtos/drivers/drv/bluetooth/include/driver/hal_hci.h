#ifndef HAL_DRIVER_H
#define HAL_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

typedef struct hal_hci_callbacks {
  int (*data_ind)(uint8_t *data, uint16_t len);
} hal_hci_callbacks_t;

/**
 * @brief           This function is called to register receive interface to controller
 *                  and init hci driver.
 *
 * @param[in]       *hcb - the point of receive interface
 *
 * @return
 *                  - 0 : Succeed
 *                  - others: failed
 */
int hal_hci_open(void *hcb);

/**
 * @brief           This function is called to deregister receive 
 *                  interface to bluetooth controller.
 *
 * @return
 *                  - 0 : Succeed
 *                  - others: failed
 */
int hal_hci_close(void);

/**
 * @brief           This function is called to send host data to bluetooth controller.
 *
 * @param[in]       *data - the point of data to be sent
 * @param[in]       len - the length of data
 *
 * @return
 *                  - 0 : Succeed
 *                  - 1 : failed
 */
int hal_hci_write(uint8_t *data, uint16_t len);

#endif
