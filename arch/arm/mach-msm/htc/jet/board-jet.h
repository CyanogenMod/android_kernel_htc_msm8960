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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_JET_H
#define __ARCH_ARM_MACH_MSM_BOARD_JET_H

#include <mach/irqs.h>
#include <mach/rpm-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
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

#define JET_LAYOUTS			{\
		{ { 0,  1, 0}, {-1,  0,  0}, {0, 0,  1} }, \
		{ { 0, -1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ {-1,  0, 0}, { 0, -1,  0}, {0, 0,  1} }, \
		{ {-1,  0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
					}

extern struct regulator_init_data msm_saw_regulator_pdata_s5;
extern struct regulator_init_data msm_saw_regulator_pdata_s6;

extern struct rpm_regulator_platform_data jet_rpm_regulator_pdata __devinitdata;

extern struct platform_device msm8960_device_ext_5v_vreg __devinitdata;
extern struct platform_device msm8960_device_ext_l2_vreg __devinitdata;
extern struct pm8xxx_regulator_platform_data
	msm_pm8921_regulator_pdata[] __devinitdata;

extern int msm_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_L2		1
#define GPIO_VREG_ID_EXT_3P3V           2
#define GPIO_VREG_ID_EXT_OTG_SW         3

/* EVM */
#define JET_GPIO_SIM_HOTSWAP			(3)
#define JET_GPIO_RAFT_UP_EN_CPU			(10)
#define JET_GPIO_NFC_I2C_SDA			(12)
#define JET_GPIO_NFC_I2C_SCL			(13)
#define JET_GPIO_NC_GPIO_14			(14)
#define JET_GPIO_SIM_CD				(15)
#define JET_GPIO_TP_I2C_DAT			(16)
#define JET_GPIO_TP_I2C_CLK			(17)
#define JET_GPIO_CAM_I2C_DAT			(20)
#define JET_GPIO_CAM_I2C_CLK			(21)
#define JET_GPIO_AC_I2C_SDA			(36)
#define JET_GPIO_AC_I2C_SCL			(37)
#define JET_GPIO_SENSOR_I2C_SDA			(44)
#define JET_GPIO_SENSOR_I2C_SCL			(45)
#define JET_GPIO_MAIN_CAM_ID			(47)
#define JET_GPIO_NC_GPIO_55			(55)
#define JET_GPIO_NC_GPIO_69			(69)
#define JET_GPIO_MCAM_CLK_ON			(75)
#define JET_GPIO_CAM2_STANDBY			(76)
#define JET_GPIO_NC_GPIO_80			(80)
#define JET_GPIO_NC_GPIO_82			(82)
#define JET_GPIO_USBz_AUDIO_SW			(89)
#define JET_GPIO_NC_GPIO_90			(90)
#define JET_GPIO_NC_GPIO_92			(92)
#define JET_GPIO_NC_GPIO_93			(93)
#define JET_GPIO_V_CAM_D1V2_EN			(95)
#define JET_GPIO_NC_GPIO_96			(96)
#define JET_GPIO_NC_GPIO_100			(100)
#define JET_GPIO_NC_GPIO_101			(101)
#define JET_GPIO_NC_GPIO_102			(102)
#define JET_GPIO_PM_MDM_INTz			(105)
#define JET_GPIO_AUDIOz_UART_SW			(107)
#define JET_GPIO_PRX_LB_SW_SEL			(111)
#define JET_GPIO_ANT_SW_SEL4			(112)
#define JET_GPIO_BC1_SW_SEL0			(113)
#define JET_GPIO_ANT_SW_SEL3			(116)
#define JET_GPIO_ANT_SW_SEL2			(117)
#define JET_GPIO_ANT_SW_SEL1			(118)
#define JET_GPIO_ANT_SW_SEL0			(119)
#define JET_GPIO_RTR0_PA_ON7_U700		(129)
#define JET_GPIO_PA_ON4_MODE			(132)
#define JET_GPIO_PA_ON1_GSMHB			(135)
#define JET_GPIO_RTR0_PA_ON0_CELL		(136)
#define JET_GPIO_G850_700_SEL			(139)
#define JET_GPIO_RTR0_GP_CLK			(144)
#define JET_GPIO_RTR0_GPRSSYNC			(145)
#define JET_GPIO_RTR0_GPDATA2			(146)
#define JET_GPIO_RTR0_GPDATA1			(147)
#define JET_GPIO_RTR0_GPDATA0			(148)
#define JET_GPIO_NC_GPIO_150			(150)
#define JET_GPIO_NC_GPIO_151			(151)

#define PMGPIO(x) (x)

#define JET_GPIO_PM_SPI_CS0			PMGPIO(5)
#define JET_GPIO_RAFT_uP_SPI_CS0		PMGPIO(6)
#define JET_GPIO_NC_PMGPIO_8			PMGPIO(8)
#define JET_GPIO_PM_SPI_CLK			PMGPIO(11)
#define JET_GPIO_RAFT_uP_SPI_CLK		PMGPIO(12)
#define JET_GPIO_PM_SPI_DO			PMGPIO(13)
#define JET_GPIO_RAFT_uP_SPI_DO			PMGPIO(14)
#define JET_GPIO_RAFT_UP_EN_CPU_PM		PMGPIO(15)
#define JET_GPIO_RAFT_UP_EN			PMGPIO(16)
#define JET_GPIO_AUD_AMP_EN			PMGPIO(18)
#define JET_GPIO_SDC3_CDz			PMGPIO(23)
#define JET_GPIO_UIM1_RST			PMGPIO(27)
#define JET_GPIO_UIM1_CLK			PMGPIO(30)
#define JET_GPIO_COVER_DETz			PMGPIO(41)
#define JET_GPIO_AD_EN_MSM			PMGPIO(42)


/* XA */
#define JET_GPIO_LCD_TE				(0) /* MDP_VSYNC_GPIO */
#define JET_GPIO_NC_GPIO_1			(1)
#define JET_GPIO_NC_GPIO_2			(2)
#define JET_GPIO_HAPTIC_EN			(3)
#define JET_GPIO_CAM_MCLK1			(4)
#define JET_GPIO_CAM_MCLK0			(5)
#define JET_GPIO_NFC_DL_MODE			(6)
#define JET_GPIO_TP_ATTz			(7)
#define JET_GPIO_TP_RSTz			(8)
#define JET_GPIO_NC_GPIO_9			(9)
#define JET_GPIO_SD_CDETz			(10)
#define JET_GPIO_RESOUTz_CONTROL		(11)
#define JET_GPIO_NC_GPIO_12			(12)
#define JET_GPIO_NC_GPIO_13			(13)
#define JET_GPIO_AUD_1WIRE_DO			(14)
#define JET_GPIO_AUD_1WIRE_DI			(15)
#define JET_GPIO_TP_I2C_DAT			(16)
#define JET_GPIO_TP_I2C_CLK			(17)
#define JET_GPIO_NC_GPIO_18			(18)
#define JET_GPIO_USB_ID1			(19)
#define JET_GPIO_CAM_I2C_DAT			(20)
#define JET_GPIO_CAM_I2C_CLK			(21)
#define JET_GPIO_NC_GPIO_22			(22)
#define JET_GPIO_NC_GPIO_23			(23)
#define JET_GPIO_NC_GPIO_24			(24)
#define JET_GPIO_NC_GPIO_25			(25)
#define JET_GPIO_FM_SSBI			(26)
#define JET_GPIO_FM_DATA			(27)
#define JET_GPIO_BT_STROBE			(28)
#define JET_GPIO_BT_DATA			(29)
#define JET_GPIO_UIM1_DATA_MSM			(30)
#define JET_GPIO_UIM1_CLK_MSM			(31)
#define JET_GPIO_TORCH_FLASHz			(32)
#define JET_GPIO_DRIVER_EN			(33)
#define JET_GPIO_DEBUG_UART_TX			(34)
#define JET_GPIO_DEBUG_UART_RX			(35)
#define JET_GPIO_MC_I2C_DAT			(36)
#define JET_GPIO_MC_I2C_CLK			(37)
#define JET_GPIO_MSM_SPI_DO			(38)
#define JET_GPIO_NC_GPIO_39			(39)
#define JET_GPIO_MSM_SPI_CSO			(40)
#define JET_GPIO_MSM_SPI_CLK			(41)
#define JET_GPIO_VOL_UPz			(42)
#define JET_GPIO_VOL_DOWNz			(43)
#define JET_GPIO_SR_I2C_DAT			(44)
#define JET_GPIO_SR_I2C_CLK			(45)
#define JET_GPIO_PWR_KEYz			(46)
#define JET_GPIO_CAM_ID				(47)
#define JET_GPIO_LCD_RSTz			(48)
#define JET_GPIO_CAM_VCM_PD			(49)
#define JET_GPIO_NFC_VEN			(50)
#define JET_GPIO_RAW_RSTN			(51)
#define JET_GPIO_RAW_INTR0			(52)
#define JET_GPIO_NC_GPIO_53			(53)
#define JET_GPIO_REGION_ID			(54)
#define JET_GPIO_TAM_DET_EN			(55)
#define JET_GPIO_V_3G_PA1_EN			(56)
#define JET_GPIO_V_3G_PA0_EN			(57)
#define JET_GPIO_NC_GPIO_58			(58)
#define JET_GPIO_AUD_WCD_MCLK			(59)
#define JET_GPIO_AUD_WCD_SB_CLK			(60)
#define JET_GPIO_AUD_WCD_SB_DATA		(61)
#define JET_GPIO_AUD_WCD_INTR_OUT		(62)
#define JET_GPIO_NC_GPIO_63			(63)
#define JET_GPIO_NC_GPIO_64			(64)
#define JET_GPIO_RAW_INTR1			(65)
#define JET_GPIO_NC_GPIO_66			(66)
#define JET_GPIO_GSENSOR_INT			(67)
#define JET_GPIO_CAM2_RSTz			(68)
#define JET_GPIO_GYRO_INT			(69)
#define JET_GPIO_COMPASS_INT			(70)
#define JET_GPIO_MCAM_SPI_DO			(71)
#define JET_GPIO_MCAM_SPI_DI			(72)
#define JET_GPIO_MCAM_SPI_CS0			(73)
#define JET_GPIO_MCAM_SPI_CLK			(74)
#define JET_GPIO_NC_GPIO_75			(75)
#define JET_GPIO_NC_GPIO_76			(76)
#define JET_GPIO_V_HAPTIC_3V3_EN		(77)
#define JET_GPIO_MHL_INT			(78)
#define JET_GPIO_V_CAM2_D1V8_EN			(79)
#define JET_GPIO_MHL_RSTz			(80)
#define JET_GPIO_V_TP_3V3_EN			(81)
#define JET_GPIO_MHL_USBz_SEL			(82)
#define JET_GPIO_WCN_BT_SSBI			(83)
#define JET_GPIO_WCN_CMD_DATA2			(84)
#define JET_GPIO_WCN_CMD_DATA1			(85)
#define JET_GPIO_WCN_CMD_DATA0			(86)
#define JET_GPIO_WCN_CMD_SET			(87)
#define JET_GPIO_WCN_CMD_CLK			(88)
#define JET_GPIO_MHL_USB_ENz			(89)
#define JET_GPIO_CAM_STEP_1			(90)
#define JET_GPIO_NC_GPIO_91			(91)
#define JET_GPIO_CAM_STEP_2			(92)
#define JET_GPIO_V_HSMIC_2v85_EN		(93)
#define JET_GPIO_MBAT_IN			(94)
#define JET_GPIO_V_CAM_D1V2_EN			(95)
#define JET_GPIO_V_BOOST_5V_EN			(96)
#define JET_GPIO_NC_GPIO_97			(97)
#define JET_GPIO_RIVA_TX			(98)
#define JET_GPIO_NC_GPIO_99			(99)
#define JET_GPIO_HDMI_DDC_CLK			(100)
#define JET_GPIO_HDMI_DDC_DATA			(101)
#define JET_GPIO_HDMI_HPD			(102)
#define JET_GPIO_PM_SEC_INTz			(103)
#define JET_GPIO_PM_USR_INTz			(104)
#define JET_GPIO_PM_MSM_INTz			(105)
#define JET_GPIO_NFC_IRQ			(106)
#define JET_GPIO_CAM_PWDN			(107)
#define JET_GPIO_PS_HOLD			(108)
#define JET_GPIO_APT1_VCON			(109)
#define JET_GPIO_BC1_SW_SEL1			(110)
#define JET_GPIO_DRX_1X_EV_SEL			(111)
#define JET_GPIO_BOOT_CONFIG_6			(112)
#define JET_GPIO_NC_GPIO_113			(113)
#define JET_GPIO_BC0_SW_SEL1			(114)
#define JET_GPIO_BC0_SW_SEL0			(115)
#define JET_GPIO_BOOT_CONFIG2			(116)
#define JET_GPIO_BOOT_CONFIG1			(117)
#define JET_GPIO_BOOT_CONFIG0			(118)
#define JET_GPIO_NC_GPIO_119			(119)
#define JET_GPIO_PA1_R0				(120)
#define JET_GPIO_PA1_R1				(121)
#define JET_GPIO_PA0_R0				(122)
#define JET_GPIO_PA0_R1				(123)
#define JET_GPIO_RTR1_RF_ON			(124)
#define JET_GPIO_RTR0_RF_ON			(125)
#define JET_GPIO_RTR_RX_ON			(126)
#define JET_GPIO_APT0_VCON			(127)
#define JET_GPIO_NC_GPIO_128			(128)
#define JET_GPIO_NC_GPIO_129			(129)
#define JET_GPIO_NC_GPIO_130			(130)
#define JET_GPIO_RTR0_PA_ON5_PCS		(131)
#define JET_GPIO_NC_GPIO_132			(132)
#define JET_GPIO_RTR1_PA_ON3_BC1_1X		(133)
#define JET_GPIO_RTR1_PA_ON2_BC0_1X		(134)
#define JET_GPIO_NC_GPIO_135			(135)
#define JET_GPIO_PTR0_PA_ON0_CELL		(136)
#define JET_GPIO_EXT_GPS_LNA_EN			(137)
#define JET_GPIO_NC_GPIO_138			(138)
#define JET_GPIO_NC_GPIO_139			(139)
#define JET_GPIO_RTR1_SSBI2			(140)
#define JET_GPIO_RTR1_SSBI1			(141)
#define JET_GPIO_RTR0_SSBI2			(142)
#define JET_GPIO_RTR0_SSBI1			(143)
#define JET_GPIO_NC_GPIO_144			(144)
#define JET_GPIO_NC_GPIO_145			(145)
#define JET_GPIO_NC_GPIO_146			(146)
#define JET_GPIO_NC_GPIO_147			(147)
#define JET_GPIO_NC_GPIO_148			(148)
#define JET_GPIO_NC_GPIO_149			(149)
#define JET_GPIO_HSIC_STROBE			(150)
#define JET_GPIO_HSIC_DATA			(151)

#define JET_GPIO_PM_HDMI_DDC_CLK	PMGPIO(1)
#define JET_GPIO_HDMI_DDC_CLK_1		PMGPIO(2)
#define JET_GPIO_NC_PMGPIO_3		PMGPIO(3)
#define JET_GPIO_NC_PMGPIO_4		PMGPIO(4)
#define JET_GPIO_MSM_SPI_CS0		PMGPIO(5)
#define JET_GPIO_HVDAC_SPI_CS0		PMGPIO(6)
#define JET_GPIO_PM_AUD_1WIRE_DO	PMGPIO(7)
#define JET_GPIO_PM_AUD_1WIRE_O		PMGPIO(8)
#define JET_GPIO_NC_PMGPIO_9		PMGPIO(9)
#define JET_GPIO_NC_PMGPIO_10		PMGPIO(10)
#define JET_GPIO_PM_MSM_SPI_CLK		PMGPIO(11)
#define JET_GPIO_HVDAC_SPI_CLK		PMGPIO(12)
#define JET_GPIO_PM_MSM_SPI_DO		PMGPIO(13)
#define JET_GPIO_HVDAC_SPI_DO		PMGPIO(14)
#define JET_GPIO_PM_AUD_REMO_PRESz	PMGPIO(15)
#define JET_GPIO_PM_AUD_1WIRE_DI	PMGPIO(16)
#define JET_GPIO_PROXIMITY_INTz		PMGPIO(17)
#define JET_GPIO_AUD_SPK_SDz		PMGPIO(18)
#define JET_GPIO_NC_PMGPIO_19		PMGPIO(19)
#define JET_GPIO_EARPHONE_DETz		PMGPIO(20)
#define JET_GPIO_NC_PMGPIO_21		PMGPIO(21)
#define JET_GPIO_NC_PMGPIO_22		PMGPIO(22)
#define JET_GPIO_NC_PMGPIO_23		PMGPIO(23)
#define JET_GPIO_NC_PMGPIO_24		PMGPIO(24)
#define JET_GPIO_NC_PMGPIO_25		PMGPIO(25)
#define JET_GPIO_HAPTIC_PWM		PMGPIO(26)
#define JET_GPIO_UIM_RST		PMGPIO(27)
#define JET_GPIO_NC_PMGPIO_28		PMGPIO(28)
#define JET_GPIO_SIM_CLK_MSM		PMGPIO(29)
#define JET_GPIO_UIM_CLK		PMGPIO(30)
#define JET_GPIO_NC_PMGPIO_31		PMGPIO(31)
#define JET_GPIO_NC_PMGPIO_32		PMGPIO(32)
#define JET_GPIO_LCD_ID0		PMGPIO(33)
#define JET_GPIO_AUD_CODEC_RSTz		PMGPIO(34)
#define JET_GPIO_LCD_ID1		PMGPIO(35)
#define JET_GPIO_NC_PMGPIO_36		PMGPIO(36)
#define JET_GPIO_NC_PMGPIO_37		PMGPIO(37)
#define JET_GPIO_NC_PMGPIO_38		PMGPIO(38)
#define JET_GPIO_SSBI_PMIC_FWD_CLK	PMGPIO(39)
#define JET_GPIO_NC_PMGPIO_40		PMGPIO(40)
#define JET_GPIO_NC_PMGPIO_41		PMGPIO(41)
#define JET_GPIO_NC_PMGPIO_42		PMGPIO(42)
#define JET_GPIO_RAW_1V2_EN		PMGPIO(43)
#define JET_GPIO_RAW_1V8_EN		PMGPIO(44)

/* XB GPIO */
#define JET_GPIO_V_CAM2_D1V2_EN		(79)

#define JET_GPIO_NC_PMGPIO_1		PMGPIO(1)
#define JET_GPIO_NC_PMGPIO_2		PMGPIO(2)

void jet_lcd_id_power(int pull);

extern struct msm_camera_board_info jet_camera_board_info;

void __init jet_init_camera(void);
void jet_init_fb(void);
void __init jet_init_pmic(void);
void jet_init_mmc(void);
int __init jet_gpiomux_init(void);
void __init msm8960_allocate_fb_region(void);
void __init jet_pm8921_gpio_mpp_init(void);
void msm8960_mdp_writeback(struct memtype_reserve *reserve_table);
int __init jet_init_keypad(void);

#ifdef CONFIG_MSM_CACHE_DUMP
extern struct msm_cache_dump_platform_data msm8960_cache_dump_pdata;
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
int hdmi_enable_5v(int on);
extern void hdmi_hpd_feature(int enable);
#endif
#endif
