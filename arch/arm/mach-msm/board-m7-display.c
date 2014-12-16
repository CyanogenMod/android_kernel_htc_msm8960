/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/msm_ion.h>
#include <asm/mach-types.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/ion.h>
#include <mach/msm_bus_board.h>
#include <mach/panel_id.h>
#include <mach/debug_display.h>
#include "devices.h"
#include "board-m7.h"
#include <linux/mfd/pm8xxx/pm8921.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include <linux/i2c.h>
#include <mach/msm_xo.h>

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE (1920 * ALIGN(1080, 32) * 4 * 3)
#else
#define MSM_FB_PRIM_BUF_SIZE (1920 * ALIGN(1080, 32) * 4 * 2)
#endif

#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1920 * 1080 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};
struct msm_xo_voter *wa_xo;
static char ptype[60] = "PANEL type = ";
const size_t ptype_len = ( 60 - sizeof("PANEL type = "));

#define MIPI_NOVATEK_PANEL_NAME "mipi_cmd_novatek_qhd"
#define MIPI_RENESAS_PANEL_NAME "mipi_video_renesas_fiwvga"
#define MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME "mipi_video_toshiba_wsvga"
#define MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME "mipi_video_chimei_wxga"
#define HDMI_PANEL_NAME "hdmi_msm"
#define TVOUT_PANEL_NAME "tvout_msm"

static int m7_detect_panel(const char *name)
{
#if 0
	if (panel_type == PANEL_ID_DLX_SONY_RENESAS) {
		if (!strncmp(name, MIPI_RENESAS_PANEL_NAME,
			strnlen(MIPI_RENESAS_PANEL_NAME,
				PANEL_NAME_MAX_LEN))){
			PR_DISP_INFO("m7_%s\n", name);
			return 0;
		}
	} else if (panel_type == PANEL_ID_DLX_SHARP_RENESAS) {
		if (!strncmp(name, MIPI_RENESAS_PANEL_NAME,
			strnlen(MIPI_RENESAS_PANEL_NAME,
				PANEL_NAME_MAX_LEN))){
			PR_DISP_INFO("m7_%s\n", name);
			return 0;
		}
	}
#endif
	if (!strncmp(name, HDMI_PANEL_NAME,
		strnlen(HDMI_PANEL_NAME,
			PANEL_NAME_MAX_LEN)))
		return 0;

	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = m7_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name              = "msm_fb",
	.id                = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init m7_allocate_fb_region(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));
}

#define MDP_VSYNC_GPIO 0

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 577474560 * 2,
		.ib = 866211840 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 605122560 * 2,
		.ib = 756403200 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 660418560 * 2,
		.ib = 825523200 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 764098560 * 2,
		.ib = 955123200 * 2,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
	},
};

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};

static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
};
#endif

struct mdp_reg *mdp_gamma = NULL;
int mdp_gamma_count = 0;
struct mdp_reg mdp_gamma_jdi[] = {
        {0x94800, 0x000000, 0x0},
        {0x94804, 0x000100, 0x0},
        {0x94808, 0x010201, 0x0},
        {0x9480C, 0x020302, 0x0},
        {0x94810, 0x030403, 0x0},
        {0x94814, 0x040504, 0x0},
        {0x94818, 0x050605, 0x0},
        {0x9481C, 0x060706, 0x0},
        {0x94820, 0x070807, 0x0},
        {0x94824, 0x080908, 0x0},
        {0x94828, 0x090A09, 0x0},
        {0x9482C, 0x0A0B0A, 0x0},
        {0x94830, 0x0B0C0B, 0x0},
        {0x94834, 0x0B0D0C, 0x0},
        {0x94838, 0x0C0E0D, 0x0},
        {0x9483C, 0x0D0F0E, 0x0},
        {0x94840, 0x0E100F, 0x0},
        {0x94844, 0x0F1110, 0x0},
        {0x94848, 0x101210, 0x0},
        {0x9484C, 0x111311, 0x0},
        {0x94850, 0x121412, 0x0},
        {0x94854, 0x131513, 0x0},
        {0x94858, 0x141614, 0x0},
        {0x9485C, 0x151715, 0x0},
        {0x94860, 0x161816, 0x0},
        {0x94864, 0x161917, 0x0},
        {0x94868, 0x171A18, 0x0},
        {0x9486C, 0x181B19, 0x0},
        {0x94870, 0x191C1A, 0x0},
        {0x94874, 0x1A1D1B, 0x0},
        {0x94878, 0x1B1E1C, 0x0},
        {0x9487C, 0x1C1F1D, 0x0},
        {0x94880, 0x1D201E, 0x0},
        {0x94884, 0x1E211F, 0x0},
        {0x94888, 0x1F2220, 0x0},
        {0x9488C, 0x202320, 0x0},
        {0x94890, 0x212421, 0x0},
        {0x94894, 0x212522, 0x0},
        {0x94898, 0x222623, 0x0},
        {0x9489C, 0x232724, 0x0},
        {0x948A0, 0x242825, 0x0},
        {0x948A4, 0x252926, 0x0},
        {0x948A8, 0x262A27, 0x0},
        {0x948AC, 0x272B28, 0x0},
        {0x948B0, 0x282C29, 0x0},
        {0x948B4, 0x292D2A, 0x0},
        {0x948B8, 0x2A2E2B, 0x0},
        {0x948BC, 0x2B2F2C, 0x0},
        {0x948C0, 0x2C302D, 0x0},
        {0x948C4, 0x2C312E, 0x0},
        {0x948C8, 0x2D322F, 0x0},
        {0x948CC, 0x2E3330, 0x0},
        {0x948D0, 0x2F3430, 0x0},
        {0x948D4, 0x303531, 0x0},
        {0x948D8, 0x313632, 0x0},
        {0x948DC, 0x323733, 0x0},
        {0x948E0, 0x333834, 0x0},
        {0x948E4, 0x343935, 0x0},
        {0x948E8, 0x353A36, 0x0},
        {0x948EC, 0x363B37, 0x0},
        {0x948F0, 0x373C38, 0x0},
        {0x948F4, 0x373D39, 0x0},
        {0x948F8, 0x383E3A, 0x0},
        {0x948FC, 0x393F3B, 0x0},
        {0x94900, 0x3A403C, 0x0},
        {0x94904, 0x3B413D, 0x0},
        {0x94908, 0x3C423E, 0x0},
        {0x9490C, 0x3D433F, 0x0},
        {0x94910, 0x3E4440, 0x0},
        {0x94914, 0x3F4540, 0x0},
        {0x94918, 0x404641, 0x0},
        {0x9491C, 0x414742, 0x0},
        {0x94920, 0x424843, 0x0},
        {0x94924, 0x424944, 0x0},
        {0x94928, 0x434A45, 0x0},
        {0x9492C, 0x444B46, 0x0},
        {0x94930, 0x454C47, 0x0},
        {0x94934, 0x464D48, 0x0},
        {0x94938, 0x474E49, 0x0},
        {0x9493C, 0x484F4A, 0x0},
        {0x94940, 0x49504B, 0x0},
        {0x94944, 0x4A514C, 0x0},
        {0x94948, 0x4B524D, 0x0},
        {0x9494C, 0x4C534E, 0x0},
        {0x94950, 0x4D544F, 0x0},
        {0x94954, 0x4E5550, 0x0},
        {0x94958, 0x4E5650, 0x0},
        {0x9495C, 0x4F5751, 0x0},
        {0x94960, 0x505852, 0x0},
        {0x94964, 0x515953, 0x0},
        {0x94968, 0x525A54, 0x0},
        {0x9496C, 0x535B55, 0x0},
        {0x94970, 0x545C56, 0x0},
        {0x94974, 0x555D57, 0x0},
        {0x94978, 0x565E58, 0x0},
        {0x9497C, 0x575F59, 0x0},
        {0x94980, 0x58605A, 0x0},
        {0x94984, 0x59615B, 0x0},
        {0x94988, 0x59625C, 0x0},
        {0x9498C, 0x5A635D, 0x0},
        {0x94990, 0x5B645E, 0x0},
        {0x94994, 0x5C655F, 0x0},
        {0x94998, 0x5D6660, 0x0},
        {0x9499C, 0x5E6760, 0x0},
        {0x949A0, 0x5F6861, 0x0},
        {0x949A4, 0x606962, 0x0},
        {0x949A8, 0x616A63, 0x0},
        {0x949AC, 0x626B64, 0x0},
        {0x949B0, 0x636C65, 0x0},
        {0x949B4, 0x646D66, 0x0},
        {0x949B8, 0x646E67, 0x0},
        {0x949BC, 0x656F68, 0x0},
        {0x949C0, 0x667069, 0x0},
        {0x949C4, 0x67716A, 0x0},
        {0x949C8, 0x68726B, 0x0},
        {0x949CC, 0x69736C, 0x0},
        {0x949D0, 0x6A746D, 0x0},
        {0x949D4, 0x6B756E, 0x0},
        {0x949D8, 0x6C766F, 0x0},
        {0x949DC, 0x6D7770, 0x0},
        {0x949E0, 0x6E7870, 0x0},
        {0x949E4, 0x6F7971, 0x0},
        {0x949E8, 0x6F7A72, 0x0},
        {0x949EC, 0x707B73, 0x0},
        {0x949F0, 0x717C74, 0x0},
        {0x949F4, 0x727D75, 0x0},
        {0x949F8, 0x737E76, 0x0},
        {0x949FC, 0x747F77, 0x0},
        {0x94A00, 0x758078, 0x0},
        {0x94A04, 0x768179, 0x0},
        {0x94A08, 0x77827A, 0x0},
        {0x94A0C, 0x78837B, 0x0},
        {0x94A10, 0x79847C, 0x0},
        {0x94A14, 0x7A857D, 0x0},
        {0x94A18, 0x7A867E, 0x0},
        {0x94A1C, 0x7B877F, 0x0},
        {0x94A20, 0x7C8880, 0x0},
        {0x94A24, 0x7D8980, 0x0},
        {0x94A28, 0x7E8A81, 0x0},
        {0x94A2C, 0x7F8B82, 0x0},
        {0x94A30, 0x808C83, 0x0},
        {0x94A34, 0x818D84, 0x0},
        {0x94A38, 0x828E85, 0x0},
        {0x94A3C, 0x838F86, 0x0},
        {0x94A40, 0x849087, 0x0},
        {0x94A44, 0x859188, 0x0},
        {0x94A48, 0x859289, 0x0},
        {0x94A4C, 0x86938A, 0x0},
        {0x94A50, 0x87948B, 0x0},
        {0x94A54, 0x88958C, 0x0},
        {0x94A58, 0x89968D, 0x0},
        {0x94A5C, 0x8A978E, 0x0},
        {0x94A60, 0x8B988F, 0x0},
        {0x94A64, 0x8C9990, 0x0},
        {0x94A68, 0x8D9A90, 0x0},
        {0x94A6C, 0x8E9B91, 0x0},
        {0x94A70, 0x8F9C92, 0x0},
        {0x94A74, 0x909D93, 0x0},
        {0x94A78, 0x909E94, 0x0},
        {0x94A7C, 0x919F95, 0x0},
        {0x94A80, 0x92A096, 0x0},
        {0x94A84, 0x93A197, 0x0},
        {0x94A88, 0x94A298, 0x0},
        {0x94A8C, 0x95A399, 0x0},
        {0x94A90, 0x96A49A, 0x0},
        {0x94A94, 0x97A59B, 0x0},
        {0x94A98, 0x98A69C, 0x0},
        {0x94A9C, 0x99A79D, 0x0},
        {0x94AA0, 0x9AA89E, 0x0},
        {0x94AA4, 0x9BA99F, 0x0},
        {0x94AA8, 0x9CAAA0, 0x0},
        {0x94AAC, 0x9CABA0, 0x0},
        {0x94AB0, 0x9DACA1, 0x0},
        {0x94AB4, 0x9EADA2, 0x0},
        {0x94AB8, 0x9FAEA3, 0x0},
        {0x94ABC, 0xA0AFA4, 0x0},
        {0x94AC0, 0xA1B0A5, 0x0},
        {0x94AC4, 0xA2B1A6, 0x0},
        {0x94AC8, 0xA3B2A7, 0x0},
        {0x94ACC, 0xA4B3A8, 0x0},
        {0x94AD0, 0xA5B4A9, 0x0},
        {0x94AD4, 0xA6B5AA, 0x0},
        {0x94AD8, 0xA7B6AB, 0x0},
        {0x94ADC, 0xA7B7AC, 0x0},
        {0x94AE0, 0xA8B8AD, 0x0},
        {0x94AE4, 0xA9B9AE, 0x0},
        {0x94AE8, 0xAABAAF, 0x0},
        {0x94AEC, 0xABBBB0, 0x0},
        {0x94AF0, 0xACBCB0, 0x0},
        {0x94AF4, 0xADBDB1, 0x0},
        {0x94AF8, 0xAEBEB2, 0x0},
        {0x94AFC, 0xAFBFB3, 0x0},
        {0x94B00, 0xB0C0B4, 0x0},
        {0x94B04, 0xB1C1B5, 0x0},
        {0x94B08, 0xB2C2B6, 0x0},
        {0x94B0C, 0xB2C3B7, 0x0},
        {0x94B10, 0xB3C4B8, 0x0},
        {0x94B14, 0xB4C5B9, 0x0},
        {0x94B18, 0xB5C6BA, 0x0},
        {0x94B1C, 0xB6C7BB, 0x0},
        {0x94B20, 0xB7C8BC, 0x0},
        {0x94B24, 0xB8C9BD, 0x0},
        {0x94B28, 0xB9CABE, 0x0},
        {0x94B2C, 0xBACBBF, 0x0},
        {0x94B30, 0xBBCCC0, 0x0},
        {0x94B34, 0xBCCDC0, 0x0},
        {0x94B38, 0xBDCEC1, 0x0},
        {0x94B3C, 0xBDCFC2, 0x0},
        {0x94B40, 0xBED0C3, 0x0},
        {0x94B44, 0xBFD1C4, 0x0},
        {0x94B48, 0xC0D2C5, 0x0},
        {0x94B4C, 0xC1D3C6, 0x0},
        {0x94B50, 0xC2D4C7, 0x0},
        {0x94B54, 0xC3D5C8, 0x0},
        {0x94B58, 0xC4D6C9, 0x0},
        {0x94B5C, 0xC5D7CA, 0x0},
        {0x94B60, 0xC6D8CB, 0x0},
        {0x94B64, 0xC7D9CC, 0x0},
        {0x94B68, 0xC8DACD, 0x0},
        {0x94B6C, 0xC8DBCE, 0x0},
        {0x94B70, 0xC9DCCF, 0x0},
        {0x94B74, 0xCADDD0, 0x0},
        {0x94B78, 0xCBDED0, 0x0},
        {0x94B7C, 0xCCDFD1, 0x0},
        {0x94B80, 0xCDE0D2, 0x0},
        {0x94B84, 0xCEE1D3, 0x0},
        {0x94B88, 0xCFE2D4, 0x0},
        {0x94B8C, 0xD0E3D5, 0x0},
        {0x94B90, 0xD1E4D6, 0x0},
        {0x94B94, 0xD2E5D7, 0x0},
        {0x94B98, 0xD3E6D8, 0x0},
        {0x94B9C, 0xD3E7D9, 0x0},
        {0x94BA0, 0xD4E8DA, 0x0},
        {0x94BA4, 0xD5E9DB, 0x0},
        {0x94BA8, 0xD6EADC, 0x0},
        {0x94BAC, 0xD7EBDD, 0x0},
        {0x94BB0, 0xD8ECDE, 0x0},
        {0x94BB4, 0xD9EDDF, 0x0},
        {0x94BB8, 0xDAEEE0, 0x0},
        {0x94BBC, 0xDBEFE0, 0x0},
        {0x94BC0, 0xDCF0E1, 0x0},
        {0x94BC4, 0xDDF1E2, 0x0},
        {0x94BC8, 0xDEF2E3, 0x0},
        {0x94BCC, 0xDEF3E4, 0x0},
        {0x94BD0, 0xDFF4E5, 0x0},
        {0x94BD4, 0xE0F5E6, 0x0},
        {0x94BD8, 0xE1F6E7, 0x0},
        {0x94BDC, 0xE2F7E8, 0x0},
        {0x94BE0, 0xE3F8E9, 0x0},
        {0x94BE4, 0xE4F9EA, 0x0},
        {0x94BE8, 0xE5FAEB, 0x0},
        {0x94BEC, 0xE6FBEC, 0x0},
        {0x94BF0, 0xE7FCED, 0x0},
        {0x94BF4, 0xE8FDEE, 0x0},
        {0x94BF8, 0xE9FEEF, 0x0},
        {0x94BFC, 0xEAFFF0, 0x0},
        {0x90070, 0x0F, 0x0},
};

