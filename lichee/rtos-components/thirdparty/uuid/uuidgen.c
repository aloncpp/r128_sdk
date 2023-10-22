/*
 * gen_uuid.c --- generate a DCE-compatible uuid
 *
 * Copyright (C) 1999, Andreas Dilger and Theodore Ts'o
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "uuid.h"

#include <console.h>

static void usage(FILE * out)
{
	fputs(("\nUsage:\n"), out);

	fputs(("\nOptions:\n"), out);
	fputs((" -r, --random     generate random-based uuid\n"
		" -t, --time       generate time-based uuid\n"
		" -V, --version    output version information and exit\n"
		" -h, --help       display this help and exit\n\n"), out);
}

int
cmd_uuidgen (int argc, char *argv[])
{
	int    c;
	int    do_type = 0;
	char   str[37];
	uuid_t uu;

	static const struct option longopts[] = {
		{"random", no_argument, NULL, 'r'},
		{"time", no_argument, NULL, 't'},
		{"version", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "rtVh", longopts, NULL)) != -1)
		switch (c) {
		case 't':
			do_type = UUID_TYPE_DCE_TIME;
			break;
		case 'r':
			do_type = UUID_TYPE_DCE_RANDOM;
			break;
		case 'V':
			return EXIT_SUCCESS;
		case 'h':
			usage(stdout);
			return 0;
		default:
			usage(stderr);
			return -1;
		}

	switch (do_type) {
	case UUID_TYPE_DCE_TIME:
		uuid_generate_time(uu);
		break;
	case UUID_TYPE_DCE_RANDOM:
		uuid_generate_random(uu);
		break;
	default:
		uuid_generate(uu);
		break;
	}

	uuid_unparse(uu, str);

	printf("%s\n", str);

	return EXIT_SUCCESS;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_uuidgen, uuidgen, uuidgen Command);
