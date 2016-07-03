/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_FIGHTER_H
#define __ARCH_ARM_MACH_MSM_BOARD_FIGHTER_H

#include <linux/regulator/msm-gpio-regulator.h>
#include <mach/irqs.h>
#include <mach/rpm-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/i2c.h>
#include <mach/msm_memtypes.h>
#ifdef CONFIG_MSM_RTB
#include <mach/msm_rtb.h>
#endif
#ifdef CONFIG_MSM_CACHE_DUMP
#include <mach/msm_cache_dump.h>
#endif

/* Macros assume PMIC GPIOs and MPPs start at 1 */
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)

#define FIGHTER_LAYOUTS			{\
		{ { 0,  1, 0}, {-1,  0,  0}, {0, 0,  1} }, \
		{ { 0, -1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ {-1,  0, 0}, { 0, -1,  0}, {0, 0,  1} }, \
		{ {-1,  0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
					}

extern struct pm8xxx_regulator_platform_data
	msm_pm8921_regulator_pdata[] __devinitdata;

extern int msm_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_L2		1
#define GPIO_VREG_ID_EXT_3P3V		2
#define GPIO_VREG_ID_EXT_OTG_SW		3

extern struct regulator_init_data msm_saw_regulator_pdata_s5;
extern struct regulator_init_data msm_saw_regulator_pdata_s6;

extern struct rpm_regulator_platform_data fighter_rpm_regulator_pdata __devinitdata;

extern struct platform_device msm8960_device_ext_5v_vreg __devinitdata;
extern struct platform_device msm8960_device_ext_l2_vreg __devinitdata;
extern struct platform_device msm8960_device_ext_3p3v_vreg __devinitdata;

/* EVM */
#define FIGHTER_GPIO_LCD_TE			(0) /* MDP_VSYNC_GPIO */
#define FIGHTER_GPIO_SIM_HOTSWAP		(3)
#define FIGHTER_GPIO_RAFT_UP_EN_CPU		(10)
#define FIGHTER_GPIO_NFC_NC_12			(12)
#define FIGHTER_GPIO_NFC_NC_13			(13)
#define FIGHTER_GPIO_NC_GPIO_14			(14)
#define FIGHTER_GPIO_SIM_CD			(15)
#define FIGHTER_GPIO_TP_I2C_SDA			(16)
#define FIGHTER_GPIO_TP_I2C_SCL			(17)
#define FIGHTER_GPIO_CAM_I2C_SDA		(20)
#define FIGHTER_GPIO_CAM_I2C_SCL		(21)
#define FIGHTER_GPIO_AC_I2C_SDA			(36)
#define FIGHTER_GPIO_AC_I2C_SCL			(37)
#define FIGHTER_GPIO_SENSOR_I2C_SDA		(44)
#define FIGHTER_GPIO_SENSOR_I2C_SCL		(45)
#define FIGHTER_GPIO_MAIN_CAM_ID		(47)
#define FIGHTER_GPIO_NC_55			(55)
#define FIGHTER_GPIO_NC_69			(69)
#define FIGHTER_GPIO_MCAM_CLK_ON		(75)
#define FIGHTER_GPIO_CAM2_STANDBY		(76)
#define FIGHTER_GPIO_NC_80			(80)
#define FIGHTER_GPIO_NC_82			(82)
#define FIGHTER_GPIO_USBz_AUDIO_SW		(89)
#define FIGHTER_GPIO_NC_90			(90)
#define FIGHTER_GPIO_NC_92			(92)
#define FIGHTER_GPIO_NC_93			(93)
#define FIGHTER_GPIO_V_CAM_D1V2_EN		(95)
#define FIGHTER_GPIO_NC_96			(96)
#define FIGHTER_GPIO_NC_100			(100)
#define FIGHTER_GPIO_NC_101			(101)
#define FIGHTER_GPIO_NC_102			(102)
#define FIGHTER_GPIO_PM_MDM_INTz		(105)
#define FIGHTER_GPIO_AUDIOz_UART_SW		(107)
#define FIGHTER_GPIO_PRX_LB_SW_SEL		(111)
#define FIGHTER_GPIO_ANT_SW_SEL4		(112)
#define FIGHTER_GPIO_BC1_SW_SEL0		(113)
#define FIGHTER_GPIO_ANT_SW_SEL3		(116)
#define FIGHTER_GPIO_ANT_SW_SEL2		(117)
#define FIGHTER_GPIO_ANT_SW_SEL1		(118)
#define FIGHTER_GPIO_ANT_SW_SEL0		(119)
#define FIGHTER_GPIO_RTR0_PA_ON7_U700		(129)
#define FIGHTER_GPIO_PA_ON4_MODE		(132)
#define FIGHTER_GPIO_PA_ON1_GSMHB		(135)
#define FIGHTER_GPIO_RTR0_PA_ON0_CELL		(136)
#define FIGHTER_GPIO_G850_700_SEL		(139)
#define FIGHTER_GPIO_RTR0_GP_CLK		(144)
#define FIGHTER_GPIO_RTR0_GPRSSYNC		(145)
#define FIGHTER_GPIO_RTR0_GPDATA2		(146)
#define FIGHTER_GPIO_RTR0_GPDATA1		(147)
#define FIGHTER_GPIO_RTR0_GPDATA0		(148)
#define FIGHTER_GPIO_NC_150			(150)
#define FIGHTER_GPIO_NC_151			(151)

#define PMGPIO(x) (x)

#define FIGHTER_PMGPIO_PM_SPI_CS0		PMGPIO(5)
#define FIGHTER_PMGPIO_RAFT_uP_SPI_CS0		PMGPIO(6)
#define FIGHTER_PMGPIO_NC_8			PMGPIO(8)
#define FIGHTER_PMGPIO_PM_SPI_CLK		PMGPIO(11)
#define FIGHTER_PMGPIO_RAFT_uP_SPI_CLK		PMGPIO(12)
#define FIGHTER_PMGPIO_PM_SPI_DO		PMGPIO(13)
#define FIGHTER_PMGPIO_RAFT_uP_SPI_DO		PMGPIO(14)
#define FIGHTER_PMGPIO_RAFT_UP_EN_CPU_PM	PMGPIO(15)
#define FIGHTER_PMGPIO_RAFT_UP_EN		PMGPIO(16)
#define FIGHTER_PMGPIO_AUD_AMP_EN		PMGPIO(18)
#define FIGHTER_PMGPIO_SDC3_CDz			PMGPIO(23)
#define FIGHTER_PMGPIO_UIM1_RST			PMGPIO(27)
#define FIGHTER_PMGPIO_UIM1_CLK			PMGPIO(30)
#define FIGHTER_PMGPIO_COVER_DETz		PMGPIO(41)
#define FIGHTER_PMGPIO_AD_EN_MSM		PMGPIO(42)

/* XA */
#define FIGHTER_GPIO_LCD_TE			(0) /* MDP_VSYNC_GPIO */
#define FIGHTER_GPIO_NC_1			(1)
#define FIGHTER_GPIO_NC_2			(2)
#define FIGHTER_GPIO_HAPTIC_EN			(3)
#define FIGHTER_GPIO_CAM_MCLK1			(4)
#define FIGHTER_GPIO_CAM_MCLK0			(5)
#define FIGHTER_GPIO_NFC_DL_MODE		(6)
#define FIGHTER_GPIO_TP_ATTz			(7)
#define FIGHTER_GPIO_TP_RSTz			(8)
#define FIGHTER_GPIO_CAM_PWDN			(9)
#define FIGHTER_GPIO_NC_10			(10)
#define FIGHTER_GPIO_RESOUTz_CONTROL		(11)
#define FIGHTER_GPIO_NC_12			(12)
#define FIGHTER_GPIO_NC_13			(13)
#define FIGHTER_GPIO_AUD_1WIRE_DO		(14)
#define FIGHTER_GPIO_AUD_1WIRE_DI		(15)
#define FIGHTER_GPIO_TP_I2C_DAT			(16)
#define FIGHTER_GPIO_TP_I2C_CLK			(17)
#define FIGHTER_GPIO_NC_18			(18)
#define FIGHTER_GPIO_USB_ID1			(19)
#define FIGHTER_GPIO_CAM_I2C_DAT		(20)
#define FIGHTER_GPIO_CAM_I2C_CLK		(21)
#define FIGHTER_GPIO_NC_22			(22)
#define FIGHTER_GPIO_NC_23			(23)
#define FIGHTER_GPIO_NC_24			(24)
#define FIGHTER_GPIO_NC_25			(25)
#define FIGHTER_GPIO_FM_SSBI			(26)
#define FIGHTER_GPIO_FM_DATA			(27)
#define FIGHTER_GPIO_BT_STROBE			(28)
#define FIGHTER_GPIO_BT_DATA			(29)
#define FIGHTER_GPIO_UIM1_DATA_MSM		(30)
#define FIGHTER_GPIO_UIM1_CLK_MSM		(31)
#define FIGHTER_GPIO_TORCH_FLASHz		(32)
#define FIGHTER_GPIO_DRIVER_EN			(33)
#define FIGHTER_GPIO_DEBUG_UART_TX		(34)
#define FIGHTER_GPIO_DEBUG_UART_RX		(35)
#define FIGHTER_GPIO_MC_I2C_DAT			(36)
#define FIGHTER_GPIO_MC_I2C_CLK			(37)
#define FIGHTER_GPIO_MSM_SPI_DO			(38)
#define FIGHTER_GPIO_NC_GPIO_39			(39)
#define FIGHTER_GPIO_MSM_SPI_CSO		(40)
#define FIGHTER_GPIO_MSM_SPI_CLK		(41)
#define FIGHTER_GPIO_VOL_UPz			(42)
#define FIGHTER_GPIO_VOL_DOWNz			(43)
#define FIGHTER_GPIO_SR_I2C_DAT			(44)
#define FIGHTER_GPIO_SR_I2C_CLK			(45)
#define FIGHTER_GPIO_PWR_KEYz 			(46)
#define FIGHTER_GPIO_CAM_ID			(47)
#define FIGHTER_GPIO_LCD_RSTz			(48)
#define FIGHTER_GPIO_CAM_VCM_PD			(49)
#define FIGHTER_GPIO_NFC_VEN			(50)
#define FIGHTER_GPIO_RAW_RSTN			(51)
#define FIGHTER_GPIO_RAW_INTR0			(52)
#define FIGHTER_GPIO_NC_53			(53)
#define FIGHTER_GPIO_REGION_ID			(54)
#define FIGHTER_GPIO_TAM_DET_EN			(55)
#define FIGHTER_GPIO_V_3G_PA1_EN		(56)
#define FIGHTER_GPIO_V_3G_PA0_EN		(57)
#define FIGHTER_GPIO_NC_58			(58)
#define FIGHTER_GPIO_AUD_WCD_MCLK		(59)
#define FIGHTER_GPIO_AUD_WCD_SB_CLK		(60)
#define FIGHTER_GPIO_AUD_WCD_SB_DATA		(61)
#define FIGHTER_GPIO_AUD_WCD_INTR_OUT		(62)
#define FIGHTER_GPIO_NC_63			(63)
#define FIGHTER_GPIO_NC_64			(64)
#define FIGHTER_GPIO_RAW_INTR1			(65)
#define FIGHTER_GPIO_NC_66			(66)
#define FIGHTER_GPIO_GSENSOR_INT		(67)
#define FIGHTER_GPIO_CAM2_RSTz			(68)
#define FIGHTER_GPIO_GYRO_INT			(69)
#define FIGHTER_GPIO_COMPASS_INT		(70)
#define FIGHTER_GPIO_MCAM_SPI_DO		(71)
#define FIGHTER_GPIO_MCAM_SPI_DI		(72)
#define FIGHTER_GPIO_AUD_UART_TX		(71)
#define FIGHTER_GPIO_AUD_UART_RX		(72)
#define FIGHTER_GPIO_MCAM_SPI_CS0		(73)
#define FIGHTER_GPIO_MCAM_SPI_CLK		(74)
#define FIGHTER_GPIO_NC_75			(75)
#define FIGHTER_GPIO_NC_76			(76)
#define FIGHTER_GPIO_V_HAPTIC_3V3_EN		(77)
#define FIGHTER_GPIO_MHL_INT			(78)
#define FIGHTER_GPIO_V_CAM2_D1V8_EN		(79)
#define FIGHTER_GPIO_MHL_RSTz			(80)
#define FIGHTER_GPIO_V_TP_3V3_EN		(81)
#define FIGHTER_GPIO_MHL_USBz_SEL		(82)
#define FIGHTER_GPIO_WCN_BT_SSBI		(83)
#define FIGHTER_GPIO_WCN_CMD_DATA2		(84)
#define FIGHTER_GPIO_WCN_CMD_DATA1		(85)
#define FIGHTER_GPIO_WCN_CMD_DATA0		(86)
#define FIGHTER_GPIO_WCN_CMD_SET		(87)
#define FIGHTER_GPIO_WCN_CMD_CLK		(88)
#define FIGHTER_GPIO_MHL_USB_ENz		(89)
#define FIGHTER_GPIO_CAM_STEP_1			(90)
#define FIGHTER_GPIO_NC_91			(91)
#define FIGHTER_GPIO_CAM_STEP_2			(92)
#define FIGHTER_GPIO_V_HSMIC_2v85_EN		(93)
#define FIGHTER_GPIO_MBAT_IN			(94)
#define FIGHTER_GPIO_CAM_EXT_LDO		(95)
#define FIGHTER_GPIO_V_BOOST_5V_EN		(96)
#define FIGHTER_GPIO_NC_97			(97)
#define FIGHTER_GPIO_RIVA_TX			(98)
#define FIGHTER_GPIO_NC_99			(99)
#define FIGHTER_GPIO_HDMI_DDC_CLK		(100)
#define FIGHTER_GPIO_HDMI_DDC_DATA		(101)
#define FIGHTER_GPIO_HDMI_HPD			(102)
#define FIGHTER_GPIO_PM_SEC_INTz		(103)
#define FIGHTER_GPIO_PM_USR_INTz		(104)
#define FIGHTER_GPIO_PM_MSM_INTz		(105)
#define FIGHTER_GPIO_NFC_IRQ			(106)
#define FIGHTER_GPIO_PS_HOLD			(108)
#define FIGHTER_GPIO_APT1_VCON			(109)
#define FIGHTER_GPIO_BC1_SW_SEL1		(110)
#define FIGHTER_GPIO_DRX_1X_EV_SEL		(111)
#define FIGHTER_GPIO_BOOT_CONFIG_6		(112)
#define FIGHTER_GPIO_NC_113			(113)
#define FIGHTER_GPIO_BC0_SW_SEL1		(114)
#define FIGHTER_GPIO_BC0_SW_SEL0		(115)
#define FIGHTER_GPIO_BOOT_CONFIG2		(116)
#define FIGHTER_GPIO_BOOT_CONFIG1		(117)
#define FIGHTER_GPIO_BOOT_CONFIG0		(118)
#define FIGHTER_GPIO_NC_119			(119)
#define FIGHTER_GPIO_PA1_R0			(120)
#define FIGHTER_GPIO_PA1_R1			(121)
#define FIGHTER_GPIO_PA0_R0			(122)
#define FIGHTER_GPIO_PA0_R1			(123)
#define FIGHTER_GPIO_RTR1_RF_ON			(124)
#define FIGHTER_GPIO_RTR0_RF_ON			(125)
#define FIGHTER_GPIO_RTR_RX_ON			(126)
#define FIGHTER_GPIO_APT0_VCON			(127)
#define FIGHTER_GPIO_NC_128			(128)
#define FIGHTER_GPIO_NC_129			(129)
#define FIGHTER_GPIO_NC_130			(130)
#define FIGHTER_GPIO_RTR0_PA_ON5_PCS		(131)
#define FIGHTER_GPIO_NC_132			(132)
#define FIGHTER_GPIO_RTR1_PA_ON3_BC1_1X		(133)
#define FIGHTER_GPIO_RTR1_PA_ON2_BC0_1X		(134)
#define FIGHTER_GPIO_NC_135			(135)
#define FIGHTER_GPIO_PTR0_PA_ON0_CELL		(136)
#define FIGHTER_GPIO_EXT_GPS_LNA_EN		(137)
#define FIGHTER_GPIO_NC_138			(138)
#define FIGHTER_GPIO_NC_139			(139)
#define FIGHTER_GPIO_RTR1_SSBI2			(140)
#define FIGHTER_GPIO_RTR1_SSBI1			(141)
#define FIGHTER_GPIO_RTR0_SSBI2			(142)
#define FIGHTER_GPIO_RTR0_SSBI1			(143)
#define FIGHTER_GPIO_NC_144			(144)
#define FIGHTER_GPIO_NC_145			(145)
#define FIGHTER_GPIO_NC_146			(146)
#define FIGHTER_GPIO_NC_147			(147)
#define FIGHTER_GPIO_NC_148			(148)
#define FIGHTER_GPIO_NC_149			(149)
#define FIGHTER_GPIO_HSIC_STROBE		(150)
#define FIGHTER_GPIO_HSIC_DATA			(151)

#define FIGHTER_PMGPIO_HDMI_DDC_CLK		PMGPIO(1)
#define FIGHTER_PMGPIO_HDMI_DDC_CLK_1		PMGPIO(2)
#define FIGHTER_PMGPIO_NC_3			PMGPIO(3)
#define FIGHTER_PMGPIO_NC_4			PMGPIO(4)
#define FIGHTER_PMGPIO_MSM_SPI_CS0		PMGPIO(5)
#define FIGHTER_PMGPIO_HVDAC_SPI_CS0		PMGPIO(6)
#define FIGHTER_PMGPIO_AUD_REMO_PRESz		PMGPIO(7)
#define FIGHTER_PMGPIO_AUD_1WIRE_DI		PMGPIO(8)
#define FIGHTER_PMGPIO_NC_9			PMGPIO(9)
#define FIGHTER_PMGPIO_NC_10			PMGPIO(10)
#define FIGHTER_PMGPIO_MSM_SPI_CLK		PMGPIO(11)
#define FIGHTER_PMGPIO_HVDAC_SPI_CLK		PMGPIO(12)
#define FIGHTER_PMGPIO_MSM_SPI_DO		PMGPIO(13)
#define FIGHTER_PMGPIO_HVDAC_SPI_DO		PMGPIO(14)
#define FIGHTER_PMGPIO_AUD_1WIRE_DO		PMGPIO(15)
#define FIGHTER_PMGPIO_AUD_1WIRE_O		PMGPIO(16)
#define FIGHTER_PMGPIO_PROXIMITY_INTz		PMGPIO(17)
#define FIGHTER_PMGPIO_AUD_SPK_SDz		PMGPIO(18)
#define FIGHTER_PMGPIO_NC_19			PMGPIO(19)
#define FIGHTER_PMGPIO_EARPHONE_DETz		PMGPIO(20)
#define FIGHTER_PMGPIO_NC_21			PMGPIO(21)
#define FIGHTER_PMGPIO_NC_22			PMGPIO(22)
#define FIGHTER_PMGPIO_SD_CDETz			PMGPIO(23)
#define FIGHTER_PMGPIO_GREEN_LED		PMGPIO(24)
#define FIGHTER_PMGPIO_AMBER_LED		PMGPIO(25)
#define FIGHTER_PMGPIO_HAPTIC_PWM		PMGPIO(26)
#define FIGHTER_PMGPIO_UIM_RST			PMGPIO(27)
#define FIGHTER_PMGPIO_NC_28			PMGPIO(28)
#define FIGHTER_PMGPIO_SIM_CLK_MSM		PMGPIO(29)
#define FIGHTER_PMGPIO_UIM_CLK			PMGPIO(30)
#define FIGHTER_PMGPIO_NC_31			PMGPIO(31)
#define FIGHTER_PMGPIO_NC_32			PMGPIO(32)
#define FIGHTER_PMGPIO_LCD_ID0			PMGPIO(33)
#define FIGHTER_PMGPIO_AUD_CODEC_RSTz		PMGPIO(34)
#define FIGHTER_PMGPIO_LCD_ID1			PMGPIO(35)
#define FIGHTER_PMGPIO_NC_36			PMGPIO(36)
#define FIGHTER_PMGPIO_NC_P37			PMGPIO(37)
#define FIGHTER_PMGPIO_NC_38			PMGPIO(38)
#define FIGHTER_PMGPIO_SSBI_PMIC_FWD_CLK	PMGPIO(39)
#define FIGHTER_PMGPIO_NC_40			PMGPIO(40)
#define FIGHTER_PMGPIO_NC_41			PMGPIO(41)
#define FIGHTER_PMGPIO_NC_42			PMGPIO(42)
#define FIGHTER_PMGPIO_RAW_1V2_EN		PMGPIO(43)
#define FIGHTER_PMGPIO_RAW_1V8_EN		PMGPIO(44)

/* XB */
#define FIGHTER_GPIO_V_CAM2_D1V2_EN		(79)

#define FIGHTER_PMGPIO_NC_1			PMGPIO(1)
#define FIGHTER_PMGPIO_NC_2			PMGPIO(2)

extern struct msm_camera_board_info fighter_camera_board_info;

void __init fighter_init_camera(void);
void fighter_init_fb(void);
void __init fighter_init_pmic(void);
void fighter_init_mmc(void);
int __init fighter_gpiomux_init(void);
void __init msm8960_allocate_fb_region(void);
void __init fighter_pm8921_gpio_mpp_init(void);
void msm8960_mdp_writeback(struct memtype_reserve *reserve_table);
int __init fighter_init_keypad(void);

#ifdef CONFIG_I2C
#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4
#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3
#define MSM_8960_GSBI2_QUP_I2C_BUS_ID 2
#define MSM_8960_GSBI8_QUP_I2C_BUS_ID 8
#define MSM_8960_GSBI12_QUP_I2C_BUS_ID 12
#endif

#ifdef CONFIG_MSM_RTB
extern struct msm_rtb_platform_data msm8960_rtb_pdata;
#endif
#ifdef CONFIG_MSM_CACHE_DUMP
extern struct msm_cache_dump_platform_data msm8960_cache_dump_pdata;
#endif

#endif
