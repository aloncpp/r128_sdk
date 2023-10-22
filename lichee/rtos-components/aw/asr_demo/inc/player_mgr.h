#ifndef __PLAYER_MGR_H__
#define __PLAYER_MGR_H__

#define AUTO_PAUSE_RES_PLAYER
#define TTS_PLAYER              (0)
#define RES_PLAYER              (1)
#define MAX_PLAYER_NUM			(2)

#define PLAYER_STATUS_ERROR     (-1)
#define PLAYER_STATUS_NULL      (0)
#define PLAYER_STATUS_RESET     (1)
#define PLAYER_STATUS_REQUEST   (2)
#define PLAYER_STATUS_SET_URL	(3)
#define PLAYER_STATUS_PREPARE   (4)
#define PLAYER_STATUS_PLAYING   (5)
#define PLAYER_STATUS_PAUSE     (6)
#define PLAYER_STATUS_COMPLETE  (7)
#define PLAYER_STATUS_STOP      (8)

//内部定义，不要使用
#define _RES_ADD_TO_FRONT	    (0<<0) //加入播放列表头部
#define _RES_ADD_TO_BACK		(1<<0) //加入播放列表尾部
#define _RES_CLEAN			    (1<<1) //清空播放列表
#define _RES_PLAY_NOW		    (1<<2) //立即播放目标资源
#define _RES_FORCE_PLAY		    (1<<3) //强制播放目标资源

//可选
#define RES_IMPORTANT			(1<<4) //目标资源重要，不允许打断
#define RES_WAIT_COMPLETE		(1<<5) //将会等待目标资源播放完成

//选1
#define RES_PLAY_NOW			(_RES_ADD_TO_FRONT | _RES_PLAY_NOW) //如果当前播放的资源是可打断的，则打断当前资源，立即播放目标资源，但目标资源是不能打断的，则会加入播放列表头部
#define RES_ADD_TO_PLAYLIST	    (_RES_ADD_TO_BACK) //不会打断当前资源，而是加入播放列表尾部
#define RES_FORCE_PLAY		    (_RES_CLEAN | _RES_ADD_TO_FRONT | _RES_FORCE_PLAY) //无论当前播放资源是否允许打断，都会清空播放列表，强制播放目标资源


void *player_mgr_create(void);
void player_mgr_destroy(void *_hdl);
int player_mgr_reset_tts(void *_hdl);
int player_mgr_reset_res(void *_hdl);
void *player_mgr_create_sem(void);
void player_mgr_destroy_sem(void *sem);
int player_mgr_wait_sem(void *sem, int wait_ms);
int player_mgr_play_tts(void *_hdl, const char *url, int flags, void *sem);
int player_mgr_play_res(void *_hdl, const char *url, int flags, void *sem);
int player_mgr_pause_tts(void *_hdl);
int player_mgr_pause_res(void *_hdl);
int player_mgr_get_tts_status(void *_hdl);
int player_mgr_get_res_status(void *_hdl);

#ifdef AUTO_PAUSE_RES_PLAYER
int player_mgr_disable_auto_resume_res_player(void *_hdl);
int player_mgr_enable_auto_resume_res_player(void *_hdl);
#endif

#endif /* __PLAYER_MGR__ */