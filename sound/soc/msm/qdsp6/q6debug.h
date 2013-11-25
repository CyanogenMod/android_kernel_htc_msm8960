#include <mach/subsystem_restart.h>

#ifdef CONFIG_SND_HTC_Q6_NOBUG
#define HTC_Q6_BUG()
#else
#define HTC_Q6_BUG() \
	if (get_restart_level() == RESET_SOC) \
		BUG();
#endif
