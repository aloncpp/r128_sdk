# opus 示例工程

> opus示例工程展示了FreeRTOS SDK中进行opus编码与解码的代码实现方法。
>
> 本工程中提供以下opus的示例：
> 1. 对/data/16000.pcm文件进行编码，得到/data/16000_opus_encode编码文件
> 2. 对编码文件/data/16000_opus_encode进行解码，得到解码文件/data/16000_opus_decode.pcm

---

## 适用平台

> 本工程适用以下芯片类型：
>
> 1. R128系列芯片：R128

> 本工程适用以下评估板类型：
> 底板：r128_demo


> 本工程的实现为opus编码/解码实现。

---

## 工程说明

> 本工程的实现为opus编码/解码实现。

### 操作说明

> 1. adb push “16000.pcm”音频源文件 到 /data目录
> 2. 编译工程，烧录镜像，串口执行test_opus 即可


### 代码结构
```
#本工程 
.
├── include	                     #opus库相关头文件
│   ├── analysis.h
│   ├── mlp.h
│   ├── opus_custom.h
│   ├── opus_defines.h
│   ├── opus.h
│   ├── opus_multistream.h
│   ├── opus_private.h
│   ├── opus_types.h
│   └── tansig_table.h
├── objects.mk
├── opus_decode.c				 # opus解码示例
├── opus_encode.c				 # opus编码示例
├── readme.md                    # 本工程的说明文档
└── test
    └── opus_test.c				 # 本工程的入口，进行opus和解码

 
```
### 代码流程

> 1. main()入口：初始化sd卡，然后调用opus_encode_demo()进行编码，调用opus_decode_demo()进行解码。
> 2. 编码函数：opus_encode_demo()
> 3. 编码流程：
>   1）创建buffer
>   2）打开源pcm文件
>   3）创建待保存的编码压缩文件
>   4）初始化opus编码库
>   5）循环执行下面的流程，直至完成整个文件的压缩编码：
>           A）获取一帧pcm数据
>           B）调用opus_encode_data()对这一帧数据进行编码
>           C）往编码文件里，先写入该编码得到的压缩数据的长度。该长度占用4bytes
>           D）往编码文件里，写入压缩数据
>   6）释放资源
> 4. 解码函数：opus_decode_demo()
> 5. 解码流程：
>     1）创建buffer
>     2）打开编码压缩文件
>     3）创建待保存的解码文件
>     4）初始化opus解码库
>     5）循环执行下面的流程，直至完成整个压缩编码文件的解码：
>          A）读取压缩文件的4bytes，该4bytes的值为压缩数据的长度
>          B）读取压缩数据
>          C）调用opus_decode_data()对压缩数据进行解码
>          D）往解码文件里写入解码数据
>     5）释放资源

---

## 性能资源 （XR系列数据）
> 运行opus算法进行编码时，系统的各项资源使用情况如下（cpu频率为384Mhz）：

| 算法库            | 静态sram | 静态psram | 动态sram | 动态psram | cpu占用率 |
| ----------------- | -------- | --------- | -------- | --------- | --------- |
| libopus.a（sram） | 145k     | 0k        | 24k      | 0k        | 17%       |
| libopus.a（xip）  | 0k       | 0k        | 24k      | 0k        | 34%       |

## 常见问题

> 问：如何兼顾sram使用和编码/解码速度？

答：1. 针对不同场景，可以对opus进行不同的配置。如在创建opus编码器时，选择application为OPUS_APPLICATION_RESTRICTED_LOWDELAY，可以有效降低cpu占用率

​       2. 编码的函数主要集中在opus_encoder.c和celt_encoder.c，因此可以opus_encoder.o和celt_encoder.o放到sram，而将其它源码文件放到xip或psram


## 参考文档

> 文档资源

1. 请参考opus官方使用指导文档

> WiKi资源

​    N/A