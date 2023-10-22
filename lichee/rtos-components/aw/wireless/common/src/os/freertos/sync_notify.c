#include <os_net_sync_notify.h>

os_net_status_t snfy_free(snfy_handle_t *handle);

snfy_handle_t *snfy_new(void)
{
    snfy_handle_t *ret = malloc(sizeof(snfy_handle_t));
    if (!ret)
        goto error;

    if (os_net_sem_create(&ret->sem, 0, 0) != OS_NET_STATUS_OK)
        goto error;

    ret->ready = true;
    return ret;
error:
    snfy_free(ret);
    return NULL;
}

os_net_status_t snfy_ready(snfy_handle_t *handle, void *value)
{
    if (NULL == handle)
        return OS_NET_STATUS_PARAM_INVALID;
    if (handle->ready == false)
        return OS_NET_STATUS_FAILED;
    handle->ready = true;
    handle->result = value;
    os_net_sem_release(&handle->sem);
}

os_net_status_t snfy_free(snfy_handle_t *handle)
{
    if (NULL == handle)
        return OS_NET_STATUS_PARAM_INVALID;
    os_net_sem_delete(&handle->sem);
    free(handle);
    return OS_NET_STATUS_OK;
}

void *snfy_await(snfy_handle_t *handle, os_net_time_t wait_ms)
{
    if (NULL == handle)
        return NULL;
    if(os_net_sem_wait(&handle->sem, wait_ms) != OS_NET_STATUS_OK)
		return NULL;

	void *result = handle->result;

    return NULL;
}
