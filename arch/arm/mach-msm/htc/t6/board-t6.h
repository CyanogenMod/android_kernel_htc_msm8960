/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_T6_H
#define __ARCH_ARM_MACH_MSM_BOARD_T6_H

#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8821.h>
#include <mach/msm_memtypes.h>
#include <mach/irqs.h>
#include <mach/rpm-regulator.h>
#include <mach/msm_rtb.h>
#include <mach/msm_cache_dump.h>

#define EVM	0x99
#define EVM1	99
#define XA	0
#define XB	1
#define XC	2
#define XD	3
#define PVT	0x80

#define _GET_REGULATOR(var, name) do {				\
	var = regulator_get(NULL, name);			\
	if (IS_ERR(var)) {					\
		pr_err("'%s' regulator not found, rc=%ld\n",	\
			name, IS_ERR(var));			\
		var = NULL;					\
		return -ENODEV;					\
	}							\
} while (0)

#define GPIO(x) (x)
#define PMGPIO(x) (x)

#define MSM_LCD_TE				GPIO(0)
#define MSM_RAW_RSTz				GPIO(1)
#define RAW_RST					GPIO(1)
#define MSM_CAM2_RSTz				GPIO(2)
#define CAM2_RSTz				GPIO(2)
#define MSM_AP2MDM_WAKEUP			GPIO(3)
#define MSM_CAM_MCLK1				GPIO(4)
#define MSM_CAM_MCLK0				GPIO(5)
#define CAM_MCLK1				GPIO(4)
#define CAM_MCLK0				GPIO(5)
#define MSM_CPU_CIR_TX				GPIO(6)
#define MSM_CPU_CIR_RX				GPIO(7)
#define MSM_TP_I2C_SDA				GPIO(8)
#define MSM_TP_I2C_SCL				GPIO(9)
#define MSM_CHARGER_STAT			GPIO(10)
#define MSM_VOL_UPz				GPIO(11)
#define MSM_CAM_I2C_SDA				GPIO(12)
#define MSM_CAM_I2C_SCL				GPIO(13)
#define CAM_I2C_SDA				GPIO(12)
#define CAM_I2C_SCL				GPIO(13)
#define MSM_WCSS_FM_SSBI			GPIO(14)
#define MSM_FM_DATA				GPIO(15)
#define MSM_BT_CTL				GPIO(16)
#define MSM_BT_DATA				GPIO(17)
#define MSM_FP_SPI_MOSI				GPIO(18)
#define MSM_FP_SPI_MISO				GPIO(19)
#define MSM_FP_SPI_CS0				GPIO(20)
#define MSM_FP_SPI_CLK				GPIO(21)
#define MSM_CPU_1WIRE_TX			GPIO(22)
#define MSM_CPU_1WIRE_RX			GPIO(23)
#define MSM_SR_I2C_SDA				GPIO(24)
#define MSM_SR_I2C_SCL				GPIO(25)
#define SR_I2C_SDA				GPIO(24)
#define SR_I2C_SCL				GPIO(25)
#define MSM_PWR_KEY_MSMz			GPIO(26)
#define MSM_AUD_CPU_RX_I2S_WS			GPIO(27)
#define MSM_AUD_CPU_RX_I2S_SCK			GPIO(28)
#define MSM_AUD_TFA_DO_A			GPIO(29)
#define MSM_AP2MDM_VDDMIN			GPIO(30)
#define MSM_NC_GPIO_31				GPIO(31)
#define MSM_AUD_CPU_RX_I2S_SD1			GPIO(32)
#define MSM_RAW_INTR1				GPIO(33)
#define RAW_INT1				GPIO(33)
#define MSM_TP_ATTz				GPIO(34)
#define MSM_MDM2AP_HSIC_READY			GPIO(35)
#define MSM_MDM2AP_ERR_FATAL			GPIO(36)
#define MSM_AP2MDM_ERR_FATAL			GPIO(37)
#define MSM_MHL_INT				GPIO(38)
#define MSM_AUD_CPU_MCLK			GPIO(39)
#define MSM_AUD_CPU_SB_CLK			jGPIO(40)
#define MSM_AUD_CPU_SB_DATA			GPIO(41)
#define MSM_AUD_WCD_INTR_OUT			GPIO(42)
#define MSM_NC_GPIO_43				GPIO(43)
#define MSM_NC_GPIO_44				GPIO(44)
#define MSM_NC_GPIO_45				GPIO(45)
#define MSM_NC_GPIO_46				GPIO(46)
#define MSM_MDM2AP_WAKEUP			GPIO(47)
#define MSM_AP2MDM_STATUS			GPIO(48)
#define MSM_MDM2AP_STATUS			GPIO(49)
#define MSM_NC_GPIO_50				GPIO(50)
#define MSM_MCAM_SPI_MOSI			GPIO(51)
#define MSM_MCAM_SPI_MISO			GPIO(52)
#define MSM_MCAM_SPI_CS0			GPIO(53)
#define MSM_MCAM_SPI_CLK			GPIO(54)
#define MCAM_FP_SPI_DO				GPIO(51)
#define MCAM_FP_SPI_DI				GPIO(52)
#define MCAM_SPI_CS0				GPIO(53)
#define MCAM_FP_SPI_CLK				GPIO(54)
#define MSM_FP_INT				GPIO(55)
#define MSM_NFC_IRQ				GPIO(56)
#define MSM_LCD_RSTz				GPIO(57)
#define MSM_WCN_PRIORITY			GPIO(58)
#define MSM_AP2MDM_PON_RESET_N			GPIO(59)
#define MSM_MDM_LTE_FRAME_SYNC			GPIO(60)
#define MSM_MDM_LTE_ACTIVE			GPIO(61)
#define MSM_PWR_MISTOUCH			GPIO(62)
#define MSM_WCSS_BT_SSBI			GPIO(63)
#define MSM_WCSS_WLAN_DATA_2			GPIO(64)
#define MSM_WCSS_WLAN_DATA_1			GPIO(65)
#define MSM_WCSS_WLAN_DATA_0			GPIO(66)
#define MSM_WCSS_WLAN_SET			GPIO(67)
#define MSM_WCSS_WLAN_CLK			GPIO(68)
#define MSM_AP2MDM_IPC3				GPIO(69)
#define MSM_HDMI_DDC_CLK			GPIO(70)
#define MSM_HDMI_DDC_DATA			GPIO(71)
#define MSM_HDMI_HPLG_DET			GPIO(72)
#define MSM_PM8921_APC_SEC_IRQ_N		GPIO(73)
#define MSM_PM8921_APC_USR_IRQ_N		GPIO(74)
#define MSM_PM8921_MDM_IRQ_N			GPIO(75)
#define MSM_PM8821_APC_SEC_IRQ_N		GPIO(76)
#define MSM_VOL_DOWNz				GPIO(77)
#define MSM_PS_HOLD_APQ				GPIO(78)
#define MSM_SSBI_PM8821				GPIO(79)
#define MSM_MDM2AP_VDDMIN			GPIO(80)
#define MSM_APQ2MDM_IPC1			GPIO(81)
#define MSM_UART_TX				GPIO(82)
#define MSM_UART_RX				GPIO(83)
#define MSM_AUD_I2C_SDA				GPIO(84)
#define MSM_AUD_I2C_SCL				GPIO(85)
#define MSM_NC_GPIO_86				GPIO(86)
#define MSM_RAW_INTR0				GPIO(87)
#define RAW_INT0				GPIO(87)
#define MSM_HSIC_STROBE				GPIO(88)
#define MSM_HSIC_DATA				GPIO(89)

