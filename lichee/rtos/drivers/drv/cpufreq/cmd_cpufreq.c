/*#####################################################################
# File Describe:cmd_cpufreq.c
# Author: flyranchaoflyranchao
# Created Time:flyranchao@allwinnertech.com
# Created Time:2019年09月20日 星期五 19时18分42秒
#====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <console.h>
#include <drivers/hal_cpufreq.h>
#include <hal_clk.h>
#include <sunxi_hal_regulator.h>

void help(void)
{
	printf("* please set pwm information for set cpu volt!\n");
	printf("* -f cpufreq\n");
	printf("* -p period_ns\n");
	printf("* -o polarity\n");
	printf("* -c chanel\n");
	printf("* -b vol_base\n");
	printf("* -m vol_max\n");
	printf("* eg set_cpuf -f 72000000 -p 5000 -o 0 -c 7\n");
	printf("*             -b 831000 -m 1102000\n");
	printf("* default only set freq: set_cpuf -f 72000000\n");
	printf("**********************************************\n");
}

int cmd_cpufreq_get(int argc, char ** argv)
{
	int i;
	u32 rate;
	struct cpufreq_frequency_table *vf_table;
	struct cpufreq_info *info;

	hal_cpufreq_info_get(0x3, &info);

	vf_table = (*info).freq_table;
	printf("\n               vf_table   \n");
	printf("**********************************************\n");
	for(i=0; i < 3; i++){
		printf("* %d--%d\n", (*vf_table).frequency, (*vf_table).target_uV);
		vf_table++;
	}
	printf("**********************************************\n");

	rate = hal_clk_get_rate(info->clk);
	printf("\ncurrent cpufreq: %d\n", rate);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_cpufreq_get, get_cpuf, get the cpu frequency)

int cmd_cpufreq_set(int argc, char *argv[])
{
	int ch,target_freq;
	int flag = 0;
	struct cpufreq_info *info;
	struct regulator_dev rdev;
	struct pwm_regulator_info pwm_info;
	printf("**********************************************\n");
	while((ch = getopt(argc,argv,"f:p:o:c:b:m:h")) != -1){
		switch(ch){
			case 'h':
				help();
				break;
			case 'f':
				printf("* taget_cpufreq: %d\n", atoi(optarg));
				target_freq = atoi(optarg);
				flag = 1;
				break;
			case 'p':
				printf("* period_ns: %d\n", atoi(optarg));
				pwm_info.period_ns = atoi(optarg);
				break;
			case 'o':
				printf("* polarity: %d\n", atoi(optarg));
				pwm_info.polarity = atoi(optarg);
				break;
			case 'c':
				printf("* chanel: %d\n", atoi(optarg));
				pwm_info.chanel = atoi(optarg);
				break;
			case 'b':
				printf("* vol_base: %d\n", atoi(optarg));
				pwm_info.vol_base = atoi(optarg);
				break;
			case 'm':
				printf("* vol_max: %d\n", atoi(optarg));
				pwm_info.vol_max = atoi(optarg);
				break;
			default:
				printf("default");
		}
	}
	printf("**********************************************\n");

	if(argc <= 1){
		help();
		return -1;
	}

	hal_cpufreq_info_get(0x3, &info);
#ifdef CONFIG_DRIVERS_PWM
	if(argc >= 4){
		/*pwm change volt function*/
		hal_regulator_init(&pwm_info, 0x2 << 30, &rdev);
		hal_cpufreq_regulator_set(info, &rdev);
	}
#endif
	if(flag)
		hal_cpufreq_target(info,target_freq);
	else{
		help();
		return -1;
	}
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_cpufreq_set, set_cpuf, set the cpu frequency)
