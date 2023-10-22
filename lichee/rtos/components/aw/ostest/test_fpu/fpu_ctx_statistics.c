#include <stdio.h>
#include <hal_cmd.h>

extern uint32_t g_skip_fpu_ctx_save_cnt;
extern uint32_t g_skip_fpu_off_ctx_restore_cnt, g_skip_fpu_init_ctx_restore_cnt, g_skip_fpu_clean_ctx_restore_cnt;

int cmd_show_c906_fpu_statistics(int argc, char **argv)
{
	printf("C906 FPU context save/restore operation statistics info:\n");
	printf("skip save operation count(FPU is not dirty): %u\n", g_skip_fpu_ctx_save_cnt);
	printf("skip restore operation count(FS field in thread stack is off): %u\n", g_skip_fpu_off_ctx_restore_cnt);
	printf("skip restore operation count(FS field in thread stack is init): %u\n", g_skip_fpu_init_ctx_restore_cnt);
	printf("skip restore operation count(FPU is clean and no thread switch): %u\n", g_skip_fpu_clean_ctx_restore_cnt);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_show_c906_fpu_statistics, show_c906_fpu_stat, Show C906 FPU context save/restore operation statistics info);
