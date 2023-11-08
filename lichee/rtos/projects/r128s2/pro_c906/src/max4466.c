#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <hal_cmd.h>
#include <hal_timer.h>
#include <hal_gpio.h>

#include <sunxi_hal_gpadc.h>

#include <microFFT.h>

int sampleWindow = 50; 


const uint16_t samples = 64; //采样点数，必须为2的整数次幂
const float samplingFrequency = 4000; //Hz, 声音采样频率
 
unsigned int sampling_period_us;
unsigned long microseconds;
 
float vReal[64]; //FFT采样输入样本数组
float vImag[64]; //FFT运算输出数组

void max4466_task(void *arg)
{
    // 读取电压
    while(1){
        uint64_t microseconds = hal_gettime_ns() / 1000 / 1000;

        for (size_t i = 0; i < samples; i++)
        {
            vReal[i] = (double)gpadc_read_channel_data(0);
            vImag[i] = 0;
            while ( (hal_gettime_ns() / 1000 / 1000) - microseconds < sampling_period_us)
            {
                hal_usleep(1);
            }
            microseconds += sampling_period_us;
            
        }

        FFT_Init(vReal, vImag, samples, samplingFrequency);
        FFT_Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT_Compute(FFT_FORWARD);
        FFT_ComplexToMagnitude();

        for(int i = 0; i < 8; i++){  //循环遍历八列LED
            int16_t vvalue=(vReal[i*3+2]+vReal[i*3+3]+vReal[i*3+4])/3/100;

            vvalue=vvalue-2;
            if(vvalue>16) vvalue=16;
            printf("channel%d=%d\r\n", i, vvalue);
        }
    }

    // 释放通道，这里没有用到
    hal_gpadc_channel_exit(0);
    // 释放GPADC，这里没有用到
    hal_gpadc_deinit();

    vTaskDelete(NULL);
}

int max4466_init()
{

    // 初始化 GPADC
    if(hal_gpadc_init() != 0){
        printf("ADC Init failed!\n");
    }

    // 初始化通道
    hal_gpadc_channel_init(0);

    xTaskCreate(max4466_task, "max4466_task", 2048, NULL, 0, NULL);

    return 0;
}
