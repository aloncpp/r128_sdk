#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <string.h>
#include <console.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <stdlib.h>
#include <hal_time.h>
#include "FreeRTOS_POSIX/utils.h"
#include "rtplayer.h"
#include "xplayer.h"
#include "AudioSystem.h"

#if CONFIG_COMPONENTS_PM
#include "pm_base.h"
#include "pm_notify.h"
#include "pm_wakelock.h"
#endif

//#define CONFIG_RTPLAYER_DIRPLAY 1

#if CONFIG_RTPLAYER_DIRPLAY
#include <dirent.h>
#define FILE_TYPE_NUM 10
#define FILE_TYPE_LEN 10
#define MAX_FILE_NAME_LEN 256
//#define CONFIG_TOTAL_AUDIO_NUM 100
#endif

#define PAUSE_CMD           'P'
#define PING_CMD            'p'
#define STOP_CMD            'S'
#define SEEK_TO_CMD         's'
#define SEEK_TO_CMD2        'j'
#define BACKGROUND_CMD      'b'
#define SHOW_BUFFER_CMD     'B'
#define QUIT_CMD            'q'
#define LOOP_CMD            'l'
#define GET_DURATION_CMD    'G'
#define GET_POSITION_CMD    'g'
#define SET_VOLUME_CMD      'v'
#define CHANGE_CARD_CMD     'c'
#define HELP_CMD            'h'
#define INFO_CMD            'i'
#define REPLAY_CMD          'r'
#define RETRY_CMD           256

#define USE_PREPARE_ASYNC 0

#define LOGD(msg, arg...)      //printf("[PLAYER_DBG] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define LOGI(msg, arg...)      //printf("[PLAYER_INFO] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define LOGW(msg, arg...)      printf("[PLAYER_WRN] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)
#define LOGE(msg, arg...)      printf("[PLAYER_ERR] <%s : %d> " msg "\n", __func__, __LINE__, ##arg)

#define TPLAYER_DEFAULT_PRIORITY ((configMAX_PRIORITIES >> 1))

typedef struct DemoPlayerContext
{
    RTPlayer* mRTplayer;
    sem_t mPreparedSem;
    mqd_t mRTplayerMq;
    pthread_t mThreadId;
    char *pUrl;
    int mSeekable;
    char isPlayingFlag;
    char mError;
    int inputMode;
    int isSetLoop;
    char quitFlag;//no_shell_input mode quitFlag
    int testMode;
    MediaInfo* mMediaInfo;
    int SoundCard;
#if CONFIG_COMPONENTS_PM
    int pm_id;
    int bPauseWhenSuspend;
#endif
#if CONFIG_RTPLAYER_DIRPLAY
    pthread_t mDirThreadId;
    int dirMode;
    int mRealFileNum;
    int mCurPlayIndex;
    int mNextFlag;
    int bDirQuit;
    char mAudioList[CONFIG_TOTAL_AUDIO_NUM][MAX_FILE_NAME_LEN];
#endif
    int retryFlag;
    int retryCnt;
}DemoPlayerContext;

typedef struct DemoPlayerMsg
{
    int msg;
    int data;
}DemoPlayerMsg;

#define INVALID_MQD     ( ( mqd_t ) -1 )
#define DEFAULT_MODE    0600
static const char *pcRTplayerMqName = "/rtplayerMq";
static volatile mqd_t mRTplayerMq = INVALID_MQD;
static int mRTplayerUserInput = 0;
static struct mq_attr xRTplayerMqAttr =
    {
        .mq_flags   =   0,
        .mq_maxmsg  =   3,
        .mq_msgsize =   sizeof(DemoPlayerMsg),
        .mq_curmsgs =   0
    };

/* callbackFromPm:
 * pause the rtplayer when suspend happen and wake the rtplayer when suspend done*/
#if CONFIG_COMPONENTS_PM
static struct wakelock rtplayer_wakelock = {
    .name = "rtplayer_wakelock",
    .ref = 0,
};

static int cmd_rtplayer_controller(int argc, char ** argv);

int callbackFromPm(suspend_mode_t mode, pm_event_t event, void *arg)
{
    DemoPlayerContext* demoPlayer = (DemoPlayerContext*)arg;
    switch (event) {
    case PM_EVENT_SYS_PERPARED:
    {
        if (demoPlayer->isPlayingFlag) {
            printf("pause the rtplayer\n");
            pause_l(demoPlayer->mRTplayer);
            demoPlayer->isPlayingFlag = 0;
            demoPlayer->bPauseWhenSuspend = 1;
            sleep(4);
            printf("********rtplayer_test:pause by PM callback when suspend begin\n");

        }
                break;
    }
    case PM_EVENT_FINISHED:
    {
        if (demoPlayer->bPauseWhenSuspend && (!demoPlayer->isPlayingFlag)) {
            printf("play the rtplayer\n");
            start(demoPlayer->mRTplayer);
            demoPlayer->isPlayingFlag = 1;
            demoPlayer->bPauseWhenSuspend = 0;
        }
        printf("********rtplayer_test:continue by PM callback when suspend done\n");
        break;
    }
    default:
        break;
    }
    return 0;
}

