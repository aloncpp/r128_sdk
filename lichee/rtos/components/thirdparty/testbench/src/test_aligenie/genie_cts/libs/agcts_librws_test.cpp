/**
 * Copyright (C) 2018 Alibaba.inc, All rights reserved.
 *
 * @file:    agcts_librws_test.cpp
 * @brief:
 * @author:  tanyan.lk@alibaba-inc.com
 * @date:    2019/7/10
 * @version: 1.0
 */
#include "librws.h"
#include "rws_socket.h"
#include "agcts.h"
#include "console.h"
#define WEBSOCKET_CONNECTED             (0x01)
#define WEBSOCKET_DISCONNECTED          (0x02)
#define WEBSOCKET_DATA_NOT_RECVED       (0x04)
#define WEBSOCKET_SSL_ENABLE

static rws_socket _socket = NULL;
static int _state_flags = 0;
//add by flyranchao@allwinnertech.com
void CHECK_RET_WITH_RET(rws_socket socket, int i)
{

}
void rws_socket_send_bin_start(rws_socket socket, const void * text, const unsigned int length)
{

}

void rws_socket_send_bin_continue(rws_socket socket, const void * text, const unsigned int length)
{

}

void rws_socket_send_bin_finish(rws_socket socket, const void * text, const unsigned int length)
{

}
int _agcts_websoc_set_get_test(void)
{
    int ret = _AGCTS_FAIL;

    const char * scheme = "ws";
    const char * host = "echo.websocket.org";
    const char * path = "/";

    rws_socket socket = rws_socket_create();
    assert(socket);

    rws_socket_set_scheme(socket, scheme);                                _AGCTS_LOGD("%i\n", (int)__LINE__);
    assert(strcmp(rws_socket_get_scheme(socket), scheme) == 0);            _AGCTS_LOGD("%i\n", (int)__LINE__);
    rws_socket_set_host(socket, host);                                    _AGCTS_LOGD("%i\n", (int)__LINE__);
    assert(strcmp(rws_socket_get_host(socket), host) == 0);                _AGCTS_LOGD("%i\n", (int)__LINE__);
    rws_socket_set_path(socket, path);                                    _AGCTS_LOGD("%i\n", (int)__LINE__);
    assert(strcmp(rws_socket_get_path(socket), path) == 0);                _AGCTS_LOGD("%i\n", (int)__LINE__);
    rws_socket_set_port(socket, 80);                                    _AGCTS_LOGD("%i\n", (int)__LINE__);
    assert(rws_socket_get_port(socket) == 80);                            _AGCTS_LOGD("%i\n", (int)__LINE__);
    rws_socket_set_port(socket, 443);                                    _AGCTS_LOGD("%i\n", (int)__LINE__);
    assert(rws_socket_get_port(socket) == 443);                            _AGCTS_LOGD("%i\n", (int)__LINE__);
    rws_socket_set_scheme(socket, NULL);                                _AGCTS_LOGD("%i\n", (int)__LINE__);
    assert(rws_socket_get_scheme(socket) == NULL);                        _AGCTS_LOGD("%i\n", (int)__LINE__);
    rws_socket_set_host(socket, NULL);                                    _AGCTS_LOGD("%i\n", (int)__LINE__);
    assert(rws_socket_get_host(socket) == NULL);                        _AGCTS_LOGD("%i\n", (int)__LINE__);
    rws_socket_set_path(socket, NULL);                                    _AGCTS_LOGD("%i\n", (int)__LINE__);
    assert(rws_socket_get_path(socket) == NULL);                        _AGCTS_LOGD("%i\n", (int)__LINE__);

    rws_socket_disconnect_and_release(socket);

    ret = _AGCTS_SUCCESS;
    return ret;
}

