/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ARCH_ARM_MACH_MSM_BOARD_monarudo_H
#define __ARCH_ARM_MACH_MSM_BOARD_monarudo_H

#include <linux/regulator/machine.h>
#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8821.h>
#include <mach/msm_memtypes.h>
#include <mach/irqs.h>
#include <mach/rpm-regulator.h>

#define EVM	0x99
#define EVM1	99
#define XA	0
#define XB	1
#define XC	2
#define XD	3
#define PVT	0x80
#define PVT_1	0x81

#define GPIO(x) (x)
#define PMGPIO(x) (x)

int __init monarudo_init_keypad(void);



#define LCD_TE			GPIO(0)
#define DRIVER_EN_XA_XB		GPIO(1)
#define RAW_RST_XC_XD		GPIO(1)
#define CAM2_RSTz		GPIO(2)
#define WDOG_DISABLE_XA		GPIO(3)
#define CHG_WDT_EN_XB_XC	GPIO(3)
#define NC_GPIO3_PVT		GPIO(3)
#define CAM2_MCLK_XA_XB_XC	GPIO(4)
#define CAM_SEL_XD		GPIO(4)
#define CAM_MCLK0		GPIO(5)
#define RAW_INT0_XA_XB		GPIO(6)
#define AUD_HAC_SDz_XC		GPIO(6)
#define RAW_INT1_XA_XB		GPIO(7)
#define HAPTIC_3V3_EN_XC_XD	GPIO(7)
#define I2C3_DATA_TS		GPIO(8)
#define I2C3_CLK_TS		GPIO(9)
#define NFC_UART_TX_XA_XB		GPIO(10)
#define NC_GPIO_10_XC		GPIO(10)
#define MCAM_D1V2_EN_XD		GPIO(10)
#define NFC_UART_RX_XA_XB		GPIO(11)
#define VOL_UPz_XC			GPIO(11)
#define I2C4_DATA_CAM		GPIO(12)
#define I2C4_CLK_CAM		GPIO(13)
#define WIRELESS_CHARGER_DISABLE_XA_XB			GPIO(14)
#define BT_UART_TX_XC		GPIO(14)
#define WIRELESS_CHARGER_INT_XA_XB			GPIO(15)
#define BT_UART_RX_XC		GPIO(15)
#define BT_CTL_XA_XB			GPIO(16)
#define BT_UART_CTSz_XC		GPIO(16)
#define BT_DATA_XA_XB			GPIO(17)
#define BT_UART_RTSz_XC		GPIO(17)
#define AP2MDM_ERR_FATAL	GPIO(18)
#define MDM2AP_ERR_FATAL	GPIO(19)
#define I2C1_DATA_APPS		GPIO(20)
#define I2C1_CLK_APPS		GPIO(21)
#define CPU_1WIRE_TX		GPIO(22)
#define CPU_1WIRE_RX		GPIO(23)
#define I2C2_DATA_SENS		GPIO(24)
#define I2C2_CLK_SENS		GPIO(25)
#define PWR_KEY_MSMz		GPIO(26)
#define WS			GPIO(27)
#define SCLK			GPIO(28)
#define DOUT			GPIO(29)
#define AP2MDM_VDDMIN		GPIO(30)
#define LCD_RST			GPIO(31)
#define DIIN			GPIO(32)
#define TP_RSTz_XA_XB			GPIO(33)
#define V_CAM_D1V2_EN_XC_XD		GPIO(33)
#define TP_ATTz			GPIO(34)
#define AP2MDM_WAKEUP_XA_XB		GPIO(35)
#define AUD_FM_I2S_BCLK_XC		GPIO(35)
#define MBAT_IN_XA_XB			GPIO(36)
#define AUD_FM_I2S_SYNC1_XC		GPIO(36)
#define MHL_RSTz_XA		GPIO(37)
#define BL_HW_EN_XB		GPIO(37)
#define AUD_FM_I2S_SYNC2_XC		GPIO(37)
#define MHL_INT			GPIO(38)
#define AUD_CPU_MCLK		GPIO(39)
#define SLIMBUS1_CLK		GPIO(40)
#define SLIMBUS1_DATA		GPIO(41)
#define AUD_WCD_INTR_OUT	GPIO(42)
#define RAW_RST_XA_XB			GPIO(43)
#define AUD_BTPCM_DIN_XC		GPIO(43)
#define APQ2MDM_IPC2_XA_XB		GPIO(44)
#define AUD_BTPCM_DOUT_XC		GPIO(44)
#define VOL_DOWNz_XA_XB		GPIO(45)
#define AUD_BTPCM_SYNC_XC		GPIO(45)
#define MDM2AP_HSIC_READY_XA_XB	GPIO(46)
#define AUD_BTPCM_CLK_XC		GPIO(46)
#define AP2MDM_SOFT_RESET	GPIO(47)
#define AP2MDM_STATUS		GPIO(48)
#define MDM2AP_STATUS		GPIO(49)
#define BOOT_CONFIG_1		GPIO(50)
#define MCAM_SPI_DO		GPIO(51)
#define MCAM_SPI_DI		GPIO(52)
#define MCAM_SPI_CS0		GPIO(53)
#define MCAM_SPI_CLK		GPIO(54)
#define VOL_UPz_XA_XB			GPIO(55)
#define NFC_DL_MODE_XC			GPIO(55)
#define NFC_IRQ			GPIO(56)
#define NFC_VEN			GPIO(57)
#define WCN_PRIORITY		GPIO(58)
#define AP2MDM_PON_RESET_N	GPIO(59)
#define MDM_LTE_FRAME_SYNC	GPIO(60)
#define MDM_LTE_ACTIVE		GPIO(61)
#define USB1_HS_ID_GPIO_XA_XB		GPIO(62)
#define GSENSOR_INT_XC		GPIO(62)
#define BT_SSBI_XA_XB			GPIO(63)
#define RAW_INT0_XC_XD		GPIO(63)
#define WLAN_DATA_2_XA_XB		GPIO(64)
#define APQ2MDM_IPC3_XC		GPIO(64)
#define WLAN_DATA_1_XA_XB		GPIO(65)
#define RAW_INT1_XC_XD		GPIO(65)
#define WLAN_DATA_0_XA_XB		GPIO(66)
#define AP2MDM_WAKEUP_XC		GPIO(66)
#define WL_CMD_SET_XA_XB		GPIO(67)
#define WIRELESS_CHARGER_DISABLE_XC			GPIO(67)
#define WL_CMD_CLK_XA_XB		GPIO(68)
#define APQ2MDM_IPC2_XC		GPIO(68)
#define HDMI_CEC		GPIO(69)
#define HDMI_DDC_CLK		GPIO(70)
#define HDMI_DDC_DATA		GPIO(71)
#define HDMI_HPLG_DET		GPIO(72)
#define PM8921_APC_SEC_IRQ_N	GPIO(73)
#define PM8921_APC_USR_IRQ_N	GPIO(74)
#define PM8921_MDM_IRQ_N	GPIO(75)
#define PM8821_APC_SEC_IRQ_N	GPIO(76)
#define RESOUTz_CONTROL_XA		GPIO(77)
#define RESET_EN_CLRz_XB		GPIO(77)
#define VOL_DOWNz_XC		GPIO(77)
#define PS_HOLD_APQ		GPIO(78)
#define SSBI_PM8821		GPIO(79)
#define MDM2AP_VDDMIN		GPIO(80)
#define APQ2MDM_IPC1		GPIO(81)
#define UART_TX			GPIO(82)
#define UART_RX			GPIO(83)
#define APQ2MDM_IPC3_XA_XB		GPIO(84)
#define MDM2AP_HSIC_READY_XC		GPIO(84)
#define V_CAM_D1V2_EN_XA_XB		GPIO(85)
#define TP_RSTz_XC			GPIO(85)
#define V_CAM2_D1V2_EN_XA_XB		GPIO(86)
#define BL_HW_EN_XC_XD		GPIO(86)
#define BL_HW_EN_XC		GPIO(86)
#define BOOT_CONFIG_0_XA_XB		GPIO(87)
#define RESET_EN_CLRz_XC		GPIO(87)
#define HSIC_STROBE		GPIO(88)
#define HSIC_DATA		GPIO(89)

