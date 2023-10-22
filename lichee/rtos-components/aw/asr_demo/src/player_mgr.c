#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_sem.h>
#include <hal_mem.h>
#include <hal_thread.h>
#include <hal_queue.h>
#include <hal_time.h>
#include <hal_mutex.h>
#include <aw_list.h>
#include "rtplayer.h"
//#define CONFIG_PLAYER_MGR_TEST_CMD
#ifdef CONFIG_PLAYER_MGR_TEST_CMD
#include <console.h>
#endif
#include "player_mgr.h"

#include "asr_demo_config.h"
#ifdef CONFIG_ASR_DEMO_NO_PRINT
#define PLAYER_DEBUG(hdl, fmt, arg...)				do{  }while(0)
#define PLAYER_INFO(hdl, fmt, arg...)				do{  }while(0)
#define PLAYER_WARN(hdl, fmt, arg...)				do{  }while(0)
#define PLAYER_ERROR(hdl, fmt, arg...)				do{  }while(0)
#define PLAYER_API_ENTRY_DEBUG(hdl, fmt, arg...)	do{  }while(0)
#define PLAYER_API_EXIT_DEBUG(hdl, fmt, arg...)		do{  }while(0)
#elif 0
#define PLAYER_DEBUG(hdl, fmt, arg...)				do{ printf("\e[35m[PLAYER-DEBUG]<%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define PLAYER_INFO(hdl, fmt, arg...)				do{ printf("\e[34m[PLAYER-INFO] <%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define PLAYER_WARN(hdl, fmt, arg...)				do{ printf("\e[33m[PLAYER-WARN] <%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define PLAYER_ERROR(hdl, fmt, arg...)				do{ printf("\e[31m[PLAYER-ERROR]<%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define PLAYER_API_ENTRY_DEBUG(hdl, fmt, arg...)	do{ printf("\e[35m[PLAYER-ENTRY]<%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#define PLAYER_API_EXIT_DEBUG(hdl, fmt, arg...)		do{ printf("\e[35m[PLAYER-EXIT] <%s:%4d:%08llx>\e[0m" fmt "\n", __func__, __LINE__, (unsigned long long)hdl, ##arg); }while(0)
#else
#define PLAYER_DEBUG(hdl, fmt, arg...)				do{  }while(0)
#define PLAYER_INFO(hdl, fmt, arg...)				do{ printf("\e[34m[PLAYER-INFO] \e[0m hdl: %08llx " fmt "\n", (unsigned long long)hdl, ##arg); }while(0)
#define PLAYER_WARN(hdl, fmt, arg...)				do{ printf("\e[33m[PLAYER-WARN] \e[0m hdl: %08llx " fmt "\n", (unsigned long long)hdl, ##arg); }while(0)
#define PLAYER_ERROR(hdl, fmt, arg...)				do{ printf("\e[31m[PLAYER-ERROR]\e[0m hdl: %08llx " fmt "\n", (unsigned long long)hdl, ##arg); }while(0)
#define PLAYER_API_ENTRY_DEBUG(hdl, fmt, arg...)	do{  }while(0)
#define PLAYER_API_EXIT_DEBUG(hdl, fmt, arg...)		do{  }while(0)
#endif

struct res_info_t {
	struct list_head list;
	char *url;
	int opt_id;
	int flags;
	hal_sem_t sem;
};

struct player_t {
	RTPlayer *hdl;
	int status;
	int opt_id;
	hal_mutex_t mutex;
	struct list_head res_list;
	struct res_info_t *cur_res;
};

struct player_mgr_t {
	struct player_t player[MAX_PLAYER_NUM];

#ifdef AUTO_PAUSE_RES_PLAYER
	int auto_resume_res_player;
	int pause_res_player;
#endif
	void *proxy_thread;
	hal_queue_t proxy_queue;
};

struct player_mgr_proxy_cmd_t {
	struct player_mgr_t *hdl;
	int player_id;
	int opt_id;
	int (*fun)(struct player_mgr_t *, int);
};

static inline const char *player_id_to_str(int player_id)
{
	switch(player_id) {
	case TTS_PLAYER:
		return "TTS_PLAYER";
	case RES_PLAYER:
		return "RES_PLAYER";
	default:
		return "unknown player";
	}
}

