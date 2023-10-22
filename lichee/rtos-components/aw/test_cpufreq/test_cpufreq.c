#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hal_cmd.h>
#include <cpufreq.h>

int cmd_test_set_cpu_freq_api(int argc, char *argv[])
{
	char *ptr = NULL;
	errno = 0;
	uint32_t cpu_freq = 0, cpu_voltage = 0;

	if (argc != 2)
	{
		printf("invalid input parameter num(%d)!\n", argc);
		return 0;
	}

	unsigned long target_cpu_freq = strtoul(argv[1], &ptr, 10);
	if (errno || (ptr && *ptr != '\0'))
	{
		printf("invalid input parameter('%s')!\n", argv[1]);
		return 0;
	}

	int ret = set_cpu_freq(target_cpu_freq);
	if (ret)
	{
		printf("set_cpu_freq failed, ret: %d\n", ret);
		return -1;
	}

	ret = get_cpu_freq(&cpu_freq);
	if (ret)
	{
		printf("get_cpu_freq failed, ret: %d\n", ret);
		return -1;
	}

	printf("current cpu frequency: %uHz\n", cpu_freq);

	ret = get_cpu_voltage(&cpu_voltage);
	if (ret)
	{
		printf("get_cpu_voltage failed, ret: %d\n", ret);
		return -1;
	}

	printf("current cpu voltage: %umV\n", cpu_voltage);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_set_cpu_freq_api, set_cpu_freq, set_cpu_freq API test);

int cmd_test_get_cpu_freq_api(int argc, char *argv[])
{
	uint32_t cpu_freq = 0, cpu_voltage = 0;
	int ret = get_cpu_freq(&cpu_freq);
	if (ret)
	{
		printf("get_cpu_freq failed, ret: %d\n", ret);
		return -1;
	}

	printf("current cpu frequency: %uHz\n", cpu_freq);

	ret = get_cpu_voltage(&cpu_voltage);
	if (ret)
	{
		printf("get_cpu_voltage failed, ret: %d\n", ret);
		return -1;
	}

	printf("current cpu voltage: %umV\n", cpu_voltage);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_get_cpu_freq_api, get_cpu_freq, get_cpu_freq API test);

int cmd_test_get_available_cpu_freq_api(int argc, char *argv[])
{
	uint32_t cpu_freq = 0, cpu_voltage = 0, freq_num, freq_index;
	int ret;

	freq_num = get_available_cpu_freq_num();
	if (!freq_num)
	{
		printf("no available cpu frequency!\n");
		return -1;
	}

	for (freq_index = 0; freq_index < freq_num; freq_index++)
	{
		ret = get_available_cpu_freq_info(freq_index, &cpu_freq, &cpu_voltage);
		if (ret)
		{
			printf("get_available_cpu_freq_info failed, freq_index: %d, ret: %d\n", freq_index, ret);
			continue;
		}
		printf("available cpu frequency %d: %uHz, %umV\n", freq_index, cpu_freq, cpu_voltage);
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_get_available_cpu_freq_api, get_available_cpu_freq, get_available_cpu_freq API test);
