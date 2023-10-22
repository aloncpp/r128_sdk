#include <stdio.h>
#include <mmu_cache.h>

typedef int (*backtrace_callback_t)(uint32_t where, uint32_t from, int32_t level);

extern void * __backtrace(unsigned int *fp, backtrace_callback_t call, int32_t level, int32_t mute);

int dump_backtrace_entry(uint32_t where, uint32_t from, int32_t level)
{
    printf("enter [<0x%08x>] from [<0x%08x>]\n", where, from);
    return level == 0 ? 0 : 1;
}

int backtrace_outof_level(uint32_t where, uint32_t from, int32_t level)
{
    return level == 0 ? 0 : 1;
}

int backtrace_hook(uint32_t where, uint32_t from, int32_t level,
        backtrace_callback_t hook)
{
    if(hook)
    {
        return hook(where, from, level);
    }
    return 1;
}

int check_frame_corrupted(uint32_t *frame, int32_t mute)
{
    int ret = 1;
    void *save_pc = NULL;
    void *save_fp = NULL;
    void *save_lr = NULL;
    void *save_sp = NULL;

    if(!xport_is_valid_address(frame, NULL))
    {
        ret = 0;
        goto corrupted_exit;
    }

    if((uint32_t)frame % 4 != 0)
    {
        ret = 0;
        goto corrupted_exit;
    }

    save_pc = (void *)(*frame);
    save_lr = (void *)(*(frame - 1));
    save_sp = (void *)(*(frame - 2));
    save_fp = (void *)(*(frame - 3));

    if(!xport_is_valid_address(save_pc, NULL)
        || !xport_is_valid_address(save_sp, NULL)
        || !xport_is_valid_address(save_lr, NULL))
    {
        ret = 0;
    }

corrupted_exit:
    if(ret == 0 && mute == 0)
    {
        printf("Frame [<0x%p>] Corrupted:\n   \
                fp = 0x%p\n   \
                sp = 0x%p\n   \
                lr = 0x%p\n   \
                pc = 0x%p\n", frame, save_fp, save_sp, save_lr, save_pc);
    }
    return ret;
}

void dump_stack(void * frame)
{
    printf("dump_stack enter\n");

    __backtrace(frame, dump_backtrace_entry, 25, 0);
    __asm__ __volatile__("": : :"memory");

    printf("dump_stack exit\n");
}

void * get_backtrace_caller(void * frame, int32_t level)
{
    return __backtrace(frame, backtrace_outof_level, level, 1);
}
