#include <stdlib.h>
#include <string.h>
#include <librws.h>
#include <hal_sem.h>
#include <hal_mem.h>
#include <hal_time.h>
#include <hal_thread.h>
#include "asr_rws_client.h"
#include "asr_demo_config.h"

#define SEM_TIMEOUT_MS		(1000)

struct asr_rws_t {
	asr_rws_get_audio_data_fun_t get_audio_data_fun;
	asr_rws_result_cb_t result_cb;
	void *priv;
	int timeout_ms;
	int id;

	//var
	rws_socket socket;
	int recv_cnt; // owner: rws
	volatile int is_connect; // owner: rws
	volatile int is_disconnect; // owner: rws
	volatile int is_running; // owner: api
	volatile int need_stop; // owner: thread/rws
	volatile int is_timeout; // owner: thread
	void *thread;

	hal_sem_t disconnect_sem; // thread -> rws
	hal_sem_t api_sem; // thread -> api
};

#ifdef CONFIG_ASR_DEMO_NO_PRINT
#define RWS_DEBUG(hdl, fmt, arg...)				do{  }while(0)
#define RWS_INFO(hdl, fmt, arg...)				do{  }while(0)
#define RWS_WARN(hdl, fmt, arg...)				do{  }while(0)
#define RWS_ERROR(hdl, fmt, arg...)				do{  }while(0)
#define RWS_API_ENTRY_DEBUG(hdl, fmt, arg...)	do{  }while(0)
#define RWS_API_EXIT_DEBUG(hdl, fmt, arg...)	do{  }while(0)
#elif 0
#define RWS_DEBUG(hdl, fmt, arg...)				do{ printf("\e[35m[RWS-DEBUG]<%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define RWS_INFO(hdl, fmt, arg...)				do{ printf("\e[34m[RWS-INFO] <%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define RWS_WARN(hdl, fmt, arg...)				do{ printf("\e[33m[RWS-WARN] <%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define RWS_ERROR(hdl, fmt, arg...)				do{ printf("\e[31m[RWS-ERROR]<%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define RWS_API_ENTRY_DEBUG(hdl, fmt, arg...)	do{ printf("\e[35m[RWS-ENTRY]<%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define RWS_API_EXIT_DEBUG(hdl, fmt, arg...)	do{ printf("\e[35m[RWS-EXIT] <%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#else
#define RWS_DEBUG(hdl, fmt, arg...)				do{  }while(0)
#define RWS_INFO(hdl, fmt, arg...)				do{ printf("\e[34m[RWS-INFO] \e[0m hdl: %08llx " fmt "\n", (unsigned long long)hdl, ##arg); }while(0)
#define RWS_WARN(hdl, fmt, arg...)				do{ printf("\e[33m[RWS-WARN] \e[0m hdl: %08llx " fmt "\n", (unsigned long long)hdl, ##arg); }while(0)
#define RWS_ERROR(hdl, fmt, arg...)				do{ printf("\e[31m[RWS-ERROR]\e[0m hdl: %08llx " fmt "\n", (unsigned long long)hdl, ##arg); }while(0)
#define RWS_API_ENTRY_DEBUG(hdl, fmt, arg...)	do{  }while(0)
#define RWS_API_EXIT_DEBUG(hdl, fmt, arg...)	do{  }while(0)
#endif

static inline void asr_rws_set_running(struct asr_rws_t *hdl)
{
	hdl->is_running = 1;
}

static inline void asr_rws_clear_running(struct asr_rws_t *hdl)
{
	hdl->is_running = 0;
}

static inline void asr_rws_set_connect(struct asr_rws_t *hdl)
{
	hdl->is_connect = 1;
}

static inline void asr_rws_set_disconnect(struct asr_rws_t *hdl)
{
	hdl->is_disconnect = 1;
}

static inline void asr_rws_set_need_stop(struct asr_rws_t *hdl)
{
	hdl->need_stop = 1;
}

static inline void asr_rws_set_timeout(struct asr_rws_t *hdl)
{
	hdl->is_timeout = 1;
}

static inline int asr_rws_is_running(struct asr_rws_t *hdl)
{
	return hdl->is_running;
}

static inline int asr_rws_is_connect(struct asr_rws_t *hdl)
{
	return hdl->is_connect;
}

static inline int asr_rws_is_disconnect(struct asr_rws_t *hdl)
{
	return hdl->is_disconnect;
}

static inline int asr_rws_is_need_stop(struct asr_rws_t *hdl)
{
	return hdl->need_stop;
}

