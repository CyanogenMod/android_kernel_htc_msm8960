#ifndef __ARCH_ARM_MACH_MSM_BOARD_ZARA_H__
#define __ARCH_ARM_MACH_MSM_BOARD_ZARA_H__

#include <mach/msm_memtypes.h>
#include <linux/regulator/msm-gpio-regulator.h>
#include <mach/rpm-regulator.h>

#define	GPIO(X)		(X)

#define PVT_VERSION	0x80
#define EVM_VERSION	0x99

extern struct gpio_regulator_platform_data
	msm8930_gpio_regulator_pdata[] __devinitdata;

extern struct rpm_regulator_platform_data
	msm8930_rpm_regulator_pdata __devinitdata;

extern struct regulator_init_data msm8930_saw_regulator_core0_pdata;
extern struct regulator_init_data msm8930_saw_regulator_core1_pdata;

#define MSM_LCD_TE					GPIO(0)
#define MSM_WL_REG_ON				GPIO(1)
#define MSM_V_RAW_1V2_EN			GPIO(2)
#define MSM_AUD_REC_EN				GPIO(3)
#define MSM_CAM_MCLK1				GPIO(4)
#define MSM_RAW_MCLK				GPIO(5)
#define MSM_WL_HOST_WAKE			GPIO(6)
#define MSM_TP_ATTz					GPIO(7)
#define MSM_V_LCMIO_1V8_EN			GPIO(8)
#define MSM_V_TP_LED_3V3_EN			GPIO(9)
#define MSM_SDC3_CDz				GPIO(10)
#define MSM_AUD_REMO_PRESz			GPIO(11)
#define MSM_AUD_I2C_SDA				GPIO(12)
#define MSM_AUD_I2C_SCL				GPIO(13)
#define MSM_MDM_TX					GPIO(14)
#define MSM_MDM_RX					GPIO(15)
#define MSM_TP_I2C_SDA				GPIO(16)
#define MSM_TP_I2C_SCL				GPIO(17)
#define MSM_V_AUD_HSMIC_2V85_EN		GPIO(18)
#define MSM_USB_ID1				GPIO(19)
#define MSM_CAM_I2C_SDA				GPIO(20)
#define MSM_CAM_I2C_SCL				GPIO(21)
#define MSM_NC_GPIO_22				GPIO(22)
#define MSM_PROXIMITY_INTz			GPIO(23)
#define MSM_NFC_DL_MODE				GPIO(24)
#define MSM_NFC_VEN					GPIO(25)
#define MSM_BT_UART_TX				GPIO(26)
#define MSM_BT_UART_RX				GPIO(27)
#define MSM_BT_UART_CTSz			GPIO(28)
#define MSM_BT_UART_RTSz			GPIO(29)
#define MSM_UIM1_DATA_MSM			GPIO(30)
#define MSM_UIM1_CLK_MSM_1			GPIO(31)
#define MSM_UIM1_RST_MSM			GPIO(32)
#define MSM_DRIVER_EN				GPIO(33)
#define MSM_UART_TX					GPIO(34)
#define MSM_UART_RX					GPIO(35)
#define MSM_CAM_PWDN				GPIO(36)
#define MSM_V_CAM2_IO_1V8_EN		GPIO(37)
#define MSM_AUD_HP_EN				GPIO(38)
#define MSM_EARPHONE_DETz			GPIO(39)
#define MSM_NC_GPIO_40				GPIO(40)
#define MSM_NC_GPIO_41				GPIO(41)
#define MSM_AUD_WCD_RESET_N			GPIO(42)
#define MSM_VOL_DOWNz				GPIO(43)
#define MSM_SR_I2C_SDA				GPIO(44)
#define MSM_SR_I2C_SCL				GPIO(45)
#define MSM_PWR_KEY_MSMz 			GPIO(46)
#define MSM_AUD_CPU_RX_I2S_WS		GPIO(47)
#define MSM_AUD_CPU_RX_I2S_SCK		GPIO(48)
#define MSM_AUD_TFA_DO_A			GPIO(49)
#define MSM_RAW_INTR1				GPIO(50)
#define MSM_RAW_RSTN				GPIO(51)
#define MSM_AUD_CPU_RX_I2S_SD1		GPIO(52)
#define MSM_CAM_VCM_PD				GPIO(53)
#define MSM_REGION_ID				GPIO(54)
#define MSM_AUD_FM_I2S_BCLK			GPIO(55)
#define MSM_AUD_FM_I2S_SYNC			GPIO(56)
#define MSM_AUD_FM_I2S_DIN			GPIO(57)
#define MSM_LCD_RSTz				GPIO(58)
#define MSM_AUD_WCD_MCLK_CPU		GPIO(59)
#define MSM_AUD_WCD_SB_CLK_CPU		GPIO(60)
#define MSM_AUD_WCD_SB_DATA			GPIO(61)
#define MSM_AUD_WCD_INTR_OUT		GPIO(62)
#define MSM_AUD_BTPCM_DOUT			GPIO(63)
#define MSM_AUD_BTPCM_DIN			GPIO(64)
#define MSM_AUD_BTPCM_SYNC			GPIO(65)
#define MSM_AUD_BTPCM_CLK			GPIO(66)
#define MSM_GSENSOR_INT				GPIO(67)
#define MSM_CAM2_RSTz				GPIO(68)
#define MSM_RAW_INTR0				GPIO(69)
#define MSM_OVP_INTz				GPIO(70)
#define MSM_MCAM_SPI_DO				GPIO(71)
#define MSM_MCAM_SPI_DI				GPIO(72)
#define MSM_MCAM_SPI_CS0			GPIO(73)
#define MSM_MCAM_SPI_CLK_CPU		GPIO(74)
#define MSM_AUD_RECEIVER_SEL		GPIO(75)
#define MSM_BT_REG_ON				GPIO(76)
#define MSM_V_CAM_D1V8_EN			GPIO(77)
#define MSM_VOL_UPz					GPIO(78)
#define MSM_BT_DEV_WAKE				GPIO(79)
#define MSM_LEVEL_SHIFTER_EN		GPIO(80)
#define MSM_TP_RSTz					GPIO(81)
#define MSM_MAIN_CAM_ID				GPIO(82)
#define MSM_WIFI_SD_D3				GPIO(83)
#define MSM_WIFI_SD_D2				GPIO(84)
#define MSM_WIFI_SD_D1				GPIO(85)
#define MSM_WIFI_SD_D0				GPIO(86)
#define MSM_WIFI_SD_CMD				GPIO(87)
#define MSM_WCN_CMD_CLK_CPU			GPIO(88)
#define MSM_CAM2_STANDBY			GPIO(89)
#define MSM_LCD_ID0					GPIO(90)
#define MSM_TORCH_FLASHz			GPIO(91)
#define MSM_BT_HOST_WAKE			GPIO(92)
#define MSM_NC_GPIO_93				GPIO(93)
#define MSM_BOOST_5V_EN				GPIO(93)
#define MSM_MBAT_IN					GPIO(94)
#define MSM_NFC_I2C_SDA				GPIO(95)
#define MSM_NFC_I2C_SCL				GPIO(96)
#define MSM_LCD_ID1					GPIO(97)
#define MSM_BATT_ALARM				GPIO(98)
#define MSM_HW_RST_CLRz				GPIO(99)
#define MSM_NC_GPIO_100				GPIO(100)
#define MSM_NC_GPIO_101				GPIO(101)
#define MSM_NC_GPIO_102				GPIO(102)
#define MSM_PM_SEC_INTz				GPIO(103)
#define MSM_PM_USR_INTz				GPIO(104)
#define MSM_PM_MSM_INTz				GPIO(105)
#define MSM_NFC_IRQ					GPIO(106)
#define MSM_SIM_CDz					GPIO(107)
#define MSM_CPU_PS_HOLD				GPIO(108)
#define MSM_NC_GPIO_109				GPIO(109)
#define MSM_DRX_SW_B5_B8			GPIO(110)
#define MSM_PRX_SW_B2_B3			GPIO(111)
#define MSM_PRX_SW_B7_B40			GPIO(112)
#define MSM_DRX_MODE_SEL2			GPIO(113)
#define MSM_DRX_MODE_SEL1			GPIO(114)
#define MSM_DRX_MODE_SEL0			GPIO(115)
#define MSM_ANT0_SW_SEL3			GPIO(116)
#define MSM_ANT0_SW_SEL2			GPIO(117)
#define MSM_ANT0_SW_SEL1			GPIO(118)
#define MSM_ANT0_SW_SEL0			GPIO(119)
#define MSM_TRX_SW_B40				GPIO(120)
#define MSM_DRX_SW_B7_B40			GPIO(121)
#define MSM_PA0_R0					GPIO(122)
#define MSM_PA0_R1					GPIO(123)
#define MSM_DRX_SW_SEL				GPIO(124)
#define MSM_WTR0_RF_ON				GPIO(125)
#define MSM_WTR0_RX_ON				GPIO(126)
#define MSM_TX_AGC_ADJ_CPU			GPIO(127)
#define MSM_NC_GPIO_128				GPIO(128)
#define MSM_NC_GPIO_129				GPIO(129)
#define MSM_PA_ON6_B25_BC1_CSFB		GPIO(130)
#define MSM_PA_ON5_B41				GPIO(131)
#define MSM_NC_GPIO_132				GPIO(132)
#define MSM_PA_ON3_LC850_CSFB		GPIO(133)
#define MSM_NC_GPIO_134				GPIO(134)
#define MSM_NC_GPIO_135				GPIO(135)
#define MSM_PA_ON0_GSMLB			GPIO(136)
#define MSM_EXT_GPS_LNA_EN			GPIO(137)
#define MSM_V_3G_PA0_EN				GPIO(138)
#define MSM_NC_GPIO_139				GPIO(139)
#define MSM_NC_GPIO_140				GPIO(140)
#define MSM_NC_GPIO_141				GPIO(141)
#define MSM_WTR0_SSBI_PRX_DRX		GPIO(142)
#define MSM_WTR0_SSBI_TX_GNSS		GPIO(143)
#define MSM_NC_GPIO_144				GPIO(144)
#define MSM_NC_GPIO_145				GPIO(145)
#define MSM_NC_GPIO_146				GPIO(146)
#define MSM_NC_GPIO_147				GPIO(147)
#define MSM_NC_GPIO_148				GPIO(148)
#define MSM_NC_GPIO_149				GPIO(149)
#define MSM_SW_ANT_0				GPIO(150)
#define MSM_SW_ANT_1				GPIO(151)

