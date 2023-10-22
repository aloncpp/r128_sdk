# play_fifo 示例工程

> play_fifo 示例工程展示了通过fifo队列缓存音频数据，继而调用Cedarx中间件进行循环音频播放的代码实现方法。
>
> 本工程中提供以下播放方式的示例：
>
> 1. 播放本地音频 data/boot.mp3
> 2. 基于fifo队列的缓存方式，循环播放时**无需重复读取文件**

------

## 适用平台

> R128 s2 PRO



## 模块依赖

> 必选项
>
> 1. libcedarx.a：音频播放核心模块
> 2. libreverb.a：音频混响核心模块

> 可选项
>
> 1. libmp3.a：播放mp3歌曲需要的解码库
> 2. libamr.a：播放amr歌曲需要的解码库
> 3. libaac.a：播放aac/m4a歌曲需要的解码库
> 4. libwav.a：播放wav歌曲需要的解码库
> 5. liblwip.a：播放网络歌曲需要依赖的库
> 6. libmbedtls.a：播放https歌曲需要依赖的库

------

## 工程说明

> 本工程的实现为循环进行fifo来源的音频播放。

### 操作说明

> 1. 确保R128PRO方案的UDISK文件夹中有boot.mp3
> 2. menuconfig中选中play_fifo，编译工程，烧录镜像，启动
> 3. 系统启动后，输入循环播放指令，进行音频的循环播放

### 控制命令

> 1. 循环播放音频指令：play_fifo
> 2. 退出循环播放指令：exit_fifo

### 代码结构

```
#play_fifo工程
.
├── play_fifo.c                 # 示例代码实现，工程的入口，完成平台初始化和播放器线程初始化
├── audiofifo.c                 # 本工程对音频数据流播放的调用接口封装
├── audiofifo.h
├── kfifoqueue.c                # 本工程音频数据流播放的队列实现
├── kfifoqueue.h
├── kfifo.c                     # 本工程使用kfifo实现队列
├── kfifo.h
├── player_app.h     			#对cedarx接口的调用接口封装
├── player_app.c    			#对cedarx接口的调用接口封装实现
└── Readme.md                   # 本工程的说明文档
```

### 代码流程

> 1. cmd_play_fifo()入口：调用创建播放线程。
> 2. 播放线程函数入口：play_task()
> 3. play_task()函数流程：
>    A）完成音频播放准备工作
>    B）完成播放器创建，即player_create()
>    C）调用play_file_music()
> 4. play_fifo()函数流程：
>
> A）创建audio_fifo结构体
>
> B）打开文件，将文件读写进BUFFER中
>
> C）读写的同时，播放音频
>
> D）读写完毕，如需循环播放，则继续播放音频
>
> E）如不需要循环播放， 则退出播放流程，销毁播放器及相关部件

## 注意事项

> 1. 本工程示例支持**循环播放**功能的音频大小有限，为128KB
> 2. 本工程示例仅用于播放开机音乐：boot.mp3


