#include <mach/msm_memtypes.h>
#include <linux/regulator/pm8xxx-regulator.h>
#include <linux/regulator/msm-gpio-regulator.h>
#include <mach/irqs.h>
#include <mach/rpm-regulator.h>

#define	GPIO(X)		(X)
#ifndef __ARCH_ARM_MACH_MSM_BOARD_M4_H__
#define __ARCH_ARM_MACH_MSM_BOARD_M4_H__

#define PVT_VERSION	0x80
extern struct gpio_regulator_platform_data
	m4_gpio_regulator_pdata[] __devinitdata;

extern struct rpm_regulator_platform_data
	m4_rpm_regulator_pdata __devinitdata;
extern struct rpm_regulator_platform_data
	m4_DVT1_2_rpm_regulator_pdata __devinitdata;

extern int m4_pm8038_regulator_pdata_len __devinitdata;
extern struct pm8xxx_regulator_platform_data
	m4_pm8038_regulator_pdata[] __devinitdata;

extern struct regulator_init_data m4_pm8038_saw_regulator_core0_pdata;
extern struct regulator_init_data m4_pm8038_saw_regulator_core1_pdata;

#define MSM_LCM_TE				GPIO(0)
#define MSM_WL_REG_ON				GPIO(1)
#define MSM_RAW_1V2_EN				GPIO(2)
#define MSM_HW_RST_CLRz				GPIO(3)
#define MSM_CAM_MCLK1				GPIO(4)
#define MSM_CAM_MCLK0				GPIO(5)
#define MSM_WL_HOST_WAKE			GPIO(6)
#define MSM_TP_ATTz				GPIO(7)
#define MSM_LED_3V3_EN				GPIO(8)
#define MSM_CAM_D1V2_EN				GPIO(9)
#define MSM_GYRO_INT				GPIO(10)
#define MSM_AUD_HP_EN				GPIO(11)
#define MSM_RAW_1V15_EN				GPIO(12)
#define MSM_MB_ID0				GPIO(13)
#define MSM_AUD_REMO_TX				GPIO(14)
#define MSM_AUD_REMO_RX				GPIO(15)
#define MSM_TP_I2C_SDA				GPIO(16)
#define MSM_TP_I2C_SCL				GPIO(17)
#define MSM_HSMIC_2V85_EN			GPIO(18)
#define MSM_USB_ID1				GPIO(19)
#define MSM_CAM_I2C_SDA				GPIO(20)
#define MSM_CAM_I2C_SCL				GPIO(21)
#define MSM_AUD_RECEIVER_EN			GPIO(22)
#define MSM_PROXIMITY_INTz			GPIO(23)
#define MSM_AUD_I2C_SDA				GPIO(24)
#define MSM_AUD_I2C_SCL				GPIO(25)
#define MSM_BT_UART_TX				GPIO(26)
#define MSM_BT_UART_RX				GPIO(27)
#define MSM_BT_UART_CTSz			GPIO(28)
#define MSM_BT_UART_RTSz			GPIO(29)
#define MSM_UIM1_DATA_MSM			GPIO(30)
#define MSM_UIM1_CLK_MSM			GPIO(31)
#define MSM_UIM1_RST_MSM			GPIO(32)
#define MSM_FLASH_EN				GPIO(33)
#define MSM_UART_TX				GPIO(34)
#define MSM_UART_RX				GPIO(35)
#define MSM_LCMIO_1V8_EN			GPIO(36)
#define MSM_LCD_RSTz				GPIO(37)
#define MSM_AUD_DMIC0_SEL			GPIO(38)
#define MSM_AUD_HP_DETz				GPIO(39)
#define MSM_AUD_REMO_PRESz			GPIO(40)
#define MSM_CAM_PWDN				GPIO(41)
#define MSM_AUD_WCD_RESET_N			GPIO(42)
#define MSM_VOL_DOWNz				GPIO(43)
#define MSM_SR_I2C_SDA				GPIO(44)
#define MSM_SR_I2C_SCL				GPIO(45)
#define MSM_PWR_KEY_MSMz			GPIO(46)
#define MSM_AUD_SPK_I2S_WS			GPIO(47)
#define MSM_AUD_SPK_I2S_BCLK			GPIO(48)
#define MSM_AUD_SPK_I2S_DO			GPIO(49)
#define MSM_RAW_INTR1				GPIO(50)
#define MSM_RAW_RSTN				GPIO(51)
#define MSM_AUD_SPK_I2S_DI			GPIO(52)
#define MSM_CAM_VCM_PD				GPIO(53)
#define MSM_REGION_ID				GPIO(54)
#define MSM_AUD_FMI2S_CLK			GPIO(55)
#define MSM_AUD_FMI2S_WS			GPIO(56)
#define MSM_AUD_FMI2S_DO			GPIO(57)
#define MSM_AUD_RECEIVER_SEL			GPIO(58)
#define MSM_AUD_WCD_MCLK_CPU			GPIO(59)
#define MSM_AUD_WCD_SB_CLK_CPU			GPIO(60)
#define MSM_AUD_WCD_SB_DATA			GPIO(61)
#define MSM_AUD_WCD_INTR_OUT			GPIO(62)
#define MSM_AUD_BTPCM_IN			GPIO(63)
#define MSM_AUD_BTPCM_OUT			GPIO(64)
#define MSM_AUD_BTPCM_SYNC			GPIO(65)
#define MSM_AUD_BTPCM_CLK			GPIO(66)
#define MSM_GSENSOR_INT				GPIO(67)
#define MSM_CAM2_RSTz				GPIO(68)
#define MSM_RAW_INTR0				GPIO(69)
#define MSM_COMPASS_INT				GPIO(70)
#define MSM_MCAM_SPI_DO				GPIO(71)
#define MSM_MCAM_SPI_DI				GPIO(72)
#define MSM_MCAM_SPI_CS0			GPIO(73)
#define MSM_MCAM_SPI_CLK_CPU			GPIO(74)
#define MSM_AUD_DMIC1_SEL			GPIO(75)
#define MSM_BT_REG_ON				GPIO(76)
#define MSM_CAMIO_1V8_EN			GPIO(77)
#define MSM_VOL_UPz				GPIO(78)
#define MSM_BT_DEV_WAKE				GPIO(79)
#define MSM_RAW_1V8_EN				GPIO(80)
#define MSM_TP_RSTz				GPIO(81)
#define MSM_CAM_ID				GPIO(82)
#define MSM_WIFI_SD_D3				GPIO(83)
#define MSM_WIFI_SD_D2				GPIO(84)
#define MSM_WIFI_SD_D1				GPIO(85)
#define MSM_WIFI_SD_D0				GPIO(86)
#define MSM_WIFI_SD_CMD				GPIO(87)
#define MSM_WCN_CMD_CLK_CPU			GPIO(88)
#define MSM_LCD_ID1				GPIO(89)
#define MSM_LCD_ID0				GPIO(90)
#define MSM_TORCH_FLASHz			GPIO(91)
#define MSM_BT_HOST_WAKE			GPIO(92)
#define MSM_HVDAC_SPI_DO			GPIO(93)
#define MSM_MBAT_INz				GPIO(94)
#define MSM_HVDAC_SPI_CS0			GPIO(95)
#define MSM_HVDAC_SPI_CLK			GPIO(96)
#define MSM_BOOST_5V_EN				GPIO(97)
#define MSM_BATT_ALARM				GPIO(98)
#define MSM_USB_ID_OTG				GPIO(99)
#define MSM_WIFI_FEM_DETECT			GPIO(100)
#define MSM_RESOUTz_CONTROL			GPIO(101)
#define MSM_CABLE_INz				GPIO(102)
#define MSM_PM_SEC_INTz				GPIO(103)
#define MSM_PM_USR_INTz				GPIO(104)
#define MSM_PM_MSM_INTz				GPIO(105)
#define MSM_AUD_UART_OEz			GPIO(106)
#define MSM_SIM_CDz				GPIO(107)
#define MSM_PS_HOLD_CPU				GPIO(108)
#define MSM_MSM_NC_GPIO_109			GPIO(109)
#define MSM_GPIO_110				GPIO(110)
#define MSM_W_TRX_B2_B3_SEL			GPIO(111)
#define MSM_BOOT_CONFIG_6			GPIO(112)
#define MSM_W_DRX_MODE_SEL2			GPIO(113)
#define MSM_W_DRX_MODE_SEL1			GPIO(114)
#define MSM_W_DRX_MODE_SEL0			GPIO(115)
#define MSM_W_ANT_SW_SEL3			GPIO(116)
#define MSM_W_ANT_SW_SEL2			GPIO(117)
#define MSM_W_ANT_SW_SEL1			GPIO(118)
#define MSM_W_ANT_SW_SEL0			GPIO(119)
#define MSM_MSM_NC_GPIO_120			GPIO(120)
#define MSM_MSM_NC_GPIO_121			GPIO(121)
#define MSM_W_PA0_R0				GPIO(122)
#define MSM_W_PA0_R1				GPIO(123)
#define MSM_W_DRX_SW_SEL0			GPIO(124)
#define MSM_W_WTR_RF_ON				GPIO(125)
#define MSM_W_WTR_RX_ON				GPIO(126)
#define MSM_APT0_VCON				GPIO(127)
#define MSM_W_PA_ON8_B3				GPIO(128)
#define MSM_W_PA_ON7_B4_B7			GPIO(129)
#define MSM_W_PA_ON6_B2				GPIO(130)
#define MSM_W_PA_ON5_B8				GPIO(131)
#define MSM_W_PA_ON4_B17_B20			GPIO(132)
#define MSM_W_PA_ON3_MODE			GPIO(133)
#define MSM_W_PA_ON2_HB				GPIO(134)
#define MSM_MSM_NC_GPIO_135			GPIO(135)
#define MSM_W_PA_ON0_LB				GPIO(136)
#define MSM_W_GPS_LNA_EN			GPIO(137)
#define MSM_DCIN_3G_PA_EN			GPIO(138)
#define MSM_LTE_TX_FRAME			GPIO(139)
#define MSM_LTE_RX_FRAME			GPIO(140)
#define MSM_WLAN_PRIORITY			GPIO(141)
#define MSM_W_WTR_SSBI_PRX_DRX			GPIO(142)
#define MSM_W_WTR_SSBI_TX_GNSS			GPIO(143)
#define MSM_W_ANT_DPDT_CTL1			GPIO(144)
#define MSM_MSM_NC_GPIO_145			GPIO(145)
#define MSM_W_GP_DATA2				GPIO(146)
#define MSM_W_GP_DATA1				GPIO(147)
#define MSM_W_GP_DATA0				GPIO(148)
#define MSM_W_ANT_DPDT_CTL2			GPIO(149)
#define MSM_SW_ANT_0				GPIO(150)
#define MSM_SW_ANT_1				GPIO(151)
#endif

extern int panel_type;
extern struct msm_camera_board_info m4_camera_board_info;
extern struct platform_device m4_msm_rawchip_device;
extern struct platform_device msm8930_msm_rawchip_device;

void __init m4_init_camera(void);
void m4_init_fb(void);
void __init m4_init_pmic(void);
int __init m4_init_mmc(void);
int __init m4_gpiomux_init(void);
void m4_allocate_fb_region(void);
void m4_mdp_writeback(struct memtype_reserve* reserve_table);
int __init m4_init_keypad(void);
int __init m4_wifi_init(void);
void __init m4_pm8038_gpio_mpp_init(void);

#ifdef CONFIG_MSM_RTB
extern struct msm_rtb_platform_data msm8930_rtb_pdata;
#endif
#ifdef CONFIG_MSM_CACHE_DUMP
extern struct msm_cache_dump_platform_data msm8930_cache_dump_pdata;
#endif