static inline const char *player_status_to_str(int status)
{
	switch(status) {
	case PLAYER_STATUS_ERROR:
		return "PLAYER_STATUS_ERROR";
	case PLAYER_STATUS_NULL:
		return "PLAYER_STATUS_NULL";
	case PLAYER_STATUS_RESET:
		return "PLAYER_STATUS_RESET";
	case PLAYER_STATUS_REQUEST:
		return "PLAYER_STATUS_REQUEST";
	case PLAYER_STATUS_SET_URL:
		return "PLAYER_STATUS_SET_URL";
	case PLAYER_STATUS_PREPARE:
		return "PLAYER_STATUS_PREPARE";
	case PLAYER_STATUS_PLAYING:
		return "PLAYER_STATUS_PLAYING";
	case PLAYER_STATUS_PAUSE:
		return "PLAYER_STATUS_PAUSE";
	case PLAYER_STATUS_COMPLETE:
		return "PLAYER_STATUS_COMPLETE";
	case PLAYER_STATUS_STOP:
		return "PLAYER_STATUS_STOP";
	default:
		return "unknown";
	}
}

static int player_reset(struct player_mgr_t *hdl, int player_id);
static int player_prepare(struct player_mgr_t *hdl, int player_id);
static int player_start(struct player_mgr_t *hdl, int player_id);
static int player_pause(struct player_mgr_t *hdl, int player_id);
static inline int proxy_ctl(struct player_mgr_t *hdl, int player_id, int id, int (*fun)(struct player_mgr_t *, int));

static volatile int opt_id = 0;
static inline int get_opt_id(struct player_mgr_t *hdl)
{
	return opt_id++;
}

static inline RTPlayer *get_player(struct player_mgr_t *hdl, int player_id)
{
	if (player_id >= MAX_PLAYER_NUM) {
		PLAYER_ERROR(hdl, "unknown player_id! %d", player_id);
		return NULL;
	}

	return hdl->player[player_id].hdl;
}

static inline hal_mutex_t get_player_lock(struct player_mgr_t *hdl, int player_id)
{
	if (player_id >= MAX_PLAYER_NUM) {
		PLAYER_ERROR(hdl, "unknown player_id! %d", player_id);
		return NULL;
	}

	return hdl->player[player_id].mutex;
}

static inline int player_mutex_lock(hal_mutex_t mutex)
{
#if 1
	PLAYER_DEBUG(hal_thread_self(), "%s try...", __func__);
	int ret = hal_mutex_lock(mutex);
	PLAYER_DEBUG(hal_thread_self(), "%s get", __func__);
	return ret;
#else
	return hal_mutex_lock(mutex);
#endif
}

static inline int player_mutex_unlock(hal_mutex_t mutex)
{
	PLAYER_DEBUG(hal_thread_self(), "%s", __func__);
	return hal_mutex_unlock(mutex);
}

static inline int get_player_opt_id(struct player_mgr_t *hdl, int player_id)
{
	if (player_id >= MAX_PLAYER_NUM) {
		PLAYER_ERROR(hdl, "unknown player_id! %d", player_id);
		return -1;
	}

	return hdl->player[player_id].opt_id;
}

static inline int set_player_opt_id(struct player_mgr_t *hdl, int player_id, int opt_id)
{
	if (player_id >= MAX_PLAYER_NUM) {
		PLAYER_ERROR(hdl, "unknown player_id! %d", player_id);
		return -1;
	}

	hdl->player[player_id].opt_id = opt_id;

	return 0;
}

static inline int *get_player_status_ptr(struct player_mgr_t *hdl, int player_id)
{
	if (player_id >= MAX_PLAYER_NUM) {
		PLAYER_ERROR(hdl, "unknown player_id! %d", player_id);
		return NULL;
	}

	return &hdl->player[player_id].status;
}

static inline void update_player_status(struct player_mgr_t *hdl, int player_id, int status)
{
	int *p_status = get_player_status_ptr(hdl, player_id);

	int old_status = PLAYER_STATUS_ERROR;

	//lock
	if(p_status) {
		old_status = *p_status;
		*p_status = status;
	}

	//unlock
	PLAYER_INFO(hdl, "%s %s to %s", player_id_to_str(player_id), player_status_to_str(old_status), player_status_to_str(status));
}