static inline int asr_rws_is_timeout(struct asr_rws_t *hdl)
{
	return hdl->is_timeout;
}

static inline int on_process_destroy(struct asr_rws_t *hdl)
{
	RWS_DEBUG(hdl, "sem wait: disconnect");
	while (0 > hal_sem_timedwait(hdl->disconnect_sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS)))
		RWS_ERROR(hdl, "wait disconnect_sem timeout!\n");
	RWS_DEBUG(hdl, "sem get: disconnect");
	rws_socket_set_user_object(hdl->socket, NULL);
	asr_rws_client_destroy(hdl);
}

static inline int on_process_stop(struct asr_rws_t *hdl)
{
	if (asr_rws_is_need_stop(hdl)) {
		RWS_INFO(hdl, "stop");
		if (!hdl->recv_cnt) {
			hdl->result_cb(hdl->priv, NULL, asr_rws_is_timeout(hdl) ? 0 : 1);
		}
		hdl->recv_cnt++;
		RWS_DEBUG(hdl, "stop on %s", rws_socket_is_connected(hdl->socket) ? "connected" : "disconnected");
		asr_rws_set_disconnect(hdl);
		rws_socket_disconnect_and_release(hdl->socket);
		on_process_destroy(hdl);
		return 0;
	}

	return -1;
}

static void on_socket_received_text(rws_socket socket, const char *text, const unsigned int length, bool is_finish)
{
	struct asr_rws_t *hdl = rws_socket_get_user_object(socket);

	RWS_DEBUG(hdl, "");
	if (!hdl || !on_process_stop(hdl))
		return;

	if(hdl) {
		if (!hdl->recv_cnt) {
			hdl->result_cb(hdl->priv, text, length);
		}
		hdl->recv_cnt++;
		asr_rws_set_need_stop(hdl);
		on_process_stop(hdl);
	}
}

static void on_socket_received_bin(rws_socket socket, const void * data, const unsigned int length, bool is_finish)
{
	struct asr_rws_t *hdl = rws_socket_get_user_object(socket);

	RWS_DEBUG(hdl, "");
	if (!hdl || !on_process_stop(hdl))
		return;
}

static void on_socket_process(rws_socket socket)
{
	struct asr_rws_t *hdl = rws_socket_get_user_object(socket);

	RWS_DEBUG(hdl, "");
	if (!hdl || !on_process_stop(hdl))
		return;
}

static void on_socket_received_pong(rws_socket socket)
{
	struct asr_rws_t *hdl = rws_socket_get_user_object(socket);

	RWS_DEBUG(hdl, "");
	if (!hdl || !on_process_stop(hdl))
		return;
}

static void on_socket_connected(rws_socket socket)
{
	struct asr_rws_t *hdl = rws_socket_get_user_object(socket);

	RWS_DEBUG(hdl, "");
	if (hdl) {
		asr_rws_set_connect(hdl);
		if (!on_process_stop(hdl))
			return;
	}
}

static void on_socket_disconnected(rws_socket socket)
{
	struct asr_rws_t *hdl = rws_socket_get_user_object(socket);

#if 1
	rws_error error = rws_socket_get_error(socket);
	if (error) {
		RWS_INFO(hdl, "%s: Socket disconnect with code, error: %i, %s",
				__FUNCTION__,
				rws_error_get_code(error),
				rws_error_get_description(error));
	}
#endif

	RWS_DEBUG(hdl, "");
	if (hdl) {
		asr_rws_set_disconnect(hdl);
		if (!hdl->recv_cnt) {
			hdl->result_cb(hdl->priv, NULL, -1);
		}
		hdl->recv_cnt++;
		on_process_stop(hdl);
	}
}

