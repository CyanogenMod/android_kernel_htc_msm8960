#ifndef _IR_I2C
#define _IR_I2C

#include <media/rc-core.h>

#define DEFAULT_POLLING_INTERVAL	100	

struct IR_i2c;

struct IR_i2c {
	char		       *ir_codes;
	struct i2c_client      *c;
	struct rc_dev          *rc;

	
	unsigned char          old;

	u32                    polling_interval; 

	struct delayed_work    work;
	char                   name[32];
	char                   phys[32];
	int                    (*get_key)(struct IR_i2c*, u32*, u32*);
};

enum ir_kbd_get_key_fn {
	IR_KBD_GET_KEY_CUSTOM = 0,
	IR_KBD_GET_KEY_PIXELVIEW,
	IR_KBD_GET_KEY_HAUP,
	IR_KBD_GET_KEY_KNC1,
	IR_KBD_GET_KEY_FUSIONHDTV,
	IR_KBD_GET_KEY_HAUP_XVR,
	IR_KBD_GET_KEY_AVERMEDIA_CARDBUS,
};

struct IR_i2c_init_data {
	char			*ir_codes;
	const char		*name;
	u64			type; 
	u32			polling_interval; 

	int                    (*get_key)(struct IR_i2c*, u32*, u32*);
	enum ir_kbd_get_key_fn internal_get_key_func;

	struct rc_dev		*rc_dev;
};
#endif