#define PM_CAM_VCM_PD				PMGPIO(1)
#define CAM_VCM_PD				PMGPIO(1)
#define PM_EXT_BUCK_EN				PMGPIO(2)
#define PM_GYRO_INT				PMGPIO(3)
#define PM_CIR_LS_EN				PMGPIO(4)
#define PM_CAM2_ID				PMGPIO(5)
#define PM_COMPASS_AKM_INT			PMGPIO(6)
#define PM_USB1_HS_ID_GPIO			PMGPIO(7)
#define PM_V_FP_1V8_EN				PMGPIO(8)
#define PM_V_AUD_HSMIC_2V85_EN			PMGPIO(9)
#define PM_EXT_BUCK_VSEL			PMGPIO(10)
#define PM_V_FP_3V3_EN				PMGPIO(11)
#define PM_AUD_DMIC1_SEL			PMGPIO(12)
#define PM_AUD_DMIC2_SEL			PMGPIO(13)
#define PM_FP_RSTz				PMGPIO(14)
#define PM_MHL_USB_SW				PMGPIO(15)
#define PM_JAC_CHG_BAT_EN			PMGPIO(16)
#define PM_PROXIMITY_INTz			PMGPIO(17)
#define PM_TORCH_FLASHz				PMGPIO(18)
#define PM_FLASH_EN				PMGPIO(19)
#define PM_EARPHONE_DETz			PMGPIO(20)
#define PM_MHL_RSTz				PMGPIO(21)
#define PM_MAIN_CAM_ID				PMGPIO(22)
#define MAIN_CAM_ID				PMGPIO(22)
#define PM_G_INT				PMGPIO(23)
#define PM_AUD_RECEIVER_SEL			PMGPIO(24)
#define PM_MCAM_D1V2_EN				PMGPIO(25)
#define MCAM_D1V2_EN				PMGPIO(25)
#define PM_BAT_CHG_JAC_EN			PMGPIO(26)
#define PM_POGO_ID				PMGPIO(27)
#define PM_OVP_INT_DECT				PMGPIO(28)
#define PM_NFC_DL_MODE				PMGPIO(29)
#define PM_NFC_VEN				PMGPIO(30)
#define PM_V_CIR_3V_EN				PMGPIO(31)
#define PM_CHARGER_SUSP				PMGPIO(32)
#define PM_AUD_RECEIVER_EN			PMGPIO(33)
#define PM_AUD_WCD_RESET_N			PMGPIO(34)
#define PM_CIR_RSTz				PMGPIO(35)
#define PM_V_CAM_D1V2_EN			PMGPIO(36)
#define V_RAW_1V2_EN				PMGPIO(36)
#define PM_AUD_HP_EN				PMGPIO(37)
#define PM_SDC3_CDz				PMGPIO(38)
#define PM_SSBI_PMIC_FWD_CLK			PMGPIO(39)
#define PM_REGION_ID				PMGPIO(40)
#define PM_AUD_UART_OEz				PMGPIO(41)
#define PM_CAM_PWDN				PMGPIO(42)
#define CAM_PWDN				PMGPIO(42)
#define PM_TP_RSTz				PMGPIO(43)
#define PM_LCD_ID1				PMGPIO(44)

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_mpp)	(pm_mpp - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)