static inline int get_player_status(struct player_mgr_t *hdl, int player_id)
{
	int *p_status = get_player_status_ptr(hdl, player_id);

	//lock
	if(p_status)
		return *p_status;
	else
		return PLAYER_STATUS_ERROR;
	//unlock
}

static inline int set_player_status(struct player_mgr_t *hdl, int player_id)
{
	int *p_status = get_player_status_ptr(hdl, player_id);

	//lock
	if(p_status)
		return *p_status;
	else
		return PLAYER_STATUS_ERROR;
	//unlock
}

void destroy_res_info(struct res_info_t *res)
{
	if (res) {
		PLAYER_DEBUG(res, "destroy (%d)url: %s", res->opt_id, res->url);
		if (res->url) {
			hal_free(res->url);
			res->url = NULL;
		}
		hal_free(res);
	}
}

struct res_info_t *create_res_info(const char *url, int flags, void *sem, int opt_id)
{
	struct res_info_t *res = hal_malloc(sizeof(*res));
	int len;

	if (!res) {
		printf("no memory!\n");
		goto err;
	}

	memset(res, 0, sizeof(*res));
	len = strlen(url);
	res->url = hal_malloc(len + 1);
	if (!res->url) {
		printf("no memory!\n");
		goto err;
	}
	memcpy(res->url, url, len + 1);

	INIT_LIST_HEAD(&res->list);
	res->flags = flags;
	res->opt_id = opt_id;
	if (flags & RES_WAIT_COMPLETE)
		res->sem = sem;
	PLAYER_DEBUG(NULL, "create (%d)url: %s", res->opt_id, res->url);
	return res;
err:
	destroy_res_info(res);
	return NULL;
}

static inline void clean_playlist(struct player_mgr_t *hdl, int player_id)
{
	struct list_head *list = &hdl->player[player_id].res_list;
	struct list_head *pos, *q;
	struct res_info_t *res;

	if (list_empty(list))
		return;

	PLAYER_DEBUG(hdl, "%s %s", player_id_to_str(player_id), __func__);
	list_for_each_safe(pos, q, list)
	{
		res = list_entry(pos, struct res_info_t, list);
		list_del(&res->list);
		if (res->sem) {
			if (hal_sem_post(res->sem))
				printf("hal_sem_post failed!\n");
		}
		PLAYER_DEBUG(hdl, "%s del (%d)url: %s", player_id_to_str(player_id), res->opt_id, res->url);
		destroy_res_info(res);
	}
}

static inline void add_res_to_playlist_head(struct player_mgr_t *hdl, int player_id, struct res_info_t *res)
{
	struct list_head *list = &hdl->player[player_id].res_list;

	list_add(&res->list, list);
	PLAYER_DEBUG(hdl, "%s head (%d)url: %s", player_id_to_str(player_id), res->opt_id, res->url);
}

static inline void add_res_to_playlist_tail(struct player_mgr_t *hdl, int player_id, struct res_info_t *res)
{
	struct list_head *list = &hdl->player[player_id].res_list;

	list_add_tail(&res->list, list);
	PLAYER_DEBUG(hdl, "%s tail (%d)url: %s", player_id_to_str(player_id), res->opt_id, res->url);
}

static inline struct res_info_t *get_res_from_playlist(struct player_mgr_t *hdl, int player_id)
{
	struct list_head *list = &hdl->player[player_id].res_list;
	struct list_head *pos, *q;
	struct res_info_t *res;

	if (list_empty(list))
		res = NULL;

	list_for_each_safe(pos, q, list)
	{
		res = list_entry(pos, struct res_info_t, list);
		list_del(&res->list);
		PLAYER_DEBUG(hdl, "%s get (%d)url: %s", player_id_to_str(player_id), res->opt_id, res->url);
		break;
	}

	return res;
}

