#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <wifimg.h>
#include <hal_time.h>

#include <lwip/tcpip.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>

#include <HTTPClient.h>
#include <HTTPCUsr_api.h>
#include <cJSON.h>

#include <sntp.h>

#include <awtrix.h>

int sntp_test()
{
    int SNTP_FAIL_NUM = 5;
    uint32_t fail_it = 0, success_it = 0;
    uint32_t test_cnt = SNTP_FAIL_NUM;
    printf("<sntp> <request>\n");

    sntp_set_server(0, "203.107.6.88");
    sntp_set_server(1, "ntp5.aliyun.com");
    sntp_set_server(2, "tw.pool.ntp.org");

    setenv("TZ", "CST-8", 1);
    tzset();

    while (1)
    {
        test_cnt--;
        if (sntp_request(NULL) != 0) {
            fail_it++;
            if(fail_it >= SNTP_FAIL_NUM)
            {
                printf("<sntp> <response : fail>\n");
                break;
            }
            printf("<sntp> <try : %d>\n", fail_it);
        }
        else
        {
            sntp_time *time = (sntp_time *)sntp_obtain_time();
            success_it++;
            printf("<sntp> <response : success>\n");
            printf("===sntp->get time form network (year mon day week hour min sec===\n");
            printf("(%04d-%02d-%02d  %u   %02d:%02d:%02d)\n", time->year, time->mon, time->day, time->week, time->hour, time->min, time->sec);
            break;
        }
        if (test_cnt == 0) {
            printf("<sntp> teste end!!! success cnt:<%d>, fail cnt:<%d>\n", success_it, fail_it);
            break;
        }
        hal_msleep(500);
    }

    struct timeval tv;
	gettimeofday(&tv, NULL);

    printf("utc time: %ld.%ld\n", tv.tv_sec, tv.tv_usec);


    time_t t = time(NULL);
    struct tm tm_time;
    char buf[128];

    localtime_r(&t, &tm_time);
    strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tm_time);
    printf("localtime_r: %s\n", buf);
    setenv("TZ", "CST-8", 1);
    tzset();
extern weather_t local_weather;
    local_weather.type = 0;
    while (1)
    {
        hal_msleep(1000);
        pixel_t *awtrix = awtrix_get_pixel();
        t = time(NULL);
        localtime_r(&t, &tm_time);
        strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tm_time);
        // printf("localtime_r: %s\n", buf);
        // awtrix_display_set_clock(awtrix, tm_time);
        awtrix_display_set_clock_2(awtrix, tm_time);
        // awtrix_display_set_weather(awtrix, 30);
    }

    return 0;
}

int http_test()
{
	int   ret = 0;
	char *buf = NULL;
	unsigned int toReadLength = 4096;
	unsigned int Received = 0;
	HTTP_CLIENT  httpClient;
	HTTPParameters *clientParams = NULL;
    char *URL = "http://api.seniverse.com/v3/weather/now.json?key=S95aobioJH_vZfDlB&location=tianjin&language=zh-Hans&unit=c";

    printf("@@@@@@@@@@@ HTTP TEST @@@@@@@@@@@\r\n");

    clientParams = malloc(sizeof(HTTPParameters));
    if( clientParams == NULL )
    {
        printf("clientParams == NULL\r\n");
        goto err_exit;
    }
    memset(clientParams,0,sizeof(HTTPParameters));

	buf = malloc(toReadLength);
	if (buf == NULL) {
        printf("buf == NULL\r\n");
		goto err_exit;
	}
    memset(buf, 0, toReadLength);
    do
    {
        printf("\nHTTP Client v1.0\n\n");
        // Reset the parameters structure
        
        clientParams->HttpVerb = VerbPost;
        clientParams->pData = buf;
        memcpy(clientParams->Uri, URL, strlen(URL));
        printf("http send to :%s\r\n", clientParams->Uri);
        clientParams->pLength = toReadLength;
        ret = HTTPC_open(clientParams);
       	if (ret != 0) {
            printf("http open err..\n");
            goto err_exit;
        }

        ret = HTTPC_request(clientParams, NULL);
        if (ret != 0) {
            printf("http request err..\n");
            goto err_exit;
        }

        ret = HTTPC_get_request_info(clientParams, &httpClient);
        if (ret != 0) {
            printf("http get request info err..\n");
            goto err_exit;
        }

        if (httpClient.HTTPStatusCode != HTTP_STATUS_OK) 
        {
            printf("http post fail: %d\r\n", httpClient.HTTPStatusCode);
        }else
        {
            printf("http post ok\r\n");
        }


	    if (httpClient.TotalResponseBodyLength != 0
		|| (httpClient.HttpFlags & HTTP_CLIENT_FLAG_CHUNKED)) {
		do {
			ret = HTTPC_read(clientParams, buf, toReadLength,
			                 (void *)&Received);
			if ((ret != HTTP_CLIENT_SUCCESS) && (ret != HTTP_CLIENT_EOS)) {
				printf("Transfer err...ret:%d\n", ret);
				ret = -1;
				break;
			}
			if (ret == HTTP_CLIENT_EOS) {
				printf("The end..\n");
				ret = 0;
				break;
			}
		} while (1);
        printf("recv :\r\n%s\r\n", buf);
        awtrix_set_weather_info(buf);
	}

    }while (0);

    sntp_test();

err_exit:

    if( buf )
    {
        free(buf);
    }

    return ret;

}