static void _on_socket_received_text(rws_socket socket, const char *text, const unsigned int length, bool is_finish)
{
    char *buff = NULL;

    if (!socket || !text || !length) {
        _AGCTS_LOGD("%s: Invalid parameter(s).", __FUNCTION__);
        return;
    }

    buff = (char *)malloc(length + 1);
    if (!buff) {
        _AGCTS_LOGD("%s: Not enough memory. len:%d", __FUNCTION__, length + 1);
        return;
    }

    _state_flags &= (~WEBSOCKET_DATA_NOT_RECVED);

    memcpy(buff, text, length);
    buff[length] = 0;

    _AGCTS_LOGD("%s: Socket text: %s", __FUNCTION__, buff);
    free(buff);
    buff = NULL;
}

static void _on_socket_received_bin(rws_socket socket, const void * data, const unsigned int length, bool is_finish)
{
    char *buff = NULL;

    if (!socket || !data || !length) {
        _AGCTS_LOGD("%s: Invalid parameter(s).", __FUNCTION__);
        return;
    }

    buff = (char *)malloc(length + 1);
    if (!buff) {
        _AGCTS_LOGD("%s: Not enough memory. len:%d", __FUNCTION__, length + 1);
        return;
    }

    _state_flags &= ~WEBSOCKET_DATA_NOT_RECVED;

    memcpy(buff, data, length);
    buff[length] = 0;

    _AGCTS_LOGD("%s: Socket bin: \n%s", __FUNCTION__, buff);
    free(buff);
    buff = NULL;
}

static void _on_socket_received_pong(rws_socket socket)
{
    if (!socket) {
        _AGCTS_LOGD("%s: Invalid parameter(s).", __FUNCTION__);
        return;
    }
    _AGCTS_LOGD("received pong!!!!!!!!!!!");
}

static void _on_socket_connected(rws_socket socket)
{
    const char * test_send_text =
        "{\"version\":\"1.0\",\"supportedConnectionTypes\":[\"websocket\"],\"minimumVersion\":\"1.0\",\"channel\":\"/meta/handshake\"}";

    _AGCTS_LOGD("%s: Socket connected", __FUNCTION__);

    _state_flags |= WEBSOCKET_CONNECTED;
    _state_flags &= ~WEBSOCKET_DISCONNECTED;
    rws_socket_send_text(socket, test_send_text);
}

static void _on_socket_disconnected(rws_socket socket)
{
    rws_error error = rws_socket_get_error(socket);
    if (error) {
        _AGCTS_LOGD("%s: Socket disconnect with code, error: %i, %s",
                __FUNCTION__,
                rws_error_get_code(error),
                rws_error_get_description(error));
    }

    _state_flags &= ~WEBSOCKET_CONNECTED;
    _state_flags |= WEBSOCKET_DISCONNECTED;
    _socket = NULL;
}