struct mdp_reg mdp_gamma_renesas[] = {
        {0x94800, 0x000000, 0x0},
        {0x94804, 0x010101, 0x0},
        {0x94808, 0x020202, 0x0},
        {0x9480C, 0x030303, 0x0},
        {0x94810, 0x040404, 0x0},
        {0x94814, 0x050505, 0x0},
        {0x94818, 0x060606, 0x0},
        {0x9481C, 0x070707, 0x0},
        {0x94820, 0x080808, 0x0},
        {0x94824, 0x090909, 0x0},
        {0x94828, 0x0A0A0A, 0x0},
        {0x9482C, 0x0B0B0B, 0x0},
        {0x94830, 0x0C0C0C, 0x0},
        {0x94834, 0x0D0D0D, 0x0},
        {0x94838, 0x0E0E0E, 0x0},
        {0x9483C, 0x0F0F0F, 0x0},
        {0x94840, 0x101010, 0x0},
        {0x94844, 0x111111, 0x0},
        {0x94848, 0x121212, 0x0},
        {0x9484C, 0x131313, 0x0},
        {0x94850, 0x141414, 0x0},
        {0x94854, 0x151515, 0x0},
        {0x94858, 0x161616, 0x0},
        {0x9485C, 0x171717, 0x0},
        {0x94860, 0x181818, 0x0},
        {0x94864, 0x191919, 0x0},
        {0x94868, 0x1A1A1A, 0x0},
        {0x9486C, 0x1B1B1B, 0x0},
        {0x94870, 0x1C1C1C, 0x0},
        {0x94874, 0x1D1D1D, 0x0},
        {0x94878, 0x1E1E1E, 0x0},
        {0x9487C, 0x1F1F1F, 0x0},
        {0x94880, 0x202020, 0x0},
        {0x94884, 0x212121, 0x0},
        {0x94888, 0x222222, 0x0},
        {0x9488C, 0x232323, 0x0},
        {0x94890, 0x242424, 0x0},
        {0x94894, 0x252525, 0x0},
        {0x94898, 0x262626, 0x0},
        {0x9489C, 0x272727, 0x0},
        {0x948A0, 0x282828, 0x0},
        {0x948A4, 0x292929, 0x0},
        {0x948A8, 0x2A2A2A, 0x0},
        {0x948AC, 0x2B2B2B, 0x0},
        {0x948B0, 0x2C2C2C, 0x0},
        {0x948B4, 0x2D2D2D, 0x0},
        {0x948B8, 0x2E2E2E, 0x0},
        {0x948BC, 0x2F2F2F, 0x0},
        {0x948C0, 0x303030, 0x0},
        {0x948C4, 0x313131, 0x0},
        {0x948C8, 0x323232, 0x0},
        {0x948CC, 0x333333, 0x0},
        {0x948D0, 0x343434, 0x0},
        {0x948D4, 0x353535, 0x0},
        {0x948D8, 0x363636, 0x0},
        {0x948DC, 0x373737, 0x0},
        {0x948E0, 0x383838, 0x0},
        {0x948E4, 0x393939, 0x0},
        {0x948E8, 0x3A3A3A, 0x0},
        {0x948EC, 0x3B3B3B, 0x0},
        {0x948F0, 0x3C3C3C, 0x0},
        {0x948F4, 0x3D3D3D, 0x0},
        {0x948F8, 0x3E3E3E, 0x0},
        {0x948FC, 0x3F3F3F, 0x0},
        {0x94900, 0x404040, 0x0},
        {0x94904, 0x414141, 0x0},
        {0x94908, 0x424242, 0x0},
        {0x9490C, 0x434343, 0x0},
        {0x94910, 0x444444, 0x0},
        {0x94914, 0x454545, 0x0},
        {0x94918, 0x464646, 0x0},
        {0x9491C, 0x474747, 0x0},
        {0x94920, 0x484848, 0x0},
        {0x94924, 0x494949, 0x0},
        {0x94928, 0x4A4A4A, 0x0},
        {0x9492C, 0x4B4B4B, 0x0},
        {0x94930, 0x4C4C4C, 0x0},
        {0x94934, 0x4D4D4D, 0x0},
        {0x94938, 0x4E4E4E, 0x0},
        {0x9493C, 0x4F4F4F, 0x0},
        {0x94940, 0x505050, 0x0},
        {0x94944, 0x515151, 0x0},
        {0x94948, 0x525252, 0x0},
        {0x9494C, 0x535353, 0x0},
        {0x94950, 0x545454, 0x0},
        {0x94954, 0x555555, 0x0},
        {0x94958, 0x565656, 0x0},
        {0x9495C, 0x575757, 0x0},
        {0x94960, 0x585858, 0x0},
        {0x94964, 0x595959, 0x0},
        {0x94968, 0x5A5A5A, 0x0},
        {0x9496C, 0x5B5B5B, 0x0},
        {0x94970, 0x5C5C5C, 0x0},
        {0x94974, 0x5D5D5D, 0x0},
        {0x94978, 0x5E5E5E, 0x0},
        {0x9497C, 0x5F5F5F, 0x0},
        {0x94980, 0x606060, 0x0},
        {0x94984, 0x616161, 0x0},
        {0x94988, 0x626262, 0x0},
        {0x9498C, 0x636363, 0x0},
        {0x94990, 0x646464, 0x0},
        {0x94994, 0x656565, 0x0},
        {0x94998, 0x666666, 0x0},
        {0x9499C, 0x676767, 0x0},
        {0x949A0, 0x686868, 0x0},
        {0x949A4, 0x696969, 0x0},
        {0x949A8, 0x6A6A6A, 0x0},
        {0x949AC, 0x6B6B6B, 0x0},
        {0x949B0, 0x6C6C6C, 0x0},
        {0x949B4, 0x6D6D6D, 0x0},
        {0x949B8, 0x6E6E6E, 0x0},
        {0x949BC, 0x6F6F6F, 0x0},
        {0x949C0, 0x707070, 0x0},
        {0x949C4, 0x717171, 0x0},
        {0x949C8, 0x727272, 0x0},
        {0x949CC, 0x737373, 0x0},
        {0x949D0, 0x747474, 0x0},
        {0x949D4, 0x757575, 0x0},
        {0x949D8, 0x767676, 0x0},
        {0x949DC, 0x777777, 0x0},
        {0x949E0, 0x787878, 0x0},
        {0x949E4, 0x797979, 0x0},
        {0x949E8, 0x7A7A7A, 0x0},
        {0x949EC, 0x7B7B7B, 0x0},
        {0x949F0, 0x7C7C7C, 0x0},
        {0x949F4, 0x7D7D7D, 0x0},
        {0x949F8, 0x7E7E7E, 0x0},
        {0x949FC, 0x7F7F7F, 0x0},
        {0x94A00, 0x808080, 0x0},
        {0x94A04, 0x818181, 0x0},
        {0x94A08, 0x828282, 0x0},
        {0x94A0C, 0x838383, 0x0},
        {0x94A10, 0x848484, 0x0},
        {0x94A14, 0x858585, 0x0},
        {0x94A18, 0x868686, 0x0},
        {0x94A1C, 0x878787, 0x0},
        {0x94A20, 0x888788, 0x0},
        {0x94A24, 0x898889, 0x0},
        {0x94A28, 0x8A898A, 0x0},
        {0x94A2C, 0x8B8A8B, 0x0},
        {0x94A30, 0x8C8B8C, 0x0},
        {0x94A34, 0x8D8C8D, 0x0},
        {0x94A38, 0x8E8D8E, 0x0},
        {0x94A3C, 0x8F8E8F, 0x0},
        {0x94A40, 0x908F90, 0x0},
        {0x94A44, 0x919091, 0x0},
        {0x94A48, 0x929192, 0x0},
        {0x94A4C, 0x939293, 0x0},
        {0x94A50, 0x949394, 0x0},
        {0x94A54, 0x959495, 0x0},
        {0x94A58, 0x969596, 0x0},
        {0x94A5C, 0x979697, 0x0},
        {0x94A60, 0x989698, 0x0},
        {0x94A64, 0x999799, 0x0},
        {0x94A68, 0x9A989A, 0x0},
        {0x94A6C, 0x9B999B, 0x0},
        {0x94A70, 0x9C9A9C, 0x0},
        {0x94A74, 0x9D9B9D, 0x0},
        {0x94A78, 0x9E9C9E, 0x0},
        {0x94A7C, 0x9F9D9F, 0x0},
        {0x94A80, 0xA09EA0, 0x0},
        {0x94A84, 0xA19FA1, 0x0},
        {0x94A88, 0xA2A0A2, 0x0},
        {0x94A8C, 0xA3A1A3, 0x0},
        {0x94A90, 0xA4A2A4, 0x0},
        {0x94A94, 0xA5A3A5, 0x0},
        {0x94A98, 0xA6A4A6, 0x0},
        {0x94A9C, 0xA7A5A7, 0x0},
        {0x94AA0, 0xA8A5A8, 0x0},
        {0x94AA4, 0xA9A6A9, 0x0},
        {0x94AA8, 0xAAA7AA, 0x0},
        {0x94AAC, 0xABA8AB, 0x0},
        {0x94AB0, 0xACA9AC, 0x0},
        {0x94AB4, 0xADAAAD, 0x0},
        {0x94AB8, 0xAEABAE, 0x0},
        {0x94ABC, 0xAFACAF, 0x0},
        {0x94AC0, 0xB0ADB0, 0x0},
        {0x94AC4, 0xB1AEB1, 0x0},
        {0x94AC8, 0xB2AFB2, 0x0},
        {0x94ACC, 0xB3B0B3, 0x0},
        {0x94AD0, 0xB4B1B4, 0x0},
        {0x94AD4, 0xB5B2B5, 0x0},
        {0x94AD8, 0xB6B3B6, 0x0},
        {0x94ADC, 0xB7B4B7, 0x0},
        {0x94AE0, 0xB8B4B8, 0x0},
        {0x94AE4, 0xB9B5B9, 0x0},
        {0x94AE8, 0xBAB6BA, 0x0},
        {0x94AEC, 0xBBB7BB, 0x0},
        {0x94AF0, 0xBCB8BC, 0x0},
        {0x94AF4, 0xBDB9BD, 0x0},
        {0x94AF8, 0xBEBABE, 0x0},
        {0x94AFC, 0xBFBBBF, 0x0},
        {0x94B00, 0xC0BCC0, 0x0},
        {0x94B04, 0xC1BDC1, 0x0},
        {0x94B08, 0xC2BEC2, 0x0},
        {0x94B0C, 0xC3BFC3, 0x0},
        {0x94B10, 0xC4C0C4, 0x0},
        {0x94B14, 0xC5C1C5, 0x0},
        {0x94B18, 0xC6C2C6, 0x0},
        {0x94B1C, 0xC7C3C7, 0x0},
        {0x94B20, 0xC8C3C8, 0x0},
        {0x94B24, 0xC9C4C9, 0x0},
        {0x94B28, 0xCAC5CA, 0x0},
        {0x94B2C, 0xCBC6CB, 0x0},
        {0x94B30, 0xCCC7CC, 0x0},
        {0x94B34, 0xCDC8CD, 0x0},
        {0x94B38, 0xCEC9CE, 0x0},
        {0x94B3C, 0xCFCACF, 0x0},
        {0x94B40, 0xD0CBD0, 0x0},
        {0x94B44, 0xD1CCD1, 0x0},
        {0x94B48, 0xD2CDD2, 0x0},
        {0x94B4C, 0xD3CED3, 0x0},
        {0x94B50, 0xD4CFD4, 0x0},
        {0x94B54, 0xD5D0D5, 0x0},
        {0x94B58, 0xD6D1D6, 0x0},
        {0x94B5C, 0xD7D2D7, 0x0},
        {0x94B60, 0xD8D2D8, 0x0},
        {0x94B64, 0xD9D3D9, 0x0},
        {0x94B68, 0xDAD4DA, 0x0},
        {0x94B6C, 0xDBD5DB, 0x0},
        {0x94B70, 0xDCD6DC, 0x0},
        {0x94B74, 0xDDD7DD, 0x0},
        {0x94B78, 0xDED8DE, 0x0},
        {0x94B7C, 0xDFD9DF, 0x0},
        {0x94B80, 0xE0DAE0, 0x0},
        {0x94B84, 0xE1DBE1, 0x0},
        {0x94B88, 0xE2DCE2, 0x0},
        {0x94B8C, 0xE3DDE3, 0x0},
        {0x94B90, 0xE4DEE4, 0x0},
        {0x94B94, 0xE5DFE5, 0x0},
        {0x94B98, 0xE6E0E6, 0x0},
        {0x94B9C, 0xE7E1E7, 0x0},
        {0x94BA0, 0xE8E1E8, 0x0},
        {0x94BA4, 0xE9E2E9, 0x0},
        {0x94BA8, 0xEAE3EA, 0x0},
        {0x94BAC, 0xEBE4EB, 0x0},
        {0x94BB0, 0xECE5EC, 0x0},
        {0x94BB4, 0xEDE6ED, 0x0},
        {0x94BB8, 0xEEE7EE, 0x0},
        {0x94BBC, 0xEFE8EF, 0x0},
        {0x94BC0, 0xF0E9F0, 0x0},
        {0x94BC4, 0xF1EAF1, 0x0},
        {0x94BC8, 0xF2EBF2, 0x0},
        {0x94BCC, 0xF3ECF3, 0x0},
        {0x94BD0, 0xF4EDF4, 0x0},
        {0x94BD4, 0xF5EEF5, 0x0},
        {0x94BD8, 0xF6EFF6, 0x0},
        {0x94BDC, 0xF7F0F7, 0x0},
        {0x94BE0, 0xF8F0F8, 0x0},
        {0x94BE4, 0xF9F1F9, 0x0},
        {0x94BE8, 0xFAF2FA, 0x0},
        {0x94BEC, 0xFBF3FB, 0x0},
        {0x94BF0, 0xFCF4FC, 0x0},
        {0x94BF4, 0xFDF5FD, 0x0},
        {0x94BF8, 0xFEF6FE, 0x0},
        {0x94BFC, 0xFFF7FF, 0x0},
        {0x90070, 0x0F, 0x0},

};