#define PM8821_MPP_BASE			(PM8921_MPP_BASE + PM8921_NR_MPPS)
#define PM8821_MPP_PM_TO_SYS(pm_mpp)	(pm_mpp - 1 + PM8821_MPP_BASE)
#define PM8821_IRQ_BASE			(PM8921_IRQ_BASE + PM8921_NR_IRQS)

#ifdef CONFIG_RESET_BY_CABLE_IN
#define AC_WDT_EN		GPIO(3)
#define AC_WDT_RST		GPIO(87)
#endif

#define PM2QSC_SOFT_RESET	PM8921_GPIO_PM_TO_SYS(26)
#define PM2QSC_PWR_EN		PM8921_GPIO_PM_TO_SYS(8)
#define PM2QSC_KEYPADPWR	PM8921_GPIO_PM_TO_SYS(16)

#define BL_HW_EN_MPP_8		PM8921_MPP_PM_TO_SYS(8)
#define LCM_N5V_EN_MPP_9	PM8921_MPP_PM_TO_SYS(9)
#define LCM_P5V_EN_MPP_10	PM8921_MPP_PM_TO_SYS(10)

#define TABLA_INTERRUPT_BASE (NR_MSM_IRQS + NR_GPIO_IRQS + NR_PM8921_IRQS)

extern struct pm8xxx_regulator_platform_data
	t6_pm8921_regulator_pdata[] __devinitdata;

extern int t6_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_3P3V		1
#define GPIO_VREG_ID_EXT_TS_SW		2
#define GPIO_VREG_ID_EXT_MPP8		3

extern struct gpio_regulator_platform_data
	t6_gpio_regulator_pdata[] __devinitdata;

extern struct rpm_regulator_platform_data
	t6_rpm_regulator_pdata __devinitdata;

extern struct regulator_init_data t6_saw_regulator_pdata_8921_s5;
extern struct regulator_init_data t6_saw_regulator_pdata_8921_s6;
extern struct regulator_init_data t6_saw_regulator_pdata_8821_s0;
extern struct regulator_init_data t6_saw_regulator_pdata_8821_s1;

struct mmc_platform_data;
int __init apq8064_add_sdcc(unsigned int controller,
		struct mmc_platform_data *plat);

void t6_init_mmc(void);
void t6_init_gpiomux(void);
void t6_init_pmic(void);

void t6_init_cam(void);

int t6_init_keypad(void);
int t6_wifi_init(void);

#define APQ_8064_GSBI1_QUP_I2C_BUS_ID 0
#define APQ_8064_GSBI2_QUP_I2C_BUS_ID 2
#define APQ_8064_GSBI3_QUP_I2C_BUS_ID 3
#define APQ_8064_GSBI4_QUP_I2C_BUS_ID 4
#define APQ_8064_GSBI7_QUP_I2C_BUS_ID 7

void t6_init_fb(void);
void t6_allocate_fb_region(void);
void t6_mdp_writeback(struct memtype_reserve *reserve_table);

void t6_init_gpu(void);
void t6_pm8xxx_gpio_mpp_init(void);

extern struct msm_rtb_platform_data apq8064_rtb_pdata;
extern struct msm_cache_dump_platform_data apq8064_cache_dump_pdata;

#ifdef CONFIG_RAWCHIPII
extern struct platform_device t6china_msm_rawchip_device;
#endif
void t6china_init_cam(void);
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
int hdmi_enable_5v(int on);
extern void hdmi_hpd_feature(int enable);
#endif
#endif
