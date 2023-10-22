#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sunxi_hal_pwm.h>
#include <hal_cmd.h>

#include "thermal_external.h"
#include "pid.h"


#define PWM_PERIOD_NS  (128 * 1000)
#define PWM_SAFETY_GAP (500)

static int pwm_channel_ = 0;
static int target_temp_ = 80;

int cycle_to_duty(int cycle)
{
	if (cycle > 3072)
		cycle = 3072;
    else if (cycle < 0)
		cycle = 0;

    /* nanoseconds */
	const float plus = 41.67f;
	float active_ns = cycle * plus;

	int duty = floor(active_ns);

	if (duty > (PWM_PERIOD_NS - PWM_SAFETY_GAP))
		duty = PWM_PERIOD_NS - PWM_SAFETY_GAP;

	return duty;
}

int cmd_pwm_tem(int argc, char** argv)
{
	struct pwm_config *config;
	int duty = 0;
	int cycle = 0;
	int i = 0;
	int id_buf[1];
	int temperature[1];

	if (argc < 3) {
		printf("Please input correct parameter!\n");
		return -1;
    }

	pwm_channel_ = strtol(argv[1], NULL, 0);
	target_temp_ = strtol(argv[2], NULL, 0);
	config = (struct pwm_config *)malloc(sizeof(struct pwm_config));
	config->duty_ns   = duty;
	config->period_ns = PWM_PERIOD_NS;
	config->polarity  = PWM_POLARITY_NORMAL;

	hal_pwm_init();
	InitPID();

	while (1) {
		thermal_external_get_temp(id_buf, temperature, 1);//thermal_temperature_get();
		temperature[0] = temperature[0] / 1000;
		//printf("The current temperature is %d\n", temperature[0]);

		if (temperature[0] < target_temp_) {
			pid.SetTemperature    = target_temp_;
			pid.ActualTemperature = temperature[0];
			cycle = LocPIDCalc();
			//printf("cycle is %d\n", cycle);
			duty  = cycle_to_duty(cycle);
        }
        else {
			duty = 0;
			pid.SumError = 0;
		}

		//printf("duty is %d\n",duty);
		i = i + 1;
		config->duty_ns = duty;
		hal_pwm_control(pwm_channel_, config);

		usleep(1000 * 1000);
		if (i == 10) {
			printf("pwm_tem is running! duty = %d \n", duty);
			i = 0;
		}
	}
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_pwm_tem, hal_pwm_tem, pwm tem)