int m7_mdp_gamma(void)
{
	if (mdp_gamma == NULL)
		return 0;

	mdp_color_enhancement(mdp_gamma, mdp_gamma_count);
	return 0;
}

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_44,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.cont_splash_enabled = 0x00,
	.mdp_gamma = m7_mdp_gamma,
	.mdp_iommu_split_domain = 1,
	.mdp_max_clk = 266667000,
};

static char wfd_check_mdp_iommu_split_domain(void)
{
    return mdp_pdata.mdp_iommu_split_domain;
}

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
static struct msm_wfd_platform_data wfd_pdata = {
    .wfd_check_mdp_iommu_split = wfd_check_mdp_iommu_split_domain,
};

static struct platform_device wfd_panel_device = {
    .name = "wfd_panel",
    .id = 0,
    .dev.platform_data = NULL,
};

static struct platform_device wfd_device = {
    .name          = "msm_wfd",
    .id            = -1,
    .dev.platform_data = &wfd_pdata,
};
#endif
void __init m7_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov0_wb_size;
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov1_wb_size;
#endif
}
static int first_init = 1;
uint32_t cfg_panel_te_active[] = {GPIO_CFG(LCD_TE, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)};
uint32_t cfg_panel_te_sleep[] = {GPIO_CFG(LCD_TE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)};

static int mipi_dsi_panel_power(int on)
{
	static bool dsi_power_on = false;
	static struct regulator *reg_lvs5, *reg_l2;
	static int gpio36, gpio37;
	int rc;

	PR_DISP_INFO("%s: on=%d\n", __func__, on);

	if (!dsi_power_on) {
		reg_lvs5 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_vddio");
		if (IS_ERR_OR_NULL(reg_lvs5)) {
			pr_err("could not get 8921_lvs5, rc = %ld\n",
				PTR_ERR(reg_lvs5));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_pll_vdda");
		if (IS_ERR_OR_NULL(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		gpio36 = PM8921_GPIO_PM_TO_SYS(V_LCM_N5V_EN); 
		rc = gpio_request(gpio36, "lcd_5v-");
		if (rc) {
			pr_err("request lcd_5v- failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio37 = PM8921_GPIO_PM_TO_SYS(V_LCM_P5V_EN); 
		rc = gpio_request(gpio37, "lcd_5v+");
		if (rc) {
			pr_err("request lcd_5v+ failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio_tlmm_config(GPIO_CFG(LCD_RST, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		dsi_power_on = true;
	}

	if (on) {
		if (!first_init) {
			rc = regulator_set_optimum_mode(reg_l2, 100000);
			if (rc < 0) {
				pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
				return -EINVAL;
			}
			rc = regulator_enable(reg_l2);
			if (rc) {
				pr_err("enable l2 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			rc = regulator_enable(reg_lvs5);
			if (rc) {
				pr_err("enable lvs5 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			hr_msleep(1);
			gpio_set_value_cansleep(gpio37, 1);
			hr_msleep(2);	
			gpio_set_value_cansleep(gpio36, 1);
			hr_msleep(7);	
			gpio_set_value(LCD_RST, 1);

			
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_ON);
			hr_msleep(10);
			
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_OFF);
		} else {
			
			rc = regulator_enable(reg_lvs5);
			if (rc) {
				pr_err("enable lvs5 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			rc = regulator_set_optimum_mode(reg_l2, 100000);
			if (rc < 0) {
				pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
				return -EINVAL;
			}
			rc = regulator_enable(reg_l2);
			if (rc) {
				pr_err("enable l2 failed, rc=%d\n", rc);
				return -ENODEV;
			}
			
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_ON);
			msleep(10);
			msm_xo_mode_vote(wa_xo, MSM_XO_MODE_OFF);
		}
#if 1
		rc = gpio_tlmm_config(cfg_panel_te_active[0], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n", __func__,
					cfg_panel_te_active[0], rc);
		}
#endif
	} else {
#if 1
		rc = gpio_tlmm_config(cfg_panel_te_sleep[0], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n", __func__,
					cfg_panel_te_sleep[0], rc);
		}
#endif
		gpio_tlmm_config(GPIO_CFG(BL_HW_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(BL_HW_EN, 0);

		gpio_set_value(LCD_RST, 0);
		hr_msleep(3);	

		gpio_set_value_cansleep(gpio36, 0);
		hr_msleep(2);
		gpio_set_value_cansleep(gpio37, 0);

		hr_msleep(8);	
		rc = regulator_disable(reg_lvs5);
		if (rc) {
			pr_err("disable reg_lvs5 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
	}

	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	
	.dsi_power_save = mipi_dsi_panel_power,
};

static struct mipi_dsi_panel_platform_data *mipi_m7_pdata;

static struct dsi_buf m7_panel_tx_buf;
static struct dsi_buf m7_panel_rx_buf;
static struct dsi_cmd_desc *video_on_cmds = NULL;
static struct dsi_cmd_desc *display_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static struct dsi_cmd_desc *backlight_cmds = NULL;
static struct dsi_cmd_desc *cmd_on_cmds = NULL;
static struct dsi_cmd_desc *dim_on_cmds = NULL;
static struct dsi_cmd_desc *dim_off_cmds = NULL;
static struct dsi_cmd_desc *color_en_on_cmds = NULL;
static struct dsi_cmd_desc *color_en_off_cmds = NULL;
static struct dsi_cmd_desc **sre_ctrl_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_UI_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_Video_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_Camera_cmds = NULL;
static int backlight_cmds_count = 0;
static int video_on_cmds_count = 0;
static int display_on_cmds_count = 0;
static int display_off_cmds_count = 0;
static int cmd_on_cmds_count = 0;
static int dim_on_cmds_count = 0;
static int dim_off_cmds_count = 0;
static int color_en_on_cmds_count = 0;
static int color_en_off_cmds_count = 0;
static int sre_ctrl_cmds_count = 0;
static int set_cabc_UI_cmds_count = 0;
static int set_cabc_Video_cmds_count = 0;
static int set_cabc_Camera_cmds_count = 0;

#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
static int cabc_mode = 0;
static int cur_cabc_mode = 0;
static struct mutex set_cabc_mutex;
void m7_set_cabc (struct msm_fb_data_type *mfd, int mode);
#endif
static unsigned int pwm_min = 6;
static unsigned int pwm_default = 81 ;
static unsigned int pwm_max = 255;
static atomic_t lcd_backlight_off;

#define CABC_DIMMING_SWITCH

static char enter_sleep[2] = {0x10, 0x00}; 
static char exit_sleep[2] = {0x11, 0x00}; 
static char display_off[2] = {0x28, 0x00}; 
static char display_on[2] = {0x29, 0x00}; 
static char nop[2] = {0x00, 0x00};
static char CABC[2] = {0x55, 0x00};
static char jdi_samsung_CABC[2] = {0x55, 0x03};

static char samsung_password_l2[3] = { 0xF0, 0x5A, 0x5A}; 
static char samsung_MIE_ctrl1[4] = {0xC0, 0x40, 0x10, 0x80};
static char samsung_MIE_ctrl2[2] = {0xC2, 0x0F}; 

#ifdef CABC_DIMMING_SWITCH
static char dsi_dim_on[] = {0x53, 0x2C};
static char dsi_dim_off[] = {0x53, 0x24};
static char dsi_sharp_cmd_dim_on[] = {0x53, 0x0C};
static char dsi_sharp_cmd_dim_off[] = {0x53, 0x04};

static struct dsi_cmd_desc jdi_renesas_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_on), dsi_dim_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc jdi_renesas_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_off), dsi_dim_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc samsung_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_on), dsi_dim_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc samsung_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_off), dsi_dim_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc renesas_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_on), dsi_dim_on},
};

static struct dsi_cmd_desc renesas_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_off), dsi_dim_off},
};

static struct dsi_cmd_desc sharp_cmd_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_sharp_cmd_dim_on), dsi_sharp_cmd_dim_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc sharp_cmd_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_sharp_cmd_dim_off), dsi_sharp_cmd_dim_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};
#endif
#if 0
static char samsung_display_ctrl[2] = { 0xB1, 0x00}; 

