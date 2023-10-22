/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef PUBLISHER_H_
#define PUBLISHER_H_

#include "sys/list.h"
#include "kernel/os/os.h"
#include "observer.h"
#include "looper.h"

typedef struct publisher_base
{
	looper_base *looper;
	struct list_head head;	/* TODO: it's more effective by 2-dimention list */
//	struct event_queue *queue;
//	XR_OS_Thread_t thd;
	XR_OS_Mutex_t lock;	// or uint32_t sync by atomic;
	int state;

	int (*touch)(struct publisher_base *base, observer_base *obs);
	int (*attach)(struct publisher_base *base, observer_base *obs);
	int (*detach)(struct publisher_base *base, observer_base *obs);
	int (*notify)(struct publisher_base *base, uint32_t event, uint32_t arg);
	int (*compare)(uint32_t newEvent, uint32_t obsEvent);
} publisher_base;

typedef struct publisher_factory
{
	struct publisher_base *publisher;
	struct event_queue *queue;
	XR_OS_Priority prio;
	uint32_t stack;
	uint32_t size;
	struct publisher_factory *(*set_compare)(struct publisher_factory *ctor, int (*compare)(uint32_t newEvent, uint32_t obsEvent));
	struct publisher_factory *(*set_thread_param)(struct publisher_factory *ctor, XR_OS_Priority prio, uint32_t stack);
	struct publisher_factory *(*set_msg_size)(struct publisher_factory *ctor, uint32_t size);
	struct publisher_base *(*create_publisher)(struct publisher_factory *ctor);
} publisher_factory;

typedef enum publisher_state
{
	PUBLISHER_IDLE,
	PUBLISHER_WORKING,
} publisher_state;

/* a simple & older function for create publisher.
   if you use a extension event_msg, please use factory to config msg size. */
publisher_base *publisher_create(struct event_queue *queue, int (*compare)(uint32_t newEvent, uint32_t obsEvent),
								 XR_OS_Priority prio, uint32_t stack);

/* a factory config publisher for create publisher. */
struct publisher_factory *publisher_factory_create(struct event_queue *queue);

#endif /* PUBLISHER_H_ */
