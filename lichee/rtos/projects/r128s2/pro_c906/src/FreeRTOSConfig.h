/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
	See http://www.freertos.org/a00110.html for an explanation of the
	definitions contained in this file.
******************************************************************************/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 * http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

unsigned long ulGetRunTimeCounterValue( void );
extern uint32_t SystemCoreClock;        /* Global variable of CMSIS */

/* Fix compile error for code port from R328 */
#define configNR_CPUS       		1
#define CLINT_CTRL_ADDR   0x14000000
#define configMTIME_BASE_ADDRESS    ( ( CLINT_CTRL_ADDR  ) + 0xBFF8UL  )
#define configMTIMECMP_BASE_ADDRESS ( ( CLINT_CTRL_ADDR  ) + 0x4000UL  )

/* Cortex M33 port configuration. */
#define configENABLE_MPU                                        0  // closed by xradio
#if defined(CONFIG_TOOLCHAIN_FLOAT_HARD) || defined(CONFIG_TOOLCHAIN_FLOAT_SOFTFP)
#define configENABLE_FPU                                        1  // by xradio
#else
#define configENABLE_FPU                                        0  // by xradio
#endif

#if defined(CONFIG_COMPONENTS_TFM)
#define configENABLE_TRUSTZONE                                  0
#define configRUN_FREERTOS_SECURE_ONLY                          0
#elif defined(CONFIG_TRUSTZONE)
#define configENABLE_TRUSTZONE                                  1
#define configRUN_FREERTOS_SECURE_ONLY                          0
#else
#define configENABLE_TRUSTZONE                                  0
#define configRUN_FREERTOS_SECURE_ONLY                          1
#endif


/* Constants related to the generation of run time stats. */
#define configCPU_CLOCK_HZ  40000000UL
#define configGENERATE_RUN_TIME_STATS                           1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE()                        ulGetRunTimeCounterValue()
#define configTICK_RATE_HZ                                      ( 1000 ) //change by xradio

/* Constants related to the behaviour or the scheduler. */
#define configUSE_PREEMPTION                                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION                 0
#define configUSE_TIME_SLICING                                  1
#define configMAX_PRIORITIES                                    ( 32 )    //change by xradio
#define configIDLE_SHOULD_YIELD                                 0    //change by xradio
#define configUSE_16_BIT_TICKS                                  0 /* Only for 8 and 16-bit hardware. */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS                 1  // add by xradio

#ifdef CONFIG_OS_DEBUG_CPU_USAGE
#define configDEBUG_CPU_USAGE_EN                                1 /* use cpu usage statistic */
#else
#define configDEBUG_CPU_USAGE_EN                                0 /* use cpu usage statistic */
#endif

#define configDEBUG_TRACE_TASK_MOREINFO                         1 /* add some info to trace tasks, use configUSE_STATS_FORMATTING_FUNCTIONS */

/* Constants that describe the hardware and memory usage. */
#define configMINIMAL_STACK_SIZE                                ( ( uint16_t ) 4096 )  //change by xradio from 128
#define configMINIMAL_SECURE_STACK_SIZE                         ( 512 )
#define configMAXIMAL_SECURE_STACK_SIZE                         ( 1024 )
#define configMAX_TASK_NAME_LEN                                 ( 16 )  // change by xradio from 12
#ifdef CONFIG_TOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE                                   ( ( size_t ) ( CONFIG_TOTAL_HEAP_SIZE ) )
#else
#define configTOTAL_HEAP_SIZE                                   ( ( size_t ) ( 50 * 1024 ) )
#endif

/* Constants that build features in or out. */
#define configUSE_MUTEXES                                       1
#ifdef CONFIG_OS_DEBUG_CPU_USAGE
#define configUSE_TICKLESS_IDLE                                 0
#else
#define configUSE_TICKLESS_IDLE                                 0
#endif
#define configUSE_APPLICATION_TASK_TAG                          1
#define configUSE_NEWLIB_REENTRANT                              0
#define configUSE_CO_ROUTINES                                   0
//#define configMAX_CO_ROUTINE_PRIORITIES                         2  // closed by xradio, old tech, different with 871 #########!!!!
#define configUSE_COUNTING_SEMAPHORES                           1
#define configUSE_ALTERNATIVE_API                               0 /* Deprecated!, add by xradio */
#define configUSE_RECURSIVE_MUTEXES                             1
#define configUSE_QUEUE_SETS                                    1  //open by xradio
#define configUSE_TASK_NOTIFICATIONS                            1
#define configUSE_TRACE_FACILITY                                1
#define configENABLE_BACKWARD_COMPATIBILITY                     1

