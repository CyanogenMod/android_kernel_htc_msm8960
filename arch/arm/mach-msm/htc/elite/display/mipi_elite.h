#ifndef MIPI_ELITE_H
#define MIPI_ELITE_H

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>

int mipi_elite_device_register(struct msm_panel_info *pinfo,
                                 u32 channel, u32 panel);

#define ELITE_USE_CMDLISTS 1
#define PWM_MIN                   6
#define PWM_DEFAULT               91
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

#define EVA_CMD_MODE_PANEL
#undef EVA_VIDEO_MODE_PANEL
#undef EVA_SWITCH_MODE_PANEL

#endif /* !MIPI_ELITE_H */