static char samsung_display_ctrl_interface[5] = { 0xB2, 0x00, 0x02, 0x04, 0x48 };

static char samsung_ctrl_BRR[2] = { 0xB3, 0x00 }; 
static char samsung_crtl_Hsync[2] = { 0xB5, 0x00 }; 
static char samsung_ctrl_GoutL[33] = { 
		0xB7, 0x00, 0x00, 0x00,
		0x00, 0x03, 0x09, 0x04,
		0x06, 0x05, 0x07, 0x08,
		0x00, 0x0F, 0x0E, 0x0D,
		0x0C, 0x0B, 0x0A, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00};

static char samsung_ctrl_GoutR[33] = { 
		0xB8, 0x00, 0x00, 0x00,
		0x00, 0x10, 0x16, 0x11,
		0x13, 0x12, 0x14, 0x15,
		0x00, 0x1C, 0x1B, 0x1A,
		0x19, 0x18, 0x17, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00};

static char samsung_ctrl_bl_mode[2] = {0xC1, 0x03}; 
static char samsung_MIE_ctrl2[2] = {0xC2, 0x00}; 

static char samsung_ctrl_bl[4] = {0xC3, 0x7C, 0x40, 0x22};

static char samsung_ctrl_SHE[2] = { 0xE8, 0x00}; 
static char samsung_ctrl_SAE[2] = { 0xE9, 0x00}; 

static char samsung_ctrl_source[17] = { 
		0xF2, 0x0C, 0x03, 0x03,
		0x06, 0x04, 0x00, 0x25,
		0x00, 0x26, 0x00, 0xD1,
		0x00, 0xD1, 0x0A, 0x0A,
		0x00};

static char samsung_ctrl_power[19] = { 
		0xF3, 0x99, 0x22, 0x0A,
		0x0A, 0x00, 0x09, 0x0C,
		0x00, 0x1B, 0x00, 0x1B,
		0x00, 0x2E, 0x08, 0x33,
		0x33, 0x53, 0x11};

static char samsung_ctrl_panel[22] = { 
		0xF5, 0x06, 0x00, 0x0D,
		0x21, 0x0A, 0x00, 0x17,
		0x11, 0x09, 0x00, 0x06,
		0x00, 0x0D, 0x21, 0x0A,
		0x00, 0x17, 0x11, 0x09,
		0x00, 0x01};

#endif
static char samsung_ctrl_source[17] = { 
		0xF2, 0x0C, 0x03, 0x73,
		0x06, 0x04, 0x00, 0x25,
		0x00, 0x26, 0x00, 0xD1,
		0x00, 0xD1, 0x00, 0x00,
		0x00};
static char samsung_ctrl_bl[4] = {0xC3, 0x63, 0x40, 0x01}; 
static char samsung_ctrl_positive_gamma[70] = { 
		0xFA, 0x00, 0x3F, 0x20,
		0x19, 0x25, 0x24, 0x27,
		0x19, 0x19, 0x13, 0x00,
		0x08, 0x0E, 0x0F, 0x14,
		0x15, 0x1F, 0x25, 0x2A,
		0x2B, 0x2A, 0x20, 0x3C,
		0x00, 0x3F, 0x20, 0x19,
		0x25, 0x24, 0x27, 0x19,
		0x19, 0x13, 0x00, 0x08,
		0x0E, 0x0F, 0x14, 0x15,
		0x1F, 0x25, 0x2A, 0x2B,
		0x2A, 0x20, 0x3C, 0x00,
		0x3F, 0x20, 0x19, 0x25,
		0x24, 0x27, 0x19, 0x19,
		0x13, 0x00, 0x08, 0x0E,
		0x0F, 0x14, 0x15, 0x1F,
		0x25, 0x2A, 0x2B, 0x2A,
		0x20, 0x3C};

static char samsung_ctrl_negative_gamma[70] = { 
		0xFB, 0x00, 0x3F, 0x20,
		0x19, 0x25, 0x24, 0x27,
		0x19, 0x19, 0x13, 0x00,
		0x08, 0x0E, 0x0F, 0x14,
		0x15, 0x1F, 0x25, 0x2A,
		0x2B, 0x2A, 0x20, 0x3C,
		0x00, 0x3F, 0x20, 0x19,
		0x25, 0x24, 0x27, 0x19,
		0x19, 0x13, 0x00, 0x08,
		0x0E, 0x0F, 0x14, 0x15,
		0x1F, 0x25, 0x2A, 0x2B,
		0x2A, 0x20, 0x3C, 0x00,
		0x3F, 0x20, 0x19, 0x25,
		0x24, 0x27, 0x19, 0x19,
		0x13, 0x00, 0x08, 0x0E,
		0x0F, 0x14, 0x15, 0x1F,
		0x25, 0x2A, 0x2B, 0x2A,
		0x20, 0x3C};
static char samsung_password_l3[3] = { 0xFC, 0x5A, 0x5A }; 

static char samsung_cmd_test[5] = { 0xFF, 0x00, 0x00, 0x00, 0x20}; 

static char samsung_panel_exit_sleep[2] = {0x11, 0x00}; 
#ifdef CABC_DIMMING_SWITCH
static char samsung_bl_ctrl[2] = {0x53, 0x24};
#else
static char samsung_bl_ctrl[2] = {0x53, 0x2C};
#endif
static char samsung_ctrl_brightness[2] = {0x51, 0xFF};
static char samsung_enable_te[2] = {0x35, 0x00};

static char samsung_set_column_address[5] = { 0x2A, 0x00, 0x00, 0x04, 0x37 }; 

static char samsung_set_page_address[5] = { 0x2B, 0x00, 0x00, 0x07, 0x7F }; 
static char samsung_panel_display_on[2] = {0x29, 0x00}; 
static char samsung_display_off[2] = {0x28, 0x00}; 
static char samsung_enter_sleep[2] = {0x10, 0x00}; 

static char samsung_deep_standby_off[2] = {0xB0, 0x01}; 
static char SAE[2] = {0xE9, 0x12};
static char samsung_swwr_mipi_speed[4] = {0xE4, 0x00, 0x04, 0x00};
static char samsung_swwr_kinky_gamma[17] = {0xF2, 0x0C, 0x03, 0x03, 0x06, 0x04, 0x00, 0x25, 0x00, 0x26, 0x00, 0xD1, 0x00, 0xD1, 0x00, 0x0A, 0x00};
static char samsung_password_l2_close[3] = { 0xF0, 0xA5, 0xA5}; 
static char samsung_password_l3_close[3] = { 0xFC, 0xA5, 0xA5}; 
static char  Oscillator_Bias_Current[4] = { 0xFD, 0x56, 0x08, 0x00}; 
static char samsung_ctrl_positive_gamma_c2_1[70] = { 
		0xFA, 0x1E, 0x38, 0x0C,
		0x0C, 0x12, 0x14, 0x16,
		0x17, 0x1A, 0x1A, 0x19,
		0x14, 0x10, 0x0D, 0x10,
		0x13, 0x1D, 0x20, 0x20,
		0x21, 0x26, 0x27, 0x36,
		0x0F, 0x3C, 0x12, 0x15,
		0x1E, 0x21, 0x24, 0x24,
		0x26, 0x24, 0x23, 0x1C,
		0x15, 0x11, 0x13, 0x16,
		0x1E, 0x21, 0x21, 0x21,
		0x26, 0x27, 0x36, 0x00,
		0x3F, 0x13, 0x18, 0x22,
		0x27, 0x29, 0x2A, 0x2B,
		0x2A, 0x29, 0x23, 0x1B,
		0x16, 0x18, 0x19, 0x1F,
		0x22, 0x23, 0x24, 0x2A,
		0x2D, 0x37};
static char samsung_ctrl_negative_gamma_c2_1[70] = { 
		0xFB, 0x1E, 0x38, 0x0C,
		0x0C, 0x12, 0x14, 0x16,
		0x17, 0x1A, 0x1A, 0x19,
		0x14, 0x10, 0x0D, 0x10,
		0x13, 0x1D, 0x20, 0x20,
		0x21, 0x26, 0x27, 0x36,
		0x0F, 0x3C, 0x12, 0x15,
		0x1E, 0x21, 0x24, 0x24,
		0x26, 0x24, 0x23, 0x1C,
		0x15, 0x11, 0x13, 0x16,
		0x1E, 0x21, 0x21, 0x21,
		0x26, 0x27, 0x36, 0x00,
		0x3F, 0x13, 0x18, 0x22,
		0x27, 0x29, 0x2A, 0x2B,
		0x2A, 0x29, 0x23, 0x1B,
		0x16, 0x18, 0x19, 0x1F,
		0x22, 0x23, 0x24, 0x2A,
		0x2D, 0x37};

static char BCSAVE[] = { 
		0xCD, 0x80, 0xB3, 0x67,
		0x1C, 0x78, 0x37, 0x00,
		0x10, 0x73, 0x41, 0x99,
		0x10, 0x00, 0x00};

static char TMF[] = { 
		0xCE, 0x33, 0x1C, 0x0D,
		0x20, 0x14, 0x00, 0x16,
		0x23, 0x18, 0x2C, 0x16,
		0x00, 0x00};