#define CAM_VCM_PD			PMGPIO(1)
#define HAPTIC_3V3_EN_XA_XB		PMGPIO(2)
#define MHL_RSTz_XC_XD		PMGPIO(2)
#define GYRO_INT		PMGPIO(3)
#define USB_ID_ADC_XA		PMGPIO(4)
#define AUDIO_SPK_RSTz_XB_XC	PMGPIO(4)
#define V_RAW_1V8_EN		PMGPIO(5)
#define COMPASS_AKM_INT		PMGPIO(6)
#define AUO_REMO_PRESz_XA_XB		PMGPIO(7)
#define USB1_HS_ID_GPIO_XC		PMGPIO(7)
#define PMIC_1WIRE_RX_XA_XB		PMGPIO(8)
#define BT_REG_ON_XC		PMGPIO(8)
#define V_AUD_HSMIC_2V85_EN	PMGPIO(9)
#define AUD_HP_EN		PMGPIO(10)
#define HAPTIC_EN		PMGPIO(11)
#define NFC_DL_MODE_XA_XB		PMGPIO(12)
#define V_CAM2_D1V8_EN_XC_XD		PMGPIO(12)
#define V_TP_3V3_EN		PMGPIO(13)
#define AUDIOz_MHL_SW		PMGPIO(14)
#define USBz_AUDIO_SW		PMGPIO(15)
#define MAIN_CAM_ID_XA_XB		PMGPIO(16)
#define WL_REG_ON_XC		PMGPIO(16)
#define PROXIMITY_INT		PMGPIO(17)
#define TORCH_FLASHz		PMGPIO(18)
#define AUO_SPK_ADC_EN_XA_XB		PMGPIO(19)
#define DRIVER_EN_XC		PMGPIO(19)
#define EARPHONE_DETz		PMGPIO(20)
#define GREEN_BACK_LED_XA_XB		PMGPIO(21)
#define NC_PMGPIO_21_XC		PMGPIO(21)
#define LCD_ID_PVT		PMGPIO(21)
#define AMBER_BACK_LED_XA_XB		PMGPIO(22)
#define NC_PMGPIO_22_XC		PMGPIO(22)
#define MAIN_CAM_ID_PVT		PMGPIO(22)
#define GSENSOR_INT_XA_XB		PMGPIO(23)
#define BT_WAKE_XC		PMGPIO(23)
#define GREEN_LED_XA_XB		PMGPIO(24)
#define GREEN_BACK_LED_XC_XD PMGPIO(24)
#define AMBER_LED_XA_XB		PMGPIO(25)
#define AMBER_BACK_LED_XC_XD PMGPIO(25)
#define HAPTIC_PWM		PMGPIO(26)
#define NC_PMGPIO_27		PMGPIO(27)
#define NC_PMGPIO_28		PMGPIO(28)
#define NC_PMGPIO_29		PMGPIO(29)
#define WC_TX_WPGz_PVT		PMGPIO(29)
#define NC_PMGPIO_30		PMGPIO(30)
#define NC_PMGPIO_31		PMGPIO(31)
#define NC_PMGPIO_32		PMGPIO(32)
#define LCD_ID0_XA_XB			PMGPIO(33)
#define BT_HOST_WAKE_XC		PMGPIO(33)
#define AUD_WCD_RESET_N		PMGPIO(34)
#define LCD_ID1_XA_XB			PMGPIO(35)
#define WL_DEV_WAKE_XC		PMGPIO(35)
#define V_LCM_N5V_EN		PMGPIO(36)
#define V_LCM_P5V_EN		PMGPIO(37)
#define AUD_UART_OEz_XA_XB		PMGPIO(38)
#define WL_HOST_WAKE_XC		PMGPIO(38)
#define SSBI_PMIC_FWD_CLK	PMGPIO(39)
#define NC_PMGPIO_40		PMGPIO(40)
#define REGION_ID_PVT		PMGPIO(40)
#define NC_PMGPIO_41_XA_XB		PMGPIO(41)
#define AUD_UART_OEz_XC		PMGPIO(41)
#define CAM1_PWDN		PMGPIO(42)
#define PM8921_SLEEP_CLK1_XA_XB		PMGPIO(43)
#define NC_PMGPIO_43		PMGPIO(43)
#define AUD_HAC_SDz_XA_XB		PMGPIO(44)
#define BCM4330_SLEEP_CLK_XC		PMGPIO(44)
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_mpp)	(pm_mpp - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)

