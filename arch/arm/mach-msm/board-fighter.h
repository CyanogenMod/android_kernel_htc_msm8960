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

#include <mach/irqs.h>
#include <mach/rpm-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>

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

extern struct platform_device msm8960_device_ext_5v_vreg __devinitdata;
extern struct platform_device msm8960_device_ext_l2_vreg __devinitdata;
extern struct platform_device msm8960_device_rpm_regulator __devinitdata;

extern struct pm8xxx_regulator_platform_data
	msm_pm8921_regulator_pdata[] __devinitdata;

extern int msm_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_L2		1
#define GPIO_VREG_ID_EXT_3P3V           2

#define FIGHTER_LCD_TE				(0) /* MDP_VSYNC_GPIO */
#define FIGHTER_NC_GPIO_1			(1)
#define FIGHTER_NC_GPIO_2			(2)
#define FIGHTER_SIM_HOTSWAP			(3)
#define FIGHTER_CAM_MCLK1			(4)
#define FIGHTER_CAM_MCLK0			(5)
#define FIGHTER_NFC_DL_MODE			(6)
#define FIGHTER_TP_ATTz				(7)
#define FIGHTER_TP_RSTz				(8)
#define FIGHTER_CAM_PWDN			(9)
#define FIGHTER_RAFT_UP_EN_CPU		(10)
#define FIGHTER_RESOUTz_CONTROL		(11)
#define FIGHTER_NC_GPIO_12			(12)
#define FIGHTER_NC_GPIO_13			(13)
#define FIGHTER_NFC_I2C_SDA			(12)
#define FIGHTER_NFC_I2C_SCL			(13)
#define FIGHTER_NC_GPIO_14			(14)
#define FIGHTER_SIM_CD				(15)
#define FIGHTER_TP_I2C_SDA			(16)
#define FIGHTER_TP_I2C_SCL			(17)
#define FIGHTER_NC_GPIO_18			(18)
#define FIGHTER_USB_ID1				(19)
#define FIGHTER_CAM_I2C_SDA			(20)
#define FIGHTER_CAM_I2C_SCL			(21)
#define FIGHTER_NC_GPIO_22			(22)
#define FIGHTER_NC_GPIO_23			(23)
#define FIGHTER_NC_GPIO_24			(24)
#define FIGHTER_NC_GPIO_25			(25)
#define FIGHTER_FM_SSBI				(26)
#define FIGHTER_FM_DATA				(27)
#define FIGHTER_BT_STROBE			(28)
#define FIGHTER_BT_DATA				(29)
#define FIGHTER_UIM1_DATA_MSM		(30)
#define FIGHTER_UIM1_CLK_MSM		(31)
#define FIGHTER_TORCH_FLASHz		(32)
#define FIGHTER_DRIVER_EN			(33)
#define FIGHTER_DEBUG_UART_TX		(34)
#define FIGHTER_DEBUG_UART_RX		(35)
#define FIGHTER_AC_I2C_SDA			(36)
#define FIGHTER_AC_I2C_SCL			(37)
#define FIGHTER_MSM_SPI_DO			(38)
#define FIGHTER_NC_GPIO_39			(39)
#define FIGHTER_MSM_SPI_CS0			(40)
#define FIGHTER_MSM_SPI_CLK			(41)
#define FIGHTER_VOL_UPz				(42)
#define FIGHTER_VOL_DOWNz			(43)
#define FIGHTER_SENSOR_I2C_SDA		(44)
#define FIGHTER_SENSOR_I2C_SCL		(45)
#define FIGHTER_PWR_KEYz 			(46)
#define FIGHTER_MAIN_CAM_ID			(47)
#define FIGHTER_LCD_RSTz			(48)
#define FIGHTER_CAM_VCM_PD			(49)
#define FIGHTER_NFC_VEN				(50)
#define FIGHTER_NC_GPIO_51			(51)
#define FIGHTER_NC_GPIO_52			(52)
#define FIGHTER_NC_GPIO_53			(53)
#define FIGHTER_REGION_ID			(54)
#define FIGHTER_NC_GPIO_55			(55)
#define FIGHTER_V_3G_PA1_EN			(56)
#define FIGHTER_V_3G_PA0_EN			(57)
#define FIGHTER_APT0_MODE_SEL		(58)
#define FIGHTER_AUD_WCD_MCLK		(59)
#define FIGHTER_AUD_WCD_SB_CLK		(60)
#define FIGHTER_AUD_WCD_SB_DATA		(61)
#define FIGHTER_AUD_WCD_INTR_OUT	(62)
#define FIGHTER_NC_GPIO_63			(63)
#define FIGHTER_NC_GPIO_64			(64)
#define FIGHTER_NC_GPIO_65			(65)
#define FIGHTER_NC_GPIO_66			(66)
#define FIGHTER_GSENSOR_INT			(67)
#define FIGHTER_CAM2_RSTz			(68)
#define FIGHTER_NC_GPIO_69			(69)
#define FIGHTER_COMPASS_INT			(70)
#define FIGHTER_HS_TX_CPU			(71)
#define FIGHTER_HS_RX_CPU			(72)
#define FIGHTER_NC_GPIO_73			(73)
#define FIGHTER_NC_GPIO_74			(74)
#define FIGHTER_WC_INz				(75)
#define FIGHTER_CAM2_STANDBY		(76)
#define FIGHTER_NC_GPIO_77			(77)
#define FIGHTER_NC_GPIO_78			(78)
#define FIGHTER_V_CAM2_D1V8_EN		(79)
#define FIGHTER_NC_GPIO_80			(80)
#define FIGHTER_V_TP_3V3_EN			(81)
#define FIGHTER_NC_GPIO_82			(82)
#define FIGHTER_WCN_BT_SSBI			(83)
#define FIGHTER_WCN_CMD_DATA2		(84)
#define FIGHTER_WCN_CMD_DATA1		(85)
#define FIGHTER_WCN_CMD_DATA0		(86)
#define FIGHTER_WCN_CMD_SET			(87)
#define FIGHTER_WCN_CMD_CLK			(88)
#define FIGHTER_USBz_AUDIO_SW		(89)
#define FIGHTER_NC_GPIO_90			(90)
#define FIGHTER_NC_GPIO_91			(91)
#define FIGHTER_NC_GPIO_92			(92)
#define FIGHTER_HS_MIC_BIAS_EN			(93)
#define FIGHTER_MBAT_IN				(94)
#define FIGHTER_CAM_EXT_LDO			(95)
#define FIGHTER_NC_GPIO_96			(96)
#define FIGHTER_NC_GPIO_97			(97)
#define FIGHTER_RIVA_TX				(98)
#define FIGHTER_NC_GPIO_99			(99)
#define FIGHTER_NC_GPIO_100			(100)
#define FIGHTER_NC_GPIO_101			(101)
#define FIGHTER_NC_GPIO_102			(102)
#define FIGHTER_PM_SEC_INTz			(103)
#define FIGHTER_PM_USR_INTz			(104)
#define FIGHTER_PM_MDM_INTz			(105)
#define FIGHTER_NFC_IRQ				(106)
#define FIGHTER_AUDIOz_UART_SW		(107)
#define FIGHTER_PS_HOLD				(108)
#define FIGHTER_APT1_VCON			(109)
#define FIGHTER_BC1_SW_SEL1			(110)
#define FIGHTER_PRX_LB_SW_SEL		(111)
#define FIGHTER_ANT_SW_SEL4			(112)
#define FIGHTER_BC1_SW_SEL0			(113)
#define FIGHTER_BC0_SW_SEL1			(114)
#define FIGHTER_BC0_SW_SEL0			(115)
#define FIGHTER_ANT_SW_SEL3			(116)
#define FIGHTER_ANT_SW_SEL2			(117)
#define FIGHTER_ANT_SW_SEL1			(118)
#define FIGHTER_ANT_SW_SEL0			(119)
#define FIGHTER_PA1_R0				(120)
#define FIGHTER_PA1_R1				(121)
#define FIGHTER_PA0_R0				(122)
#define FIGHTER_PA0_R1				(123)
#define FIGHTER_RTR1_RF_ON			(124)
#define FIGHTER_RTR0_RF_ON			(125)
#define FIGHTER_RTR_RX_ON			(126)
#define FIGHTER_APT0_VCON			(127)
#define FIGHTER_RTR0_PA_ON8_CELL	(128)
#define FIGHTER_RTR0_PA_ON7_U700	(129)
#define FIGHTER_NC_GPIO_130			(130)
#define FIGHTER_RTR0_PA_ON5_PCS		(131)
#define FIGHTER_PA_ON4_MODE			(132)
#define FIGHTER_RTR1_PA_ON3_BC1_1X	(133)
#define FIGHTER_RTR1_PA_ON2_BC0_1X	(134)
#define FIGHTER_PA_ON1_GSMHB		(135)
#define FIGHTER_PA_ON0_GSMLB		(136)
#define FIGHTER_EXT_GPS_LNA_EN		(137)
#define FIGHTER_NC_GPIO_138			(138)
#define FIGHTER_G850_700_SEL		(139)
#define FIGHTER_RTR1_SSBI2			(140)
#define FIGHTER_RTR1_SSBI1			(141)
#define FIGHTER_RTR0_SSBI2			(142)
#define FIGHTER_RTR0_SSBI1			(143)
#define FIGHTER_RTR0_GP_CLK			(144)
#define FIGHTER_RTR0_GPRSSYNC		(145)
#define FIGHTER_RTR0_GPDATA2		(146)
#define FIGHTER_RTR0_GPDATA1		(147)
#define FIGHTER_RTR0_GPDATA0		(148)
#define FIGHTER_NC_GPIO_149			(149)
#define FIGHTER_NC_GPIO_150			(150)
#define FIGHTER_NC_GPIO_151			(151)