static int player_prepare(struct player_mgr_t *hdl, int player_id)
{
	RTPlayer *player = get_player(hdl, player_id);
	hal_mutex_t mutex = get_player_lock(hdl, player_id);
	struct res_info_t *res;
	int status;
	int ret = 0;

	player_mutex_lock(mutex);

	status = get_player_status(hdl, player_id);
	res = hdl->player[player_id].cur_res;
	if(PLAYER_STATUS_REQUEST != get_player_status(hdl, player_id)) {
		PLAYER_ERROR(hdl, "%s status error! %s", player_id_to_str(player_id), player_status_to_str(status));
		proxy_ctl(hdl, player_id, -1, player_reset);
		goto exit;
	}

	set_player_opt_id(hdl, player_id, res->opt_id);
	setDataSource_url(player, hdl, res->url, res->opt_id);
	update_player_status(hdl, player_id, PLAYER_STATUS_SET_URL);
	player_mutex_unlock(mutex);

	//can not lock
	if (0 > prepareAsync(player)) {
		player_mutex_lock(mutex);
		PLAYER_ERROR(hdl, "%s prepareAsync failed!", player_id_to_str(player_id));
		proxy_ctl(hdl, player_id, -1, player_reset);
		player_mutex_unlock(mutex);
		ret = -1;
		goto exit;
	}

exit:
#ifdef AUTO_PAUSE_RES_PLAYER
	if(player_id == TTS_PLAYER && MAX_PLAYER_NUM > RES_PLAYER) {
		hdl->pause_res_player = 1;
		player_pause(hdl, RES_PLAYER);
	}
#endif
	return ret;
}

static int player_request_no_lock(struct player_mgr_t *hdl, int player_id)
{
	int status = get_player_status(hdl, player_id);
	struct res_info_t *res;

	if (PLAYER_STATUS_RESET != status) {
		PLAYER_INFO(hdl, "%s status error! %s", player_id_to_str(player_id), player_status_to_str(status));
		return -1;
	}

	res = get_res_from_playlist(hdl, player_id);
	if (!res) {
		PLAYER_INFO(hdl, "%s playlist empty!", player_id_to_str(player_id));
		return -1;
	}

	hdl->player[player_id].cur_res = res;
	update_player_status(hdl, player_id, PLAYER_STATUS_REQUEST);

	return 0;
}

static int player_reset(struct player_mgr_t *hdl, int player_id)
{
	RTPlayer *player = get_player(hdl, player_id);
	struct res_info_t *res;
	hal_mutex_t mutex = get_player_lock(hdl, player_id);
	int status;

	player_mutex_lock(mutex);
	status = get_player_status(hdl, player_id);
	if(PLAYER_STATUS_RESET != status) {
		res = hdl->player[player_id].cur_res;
		//res理应是有值的，没有就直接崩溃
		hdl->player[player_id].cur_res = NULL;
		if (res->sem) {
			if (hal_sem_post(res->sem))
				printf("hal_sem_post failed!\n");
		}
		destroy_res_info(res);
		reset(player);
		update_player_status(hdl, player_id, PLAYER_STATUS_RESET);
	}

	//如果有可以播放的资源，在临界区内就不会获取到RESET状态
	if (!player_request_no_lock(hdl, player_id))
		proxy_ctl(hdl, player_id, -1, player_prepare);
exit:
	player_mutex_unlock(mutex);
	return 0;
}

static int player_start(struct player_mgr_t *hdl, int player_id)
{
	RTPlayer *player = get_player(hdl, player_id);
	hal_mutex_t mutex = get_player_lock(hdl, player_id);
	int ret = 0;
	int status;

	player_mutex_lock(mutex);
	status = get_player_status(hdl, player_id);
	if(PLAYER_STATUS_PREPARE == status || PLAYER_STATUS_PAUSE == status) {
		if (0 > start(player)) {
			PLAYER_ERROR(hdl, "%s start failed!", player_id_to_str(player_id));
			proxy_ctl(hdl, player_id, -1, player_reset);
			ret = -1;
			goto exit;
		}
		update_player_status(hdl, player_id, PLAYER_STATUS_PLAYING);
	}
exit:
	player_mutex_unlock(mutex);
	return ret;
}

static int player_pause(struct player_mgr_t *hdl, int player_id)
{
	int ret = 0;
	RTPlayer *player = get_player(hdl, player_id);
	hal_mutex_t mutex = get_player_lock(hdl, player_id);

	player_mutex_lock(mutex);
	if(PLAYER_STATUS_PLAYING == get_player_status(hdl, player_id)) {
		if (0 > pause_l(player)) {
			PLAYER_ERROR(hdl, "%s pause failed!", player_id_to_str(player_id));
			proxy_ctl(hdl, player_id, -1, player_reset);
			ret = -1;
			goto exit;
		}
		update_player_status(hdl, player_id, PLAYER_STATUS_PAUSE);
	}

exit:
	player_mutex_unlock(mutex);
	return ret;
}

