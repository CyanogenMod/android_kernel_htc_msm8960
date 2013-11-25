
#include <linux/i2c.h>
#include <linux/module.h>
#include <media/tuner.h>
#include <media/tuner-types.h>




static u8 tua603x_agc103[] = { 2, 0x80|0x40|0x18|0x06|0x01, 0x00|0x50 };

static u8 tua603x_agc112[] = { 2, 0x80|0x40|0x18|0x04|0x01, 0x80|0x20 };


static struct tuner_range tuner_temic_pal_ranges[] = {
	{ 16 * 140.25 , 0x8e, 0x02, },
	{ 16 * 463.25 , 0x8e, 0x04, },
	{ 16 * 999.99        , 0x8e, 0x01, },
};

static struct tuner_params tuner_temic_pal_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_pal_ranges),
	},
};


static struct tuner_range tuner_philips_pal_i_ranges[] = {
	{ 16 * 140.25 , 0x8e, 0xa0, },
	{ 16 * 463.25 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_philips_pal_i_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_philips_pal_i_ranges,
		.count  = ARRAY_SIZE(tuner_philips_pal_i_ranges),
	},
};


static struct tuner_range tuner_philips_ntsc_ranges[] = {
	{ 16 * 157.25 , 0x8e, 0xa0, },
	{ 16 * 451.25 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_philips_ntsc_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_philips_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_philips_ntsc_ranges),
		.cb_first_if_lower_freq = 1,
	},
};


static struct tuner_range tuner_philips_secam_ranges[] = {
	{ 16 * 168.25 , 0x8e, 0xa7, },
	{ 16 * 447.25 , 0x8e, 0x97, },
	{ 16 * 999.99        , 0x8e, 0x37, },
};

static struct tuner_params tuner_philips_secam_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_SECAM,
		.ranges = tuner_philips_secam_ranges,
		.count  = ARRAY_SIZE(tuner_philips_secam_ranges),
		.cb_first_if_lower_freq = 1,
	},
};


static struct tuner_range tuner_philips_pal_ranges[] = {
	{ 16 * 168.25 , 0x8e, 0xa0, },
	{ 16 * 447.25 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_philips_pal_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_philips_pal_ranges,
		.count  = ARRAY_SIZE(tuner_philips_pal_ranges),
		.cb_first_if_lower_freq = 1,
	},
};


static struct tuner_range tuner_temic_ntsc_ranges[] = {
	{ 16 * 157.25 , 0x8e, 0x02, },
	{ 16 * 463.25 , 0x8e, 0x04, },
	{ 16 * 999.99        , 0x8e, 0x01, },
};

static struct tuner_params tuner_temic_ntsc_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_temic_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_temic_ntsc_ranges),
	},
};


static struct tuner_range tuner_temic_pal_i_ranges[] = {
	{ 16 * 170.00 , 0x8e, 0x02, },
	{ 16 * 450.00 , 0x8e, 0x04, },
	{ 16 * 999.99        , 0x8e, 0x01, },
};

static struct tuner_params tuner_temic_pal_i_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_pal_i_ranges,
		.count  = ARRAY_SIZE(tuner_temic_pal_i_ranges),
	},
};


static struct tuner_range tuner_temic_4036fy5_ntsc_ranges[] = {
	{ 16 * 157.25 , 0x8e, 0xa0, },
	{ 16 * 463.25 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_temic_4036fy5_ntsc_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_temic_4036fy5_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_temic_4036fy5_ntsc_ranges),
	},
};


static struct tuner_range tuner_alps_tsb_1_ranges[] = {
	{ 16 * 137.25 , 0x8e, 0x01, },
	{ 16 * 385.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_params tuner_alps_tsbh1_ntsc_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_alps_tsb_1_ranges,
		.count  = ARRAY_SIZE(tuner_alps_tsb_1_ranges),
	},
};


static struct tuner_params tuner_alps_tsb_1_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_alps_tsb_1_ranges,
		.count  = ARRAY_SIZE(tuner_alps_tsb_1_ranges),
	},
};


static struct tuner_range tuner_alps_tsb_5_pal_ranges[] = {
	{ 16 * 133.25 , 0x8e, 0x01, },
	{ 16 * 351.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_params tuner_alps_tsbb5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_alps_tsb_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_alps_tsb_5_pal_ranges),
	},
};


static struct tuner_params tuner_alps_tsbe5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_alps_tsb_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_alps_tsb_5_pal_ranges),
	},
};


static struct tuner_params tuner_alps_tsbc5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_alps_tsb_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_alps_tsb_5_pal_ranges),
	},
};


static struct tuner_range tuner_lg_pal_ranges[] = {
	{ 16 * 170.00 , 0x8e, 0xa0, },
	{ 16 * 450.00 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_temic_4006fh5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_pal_ranges,
		.count  = ARRAY_SIZE(tuner_lg_pal_ranges),
	},
};


static struct tuner_range tuner_alps_tshc6_ntsc_ranges[] = {
	{ 16 * 137.25 , 0x8e, 0x14, },
	{ 16 * 385.25 , 0x8e, 0x12, },
	{ 16 * 999.99        , 0x8e, 0x11, },
};

static struct tuner_params tuner_alps_tshc6_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_alps_tshc6_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_alps_tshc6_ntsc_ranges),
	},
};


static struct tuner_range tuner_temic_pal_dk_ranges[] = {
	{ 16 * 168.25 , 0x8e, 0xa0, },
	{ 16 * 456.25 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_temic_pal_dk_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_pal_dk_ranges,
		.count  = ARRAY_SIZE(tuner_temic_pal_dk_ranges),
	},
};


static struct tuner_range tuner_philips_ntsc_m_ranges[] = {
	{ 16 * 160.00 , 0x8e, 0xa0, },
	{ 16 * 454.00 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_philips_ntsc_m_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_philips_ntsc_m_ranges,
		.count  = ARRAY_SIZE(tuner_philips_ntsc_m_ranges),
	},
};


static struct tuner_range tuner_temic_40x6f_5_pal_ranges[] = {
	{ 16 * 169.00 , 0x8e, 0xa0, },
	{ 16 * 454.00 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_temic_4066fy5_pal_i_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_40x6f_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_40x6f_5_pal_ranges),
	},
};


static struct tuner_params tuner_temic_4006fn5_multi_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_40x6f_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_40x6f_5_pal_ranges),
	},
};


static struct tuner_range tuner_temic_4009f_5_pal_ranges[] = {
	{ 16 * 141.00 , 0x8e, 0xa0, },
	{ 16 * 464.00 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_temic_4009f_5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_4009f_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_4009f_5_pal_ranges),
	},
};


static struct tuner_range tuner_temic_4x3x_f_5_ntsc_ranges[] = {
	{ 16 * 158.00 , 0x8e, 0xa0, },
	{ 16 * 453.00 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_temic_4039fr5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_temic_4x3x_f_5_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_temic_4x3x_f_5_ntsc_ranges),
	},
};


static struct tuner_params tuner_temic_4046fm5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_40x6f_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_40x6f_5_pal_ranges),
	},
};


static struct tuner_params tuner_philips_pal_dk_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_pal_ranges,
		.count  = ARRAY_SIZE(tuner_lg_pal_ranges),
	},
};


