/*
 * Copyright (C) 2010 NXP Semiconductors
 */

#define PN544_I2C_NAME "pn544"

#define PN544_MAGIC	0xE9
#define PN544_SET_PWR	_IOW(PN544_MAGIC, 0x01, unsigned int)
#define IO_WAKE_LOCK_TIMEOUT (2*HZ)

struct pn544_i2c_platform_data {
	void (*gpio_init) (void);
	unsigned int irq_gpio;
	unsigned int ven_gpio;
	unsigned int firm_gpio;
	unsigned int ven_isinvert;
};