static inline int proxy_ctl(struct player_mgr_t *hdl, int player_id, int id, int (*fun)(struct player_mgr_t *, int))
{
	struct player_mgr_proxy_cmd_t cmd = {
		.hdl = hdl,
		.player_id = player_id,
		.opt_id = id,
		.fun = fun,
	};
	if(0 > hal_queue_send_wait(hdl->proxy_queue, &cmd, 200)) {
		PLAYER_ERROR(hdl, "proxy_ctl sned failed!");
		return -1;
	}

	return 0;
}

static inline void player_cb(int player_id, void* userData, int msg, int id, int ext1, int ext2)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)userData;
	RTPlayer *player = get_player(hdl, player_id);
	hal_mutex_t mutex = get_player_lock(hdl, player_id);
	int opt_id = get_player_opt_id(hdl, player_id);
	struct res_info_t *cur_res;

#if 1
	if (id < opt_id) {
		PLAYER_ERROR(hdl, "%d<%d, check code!", id, opt_id);
		PLAYER_ERROR(hdl, "%d<%d, check code!", id, opt_id);
		PLAYER_ERROR(hdl, "%d<%d, check code!", id, opt_id);
	}
#else
	if (id < opt_id) {
		PLAYER_INFO(hdl, "%d<%d, ignore this cb", id, opt_id);
		//PLAYER_DEBUG(hdl, "%s msg:%d id:%d ext1:%d ext2:%x", player_id_to_str(player_id), msg, id, ext1, ext2);
		PLAYER_INFO(hdl, "%s msg:%d id:%d ext1:%d ext2:%x", player_id_to_str(player_id), msg, id, ext1, ext2);
		return;
	}
#endif

	PLAYER_DEBUG(hdl, "%s msg:%d id:%d ext1:%d ext2:%x", player_id_to_str(player_id), msg, id, ext1, ext2);
	switch (msg) {
	case RTPLAYER_NOTIFY_MEDIA_ERROR:
		PLAYER_ERROR(hdl, "%s res midea error!", player_id_to_str(player_id));
	case RTPLAYER_NOTIFY_PLAYBACK_COMPLETE:
#ifdef AUTO_PAUSE_RES_PLAYER
		if (player_id == TTS_PLAYER) {
			if (hdl->pause_res_player && hdl->auto_resume_res_player) {
				int status = get_player_status(hdl, RES_PLAYER);
				if(PLAYER_STATUS_PAUSE == status || PLAYER_STATUS_PREPARE == status) {
					hdl->pause_res_player = 0;
					proxy_ctl(hdl, RES_PLAYER, id, player_start);
					PLAYER_INFO(hdl, "resume res player");
				} else {
					PLAYER_INFO(hdl, "%s not in %s", player_id_to_str(RES_PLAYER), player_status_to_str(PLAYER_STATUS_PAUSE));
				}
			} else {
				PLAYER_INFO(hdl, "not need to resume res player %d %d", hdl->pause_res_player, hdl->auto_resume_res_player);
			}
		}
#endif
		player_mutex_lock(mutex);
		update_player_status(hdl, player_id, PLAYER_STATUS_COMPLETE);
		proxy_ctl(hdl, player_id, id, player_reset);
		player_mutex_unlock(mutex);
		break;
	case RTPLAYER_NOTIFY_PREPARED:
		player_mutex_lock(mutex);
		update_player_status(hdl, player_id, PLAYER_STATUS_PREPARE);
		player_mutex_unlock(mutex);
#ifdef AUTO_PAUSE_RES_PLAYER
		if(player_id == RES_PLAYER && hdl->pause_res_player && hdl->auto_resume_res_player) {
			PLAYER_INFO(hdl, "not need to start res player");
			break;
		}
#endif
		proxy_ctl(hdl, player_id, id, player_start);
		break;
	case RTPLAYER_NOTIFY_SEEK_COMPLETE:
	case RTPLAYER_NOTIFY_NOT_SEEKABLE:
	case RTPLAYER_NOTIFY_DETAIL_INFO:
		break;
	}
}

