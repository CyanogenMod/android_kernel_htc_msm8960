#ifndef MIPI_JET_H
#define MIPI_JET_H

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>

int mipi_jet_device_register(struct msm_panel_info *pinfo,
                                 u32 channel, u32 panel);

#define JET_USE_CMDLISTS 1

#define PWM_MIN                   6
#define PWM_DEFAULT               91
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

#define JEL_CMD_MODE_PANEL
#undef JEL_VIDEO_MODE_PANEL
#undef JEL_SWITCH_MODE_PANEL

#endif /* !MIPI_JET_H */
