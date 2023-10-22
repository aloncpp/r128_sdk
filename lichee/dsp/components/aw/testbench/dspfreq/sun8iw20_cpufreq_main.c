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

extern int dsp_set_freq(int clk_rate);
extern int dsp_get_freq(void);
extern int get_dsp_freq_table_size(void);
extern int get_dsp_freq_table_freq(int index);

int cmd_dspfreq(int argc, char **argv)
{
	int opt ;
	int ret ;
	int sfreq = -1;
	int gfreq = -1;
	int savefreq = 0;
	int cnt = 0;

	while ((opt = getopt(argc, argv, "ltghs:")) != -1) {
		switch (opt) {
		case 'l':
			cnt =  get_dsp_freq_table_size();
			printf("support freq(Unit: Hz):\n");
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
			printf("Set frequency: %d Hz.\n", sfreq);
			ret = dsp_set_freq(sfreq);
			if (ret < 0)
				printf("Error: Set frequency, return %d.\n", ret);
			break;
		case 'g':
			gfreq = dsp_get_freq();
			if (gfreq <=0 ) {
				printf("Error: get freq failed, return %d.\n", gfreq);
			} else {
				printf("Current frequency: %d Hz.\n", gfreq);
			}
			break;
		case 't':
			cnt =  get_dsp_freq_table_size();
			savefreq = dsp_get_freq();
			printf("Save Cur frequency: %d Hz.\n", savefreq);
			for (; cnt > 0; cnt--) {
				sfreq=get_dsp_freq_table_freq(cnt-1);
				dsp_set_freq(sfreq);
				printf("Set frequency: %d Hz.\n", sfreq);

				gfreq = dsp_get_freq();
				printf("Get frequency: %d Hz.\n", gfreq);
			}
			dsp_set_freq(savefreq);
			printf("Restore frequency: %d Hz.\n", savefreq);
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
