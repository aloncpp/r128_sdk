#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <wifimg.h>

#include <lwip/tcpip.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>

#include <HTTPClient.h>
#include <HTTPCUsr_api.h>

int http_test()
{
	int   ret = 0;
	char *buf = NULL;
	unsigned int toReadLength = 4096;
	unsigned int Received = 0;
	HTTP_CLIENT  httpClient;
	HTTPParameters *clientParams = NULL;
    char *URL = "http://api.seniverse.com/v3/weather/now.json?key=S95aobioJH_vZfDlB&location=beijing&language=zh-Hans&unit=c";

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
	}

    }while (0);

err_exit:

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
    
    if( wifi_get_scan_results(result, &bss_num, arr_size) !=  WMG_STATUS_SUCCESS)
        return status;

    printf("@@@@@@@@@@@ END @@@@@@@@@@@\r\n");

    printf("bss_num: %d\r\n", bss_num);

    for(int i=0;i<((bss_num<=arr_size)?bss_num:arr_size);i++)
    {
        wifi_scan_result_t *p = (wifi_scan_result_t *)&result[i];
        printf("[%d]rssi: %d, freq: %d, ssid: %s\r\n", i, p->rssi, p->freq, p->ssid);
        if ( strncmp("ChinaNet-BVBi", p->ssid, strlen(p->ssid)) == 0)
        {
            scanf = true;
            scanf_p = (wifi_scan_result_t *)&result[i];
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
