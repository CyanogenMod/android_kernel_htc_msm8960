#ifndef CY8C_CS_I2C_H
#define CY8C_CS_I2C_H


#include <linux/types.h>

#define CYPRESS_CS_NAME 	"CY8C20224"
#define CYPRESS_SS_NAME 	"CY8C21x34B"

#if defined(CONFIG_TOUCH_KEY_FILTER)
#include <linux/notifier.h>
#endif

#define CS_STATUS		(uint8_t) 0x00

#define CS_FW_VERSION		(uint8_t) 0x01
#define CS_FW_CONFIG		(uint8_t) 0xAA		

#define CS_IDAC_BTN_BASE	(uint8_t) 0x02
#define CS_IDAC_BTN_PAD1	(uint8_t) 0x02
#define CS_IDAC_BTN_PAD2        (uint8_t) 0x03
#define CS_IDAC_BTN_PAD3        (uint8_t) 0x04
#define CS_IDAC_BTN_PAD4        (uint8_t) 0x05

#define CS_MODE			(uint8_t) 0x06
#define CS_DTIME		(uint8_t) 0x07
#define CS_SLEEPTIME		(uint8_t) 0x08
#define CS_FW_CHIPID		(uint8_t) 0x0A		
#define CS_FW_KEYCFG		(uint8_t) 0x0B		

#define CS_SELECT		(uint8_t) 0x0C
#define CS_BL_HB		(uint8_t) 0x0D
#define CS_BL_LB		(uint8_t) 0x0E
#define CS_RC_HB		(uint8_t) 0x0F
#define	CS_RC_LB		(uint8_t) 0x10
#define CS_DF_HB		(uint8_t) 0x11
#define CS_DF_LB		(uint8_t) 0x12
#define CS_INT_STATUS		(uint8_t) 0x13


#define CS_CMD_BASELINE		(0x55)
#define CS_CMD_DSLEEP		(0x02)
#define CS_CMD_BTN1		(0xA0)
#define CS_CMD_BTN2		(0xA1)
#define CS_CMD_BTN3		(0xA2)
#define CS_CMD_BTN4		(0xA3)

#define CS_CHIPID		(0x36)
#define CS_KEY_4		(0x04)
#define CS_KEY_3		(0x03)

#define CS_FUNC_PRINTRAW	(0x01)

#define ENABLE_CAP_ONLY_3KEY   1        

struct infor {
	uint8_t  config;
	uint16_t chipid;
	uint16_t version;
};

struct cy8c_i2c_cs_platform_data {
	struct 	infor id;
	uint16_t gpio_rst;
	uint16_t gpio_irq;
	int 	(*power)(int on);
	int 	(*reset)(void);
	int	keycode[4];
	void 	(*gpio_init)(void);
	int 	func_support;
	int     prj_info;
};


#if defined(CONFIG_TOUCH_KEY_FILTER)
extern struct blocking_notifier_head touchkey_notifier_list;

extern int register_notifier_by_touchkey(struct notifier_block *nb);
extern int unregister_notifier_by_touchkey(struct notifier_block *nb);
#endif

#endif