static struct tuner_params tuner_philips_fq1216me_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_pal_ranges,
		.count  = ARRAY_SIZE(tuner_lg_pal_ranges),
		.has_tda9887 = 1,
		.port1_active = 1,
		.port2_active = 1,
		.port2_invert_for_secam_lc = 1,
	},
};


static struct tuner_params tuner_lg_pal_i_fm_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_pal_ranges,
		.count  = ARRAY_SIZE(tuner_lg_pal_ranges),
	},
};


static struct tuner_params tuner_lg_pal_i_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_pal_ranges,
		.count  = ARRAY_SIZE(tuner_lg_pal_ranges),
	},
};


static struct tuner_range tuner_lg_ntsc_fm_ranges[] = {
	{ 16 * 210.00 , 0x8e, 0xa0, },
	{ 16 * 497.00 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_lg_ntsc_fm_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_lg_ntsc_fm_ranges,
		.count  = ARRAY_SIZE(tuner_lg_ntsc_fm_ranges),
	},
};


static struct tuner_params tuner_lg_pal_fm_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_pal_ranges,
		.count  = ARRAY_SIZE(tuner_lg_pal_ranges),
	},
};


static struct tuner_params tuner_lg_pal_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_pal_ranges,
		.count  = ARRAY_SIZE(tuner_lg_pal_ranges),
	},
};


static struct tuner_params tuner_temic_4009_fn5_multi_pal_fm_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_4009f_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_4009f_5_pal_ranges),
	},
};


static struct tuner_range tuner_sharp_2u5jf5540_ntsc_ranges[] = {
	{ 16 * 137.25 , 0x8e, 0x01, },
	{ 16 * 317.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_params tuner_sharp_2u5jf5540_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_sharp_2u5jf5540_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_sharp_2u5jf5540_ntsc_ranges),
	},
};


static struct tuner_range tuner_samsung_pal_tcpm9091pd27_ranges[] = {
	{ 16 * 169 , 0x8e, 0xa0, },
	{ 16 * 464 , 0x8e, 0x90, },
	{ 16 * 999.99     , 0x8e, 0x30, },
};

static struct tuner_params tuner_samsung_pal_tcpm9091pd27_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_samsung_pal_tcpm9091pd27_ranges,
		.count  = ARRAY_SIZE(tuner_samsung_pal_tcpm9091pd27_ranges),
	},
};


static struct tuner_params tuner_temic_4106fh5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_4009f_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_4009f_5_pal_ranges),
	},
};


static struct tuner_params tuner_temic_4012fy5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_pal_ranges),
	},
};


static struct tuner_params tuner_temic_4136_fy5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_temic_4x3x_f_5_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_temic_4x3x_f_5_ntsc_ranges),
	},
};


static struct tuner_range tuner_lg_new_tapc_ranges[] = {
	{ 16 * 170.00 , 0x8e, 0x01, },
	{ 16 * 450.00 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_params tuner_lg_pal_new_tapc_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_new_tapc_ranges,
		.count  = ARRAY_SIZE(tuner_lg_new_tapc_ranges),
	},
};


static struct tuner_range tuner_fm1216me_mk3_pal_ranges[] = {
	{ 16 * 158.00 , 0x8e, 0x01, },
	{ 16 * 442.00 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x04, },
};

static struct tuner_params tuner_fm1216me_mk3_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_fm1216me_mk3_pal_ranges,
		.count  = ARRAY_SIZE(tuner_fm1216me_mk3_pal_ranges),
		.cb_first_if_lower_freq = 1,
		.has_tda9887 = 1,
		.port1_active = 1,
		.port2_active = 1,
		.port2_invert_for_secam_lc = 1,
		.port1_fm_high_sensitivity = 1,
		.default_top_mid = -2,
		.default_top_secam_mid = -2,
		.default_top_secam_high = -2,
	},
};


static struct tuner_range tuner_fm1216mk5_pal_ranges[] = {
	{ 16 * 158.00 , 0xce, 0x01, },
	{ 16 * 441.00 , 0xce, 0x02, },
	{ 16 * 864.00        , 0xce, 0x04, },
};

static struct tuner_params tuner_fm1216mk5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_fm1216mk5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_fm1216mk5_pal_ranges),
		.cb_first_if_lower_freq = 1,
		.has_tda9887 = 1,
		.port1_active = 1,
		.port2_active = 1,
		.port2_invert_for_secam_lc = 1,
		.port1_fm_high_sensitivity = 1,
		.default_top_mid = -2,
		.default_top_secam_mid = -2,
		.default_top_secam_high = -2,
	},
};


static struct tuner_params tuner_lg_ntsc_new_tapc_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_lg_new_tapc_ranges,
		.count  = ARRAY_SIZE(tuner_lg_new_tapc_ranges),
	},
};


static struct tuner_params tuner_hitachi_ntsc_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_lg_new_tapc_ranges,
		.count  = ARRAY_SIZE(tuner_lg_new_tapc_ranges),
	},
};


static struct tuner_range tuner_philips_pal_mk_pal_ranges[] = {
	{ 16 * 140.25 , 0x8e, 0x01, },
	{ 16 * 463.25 , 0x8e, 0xc2, },
	{ 16 * 999.99        , 0x8e, 0xcf, },
};

static struct tuner_params tuner_philips_pal_mk_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_philips_pal_mk_pal_ranges,
		.count  = ARRAY_SIZE(tuner_philips_pal_mk_pal_ranges),
	},
};


static struct tuner_range tuner_philips_fcv1236d_ntsc_ranges[] = {
	{ 16 * 157.25 , 0x8e, 0xa2, },
	{ 16 * 451.25 , 0x8e, 0x92, },
	{ 16 * 999.99        , 0x8e, 0x32, },
};

static struct tuner_range tuner_philips_fcv1236d_atsc_ranges[] = {
	{ 16 * 159.00 , 0x8e, 0xa0, },
	{ 16 * 453.00 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_philips_fcv1236d_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_philips_fcv1236d_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_philips_fcv1236d_ntsc_ranges),
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_philips_fcv1236d_atsc_ranges,
		.count  = ARRAY_SIZE(tuner_philips_fcv1236d_atsc_ranges),
		.iffreq = 16 * 44.00,
	},
};


static struct tuner_range tuner_fm1236_mk3_ntsc_ranges[] = {
	{ 16 * 160.00 , 0x8e, 0x01, },
	{ 16 * 442.00 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x04, },
};

static struct tuner_params tuner_fm1236_mk3_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_fm1236_mk3_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_fm1236_mk3_ntsc_ranges),
		.cb_first_if_lower_freq = 1,
		.has_tda9887 = 1,
		.port1_active = 1,
		.port2_active = 1,
		.port1_fm_high_sensitivity = 1,
	},
};


static struct tuner_params tuner_philips_4in1_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_fm1236_mk3_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_fm1236_mk3_ntsc_ranges),
	},
};


static struct tuner_params tuner_microtune_4049_fm5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_temic_4009f_5_pal_ranges,
		.count  = ARRAY_SIZE(tuner_temic_4009f_5_pal_ranges),
		.has_tda9887 = 1,
		.port1_invert_for_secam_lc = 1,
		.default_pll_gating_18 = 1,
		.fm_gain_normal=1,
		.radio_if = 1, 
	},
};