static struct dsi_cmd_desc samsung_cmd_backlight_cmds_nop[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(samsung_ctrl_brightness), samsung_ctrl_brightness},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc samsung_cmd_backlight_cmds[] = {
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(samsung_ctrl_brightness), samsung_ctrl_brightness},
};

static struct dsi_cmd_desc samsung_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(samsung_panel_display_on), samsung_panel_display_on},
};

static struct dsi_cmd_desc samsung_jdi_panel_cmd_mode_cmds[] = {
#if 0
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(samsung_display_ctrl), samsung_display_ctrl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_display_ctrl_interface), samsung_display_ctrl_interface},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(samsung_ctrl_BRR), samsung_ctrl_BRR},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(samsung_crtl_Hsync), samsung_crtl_Hsync},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_GoutL), samsung_ctrl_GoutL},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_GoutR), samsung_ctrl_GoutR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(samsung_ctrl_bl_mode),samsung_ctrl_bl_mode},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl2), samsung_MIE_ctrl2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(samsung_ctrl_SHE), samsung_ctrl_SHE},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(samsung_ctrl_SAE), samsung_ctrl_SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_source), samsung_ctrl_source},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_power), samsung_ctrl_power},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_panel), samsung_ctrl_panel},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_positive_gamma), samsung_ctrl_positive_gamma},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_negative_gamma), samsung_ctrl_negative_gamma},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},
#endif
	{DTYPE_DCS_WRITE,  1, 0, 0, 120, sizeof(samsung_panel_exit_sleep), samsung_panel_exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(SAE),SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_positive_gamma), samsung_ctrl_positive_gamma},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_negative_gamma), samsung_ctrl_negative_gamma},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_swwr_mipi_speed), samsung_swwr_mipi_speed},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_swwr_kinky_gamma), samsung_swwr_kinky_gamma},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3_close), samsung_password_l3_close},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(samsung_set_column_address), samsung_set_column_address},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(samsung_set_page_address), samsung_set_page_address},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_bl_ctrl), samsung_bl_ctrl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_enable_te), samsung_enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc samsung_jdi_panel_cmd_mode_cmds_c2_nvm[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(samsung_panel_exit_sleep), samsung_panel_exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SAE), SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3_close), samsung_password_l3_close},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_bl_ctrl), samsung_bl_ctrl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_enable_te), samsung_enable_te},
	
};

static struct dsi_cmd_desc samsung_jdi_panel_cmd_mode_cmds_c2_1[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Oscillator_Bias_Current), Oscillator_Bias_Current},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(samsung_panel_exit_sleep), samsung_panel_exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl2), samsung_MIE_ctrl2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BCSAVE), BCSAVE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(TMF), TMF},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SAE), SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_source), samsung_ctrl_source},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_positive_gamma_c2_1), samsung_ctrl_positive_gamma_c2_1},
        {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_negative_gamma_c2_1), samsung_ctrl_negative_gamma_c2_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3_close), samsung_password_l3_close},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_bl_ctrl), samsung_bl_ctrl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(jdi_samsung_CABC), jdi_samsung_CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_enable_te), samsung_enable_te},
	
};
static struct dsi_cmd_desc samsung_jdi_panel_cmd_mode_cmds_c2_2[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Oscillator_Bias_Current), Oscillator_Bias_Current},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(samsung_panel_exit_sleep), samsung_panel_exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl2), samsung_MIE_ctrl2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BCSAVE), BCSAVE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(TMF), TMF},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SAE), SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_source), samsung_ctrl_source},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3_close), samsung_password_l3_close},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_bl_ctrl), samsung_bl_ctrl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(jdi_samsung_CABC), jdi_samsung_CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_enable_te), samsung_enable_te},
	
};

static struct dsi_cmd_desc samsung_display_off_cmds[] = {
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(samsung_display_off), samsung_display_off},
	{DTYPE_DCS_WRITE,  1, 0, 0, 48, sizeof(samsung_enter_sleep), samsung_enter_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE,  1, 0, 0, 0, sizeof(samsung_deep_standby_off), samsung_deep_standby_off},
};

static char write_display_brightness[3]= {0x51, 0x0F, 0xFF};
static char write_control_display[2] = {0x53, 0x24}; 
static struct dsi_cmd_desc renesas_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_display_brightness), write_display_brightness},
};
static struct dsi_cmd_desc renesas_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};
static char interface_setting_0[2] = {0xB0, 0x04};

static char Color_enhancement[33]= {
	0xCA, 0x01, 0x02, 0xA4,
	0xA4, 0xB8, 0xB4, 0xB0,
	0xA4, 0x3F, 0x28, 0x05,
	0xB9, 0x90, 0x70, 0x01,
	0xFF, 0x05, 0xF8, 0x0C,
	0x0C, 0x0C, 0x0C, 0x13,
	0x13, 0xF0, 0x20, 0x10,
	0x10, 0x10, 0x10, 0x10,
	0x10};
static char m7_Color_enhancement[33]= {
        0xCA, 0x01, 0x02, 0x9A,
        0xA4, 0xB8, 0xB4, 0xB0,
        0xA4, 0x08, 0x28, 0x05,
        0x87, 0xB0, 0x50, 0x01,
        0xFF, 0x05, 0xF8, 0x0C,
        0x0C, 0x50, 0x40, 0x13,
        0x13, 0xF0, 0x08, 0x10,
        0x10, 0x3F, 0x3F, 0x3F,
        0x3F};
static char Outline_Sharpening_Control[3]= {
	0xDD, 0x11, 0xA1};

static char BackLight_Control_6[8]= {
	0xCE, 0x00, 0x07, 0x00,
	0xC1, 0x24, 0xB2, 0x02};
static char BackLight_Control_6_28kHz[8]= {
       0xCE, 0x00, 0x01, 0x00,
       0xC1, 0xF4, 0xB2, 0x02}; 
static char Manufacture_Command_setting[4] = {0xD6, 0x01};
static char hsync_output[4] = {0xC3, 0x01, 0x00, 0x10};
static char protect_on[4] = {0xB0, 0x03};
static char TE_OUT[4] = {0x35, 0x00};
static char deep_standby_off[2] = {0xB1, 0x01};

static char unlock[] = {0xB0, 0x00};
static char display_brightness[] = {0x51, 0xFF};
#ifdef CABC_DIMMING_SWITCH
static char led_pwm_en[] = {0x53, 0x04};
#else
static char led_pwm_en[] = {0x53, 0x0C};
#endif
static char enable_te[] = {0x35, 0x00};
static char Source_Timing_Setting[23]= {
	0xC4, 0x70, 0x0C, 0x0C,
	0x55, 0x55, 0x00, 0x00,
	0x00, 0x00, 0x05, 0x05,
	0x00, 0x0C, 0x0C, 0x55,
	0x55, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x05};
static char lock[] = {0xB0, 0x03};
static char Write_Content_Adaptive_Brightness_Control[2] = {0x55, 0x42};
static char common_setting[] = {
       0xCE, 0x6C, 0x40, 0x43,
       0x49, 0x55, 0x62, 0x71,
       0x82, 0x94, 0xA8, 0xB9,
       0xCB, 0xDB, 0xE9, 0xF5,
       0xFC, 0xFF, 0x04, 0xD3, 
       0x00, 0x00, 0x54, 0x24};

static char cabc_still[] = {0xB9, 0x03, 0x82, 0x3C, 0x10, 0x3C, 0x87};
static char cabc_movie[] = {0xBA, 0x03, 0x78, 0x64, 0x10, 0x64, 0xB4};
static char SRE_Manual_0[] = {0xBB, 0x01, 0x00, 0x00};

static char blue_shift_adjust_1[] = {
	0xC7, 0x01, 0x0B, 0x12,
	0x1B, 0x2A, 0x3A, 0x45,
	0x56, 0x3A, 0x42, 0x4E,
	0x5B, 0x64, 0x6C, 0x75,
	0x01, 0x0B, 0x12, 0x1A,
	0x29, 0x37, 0x41, 0x52,
	0x36, 0x3F, 0x4C, 0x59,
	0x62, 0x6A, 0x74};

static char blue_shift_adjust_2[] = {
	0xC8, 0x01, 0x00, 0xF4,
	0x00, 0x00, 0xFC, 0x00,
	0x00, 0xF7, 0x00, 0x00,
	0xFC, 0x00, 0x00, 0xFF,
	0x00, 0x00, 0xFC, 0x0F};

static struct dsi_cmd_desc sharp_cmd_backlight_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_brightness), display_brightness},
};

static struct dsi_cmd_desc sharp_renesas_cmd_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(common_setting), common_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_movie), cabc_movie},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SRE_Manual_0), SRE_Manual_0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(m7_Color_enhancement), m7_Color_enhancement},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(Write_Content_Adaptive_Brightness_Control), Write_Content_Adaptive_Brightness_Control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm_en), led_pwm_en},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(blue_shift_adjust_1), blue_shift_adjust_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(blue_shift_adjust_2), blue_shift_adjust_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Source_Timing_Setting), Source_Timing_Setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(lock), lock},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(nop), nop},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
};
static struct dsi_cmd_desc m7_sharp_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Color_enhancement), Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Outline_Sharpening_Control), Outline_Sharpening_Control},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BackLight_Control_6_28kHz), BackLight_Control_6_28kHz},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(TE_OUT), TE_OUT},
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(exit_sleep), exit_sleep},

};

static struct dsi_cmd_desc sharp_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Color_enhancement), Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Outline_Sharpening_Control), Outline_Sharpening_Control},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(TE_OUT), TE_OUT},
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},

};

static struct dsi_cmd_desc sony_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hsync_output), hsync_output},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Color_enhancement), Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Outline_Sharpening_Control), Outline_Sharpening_Control},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(protect_on), protect_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(TE_OUT), TE_OUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 20,
		sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 48, sizeof(enter_sleep), enter_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(deep_standby_off), deep_standby_off},
};

static char unlock_command[2] = {0xB0, 0x04}; 
static char lock_command[2] = {0xB0, 0x03}; 
static struct dsi_cmd_desc jdi_renesas_cmd_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(common_setting), common_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_movie), cabc_movie},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SRE_Manual_0), SRE_Manual_0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(m7_Color_enhancement), m7_Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(lock_command), lock_command},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(Write_Content_Adaptive_Brightness_Control), Write_Content_Adaptive_Brightness_Control},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
};

static struct dsi_cmd_desc jdi_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 48, sizeof(enter_sleep), enter_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(deep_standby_off), deep_standby_off}
};

static int resume_blk = 0;
static struct i2c_client *blk_pwm_client;
static struct dcs_cmd_req cmdreq;
static int pwmic_ver;

static int m7_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;
	if (!first_init) {
		if (mipi->mode == DSI_VIDEO_MODE) {
			cmdreq.cmds = video_on_cmds;
			cmdreq.cmds_cnt = video_on_cmds_count;
		} else {
			cmdreq.cmds = cmd_on_cmds;
			cmdreq.cmds_cnt = cmd_on_cmds_count;
		}
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);
	}
	first_init = 0;

	PR_DISP_INFO("%s, %s, PWM A%d\n", __func__, ptype, pwmic_ver);
	return 0;
}