static void player_mgr_proxy_thread(void *arg)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)arg;
	struct player_mgr_proxy_cmd_t cmd;

	PLAYER_DEBUG(hdl, "%s enter", __func__);

	while(1) {
		int ret = hal_queue_recv(hdl->proxy_queue, &cmd, 200);

		if(ret < 0) {
			//hal_msleep(10);
			continue;
		}
		if (cmd.hdl != hdl) {
			PLAYER_ERROR(hdl, "%s hdl error! %p != %p", __func__, cmd.hdl, hdl);
			break;
		}
		if (!cmd.fun) {
			PLAYER_INFO(hdl, "%s need exit", __func__);
			break;
		}
		cmd.fun(cmd.hdl, cmd.player_id);
	}

	PLAYER_DEBUG(hdl, "%s exit", __func__);
	hdl->proxy_thread = NULL;
	hal_thread_stop(NULL);
}

static void tts_player_cb(void* userData, int msg, int id, int ext1, int ext2)
{
	player_cb(TTS_PLAYER, userData, msg, id, ext1, ext2);
}

static void res_player_cb(void* userData, int msg, int id, int ext1, int ext2)
{
	player_cb(RES_PLAYER, userData, msg, id, ext1, ext2);
}

static inline void *get_player_cb(int player_id)
{
	if (player_id >= MAX_PLAYER_NUM) {
		PLAYER_ERROR(NULL, "unknown player_id! %d", player_id);
		return NULL;
	}

	switch(player_id) {
	case TTS_PLAYER:
		return tts_player_cb;
	case RES_PLAYER:
		return res_player_cb;
	default:
		return NULL;
	}
}

static inline int play_url(struct player_mgr_t *hdl, int player_id, const char *url, int flags, void *sem)
{
	RTPlayer *player = get_player(hdl, player_id);
	int opt_id = get_opt_id(hdl);
	hal_mutex_t mutex = get_player_lock(hdl, player_id);
	struct res_info_t *res = NULL;
	int status;
	int need_reset = 0;

	if (url)
		res = create_res_info(url, flags, sem, opt_id);

	player_mutex_lock(mutex);
	if (flags & _RES_CLEAN) {
		clean_playlist(hdl, player_id);
	}
	if (res) {
		if (flags & _RES_ADD_TO_BACK) {
			add_res_to_playlist_tail(hdl, player_id, res);
		} else {
			add_res_to_playlist_head(hdl, player_id, res);
		}
	}

	status = get_player_status(hdl, player_id);
	if(PLAYER_STATUS_RESET == status) {
		//列表播放已经停止，执行player_reset后半部的工作
		if (!player_request_no_lock(hdl, player_id))
			proxy_ctl(hdl, player_id, -1, player_prepare);
	} else if(PLAYER_STATUS_COMPLETE != status) {
		//需要考虑是否通过reset停止当前播放
		if (flags & _RES_FORCE_PLAY) {
			proxy_ctl(hdl, player_id, -1, player_reset);
		} else if ((flags & _RES_PLAY_NOW)
					&& !(hdl->player[player_id].cur_res->flags & RES_IMPORTANT)) {
			proxy_ctl(hdl, player_id, -1, player_reset);
		}
	}

	player_mutex_unlock(mutex);
	return opt_id;
}

void *player_mgr_create_sem(void)
{
	hal_sem_t sem;

	PLAYER_API_ENTRY_DEBUG(NULL, "");

	sem = hal_sem_create(0);
	if (!sem) {
		printf("no memory!\n");
	}

	PLAYER_API_EXIT_DEBUG(NULL, "");
	return sem;
}

void player_mgr_destroy_sem(void *_sem)
{
	hal_sem_t sem = (hal_sem_t)_sem;

	PLAYER_API_ENTRY_DEBUG(NULL, "");

	if (sem) {
		hal_sem_delete(sem);
	}

	PLAYER_API_EXIT_DEBUG(NULL, "");
}

int player_mgr_wait_sem(void *_sem, int wait_ms)
{
	hal_sem_t sem = (hal_sem_t)_sem;
	int ret;

	PLAYER_API_ENTRY_DEBUG(NULL, "");

	if (sem && wait_ms >= 0)
		ret = hal_sem_timedwait(sem, pdMS_TO_TICKS(wait_ms));
	else if (sem && wait_ms < 0)
		ret = hal_sem_wait(sem);
	else
		ret = -1;

	PLAYER_API_EXIT_DEBUG(NULL, "");

	return ret;
}