static struct tuner_range tuner_panasonic_vp27_ntsc_ranges[] = {
	{ 16 * 160.00 , 0xce, 0x01, },
	{ 16 * 454.00 , 0xce, 0x02, },
	{ 16 * 999.99        , 0xce, 0x08, },
};

static struct tuner_params tuner_panasonic_vp27_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_panasonic_vp27_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_panasonic_vp27_ntsc_ranges),
		.has_tda9887 = 1,
		.intercarrier_mode = 1,
		.default_top_low = -3,
		.default_top_mid = -3,
		.default_top_high = -3,
	},
};


static struct tuner_range tuner_tnf_8831bgff_pal_ranges[] = {
	{ 16 * 161.25 , 0x8e, 0xa0, },
	{ 16 * 463.25 , 0x8e, 0x90, },
	{ 16 * 999.99        , 0x8e, 0x30, },
};

static struct tuner_params tuner_tnf_8831bgff_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_tnf_8831bgff_pal_ranges,
		.count  = ARRAY_SIZE(tuner_tnf_8831bgff_pal_ranges),
	},
};


static struct tuner_range tuner_microtune_4042fi5_ntsc_ranges[] = {
	{ 16 * 162.00 , 0x8e, 0xa2, },
	{ 16 * 457.00 , 0x8e, 0x94, },
	{ 16 * 999.99        , 0x8e, 0x31, },
};

static struct tuner_range tuner_microtune_4042fi5_atsc_ranges[] = {
	{ 16 * 162.00 , 0x8e, 0xa1, },
	{ 16 * 457.00 , 0x8e, 0x91, },
	{ 16 * 999.99        , 0x8e, 0x31, },
};

static struct tuner_params tuner_microtune_4042fi5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_microtune_4042fi5_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_microtune_4042fi5_ntsc_ranges),
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_microtune_4042fi5_atsc_ranges,
		.count  = ARRAY_SIZE(tuner_microtune_4042fi5_atsc_ranges),
		.iffreq = 16 * 44.00 ,
	},
};


static struct tuner_range tuner_tcl_2002n_ntsc_ranges[] = {
	{ 16 * 172.00 , 0x8e, 0x01, },
	{ 16 * 448.00 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_params tuner_tcl_2002n_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_tcl_2002n_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_tcl_2002n_ntsc_ranges),
		.cb_first_if_lower_freq = 1,
	},
};


static struct tuner_params tuner_philips_fm1256_ih3_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_fm1236_mk3_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_fm1236_mk3_ntsc_ranges),
		.radio_if = 1, 
	},
};


static struct tuner_range tuner_thomson_dtt7610_ntsc_ranges[] = {
	{ 16 * 157.25 , 0x8e, 0x39, },
	{ 16 * 454.00 , 0x8e, 0x3a, },
	{ 16 * 999.99        , 0x8e, 0x3c, },
};

static struct tuner_params tuner_thomson_dtt7610_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_thomson_dtt7610_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_thomson_dtt7610_ntsc_ranges),
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_thomson_dtt7610_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_thomson_dtt7610_ntsc_ranges),
		.iffreq = 16 * 44.00 ,
	},
};


static struct tuner_range tuner_philips_fq1286_ntsc_ranges[] = {
	{ 16 * 160.00 , 0x8e, 0x41, },
	{ 16 * 454.00 , 0x8e, 0x42, },
	{ 16 * 999.99        , 0x8e, 0x04, },
};

static struct tuner_params tuner_philips_fq1286_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_philips_fq1286_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_philips_fq1286_ntsc_ranges),
	},
};


static struct tuner_range tuner_tcl_2002mb_pal_ranges[] = {
	{ 16 * 170.00 , 0xce, 0x01, },
	{ 16 * 450.00 , 0xce, 0x02, },
	{ 16 * 999.99        , 0xce, 0x08, },
};

static struct tuner_params tuner_tcl_2002mb_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_tcl_2002mb_pal_ranges,
		.count  = ARRAY_SIZE(tuner_tcl_2002mb_pal_ranges),
	},
};


static struct tuner_range tuner_philips_fq12_6a___mk4_pal_ranges[] = {
	{ 16 * 160.00 , 0xce, 0x01, },
	{ 16 * 442.00 , 0xce, 0x02, },
	{ 16 * 999.99        , 0xce, 0x04, },
};

static struct tuner_params tuner_philips_fq1216ame_mk4_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_philips_fq12_6a___mk4_pal_ranges,
		.count  = ARRAY_SIZE(tuner_philips_fq12_6a___mk4_pal_ranges),
		.has_tda9887 = 1,
		.port1_active = 1,
		.port2_invert_for_secam_lc = 1,
		.default_top_mid = -2,
		.default_top_secam_low = -2,
		.default_top_secam_mid = -2,
		.default_top_secam_high = -2,
	},
};


static struct tuner_params tuner_philips_fq1236a_mk4_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_fm1236_mk3_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_fm1236_mk3_ntsc_ranges),
	},
};


static struct tuner_params tuner_ymec_tvf_8531mf_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_philips_ntsc_m_ranges,
		.count  = ARRAY_SIZE(tuner_philips_ntsc_m_ranges),
	},
};


static struct tuner_range tuner_ymec_tvf_5533mf_ntsc_ranges[] = {
	{ 16 * 160.00 , 0x8e, 0x01, },
	{ 16 * 454.00 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x04, },
};

static struct tuner_params tuner_ymec_tvf_5533mf_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_ymec_tvf_5533mf_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_ymec_tvf_5533mf_ntsc_ranges),
	},
};


static struct tuner_range tuner_thomson_dtt761x_ntsc_ranges[] = {
	{ 16 * 145.25 , 0x8e, 0x39, },
	{ 16 * 415.25 , 0x8e, 0x3a, },
	{ 16 * 999.99        , 0x8e, 0x3c, },
};

static struct tuner_range tuner_thomson_dtt761x_atsc_ranges[] = {
	{ 16 * 147.00 , 0x8e, 0x39, },
	{ 16 * 417.00 , 0x8e, 0x3a, },
	{ 16 * 999.99        , 0x8e, 0x3c, },
};

static struct tuner_params tuner_thomson_dtt761x_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_thomson_dtt761x_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_thomson_dtt761x_ntsc_ranges),
		.has_tda9887 = 1,
		.fm_gain_normal = 1,
		.radio_if = 2, 
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_thomson_dtt761x_atsc_ranges,
		.count  = ARRAY_SIZE(tuner_thomson_dtt761x_atsc_ranges),
		.iffreq = 16 * 44.00, 
	},
};


static struct tuner_range tuner_tena_9533_di_pal_ranges[] = {
	{ 16 * 160.25 , 0x8e, 0x01, },
	{ 16 * 464.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x04, },
};

static struct tuner_params tuner_tena_9533_di_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_tena_9533_di_pal_ranges,
		.count  = ARRAY_SIZE(tuner_tena_9533_di_pal_ranges),
	},
};


static struct tuner_range tuner_tena_tnf_5337_ntsc_ranges[] = {
	{ 16 * 166.25 , 0x86, 0x01, },
	{ 16 * 466.25 , 0x86, 0x02, },
	{ 16 * 999.99        , 0x86, 0x08, },
};

static struct tuner_params tuner_tena_tnf_5337_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_tena_tnf_5337_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_tena_tnf_5337_ntsc_ranges),
	},
};


