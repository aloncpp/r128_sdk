#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <tinatest.h>

extern int rwcheck_main(int argc, char **argv);
extern int rwspeed_main(int argc, char **argv);

int tt_nor(int argc, char **argv)
{
    int ret = 0;

    char *nor_rwcheck_argv[] =
    {
        "rwcheck",
        "-d",
        "/data",
        "-t",
        "1",
        "-s",
        "256K",
    };

    char *nor_rwspeed_argv[] =
    {
        "rwspeed",
        "-d",
        "/data",
        "-s",
        "256K",
    };

    int nor_rwcheck_argc = sizeof(nor_rwcheck_argv) / sizeof(nor_rwcheck_argv[0]);

    int nor_rwspeed_argc = sizeof(nor_rwspeed_argv) / sizeof(nor_rwspeed_argv[0]);

    ret = rwcheck_main(nor_rwcheck_argc, nor_rwcheck_argv);
    if (ret)
    {
        printf("rwcheck failed!\n");
        goto error;
    }

    ret = rwspeed_main(nor_rwspeed_argc, nor_rwspeed_argv);
    if (ret)
    {
        printf("rwspeed failed!\n");
        goto error;
    }

    return 0;
error:
    printf("nortester filed!\n");
    return -1;
}
testcase_init(tt_nor, nortester, nortester for tinatest);
