#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tinatest.h>
#include <hal_timer.h>

#define POWERFAIL_STATUS CONFIG_TESTCASE_POWERFAIL_PATH "/tinatest.powerfail.status"
#define POWERFAIL_TIMES CONFIG_POWERFAIL_TIMES

extern int rwcheck_main(int argc, char **argv);

int auto_runpowerfail_test(int argc, char **argv)
{
    FILE *fp = NULL;
    char buf[4] = {0}, recv[4] = {0};
    char fcnt[] = POWERFAIL_TIMES;
    int tmp = 0;
    int ret = 0;

    char *fulldisk_data_argv[] =
    {
        "rwcheck",
        "-d",
        CONFIG_TESTCASE_FULLDISK_PATH,
        "-t",
        "100000",
    };

    int fulldisk_data_argc = sizeof(fulldisk_data_argv) / sizeof(fulldisk_data_argv[0]);

    fp = fopen(POWERFAIL_STATUS, "r+");
    if (fp == NULL)
    {
        return -1;
    }

    printf("=============Test for powerfail==============\n");

    fseek(fp, 0, SEEK_SET);
    fread(buf, strlen(fcnt) + 1, 1, fp);
    fclose(fp);

    tmp = strtol(buf, NULL, 0);
    if (tmp == 0)
    {
        printf("Tinatest Reboot successful!\n");
        remove(POWERFAIL_STATUS);
        return 0;
    }
    printf("=====powerfail times is %d\n", (strtol(fcnt, NULL, 0) - tmp + 1));

    tmp = tmp - 1;
    printf("=====powerfail left %d times\n", tmp);
    sprintf(recv, "%d", tmp);

    fp = fopen(POWERFAIL_STATUS, "w+");
    fwrite(recv, strlen(recv) + 1, 1, fp);
    fseek(fp, 0, SEEK_SET);
    fread(buf, strlen(recv) + 1, 1, fp);

    fclose(fp);

    if (tmp == 0)
    {
        remove(POWERFAIL_STATUS);
    }

    ret = rwcheck_main(fulldisk_data_argc, fulldisk_data_argv);
    return ret;
}

//tt powerfailtester
int tt_powerfailtest(int argc, char **argv)
{
    FILE *fp = NULL;
    char buf[4] = {0}, recv[4] = {0};
    char fcnt[] = POWERFAIL_TIMES;
    int tmp = 0;
    int ret = 0;

    char *fulldisk_data_argv[] =
    {
        "rwcheck",
        "-d",
        CONFIG_TESTCASE_FULLDISK_PATH,
        "-t",
        "100000",
    };

    int fulldisk_data_argc = sizeof(fulldisk_data_argv) / sizeof(fulldisk_data_argv[0]);

    printf("=============Test for powerfail==============\n");

    fp = fopen(POWERFAIL_STATUS, "r+");
    if (fp == NULL)
    {
        printf("open file failed, touch a new one\n");
        fp = fopen(POWERFAIL_STATUS, "w+");
        fwrite(fcnt, strlen(fcnt) + 1, 1, fp);
    }

    fseek(fp, 0, SEEK_SET);
    fread(buf, strlen(fcnt) + 1, 1, fp);
    fclose(fp);

    tmp = strtol(buf, NULL, 0);
    if (tmp == 0)
    {
        printf("Tinatest Reboot successful!\n");
        remove(POWERFAIL_STATUS);
        return 0;
    }
    printf("=====powerfail times is %d\n", (strtol(fcnt, NULL, 0) - tmp + 1));

    tmp = tmp - 1;
    printf("=====powerfail left %d times\n", tmp);
    sprintf(recv, "%d", tmp);

    fp = fopen(POWERFAIL_STATUS, "w+");
    fwrite(recv, strlen(recv) + 1, 1, fp);
    fseek(fp, 0, SEEK_SET);
    fread(buf, strlen(recv) + 1, 1, fp);

    fclose(fp);

    ret = rwcheck_main(fulldisk_data_argc, fulldisk_data_argv);

    return ret;
}
testcase_init(tt_powerfailtest, powerfail, powerfail for tinatest);
//auto_testcase_init(auto_runpowerfail_test, auto_powerfail, auto powerfail for tinatest);
