#include <stdio.h>
#include <bsp.h>
#include <stdbool.h>

#include <bt_rf.h>


void bt_bsp_init(void)
{
	bt_rf_probe();
}

void bt_reset_gpio(void)
{

}

void bt_reset_set(void)
{
	bt_set_rfkill_state(true);
}

void bt_reset_clear(void)
{
	bt_set_rfkill_state(false);
}