static struct tuner_range tuner_philips_fmd1216me_mk3_pal_ranges[] = {
	{ 16 * 160.00 , 0x86, 0x51, },
	{ 16 * 442.00 , 0x86, 0x52, },
	{ 16 * 999.99        , 0x86, 0x54, },
};

static struct tuner_range tuner_philips_fmd1216me_mk3_dvb_ranges[] = {
	{ 16 * 143.87 , 0xbc, 0x41 },
	{ 16 * 158.87 , 0xf4, 0x41 },
	{ 16 * 329.87 , 0xbc, 0x42 },
	{ 16 * 441.87 , 0xf4, 0x42 },
	{ 16 * 625.87 , 0xbc, 0x44 },
	{ 16 * 803.87 , 0xf4, 0x44 },
	{ 16 * 999.99        , 0xfc, 0x44 },
};

static struct tuner_params tuner_philips_fmd1216me_mk3_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_philips_fmd1216me_mk3_pal_ranges,
		.count  = ARRAY_SIZE(tuner_philips_fmd1216me_mk3_pal_ranges),
		.has_tda9887 = 1,
		.port1_active = 1,
		.port2_active = 1,
		.port2_fm_high_sensitivity = 1,
		.port2_invert_for_secam_lc = 1,
		.port1_set_for_fm_mono = 1,
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_philips_fmd1216me_mk3_dvb_ranges,
		.count  = ARRAY_SIZE(tuner_philips_fmd1216me_mk3_dvb_ranges),
		.iffreq = 16 * 36.125, 
	},
};

static struct tuner_params tuner_philips_fmd1216mex_mk3_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_philips_fmd1216me_mk3_pal_ranges,
		.count  = ARRAY_SIZE(tuner_philips_fmd1216me_mk3_pal_ranges),
		.has_tda9887 = 1,
		.port1_active = 1,
		.port2_active = 1,
		.port2_fm_high_sensitivity = 1,
		.port2_invert_for_secam_lc = 1,
		.port1_set_for_fm_mono = 1,
		.radio_if = 1,
		.fm_gain_normal = 1,
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_philips_fmd1216me_mk3_dvb_ranges,
		.count  = ARRAY_SIZE(tuner_philips_fmd1216me_mk3_dvb_ranges),
		.iffreq = 16 * 36.125, 
	},
};


static struct tuner_range tuner_tua6034_ntsc_ranges[] = {
	{ 16 * 165.00 , 0x8e, 0x01 },
	{ 16 * 450.00 , 0x8e, 0x02 },
	{ 16 * 999.99        , 0x8e, 0x04 },
};

static struct tuner_range tuner_tua6034_atsc_ranges[] = {
	{ 16 * 165.00 , 0xce, 0x01 },
	{ 16 * 450.00 , 0xce, 0x02 },
	{ 16 * 999.99        , 0xce, 0x04 },
};

static struct tuner_params tuner_lg_tdvs_h06xf_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_tua6034_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_tua6034_ntsc_ranges),
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_tua6034_atsc_ranges,
		.count  = ARRAY_SIZE(tuner_tua6034_atsc_ranges),
		.iffreq = 16 * 44.00,
	},
};


static struct tuner_range tuner_ymec_tvf66t5_b_dff_pal_ranges[] = {
	{ 16 * 160.25 , 0x8e, 0x01, },
	{ 16 * 464.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_params tuner_ymec_tvf66t5_b_dff_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_ymec_tvf66t5_b_dff_pal_ranges,
		.count  = ARRAY_SIZE(tuner_ymec_tvf66t5_b_dff_pal_ranges),
	},
};


static struct tuner_range tuner_lg_taln_ntsc_ranges[] = {
	{ 16 * 137.25 , 0x8e, 0x01, },
	{ 16 * 373.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_range tuner_lg_taln_pal_secam_ranges[] = {
	{ 16 * 150.00 , 0x8e, 0x01, },
	{ 16 * 425.00 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_params tuner_lg_taln_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_lg_taln_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_lg_taln_ntsc_ranges),
	},{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_lg_taln_pal_secam_ranges,
		.count  = ARRAY_SIZE(tuner_lg_taln_pal_secam_ranges),
	},
};


static struct tuner_range tuner_philips_td1316_pal_ranges[] = {
	{ 16 * 160.00 , 0xc8, 0xa1, },
	{ 16 * 442.00 , 0xc8, 0xa2, },
	{ 16 * 999.99        , 0xc8, 0xa4, },
};

static struct tuner_range tuner_philips_td1316_dvb_ranges[] = {
	{ 16 *  93.834 , 0xca, 0x60, },
	{ 16 * 123.834 , 0xca, 0xa0, },
	{ 16 * 163.834 , 0xca, 0xc0, },
	{ 16 * 253.834 , 0xca, 0x60, },
	{ 16 * 383.834 , 0xca, 0xa0, },
	{ 16 * 443.834 , 0xca, 0xc0, },
	{ 16 * 583.834 , 0xca, 0x60, },
	{ 16 * 793.834 , 0xca, 0xa0, },
	{ 16 * 999.999        , 0xca, 0xe0, },
};

static struct tuner_params tuner_philips_td1316_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_philips_td1316_pal_ranges,
		.count  = ARRAY_SIZE(tuner_philips_td1316_pal_ranges),
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_philips_td1316_dvb_ranges,
		.count  = ARRAY_SIZE(tuner_philips_td1316_dvb_ranges),
		.iffreq = 16 * 36.166667 ,
	},
};


static struct tuner_range tuner_tuv1236d_ntsc_ranges[] = {
	{ 16 * 157.25 , 0xce, 0x01, },
	{ 16 * 454.00 , 0xce, 0x02, },
	{ 16 * 999.99        , 0xce, 0x04, },
};

static struct tuner_range tuner_tuv1236d_atsc_ranges[] = {
	{ 16 * 157.25 , 0xc6, 0x41, },
	{ 16 * 454.00 , 0xc6, 0x42, },
	{ 16 * 999.99        , 0xc6, 0x44, },
};

static struct tuner_params tuner_tuv1236d_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_tuv1236d_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_tuv1236d_ntsc_ranges),
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_tuv1236d_atsc_ranges,
		.count  = ARRAY_SIZE(tuner_tuv1236d_atsc_ranges),
		.iffreq = 16 * 44.00,
	},
};


static struct tuner_range tuner_tnf_5335_d_if_pal_ranges[] = {
	{ 16 * 168.25 , 0x8e, 0x01, },
	{ 16 * 471.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_range tuner_tnf_5335mf_ntsc_ranges[] = {
	{ 16 * 169.25 , 0x8e, 0x01, },
	{ 16 * 469.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x08, },
};

static struct tuner_params tuner_tnf_5335mf_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_tnf_5335mf_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_tnf_5335mf_ntsc_ranges),
	},
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_tnf_5335_d_if_pal_ranges,
		.count  = ARRAY_SIZE(tuner_tnf_5335_d_if_pal_ranges),
	},
};


static struct tuner_range tuner_samsung_tcpn_2121p30a_ntsc_ranges[] = {
	{ 16 * 130.00 , 0xce, 0x01 + 4, },
	{ 16 * 364.50 , 0xce, 0x02 + 4, },
	{ 16 * 999.99        , 0xce, 0x08 + 4, },
};