static pm_notify_t pm_rtplayer = {
    .name = "pm_rtplayer",
    .pm_notify_cb = callbackFromPm,
    .arg = NULL,
};
#endif

static void showHelp(){
    printf("\n");
    printf("**************************\n");
    printf("* This is a simple audio player, when it is started, you can input commands to tell\n");
    printf("* what you want it to do.\n");
    printf("* Usage: \n");
    printf("*   tplayer_demo /data/test.mp3  : this means play test.mp3\n");
    printf("*   P  :this will Pause if in playing status,or Play in paused status \n");
    printf("*   S  :this means Stop \n");
    printf("*   s  :this means seek to 10s \n");
    printf("*   B  :show buffer \n");
    printf("*   b  :this means player will run in the background \n");
    printf("*   q  :this means quit the player \n");
    printf("*   l  :this means loop play \n");
    printf("*   G :this means Get  duration \n");
    printf("*   g :this means get  position \n");
    printf("*   v :this means set volume \n");
    printf("*   c :this means change sound card \n");
    printf("*   i :this means show media info \n");
    printf("*   h :this means show the help information \n");
    printf("*   r : replay the current audio\n");
    printf("**************************\n");
}
static int rtplayer_clear_cmd(mqd_t mq){
    struct timespec cur, delay, abstime;
    clock_gettime( CLOCK_REALTIME, &cur );
    delay.tv_sec = 0;
    delay.tv_nsec = 5*1000*1000;
    UTILS_TimespecAdd(&cur, &delay, &abstime);
    DemoPlayerMsg msg;
    while(mq_timedreceive(mq, (char *)&msg, sizeof(msg), NULL, &abstime)!=-1);
    return 0;
}
static int rtplayer_send_cmd(mqd_t mq, int msg, int data){
    DemoPlayerMsg pmsg = {msg, data};
    struct timespec tsn, ts;
    clock_gettime(CLOCK_REALTIME, &tsn);
    UTILS_TimespecAddNanoseconds(&tsn, 20*1000*1000, &ts);
    int status = mq_timedsend(mq, (char *)&pmsg, sizeof(pmsg), 0, &ts);
    if(status)
        LOGE("send cmd %c,%d failed!", pmsg.msg, pmsg.data);
    return status;
}

static int rtplayer_send_cmd_force(mqd_t mq, int msg, int data){
    int try_times = 0;
    DemoPlayerMsg pmsg = {msg, data};
    struct timespec tsn, ts;
    int status;
try_send:
    clock_gettime(CLOCK_REALTIME, &tsn);
    UTILS_TimespecAddNanoseconds(&tsn, 20*1000*1000, &ts);
    status = mq_timedsend(mq, (char *)&pmsg, sizeof(pmsg), 0, &ts);
    if(status){
        try_times++;
        if(try_times<5){
            LOGE("send cmd %c,%d failed, retry...", pmsg.msg, pmsg.data);
            goto try_send;
        }
        else if(try_times<10){
            DemoPlayerMsg tmp;
            LOGE("send cmd %c,%d failed, retry...", pmsg.msg, pmsg.data);
            clock_gettime(CLOCK_REALTIME, &tsn);
            UTILS_TimespecAddNanoseconds(&tsn, 20*1000*1000, &ts);
            status = mq_timedreceive(mq, (char *)&tmp, sizeof(tmp), NULL, &ts);
            if(status<0){
                LOGE("mq_receive fail %d", status);
                goto fail_exit;
            }
            LOGW("drop: %c, %d", tmp.msg, tmp.data);
            goto try_send;
        }
        goto fail_exit;
    }
    return status;
fail_exit:
    LOGE("send cmd %c,%d failed!\n", pmsg.msg, pmsg.data);
    return status;
}