static int m7_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	resume_blk = 1;

	PR_DISP_INFO("%s\n", __func__);
	return 0;
}
static int __devinit m7_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_m7_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	PR_DISP_INFO("%s\n", __func__);
	return 0;
}
static void m7_display_on(struct msm_fb_data_type *mfd)
{
	
	if (panel_type == PANEL_ID_DLXJ_SHARP_RENESAS ||
		panel_type == PANEL_ID_DLXJ_SONY_RENESAS ||
		panel_type == PANEL_ID_M7_SHARP_RENESAS)
		msleep(120);

	cmdreq.cmds = display_on_cmds;
	cmdreq.cmds_cnt = display_on_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		cmdreq.flags |= CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

static void m7_display_off(struct msm_fb_data_type *mfd)
{
	cmdreq.cmds = display_off_cmds;
	cmdreq.cmds_cnt = display_off_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		cmdreq.flags |= CMD_CLK_CTRL;

	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	PR_DISP_INFO("%s\n", __func__);
}

#ifdef CABC_DIMMING_SWITCH
static void m7_dim_on(struct msm_fb_data_type *mfd)
{
	if (atomic_read(&lcd_backlight_off)) {
		PR_DISP_DEBUG("%s: backlight is off. Skip dimming setting\n", __func__);
		return;
	}

	if (dim_on_cmds == NULL)
		return;

	PR_DISP_DEBUG("%s\n", __func__);

	cmdreq.cmds = dim_on_cmds;
	cmdreq.cmds_cnt = dim_on_cmds_count;

	cmdreq.flags = CMD_REQ_COMMIT;
	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		cmdreq.flags |= CMD_CLK_CTRL;

	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
}
#endif


#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char pwm_value;
static int blk_low = 0;

static unsigned char m7_shrink_pwm(int val)
{
	unsigned char shrink_br = BRI_SETTING_MAX;
	if(pwmic_ver >= 2)
		pwm_min = 6;
	else
		pwm_min = 11;

	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN)) {
		shrink_br = pwm_min;
	} else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
		shrink_br = (val - BRI_SETTING_MIN) * (pwm_default - pwm_min) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + pwm_min;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
		shrink_br = (val - BRI_SETTING_DEF) * (pwm_max - pwm_default) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + pwm_default;
	} else if (val > BRI_SETTING_MAX)
		shrink_br = pwm_max;

	pwm_value = shrink_br; 

	PR_DISP_INFO("brightness orig=%d, transformed=%d\n", val, shrink_br);

	return shrink_br;
}

void pwmic_config(unsigned char* index, unsigned char* value, int count)
{
	int i, rc;

	for(i = 0; i < count; ++i) {
		rc = i2c_smbus_write_byte_data(blk_pwm_client, index[i], value[i]);
		if (rc)
			pr_err("i2c write fail\n");
	}
}

unsigned char idx[5] = {0x50, 0x01, 0x02, 0x05, 0x00};
unsigned char val[5] = {0x02, 0x09, 0x78, 0x14, 0x04};
unsigned char idx0[1] = {0x03};
unsigned char val0[1] = {0xFF};
unsigned char idx1[5] = {0x00, 0x01, 0x02, 0x03, 0x05};
unsigned char val1[5] = {0x04, 0x09, 0x78, 0xff, 0x14};
unsigned char val2[5] = {0x14, 0x08, 0x78, 0xff, 0x14};
unsigned char idx2[6] = {0x00, 0x01, 0x03, 0x03, 0x03, 0x03};
unsigned char idx3[6] = {0x00, 0x03, 0x03, 0x03, 0x03, 0x01};
unsigned char val3[6] = {0x14, 0x09, 0x50, 0xA0, 0xE0, 0xFF};
unsigned char val4[6] = {0x14, 0xF0, 0xA0, 0x50, 0x11, 0x08};

static void m7_set_backlight(struct msm_fb_data_type *mfd)
{
	int rc;

	if ((panel_type == PANEL_ID_M7_JDI_SAMSUNG) ||
		(panel_type == PANEL_ID_M7_JDI_SAMSUNG_C2) ||
		(panel_type == PANEL_ID_M7_JDI_SAMSUNG_C2_1) ||
		(panel_type == PANEL_ID_M7_JDI_SAMSUNG_C2_2) ||
		(panel_type == PANEL_ID_M7_JDI_RENESAS))
		samsung_ctrl_brightness[1] = m7_shrink_pwm((unsigned char)(mfd->bl_level));
	else if (panel_type == PANEL_ID_M7_SHARP_RENESAS_C1)
		display_brightness[1] = m7_shrink_pwm((unsigned char)(mfd->bl_level));
	else
		write_display_brightness[2] = m7_shrink_pwm((unsigned char)(mfd->bl_level));

	if(pwmic_ver >= 2) { 
		if (resume_blk) {
			resume_blk = 0;

			gpio_tlmm_config(GPIO_CFG(BL_HW_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(BL_HW_EN, 1);
			msleep(1);
			pwmic_config(idx, val, sizeof(idx));
			msleep(1);
			pwmic_config(idx0, val0, sizeof(idx0));
		}
	} else {
		if (resume_blk) {
			resume_blk = 0;
			gpio_tlmm_config(GPIO_CFG(BL_HW_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(BL_HW_EN, 1);
			msleep(1);

			if (pwm_value >= 21 ) {
				pwmic_config(idx1, val1, sizeof(idx1));
				blk_low = 0;
			} else {
				val2[3] = pwm_value-5;
				pwmic_config(idx1, val2, sizeof(idx1));
				blk_low = 1;
			}
		}

		if (pwm_value >= 21 ) {
			if ( blk_low == 1) {
				pwmic_config(idx2, val3, sizeof(idx2));
				blk_low = 0;
				PR_DISP_INFO("bl >= 21\n");
			}
		} else if ((pwm_value > 0)&&(pwm_value < 21)) {
			if ( blk_low == 0) {
				pwmic_config(idx3, val4, sizeof(idx3));
				blk_low = 1;
				PR_DISP_INFO("bl < 21\n");
			}
			rc = i2c_smbus_write_byte_data(blk_pwm_client, 0x03, (pwm_value - 5));
			if (rc)
				pr_err("i2c write fail\n");
		}
	}

#ifdef CABC_DIMMING_SWITCH
        
        if (samsung_ctrl_brightness[1] == 0 || display_brightness[1] == 0 || write_display_brightness[2] == 0) {
                atomic_set(&lcd_backlight_off, 1);
		cmdreq.cmds = dim_off_cmds;
		cmdreq.cmds_cnt = dim_off_cmds_count;
		cmdreq.flags = CMD_REQ_COMMIT;
		if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
			cmdreq.flags |= CMD_CLK_CTRL;

		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);
        } else
                atomic_set(&lcd_backlight_off, 0);
#endif
	cmdreq.cmds = backlight_cmds;
	cmdreq.cmds_cnt = backlight_cmds_count;
	cmdreq.flags = CMD_REQ_COMMIT;
	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		cmdreq.flags |= CMD_CLK_CTRL;

	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	
	if (cabc_mode == 3) {
		m7_set_cabc(mfd, cabc_mode);
	}
#endif
	if ((mfd->bl_level) == 0) {
		gpio_tlmm_config(GPIO_CFG(BL_HW_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(BL_HW_EN, 0);
		resume_blk = 1;
	}

	return;
}
static char SAE_on[2] = {0xE9, 0x12};
static char SAE_off[2] = {0xE9, 0x00};
static struct dsi_cmd_desc samsung_color_enhance_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(SAE_on),SAE_on},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2_close), samsung_password_l2_close},
};
static struct dsi_cmd_desc samsung_color_enhance_off_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(SAE_off),SAE_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2_close), samsung_password_l2_close},
};

static char renesas_color_en_on[2]= {0xCA, 0x01};
static char renesas_color_en_off[2]= {0xCA, 0x00};
static struct dsi_cmd_desc sharp_renesas_color_enhance_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_on), renesas_color_en_on},
};
static struct dsi_cmd_desc sharp_renesas_color_enhance_off_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_off), renesas_color_en_off},
};

static struct dsi_cmd_desc sharp_renesas_c1_color_enhance_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_on), renesas_color_en_on},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_c1_color_enhance_off_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_off), renesas_color_en_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};

static void m7_color_enhance(struct msm_fb_data_type *mfd, int on)
{
	if (color_en_on_cmds == NULL || color_en_off_cmds == NULL)
		return;

	if (on) {
		cmdreq.cmds = color_en_on_cmds;
		cmdreq.cmds_cnt = color_en_on_cmds_count;
		cmdreq.flags = CMD_REQ_COMMIT;
		if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
			cmdreq.flags |= CMD_CLK_CTRL;

		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);

		PR_DISP_INFO("color enhance on\n");
	} else {
		cmdreq.cmds = color_en_off_cmds;
		cmdreq.cmds_cnt = color_en_off_cmds_count;
		cmdreq.flags = CMD_REQ_COMMIT;
		if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
			cmdreq.flags |= CMD_CLK_CTRL;

		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);

		PR_DISP_INFO("color enhance off\n");
	}
}

static char SRE_Manual1[] = {0xBB, 0x01, 0x00, 0x00};
static char SRE_Manual2[] = {0xBB, 0x01, 0x03, 0x02};
static char SRE_Manual3[] = {0xBB, 0x01, 0x08, 0x05};
static char SRE_Manual4[] = {0xBB, 0x01, 0x13, 0x08};
static char SRE_Manual5[] = {0xBB, 0x01, 0x1C, 0x0E};
static char SRE_Manual6[] = {0xBB, 0x01, 0x25, 0x10};
static char SRE_Manual7[] = {0xBB, 0x01, 0x38, 0x18};
static char SRE_Manual8[] = {0xBB, 0x01, 0x5D, 0x28};
static char SRE_Manual9[] = {0xBB, 0x01, 0x83, 0x38};
static char SRE_Manual10[] = {0xBB, 0x01, 0xA8, 0x48};

static struct dsi_cmd_desc sharp_renesas_sre1_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual1), SRE_Manual1},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre2_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual2), SRE_Manual2},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre3_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual3), SRE_Manual3},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre4_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual4), SRE_Manual4},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre5_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual5), SRE_Manual5},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre6_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual6), SRE_Manual6},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre7_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual7), SRE_Manual7},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre8_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual8), SRE_Manual8},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre9_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual9), SRE_Manual9},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre10_ctrl_cmds[] = {
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual10), SRE_Manual10},
	   {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};

static struct dsi_cmd_desc *sharp_renesas_sre_ctrl_cmds[10] = {
		sharp_renesas_sre1_ctrl_cmds,
		sharp_renesas_sre2_ctrl_cmds,
		sharp_renesas_sre3_ctrl_cmds,
		sharp_renesas_sre4_ctrl_cmds,
		sharp_renesas_sre5_ctrl_cmds,
		sharp_renesas_sre6_ctrl_cmds,
		sharp_renesas_sre7_ctrl_cmds,
		sharp_renesas_sre8_ctrl_cmds,
		sharp_renesas_sre9_ctrl_cmds,
		sharp_renesas_sre10_ctrl_cmds,
};
static void m7_sre_ctrl(struct msm_fb_data_type *mfd, unsigned long level)
{
	static long prev_level = 0, current_stage = 0, prev_stage = 0, tmp_stage = 0;

	if (prev_level != level) {

		prev_level = level;

		if (level >= 0 && level < 8000) {
			current_stage = 1;
		} else if (level >= 8000 && level < 16000) {
			current_stage = 2;
		} else if (level >= 16000 && level < 24000) {
			current_stage = 3;
		} else if (level >= 24000 && level < 32000) {
			current_stage = 4;
		} else if (level >= 32000 && level < 40000) {
			current_stage = 5;
		} else if (level >= 40000 && level < 48000) {
			current_stage = 6;
		} else if (level >= 48000 && level < 56000) {
			current_stage = 7;
		} else if (level >= 56000 && level < 65000) {
			current_stage = 8;
		} else if (level >= 65000 && level < 65500) {
			current_stage = 9;
		} else if (level >= 65500 && level < 65536) {
			current_stage = 10;
		} else {
			current_stage = 11;
			PR_DISP_INFO("out of range of ADC, set it to 11 as default\n");
		}

		if ( prev_stage == current_stage)
			return;
		tmp_stage = prev_stage;
		prev_stage = current_stage;

		if (sre_ctrl_cmds == NULL)
			return;

		if (current_stage == 1) {
			cmdreq.cmds = sre_ctrl_cmds[0];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 2) {
			cmdreq.cmds = sre_ctrl_cmds[1];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 3) {
			cmdreq.cmds = sre_ctrl_cmds[2];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 4) {
			cmdreq.cmds = sre_ctrl_cmds[3];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 5) {
			cmdreq.cmds = sre_ctrl_cmds[4];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 6) {
			cmdreq.cmds = sre_ctrl_cmds[5];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 7) {
			cmdreq.cmds = sre_ctrl_cmds[6];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 8) {
			cmdreq.cmds = sre_ctrl_cmds[7];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 9) {
			cmdreq.cmds = sre_ctrl_cmds[8];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else if (current_stage == 10) {
			cmdreq.cmds = sre_ctrl_cmds[9];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		} else {
			cmdreq.cmds = sre_ctrl_cmds[0];
			cmdreq.cmds_cnt = sre_ctrl_cmds_count;
		}

		cmdreq.flags = CMD_REQ_COMMIT;
		if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
			cmdreq.flags |= CMD_CLK_CTRL;

		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);

		PR_DISP_INFO("SRE level %lu prev_stage %lu current_stage %lu\n", level, tmp_stage, current_stage);
	}
}