static struct tuner_params tuner_samsung_tcpn_2121p30a_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_samsung_tcpn_2121p30a_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_samsung_tcpn_2121p30a_ntsc_ranges),
	},
};


static struct tuner_range tuner_thomson_fe6600_pal_ranges[] = {
	{ 16 * 160.00 , 0xfe, 0x11, },
	{ 16 * 442.00 , 0xf6, 0x12, },
	{ 16 * 999.99        , 0xf6, 0x18, },
};

static struct tuner_range tuner_thomson_fe6600_dvb_ranges[] = {
	{ 16 * 250.00 , 0xb4, 0x12, },
	{ 16 * 455.00 , 0xfe, 0x11, },
	{ 16 * 775.50 , 0xbc, 0x18, },
	{ 16 * 999.99        , 0xf4, 0x18, },
};

static struct tuner_params tuner_thomson_fe6600_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_thomson_fe6600_pal_ranges,
		.count  = ARRAY_SIZE(tuner_thomson_fe6600_pal_ranges),
	},
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_thomson_fe6600_dvb_ranges,
		.count  = ARRAY_SIZE(tuner_thomson_fe6600_dvb_ranges),
		.iffreq = 16 * 36.125 ,
	},
};


static struct tuner_range tuner_samsung_tcpg_6121p30a_pal_ranges[] = {
	{ 16 * 146.25 , 0xce, 0x01 + 4, },
	{ 16 * 428.50 , 0xce, 0x02 + 4, },
	{ 16 * 999.99        , 0xce, 0x08 + 4, },
};

static struct tuner_params tuner_samsung_tcpg_6121p30a_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_samsung_tcpg_6121p30a_pal_ranges,
		.count  = ARRAY_SIZE(tuner_samsung_tcpg_6121p30a_pal_ranges),
		.has_tda9887 = 1,
		.port1_active = 1,
		.port2_active = 1,
		.port2_invert_for_secam_lc = 1,
	},
};


static struct tuner_range tuner_tcl_mf02gip_5n_ntsc_ranges[] = {
	{ 16 * 172.00 , 0x8e, 0x01, },
	{ 16 * 448.00 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x04, },
};

static struct tuner_params tuner_tcl_mf02gip_5n_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_tcl_mf02gip_5n_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_tcl_mf02gip_5n_ntsc_ranges),
		.cb_first_if_lower_freq = 1,
	},
};


static struct tuner_params tuner_fq1216lme_mk3_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_PAL,
		.ranges = tuner_fm1216me_mk3_pal_ranges,
		.count  = ARRAY_SIZE(tuner_fm1216me_mk3_pal_ranges),
		.cb_first_if_lower_freq = 1, 
		.has_tda9887 = 1, 
		.port1_active = 1,
		.port2_active = 1,
		.port2_invert_for_secam_lc = 1,
		.default_top_low = 4,
		.default_top_mid = 4,
		.default_top_high = 4,
		.default_top_secam_low = 4,
		.default_top_secam_mid = 4,
		.default_top_secam_high = 4,
	},
};


static struct tuner_range tuner_partsnic_pti_5nf05_ranges[] = {
	
	
	{ 16 * 133.25 , 0x8e, 0x01, }, 
	{ 16 * 367.25 , 0x8e, 0x02, }, 
	{ 16 * 999.99        , 0x8e, 0x08, }, 
};

static struct tuner_params tuner_partsnic_pti_5nf05_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_partsnic_pti_5nf05_ranges,
		.count  = ARRAY_SIZE(tuner_partsnic_pti_5nf05_ranges),
		.cb_first_if_lower_freq = 1, 
	},
};


static struct tuner_range tuner_cu1216l_ranges[] = {
	{ 16 * 160.25 , 0xce, 0x01 },
	{ 16 * 444.25 , 0xce, 0x02 },
	{ 16 * 999.99        , 0xce, 0x04 },
};

static struct tuner_params tuner_philips_cu1216l_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_DIGITAL,
		.ranges = tuner_cu1216l_ranges,
		.count  = ARRAY_SIZE(tuner_cu1216l_ranges),
		.iffreq = 16 * 36.125, 
	},
};


static struct tuner_range tuner_sony_btf_pxn01z_ranges[] = {
	{ 16 * 137.25 , 0x8e, 0x01, },
	{ 16 * 367.25 , 0x8e, 0x02, },
	{ 16 * 999.99        , 0x8e, 0x04, },
};

static struct tuner_params tuner_sony_btf_pxn01z_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_sony_btf_pxn01z_ranges,
		.count  = ARRAY_SIZE(tuner_sony_btf_pxn01z_ranges),
	},
};


static struct tuner_params tuner_philips_fq1236_mk5_params[] = {
	{
		.type   = TUNER_PARAM_TYPE_NTSC,
		.ranges = tuner_fm1236_mk3_ntsc_ranges,
		.count  = ARRAY_SIZE(tuner_fm1236_mk3_ntsc_ranges),
		.has_tda9887 = 1, 
	},
};


struct tunertype tuners[] = {
	