static void callbackFromRTplayer(void* userData,int msg, int id, int ext1, int ext2);
static void* RTplayerThread(void* arg){
    DemoPlayerContext* demoPlayer = (DemoPlayerContext*)arg;
    char quitFlag = 0;

    if(demoPlayer->inputMode)
    {
	while(1)
	{
	     if(demoPlayer->quitFlag)
	     {
		if(demoPlayer->mRTplayer != NULL)
		{

#if CONFIG_COMPONENTS_PM
            pm_notify_unregister(demoPlayer->pm_id);
#endif
			printf("player finsh, quit the rtplayer\n");
			mRTplayerMq = INVALID_MQD;
#if USE_PREPARE_ASYNC
			sem_destroy(&demoPlayer->mPreparedSem);
#endif
			player_deinit(demoPlayer->mRTplayer);
			free(demoPlayer->pUrl);
			free(demoPlayer);
		}
		break;
	     }
		usleep(50*1000);
	}
	return NULL;

    }
    while(!quitFlag){
        int cRxed = 0;
        int data = 0;
        DemoPlayerMsg msg;
        ssize_t status;
        if(demoPlayer->mRTplayerMq!=INVALID_MQD){
            usleep(50*1000);
            ssize_t status = mq_receive(demoPlayer->mRTplayerMq, (char *)&msg, sizeof(msg), NULL);
            if(status<=-1){
                LOGE("mq_receive fail %d", status);
                usleep(1*1000*1000);
                continue;
            }
            printf("receive %c,%d\n", msg.msg, msg.data);
            cRxed = msg.msg;
            data = msg.data;
        }
        else{
            cRxed = QUIT_CMD;
        }
        switch(cRxed){
            case PAUSE_CMD:
            {
                if(demoPlayer->isPlayingFlag){
                    //pm_wakelocks_acquire(&rtplayer_wakelock, PM_WL_TYPE_WAIT_INC, OS_WAIT_FOREVER);
                    printf("pause the rtplayer\n");
                    pause_l(demoPlayer->mRTplayer);
                    demoPlayer->isPlayingFlag = 0;
#if CONFIG_COMPONENTS_PM
//                    demoPlayer->bPauseWhenSuspend = 0;
#endif
                    //pm_wakelocks_release(&rtplayer_wakelock);
                }else{
                    printf("play the rtplayer\n");
                    start(demoPlayer->mRTplayer);
                    demoPlayer->isPlayingFlag = 1;
#if CONFIG_COMPONENTS_PM
//                    demoPlayer->bPauseWhenSuspend = 1;
#endif
                }
                break;
            }
            case STOP_CMD:
            {
                printf("stop the rtplayer\n");
                stop(demoPlayer->mRTplayer);
                demoPlayer->isPlayingFlag = 0;
                break;
            }
            case SEEK_TO_CMD:
            {
                printf("rtplayer seek to 10 second\n");
                seekTo(demoPlayer->mRTplayer,10);
                break;
            }
            case SEEK_TO_CMD2:
            {
                printf("rtplayer seek to %d second\n", data);
                seekTo(demoPlayer->mRTplayer,data);
                break;
            }
            case QUIT_CMD:
            {
                printf("[%s]%d:quit the rtplayer\n", __func__, __LINE__);
                demoPlayer->quitFlag = 1;
#if CONFIG_COMPONENTS_PM
                pm_notify_unregister(demoPlayer->pm_id);
#endif
                mRTplayerMq = INVALID_MQD;
                //mq_close(demoPlayer->mRTplayerMq);
#if USE_PREPARE_ASYNC
                sem_destroy(&demoPlayer->mPreparedSem);
#endif
#if CONFIG_RTPLAYER_DIRPLAY
                if (demoPlayer->dirMode) {
                    demoPlayer->bDirQuit = 1;
                    quitFlag = 1;
                    break;
                }
#endif

                player_deinit(demoPlayer->mRTplayer);
                free(demoPlayer->pUrl);
                free(demoPlayer);
                quitFlag = 1;
                break;
            }
            case LOOP_CMD:
            {
                printf("let the rtplayer loop play\n");
                demoPlayer->isSetLoop = 1;
                setLooping(demoPlayer->mRTplayer,1);
                break;
            }
            case GET_DURATION_CMD:
            {
                printf("get the audio duration\n");
                int duration;
                getDuration(demoPlayer->mRTplayer,&duration);
                printf("duration:%d s\n",duration);
                break;
            }
            case GET_POSITION_CMD:
            {
                printf("get the current position\n");
                int position;
                getCurrentPosition(demoPlayer->mRTplayer,&position);
                printf("current position:%d s\n",position);
                break;
            }
            case SET_VOLUME_CMD:
            {
                uint32_t volume_value = 0;
                printf("set the volume %d\n",data);
                int ret = softvol_control_with_streamtype(AUDIO_STREAM_MUSIC, &volume_value, 2);
                if (ret != 0) {
                    printf("get softvol range failed:%d\n", ret);
                }
                int volume_max = (uint16_t)((volume_value >> 16) & 0xffff);
                int volume_min = (uint16_t)(volume_value & 0xffff);
                if ((data < volume_min) || (data > volume_max)) {
                    printf("warning:out range : %d ~ %d\n",volume_min,volume_max);
                    break;
                }
                volume_value = ((data << 16) | data);
                ret = softvol_control_with_streamtype(AUDIO_STREAM_MUSIC, &volume_value, 1);
                if (ret != 0) {
                    printf("error:set softvol failed!\n");
                }
                break;
            }
            case CHANGE_CARD_CMD:
            {
                switch(data) {
                    case CARD_DEFAULT:
                        printf("change to default\n");
                        break;
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_MULTI_PCM
                    case CARD_MULTI:
                        printf("change to multi\n");
                        break;
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_AMP
                    case CARD_AMP_PB:
                        printf("change to amp\n");
                        break;
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_BT
                    case CARD_BT_SRC:
                        printf("change to bt\n");
                        break;
#endif
                    default:
                        data = -1;
                        printf("error:not support!\n");
                        break;
                }
                if (data >= 0) {
                    changeSoundCard(demoPlayer->mRTplayer,data);
                }
                break;
            }
            case HELP_CMD:
            {
                printf("show the help information\n");
                showHelp();
                break;
            }
	    case INFO_CMD:
	    {
		printf("**************************\n");
		printf("* show media information:\n");
		MediaInfo* mi = NULL;
		demoPlayer->mMediaInfo = getMediaInfo(demoPlayer->mRTplayer);
		if(demoPlayer->mMediaInfo != NULL){
                        mi = demoPlayer->mMediaInfo;
                        printf("* file size = %lld KB\n",mi->nFileSize/1024);
                        printf("* duration = %lld ms\n",mi->nDurationMs);
                        printf("* bitrate = %d Kbps\n",mi->nBitrate/1024);
                        printf("* container type = %d\n",mi->eContainerType);
                        printf("* audio stream num = %d\n",mi->nAudioStreamNum);
                        if(mi->pAudioStreamInfo != NULL){
                            printf("* audio codec tpye = %d\n",mi->pAudioStreamInfo->eCodecFormat);
                            printf("* audio channel num = %d\n",mi->pAudioStreamInfo->nChannelNum);
                            printf("* audio BitsPerSample = %d\n",mi->pAudioStreamInfo->nBitsPerSample);
                            printf("* audio sample rate  = %d\n",mi->pAudioStreamInfo->nSampleRate);
                        }
		printf("**************************\n");
                    }
                    break;

	    }
	    case SHOW_BUFFER_CMD:
	    {
		printf("**************************\n");
		printf("* show buffer information:\n");
		player_show_buffer();
		printf("**************************\n");
		break;

	    }
            case REPLAY_CMD:
            {
                printf("replay %s\n", demoPlayer->pUrl);
                int ret;
                if(demoPlayer->testMode){
                    printf("test mode: destroy & create instead of reset\n");
                    player_deinit(demoPlayer->mRTplayer);
                    usleep(50*1000);
                    demoPlayer->mRTplayer = (RTPlayer*)(uintptr_t)player_init();
                    printf("demoPlayer.mRTplayer = %p\n",demoPlayer->mRTplayer);
                    if(!demoPlayer->mRTplayer){
                        printf("init rtplayer fail\n");
                        free(demoPlayer->pUrl);
                        free(demoPlayer);
                        quitFlag = 1;
                        continue;
                    }
                    registerCallback(demoPlayer->mRTplayer, demoPlayer, callbackFromRTplayer);
                }
                else
                    reset(demoPlayer->mRTplayer);
                ret = setDataSource_url(demoPlayer->mRTplayer, demoPlayer, demoPlayer->pUrl, 0);
                if(ret){
                    printf("setDataSource_url failed\n");
                    break;
                }
                ret = prepare(demoPlayer->mRTplayer);
                if(ret){
                    printf("prepare failed\n");
                    break;
                }
                start(demoPlayer->mRTplayer);
                demoPlayer->isPlayingFlag = 1;

		if(demoPlayer->isSetLoop)
		{
                    setLooping(demoPlayer->mRTplayer,1);
		}
                break;
            }
            case RETRY_CMD:
            {
                int position = data;
                if(data==-1)
                    getCurrentPosition(demoPlayer->mRTplayer,&position);

                if (demoPlayer->retryCnt > 59) {
                    LOGE("error: retry failed too many times");
#if CONFIG_RTPLAYER_DIRPLAY
                    if (demoPlayer->dirMode) {
                        demoPlayer->retryCnt = 0;
                        demoPlayer->retryFlag = 0;
                        demoPlayer->mNextFlag = 1;
                        break;
                    }
#endif
                    rtplayer_send_cmd_force(demoPlayer->mRTplayerMq, QUIT_CMD, position);
                    break;
                }
                printf("retry %s round:%d\n", demoPlayer->pUrl, demoPlayer->retryCnt+1);
                demoPlayer->retryFlag = 1;
                int ret;
                if(demoPlayer->testMode){
                    printf("test mode: destroy & create instead of reset\n");
                    player_deinit(demoPlayer->mRTplayer);
                    usleep(50*1000);
                    demoPlayer->mRTplayer = (RTPlayer*)(uintptr_t)player_init();
                    printf("demoPlayer.mRTplayer = %p\n",demoPlayer->mRTplayer);
                    if(!demoPlayer->mRTplayer){
                        LOGE("init rtplayer fail");
                        free(demoPlayer->pUrl);
                        free(demoPlayer);
                        quitFlag = 1;
                        continue;
                    }
                    registerCallback(demoPlayer->mRTplayer, demoPlayer, callbackFromRTplayer);
                }
                else
                    reset(demoPlayer->mRTplayer);
                ret = setDataSource_url(demoPlayer->mRTplayer, demoPlayer, demoPlayer->pUrl, 0);
                if(ret){
                    LOGE("setDataSource_url failed");
                    demoPlayer->retryCnt++;
                    rtplayer_send_cmd_force(demoPlayer->mRTplayerMq, RETRY_CMD, position);
                    usleep(500*1000);
                    break;
                }
                ret = prepare(demoPlayer->mRTplayer);
                if(ret){
                    LOGE("prepare failed");
                    demoPlayer->retryCnt++;
                    rtplayer_send_cmd_force(demoPlayer->mRTplayerMq, RETRY_CMD, position);
                    usleep(500*1000);
                    break;
                }
                start(demoPlayer->mRTplayer);
                demoPlayer->isPlayingFlag = 1;
                demoPlayer->retryCnt = 0;
                demoPlayer->retryFlag = 0;
#if CONFIG_COMPONENTS_PM
                demoPlayer->bPauseWhenSuspend = 0;
#endif
                //seekTo(demoPlayer->mRTplayer, position);
                if(demoPlayer->isSetLoop)
                    setLooping(demoPlayer->mRTplayer,1);
                break;
            }
            default:
            {
                LOGW("warning: unknown command,cmd = %d",cRxed);
                break;
            }
        }
        if(quitFlag){
            return NULL;
        }
    }
    return NULL;
}
static void callbackFromRTplayer(void* userData,int msg, int id, int ext1, int ext2){

    LOGI("call back from RTplayer,msg = %d,id = %d,ext1 = %d,ext2 = %d\n",msg,id,ext1,ext2);

	DemoPlayerContext* pDemoPlayer = (DemoPlayerContext*)userData;
    switch(msg)
    {
        case RTPLAYER_NOTIFY_PREPARED:
        {
            printf("RTPLAYER_NOTIFY_PREPARED:has prepared.\n");
 #if USE_PREPARE_ASYNC           
            sem_post(&pDemoPlayer->mPreparedSem);
            pDemoPlayer->mPreparedFlag = 1;
 #endif
            break;
        }
        case RTPLAYER_NOTIFY_PLAYBACK_COMPLETE:
        {
            printf("RTPLAYER_NOTIFY_PLAYBACK_COMPLETE:play complete\n");
            pDemoPlayer->isPlayingFlag = 0;
	    if(pDemoPlayer->inputMode)
	    {
    		pDemoPlayer->quitFlag = 1;
	    }
#if CONFIG_RTPLAYER_DIRPLAY
        if (pDemoPlayer->dirMode)
            pDemoPlayer->mNextFlag = 1;
#endif
            break;
        }
        case RTPLAYER_NOTIFY_SEEK_COMPLETE:
        {
            printf("RTPLAYER_NOTIFY_SEEK_COMPLETE:seek ok\n");
            break;
        }
        case RTPLAYER_NOTIFY_MEDIA_ERROR:
        {
            switch (ext1)
            {
                case RTPLAYER_MEDIA_ERROR_UNKNOWN:
                {
                    printf("erro type:TPLAYER_MEDIA_ERROR_UNKNOWN\n");
                    break;
                }
                case RTPLAYER_MEDIA_ERROR_UNSUPPORTED:
                {
                    printf("erro type:TPLAYER_MEDIA_ERROR_UNSUPPORTED\n");
                    break;
                }
                case RTPLAYER_MEDIA_ERROR_IO:
                {
                    printf("erro type:TPLAYER_MEDIA_ERROR_IO\n");
                    break;
                }
            }
            printf("RTPLAYER_NOTIFY_MEDIA_ERROR\n");
            pDemoPlayer->mError = 1;
#if USE_PREPARE_ASYNC
            if(pDemoPlayer->mPreparedFlag == 0){
                printf("recive err when preparing\n");
                sem_post(&pDemoPlayer->mPreparedSem);
            }
#endif
            if( (pDemoPlayer->mRTplayerMq!=INVALID_MQD)&&(!pDemoPlayer->retryFlag) ){
                pDemoPlayer->retryCnt++;
                rtplayer_send_cmd_force(pDemoPlayer->mRTplayerMq, RETRY_CMD, -1);
            }
            else if (pDemoPlayer->mRTplayerMq == INVALID_MQD) {
                printf("io error, mqueue not exist\n");
            }
            break;
        }
        case RTPLAYER_NOTIFY_NOT_SEEKABLE:
        {
            pDemoPlayer->mSeekable = 0;
            printf("info: media source is unseekable.\n");
            break;
        }
        case RTPLAYER_NOTIFY_DETAIL_INFO:
        {
            int flag = *(int *)(uintptr_t)ext2;
            //printf("detail info: %d\n", flag);
            break;
        }
        default:
        {
            printf("warning: unknown callback from RTplayer.\n");
            break;
        }
    }
}
#if CONFIG_RTPLAYER_DIRPLAY
static int DirPlayConfig(DemoPlayerContext* demoPlayer, char* pUrl)
{
    char fileType[FILE_TYPE_NUM][FILE_TYPE_LEN] = {".mp3", ".mp1", ".mp2", ".ogg", ".flac", ".wav", ".m4a", ".amr", ".aac", ".opus"};
   /* *********************
    * open the dir and check if it exist or not
    ********************* */
    if (strlen(pUrl) > 256) {
        LOGE("dir path is out of range\n");
        return -1;
    }
    DIR *dir = opendir(pUrl);
    if (dir == NULL) {
        printf("[DirPlayConfig]the dir path is invalid!!!\n");
        return -1;
    }
   /* *********************
    * search the file for playing
    ********************* */
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        printf("[DirPlayConfig]get file:%s, type:%d\n", entry->d_name, entry->d_type);
        if (entry->d_type == DT_REG) {
            char *strpos;
            if ((strpos = strrchr(entry->d_name, '.')) != NULL) {
                int i = 0;

                /* *********************
                 *  judge the suffix
                 ********************* */
                printf("[DirPlayConfig]cut down the suffix of %s is %s\n", entry->d_name, strpos);
                for (i = 0; i < FILE_TYPE_NUM; i++) {
                    if(!strncasecmp(strpos,fileType[i],strlen(fileType[i]))) {
                        printf("[DirPlayConfig]find the match type:%s\n", &(fileType[i]));

                        /* ***************************************
                         *  if match the file type with the suffix
                         *  then add the file name to player's context
                         *************************************** */
                        if (count < CONFIG_TOTAL_AUDIO_NUM) {
                            strncpy(demoPlayer->mAudioList[count], entry->d_name, strlen(entry->d_name));
                            printf("[DirPlayConfig]Audio file name = %s\n", demoPlayer->mAudioList[count]);
                            count++;
                        } else {
                            printf("warning:too many files, only support:%d\n", CONFIG_TOTAL_AUDIO_NUM);
                            goto File_Match_End;
                        }
                        break;
                    }
                }

            }
        }
    }