static void asr_rws_client_thread(void *arg)
{
	struct asr_rws_t *hdl = (struct asr_rws_t *)arg;
	unsigned char buffer[FRAME_SIZE];
	unsigned int time_ms = 0;
	unsigned int total = 0;

	RWS_DEBUG(hdl, "sem post: api");
	if (hal_sem_post(hdl->api_sem))
		RWS_ERROR(hdl, "post api_sem failed!");

#if !defined(RWS_APPVEYOR_CI)
	RWS_INFO(hdl, "connecting server...");
	rws_socket_connect(hdl->socket);
#endif

	while(!asr_rws_is_connect(hdl) && !asr_rws_is_disconnect(hdl) && asr_rws_is_running(hdl) && time_ms < hdl->timeout_ms) {
		hal_msleep(DELAY_PERIOD_MIN);
		time_ms += DELAY_PERIOD_MIN;
	}
	if (!asr_rws_is_running(hdl) || asr_rws_is_disconnect(hdl)) {
		RWS_INFO(hdl, "not connect");
		goto exit;
	}

	RWS_INFO(hdl, "connected");
	rws_socket_send_text(hdl->socket, ONLINE_ASR_START_TEXT);

	while(!asr_rws_is_disconnect(hdl) && asr_rws_is_running(hdl) && time_ms < hdl->timeout_ms) {
		int len = hdl->get_audio_data_fun(hdl->priv, buffer, FRAME_SIZE);
		if (asr_rws_is_disconnect(hdl))
			break;
		if(len < 0) {
			RWS_INFO(hdl, "%s EOS, total %u bytes", __func__, total);
			rws_socket_send_text(hdl->socket, ONLINE_ASR_END_TEXT);
			break;
		} else if (!len) {
			hal_msleep(DELAY_PERIOD_MIN);
			time_ms += DELAY_PERIOD_MIN;
		} else {
			unsigned int sleep_ms = DELAY_PERIOD * len / FRAME_SIZE;
			sleep_ms = sleep_ms < DELAY_PERIOD_MIN ? DELAY_PERIOD_MIN : sleep_ms;
			//RWS_DEBUG(hdl, "sen bin len: %d\n", len);
			total += len;
			rws_socket_send_bin_start(hdl->socket, buffer, len);
			rws_socket_send_bin_finish(hdl->socket, buffer, 0);
			hal_msleep(DELAY_PERIOD * len / FRAME_SIZE);
			time_ms += DELAY_PERIOD * len / FRAME_SIZE;
		}
	}
	while(!asr_rws_is_disconnect(hdl) && asr_rws_is_running(hdl) && time_ms < hdl->timeout_ms) {
		hal_msleep(DELAY_PERIOD_MIN);
		time_ms += DELAY_PERIOD_MIN;
	}
exit:
	if (time_ms >= hdl->timeout_ms) {
		asr_rws_set_timeout(hdl);
		RWS_INFO(hdl, "%s timeout", __func__);
	}
	RWS_INFO(hdl, "%s wait stop", __func__);
	asr_rws_set_need_stop(hdl);
	while(asr_rws_is_running(hdl)) {
		hal_msleep(DELAY_PERIOD_MIN);
		time_ms += DELAY_PERIOD_MIN;
	}
	RWS_DEBUG(hdl, "sem post: api");
	if (hal_sem_post(hdl->api_sem))
		RWS_ERROR(hdl, "post api_sem failed!");

	while(!asr_rws_is_disconnect(hdl)) {
		hal_msleep(DELAY_PERIOD_MIN);
		time_ms += DELAY_PERIOD_MIN;
	}
	RWS_DEBUG(hdl, "sem post: disconnect");
	if (hal_sem_post(hdl->disconnect_sem))
		RWS_ERROR(hdl, "post disconnect_sem failed!");

	RWS_INFO(hdl, "%s exit, total %ums", __func__, time_ms);
	hal_thread_stop(NULL);
}

int asr_rws_client_config(void *_hdl, const struct asr_rws_client_config_t *config)
{
	struct asr_rws_t *hdl = (struct asr_rws_t *)_hdl;

	RWS_API_ENTRY_DEBUG(hdl, "");
	hdl->socket = rws_socket_create();
	RWS_DEBUG(hdl, "socket: %p\n", hdl->socket);
	if (!hdl->socket) {
		RWS_ERROR(hdl, "rws_socket_create failed!");
		RWS_API_EXIT_DEBUG(hdl, "");
		return -1;
	}

	hdl->get_audio_data_fun = config->get_audio_data_fun;
	hdl->result_cb = config->result_cb;
	hdl->priv = config->priv;
	hdl->timeout_ms = config->timeout_ms;
	hdl->id = config->id;
	rws_socket_set_user_object(hdl->socket, hdl);
	rws_socket_set_scheme(hdl->socket, config->scheme);
	rws_socket_set_host(hdl->socket, config->host);
	rws_socket_set_path(hdl->socket, config->path);
	rws_socket_set_port(hdl->socket, config->port);
#ifdef WEBSOCKET_SSL_ENABLE
	if (config->cert) {
		rws_socket_set_server_cert(hdl->socket, config->cert, strlen(config->cert) + 1);
	}
#endif

	rws_socket_set_on_socket_process(hdl->socket, &on_socket_process);
	rws_socket_set_on_disconnected(hdl->socket, &on_socket_disconnected);
	rws_socket_set_on_connected(hdl->socket, &on_socket_connected);
	rws_socket_set_on_received_text(hdl->socket, &on_socket_received_text);
	rws_socket_set_on_received_bin(hdl->socket, &on_socket_received_bin);
	rws_socket_set_on_received_pong(hdl->socket, &on_socket_received_pong);

	RWS_API_EXIT_DEBUG(hdl, "");
	return 0;
}