#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
static char sharp_renesas_cabc_UI[2] = {0x55, 0x42};
static char sharp_renesas_cabc_Video[2] = {0x55, 0x43};
static struct dsi_cmd_desc sharp_renesas_set_cabc_UI_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(sharp_renesas_cabc_UI), sharp_renesas_cabc_UI},
};
static struct dsi_cmd_desc sharp_renesas_set_cabc_Video_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(sharp_renesas_cabc_Video), sharp_renesas_cabc_Video},
};
static char samsung_MIE_ctrl1_cabc_UI[4] = {0xC0, 0x40, 0x10, 0x80};
static char BCSAVE_cabc_UI[] = {
		0xCD, 0x80, 0xB3, 0x67,
		0x1C, 0x78, 0x37, 0x00,
		0x10, 0x73, 0x41, 0x99,
		0x10, 0x00, 0x00};
static char TMF_cabc_UI[] = {
		0xCE, 0x33, 0x1C, 0x0D,
		0x20, 0x14, 0x00, 0x16,
		0x23, 0x18, 0x2C, 0x16,
		0x00, 0x00};
static char samsung_MIE_ctrl1_cabc_Video[4] = {0xC0, 0x80, 0x10, 0x80};
static char BCSAVE_cabc_Video[] = {
		0xCD, 0x80, 0x99, 0x67,
		0x1C, 0x78, 0x37, 0x00,
		0x10, 0x73, 0x41, 0x99,
		0x10, 0x00, 0x00};
static char TMF_cabc_Video[] = {
		0xCE, 0x2C, 0x1C, 0x0D,
		0x20, 0x14, 0x00, 0x16,
		0x23, 0x18, 0x2C, 0x16,
		0x00, 0x00};
static struct dsi_cmd_desc jdi_samsung_set_cabc_UI_cmds[] = {
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1_cabc_UI), samsung_MIE_ctrl1_cabc_UI},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BCSAVE_cabc_UI), BCSAVE_cabc_UI},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(TMF_cabc_UI), TMF_cabc_UI},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
};
static struct dsi_cmd_desc jdi_samsung_set_cabc_Video_cmds[] = {
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1_cabc_Video), samsung_MIE_ctrl1_cabc_Video},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BCSAVE_cabc_Video), BCSAVE_cabc_Video},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(TMF_cabc_Video), TMF_cabc_Video},
       {DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
};

void m7_set_cabc (struct msm_fb_data_type *mfd, int mode)
{
	int req_cabc_zoe_mode = 2;

	if (set_cabc_UI_cmds == NULL || set_cabc_Video_cmds == NULL || set_cabc_Camera_cmds == NULL)
		return;

	mutex_lock(&set_cabc_mutex);
	cabc_mode = mode;

	if (mode == 1) {
               cmdreq.cmds = set_cabc_UI_cmds;
               cmdreq.cmds_cnt = set_cabc_UI_cmds_count;
	} else if (mode == 2) {
               cmdreq.cmds = set_cabc_Video_cmds;
               cmdreq.cmds_cnt = set_cabc_Video_cmds_count;
	} else if (mode == 3) {
		if (pwm_value < 168 && cur_cabc_mode == 3) {
			req_cabc_zoe_mode = 2;
			cmdreq.cmds = set_cabc_Video_cmds;
			cmdreq.cmds_cnt = set_cabc_Video_cmds_count;
		} else if (pwm_value >= 168 && cur_cabc_mode == 3) {
			req_cabc_zoe_mode = 3;
			cmdreq.cmds = set_cabc_Camera_cmds;
			cmdreq.cmds_cnt = set_cabc_Camera_cmds_count;
		} else if (pwm_value == 255) {
			req_cabc_zoe_mode = 3;
			cmdreq.cmds = set_cabc_Camera_cmds;
			cmdreq.cmds_cnt = set_cabc_Camera_cmds_count;
		} else {
			req_cabc_zoe_mode = 2;
			cmdreq.cmds = set_cabc_Video_cmds;
			cmdreq.cmds_cnt = set_cabc_Video_cmds_count;
		}

		if (cur_cabc_mode != req_cabc_zoe_mode) {
			cmdreq.flags = CMD_REQ_COMMIT;
			if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
				cmdreq.flags |= CMD_CLK_CTRL;

			cmdreq.rlen = 0;
			cmdreq.cb = NULL;
			mipi_dsi_cmdlist_put(&cmdreq);

			cur_cabc_mode = req_cabc_zoe_mode;
			PR_DISP_INFO("set_cabc_zoe mode = %d\n", cur_cabc_mode);
		}
		mutex_unlock(&set_cabc_mutex);
		return;
	} else {
		mutex_unlock(&set_cabc_mutex);
		return;
	}

	cmdreq.flags = CMD_REQ_COMMIT;
	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		cmdreq.flags |= CMD_CLK_CTRL;

	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	cur_cabc_mode = mode;
	mutex_unlock(&set_cabc_mutex);
	PR_DISP_INFO("set_cabc mode = %d\n", mode);
}
#endif

static struct platform_driver this_driver = {
	.probe  = m7_lcd_probe,
	.driver = {
		.name   = "mipi_m7",
	},
};

static struct msm_fb_panel_data m7_panel_data = {
	.on	= m7_lcd_on,
	.off	= m7_lcd_off,
	.set_backlight = m7_set_backlight,
	.display_on = m7_display_on,
	.display_off = m7_display_off,
	.color_enhance = m7_color_enhance,
#ifdef CABC_DIMMING_SWITCH
	.dimming_on = m7_dim_on,
#endif
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	.set_cabc = m7_set_cabc,
#endif
	.sre_ctrl = m7_sre_ctrl,
};

static struct msm_panel_info pinfo;
static int ch_used[3] = {0};

static int mipi_m7_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;
	pr_err("%s\n", __func__);

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_m7", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	m7_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &m7_panel_data,
		sizeof(m7_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}
	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	
	
	{0x03, 0x08, 0x05, 0x00, 0x20},
	
	{0xDD, 0x51, 0x27, 0x00, 0x6E, 0x74, 0x2C,
	0x55, 0x3E, 0x3, 0x4, 0xA0},
	
	{0x5F, 0x00, 0x00, 0x10},
	
	{0xFF, 0x00, 0x06, 0x00},
	
	{0x00, 0x38, 0x32, 0xDA, 0x00, 0x10, 0x0F, 0x61,
	0x41, 0x0F, 0x01,
	0x00, 0x1A, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02 },
};

static struct mipi_dsi_phy_ctrl dsi_jdi_cmd_mode_phy_db = {
	
        {0x03, 0x08, 0x05, 0x00, 0x20},
        
	{0xD7, 0x34, 0x23, 0x00, 0x63, 0x6A, 0x28, 0x37, 0x3C, 0x03, 0x04},
        
        {0x5F, 0x00, 0x00, 0x10},
        
        {0xFF, 0x00, 0x06, 0x00},
        
	{0x00, 0xA8, 0x30, 0xCA, 0x00, 0x20, 0x0F, 0x62, 0x70, 0x88, 0x99, 0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};

static int __init mipi_cmd_jdi_renesas_init(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 58;
	pinfo.height = 103;
	pinfo.camera_backlight = 183;

	pinfo.lcdc.h_back_porch = 27;
	pinfo.lcdc.h_front_porch = 38;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = pinfo.lcdc.v_back_porch;
	pinfo.lcd.v_front_porch = pinfo.lcdc.v_front_porch;
	pinfo.lcd.v_pulse_width = pinfo.lcdc.v_pulse_width;

	pinfo.lcdc.border_clr = 0;      
	pinfo.lcdc.underflow_clr = 0xff;        
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 830000000;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000; 

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;

	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x3;
	pinfo.mipi.t_clk_pre = 0x2B;
	pinfo.mipi.stream = 0;  
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; 
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;

	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_jdi_cmd_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 5;

	ret = mipi_m7_device_register(&pinfo, MIPI_DSI_PRIM,
		MIPI_DSI_PANEL_FWVGA_PT);

	strncat(ptype, "PANEL_ID_M7_JDI_RENESAS", ptype_len);
	cmd_on_cmds = jdi_renesas_cmd_on_cmds;
	cmd_on_cmds_count = ARRAY_SIZE(jdi_renesas_cmd_on_cmds);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
	display_off_cmds = jdi_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(jdi_display_off_cmds);
	backlight_cmds = samsung_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
	dim_on_cmds = jdi_renesas_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(jdi_renesas_dim_on_cmds);
	dim_off_cmds = jdi_renesas_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(jdi_renesas_dim_off_cmds);
	color_en_on_cmds = sharp_renesas_c1_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_on_cmds);
	color_en_off_cmds = sharp_renesas_c1_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_off_cmds);
	sre_ctrl_cmds = sharp_renesas_sre_ctrl_cmds;
	sre_ctrl_cmds_count = ARRAY_SIZE(sharp_renesas_sre1_ctrl_cmds);
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	set_cabc_UI_cmds = sharp_renesas_set_cabc_UI_cmds;
	set_cabc_UI_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_UI_cmds);
	set_cabc_Video_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Video_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
	set_cabc_Camera_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Camera_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
#endif
	mdp_gamma = mdp_gamma_renesas;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_renesas);

	pwm_min = 6;
	pwm_default = 69;
	pwm_max = 255;

	PR_DISP_INFO("%s\n", __func__);

	return ret;
}

static int __init mipi_cmd_sharp_init(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
        pinfo.width = 58;
        pinfo.height = 103;
	pinfo.camera_backlight = 183;

	pinfo.lcdc.h_back_porch = 27;
	pinfo.lcdc.h_front_porch = 38;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = pinfo.lcdc.v_back_porch;
	pinfo.lcd.v_front_porch = pinfo.lcdc.v_front_porch;
	pinfo.lcd.v_pulse_width = pinfo.lcdc.v_pulse_width;

	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 830000000;

	
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000; 

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;

	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x3;
	pinfo.mipi.t_clk_pre = 0x2B;
	pinfo.mipi.stream = 0;	
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; 
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;

	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_jdi_cmd_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 5;

	ret = mipi_m7_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_FWVGA_PT);

	strncat(ptype, "PANEL_ID_M7_SHARP_RENESAS_CMD", ptype_len);
	cmd_on_cmds = sharp_renesas_cmd_on_cmds;
	cmd_on_cmds_count = ARRAY_SIZE(sharp_renesas_cmd_on_cmds);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
	display_off_cmds = sharp_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sharp_display_off_cmds);
	backlight_cmds = sharp_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(sharp_cmd_backlight_cmds);
	dim_on_cmds = sharp_cmd_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(sharp_cmd_dim_on_cmds);
	dim_off_cmds = sharp_cmd_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(sharp_cmd_dim_off_cmds);
	color_en_on_cmds = sharp_renesas_c1_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_on_cmds);
	color_en_off_cmds = sharp_renesas_c1_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_off_cmds);
	sre_ctrl_cmds = sharp_renesas_sre_ctrl_cmds;
	sre_ctrl_cmds_count = ARRAY_SIZE(sharp_renesas_sre1_ctrl_cmds);
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	set_cabc_UI_cmds = sharp_renesas_set_cabc_UI_cmds;
	set_cabc_UI_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_UI_cmds);
	set_cabc_Video_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Video_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
	set_cabc_Camera_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Camera_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
#endif
	mdp_gamma = mdp_gamma_renesas;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_renesas);

	pwm_min = 6;
	pwm_default = 69;
	pwm_max = 255;

	PR_DISP_INFO("%s\n", __func__);
	return ret;

}