#define PMGPIO(x) (x)

#define FIGHTER_NC_PMGPIO_1			PMGPIO(1)
#define FIGHTER_NC_PMGPIO_2			PMGPIO(2)
#define FIGHTER_NC_PMGPIO_3			PMGPIO(3)
#define FIGHTER_NC_PMGPIO_4			PMGPIO(4)
#define FIGHTER_PM_SPI_CS0			PMGPIO(5)
#define FIGHTER_RAFT_uP_SPI_CS0		PMGPIO(6)
#define FIGHTER_HS_RX_PMIC_REMO			PMGPIO(7)
#define FIGHTER_HS_RX_PMIC_UART			PMGPIO(8)
#define FIGHTER_NC_PMGPIO_9			PMGPIO(9)
#define FIGHTER_NC_PMGPIO_10		PMGPIO(10)
#define FIGHTER_PM_SPI_CLK			PMGPIO(11)
#define FIGHTER_RAFT_uP_SPI_CLK		PMGPIO(12)
#define FIGHTER_PM_SPI_DO			PMGPIO(13)
#define FIGHTER_RAFT_uP_SPI_DO		PMGPIO(14)
#define FIGHTER_RAFT_UP_EN_CPU_PM	PMGPIO(15)
#define FIGHTER_HS_TX_PMIC_UART			PMGPIO(15)
#define FIGHTER_RAFT_UP_EN			PMGPIO(16)
#define FIGHTER_HS_TX_PMIC_REMO			PMGPIO(16)
#define FIGHTER_PROXIMITY_INTz		PMGPIO(17)
#define FIGHTER_AUD_AMP_EN			PMGPIO(18)
#define FIGHTER_AUD_HAC_SDz			PMGPIO(19)
#define FIGHTER_EARPHONE_DETz		PMGPIO(20)
#define FIGHTER_NC_PMGPIO_21		PMGPIO(21)
#define FIGHTER_NC_PMGPIO_22		PMGPIO(22)
#define FIGHTER_SDC3_CDz			PMGPIO(23)
#define FIGHTER_GREEN_LED			PMGPIO(24)
#define FIGHTER_AMBER_LED			PMGPIO(25)
#define FIGHTER_NC_PMGPIO_26		PMGPIO(26)
#define FIGHTER_UIM1_RST			PMGPIO(27)
#define FIGHTER_NC_PMGPIO_28		PMGPIO(28)
#define FIGHTER_SIM_CLK_MSM			PMGPIO(29)
#define FIGHTER_UIM1_CLK			PMGPIO(30)
#define FIGHTER_NC_PMGPIO_31		PMGPIO(31)
#define FIGHTER_NC_PMGPIO_32		PMGPIO(32)
#define FIGHTER_LCD_ID0				PMGPIO(33)
#define FIGHTER_AUD_CODEC_RSTz		PMGPIO(34)
#define FIGHTER_LCD_ID1				PMGPIO(35)
#define FIGHTER_NC_PMGPIO_36		PMGPIO(36)
#define FIGHTER_NC_PMGPIO_37		PMGPIO(37)
#define FIGHTER_NC_PMGPIO_38		PMGPIO(38)
#define FIGHTER_SSBI_PMIC_FWD_CLK	PMGPIO(39)
#define FIGHTER_LS_EN			PMGPIO(40)
#define FIGHTER_COVER_DETz			PMGPIO(41)
#define FIGHTER_AD_EN_MSM			PMGPIO(42)
#define FIGHTER_NC_PMGPIO_43		PMGPIO(43)
#define FIGHTER_NC_PMGPIO_44		PMGPIO(44)

extern struct regulator_init_data msm_saw_regulator_pdata_s5;
extern struct regulator_init_data msm_saw_regulator_pdata_s6;

extern struct rpm_regulator_platform_data msm_rpm_regulator_pdata __devinitdata;

int __init fighter_init_keypad(void);
int __init fighter_gpiomux_init(void);

#endif
