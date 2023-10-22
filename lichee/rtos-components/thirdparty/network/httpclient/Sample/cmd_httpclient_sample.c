/*#####################################################################
# File Describe:components/common/thirdparty/network/httpclient/Sample/cmd_httpclient.c
# Author: flyranchaoflyranchao
# Created Time:flyranchao@allwinnertech.com
# Created Time:2019年09月11日 星期三 10时08分59秒
#====================================================================*/
#include <stdlib.h>
#include <console.h>

int httpclient_sample_main(int argc, char *argv[]);

int cmd_sample_httpclient(int argc, char ** argv)
{
    httpclient_sample_main(argc, argv);
}
FINSH_FUNCTION_EXPORT_CMD(cmd_sample_httpclient, hp_test, httpclient test)