#define PM8821_MPP_BASE			(PM8921_MPP_BASE + PM8921_NR_MPPS)
#define PM8821_MPP_PM_TO_SYS(pm_mpp)	(pm_mpp - 1 + PM8821_MPP_BASE)
#define PM8821_IRQ_BASE			(PM8921_IRQ_BASE + PM8921_NR_IRQS)

#ifdef CONFIG_RESET_BY_CABLE_IN
#define AC_WDT_EN_XB		GPIO(3)
#define AC_WDT_RST_XB		GPIO(77)
#define AC_WDT_EN_XC		GPIO(3)
#define AC_WDT_RST_XC		GPIO(87)
#endif

extern struct pm8xxx_regulator_platform_data
	monarudo_pm8921_regulator_pdata[] __devinitdata;

extern int monarudo_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_3P3V		1
#define GPIO_VREG_ID_EXT_TS_SW		2
#define GPIO_VREG_ID_EXT_MPP8		3
#define monarudo_EXT_3P3V_REG_EN_GPIO	77

extern struct gpio_regulator_platform_data
	monarudo_gpio_regulator_pdata[] __devinitdata;

extern struct rpm_regulator_platform_data
	monarudo_rpm_regulator_pdata __devinitdata;

extern struct regulator_init_data monarudo_saw_regulator_pdata_8921_s5;
extern struct regulator_init_data monarudo_saw_regulator_pdata_8921_s6;
extern struct regulator_init_data monarudo_saw_regulator_pdata_8821_s0;
extern struct regulator_init_data monarudo_saw_regulator_pdata_8821_s1;

struct mmc_platform_data;
int __init apq8064_add_sdcc(unsigned int controller,
		struct mmc_platform_data *plat);

void monarudo_init_mmc(void);
int monarudo_wifi_init(void);
void monarudo_init_gpiomux(void);
void monarudo_init_pmic(void);

#if 1	
extern struct platform_device monarudo_msm_rawchip_device;
#endif
extern struct msm_camera_board_info monarudo_camera_board_info;
extern struct msm_camera_board_info monarudo_camera_board_info_xd;
void monarudo_init_cam(void);

#define APQ_8064_GSBI1_QUP_I2C_BUS_ID 0
#define APQ_8064_GSBI3_QUP_I2C_BUS_ID 3
#define APQ_8064_GSBI4_QUP_I2C_BUS_ID 4

void monarudo_init_fb(void);
void monarudo_allocate_fb_region(void);
void monarudo_mdp_writeback(struct memtype_reserve *reserve_table);

void monarudo_init_gpu(void);
void monarudo_pm8xxx_gpio_mpp_init(void);
void monarudo_usb_uart_switch(int nvbus);

#ifdef CONFIG_RESET_BY_CABLE_IN
void reset_dflipflop(void);
#endif

#endif