	[TUNER_TEMIC_PAL] = { 
		.name   = "Temic PAL (4002 FH5)",
		.params = tuner_temic_pal_params,
		.count  = ARRAY_SIZE(tuner_temic_pal_params),
	},
	[TUNER_PHILIPS_PAL_I] = { 
		.name   = "Philips PAL_I (FI1246 and compatibles)",
		.params = tuner_philips_pal_i_params,
		.count  = ARRAY_SIZE(tuner_philips_pal_i_params),
	},
	[TUNER_PHILIPS_NTSC] = { 
		.name   = "Philips NTSC (FI1236,FM1236 and compatibles)",
		.params = tuner_philips_ntsc_params,
		.count  = ARRAY_SIZE(tuner_philips_ntsc_params),
	},
	[TUNER_PHILIPS_SECAM] = { 
		.name   = "Philips (SECAM+PAL_BG) (FI1216MF, FM1216MF, FR1216MF)",
		.params = tuner_philips_secam_params,
		.count  = ARRAY_SIZE(tuner_philips_secam_params),
	},
	[TUNER_ABSENT] = { 
		.name   = "NoTuner",
	},
	[TUNER_PHILIPS_PAL] = { 
		.name   = "Philips PAL_BG (FI1216 and compatibles)",
		.params = tuner_philips_pal_params,
		.count  = ARRAY_SIZE(tuner_philips_pal_params),
	},
	[TUNER_TEMIC_NTSC] = { 
		.name   = "Temic NTSC (4032 FY5)",
		.params = tuner_temic_ntsc_params,
		.count  = ARRAY_SIZE(tuner_temic_ntsc_params),
	},
	[TUNER_TEMIC_PAL_I] = { 
		.name   = "Temic PAL_I (4062 FY5)",
		.params = tuner_temic_pal_i_params,
		.count  = ARRAY_SIZE(tuner_temic_pal_i_params),
	},
	[TUNER_TEMIC_4036FY5_NTSC] = { 
		.name   = "Temic NTSC (4036 FY5)",
		.params = tuner_temic_4036fy5_ntsc_params,
		.count  = ARRAY_SIZE(tuner_temic_4036fy5_ntsc_params),
	},
	[TUNER_ALPS_TSBH1_NTSC] = { 
		.name   = "Alps HSBH1",
		.params = tuner_alps_tsbh1_ntsc_params,
		.count  = ARRAY_SIZE(tuner_alps_tsbh1_ntsc_params),
	},

	
	[TUNER_ALPS_TSBE1_PAL] = { 
		.name   = "Alps TSBE1",
		.params = tuner_alps_tsb_1_params,
		.count  = ARRAY_SIZE(tuner_alps_tsb_1_params),
	},
	[TUNER_ALPS_TSBB5_PAL_I] = { 
		.name   = "Alps TSBB5",
		.params = tuner_alps_tsbb5_params,
		.count  = ARRAY_SIZE(tuner_alps_tsbb5_params),
	},
	[TUNER_ALPS_TSBE5_PAL] = { 
		.name   = "Alps TSBE5",
		.params = tuner_alps_tsbe5_params,
		.count  = ARRAY_SIZE(tuner_alps_tsbe5_params),
	},
	[TUNER_ALPS_TSBC5_PAL] = { 
		.name   = "Alps TSBC5",
		.params = tuner_alps_tsbc5_params,
		.count  = ARRAY_SIZE(tuner_alps_tsbc5_params),
	},
	[TUNER_TEMIC_4006FH5_PAL] = { 
		.name   = "Temic PAL_BG (4006FH5)",
		.params = tuner_temic_4006fh5_params,
		.count  = ARRAY_SIZE(tuner_temic_4006fh5_params),
	},
	[TUNER_ALPS_TSHC6_NTSC] = { 
		.name   = "Alps TSCH6",
		.params = tuner_alps_tshc6_params,
		.count  = ARRAY_SIZE(tuner_alps_tshc6_params),
	},
	[TUNER_TEMIC_PAL_DK] = { 
		.name   = "Temic PAL_DK (4016 FY5)",
		.params = tuner_temic_pal_dk_params,
		.count  = ARRAY_SIZE(tuner_temic_pal_dk_params),
	},
	[TUNER_PHILIPS_NTSC_M] = { 
		.name   = "Philips NTSC_M (MK2)",
		.params = tuner_philips_ntsc_m_params,
		.count  = ARRAY_SIZE(tuner_philips_ntsc_m_params),
	},
	[TUNER_TEMIC_4066FY5_PAL_I] = { 
		.name   = "Temic PAL_I (4066 FY5)",
		.params = tuner_temic_4066fy5_pal_i_params,
		.count  = ARRAY_SIZE(tuner_temic_4066fy5_pal_i_params),
	},
	[TUNER_TEMIC_4006FN5_MULTI_PAL] = { 
		.name   = "Temic PAL* auto (4006 FN5)",
		.params = tuner_temic_4006fn5_multi_params,
		.count  = ARRAY_SIZE(tuner_temic_4006fn5_multi_params),
	},

	
	[TUNER_TEMIC_4009FR5_PAL] = { 
		.name   = "Temic PAL_BG (4009 FR5) or PAL_I (4069 FR5)",
		.params = tuner_temic_4009f_5_params,
		.count  = ARRAY_SIZE(tuner_temic_4009f_5_params),
	},
	[TUNER_TEMIC_4039FR5_NTSC] = { 
		.name   = "Temic NTSC (4039 FR5)",
		.params = tuner_temic_4039fr5_params,
		.count  = ARRAY_SIZE(tuner_temic_4039fr5_params),
	},
	[TUNER_TEMIC_4046FM5] = { 
		.name   = "Temic PAL/SECAM multi (4046 FM5)",
		.params = tuner_temic_4046fm5_params,
		.count  = ARRAY_SIZE(tuner_temic_4046fm5_params),
	},
	[TUNER_PHILIPS_PAL_DK] = { 
		.name   = "Philips PAL_DK (FI1256 and compatibles)",
		.params = tuner_philips_pal_dk_params,
		.count  = ARRAY_SIZE(tuner_philips_pal_dk_params),
	},
	[TUNER_PHILIPS_FQ1216ME] = { 
		.name   = "Philips PAL/SECAM multi (FQ1216ME)",
		.params = tuner_philips_fq1216me_params,
		.count  = ARRAY_SIZE(tuner_philips_fq1216me_params),
	},
	[TUNER_LG_PAL_I_FM] = { 
		.name   = "LG PAL_I+FM (TAPC-I001D)",
		.params = tuner_lg_pal_i_fm_params,
		.count  = ARRAY_SIZE(tuner_lg_pal_i_fm_params),
	},
	[TUNER_LG_PAL_I] = { 
		.name   = "LG PAL_I (TAPC-I701D)",
		.params = tuner_lg_pal_i_params,
		.count  = ARRAY_SIZE(tuner_lg_pal_i_params),
	},
	[TUNER_LG_NTSC_FM] = { 
		.name   = "LG NTSC+FM (TPI8NSR01F)",
		.params = tuner_lg_ntsc_fm_params,
		.count  = ARRAY_SIZE(tuner_lg_ntsc_fm_params),
	},
	[TUNER_LG_PAL_FM] = { 
		.name   = "LG PAL_BG+FM (TPI8PSB01D)",
		.params = tuner_lg_pal_fm_params,
		.count  = ARRAY_SIZE(tuner_lg_pal_fm_params),
	},
	[TUNER_LG_PAL] = { 
		.name   = "LG PAL_BG (TPI8PSB11D)",
		.params = tuner_lg_pal_params,
		.count  = ARRAY_SIZE(tuner_lg_pal_params),
	},

	
	[TUNER_TEMIC_4009FN5_MULTI_PAL_FM] = { 
		.name   = "Temic PAL* auto + FM (4009 FN5)",
		.params = tuner_temic_4009_fn5_multi_pal_fm_params,
		.count  = ARRAY_SIZE(tuner_temic_4009_fn5_multi_pal_fm_params),
	},
	[TUNER_SHARP_2U5JF5540_NTSC] = { 
		.name   = "SHARP NTSC_JP (2U5JF5540)",
		.params = tuner_sharp_2u5jf5540_params,
		.count  = ARRAY_SIZE(tuner_sharp_2u5jf5540_params),
	},
	[TUNER_Samsung_PAL_TCPM9091PD27] = { 
		.name   = "Samsung PAL TCPM9091PD27",
		.params = tuner_samsung_pal_tcpm9091pd27_params,
		.count  = ARRAY_SIZE(tuner_samsung_pal_tcpm9091pd27_params),
	},
	[TUNER_MT2032] = { 
		.name   = "MT20xx universal",
		 },
	[TUNER_TEMIC_4106FH5] = { 
		.name   = "Temic PAL_BG (4106 FH5)",
		.params = tuner_temic_4106fh5_params,
		.count  = ARRAY_SIZE(tuner_temic_4106fh5_params),
	},
	[TUNER_TEMIC_4012FY5] = { 
		.name   = "Temic PAL_DK/SECAM_L (4012 FY5)",
		.params = tuner_temic_4012fy5_params,
		.count  = ARRAY_SIZE(tuner_temic_4012fy5_params),
	},
	[TUNER_TEMIC_4136FY5] = { 
		.name   = "Temic NTSC (4136 FY5)",
		.params = tuner_temic_4136_fy5_params,
		.count  = ARRAY_SIZE(tuner_temic_4136_fy5_params),
	},
	[TUNER_LG_PAL_NEW_TAPC] = { 
		.name   = "LG PAL (newer TAPC series)",
		.params = tuner_lg_pal_new_tapc_params,
		.count  = ARRAY_SIZE(tuner_lg_pal_new_tapc_params),
	},
	[TUNER_PHILIPS_FM1216ME_MK3] = { 
		.name   = "Philips PAL/SECAM multi (FM1216ME MK3)",
		.params = tuner_fm1216me_mk3_params,
		.count  = ARRAY_SIZE(tuner_fm1216me_mk3_params),
	},
	[TUNER_LG_NTSC_NEW_TAPC] = { 
		.name   = "LG NTSC (newer TAPC series)",
		.params = tuner_lg_ntsc_new_tapc_params,
		.count  = ARRAY_SIZE(tuner_lg_ntsc_new_tapc_params),
	},

	
	[TUNER_HITACHI_NTSC] = { 
		.name   = "HITACHI V7-J180AT",
		.params = tuner_hitachi_ntsc_params,
		.count  = ARRAY_SIZE(tuner_hitachi_ntsc_params),
	},
	[TUNER_PHILIPS_PAL_MK] = { 
		.name   = "Philips PAL_MK (FI1216 MK)",
		.params = tuner_philips_pal_mk_params,
		.count  = ARRAY_SIZE(tuner_philips_pal_mk_params),
	},
	[TUNER_PHILIPS_FCV1236D] = { 
		.name   = "Philips FCV1236D ATSC/NTSC dual in",
		.params = tuner_philips_fcv1236d_params,
		.count  = ARRAY_SIZE(tuner_philips_fcv1236d_params),
		.min = 16 *  53.00,
		.max = 16 * 803.00,
		.stepsize = 62500,
	},
	[TUNER_PHILIPS_FM1236_MK3] = { 
		.name   = "Philips NTSC MK3 (FM1236MK3 or FM1236/F)",
		.params = tuner_fm1236_mk3_params,
		.count  = ARRAY_SIZE(tuner_fm1236_mk3_params),
	},
	[TUNER_PHILIPS_4IN1] = { 
		.name   = "Philips 4 in 1 (ATI TV Wonder Pro/Conexant)",
		.params = tuner_philips_4in1_params,
		.count  = ARRAY_SIZE(tuner_philips_4in1_params),
	},
	[TUNER_MICROTUNE_4049FM5] = { 
		.name   = "Microtune 4049 FM5",
		.params = tuner_microtune_4049_fm5_params,
		.count  = ARRAY_SIZE(tuner_microtune_4049_fm5_params),
	},
	[TUNER_PANASONIC_VP27] = { 
		.name   = "Panasonic VP27s/ENGE4324D",
		.params = tuner_panasonic_vp27_params,
		.count  = ARRAY_SIZE(tuner_panasonic_vp27_params),
	},
	[TUNER_LG_NTSC_TAPE] = { 
		.name   = "LG NTSC (TAPE series)",
		.params = tuner_fm1236_mk3_params,
		.count  = ARRAY_SIZE(tuner_fm1236_mk3_params),
	},
	[TUNER_TNF_8831BGFF] = { 
		.name   = "Tenna TNF 8831 BGFF)",
		.params = tuner_tnf_8831bgff_params,
		.count  = ARRAY_SIZE(tuner_tnf_8831bgff_params),
	},
	[TUNER_MICROTUNE_4042FI5] = { 
		.name   = "Microtune 4042 FI5 ATSC/NTSC dual in",
		.params = tuner_microtune_4042fi5_params,
		.count  = ARRAY_SIZE(tuner_microtune_4042fi5_params),
		.min    = 16 *  57.00,
		.max    = 16 * 858.00,
		.stepsize = 62500,
	},

	
	[TUNER_TCL_2002N] = { 
		.name   = "TCL 2002N",
		.params = tuner_tcl_2002n_params,
		.count  = ARRAY_SIZE(tuner_tcl_2002n_params),
	},
	[TUNER_PHILIPS_FM1256_IH3] = { 
		.name   = "Philips PAL/SECAM_D (FM 1256 I-H3)",
		.params = tuner_philips_fm1256_ih3_params,
		.count  = ARRAY_SIZE(tuner_philips_fm1256_ih3_params),
	},
	[TUNER_THOMSON_DTT7610] = { 
		.name   = "Thomson DTT 7610 (ATSC/NTSC)",
		.params = tuner_thomson_dtt7610_params,
		.count  = ARRAY_SIZE(tuner_thomson_dtt7610_params),
		.min    = 16 *  44.00,
		.max    = 16 * 958.00,
		.stepsize = 62500,
	},
	[TUNER_PHILIPS_FQ1286] = { 
		.name   = "Philips FQ1286",
		.params = tuner_philips_fq1286_params,
		.count  = ARRAY_SIZE(tuner_philips_fq1286_params),
	},
	[TUNER_PHILIPS_TDA8290] = { 
		.name   = "Philips/NXP TDA 8290/8295 + 8275/8275A/18271",
		 },
	[TUNER_TCL_2002MB] = { 
		.name   = "TCL 2002MB",
		.params = tuner_tcl_2002mb_params,
		.count  = ARRAY_SIZE(tuner_tcl_2002mb_params),
	},
	[TUNER_PHILIPS_FQ1216AME_MK4] = { 
		.name   = "Philips PAL/SECAM multi (FQ1216AME MK4)",
		.params = tuner_philips_fq1216ame_mk4_params,
		.count  = ARRAY_SIZE(tuner_philips_fq1216ame_mk4_params),
	},
	[TUNER_PHILIPS_FQ1236A_MK4] = { 
		.name   = "Philips FQ1236A MK4",
		.params = tuner_philips_fq1236a_mk4_params,
		.count  = ARRAY_SIZE(tuner_philips_fq1236a_mk4_params),
	},
	[TUNER_YMEC_TVF_8531MF] = { 
		.name   = "Ymec TVision TVF-8531MF/8831MF/8731MF",
		.params = tuner_ymec_tvf_8531mf_params,
		.count  = ARRAY_SIZE(tuner_ymec_tvf_8531mf_params),
	},
	[TUNER_YMEC_TVF_5533MF] = { 
		.name   = "Ymec TVision TVF-5533MF",
		.params = tuner_ymec_tvf_5533mf_params,
		.count  = ARRAY_SIZE(tuner_ymec_tvf_5533mf_params),
	},

	
	[TUNER_THOMSON_DTT761X] = { 
		
		.name   = "Thomson DTT 761X (ATSC/NTSC)",
		.params = tuner_thomson_dtt761x_params,
		.count  = ARRAY_SIZE(tuner_thomson_dtt761x_params),
		.min    = 16 *  57.00,
		.max    = 16 * 863.00,
		.stepsize = 62500,
		.initdata = tua603x_agc103,
	},
	[TUNER_TENA_9533_DI] = { 
		.name   = "Tena TNF9533-D/IF/TNF9533-B/DF",
		.params = tuner_tena_9533_di_params,
		.count  = ARRAY_SIZE(tuner_tena_9533_di_params),
	},
	[TUNER_TEA5767] = { 
		.name   = "Philips TEA5767HN FM Radio",
		
	},
	[TUNER_PHILIPS_FMD1216ME_MK3] = { 
		.name   = "Philips FMD1216ME MK3 Hybrid Tuner",
		.params = tuner_philips_fmd1216me_mk3_params,
		.count  = ARRAY_SIZE(tuner_philips_fmd1216me_mk3_params),
		.min = 16 *  50.87,
		.max = 16 * 858.00,
		.stepsize = 166667,
		.initdata = tua603x_agc112,
		.sleepdata = (u8[]){ 4, 0x9c, 0x60, 0x85, 0x54 },
	},
	[TUNER_LG_TDVS_H06XF] = { 
		.name   = "LG TDVS-H06xF", 
		.params = tuner_lg_tdvs_h06xf_params,
		.count  = ARRAY_SIZE(tuner_lg_tdvs_h06xf_params),
		.min    = 16 *  54.00,
		.max    = 16 * 863.00,
		.stepsize = 62500,
		.initdata = tua603x_agc103,
	},
	[TUNER_YMEC_TVF66T5_B_DFF] = { 
		.name   = "Ymec TVF66T5-B/DFF",
		.params = tuner_ymec_tvf66t5_b_dff_params,
		.count  = ARRAY_SIZE(tuner_ymec_tvf66t5_b_dff_params),
	},
	[TUNER_LG_TALN] = { 
		.name   = "LG TALN series",
		.params = tuner_lg_taln_params,
		.count  = ARRAY_SIZE(tuner_lg_taln_params),
	},
	[TUNER_PHILIPS_TD1316] = { 
		.name   = "Philips TD1316 Hybrid Tuner",
		.params = tuner_philips_td1316_params,
		.count  = ARRAY_SIZE(tuner_philips_td1316_params),
		.min    = 16 *  87.00,
		.max    = 16 * 895.00,
		.stepsize = 166667,
	},
	[TUNER_PHILIPS_TUV1236D] = { 
		.name   = "Philips TUV1236D ATSC/NTSC dual in",
		.params = tuner_tuv1236d_params,
		.count  = ARRAY_SIZE(tuner_tuv1236d_params),
		.min    = 16 *  54.00,
		.max    = 16 * 864.00,
		.stepsize = 62500,
	},
	[TUNER_TNF_5335MF] = { 
		.name   = "Tena TNF 5335 and similar models",
		.params = tuner_tnf_5335mf_params,
		.count  = ARRAY_SIZE(tuner_tnf_5335mf_params),
	},

	
	[TUNER_SAMSUNG_TCPN_2121P30A] = { 
		.name   = "Samsung TCPN 2121P30A",
		.params = tuner_samsung_tcpn_2121p30a_params,
		.count  = ARRAY_SIZE(tuner_samsung_tcpn_2121p30a_params),
	},
	[TUNER_XC2028] = { 
		.name   = "Xceive xc2028/xc3028 tuner",
		
	},
	[TUNER_THOMSON_FE6600] = { 
		.name   = "Thomson FE6600",
		.params = tuner_thomson_fe6600_params,
		.count  = ARRAY_SIZE(tuner_thomson_fe6600_params),
		.min    = 16 *  44.25,
		.max    = 16 * 858.00,
		.stepsize = 166667,
	},
	[TUNER_SAMSUNG_TCPG_6121P30A] = { 
		.name   = "Samsung TCPG 6121P30A",
		.params = tuner_samsung_tcpg_6121p30a_params,
		.count  = ARRAY_SIZE(tuner_samsung_tcpg_6121p30a_params),
	},
	[TUNER_TDA9887] = { 
		.name   = "Philips TDA988[5,6,7] IF PLL Demodulator",
		
	},
	[TUNER_TEA5761] = { 
		.name   = "Philips TEA5761 FM Radio",
		
	},
	[TUNER_XC5000] = { 
		.name   = "Xceive 5000 tuner",
		
	},
	[TUNER_XC4000] = { 
		.name   = "Xceive 4000 tuner",
		
	},
	[TUNER_TCL_MF02GIP_5N] = { 
		.name   = "TCL tuner MF02GIP-5N-E",
		.params = tuner_tcl_mf02gip_5n_params,
		.count  = ARRAY_SIZE(tuner_tcl_mf02gip_5n_params),
	},
	[TUNER_PHILIPS_FMD1216MEX_MK3] = { 
		.name   = "Philips FMD1216MEX MK3 Hybrid Tuner",
		.params = tuner_philips_fmd1216mex_mk3_params,
		.count  = ARRAY_SIZE(tuner_philips_fmd1216mex_mk3_params),
		.min = 16 *  50.87,
		.max = 16 * 858.00,
		.stepsize = 166667,
		.initdata = tua603x_agc112,
		.sleepdata = (u8[]){ 4, 0x9c, 0x60, 0x85, 0x54 },
	},
		[TUNER_PHILIPS_FM1216MK5] = { 
		.name   = "Philips PAL/SECAM multi (FM1216 MK5)",
		.params = tuner_fm1216mk5_params,
		.count  = ARRAY_SIZE(tuner_fm1216mk5_params),
	},

	
	[TUNER_PHILIPS_FQ1216LME_MK3] = { 
		.name = "Philips FQ1216LME MK3 PAL/SECAM w/active loopthrough",
		.params = tuner_fq1216lme_mk3_params,
		.count  = ARRAY_SIZE(tuner_fq1216lme_mk3_params),
	},