int player_mgr_reset_tts(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;
	int ret;

	PLAYER_API_ENTRY_DEBUG(hdl, "");
	PLAYER_INFO(hdl, "%s", __func__);
	ret = proxy_ctl(hdl, TTS_PLAYER, -1, player_reset);

	PLAYER_API_EXIT_DEBUG(hdl, "");
	return ret;
}

int player_mgr_reset_res(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;
	int ret;

	PLAYER_API_ENTRY_DEBUG(hdl, "");
	PLAYER_INFO(hdl, "%s", __func__);

	if (MAX_PLAYER_NUM <= RES_PLAYER) {
		PLAYER_API_EXIT_DEBUG(hdl, "");
		return 0;
	}

	ret = proxy_ctl(hdl, RES_PLAYER, -1, player_reset);

	PLAYER_API_EXIT_DEBUG(hdl, "");
	return ret;
}

int player_mgr_play_tts(void *_hdl, const char *url, int flags, void *sem)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;
	int ret;

	PLAYER_API_ENTRY_DEBUG(hdl, "");
	PLAYER_INFO(hdl, "%s url: %s", __func__, url ? url : "(NULL)");
	ret = play_url(hdl, TTS_PLAYER, url, flags, sem);
	PLAYER_API_EXIT_DEBUG(hdl, "");
	return ret;
}

int player_mgr_play_res(void *_hdl, const char *url, int flags, void *sem)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;
	int ret;

	PLAYER_API_ENTRY_DEBUG(hdl, "");
	PLAYER_INFO(hdl, "%s url: %s", __func__, url ? url : "(NULL)");

	if (MAX_PLAYER_NUM <= RES_PLAYER) {
		PLAYER_API_EXIT_DEBUG(hdl, "");
		return 0;
	}

	ret = play_url(hdl, RES_PLAYER, url, flags, sem);
	PLAYER_API_EXIT_DEBUG(hdl, "");
	return ret;
}

int player_mgr_pause_tts(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;
	int ret;

	PLAYER_API_ENTRY_DEBUG(hdl, "");
	PLAYER_INFO(hdl, "%s", __func__);

	ret = proxy_ctl(hdl, TTS_PLAYER, -1, player_pause);

	PLAYER_API_EXIT_DEBUG(hdl, "");
	return ret;
}

int player_mgr_pause_res(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;
	int ret;

	PLAYER_API_ENTRY_DEBUG(hdl, "");
	PLAYER_INFO(hdl, "%s", __func__);

	if (MAX_PLAYER_NUM <= RES_PLAYER) {
		PLAYER_API_EXIT_DEBUG(hdl, "");
		return 0;
	}

	ret = proxy_ctl(hdl, RES_PLAYER, -1, player_pause);

	PLAYER_API_EXIT_DEBUG(hdl, "");
	return ret;
}

int player_mgr_get_tts_status(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;

	return get_player_status(hdl, TTS_PLAYER);
}

int player_mgr_get_res_status(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;

	return get_player_status(hdl, RES_PLAYER);
}

#ifdef AUTO_PAUSE_RES_PLAYER
int player_mgr_disable_auto_resume_res_player(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;

	hdl->auto_resume_res_player = 0;
	PLAYER_INFO(hdl, "disable auto resume res player");

	return 0;
}

int player_mgr_enable_auto_resume_res_player(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;

	hdl->auto_resume_res_player = 1;
	PLAYER_INFO(hdl, "enable auto resume res player");

	return 0;
}
#endif

void player_mgr_destroy(void *_hdl)
{
	struct player_mgr_t *hdl = (struct player_mgr_t *)_hdl;

	PLAYER_API_ENTRY_DEBUG(hdl, "");
	if(hdl) {
		if (hdl->proxy_queue) {
			proxy_ctl(hdl, 0, 0, NULL);
			while(hdl->proxy_thread)
				hal_msleep(10);
		}
		for (int i = 0; i < MAX_PLAYER_NUM; i++) {
			if (hdl->player[i].hdl) {
				player_deinit(hdl->player[i].hdl);
				hdl->player[i].hdl = NULL;
			}
			if (hdl->player[i].mutex) {
				hal_mutex_delete(hdl->player[i].mutex);
				hdl->player[i].mutex = NULL;
			}
		}
		if (hdl->proxy_queue) {
			hal_queue_delete(hdl->proxy_queue);
			hdl->proxy_queue = NULL;
		}
		hal_free(hdl);
	}
}