int wifi_init()
{
    wmg_status_t status = WMG_STATUS_FAIL;
    wifi_wmg_state_t wmg_state;
    int scanf = false;

     printf("@@@@@@@@@@@ WIFI TEST @@@@@@@@@@@\r\n");

    if( wifi_get_wmg_state(&wmg_state) !=  WMG_STATUS_SUCCESS)
    {
        printf("wifi_get_wmg_state error\r\n");
        return status;
    }
    
    if( wmg_state.sta_state == WIFI_STA_CONNECTED || wmg_state.sta_state == WIFI_STA_NET_CONNECTED )
    {
        printf("@@@ wifi connected @@@@\r\n");
        return WMG_STATUS_SUCCESS;
    }

    printf("wifi sta     status: %d\r\n", wmg_state.sta_state);
    printf("wifi ap      status: %d\r\n", wmg_state.ap_state);
    printf("wifi monitor status: %d\r\n", wmg_state.monitor_state);
    printf("wifi p2p     status: %d\r\n", wmg_state.p2p_state);

    status = wifimanager_init();
    if( status !=  WMG_STATUS_SUCCESS)
        return status;

    if( wifi_on(WIFI_STATION) !=  WMG_STATUS_SUCCESS)
        return status;

    return 1;
}

int wifi_test()
{
    
    wmg_status_t status = WMG_STATUS_FAIL;
    int scanf = false;

    uint32_t bss_num;
    uint32_t arr_size = 10;
    wifi_scan_result_t *result = malloc(sizeof(wifi_scan_result_t)*arr_size);
    wifi_scan_result_t *scanf_p;

    wifi_wmg_state_t wmg_state;
    if( wifi_get_wmg_state(&wmg_state) !=  WMG_STATUS_SUCCESS)
    {
        printf("wifi_get_wmg_state error\r\n");
        return status;
    }

    if( wmg_state.sta_state == WIFI_STA_CONNECTED || wmg_state.sta_state == WIFI_STA_NET_CONNECTED )
    {
        printf("@@@ wifi connected @@@@\r\n");
        return WMG_STATUS_SUCCESS;
    }

    if( wifi_get_scan_results(result, &bss_num, arr_size) !=  WMG_STATUS_SUCCESS)
        return status;

    printf("@@@@@@@@@@@ END @@@@@@@@@@@\r\n");

    printf("bss_num: %d\r\n", bss_num);

    for(int i=0;i<((bss_num<=arr_size)?bss_num:arr_size);i++)
    {
        wifi_scan_result_t *p = (wifi_scan_result_t *)&result[i];
        printf("[%d]rssi: %d, freq: %d, ssid: %s\r\n", i, p->rssi, p->freq, p->ssid);
        if ( strcmp("ChinaNet-BVBi", p->ssid) == 0)
        {
            scanf = true;
            scanf_p = (wifi_scan_result_t *)&result[i];
            break;
        }
    }

    if(scanf)
    {
        wifi_sta_cn_para_t sta_info = {
            .ssid = scanf_p->ssid,
            .password = "e6s7tmuq",
            .fast_connect = 0,
            .sec = WIFI_SEC_WPA2_PSK
        };
        if(wifi_sta_connect(&sta_info) == WMG_STATUS_SUCCESS)
        {
            printf("wifi connect success\r\n");
            return WMG_STATUS_SUCCESS;
        }else
        {
            printf("wifi connect error\r\n");
        }
    }
    return WMG_STATUS_FAIL;
}