/* Constants that define which hook (callback) functions should be used. */
#ifdef CONFIG_OS_DEBUG_CPU_USAGE
#define configUSE_IDLE_HOOK                                     1
#else
#ifdef CONFIG_OS_USE_IDLE_HOOK
#define configUSE_IDLE_HOOK                                     1
#else
#define configUSE_IDLE_HOOK                                     0
#endif
#endif
#define configUSE_TICK_HOOK                                     0
#define configUSE_MALLOC_FAILED_HOOK                            0
#define configUSE_DAEMON_TASK_STARTUP_HOOK                      0 // add by xradio

/* Constants provided for debugging and optimisation assistance. */
#define configCHECK_FOR_STACK_OVERFLOW                          2
void vAssertCalled(const char *pcFile, unsigned long ulLine);
#define configASSERT( x ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ );
#define configQUEUE_REGISTRY_SIZE                               0

/* Software timer definitions. */
#define configUSE_TIMERS                                        1
#define configTIMER_TASK_PRIORITY                               ( configMAX_PRIORITIES - 1 )  // change by xradio from 3
#define configTIMER_QUEUE_LENGTH                                10 //change by xradio from 5
#define configTIMER_TASK_STACK_DEPTH                            ( 512 ) // change by xradio from configMINIMAL_STACK_SIZE

/* Set the following definitions to 1 to include the API function, or zero
 * to exclude the API function.  NOTE:  Setting an INCLUDE_ parameter to 0 is
 * only necessary if the linker does not automatically remove functions that are
 * not referenced anyway. */
#define INCLUDE_vTaskPrioritySet                                1
#define INCLUDE_uxTaskPriorityGet                               1
#define INCLUDE_vTaskDelete                                     1
#define INCLUDE_vTaskCleanUpResources                           0
#define INCLUDE_vTaskSuspend                                    1
#define INCLUDE_vTaskDelayUntil                                 1
#define INCLUDE_vTaskDelay                                      1
#define INCLUDE_uxTaskGetStackHighWaterMark                     1  //open by xradio for tasklist
#define INCLUDE_xTaskGetIdleTaskHandle                          0
#define INCLUDE_eTaskGetState                                   1
#define INCLUDE_xTaskResumeFromISR                              1  //open by xradio
#define INCLUDE_xTaskGetCurrentTaskHandle                       1
#define INCLUDE_xTaskGetSchedulerState                          1  //open by xradio for tasklist
#define INCLUDE_xSemaphoreGetMutexHolder                        1  //open by xradio
#define INCLUDE_xTimerPendFunctionCall                          1
#define INCLUDE_xTaskGetHandle                                  1

/* This demo makes use of one or more example stats formatting functions.  These
 * format the raw data provided by the uxTaskGetSystemState() function in to
 * human readable ASCII form.  See the notes in the implementation of vTaskList()
 * within FreeRTOS/Source/tasks.c for limitations. */
#define configUSE_STATS_FORMATTING_FUNCTIONS                    1

/* Dimensions a buffer that can be used by the FreeRTOS+CLI command interpreter.
 * See the FreeRTOS+CLI documentation for more information:
 * http://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/ */
#define configCOMMAND_INT_MAX_OUTPUT_SIZE                       2048

/* Interrupt priority configuration follows...................... */

/* Use the system definition, if there is one. */
#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS                                         __NVIC_PRIO_BITS
#else
#define configPRIO_BITS                                         3         /* 8 priority levels. */
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
 * function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY                 0x07

/* The highest interrupt priority that can be used by any interrupt service
 * routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT
 * CALL INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A
 * HIGHER PRIORITY THAN THIS! (higher priorities are lower numeric values). */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY            5

/* Interrupt priorities used by the kernel port layer itself.  These are generic
 * to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY                         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
 * See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#ifdef CONFIG_COMPONENTS_TFM
#define configMAX_SYSCALL_INTERRUPT_PRIORITY                    ( 2 << ( 8 - configPRIO_BITS ) ) //changed by xradio from configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#else
/* Note: interrupt with interrupt logic priority higher than this threshold can not be blocked by kernel critical area
 * which will cause unsafe kernel critical area function. Therefore, it is recommended not to modify this threshold,
 * setting to 1 is the highest, and other peripheral interrupt priority value should be set above 1 -- @xradio
 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY                    ( 1 << ( 8 - configPRIO_BITS ) ) //changed by xradio from configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#endif

/* Enable static allocation. */
#define configSUPPORT_STATIC_ALLOCATION                         1  //closed by xradio

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
   standard names. */
#if 1
#define vPortSVCHandler                 SVC_Handler
#define xPortPendSVHandler              PendSV_Handler
#define xPortSysTickHandler             SysTick_Handler
#endif

/* A header file that defines trace macro can be included here. */

////////////////////////////////////////////////////////////////////////////////

/* disable some features for bootloader to reduce code size */
#if (defined(CONFIG_BOOTLOADER) && !defined(CONFIG_ROM_FREERTOS))

//TODO: undef some micro to use little FREERTOS

#endif /* CONFIG_BOOTLOADER */

#endif /* FREERTOS_CONFIG_H */