int asr_rws_get_id(void *_hdl)
{
	struct asr_rws_t *hdl = (struct asr_rws_t *)_hdl;

	return hdl->id;
}

static unsigned int thread_cnt = 0;
int asr_rws_client_start(void *_hdl)
{
	struct asr_rws_t *hdl = (struct asr_rws_t *)_hdl;
	char name[16];

	RWS_API_ENTRY_DEBUG(hdl, "");
	memset(name, 0, 16);
	snprintf(name, 16, "asr_rws_t%u", thread_cnt++);
	asr_rws_set_running(hdl);
	hdl->thread = hal_thread_create(asr_rws_client_thread, hdl, name, 1024 + (FRAME_SIZE + 4 - 1) / 4 , 5);
	if (!hdl->thread) {
		RWS_ERROR(hdl, "create thread failed!\n");
		RWS_API_EXIT_DEBUG(hdl, "");
		return -1;
	}
	hal_thread_start(hdl->thread);

	RWS_DEBUG(hdl, "sem wait: api");
	while (0 > hal_sem_timedwait(hdl->api_sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS)))
		RWS_ERROR(hdl, "wait api_sem timeout!\n");
	RWS_DEBUG(hdl, "sem get: api");

	RWS_API_EXIT_DEBUG(hdl, "");
	return 0;
}

int asr_rws_client_wait_stop(void *_hdl, int timeout_ms)
{
	struct asr_rws_t *hdl = (struct asr_rws_t *)_hdl;

	RWS_API_ENTRY_DEBUG(hdl, "");

	RWS_DEBUG(hdl, "sem wait: api");
	while (0 > hal_sem_timedwait(hdl->api_sem, pdMS_TO_TICKS(timeout_ms)))
		RWS_ERROR(hdl, "wait api_sem timeout!\n");
	RWS_DEBUG(hdl, "sem get: api");

	RWS_API_EXIT_DEBUG(hdl, "");
	return 0;
}

int asr_rws_client_stop(void *_hdl, int force)
{
	struct asr_rws_t *hdl = (struct asr_rws_t *)_hdl;

	RWS_API_ENTRY_DEBUG(hdl, "");

	asr_rws_clear_running(hdl);
	if (force) {
		RWS_DEBUG(hdl, "socket: %p\n", hdl->socket);
		rws_socket_disconnect_and_release(hdl->socket);
	}

	RWS_API_EXIT_DEBUG(hdl, "");
	return 0;
}

void asr_rws_client_destroy(void *_hdl)
{
	struct asr_rws_t *hdl = (struct asr_rws_t *)_hdl;

	RWS_API_ENTRY_DEBUG(hdl, "");
	if (hdl) {
		if (hdl->api_sem) {
			hal_sem_delete(hdl->api_sem);
			hdl->api_sem = NULL;
		}
		if (hdl->disconnect_sem) {
			hal_sem_delete(hdl->disconnect_sem);
			hdl->disconnect_sem = NULL;
		}
		hal_free(hdl);
	}
	RWS_API_EXIT_DEBUG(hdl, "");
}

void *asr_rws_client_create(void)
{
	struct asr_rws_t *hdl = hal_malloc(sizeof(*hdl));
	RWS_API_ENTRY_DEBUG(hdl, "");

	if (!hdl) {
		RWS_ERROR(hdl, "no memory!\n");
		goto err;
	}

	memset(hdl, 0, sizeof(*hdl));
	hdl->api_sem = hal_sem_create(0);
	if (!hdl->api_sem) {
		RWS_ERROR(hdl, "no memory!\n");
		goto err;
	}

	hdl->disconnect_sem = hal_sem_create(0);
	if (!hdl->disconnect_sem) {
		RWS_ERROR(hdl, "no memory!\n");
		goto err;
	}

	RWS_API_EXIT_DEBUG(hdl, "");
	return hdl;
err:
	asr_rws_client_destroy(hdl);
	RWS_API_EXIT_DEBUG(hdl, "");
	return NULL;
}
