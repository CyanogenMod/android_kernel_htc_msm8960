
#ifndef __TUNER_TYPES_H__
#define __TUNER_TYPES_H__

enum param_type {
	TUNER_PARAM_TYPE_RADIO,
	TUNER_PARAM_TYPE_PAL,
	TUNER_PARAM_TYPE_SECAM,
	TUNER_PARAM_TYPE_NTSC,
	TUNER_PARAM_TYPE_DIGITAL,
};

struct tuner_range {
	unsigned short limit;
	unsigned char config;
	unsigned char cb;
};

struct tuner_params {
	enum param_type type;

	unsigned int cb_first_if_lower_freq:1;
	
	unsigned int has_tda9887:1;
	unsigned int port1_fm_high_sensitivity:1;
	unsigned int port2_fm_high_sensitivity:1;
	unsigned int fm_gain_normal:1;
	unsigned int intercarrier_mode:1;
	/* This setting sets the default value for PORT1.
	   0 means inactive, 1 means active. Note: the actual bit
	   value written to the tda9887 is inverted. So a 0 here
	   means a 1 in the B6 bit. */
	unsigned int port1_active:1;
	/* This setting sets the default value for PORT2.
	   0 means inactive, 1 means active. Note: the actual bit
	   value written to the tda9887 is inverted. So a 0 here
	   means a 1 in the B7 bit. */
	unsigned int port2_active:1;
	unsigned int port1_invert_for_secam_lc:1;
	unsigned int port2_invert_for_secam_lc:1;
	
	unsigned int port1_set_for_fm_mono:1;
	unsigned int default_pll_gating_18:1;
	unsigned int radio_if:2;
	signed int default_top_low:5;
	signed int default_top_mid:5;
	signed int default_top_high:5;
	signed int default_top_secam_low:5;
	signed int default_top_secam_mid:5;
	signed int default_top_secam_high:5;

	u16 iffreq;

	unsigned int count;
	struct tuner_range *ranges;
};

struct tunertype {
	char *name;
	unsigned int count;
	struct tuner_params *params;

	u16 min;
	u16 max;
	u32 stepsize;

	u8 *initdata;
	u8 *sleepdata;
};

extern struct tunertype tuners[];
extern unsigned const int tuner_count;

#endif
