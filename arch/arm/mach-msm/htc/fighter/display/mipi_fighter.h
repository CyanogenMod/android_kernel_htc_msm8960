#ifndef MIPI_FIGHTER_H
#define MIPI_FIGHTER_H

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>

int mipi_fighter_device_register(struct msm_panel_info *pinfo,
                                 u32 channel, u32 panel);

#define FIGHTER_USE_CMDLISTS 1

#define PWM_MIN                   7
#define PWM_DEFAULT               91
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

#endif
