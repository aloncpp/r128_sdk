#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <console.h>

#include <config/aw/dspfreq.h>
#include <hal_prcm.h>
#include <aw_common.h>
#include <sunxi_hal_common.h>

void dsp_freq_usage(void)
{
	char buff[] = \
	"Usage:\n"
	"-l: list support freqtable.\n"
	"-s: set freq, A parameter is required.\n"
	"-g: get freq.\n"
	"-t: test demo.\n"
	"-h: print the message.\n"
	"\n";
	printf("%s", buff);
}

extern void dsp_set_freq(int freq_m);
extern int get_dsp_freq_table_size(void);
extern u32 get_dsp_freq_table_freq(int index);

int cmd_dspfreq(int argc, char **argv)
{
	int sfreq = -1;
	int opt;
	unsigned int gfreq = 0;
	unsigned int savefreq = 0;
	int cnt =  get_dsp_freq_table_size();

	while ((opt = getopt(argc, argv, "ltghs:")) != -1) {
		switch (opt) {
		case 'l':
			cnt =  get_dsp_freq_table_size();
			printf("support freq(Unit: KHz):\n");
			for (; cnt > 0; cnt--) {
				printf("%d ", get_dsp_freq_table_freq(cnt-1));
			}
			printf("\n");
			break;
		case 's':
			sfreq = atoi(optarg);
			if (sfreq < 0) {
				printf("Error: value(%d) less than 0.\n",
				       sfreq);
				goto err;
			}
			printf("Set frequency: %d KHz.\n", sfreq);
			dsp_set_freq(sfreq);
			break;
		case 'g':
			gfreq = ccu_get_sclk_freq(CCU_SYS_CLK_CPUS);
			if (!gfreq) {
				printf("Error: get freq failed.\n");
			} else {
				printf("Current frequency: %d Hz.\n", gfreq);
			}
			break;
		case 't':
			cnt =  get_dsp_freq_table_size();
			savefreq = ccu_get_sclk_freq(CCU_SYS_CLK_CPUS)/1000;
			printf("Save Cur frequency: %d KHz.\n", savefreq);
			for (; cnt > 0; cnt--) {
				sfreq=get_dsp_freq_table_freq(cnt-1);
				dsp_set_freq(sfreq);
				printf("Set frequency: %d KHz.\n", sfreq);

				gfreq = ccu_get_sclk_freq(CCU_SYS_CLK_CPUS);
				printf("Get frequency: %d Hz.\n", gfreq);
			}
			dsp_set_freq(savefreq);
			printf("Restore frequency: %d KHz.\n", savefreq);
			break;
		case 'h':
		default:	/* '?' */
			dsp_freq_usage();
			break;
		}
	}
	return 0;
err:
	return -1;

}

FINSH_FUNCTION_EXPORT_CMD(cmd_dspfreq, dspfreq, "set/get dsp freq.");
