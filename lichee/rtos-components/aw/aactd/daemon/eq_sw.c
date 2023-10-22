#include "eq_sw.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

static pthread_t eq_thread_id;

static pthread_mutex_t mutex_eq_process;

static uint8_t *local_com_buf = NULL;
static struct aactd_com local_com;
static pthread_mutex_t mutex_local_com;
static pthread_cond_t cond_local_com_updated;
static pthread_cond_t cond_local_com_release;

static void *eq_send_thread(void *arg)
{
	tAudioEqual *ae = (tAudioEqual *)arg;
	ssize_t write_bytes;
	unsigned int com_buf_actual_len = com_buf_len_max;
	struct AudioEqualProcessItem item;
	BaseType_t xStatus;

	aactd_info("EQ_SW send start \n");

	ae->keep_alive |= EQ_ALREADY_RUN_BIT;

	pthread_mutex_lock(&mutex_local_com);
	/*
	 * If local_com.header.data_len is not 0, meaning that local_com data has
	 * been updated before. So write the previous data to the client.
	 */
	if (local_com.header.data_len != 0) {
		AACTD_DEBUG(1, "Write current internal EQ_SW parameters to client\n");
		com_buf_actual_len =
					sizeof(struct aactd_com_header) + local_com.header.data_len + 1;

		item.data = local_com_buf;
		item.len = com_buf_actual_len;
		xStatus = xQueueSend(ae->ae_queue, &item, 0);
		if (xStatus != 0) {
			aactd_error("queue send error\n");
			pthread_mutex_unlock(&mutex_local_com);
			goto exit;
		}
	}
	pthread_mutex_unlock(&mutex_local_com);

	while (1) {

		pthread_mutex_lock(&mutex_local_com);
		pthread_cond_wait(&cond_local_com_updated, &mutex_local_com);

		if (ae->keep_alive & EQ_DESTROY_BIT) {
			aactd_error("stop send thread\n");
			pthread_mutex_unlock(&mutex_local_com);
			goto exit;
		}
		if (!(ae->keep_alive & EQ_WANT_RUN_BIT)){
			AACTD_DEBUG(1, "stop send thread\n");
			pthread_mutex_unlock(&mutex_local_com);
			goto exit;
		}

		AACTD_DEBUG(1, "EQ_SW parameters updated, write to client\n");
		AACTD_DEBUG(2, "Print EQ_SW local parameters:\n");
		if (verbose_level >= 2) {
			aactd_com_print_content(&local_com);
		}

		com_buf_actual_len =
					sizeof(struct aactd_com_header) + local_com.header.data_len + 1;

		item.data = local_com_buf;
		item.len = com_buf_actual_len;
		xStatus = xQueueSend(ae->ae_queue, &item, 0);
		if (xStatus != pdPASS) {
			aactd_error("queue send error\n");
			pthread_mutex_unlock(&mutex_local_com);
			goto exit;
		}

		pthread_mutex_unlock(&mutex_local_com);
	}

exit:
	ae->keep_alive &= ~EQ_ALREADY_RUN_BIT;

	com_buf_actual_len = com_buf_len_max;

	memset(local_com_buf, 0 , com_buf_actual_len);
	item.data = local_com_buf;
	item.len = com_buf_len_max;
	xStatus = xQueueSend(ae->ae_queue, &item, 0);
	if (xStatus != pdPASS) {
		aactd_error("queue send error\n");
	}

	AACTD_DEBUG(1, "send with EQ_SW closed\n");

	pthread_exit(NULL);
}

static void *eq_process_thread(void *arg)
{
	int ret;
	pthread_t eq_send_thread_id;
	tAudioEqual *ae;
	pthread_attr_t attr;
	struct sched_param sched;

	ae = (tAudioEqual *)malloc(sizeof(tAudioEqual));
	if (!ae) {
		aactd_error("no memory");
		return NULL;
	}

	memset(ae, 0, sizeof(tAudioEqual));

	pthread_mutex_lock(&mutex_eq_process);

	ae->keep_alive |= EQ_WANT_RUN_BIT;

	if (ae->keep_alive & EQ_ALREADY_RUN_BIT) {
		aactd_info("keep_alive state:0x%x", ae->keep_alive);
		goto wait;
	}

	ae->verbose_level = verbose_level;

	ret = AudioEqualThreadAddAE(ae);

	memset(&sched, 0, sizeof(sched));
	sched.sched_priority = configAPPLICATION_EQ_SW_PRIORITY;

	pthread_attr_init(&attr);
	pthread_attr_setschedparam(&attr, &sched);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setstacksize(&attr, EQ_SW_THREAD_STACK_SIZE);

	ret = pthread_create(&eq_send_thread_id, &attr,
			 eq_send_thread, (void *)ae);
	if (ret != 0) {
		aactd_error("Failed to create thread: eq_send_thread\n");
		ret = -1;
		goto out;
	}
	pthread_setname_np(eq_send_thread_id, "eq_sw_send");

wait:

	pthread_cond_wait(&cond_local_com_release, &mutex_eq_process);

	ae->keep_alive &= ~EQ_WANT_RUN_BIT;

	pthread_join(eq_send_thread_id, NULL);

	pthread_mutex_unlock(&mutex_eq_process);

out:

	AudioEqualThreadRemoveAE(ae);

	if (ae != NULL) {
		free(ae);
		ae = NULL;
	}
	pthread_exit(NULL);
}