File_Match_End:
    closedir(dir);
/* ***************************************
 *  add some flag for dir loop playing
 *************************************** */
    demoPlayer->mNextFlag = 1;
/* ******************************************
 *  set the total num of files going to play
 ****************************************** */

    demoPlayer->mRealFileNum = count;
    if (demoPlayer->mRealFileNum == 0) {
        printf("[DirPlayConfig]there's no supported media file in %s, exit(-1)\n", pUrl);
        return -1;
    }
    printf("[DirPlayConfig]there are %d media files ready to play\n", demoPlayer->mRealFileNum);

    return 0;
}

static void* DirPlayThread(void* arg)
{
    int nDirErrCnt = 0;
    char fileUrl[512];
    DemoPlayerContext* demoPlayer = (DemoPlayerContext*)arg;
    printf("[%s]testing-->files num that ready to play:%d\n",__func__, demoPlayer->mRealFileNum);
    demoPlayer->mCurPlayIndex = 0;
    demoPlayer->bDirQuit = 0;
/* ******************************************
 *  start the loop of dir play
 ****************************************** */

    while(!demoPlayer->bDirQuit) {
        if (demoPlayer->mNextFlag) {
DirLoop_begin:
            if (nDirErrCnt >= demoPlayer->mRealFileNum) {
                LOGE("there's no avaliable files in current dir");
                goto QUIT;
            }

            demoPlayer->mNextFlag = 0;
            if (reset(demoPlayer->mRTplayer) != 0) {
                LOGE("rtplayer reset fail!");
            } else {
                printf("reset the player ok.\n");
                if (demoPlayer->mError == 1)
                    demoPlayer->mError = 0;
            }
            demoPlayer->mSeekable = 1;
            /* ******************************
             *   check and adjust the index
             *   then set the url
             ******************************** */
            if (demoPlayer->mCurPlayIndex == demoPlayer->mRealFileNum)
                demoPlayer->mCurPlayIndex = 0;
            strcpy(fileUrl, demoPlayer->pUrl);
            strcat(fileUrl, "/"); /* divide the dir and file */
            strcat(fileUrl, demoPlayer->mAudioList[demoPlayer->mCurPlayIndex]);
            demoPlayer->mCurPlayIndex++;
            if (setDataSource_url(demoPlayer->mRTplayer,demoPlayer,fileUrl, 0) != 0) {
                LOGE("DirPlay set url failed");
            } else
               printf("[%s]set url done:%s\n", __func__, fileUrl);
            /* ******************************************
             *  prepare and start
             *  if prepare failed, count and skip it
             ****************************************** */
            if ((prepare(demoPlayer->mRTplayer) != 0) || (demoPlayer->mError == 1)) {
                LOGE("prepare failed");
                nDirErrCnt++;
                goto DirLoop_begin;
            }
            start(demoPlayer->mRTplayer);
            demoPlayer->isPlayingFlag = 1;
        }
        hal_msleep(500);
    }
QUIT:
#if USE_PREPARE_ASYNC
    sem_destroy(&demoPlayer->mPreparedSem);
#endif
    player_deinit(demoPlayer->mRTplayer);
    free(demoPlayer->pUrl);
	free(demoPlayer);
    return 0;
}
#endif
int cmd_rtplayer_test(int argc, char ** argv)
{
    int inputMode = 0;
    int testMode = 0;
#if CONFIG_RTPLAYER_DIRPLAY
    int dirMode = 0;
#endif
	/*
    printf("argc = %d\n",argc);
    for(int i=0; i < argc;i++){
        printf("argv[%d]=%s\n",i,argv[i]);
    }
	*/
	printf("rtplayer source:%s\n", argv[1]);

    if(argc == 3){
        if( !strncmp("no_shell_input", argv[2], sizeof("no_shell_input")-1) ){
            argc--;
            inputMode = 1;
        }
        else if( !strncmp("test_mode", argv[2], sizeof("test_mode")-1) ){
            argc--;
            testMode = 1;
        }
#if CONFIG_RTPLAYER_DIRPLAY
        else if (!strncmp("dir_mode", argv[2], sizeof("dir_mode")-1)) {
            argc--;
            dirMode = 1;
        }
#endif
    }
    if(argc != 2){
        LOGW("the parameter is error,usage is as following:");
        showHelp();
        goto rtp_failed;
    }
#if USE_PREPARE_ASYNC
    int waitErr = 0;
#endif
    DemoPlayerContext* demoPlayer = (DemoPlayerContext*)malloc(sizeof(DemoPlayerContext));
    if(demoPlayer == NULL){
        LOGE("malloc DemoPlayerContext fail");
        goto rtp_failed;
    }
    memset(demoPlayer, 0, sizeof(DemoPlayerContext));
    demoPlayer->mSeekable = 1;
    demoPlayer->mRTplayerMq = INVALID_MQD;
    demoPlayer->inputMode = inputMode;
    demoPlayer->testMode = testMode;
    demoPlayer->quitFlag = 0;
    demoPlayer->mMediaInfo = NULL;
    demoPlayer->retryFlag = 0;
    demoPlayer->retryCnt = 0;

    if(strlen(argv[1])<=0){
        LOGE("url error");
        goto rtp_url_failed;
    }
    demoPlayer->pUrl = malloc(strlen(argv[1])+1);
    if(!demoPlayer->pUrl){
        LOGE("pUrl malloc fail");
        goto rtp_url_failed;
    }
    memset(demoPlayer->pUrl, 0, strlen(argv[1]));
    strcpy(demoPlayer->pUrl, argv[1]);
#if USE_PREPARE_ASYNC    
     sem_init(&demoPlayer->mPreparedSem, 0, 0);
#endif
    demoPlayer->mRTplayer = (RTPlayer*)(uintptr_t)player_init();
    LOGI("demoPlayer.mRTplayer = %p",demoPlayer->mRTplayer);
    if(!demoPlayer->mRTplayer){
        LOGE("init rtplayer fail");
        goto rtp_init_failed;
    }
#if CONFIG_COMPONENTS_PM
    int pm_ret = 0;
    pm_rtplayer.arg = demoPlayer;
    pm_ret = pm_notify_register(&pm_rtplayer);
    if (pm_ret<0) {
        LOGE("pm_notify register fail");
        goto rtp_prepare_failed;
    }
    printf("***************rtplayer_test:pm notify register success! id:%d\n", pm_ret);
    demoPlayer->pm_id = pm_ret;
    demoPlayer->bPauseWhenSuspend = 0;

#endif

#if CONFIG_RTPLAYER_DIRPLAY
    demoPlayer->dirMode = dirMode;
    if (demoPlayer->dirMode) {
        int dirconfig_ret = DirPlayConfig(demoPlayer, argv[1]);
        if (dirconfig_ret < 0)
            goto rtp_prepare_failed;

        registerCallback(demoPlayer->mRTplayer, demoPlayer, callbackFromRTplayer);

        pthread_attr_t dir_attr;
        pthread_attr_init(&dir_attr);
        struct sched_param dir_sched;
        dir_sched.sched_priority = TPLAYER_DEFAULT_PRIORITY;
        pthread_attr_setschedparam(&dir_attr, &dir_sched);
        pthread_attr_setstacksize(&dir_attr, 32768);
        pthread_attr_setdetachstate(&dir_attr, PTHREAD_CREATE_DETACHED);

        if ( pthread_create(&demoPlayer->mDirThreadId, &dir_attr, DirPlayThread, demoPlayer) ) {
            LOGE("pthread_create failed, quit the rtplayer");
            goto rtp_prepare_failed;
        }
        pthread_setname_np(demoPlayer->mDirThreadId, "DirPlayThread");
        goto Msg_Process;
    }
#endif

    //demoPlayer->SoundCard = 0;
    //printf("SoundCard number:%d\n", demoPlayer->SoundCard);
    //setSoundCard(demoPlayer->mRTplayer, demoPlayer->SoundCard);



    registerCallback(demoPlayer->mRTplayer, demoPlayer, callbackFromRTplayer);
    status_t ret = setDataSource_url(demoPlayer->mRTplayer,demoPlayer,demoPlayer->pUrl, 0);
    if(ret){
        LOGE("set DataSource url fail");
        goto rtp_prepare_failed;
    }
#if USE_PREPARE_ASYNC
    demoPlayer->mPreparedFlag = 0;
    if(prepareAsync(demoPlayer->mRTplayer) != 0)
    {
        printf("TPlayerPrepareAsync() return fail.\n");
    }else{
        printf("preparing...\n");
    }
    struct timespec t;
    t.tv_nsec = 0;
    t.tv_sec = 30;
    waitErr = sem_timedwait(&demoPlayer->mPreparedSem, &t);
    if(waitErr == -1){
        printf("prepare timeout,has wait %d s\n",t.tv_sec);
        sem_destroy(&demoPlayer->mPreparedSem);
        goto rtp_prepare_failed;
    }else if(demoPlayer.mError == 1){
        printf("prepare fail\n");
        sem_destroy(&demoPlayer->mPreparedSem);
        goto rtp_prepare_failed;
    }
    printf("prepared ok\n");
#else
    ret = prepare(demoPlayer->mRTplayer);
    if(ret){
        LOGE("prepare fail");
        goto rtp_prepare_failed;
    }
#endif
    start(demoPlayer->mRTplayer);
    demoPlayer->isPlayingFlag = 1;

Msg_Process:
    if( mRTplayerMq==INVALID_MQD ){
        mRTplayerMq = mq_open( pcRTplayerMqName, O_CREAT | O_RDWR, DEFAULT_MODE, &xRTplayerMqAttr );
        if(mRTplayerMq==INVALID_MQD){
            LOGE("mq_open fail");
        }
    }
    demoPlayer->mRTplayerMq = mRTplayerMq;
    rtplayer_clear_cmd(demoPlayer->mRTplayerMq);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    struct sched_param sched;
    sched.sched_priority = TPLAYER_DEFAULT_PRIORITY;
    pthread_attr_setschedparam(&attr, &sched);
    pthread_attr_setstacksize(&attr, 32768);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if( pthread_create(&demoPlayer->mThreadId, &attr, RTplayerThread, demoPlayer) ){
        LOGE("pthread_create failed, quit the rtplayer");
        mRTplayerMq = INVALID_MQD;
        //mq_close(demoPlayer->mRTplayerMq);
#if USE_PREPARE_ASYNC
        sem_destroy(&demoPlayer->mPreparedSem);
#endif
        goto rtp_prepare_failed;
    }
    pthread_setname_np(demoPlayer->mThreadId, "RTplayerThread");

#if CONFIG_RTPLAYER_DIRPLAY
    if(demoPlayer->dirMode)
        goto rtp_succeed;
#endif
    if(demoPlayer->inputMode)
        goto rtp_succeed;
    while(!demoPlayer->quitFlag){
        int data = 0;
        char cRxed = getchar();
        if (cRxed != '\n') {
        if(cRxed==BACKGROUND_CMD){
            printf("shell input exit, rtplayer will run in the background\n");
            break;
        } else if (cRxed == SET_VOLUME_CMD) {
                printf("input target volume and enter\n");
                scanf("%d", &data);
        } else if (cRxed == CHANGE_CARD_CMD) {
                printf("========================\n");
                printf("%d: default\n", CARD_DEFAULT);
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_MULTI_PCM
                printf("%d: multi\n", CARD_MULTI);
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_AMP
                printf("%d: amp\n", CARD_AMP_PB);
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_BT
                printf("%d: bt\n", CARD_BT_SRC);
#endif
                scanf("%d", &data);
            } else {
        }
        rtplayer_send_cmd(demoPlayer->mRTplayerMq, cRxed, data);
        if(cRxed==QUIT_CMD)
            break;
        }
        usleep(50*1000);
    }
rtp_succeed:
    return 0;
rtp_prepare_failed:
    player_deinit(demoPlayer->mRTplayer);
#if CONFIG_COMPONENTS_PM
	if (demoPlayer->pm_id >= 0)
        pm_notify_unregister(demoPlayer->pm_id);
#endif
rtp_init_failed:
    free(demoPlayer->pUrl);
rtp_url_failed:

	free(demoPlayer);
rtp_failed:
    return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rtplayer_test, rtplayer_test, test the rtplayer);

static int cmd_rtplayer_controller(int argc, char ** argv){
    if(mRTplayerMq==INVALID_MQD){
        printf("mRTplayerMq = INVALID_MQD!\n");
        return -1;
    }
    if( (argc!=2) && (argc!=3) ){
        printf("usage:rtpc <cmd> [data]\n");
        return -1;
    }
    int data = 0;

    if(argc==3)
        data = atoi(argv[2]);
    rtplayer_send_cmd(mRTplayerMq, argv[1][0], data);

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_rtplayer_controller, rtpc, control the rtplayer);
