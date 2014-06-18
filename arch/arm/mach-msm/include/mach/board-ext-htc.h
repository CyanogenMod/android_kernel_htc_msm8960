/* arch/arm/mach-msm/include/mach/board-ext-htc.h
 *
 * HTC board.h extensions
 *
 * Copyright (C) 2013 CyanogenMod
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_MSM_BOARD_EXT_HTC_H
#define __ASM_ARCH_MSM_BOARD_EXT_HTC_H

#define SHIP_BUILD	0
#define MFG_BUILD	1
#define ENG_BUILD	2

#if defined(CONFIG_USB_FUNCTION_MSM_HSUSB) \
	|| defined(CONFIG_USB_MSM_72K) || defined(CONFIG_USB_MSM_72K_MODULE) \
	|| defined(CONFIG_USB_CI13XXX_MSM)
void msm_otg_set_vbus_state(int online);

enum usb_connect_type {
	CONNECT_TYPE_NOTIFY = -3,
	CONNECT_TYPE_CLEAR = -2,
	CONNECT_TYPE_UNKNOWN = -1,
	CONNECT_TYPE_NONE = 0,
	CONNECT_TYPE_USB,
	CONNECT_TYPE_AC,
	CONNECT_TYPE_9V_AC,
	CONNECT_TYPE_WIRELESS,
	CONNECT_TYPE_INTERNAL,
	CONNECT_TYPE_UNSUPPORTED,
#ifdef CONFIG_MACH_VERDI_LTE
	CONNECT_TYPE_USB_9V_AC,
#endif
	CONNECT_TYPE_MHL_AC,
};
#endif

struct t_usb_status_notifier {
	struct list_head notifier_link;
	const char *name;
	void (*func)(int cable_type);
};
int htc_usb_register_notifier(struct t_usb_status_notifier *notifer);
int usb_get_connect_type(void);
static LIST_HEAD(g_lh_usb_notifier_list);

struct t_cable_status_notifier{
	struct list_head cable_notifier_link;
	const char *name;
	void (*func)(int cable_type);
};
int cable_detect_register_notifier(struct t_cable_status_notifier *);
static LIST_HEAD(g_lh_cable_detect_notifier_list);

struct t_owe_charging_notifier{
	struct list_head owe_charging_notifier_link;
	const char *name;
	void (*func)(int charging_type);
};
int owe_charging_register_notifier(struct t_owe_charging_notifier *);
static LIST_HEAD(g_lh_owe_charging_notifier_list);

struct t_mhl_status_notifier{
	struct list_head mhl_notifier_link;
	const char *name;
	void (*func)(bool isMHL, int charging_type);
};
int mhl_detect_register_notifier(struct t_mhl_status_notifier *);
static LIST_HEAD(g_lh_mhl_detect_notifier_list);

struct t_usb_host_status_notifier{
	struct list_head usb_host_notifier_link;
	const char *name;
	void (*func)(bool cable_in);
};
int usb_host_detect_register_notifier(struct t_usb_host_status_notifier *);
static LIST_HEAD(g_lh_usb_host_detect_notifier_list);

int board_mfg_mode(void);

int board_build_flag(void);

extern struct flash_platform_data msm_nand_data;

extern int emmc_partition_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data);

extern int dying_processors_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data);

extern int get_partition_num_by_name(char *name);
extern const char *get_partition_name_by_num(int partnum);

#ifdef CONFIG_FB_MSM_HDMI_MHL
typedef struct {
	uint8_t format;
	uint8_t reg_a3;
	uint8_t reg_a6;
} mhl_driving_params;
#endif

#ifdef CONFIG_MSM_CAMERA
enum msm_camera_csi_data_format {
	CSI_8BIT,
	CSI_10BIT,
	CSI_12BIT,
};

struct msm_camera_csi_params {
	enum msm_camera_csi_data_format data_format;
	uint8_t lane_cnt;
	uint8_t lane_assign;
	uint8_t settle_cnt;
	uint8_t dpcm_scheme;
};

struct camera_led_info {
	uint16_t enable;
	uint16_t low_limit_led_state;
	uint16_t max_led_current_ma;
	uint16_t num_led_est_table;
};

struct camera_led_est {
	uint16_t enable;
	uint16_t led_state;
	uint16_t current_ma;
	uint16_t lumen_value;
	uint16_t min_step;
	uint16_t max_step;
};

struct camera_flash_info {
	struct camera_led_info *led_info;
	struct camera_led_est *led_est_table;
};

struct camera_flash_cfg {
	int num_flash_levels;
	int (*camera_flash)(int level);
	uint16_t low_temp_limit;
	uint16_t low_cap_limit;
	uint16_t low_cap_limit_dual;
	uint8_t postpone_led_mode;
	struct camera_flash_info *flash_info;
};

struct msm_camera_rawchip_info {
	int rawchip_reset;
	int rawchip_intr0;
	int rawchip_intr1;
	uint8_t rawchip_spi_freq;
	uint8_t rawchip_mclk_freq;
	int (*camera_rawchip_power_on)(void);
	int (*camera_rawchip_power_off)(void);
	int (*rawchip_use_ext_1v2)(void);
};

enum rawchip_enable_type {
	RAWCHIP_DISABLE,
	RAWCHIP_ENABLE,
	RAWCHIP_DXO_BYPASS,
	RAWCHIP_MIPI_BYPASS,
};

enum hdr_mode_type {
	NON_HDR_MODE,
	HDR_MODE,
};

enum camera_vreg_type {
	REG_LDO,
	REG_VS,
	REG_GPIO,
};

enum sensor_flip_mirror_info {
	CAMERA_SENSOR_NONE,
	CAMERA_SENSOR_MIRROR,
	CAMERA_SENSOR_FLIP,
	CAMERA_SENSOR_MIRROR_FLIP,
};

struct camera_vreg_t {
	char *reg_name;
	enum camera_vreg_type type;
	int min_voltage;
	int max_voltage;
	int op_mode;
};

enum msm_camera_pixel_order_default {
	MSM_CAMERA_PIXEL_ORDER_GR,
	MSM_CAMERA_PIXEL_ORDER_RG,
	MSM_CAMERA_PIXEL_ORDER_BG,
	MSM_CAMERA_PIXEL_ORDER_GB,
};

enum sensor_mount_angle {
	ANGLE_90,
	ANGLE_180,
	ANGLE_270,
};

enum htc_camera_image_type_board {
	HTC_CAMERA_IMAGE_NONE_BOARD,
	HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	HTC_CAMERA_IMAGE_MAX_BOARD,
};

enum cam_vcm_onoff_type {
	STATUS_OFF,
	STATUS_ON,
};
#endif /* CONFIG_MSM_CAMERA */

#endif /* __ASM_ARCH_MSM_BOARD_EXT_HTC_H */