#define PM_AP_BL_LED				PMGPIO(1)
#define PM_NC_GPIO_2				PMGPIO(2)
#define PM_NC_GPIO_3				PMGPIO(3)
#define PM_SSBI_PMIC_FCLK			PMGPIO(4)
#define PM_NC_GPIO_5				PMGPIO(5)
#define PM_NC_GPIO_6				PMGPIO(6)
#define PM_NC_GPIO_7				PMGPIO(7)
#define PM_BCM4330_SLEEP_CLK		PMGPIO(8)
#define PM_BATT_ALARM				PMGPIO(9)
#define PM_NC_GPIO_10				PMGPIO(10)
#define PM_NC_GPIO_11				PMGPIO(11)
#define PM_NC_GPIO_12				PMGPIO(12)

#endif

extern int panel_type;
extern struct msm_camera_board_info zara_camera_board_info_XA;
extern struct msm_camera_board_info zara_camera_board_info_XB;
extern struct platform_device zara_msm_rawchip_device;

void __init zara_init_cam(void);
void __init zara_init_fb(void);
void __init zara_init_pmic(void);
int __init zara_init_mmc(void);
int __init zara_gpiomux_init(void);
void __init  zara_allocate_fb_region(void);
void __init  zara_mdp_writeback(struct memtype_reserve* reserve_table);
int __init zara_init_keypad(void);
int __init zara_wifi_init(void);
void msm8930_init_gpu(void);

#ifdef CONFIG_MSM_RTB
extern struct msm_rtb_platform_data msm8930_rtb_pdata;
#endif
#ifdef CONFIG_MSM_CACHE_DUMP
extern struct msm_cache_dump_platform_data msm8930_cache_dump_pdata;
#endif
