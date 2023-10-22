#ifndef __PM_M33_WAKESRC_H__
#define __PM_M33_WAKESRC_H__

#include "pm_wakesrc.h"

#define INVALID_IRQn -20

uint32_t pm_wakesrc_read_pending(void);
uint32_t pm_wakesrc_pending_is_for_affinity(wakesrc_affinity_t core);
uint32_t pm_wakesrc_check_irqs(void);

uint32_t pm_wakesrc_get_active(void);
uint32_t pm_wakesrc_get_event(void);
void pm_wakesrc_set_wakeup_irq(uint32_t event, const int irq);
void pm_wakesrc_clear_wakeup_irq(void);
int pm_wakeup_ops_m33_init(void);

int pm_wakesrc_prepared(void);
int pm_wakesrc_complete(void);

int pm_wakesrc_active(wakesrc_id_t id, wakesrc_affinity_t core);
int pm_wakesrc_deactive(wakesrc_id_t id, wakesrc_affinity_t core);
int pm_wakesrc_mask_affinity(wakesrc_affinity_t core);
int pm_wakesrc_unmask_affinity(wakesrc_affinity_t core);
uint32_t pm_wakesrc_pending_in_affinity(wakesrc_affinity_t core);

int pm_m33_wakesrc_init(void);

#endif /* __PM_M33_WAKESRC_H__ */
