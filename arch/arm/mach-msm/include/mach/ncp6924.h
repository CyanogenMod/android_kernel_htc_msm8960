
#ifndef NCP6924_H
#define NCP6924_H

#include <linux/types.h>

#define NCP6924_I2C_NAME	"ncp6924"
#define NCP6924_ENABLE		0x14
#define NCP6924_VPROGDCDC1	0x20
#define NCP6924_VPROGDCDC2	0x22
#define NCP6924_VPROGLDO1	0x24
#define NCP6924_VPROGLDO2	0x25
#define NCP6924_VPROGLDO3	0x26
#define NCP6924_VPROGLDO4	0x27

#define NCP6924_ENABLE_BIT_DCDC1	0x1
#define NCP6924_ENABLE_BIT_DCDC2	0x3
#define NCP6924_ENABLE_BIT_LDO1		0x4
#define NCP6924_ENABLE_BIT_LDO2		0x5
#define NCP6924_ENABLE_BIT_LDO3		0x6
#define NCP6924_ENABLE_BIT_LDO4		0x7

#define NCP6924_ID_DCDC1	1
#define NCP6924_ID_DCDC2	2
#define NCP6924_ID_LDO1		1
#define NCP6924_ID_LDO2		2
#define NCP6924_ID_LDO3		3
#define NCP6924_ID_LDO4		4

struct ncp6924_platform_data {
	void (*voltage_init) (void);
	int     en_gpio;
	uint8_t en_init_val;
	uint8_t dc1_init_val;
	uint8_t dc2_init_val;
	uint8_t ldo1_init_val;
	uint8_t ldo2_init_val;
	uint8_t ldo3_init_val;
	uint8_t ldo4_init_val;
};

int ncp6924_enable_ldo(u8 id, bool onoff);
int ncp6924_enable_dcdc(u8 id, bool onoff);
#endif