static int __init mipi_video_sharp_init(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 61;
	pinfo.height = 110;
	pinfo.camera_backlight = 176;

	pinfo.lcdc.h_back_porch = 58;
	pinfo.lcdc.h_front_porch = 100;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = 4;
	pinfo.lcd.v_front_porch = 4;
	pinfo.lcd.v_pulse_width = 2;

	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 860000000;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x05;
	pinfo.mipi.t_clk_pre = 0x2D;
	pinfo.mipi.stream = 0; 
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 5;

	ret = mipi_m7_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_FWVGA_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	if (panel_type == PANEL_ID_DLXJ_SHARP_RENESAS) {
		strncat(ptype, "PANEL_ID_DLXJ_SHARP_RENESAS", ptype_len);
		video_on_cmds = sharp_video_on_cmds;
		video_on_cmds_count = ARRAY_SIZE(sharp_video_on_cmds);
	} else {
		strncat(ptype, "PANEL_ID_M7_SHARP_RENESAS", ptype_len);
		video_on_cmds = m7_sharp_video_on_cmds;
		video_on_cmds_count = ARRAY_SIZE(m7_sharp_video_on_cmds);

	}
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
	display_off_cmds = sharp_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sharp_display_off_cmds);
	backlight_cmds = renesas_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(renesas_cmd_backlight_cmds);
	dim_on_cmds = renesas_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(renesas_dim_on_cmds);
	dim_off_cmds = renesas_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(renesas_dim_off_cmds);
	color_en_on_cmds = sharp_renesas_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(sharp_renesas_color_enhance_on_cmds);
	color_en_off_cmds = sharp_renesas_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(sharp_renesas_color_enhance_off_cmds);

	pwm_min = 13;
	pwm_default = 82;
	pwm_max = 255;

	PR_DISP_INFO("%s\n", __func__);
	return ret;
}

static int __init mipi_video_sony_init(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 61;
	pinfo.height = 110;
	pinfo.camera_backlight = 176;

	pinfo.lcdc.h_back_porch = 58;
	pinfo.lcdc.h_front_porch = 100;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 3;
	pinfo.lcdc.v_front_porch = 3;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = 3;
	pinfo.lcd.v_front_porch = 3;
	pinfo.lcd.v_pulse_width = 2;

	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 860000000;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x05;
	pinfo.mipi.t_clk_pre = 0x2D;
	pinfo.mipi.stream = 0; 
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 5;

	ret = mipi_m7_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_FWVGA_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	video_on_cmds = sony_video_on_cmds;
	video_on_cmds_count = ARRAY_SIZE(sony_video_on_cmds);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
	display_off_cmds = sony_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	backlight_cmds = renesas_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(renesas_cmd_backlight_cmds);
	dim_on_cmds = renesas_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(renesas_dim_on_cmds);
	dim_off_cmds = renesas_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(renesas_dim_off_cmds);
	color_en_on_cmds = NULL;
	color_en_on_cmds_count = 0;
	color_en_off_cmds = NULL;
	color_en_off_cmds_count = 0;

	pwm_min = 13;
	pwm_default = 82;
	pwm_max = 255;

	return ret;
}

static int __init mipi_command_samsung_init(void)
{
	int ret;

	pinfo.xres = 1080;
	pinfo.yres = 1920;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
        pinfo.width = 58;
        pinfo.height = 103;
	pinfo.camera_backlight = 183;

	pinfo.lcdc.h_back_porch = 27;
	pinfo.lcdc.h_front_porch = 38;
	pinfo.lcdc.h_pulse_width = 10;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 4;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcd.v_back_porch = pinfo.lcdc.v_back_porch;
	pinfo.lcd.v_front_porch = pinfo.lcdc.v_front_porch;
	pinfo.lcd.v_pulse_width = pinfo.lcdc.v_pulse_width;

	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 830000000;

	
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000; 

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;

	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x3;
	pinfo.mipi.t_clk_pre = 0x2B;
	pinfo.mipi.stream = 0;	
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; 
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;
	pinfo.mipi.esc_byte_ratio = 5;

	pinfo.mipi.frame_rate = 60;
	
	pinfo.lcdc.no_set_tear = 1;

	pinfo.mipi.dsi_phy_db = &dsi_jdi_cmd_mode_phy_db;

#ifdef CONFIG_FB_MSM_ESD_WORKAROUND
	m7_panel_data.esd_workaround = true;
#endif

	ret = mipi_m7_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_FWVGA_PT);

	if(panel_type == PANEL_ID_M7_JDI_SAMSUNG) {
		strncat(ptype, "PANEL_ID_M7_JDI_SAMSUNG",  ptype_len);
		cmd_on_cmds = samsung_jdi_panel_cmd_mode_cmds;
		cmd_on_cmds_count = ARRAY_SIZE(samsung_jdi_panel_cmd_mode_cmds);
		backlight_cmds = samsung_cmd_backlight_cmds_nop;
		backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds_nop);
		mdp_gamma = mdp_gamma_jdi;
		mdp_gamma_count = ARRAY_SIZE(mdp_gamma_jdi);
	} else if(panel_type == PANEL_ID_M7_JDI_SAMSUNG_C2) {
		strncat(ptype, "PANEL_ID_M7_JDI_SAMSUNG_C2", ptype_len);
		cmd_on_cmds = samsung_jdi_panel_cmd_mode_cmds_c2_nvm;
		cmd_on_cmds_count = ARRAY_SIZE(samsung_jdi_panel_cmd_mode_cmds_c2_nvm);
		backlight_cmds = samsung_cmd_backlight_cmds;
		backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
		mdp_gamma = mdp_gamma_jdi;
		mdp_gamma_count = ARRAY_SIZE(mdp_gamma_jdi);
	} else if(panel_type == PANEL_ID_M7_JDI_SAMSUNG_C2_1) {
		strncat(ptype, "PANEL_ID_M7_JDI_SAMSUNG_C2_1", ptype_len);
		cmd_on_cmds = samsung_jdi_panel_cmd_mode_cmds_c2_1;
		cmd_on_cmds_count = ARRAY_SIZE(samsung_jdi_panel_cmd_mode_cmds_c2_1);
		backlight_cmds = samsung_cmd_backlight_cmds;
		backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
		set_cabc_UI_cmds = jdi_samsung_set_cabc_UI_cmds;
		set_cabc_UI_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_UI_cmds);
		set_cabc_Video_cmds = jdi_samsung_set_cabc_UI_cmds;
		set_cabc_Video_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_UI_cmds);
		set_cabc_Camera_cmds = jdi_samsung_set_cabc_Video_cmds;
		set_cabc_Camera_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_Video_cmds);
#endif
	} else {
		strncat(ptype, "PANEL_ID_M7_JDI_SAMSUNG_C2_2", ptype_len);
		cmd_on_cmds = samsung_jdi_panel_cmd_mode_cmds_c2_2;
		cmd_on_cmds_count = ARRAY_SIZE(samsung_jdi_panel_cmd_mode_cmds_c2_2);
		backlight_cmds = samsung_cmd_backlight_cmds;
		backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
		set_cabc_UI_cmds = jdi_samsung_set_cabc_UI_cmds;
		set_cabc_UI_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_UI_cmds);
		set_cabc_Video_cmds = jdi_samsung_set_cabc_UI_cmds;
		set_cabc_Video_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_UI_cmds);
		set_cabc_Camera_cmds = jdi_samsung_set_cabc_Video_cmds;
		set_cabc_Camera_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_Video_cmds);
#endif
	}

	display_on_cmds = samsung_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(samsung_display_on_cmds);
	display_off_cmds = samsung_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(samsung_display_off_cmds);
	dim_on_cmds = samsung_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(samsung_dim_on_cmds);
	dim_off_cmds = samsung_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(samsung_dim_off_cmds);
	color_en_on_cmds = samsung_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(samsung_color_enhance_on_cmds);
	color_en_off_cmds = samsung_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(samsung_color_enhance_on_cmds);

	pwm_min = 6;
	pwm_default = 69;
	pwm_max = 255;

	PR_DISP_INFO("%s\n", __func__);
	return ret;
}

static const struct i2c_device_id pwm_i2c_id[] = {
	{ "pwm_i2c", 0 },
	{ }
};

static int pwm_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_BYTE | I2C_FUNC_I2C))
		return -ENODEV;

	blk_pwm_client = client;

	return rc;
}

static struct i2c_driver pwm_i2c_driver = {
	.driver = {
		.name = "pwm_i2c",
		.owner = THIS_MODULE,
	},
	.probe = pwm_i2c_probe,
	.remove =  __exit_p( pwm_i2c_remove),
	.id_table =  pwm_i2c_id,
};
static void __exit pwm_i2c_remove(void)
{
	i2c_del_driver(&pwm_i2c_driver);
}

void __init m7_init_fb(void)
{

	platform_device_register(&msm_fb_device);

	if (panel_type == PANEL_ID_M7_SHARP_RENESAS)
		mdp_pdata.cont_splash_enabled = 0x1;

	if(panel_type != PANEL_ID_NONE) {
		msm_fb_register_device("mdp", &mdp_pdata);
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
		wa_xo = msm_xo_get(MSM_XO_TCXO_D0, "mipi");
	}
	msm_fb_register_device("dtv", &dtv_pdata);
#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
	platform_device_register(&wfd_panel_device);
	platform_device_register(&wfd_device);
#endif
}

static int __init m7_panel_init(void)
{
	int ret;

	if(panel_type == PANEL_ID_NONE)	{
		PR_DISP_INFO("%s panel ID = PANEL_ID_NONE\n", __func__);
		return 0;
	}

	ret = i2c_add_driver(&pwm_i2c_driver);

	if (ret)
		pr_err(KERN_ERR "%s: failed to add i2c driver\n", __func__);

	mipi_dsi_buf_alloc(&m7_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&m7_panel_rx_buf, DSI_BUF_SIZE);

	if (panel_type == PANEL_ID_DLXJ_SHARP_RENESAS) {
		mipi_video_sharp_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_DLXJ_SHARP_RENESAS\n", __func__);
	} else if (panel_type == PANEL_ID_DLXJ_SONY_RENESAS) {
		mipi_video_sony_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_DLXJ_SONY_RENESAS\n", __func__);
	} else if (panel_type == PANEL_ID_M7_JDI_SAMSUNG) {
		mipi_command_samsung_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_M7_JDI_SAMSUNG\n", __func__);
	} else if (panel_type == PANEL_ID_M7_JDI_SAMSUNG_C2) {
		mipi_command_samsung_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_M7_JDI_SAMSUNG_C2\n", __func__);
	} else if (panel_type == PANEL_ID_M7_JDI_SAMSUNG_C2_1) {
		mipi_command_samsung_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_M7_JDI_SAMSUNG_C2_1\n", __func__);
	} else if (panel_type == PANEL_ID_M7_JDI_SAMSUNG_C2_2) {
		mipi_command_samsung_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_M7_JDI_SAMSUNG_C2_2\n", __func__);
	} else if (panel_type == PANEL_ID_M7_SHARP_RENESAS) {
		mipi_video_sharp_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_M7_SHARP_RENESAS\n", __func__);
	} else if (panel_type == PANEL_ID_M7_SHARP_RENESAS_C1) {
		mipi_cmd_sharp_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_M7_SHARP_RENESAS_CMD\n", __func__);
	} else if (panel_type == PANEL_ID_M7_JDI_RENESAS) {
		mipi_cmd_jdi_renesas_init();
		PR_DISP_INFO("%s panel ID = PANEL_ID_M7_JDI_RENESAS_CMD\n", __func__);
	} else {
		PR_DISP_ERR("%s: panel not supported!!\n", __func__);
		return -ENODEV;
	}

	pwmic_ver = i2c_smbus_read_byte_data(blk_pwm_client, 0x1f);
	PR_DISP_INFO("%s: PWM IC version A%d\n", __func__, pwmic_ver);

#ifdef CONFIG_FB_MSM_CABC_LEVEL_CONTROL
	mutex_init(&set_cabc_mutex);
#endif
	PR_DISP_INFO("%s\n", __func__);

	return platform_driver_register(&this_driver);
}
device_initcall_sync(m7_panel_init);