void *player_mgr_create(void)
{
	struct player_mgr_t *hdl = hal_malloc(sizeof(*hdl));

	PLAYER_API_ENTRY_DEBUG(hdl, "");
	if (!hdl) {
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));

	for (int i = 0; i < MAX_PLAYER_NUM; i++) {
		hdl->player[i].hdl = player_init();
		if (!hdl->player[i].hdl) {
			printf("no memory!\n");
			goto err;
		}
		registerCallback(hdl->player[i].hdl, hdl, get_player_cb(i));
		update_player_status(hdl, i, PLAYER_STATUS_RESET);
		INIT_LIST_HEAD(&hdl->player[i].res_list);
		hdl->player[i].mutex = hal_mutex_create();
		if (!hdl->player[i].mutex) {
			printf("no memory!\n");
			goto err;
		}
	}

	hdl->proxy_queue = hal_queue_create("player_mgr_q", sizeof(struct player_mgr_proxy_cmd_t), 5);
	if (!hdl->proxy_queue) {
		goto err;
	}

	hdl->proxy_thread = hal_thread_create(player_mgr_proxy_thread, hdl, "player_mgr_t", 1024, 5);
	if (!hdl->proxy_thread) {
		goto err;
	}
	hal_thread_start(hdl->proxy_thread);

	PLAYER_API_EXIT_DEBUG(hdl, "");
	return hdl;
err:
	player_mgr_destroy(hdl);
	PLAYER_API_EXIT_DEBUG(hdl, "");
	return NULL;
}

#ifdef CONFIG_PLAYER_MGR_TEST_CMD
int cmd_player_mgr_test(int argc, char ** argv)
{
	void *sem = player_mgr_create_sem();
	void *hdl = player_mgr_create();

	if (!hdl) {
		PLAYER_ERROR(hdl, "player_mgr_create failed!");
		goto err;
	}

	//play music
	if (argc >=2 && 0 > player_mgr_play_res(hdl, argv[1], RES_PLAY_NOW, NULL)) {
		PLAYER_ERROR(hdl, "player_mgr_play_res(%s) failed!", argv[1]);
		goto err;
	}

	//wait wakeup
	hal_msleep(6*1000);

	//wakeup cmd
	//disable auto play music
	player_mgr_disable_auto_resume_res_player(hdl);
	//play hello tts
	if (argc >=3 && 0 > player_mgr_play_tts(hdl, argv[2], RES_PLAY_NOW | RES_WAIT_COMPLETE, sem)) {
		PLAYER_ERROR(hdl, "player_mgr_play_tts(%s) failed!", argv[2]);
		goto err;
	}
	//wait tts complete
	player_mgr_wait_sem(sem, -1);
	PLAYER_INFO(hdl, "tts finish!");

	//online asr
	hal_msleep(6*1000);

	//get result
	if (argc >=5 && 0 > player_mgr_play_res(hdl, argv[4], RES_PLAY_NOW, NULL)) {
		PLAYER_ERROR(hdl, "player_mgr_play_res(%s) failed!", argv[4]);
		goto err;
	}
	player_mgr_enable_auto_resume_res_player(hdl);

	//play respone tts
	if (argc >=4 && 0 > player_mgr_play_tts(hdl, argv[3], RES_PLAY_NOW | RES_WAIT_COMPLETE, sem)) {
		PLAYER_ERROR(hdl, "player_mgr_play_tts(%s) failed!", argv[3]);
		goto err;
	}

	//wait tts complete
	player_mgr_wait_sem(sem, -1);
	PLAYER_ERROR(hdl, "tts finish!");

	hal_msleep(6*1000);

	player_mgr_destroy(hdl);
	PLAYER_INFO(hdl, "");
	return 0;
err:
	player_mgr_destroy(hdl);
	PLAYER_INFO(hdl, "");
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_player_mgr_test, player_mgr_test, player mgr test);
#endif