	[TUNER_PARTSNIC_PTI_5NF05] = {
		.name = "Partsnic (Daewoo) PTI-5NF05",
		.params = tuner_partsnic_pti_5nf05_params,
		.count  = ARRAY_SIZE(tuner_partsnic_pti_5nf05_params),
	},
	[TUNER_PHILIPS_CU1216L] = {
		.name = "Philips CU1216L",
		.params = tuner_philips_cu1216l_params,
		.count  = ARRAY_SIZE(tuner_philips_cu1216l_params),
		.stepsize = 62500,
	},
	[TUNER_NXP_TDA18271] = {
		.name   = "NXP TDA18271",
		
	},
	[TUNER_SONY_BTF_PXN01Z] = {
		.name   = "Sony BTF-Pxn01Z",
		.params = tuner_sony_btf_pxn01z_params,
		.count  = ARRAY_SIZE(tuner_sony_btf_pxn01z_params),
	},
	[TUNER_PHILIPS_FQ1236_MK5] = { 
		.name   = "Philips FQ1236 MK5",
		.params = tuner_philips_fq1236_mk5_params,
		.count  = ARRAY_SIZE(tuner_philips_fq1236_mk5_params),
	},
	[TUNER_TENA_TNF_5337] = { 
		.name   = "Tena TNF5337 MFD",
		.params = tuner_tena_tnf_5337_params,
		.count  = ARRAY_SIZE(tuner_tena_tnf_5337_params),
	},
	[TUNER_XC5000C] = { 
		.name   = "Xceive 5000C tuner",
		
	},
};
EXPORT_SYMBOL(tuners);

unsigned const int tuner_count = ARRAY_SIZE(tuners);
EXPORT_SYMBOL(tuner_count);

MODULE_DESCRIPTION("Simple tuner device type database");
MODULE_AUTHOR("Ralph Metzler, Gerd Knorr, Gunther Mayer");
MODULE_LICENSE("GPL");