int _agcts_websoc_connection_test(void)
{
    int ret = _AGCTS_FAIL;
    int sleep_count = 0;
    const char *scheme = "wss";
    const char *host = "echo.websocket.org";
    const char *path = "/";
    const int port = 443;
    const char *ECHO_WEBSOCKET_CER = {
        "-----BEGIN CERTIFICATE-----\r\n"
        "MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\r\n"
        "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\r\n"
        "DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\r\n"
        "SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\r\n"
        "GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\r\n"
        "AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\r\n"
        "q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\r\n"
        "SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\r\n"
        "Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\r\n"
        "a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\r\n"
        "/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\r\n"
        "AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\r\n"
        "CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\r\n"
        "bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\r\n"
        "c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\r\n"
        "VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\r\n"
        "ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\r\n"
        "MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\r\n"
        "Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\r\n"
        "AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\r\n"
        "uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\r\n"
        "wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\r\n"
        "X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\r\n"
        "PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\r\n"
        "KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\r\n"
        "-----END CERTIFICATE-----\r\n"
    };

    if (!scheme || !host || !path) {
        _AGCTS_LOGD("%s: Invalid parameter(s).", __FUNCTION__);
        return ret;
    }

      if (_socket) {
        _AGCTS_LOGD("%s: Socket is not closed.", __FUNCTION__);
        return ret;
    }

    _socket = rws_socket_create(); // create and store socket handle
    CHECK_RET_WITH_RET(_socket, -1);

    _state_flags = 0;
    _state_flags |= WEBSOCKET_DATA_NOT_RECVED;

    rws_socket_set_scheme(_socket, scheme);
    rws_socket_set_host(_socket, host);
    rws_socket_set_path(_socket, path);
    rws_socket_set_port(_socket, port);

#ifdef WEBSOCKET_SSL_ENABLE
    if (ECHO_WEBSOCKET_CER) {
        rws_socket_set_server_cert(_socket, ECHO_WEBSOCKET_CER, strlen(ECHO_WEBSOCKET_CER) + 1);
    }
#endif

    rws_socket_set_on_disconnected(_socket, &_on_socket_disconnected);
    rws_socket_set_on_connected(_socket, &_on_socket_connected);
    rws_socket_set_on_received_text(_socket, &_on_socket_received_text);
    rws_socket_set_on_received_bin(_socket, &_on_socket_received_bin);
    rws_socket_set_on_received_pong(_socket, &_on_socket_received_pong);

    // connection denied for client applications
    rws_socket_connect(_socket);

    /* Connecting */
    while ((!(_state_flags & WEBSOCKET_CONNECTED)) &&
             (!(_state_flags & WEBSOCKET_DISCONNECTED))) {
        rws_thread_sleep(1000);
#if 0
        sleep_count++;
        if (30 == sleep_count) {
            break;
        }
#endif
    }

    /* Receiving data */
    sleep_count = 0;
    if (_state_flags & WEBSOCKET_CONNECTED) {
        while ((_state_flags & WEBSOCKET_DATA_NOT_RECVED) && _socket &&
                (rws_true == rws_socket_is_connected(_socket))) {
            rws_thread_sleep(1000);
            sleep_count++;
            if (20 == sleep_count) {
                break;
            }
        }
    }

    const char *text_arr[] = {
        "-----BEGIN CERTIFICATE-----"
        "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"
        "-----END CERTIFICATE-----",
        "AABBsdfasdfasdfasdfasdfasdfasdfadfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfadfasdfasdfasdfasdfasdf",
        "09840523490582034850-2385023845023845023840523orjoewjrt0932u4ojt[iq3w04tu32094503u4t32u4059",
        ";';,,'s'd,fdg;sm;lsdf;g,s;d  ;s,g ;,df;gl s;dg, ;sd,;gf,sd; g,",
        "46s4f64s6df4a6sd4f64sdgf654segf654df66dsfg4e4rt65w4t6w4et64ewr6g4sd64fg65ds4fg",
        "sdfasdg4sag64a6g45sd4 64365 46 4d6f4asd64 f6as4 f6as74f987s6543654165JJKK",
        NULL
    };

    // rws_socket_send_text(_socket, "hello test send text!!??##OOOKKKK.");
    rws_socket_send_bin_start(_socket, "start", strlen("start"));
    int i = 0;
    while(text_arr[i] != NULL) {
        rws_socket_send_bin_continue(_socket, text_arr[i], strlen(text_arr[i]));
        usleep(1000);
        i ++;
    }
    rws_socket_send_bin_finish(_socket, "finish", strlen("finish"));

    ret = _AGCTS_SUCCESS;
    return ret;
}

AGCTS_TEST_CASE agcts_librws_test_cases[] = {
    {(char *)"librws set, get API", _agcts_websoc_set_get_test},
    {(char *)"librws connection API", _agcts_websoc_connection_test},
};

void agcts_librws_test()
{
    _AGCTS_TEST_BEGIN

    int i;
    for (i = 0; i < sizeof(agcts_librws_test_cases) / sizeof(AGCTS_TEST_CASE); i++) {
        agcts_run_test(&agcts_librws_test_cases[i]);
    }

    _AGCTS_TEST_END
}
int get_test(int argc, char **argv)
{
	_agcts_websoc_set_get_test();
}

FINSH_FUNCTION_EXPORT_CMD(get_test, rws_get, websocket test);

int connect_test(int argc, char **argv)
{
	_agcts_websoc_connection_test();
}

FINSH_FUNCTION_EXPORT_CMD(connect_test, rws_cnt, websocket test);

