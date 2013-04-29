#ifndef MIPI_SAMSUNG_H
#define MIPI_SAMSUNG_H

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>

int mipi_ville_device_register(struct msm_panel_info *pinfo,
                                 u32 channel, u32 panel);

#define VILLE_USE_CMDLISTS 1

#define PWM_MIN                   30
#define PWM_DEFAULT               142
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

#define AMOLED_MIN_VAL 		30
#define AMOLED_MAX_VAL 		255
#define AMOLED_DEF_VAL 		(AMOLED_MIN_VAL + (AMOLED_MAX_VAL - \
								AMOLED_MIN_VAL) / 2)
#define AMOLED_LEVEL_STEP 	((AMOLED_MAX_VAL - AMOLED_MIN_VAL) /  \
								(AMOLED_NUM_LEVELS - 1))
#define AMOLED_LEVEL_STEP_C2 	((AMOLED_MAX_VAL - AMOLED_MIN_VAL) /  \
								(AMOLED_NUM_LEVELS_C2 - 1))
#define AMOLED_GAMMA_TABLE_SIZE 25+1

#endif
