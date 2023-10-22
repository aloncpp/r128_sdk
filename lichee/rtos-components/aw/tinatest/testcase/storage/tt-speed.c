#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <tinatest.h>

extern int rwspeed_main(int argc, char **argv);

int tt_speed(int argc, char **argv)
{
    int ret = 0;
    char *rwspeed_argv[] =
    {
        "rwspeed",
        "-d",
        "/data",
        "-s",
        "256K",
    };

    int rwspeed_argc = sizeof(rwspeed_argv) / sizeof(rwspeed_argv[0]);

    ret = rwspeed_main(rwspeed_argc, rwspeed_argv);
    if (ret)
    {
        printf("rwspeed failed!\n");
        return -1;
    }

    return ret;
}
testcase_init(tt_speed, speed, speed for tinatest);
