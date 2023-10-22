#ifndef X_CPUFREQ_H_
#define X_CPUFREQ_H_

struct cpufreq_frequency_table {
	unsigned int	frequency; /* Hz, max: 4294967295 */
	int		target_uV;
};

struct cpufreq_info{
	unsigned int			cpu;
	unsigned int			clk;
	struct regulator_dev		*rdev;
	struct cpufreq_frequency_table	*freq_table;
};
#endif /* X_CPUFREQ_H_ */
