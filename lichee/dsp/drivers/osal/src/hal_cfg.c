#include <hal_osal.h>
#include <hal_cfg.h>
#include <script.h>

script_parser_t *Hal_Cfg_Parser = NULL;

#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
int32_t hal_cfg_init(void)
{
#ifdef CONFIG_SYS_CONFIG_BUILDIN
	extern unsigned char blob_fexconfig_start[];
	Hal_Cfg_Parser = script_parser_init(blob_fexconfig_start);
#elif defined(CONFIG_SYS_CONFIG_PACK)
	Hal_Cfg_Parser = script_parser_init((void *)CONFIG_SYS_CONFIG_PACK_ADDRESS);
#endif
	if(Hal_Cfg_Parser)
		return HAL_OK;
	else
		return HAL_ERROR;
}

int32_t hal_cfg_exit(void)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_exit(Hal_Cfg_Parser);
}

int32_t hal_cfg_get_keyvalue(char *SecName, char *KeyName, int32_t Value[], int32_t Count)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_fetch(Hal_Cfg_Parser, SecName, KeyName, Value, Count);
}

int32_t hal_cfg_get_sec_keycount(char *SecName)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_subkey_count(Hal_Cfg_Parser, SecName);
}

int32_t hal_cfg_get_sec_count(void)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_mainkey_count(Hal_Cfg_Parser);
}

int32_t hal_cfg_get_gpiosec_keycount(char *name)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_mainkey_get_gpio_count(Hal_Cfg_Parser, name);
}

int32_t hal_cfg_get_gpiosec_data(char *GPIOSecName, void *pGPIOCfg, int32_t GPIONum)
{
	if (!Hal_Cfg_Parser)
		return HAL_INVALID;
	return script_parser_mainkey_get_gpio_cfg(Hal_Cfg_Parser, GPIOSecName, pGPIOCfg, GPIONum);
}

void hal_cfg_show(void)
{
	script_show(Hal_Cfg_Parser);
}
#else
int32_t hal_cfg_init(void)
{
	return HAL_ERROR;
}

int32_t hal_cfg_exit(void)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_keyvalue(char *SecName, char *KeyName, int32_t Value[], int32_t Count)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_sec_keycount(char *SecName)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_sec_count(void)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_gpiosec_keycount(char *name)
{
	return HAL_ERROR;
}

int32_t hal_cfg_get_gpiosec_data(char *GPIOSecName, void *pGPIOCfg, int32_t GPIONum)
{
	return HAL_ERROR;
}

void Hal_cfg_show(void)
{
	printf("Not support sys config interface\r\n");
}

#endif
