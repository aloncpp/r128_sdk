#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <tinatest.h>

extern int rwcheck_main(int argc, char **argv);

//tt fulldisk
int tt_fulldisk(int argc, char **argv)
{
    int ret = 0;

    char *fulldisk_data_argv[] = {
        "rwcheck",
        "-d",
        "/data",
        "-t",
        "100",
    };

    int fulldisk_data_argc = sizeof(fulldisk_data_argv) / sizeof(fulldisk_data_argv[0]);

    ret = rwcheck_main(fulldisk_data_argc, fulldisk_data_argv);

    return ret;
}
testcase_init(tt_fulldisk, fulldisk, fulldisk for tinatest);
