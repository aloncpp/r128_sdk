# speex 示例工程

> speex 示例工程展示了FreeRTOS SDK中进行speex编码与解码的代码实现方法。
>
> 本工程中提供以下speex的示例：
> 1. 对/data/16000.pcm文件进行编码，得到/data/16000_encode编码文件
> 2. 对编码文件/data/16000_encode进行解码，得到解码文件/data/16000_decode.pcm

---

## 适用平台

> 本工程适用以下芯片类型：
>
> 1. R128系列芯片：R128

> 本工程适用以下评估板类型：
> 底板：r128_demo


## 工程说明

> 本工程的实现为speex编码/解码实现。

### 操作说明

> 1. adb push “16000.pcm”音频源文件 到 /data目录

> 2. 编译工程，烧录镜像，串口执行test_speex 即可


### 代码结构
```
#本工程
.
├── include                    #speex库相关头文件
│   ├── speex_bits.h
│   ├── speex_callbacks.h
│   ├── speex_config_types.h
│   ├── speex.h
│   ├── speex_header.h
│   ├── speex_stereo.h
│   └── speex_types.h
├── Kconfig
├── libs
│   └── libaw_speex.a          # speex静态库
├── objects.mk
├── readme.md                  # 本工程的说明文档
├── speex_decode.c             # speex解码的实现
├── speex_encode.c             # speex编码的实现
└── test
    └── test_speex.c           # 本工程的入口，进行speex编码和解码

```
### 代码流程

> 1. main()入口：调用encode_demo()进行编码，调用decode_demo()进行解码。
> 2. 编码函数：speex_encode_demo()
> 3. 编码流程：
>   1）打开源pcm文件，创建待保存的编码压缩文件
>   2）初始化speex编码库
>   3）创建buffer
>   4）循环执行下面的流程，直至完成整个文件的压缩编码：
>           A）获取一帧pcm数据
>           B）调用speex_encode_data()对这一帧数据进行编码
>           C）往编码文件里，先写入该编码得到的压缩数据的长度。该长度占用4bytes
>           D）往编码文件里，写入压缩数据
>   5）释放资源
> 4. 解码函数：speex_decode_demo()
> 5. 解码流程：
>   1）打开编码压缩文件，闯将待保存的解码文件
>   2）初始化speex解码库
>   3）创建buffer
>   4）循环执行下面的流程，直至完成整个压缩编码文件的解码：
>           A）读取压缩文件的4bytes，该4bytes的值为压缩数据的长度
>           B）读取压缩数据
>           C）调用speex_decode_data()对压缩数据进行解码
>           D）往解码文件里写入解码数据
>   5）释放资源

---

## 性能资源 （XR系列数据）

> 运行speex算法进行编码时，系统的各项资源使用情况如下（cpu频率为384Mhz）：

| 算法库             | 静态sram | 静态psram | 动态sram | 动态psram | cpu占用率 |
| ------------------ | -------- | --------- | -------- | --------- | --------- |
| libspeex.a（sram） | 40k      | 0k        | 4k       | 0k        | 19%       |
| libspeex.a（xip）  | 0k       | 0k        | 4k       | 0k        | 23%       |

## 常见问题

> 问：如何兼顾sram使用和编码/解码速度？

答：1.对于窄带编码/解码，编码/解码的函数主要集中在nb_celp.c文件中，因此可将nb_celp.o放到sram，而将其它源码文件放到xip或psram；

​        2.对于宽带编码/解码，编码/解码的函数主要集中在sb_celp.c文件中，因此可将sb_celp.o放到sram，而将其它源码文件放到xip或psram


## 参考文档

> 文档资源

1. 请参考speex官方使用指导文档

> WiKi资源

​    N/A