int eq_sw_local_init(void)
{
	int ret;
	pthread_attr_t attr;
	struct sched_param sched;

	local_com_buf = malloc(com_buf_len_max);
	if (!local_com_buf) {
		aactd_error("No memory\n");
		ret = -ENOMEM;
		goto err_out;
	}
	local_com.data = local_com_buf + sizeof(struct aactd_com_header);
	local_com.header.data_len = 0;

	ret = pthread_mutex_init(&mutex_eq_process, NULL);
	if (ret < 0) {
		aactd_error("pthread_mutex_init error");
		goto err_out;
	}

	ret = pthread_mutex_init(&mutex_local_com, NULL);
	if (ret < 0) {
		aactd_error("pthread_mutex_init error");
		goto err_out;
	}

	ret = pthread_cond_init(&cond_local_com_updated, NULL);
	if (ret < 0) {
		aactd_error("pthread_cond_init error");
		goto err_out;
	}

	ret = pthread_cond_init(&cond_local_com_release, NULL);
	if (ret < 0) {
		aactd_error("pthread_cond_init error");
		goto err_out;
	}

	ret = AudioEqualThreadInit();
	if (ret < 0) {
		aactd_error("EqThreadInit error");
		goto err_out;
	}

	memset(&sched, 0, sizeof(sched));
	sched.sched_priority = configAPPLICATION_EQ_SW_PRIORITY;

	pthread_attr_init(&attr);
	pthread_attr_setschedparam(&attr, &sched);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setstacksize(&attr, EQ_SW_THREAD_STACK_SIZE);

	ret = pthread_create(&eq_thread_id, &attr, eq_process_thread, NULL);
	if (ret != 0) {
		aactd_error("Failed to create thread: eq_process_thread\n");
		ret = -1;
		goto free_local_data;
	}
	pthread_setname_np(eq_thread_id, "eq_sw_process");

	return 0;

free_local_data:
	free(local_com_buf);
	local_com_buf = NULL;
err_out:
	if (&mutex_eq_process) {
		pthread_mutex_destroy(&mutex_eq_process);
	}
	if (&mutex_local_com) {
		pthread_mutex_destroy(&mutex_local_com);
	}
	if (&cond_local_com_updated) {
		pthread_cond_destroy(&cond_local_com_updated);
	}
	if (&cond_local_com_release) {
		pthread_cond_destroy(&cond_local_com_release);
	}
    return ret;
}

int eq_sw_local_release(void)
{
	int ret = 0;

	if (!(&cond_local_com_release)) {
		aactd_error("Failed to create thread: eq_process_thread\n");
		ret = -1;
		return 0;
	}

	pthread_cond_signal(&cond_local_com_release);

	pthread_cond_signal(&cond_local_com_updated);

	pthread_join(eq_thread_id, NULL);

	if (&mutex_eq_process) {
		pthread_mutex_destroy(&mutex_eq_process);
	}

	if (&mutex_local_com) {
		pthread_mutex_destroy(&mutex_local_com);
	}

	if (&cond_local_com_updated) {
		pthread_cond_destroy(&cond_local_com_updated);
	}

	if (&cond_local_com_release) {
		pthread_cond_destroy(&cond_local_com_release);
	}

	if (local_com_buf) {
		free(local_com_buf);
		local_com_buf = NULL;
	}

	AudioEqualThreadDeInit();

    return ret;
}

int eq_sw_write_com_to_local(const struct aactd_com *com)
{
	unsigned int com_buf_actual_len;

	pthread_mutex_lock(&mutex_local_com);
	aactd_com_copy(com, &local_com);
	aactd_com_header_to_buf(&local_com.header, local_com_buf);
	com_buf_actual_len =
		sizeof(struct aactd_com_header) + local_com.header.data_len + 1;
	local_com.checksum = aactd_calculate_checksum(local_com_buf, com_buf_actual_len - 1);
	*(local_com_buf + com_buf_actual_len - 1) = local_com.checksum;
	pthread_cond_signal(&cond_local_com_updated);
	pthread_mutex_unlock(&mutex_local_com);

	return 0;
}

