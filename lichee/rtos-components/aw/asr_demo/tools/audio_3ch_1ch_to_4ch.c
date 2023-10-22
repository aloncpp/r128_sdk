#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

static inline void audio_3ch_1ch_to_4ch(short *p4ch, const short *p3ch0, const short *p1ch, unsigned int n)
{
	for (unsigned int i = 0; i < n; i++) {
		p4ch[i * 4]     = p3ch0[i * 3];
		p4ch[i * 4 + 1] = p3ch0[i * 3 + 1];
		p4ch[i * 4 + 2] = p3ch0[i * 3 + 2];
		p4ch[i * 4 + 3] = p1ch[i];
	}
}

static void usage(const char *cmd)
{
	printf("usage:\n");
	printf("\t%s <input file path> <output file path>\n", cmd ? cmd : "cmd");
}

#define FRAME_SIZE	(160)
int main(int argc, char *argv[])
{
	int ret = -1;
	FILE *fp_in = NULL;
	FILE *fp_out = NULL;
	unsigned char read_buffer[4 * sizeof (short) * FRAME_SIZE];
	unsigned char write_buffer[4 * sizeof (short) * FRAME_SIZE];
	short *p3ch;
	short *p1ch;

	if (argc != 3 || !strcmp(argv[1], argv[2])) {
		usage(argv[0]);
		goto exit;
	}

	fp_in = fopen(argv[1], "rb");
	if (!fp_in) {
		printf("open file error! %s\n", argv[1]);
		goto exit;
	}

	fp_out = fopen(argv[2], "wb+");
	if (!fp_out) {
		printf("open file error! %s\n", argv[2]);
		goto exit;
	}

	while (1) {
		ret = fread(read_buffer, 1, sizeof(read_buffer), fp_in);
		if (sizeof(read_buffer) != ret) {
			printf("read ret: %u\n", ret);
			break;
		}

		p3ch = (short *)&read_buffer[0];
		p1ch = (short *)&read_buffer[sizeof(short) * FRAME_SIZE * 3];
		audio_3ch_1ch_to_4ch((short *)write_buffer, p3ch, p1ch, FRAME_SIZE);

		ret = fwrite(write_buffer, 1, sizeof(write_buffer), fp_out);
		if (sizeof(write_buffer) != ret) {
			printf("write ret: %u\n", ret);
			break;
		}
	}

	ret = 0;
exit:
	if (fp_in)
		fclose(fp_in);
	if (fp_out)
		fclose(fp_out);
	return ret;
}