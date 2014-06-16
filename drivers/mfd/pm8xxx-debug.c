/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#define PM8XXX_DEBUG_DEV_NAME "pm8xxx-debug"

struct pm8xxx_debug_device {
	struct mutex		debug_mutex;
	struct device		*parent;
#if 0 
	struct dentry		*dir;
#endif
	int			addr;
};

struct s_pm8921_ssbi_reg {
    u16 addr;
    int multi_bank;
};

struct s_pm8921_ssbi_reg pm8921_regs[] = {
    
    {0x000, 0},    {0x001, 0},    {0x002, 0},    {0x003, 0},    {0x004, 0},
    {0x005, 0},    {0x006, 0},    {0x007, 0},    {0x008, 0},    {0x009, 0},
    {0x00A, 0},    {0x00B, 0},    {0x00C, 0},    {0x00D, 0},    {0x00E, 0},
    {0x010, 0},    {0x011, 0},    {0x012, 0},    {0x013, 0},    {0x014, 0},
    {0x015, 0},    {0x016, 0},    {0x018, 0},    {0x019, 0},    {0x01A, 0},
    {0x01B, 0},    {0x01C, 0},    {0x01D, 0},    {0x01E, 0},    {0x01F, 0},
    {0x020, 0},    {0x021, 0},    {0x022, 0},    {0x023, 0},    {0x024, 0},
    {0x025, 0},    {0x026, 0},    {0x027, 0},    {0x028, 0},    {0x029, 0},
    {0x02A, 8},    {0x02B, 0},    {0x02C, 0},    {0x02D, 0},    {0x02E, 8},
    {0x02F, 8},    {0x030, 8},    {0x031, 8},    {0x032, 0},    {0x033, 0},

    {0x034, 8},    {0x035, 8},    {0x036, 0},    {0x037, 0},    {0x038, 0},
    {0x039, 0},    {0x03A, 0},    {0x03B, 8},    {0x03C, 0},    {0x03D, 0},
    {0x03E, 0},    {0x03F, 8},    {0x040, 8},    {0x041, 8},    {0x042, 8},
    {0x043, 0},    {0x044, 0},    {0x045, 8},    {0x046, 8},    {0x047, 0},
    {0x048, 0},    {0x04A, 0},    {0x04F, 0},    {0x050, 0},    {0x051, 0},
    {0x052, 0},    {0x053, 0},    {0x054, 0},    {0x055, 0},    {0x056, 0},
    {0x057, 0},    {0x058, 0},    {0x059, 0},    {0x05A, 0},    {0x05B, 0},
    {0x060, 0},    {0x061, 0},    {0x062, 0},    {0x063, 0},    {0x064, 0},
    {0x065, 0},    {0x066, 0},    {0x067, 0},    {0x068, 0},    {0x069, 0},
    {0x06A, 0},    {0x06B, 0},    {0x06C, 0},    {0x06D, 0},    {0x06E, 0},

    {0x06F, 0},    {0x070, 0},    {0x071, 0},    {0x074, 8},    {0x07A, 8},
    {0x07D, 0},    {0x07E, 0},    {0x07F, 0},    {0x080, 0},    {0x081, 0},
    {0x082, 0},    {0x083, 0},    {0x084, 0},    {0x085, 0},    {0x086, 0},
    {0x087, 0},    {0x090, 0},    {0x09B, 0},    {0x09C, 0},    {0x0A0, 0},
    {0x0A1, 0},    {0x0A2, 0},    {0x0AB, 0},    {0x0AE, 0},    {0x0AF, 8},
    {0x0B0, 0},    {0x0B1, 8},    {0x0B2, 0},    {0x0B3, 8},    {0x0B4, 0},
    {0x0B5, 8},
    
    {0x0B7, 8},    {0x0B8, 0},    {0x0B9, 8},    {0x0BA, 0},
    {0x0BB, 8},    {0x0BC, 0},    {0x0BD, 8},    {0x0BE, 0},    {0x0BF, 8},
    {0x0C0, 0},    {0x0C1, 8},    {0x0C2, 0},    {0x0C3, 8},    {0x0C4, 0},
    {0x0C5, 8},    {0x0C8, 0},    {0x0C9, 8},    {0x0CA, 0},    {0x0CB, 8},

    {0x0CC, 0},    {0x0CD, 8},    {0x0CE, 0},    {0x0CF, 8},    {0x0D0, 0},
    {0x0D1, 8},    {0x0D6, 0},    {0x0D7, 8},    {0x0D8, 0},    {0x0D9, 8},
    {0x0DA, 0},    {0x0DB, 8},    {0x0DC, 0},    {0x0DD, 8},    {0x0DE, 0},
    {0x0DF, 8},    {0x0E0, 0},    {0x0E1, 8},    {0x0E2, 0},    {0x0E3, 8},
    {0x0E4, 0},    {0x0E5, 8},    {0x0E6, 0},    {0x0E7, 8},    {0x0E8, 0},
    {0x0EC, 0},    {0x0ED, 0},    {0x0EE, 0},    {0x0EF, 0},    {0x0F0, 0},
    {0x0F1, 0},    {0x0F2, 0},    {0x0F4, 0},    {0x0F5, 0},    {0x0FA, 0},
    {0x0FC, 0},    {0x100, 0},    {0x103, 0},    {0x104, 0},    {0x105, 0},
    {0x106, 0},    {0x107, 0},    {0x108, 0},    {0x109, 0},    {0x10A, 0},
    {0x10B, 0},    {0x10C, 0},    {0x10D, 0},    {0x10E, 0},    {0x10F, 0},

    {0x110, 0},    {0x111, 0},    {0x112, 0},    {0x113, 0},    {0x114, 0},
    {0x115, 0},    {0x116, 0},    {0x117, 0},    {0x118, 0},    {0x119, 0},
    {0x11A, 0},    {0x11B, 0},    {0x11C, 0},    {0x11D, 0},    {0x11E, 0},
    {0x11F, 0},    {0x120, 0},    {0x121, 0},    {0x122, 0},    {0x123, 0},
    {0x124, 0},    {0x125, 0},    {0x126, 0},    {0x127, 0},    {0x128, 0},
    {0x129, 0},    {0x12A, 0},    {0x12B, 0},    {0x131, 0},    {0x132, 0},
    {0x133, 0},    {0x134, 0},    {0x135, 0},    {0x136, 0},    {0x137, 0},
    {0x138, 0},    {0x139, 0},    {0x13A, 0},    {0x13B, 0},    {0x13C, 0},
    {0x13D, 0},    {0x13E, 0},    {0x13F, 0},    {0x140, 0},    {0x141, 0},
    {0x142, 0},    {0x143, 0},    {0x144, 0},    {0x145, 0},    {0x146, 0},

    {0x147, 0},    {0x148, 0},    {0x149, 0},    {0x14A, 0},    {0x14B, 0},
    {0x14C, 0},    {0x14E, 8},    {0x14F, 0},    {0x150, 8},    {0x151, 8},
    {0x152, 8},    {0x153, 8},    {0x154, 8},    {0x155, 8},    {0x156, 8},
    {0x157, 8},    {0x158, 8},    {0x159, 8},    {0x15A, 8},    {0x15B, 8},
    {0x15C, 8},    {0x15D, 8},    {0x15E, 8},    {0x15F, 8},    {0x160, 8},
    {0x161, 8},    {0x162, 8},    {0x163, 8},    {0x164, 8},    {0x165, 8},
    {0x166, 8},    {0x167, 8},    {0x168, 8},    {0x169, 8},    {0x16A, 8},
    {0x16B, 8},    {0x16C, 8},    {0x16D, 8},    {0x16E, 8},    {0x16F, 8},
    {0x170, 8},    {0x171, 8},    {0x172, 8},    {0x173, 8},    {0x174, 8},
    {0x175, 8},    {0x176, 8},    {0x177, 8},    {0x178, 8},    {0x179, 8},

    {0x17A, 8},    {0x17B, 8},    {0x17E, 0},    {0x17F, 0},    {0x180, 0},
    {0x181, 0},    {0x182, 0},    {0x183, 0},    {0x184, 0},    {0x185, 0},
    {0x186, 0},    {0x187, 0},    {0x188, 0},    {0x189, 0},    {0x18A, 0},
    {0x18B, 0},    {0x18C, 0},    {0x18D, 0},    {0x18E, 0},    {0x18F, 0},
    {0x190, 0},    {0x191, 0},    {0x192, 0},    {0x193, 0},    {0x194, 0},
    {0x195, 0},    {0x196, 0},    {0x197, 0},    {0x198, 0},    {0x199, 0},
    {0x19A, 0},    {0x19B, 0},    {0x19C, 0},    {0x19D, 0},    {0x19E, 0},
    {0x19F, 0},    {0x1A0, 0},    {0x1A1, 0},    {0x1A2, 0},    {0x1A3, 0},
    {0x1A4, 0},    {0x1A5, 0},    {0x1A6, 0},    {0x1A7, 0},    {0x1A8, 0},
    {0x1A9, 0},    {0x1AA, 0},    {0x1AB, 0},    {0x1AC, 0},    {0x1AD, 0},

    {0x1AE, 0},    {0x1AF, 0},    {0x1B1, 0},    {0x1B2, 0},    {0x1B3, 0},
    {0x1B4, 0},    {0x1B5, 0},    {0x1B6, 0},    {0x1B7, 0},    {0x1B8, 0},
    {0x1B9, 0},    {0x1BB, 0},    {0x1BC, 0},    {0x1BD, 0},    {0x1BE, 0},
    {0x1BF, 0},    {0x1C0, 0},    {0x1C1, 0},    {0x1C2, 0},    {0x1C3, 0},
    {0x1C5, 0},    {0x1C6, 0},    {0x1C7, 0},    {0x1C8, 0},    {0x1C9, 0},
    {0x1CA, 0},    {0x1CB, 0},    {0x1CC, 0},    {0x1CD, 0},    {0x1CE, 0},
    {0x1CF, 0},    {0x1D0, 0},    {0x1D1, 0},    {0x1D2, 0},    {0x1D3, 0},
    {0x1D4, 8},    {0x1D5, 8},    {0x1D6, 8},    {0x1D8, 0},    {0x1D9, 0},
    {0x1DA, 0},    {0x1DB, 0},    {0x1DC, 8},    {0x1DD, 8},    {0x1DE, 8},
    {0x1E0, 0},    {0x1E1, 0},    {0x1E2, 0},    {0x1E3, 0},    {0x1E4, 8},

    {0x1E5, 8},    {0x1E6, 8},    {0x1E8, 0},    {0x1E9, 0},    {0x1EA, 0},
    {0x1EB, 0},    {0x1EC, 8},    {0x1ED, 8},    {0x1EE, 8},    {0x1F0, 0},
    {0x1F1, 0},    {0x1F2, 0},    {0x1F3, 0},    {0x1F4, 8},    {0x1F5, 8},
    {0x1F6, 8},    {0x1F8, 0},    {0x1F9, 0},    {0x1FA, 0},    {0x1FB, 0},
    {0x1FC, 8},    {0x1FD, 8},    {0x1FE, 8},    {0x200, 0},    {0x204, 0},
    {0x205, 0},    {0x206, 8},    {0x207, 8},    {0x208, 8},    {0x209, 8},
    {0x20A, 8},    {0x20B, 0},    {0x20C, 0},    {0x20D, 0},    {0x20E, 0},
    {0x20F, 0},    {0x210, 0},    {0x211, 0},    {0x212, 0},    {0x213, 0},
    {0x214, 0},    {0x215, 0},    {0x216, 0},    {0x217, 0},    {0x218, 0},
    {0x219, 0},    {0x21A, 0},    {0x21B, 0},    {0x21C, 0},    {0x21D, 0},

    {0x21E, 8},    {0x21F, 8},    {0x220, 0},    {0x221, 0},    {0x222, 0},
    {0x224, 0},    {0x225, 0},    {0x226, 0},    {0x227, 0},    {0x228, 0},
    {0x229, 0},    {0x22A, 0},    {0x22B, 0},    {0x22C, 0},    {0x22D, 0},
    {0x22E, 0},    {0x22F, 0},    {0x230, 0},    {0x231, 0},    {0x232, 0},
    {0x233, 0},    {0x234, 0},    {0x235, 0},    {0x236, 0},    {0x237, 0},
    {0x238, 0},    {0x239, 0},    {0x23A, 0},    {0x23B, 0},    {0x23C, 0},
    {0x240, 0},    {0x241, 0},    {0x242, 0},    {0x243, 0},    {0x244, 0},
    {0x245, 0},    {0x246, 0},    {0x247, 0},    {0x24F, 0},    {0x300, 0},
    {0x303, 0},    {0x304, 0},    {0x305, 0},    {0x306, 0},    {0x307, 0},
    {0x308, 0},    {0x30A, 0},    {0x30B, 0},    {0x30C, 0},    {0x30D, 0},

    {0x30E, 0},    {0x30F, 0},    {0x310, 0},    {0x311, 0},    {0x312, 0},
    {0x313, 0},    {0x314, 0},    {0x315, 0},    {0x317, 0},    {0x318, 0},
    {0x319, 0},    {0x31A, 0},    {0x31B, 0},    {0x31C, 0},    {0x31D, 0},
    {0x31E, 0},    {0x31F, 0},    {0x321, 0},    {0x322, 0},    {0x323, 0},
    {0x324, 0},    {0x325, 0},    {0x327, 0},    {0x32A, 0},    {0x32B, 0},
    {0x32C, 0},    {0x32D, 0},    {0x32E, 0},    {0x32F, 0},    {0x330, 0},
    {0x331, 0},    {0x332, 0},    {0x333, 0},    {0x334, 0},    {0x336, 0},
    {0x337, 0},    {0x338, 0},    {0x339, 0},    {0x33A, 0},    {0x33B, 0},
    {0x33C, 0},    {0x33D, 0},    {0x33E, 0},    {0x33F, 0},    {0x342, 0},
    {0x343, 0},    {0x344, 0},    {0x345, 0},    {0x346, 0},    {0x347, 0},

    {0x34A, 0},    {0x34B, 0},    {0x34C, 0},    {0x34D, 0},    {0x34F, 0},
    {0x350, 0},    {0x351, 0},    {0x352, 0},    {0x353, 0},    {0x355, 0},
    {0x356, 0},    {0x357, 0},    {0x358, 0},    {0x359, 0},    {0x35A, 0},
    {0x35B, 0},    {0x35C, 0},    {0x35E, 0},    {0x35F, 0},    {0x360, 0},
    {0x361, 0},    {0x362, 0},    {0x363, 0},    {0x364, 0},    {0x365, 0},
    {0x366, 0},    {0x367, 0},    {0x368, 0},    {0x369, 0},    {0x36A, 0},
    {0x36B, 0},    {0x36C, 0},    {0x36D, 0},    {0x36E, 0},    {0x36F, 0},
    {0x370, 0},    {0x371, 0},    {0x374, 0},    {0x375, 0},    {0x376, 0},
    {0x377, 0},    {0x378, 0},    {0x379, 0},    {0x37A, 0},    {0x37C, 0},
    {0x37D, 0},    {0x37F, 0},

    {0xFFF, -1}    
}; 

static bool pm8xxx_debug_addr_is_valid(int addr)
{
	if (addr < 0 || addr > 0x3FF) {
		pr_err("PMIC register address is invalid: %d\n", addr);
		return false;
	}
	return true;
}

static int pm8xxx_debug_data_set(void *data, u64 val)
{
	struct pm8xxx_debug_device *debugdev = data;
	u8 reg = val;
	int rc;

	mutex_lock(&debugdev->debug_mutex);

	if (pm8xxx_debug_addr_is_valid(debugdev->addr)) {
		rc = pm8xxx_writeb(debugdev->parent, debugdev->addr, reg);

		if (rc)
			pr_err("pm8xxx_writeb(0x%03X)=0x%02X failed: rc=%d\n",
				debugdev->addr, reg, rc);
	}

	mutex_unlock(&debugdev->debug_mutex);
	return 0;
}

static int pm8xxx_debug_data_get(void *data, u64 *val)
{
	struct pm8xxx_debug_device *debugdev = data;
	int rc;
	u8 reg;

	mutex_lock(&debugdev->debug_mutex);

	if (pm8xxx_debug_addr_is_valid(debugdev->addr)) {
		rc = pm8xxx_readb(debugdev->parent, debugdev->addr, &reg);

		if (rc)
			pr_err("pm8xxx_readb(0x%03X) failed: rc=%d\n",
				debugdev->addr, rc);
		else
			*val = reg;
	}

	mutex_unlock(&debugdev->debug_mutex);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_data_fops, pm8xxx_debug_data_get,
			pm8xxx_debug_data_set, "0x%02llX\n");

static int pm8xxx_debug_addr_set(void *data, u64 val)
{
	struct pm8xxx_debug_device *debugdev = data;

	if (pm8xxx_debug_addr_is_valid(val)) {
		mutex_lock(&debugdev->debug_mutex);
		debugdev->addr = val;
		mutex_unlock(&debugdev->debug_mutex);
	}

	return 0;
}

static int pm8xxx_debug_addr_get(void *data, u64 *val)
{
	struct pm8xxx_debug_device *debugdev = data;

	mutex_lock(&debugdev->debug_mutex);

	if (pm8xxx_debug_addr_is_valid(debugdev->addr))
		*val = debugdev->addr;

	mutex_unlock(&debugdev->debug_mutex);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_addr_fops, pm8xxx_debug_addr_get,
			pm8xxx_debug_addr_set, "0x%03llX\n");

enum pm8xxx_vreg_id {
	PM8xxx_VREG_ID_L1 = 0,
	PM8xxx_VREG_ID_L2,
	PM8xxx_VREG_ID_L3,
	PM8xxx_VREG_ID_L4,
	PM8xxx_VREG_ID_L5,
	PM8xxx_VREG_ID_L6,
	PM8xxx_VREG_ID_L7,
	PM8xxx_VREG_ID_L8,
	PM8xxx_VREG_ID_L9,
	PM8xxx_VREG_ID_L10,
	PM8xxx_VREG_ID_L11,
	PM8xxx_VREG_ID_L12,
	PM8xxx_VREG_ID_L14,
	PM8xxx_VREG_ID_L15,
	PM8xxx_VREG_ID_L16,
	PM8xxx_VREG_ID_L17,
	PM8xxx_VREG_ID_L18,
	PM8xxx_VREG_ID_L19,
	PM8xxx_VREG_ID_L20,
	PM8xxx_VREG_ID_L21,
	PM8xxx_VREG_ID_L22,
	PM8xxx_VREG_ID_L23,
	PM8xxx_VREG_ID_L24,
	PM8xxx_VREG_ID_L25,
	PM8xxx_VREG_ID_L26,
	PM8xxx_VREG_ID_L27,
	PM8xxx_VREG_ID_L28,
	PM8xxx_VREG_ID_L29,
	PM8xxx_VREG_ID_S1,
	PM8xxx_VREG_ID_S2,
	PM8xxx_VREG_ID_S3,
	PM8xxx_VREG_ID_S4,
	PM8xxx_VREG_ID_S5,
	PM8xxx_VREG_ID_S6,
	PM8xxx_VREG_ID_S7,
	PM8xxx_VREG_ID_S8,
	PM8xxx_VREG_ID_LVS1,
	PM8xxx_VREG_ID_LVS2,
	PM8xxx_VREG_ID_LVS3,
	PM8xxx_VREG_ID_LVS4,
	PM8xxx_VREG_ID_LVS5,
	PM8xxx_VREG_ID_LVS6,
	PM8xxx_VREG_ID_LVS7,
	PM8xxx_VREG_ID_USB_OTG,
	PM8xxx_VREG_ID_HDMI_MVS,
	PM8xxx_VREG_ID_NCP,
	
	PM8xxx_VREG_ID_L1_PC,
	PM8xxx_VREG_ID_L2_PC,
	PM8xxx_VREG_ID_L3_PC,
	PM8xxx_VREG_ID_L4_PC,
	PM8xxx_VREG_ID_L5_PC,
	PM8xxx_VREG_ID_L6_PC,
	PM8xxx_VREG_ID_L7_PC,
	PM8xxx_VREG_ID_L8_PC,
	PM8xxx_VREG_ID_L9_PC,
	PM8xxx_VREG_ID_L10_PC,
	PM8xxx_VREG_ID_L11_PC,
	PM8xxx_VREG_ID_L12_PC,
	PM8xxx_VREG_ID_L13_PC,
	PM8xxx_VREG_ID_L14_PC,
	PM8xxx_VREG_ID_L15_PC,
	PM8xxx_VREG_ID_L16_PC,
	PM8xxx_VREG_ID_L17_PC,
	PM8xxx_VREG_ID_L18_PC,
	PM8xxx_VREG_ID_L21_PC,
	PM8xxx_VREG_ID_L22_PC,
	PM8xxx_VREG_ID_L23_PC,

	PM8xxx_VREG_ID_L29_PC,
	PM8xxx_VREG_ID_S1_PC,
	PM8xxx_VREG_ID_S2_PC,
	PM8xxx_VREG_ID_S3_PC,
	PM8xxx_VREG_ID_S4_PC,

	PM8xxx_VREG_ID_S7_PC,
	PM8xxx_VREG_ID_S8_PC,
	PM8xxx_VREG_ID_LVS1_PC,

	PM8xxx_VREG_ID_LVS3_PC,
	PM8xxx_VREG_ID_LVS4_PC,
	PM8xxx_VREG_ID_LVS5_PC,
	PM8xxx_VREG_ID_LVS6_PC,
	PM8xxx_VREG_ID_LVS7_PC,

	PM8xxx_VREG_ID_MAX,
};

#define PM8xxx_VREG_PIN_CTRL_NONE	0x00
#define PM8xxx_VREG_PIN_CTRL_D1		0x01
#define PM8xxx_VREG_PIN_CTRL_A0		0x02
#define PM8xxx_VREG_PIN_CTRL_A1		0x04
#define PM8xxx_VREG_PIN_CTRL_A2		0x08

#define PM8xxx_VREG_LDO_50_HPM_MIN_LOAD		5000
#define PM8xxx_VREG_LDO_150_HPM_MIN_LOAD	10000
#define PM8xxx_VREG_LDO_300_HPM_MIN_LOAD	10000
#define PM8xxx_VREG_LDO_600_HPM_MIN_LOAD	10000
#define PM8xxx_VREG_LDO_1200_HPM_MIN_LOAD	10000
#define PM8xxx_VREG_SMPS_1500_HPM_MIN_LOAD	100000
#define PM8xxx_VREG_SMPS_2000_HPM_MIN_LOAD	100000

enum pm8xxx_vreg_pin_function {
	PM8xxx_VREG_PIN_FN_ENABLE = 0,
	PM8xxx_VREG_PIN_FN_MODE,
};

#define REGULATOR_TYPE_PLDO		0
#define REGULATOR_TYPE_NLDO		1
#define REGULATOR_TYPE_NLDO1200		2
#define REGULATOR_TYPE_SMPS		3
#define REGULATOR_TYPE_FTSMPS		4
#define REGULATOR_TYPE_VS		5
#define REGULATOR_TYPE_VS300		6
#define REGULATOR_TYPE_NCP		7
#define REGULATOR_ENABLE_MASK		0x80
#define REGULATOR_ENABLE		0x80
#define REGULATOR_DISABLE		0x00

#define REGULATOR_BANK_MASK		0xF0
#define REGULATOR_BANK_SEL(n)		((n) << 4)
#define REGULATOR_BANK_WRITE		0x80

#define LDO_TEST_BANKS			7
#define NLDO1200_TEST_BANKS		5
#define SMPS_TEST_BANKS			8
#define REGULATOR_TEST_BANKS_MAX	SMPS_TEST_BANKS

#define VOLTAGE_UNKNOWN			1


#define LDO_ENABLE_MASK			0x80
#define LDO_DISABLE			0x00
#define LDO_ENABLE			0x80
#define LDO_PULL_DOWN_ENABLE_MASK	0x40
#define LDO_PULL_DOWN_ENABLE		0x40

#define LDO_CTRL_PM_MASK		0x20
#define LDO_CTRL_PM_HPM			0x00
#define LDO_CTRL_PM_LPM			0x20

#define LDO_CTRL_VPROG_MASK		0x1F

#define LDO_TEST_LPM_MASK		0x04
#define LDO_TEST_LPM_SEL_CTRL		0x00
#define LDO_TEST_LPM_SEL_TCXO		0x04

#define LDO_TEST_VPROG_UPDATE_MASK	0x08
#define LDO_TEST_RANGE_SEL_MASK		0x04
#define LDO_TEST_FINE_STEP_MASK		0x02
#define LDO_TEST_FINE_STEP_SHIFT	1

#define LDO_TEST_RANGE_EXT_MASK		0x01

#define LDO_TEST_PIN_CTRL_MASK		0x0F
#define LDO_TEST_PIN_CTRL_EN3		0x08
#define LDO_TEST_PIN_CTRL_EN2		0x04
#define LDO_TEST_PIN_CTRL_EN1		0x02
#define LDO_TEST_PIN_CTRL_EN0		0x01

#define LDO_TEST_PIN_CTRL_LPM_MASK	0x0F


#define PLDO_LOW_UV_MIN			750000
#define PLDO_LOW_UV_MAX			1487500
#define PLDO_LOW_UV_FINE_STEP		12500

#define PLDO_NORM_UV_MIN		1500000
#define PLDO_NORM_UV_MAX		3075000
#define PLDO_NORM_UV_FINE_STEP		25000

#define PLDO_HIGH_UV_MIN		1750000
#define PLDO_HIGH_UV_SET_POINT_MIN	3100000
#define PLDO_HIGH_UV_MAX		4900000
#define PLDO_HIGH_UV_FINE_STEP		50000

#define PLDO_LOW_SET_POINTS		((PLDO_LOW_UV_MAX - PLDO_LOW_UV_MIN) \
						/ PLDO_LOW_UV_FINE_STEP + 1)
#define PLDO_NORM_SET_POINTS		((PLDO_NORM_UV_MAX - PLDO_NORM_UV_MIN) \
						/ PLDO_NORM_UV_FINE_STEP + 1)
#define PLDO_HIGH_SET_POINTS		((PLDO_HIGH_UV_MAX \
						- PLDO_HIGH_UV_SET_POINT_MIN) \
					/ PLDO_HIGH_UV_FINE_STEP + 1)
#define PLDO_SET_POINTS			(PLDO_LOW_SET_POINTS \
						+ PLDO_NORM_SET_POINTS \
						+ PLDO_HIGH_SET_POINTS)

#define NLDO_UV_MIN			750000
#define NLDO_UV_MAX			1537500
#define NLDO_UV_FINE_STEP		12500

#define NLDO_SET_POINTS			((NLDO_UV_MAX - NLDO_UV_MIN) \
						/ NLDO_UV_FINE_STEP + 1)


#define NLDO1200_ENABLE_MASK		0x80
#define NLDO1200_DISABLE		0x00
#define NLDO1200_ENABLE			0x80

#define NLDO1200_LEGACY_PM_MASK		0x20
#define NLDO1200_LEGACY_PM_HPM		0x00
#define NLDO1200_LEGACY_PM_LPM		0x20

#define NLDO1200_CTRL_RANGE_MASK	0x40
#define NLDO1200_CTRL_RANGE_HIGH	0x00
#define NLDO1200_CTRL_RANGE_LOW		0x40
#define NLDO1200_CTRL_VPROG_MASK	0x3F

#define NLDO1200_LOW_UV_MIN		375000
#define NLDO1200_LOW_UV_MAX		743750
#define NLDO1200_LOW_UV_STEP		6250

#define NLDO1200_HIGH_UV_MIN		750000
#define NLDO1200_HIGH_UV_MAX		1537500
#define NLDO1200_HIGH_UV_STEP		12500

#define NLDO1200_LOW_SET_POINTS		((NLDO1200_LOW_UV_MAX \
						- NLDO1200_LOW_UV_MIN) \
					/ NLDO1200_LOW_UV_STEP + 1)
#define NLDO1200_HIGH_SET_POINTS	((NLDO1200_HIGH_UV_MAX \
						- NLDO1200_HIGH_UV_MIN) \
					/ NLDO1200_HIGH_UV_STEP + 1)
#define NLDO1200_SET_POINTS		(NLDO1200_LOW_SET_POINTS \
						+ NLDO1200_HIGH_SET_POINTS)

#define NLDO1200_TEST_LPM_MASK		0x04
#define NLDO1200_TEST_LPM_SEL_CTRL	0x00
#define NLDO1200_TEST_LPM_SEL_TCXO	0x04

#define NLDO1200_PULL_DOWN_ENABLE_MASK	0x02
#define NLDO1200_PULL_DOWN_ENABLE	0x02

#define NLDO1200_ADVANCED_MODE_MASK	0x08
#define NLDO1200_ADVANCED_MODE		0x00
#define NLDO1200_LEGACY_MODE		0x08

#define NLDO1200_ADVANCED_PM_MASK	0x02
#define NLDO1200_ADVANCED_PM_HPM	0x00
#define NLDO1200_ADVANCED_PM_LPM	0x02

#define NLDO1200_IN_ADVANCED_MODE(vreg) \
	((vreg->test_reg[2] & NLDO1200_ADVANCED_MODE_MASK) \
	 == NLDO1200_ADVANCED_MODE)



#define SMPS_LEGACY_ENABLE_MASK		0x80
#define SMPS_LEGACY_DISABLE		0x00
#define SMPS_LEGACY_ENABLE		0x80
#define SMPS_LEGACY_PULL_DOWN_ENABLE	0x40
#define SMPS_LEGACY_VREF_SEL_MASK	0x20
#define SMPS_LEGACY_VPROG_MASK		0x1F

#define SMPS_ADVANCED_BAND_MASK		0xC0
#define SMPS_ADVANCED_BAND_OFF		0x00
#define SMPS_ADVANCED_BAND_1		0x40
#define SMPS_ADVANCED_BAND_2		0x80
#define SMPS_ADVANCED_BAND_3		0xC0
#define SMPS_ADVANCED_VPROG_MASK	0x3F

#define SMPS_MODE3_UV_MIN		375000
#define SMPS_MODE3_UV_MAX		725000
#define SMPS_MODE3_UV_STEP		25000

#define SMPS_MODE2_UV_MIN		750000
#define SMPS_MODE2_UV_MAX		1475000
#define SMPS_MODE2_UV_STEP		25000

#define SMPS_MODE1_UV_MIN		1500000
#define SMPS_MODE1_UV_MAX		3050000
#define SMPS_MODE1_UV_STEP		50000

#define SMPS_MODE3_SET_POINTS		((SMPS_MODE3_UV_MAX \
						- SMPS_MODE3_UV_MIN) \
					/ SMPS_MODE3_UV_STEP + 1)
#define SMPS_MODE2_SET_POINTS		((SMPS_MODE2_UV_MAX \
						- SMPS_MODE2_UV_MIN) \
					/ SMPS_MODE2_UV_STEP + 1)
#define SMPS_MODE1_SET_POINTS		((SMPS_MODE1_UV_MAX \
						- SMPS_MODE1_UV_MIN) \
					/ SMPS_MODE1_UV_STEP + 1)
#define SMPS_LEGACY_SET_POINTS		(SMPS_MODE3_SET_POINTS \
						+ SMPS_MODE2_SET_POINTS \
						+ SMPS_MODE1_SET_POINTS)

#define SMPS_BAND1_UV_MIN		375000
#define SMPS_BAND1_UV_MAX		737500
#define SMPS_BAND1_UV_STEP		12500

#define SMPS_BAND2_UV_MIN		750000
#define SMPS_BAND2_UV_MAX		1487500
#define SMPS_BAND2_UV_STEP		12500

#define SMPS_BAND3_UV_MIN		1500000
#define SMPS_BAND3_UV_MAX		3075000
#define SMPS_BAND3_UV_STEP		25000

#define SMPS_BAND1_SET_POINTS		((SMPS_BAND1_UV_MAX \
						- SMPS_BAND1_UV_MIN) \
					/ SMPS_BAND1_UV_STEP + 1)
#define SMPS_BAND2_SET_POINTS		((SMPS_BAND2_UV_MAX \
						- SMPS_BAND2_UV_MIN) \
					/ SMPS_BAND2_UV_STEP + 1)
#define SMPS_BAND3_SET_POINTS		((SMPS_BAND3_UV_MAX \
						- SMPS_BAND3_UV_MIN) \
					/ SMPS_BAND3_UV_STEP + 1)
#define SMPS_ADVANCED_SET_POINTS	(SMPS_BAND1_SET_POINTS \
						+ SMPS_BAND2_SET_POINTS \
						+ SMPS_BAND3_SET_POINTS)

#define SMPS_LEGACY_VLOW_SEL_MASK	0x01

#define SMPS_ADVANCED_PULL_DOWN_ENABLE	0x08

#define SMPS_ADVANCED_MODE_MASK		0x02
#define SMPS_ADVANCED_MODE		0x02
#define SMPS_LEGACY_MODE		0x00

#define SMPS_IN_ADVANCED_MODE(vreg) \
	((vreg->test_reg[7] & SMPS_ADVANCED_MODE_MASK) == SMPS_ADVANCED_MODE)

#define SMPS_PIN_CTRL_MASK		0xF0
#define SMPS_PIN_CTRL_EN3		0x80
#define SMPS_PIN_CTRL_EN2		0x40
#define SMPS_PIN_CTRL_EN1		0x20
#define SMPS_PIN_CTRL_EN0		0x10

#define SMPS_PIN_CTRL_LPM_MASK		0x0F
#define SMPS_PIN_CTRL_LPM_EN3		0x08
#define SMPS_PIN_CTRL_LPM_EN2		0x04
#define SMPS_PIN_CTRL_LPM_EN1		0x02
#define SMPS_PIN_CTRL_LPM_EN0		0x01

#define SMPS_CLK_DIVIDE2		0x40

#define SMPS_CLK_CTRL_MASK		0x30
#define SMPS_CLK_CTRL_FOLLOW_TCXO	0x00
#define SMPS_CLK_CTRL_PWM		0x10
#define SMPS_CLK_CTRL_PFM		0x20


#define FTSMPS_VCTRL_BAND_MASK		0xC0
#define FTSMPS_VCTRL_BAND_OFF		0x00
#define FTSMPS_VCTRL_BAND_1		0x40
#define FTSMPS_VCTRL_BAND_2		0x80
#define FTSMPS_VCTRL_BAND_3		0xC0
#define FTSMPS_VCTRL_VPROG_MASK		0x3F

#define FTSMPS_BAND1_UV_MIN		350000
#define FTSMPS_BAND1_UV_MAX		650000
#define FTSMPS_BAND1_UV_LOG_STEP	50000
#define FTSMPS_BAND1_UV_PHYS_STEP	6250

#define FTSMPS_BAND2_UV_MIN		700000
#define FTSMPS_BAND2_UV_MAX		1400000
#define FTSMPS_BAND2_UV_STEP		12500

#define FTSMPS_BAND3_UV_MIN		1400000
#define FTSMPS_BAND3_UV_SET_POINT_MIN	1500000
#define FTSMPS_BAND3_UV_MAX		3300000
#define FTSMPS_BAND3_UV_STEP		50000

#define FTSMPS_BAND1_SET_POINTS		((FTSMPS_BAND1_UV_MAX \
						- FTSMPS_BAND1_UV_MIN) \
					/ FTSMPS_BAND1_UV_LOG_STEP + 1)
#define FTSMPS_BAND2_SET_POINTS		((FTSMPS_BAND2_UV_MAX \
						- FTSMPS_BAND2_UV_MIN) \
					/ FTSMPS_BAND2_UV_STEP + 1)
#define FTSMPS_BAND3_SET_POINTS		((FTSMPS_BAND3_UV_MAX \
					  - FTSMPS_BAND3_UV_SET_POINT_MIN) \
					/ FTSMPS_BAND3_UV_STEP + 1)
#define FTSMPS_SET_POINTS		(FTSMPS_BAND1_SET_POINTS \
						+ FTSMPS_BAND2_SET_POINTS \
						+ FTSMPS_BAND3_SET_POINTS)

#define FTSMPS_CNFG1_PM_MASK		0x0C
#define FTSMPS_CNFG1_PM_PWM		0x00
#define FTSMPS_CNFG1_PM_PFM		0x08

#define FTSMPS_PULL_DOWN_ENABLE_MASK	0x40
#define FTSMPS_PULL_DOWN_ENABLE		0x40


#define VS_ENABLE_MASK			0x80
#define VS_DISABLE			0x00
#define VS_ENABLE			0x80
#define VS_PULL_DOWN_ENABLE_MASK	0x40
#define VS_PULL_DOWN_DISABLE		0x40
#define VS_PULL_DOWN_ENABLE		0x00

#define VS_PIN_CTRL_MASK		0x0F
#define VS_PIN_CTRL_EN0			0x08
#define VS_PIN_CTRL_EN1			0x04
#define VS_PIN_CTRL_EN2			0x02
#define VS_PIN_CTRL_EN3			0x01


#define VS300_CTRL_ENABLE_MASK		0xC0
#define VS300_CTRL_DISABLE		0x00
#define VS300_CTRL_ENABLE		0x40

#define VS300_PULL_DOWN_ENABLE_MASK	0x20
#define VS300_PULL_DOWN_ENABLE		0x20


#define NCP_ENABLE_MASK			0x80
#define NCP_DISABLE			0x00
#define NCP_ENABLE			0x80
#define NCP_VPROG_MASK			0x1F

#define NCP_UV_MIN			1500000
#define NCP_UV_MAX			3050000
#define NCP_UV_STEP			50000

#define NCP_SET_POINTS			((NCP_UV_MAX - NCP_UV_MIN) \
						/ NCP_UV_STEP + 1)
#define IS_REAL_REGULATOR(id)	((id) >= 0 && \
				 (id) < pm8xxx_current_table->table_size)

#define MODE_LDO_HPM 0
#define MODE_LDO_LPM 1
#define MODE_NLDO1200_HPM 2
#define MODE_NLDO1200_LPM 3
#define MODE_SMPS_FOLLOW_TCXO 4
#define MODE_SMPS_PWM 5
#define MODE_SMPS_PFM 6
#define MODE_SMPS_AUTO 7
#define MODE_FTSMPS_PWM 8
#define MODE_FTSMPS_1 9
#define MODE_FTSMPS_PFM 10
#define MODE_FTSMPS_3 11

struct pm8xxx_vreg {
	
	const char				*name;
	const int				hpm_min_load;
	const u16				ctrl_addr;
	const u16				test_addr;
	const u16				clk_ctrl_addr;
	const u16				sleep_ctrl_addr;
	const u16				pfm_ctrl_addr;
	const u16				pwr_cnfg_addr;
	const u8				type;
	
	u8				test_reg[REGULATOR_TEST_BANKS_MAX];
	u8					ctrl_reg;
	u8					clk_ctrl_reg;
	u8					sleep_ctrl_reg;
	u8					pfm_ctrl_reg;
	u8					pwr_cnfg_reg;
};

#define vreg_err(vreg, fmt, ...) \
	pr_err("%s: " fmt, vreg->name, ##__VA_ARGS__)

#define PLDO(_id, _ctrl_addr, _test_addr, _hpm_min_load, _name) \
	[PM8xxx_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_PLDO, \
		.ctrl_addr	= _ctrl_addr, \
		.test_addr	= _test_addr, \
		.hpm_min_load	= PM8xxx_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define NLDO(_id, _ctrl_addr, _test_addr, _hpm_min_load, _name) \
	[PM8xxx_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_NLDO, \
		.ctrl_addr	= _ctrl_addr, \
		.test_addr	= _test_addr, \
		.hpm_min_load	= PM8xxx_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define NLDO1200(_id, _ctrl_addr, _test_addr, _hpm_min_load, _name) \
	[PM8xxx_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_NLDO1200, \
		.ctrl_addr	= _ctrl_addr, \
		.test_addr	= _test_addr, \
		.hpm_min_load	= PM8xxx_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define SMPS(_id, _ctrl_addr, _test_addr, _clk_ctrl_addr, _sleep_ctrl_addr, \
	     _hpm_min_load, _name) \
	[PM8xxx_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_SMPS, \
		.ctrl_addr	= _ctrl_addr, \
		.test_addr	= _test_addr, \
		.clk_ctrl_addr	= _clk_ctrl_addr, \
		.sleep_ctrl_addr = _sleep_ctrl_addr, \
		.hpm_min_load	= PM8xxx_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define FTSMPS(_id, _pwm_ctrl_addr, _fts_cnfg1_addr, _pfm_ctrl_addr, \
	       _pwr_cnfg_addr, _hpm_min_load, _name) \
	[PM8xxx_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_FTSMPS, \
		.ctrl_addr	= _pwm_ctrl_addr, \
		.test_addr	= _fts_cnfg1_addr, \
		.pfm_ctrl_addr = _pfm_ctrl_addr, \
		.pwr_cnfg_addr = _pwr_cnfg_addr, \
		.hpm_min_load	= PM8xxx_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define VS(_id, _ctrl_addr, _name) \
	[PM8xxx_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_VS, \
		.ctrl_addr	= _ctrl_addr, \
		.name = _name, \
	}

#define VS300(_id, _ctrl_addr, _name) \
	[PM8xxx_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_VS300, \
		.ctrl_addr	= _ctrl_addr, \
		.name = _name, \
	}

#define NCP(_id, _ctrl_addr, _name) \
	[PM8xxx_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_NCP, \
		.ctrl_addr	= _ctrl_addr, \
		.name = _name, \
	}

static struct pm8xxx_vreg pm8921_vreg[] = {
	
	NLDO(L1,  0x0AE, 0x0AF, LDO_150, "8921_l1"),
	NLDO(L2,  0x0B0, 0x0B1, LDO_150, "8921_l2"),
	PLDO(L3,  0x0B2, 0x0B3, LDO_150, "8921_l3"),
	PLDO(L4,  0x0B4, 0x0B5, LDO_50, "8921_l4"),
	PLDO(L5,  0x0B6, 0x0B7, LDO_300, "8921_l5"),
	PLDO(L6,  0x0B8, 0x0B9, LDO_600, "8921_l6"),
	PLDO(L7,  0x0BA, 0x0BB, LDO_150, "8921_l7"),
	PLDO(L8,  0x0BC, 0x0BD, LDO_300, "8921_l8"),
	PLDO(L9,  0x0BE, 0x0BF, LDO_300, "8921_l9"),
	PLDO(L10, 0x0C0, 0x0C1, LDO_600, "8921_l10"),
	PLDO(L11, 0x0C2, 0x0C3, LDO_150, "8921_l11"),
	NLDO(L12, 0x0C4, 0x0C5, LDO_150, "8921_l12"),
	PLDO(L14, 0x0C8, 0x0C9, LDO_50, "8921_l14"),
	PLDO(L15, 0x0CA, 0x0CB, LDO_150, "8921_l15"),
	PLDO(L16, 0x0CC, 0x0CD, LDO_300, "8921_l16"),
	PLDO(L17, 0x0CE, 0x0CF, LDO_150, "8921_l17"),
	NLDO(L18, 0x0D0, 0x0D1, LDO_150, "8921_l18"),
	PLDO(L21, 0x0D6, 0x0D7, LDO_150, "8921_l21"),
	PLDO(L22, 0x0D8, 0x0D9, LDO_150, "8921_l22"),
	PLDO(L23, 0x0DA, 0x0DB, LDO_150, "8921_l23"),

	
	NLDO1200(L24, 0x0DC, 0x0DD, LDO_1200, "8921_l24"),
	NLDO1200(L25, 0x0DE, 0x0DF, LDO_1200, "8921_l25"),
	NLDO1200(L26, 0x0E0, 0x0E1, LDO_1200, "8921_l26"),
	NLDO1200(L27, 0x0E2, 0x0E3, LDO_1200, "8921_l27"),
	NLDO1200(L28, 0x0E4, 0x0E5, LDO_1200, "8921_l28"),

	
	PLDO(L29, 0x0E6, 0x0E7, LDO_150, "8921_l29"),

	
	SMPS(S1, 0x1D0, 0x1D5, 0x009, 0x1D2, SMPS_1500, "8921_s1"),
	SMPS(S2, 0x1D8, 0x1DD, 0x00A, 0x1DA, SMPS_1500, "8921_s2"),
	SMPS(S3, 0x1E0, 0x1E5, 0x00B, 0x1E2, SMPS_1500, "8921_s3"),
	SMPS(S4, 0x1E8, 0x1ED, 0x011, 0x1EA, SMPS_1500, "8921_s4"),

	
	FTSMPS(S5, 0x025, 0x02E, 0x026, 0x032, SMPS_2000, "8921_s5"),
	FTSMPS(S6, 0x036, 0x03F, 0x037, 0x043, SMPS_2000, "8921_s6"),

	
	SMPS(S7, 0x1F0, 0x1F5, 0x012, 0x1F2, SMPS_1500, "8921_s7"),
	SMPS(S8, 0x1F8, 0x1FD, 0x013, 0x1FA, SMPS_1500, "8921_s8"),

	
	VS(LVS1,	0x060, "8921_lvs1"),
	VS300(LVS2,     0x062, "8921_lvs2"),
	VS(LVS3,	0x064, "8921_lvs3"),
	VS(LVS4,	0x066, "8921_lvs4"),
	VS(LVS5,	0x068, "8921_lvs5"),
	VS(LVS6,	0x06A, "8921_lvs6"),
	VS(LVS7,	0x06C, "8921_lvs7"),
	VS300(USB_OTG,  0x06E, "8921_usb_otg"),
	VS300(HDMI_MVS, 0x070, "8921_hdmi_mvs"),

	
	NCP(NCP, 0x090, "8921_ncp"),
};

static struct pm8xxx_vreg pm8038_vreg[] = {
	
	NLDO1200(L1, 0x0AE, 0x0AF, LDO_1200, "8038_l1"),
	NLDO(L2,  0x0B0, 0x0B1, LDO_150, "8038_l2"),
	PLDO(L3,  0x0B2, 0x0B3, LDO_50, "8038_l3"),
	PLDO(L4,  0x0B4, 0x0B5, LDO_50, "8038_l4"),
	PLDO(L5,  0x0B6, 0x0B7, LDO_600, "8038_l5"),
	PLDO(L6,  0x0B8, 0x0B9, LDO_600, "8038_l6"),
	PLDO(L7,  0x0BA, 0x0BB, LDO_600, "8038_l7"),
	PLDO(L8,  0x0BC, 0x0BD, LDO_300, "8038_l8"),
	PLDO(L9,  0x0BE, 0x0BF, LDO_300, "8038_l9"),
	PLDO(L10, 0x0C0, 0x0C1, LDO_600, "8038_l10"),
	PLDO(L11, 0x0C2, 0x0C3, LDO_600, "8038_l11"),
	NLDO(L12, 0x0C4, 0x0C5, LDO_300, "8038_l12"),
	PLDO(L14, 0x0C8, 0x0C9, LDO_50, "8038_l14"),
	PLDO(L15, 0x0CA, 0x0CB, LDO_150, "8038_l15"),
	NLDO1200(L16, 0x0CC, 0x0CD, LDO_1200, "8038_l16"),
	PLDO(L17, 0x0CE, 0x0CF, LDO_150, "8038_l17"),
	PLDO(L18, 0x0D0, 0x0D1, LDO_50, "8038_l18"),
	NLDO1200(L19, 0x0D2, 0x0D3, LDO_1200, "8038_l19"),
	NLDO1200(L20, 0x0D4, 0x0D5, LDO_1200, "8038_l20"),
	PLDO(L21, 0x0D6, 0x0D7, LDO_150, "8038_l21"),
	PLDO(L22, 0x0D8, 0x0D9, LDO_50, "8038_l22"),
	PLDO(L23, 0x0DA, 0x0DB, LDO_50, "8038_l23"),
	NLDO1200(L24, 0x0DC, 0x0DD, LDO_1200, "8038_l24"),
	NLDO(L26, 0x0E0, 0x0E1, LDO_150, "8038_l26"),
	NLDO1200(L27, 0x0E2, 0x0E3, LDO_1200, "8038_l27"),

	
	SMPS(S1, 0x1E0, 0x1E5, 0x009, 0x1E2, SMPS_1500, "8038_s1"),
	SMPS(S2, 0x1D8, 0x1DD, 0x00A, 0x1DA, SMPS_1500, "8038_s2"),
	SMPS(S3, 0x1D0, 0x1D5, 0x00B, 0x1D2, SMPS_1500, "8038_s3"),
	SMPS(S4, 0x1E8, 0x1ED, 0x00C, 0x1EA, SMPS_1500, "8038_s4"),

	
	FTSMPS(S5, 0x025, 0x02E, 0x026, 0x032, SMPS_2000, "8038_s5"),
	FTSMPS(S6, 0x036, 0x03F, 0x037, 0x043, SMPS_2000, "8038_s6"),

	
	VS(LVS1, 0x060, "8038_lvs1"),
	VS(LVS2, 0x062, "8038_lvs2"),
};

struct pm8xxx_vreg_table {
	const char			*name;
	struct pm8xxx_vreg 	*reg_list;
	size_t				table_size;
};

static struct pm8xxx_vreg_table pm8xxx_table[] = {
	{
		.name 		= "pm8038-dbg",
		.reg_list 	= pm8038_vreg,
		.table_size	= ARRAY_SIZE(pm8038_vreg),
	},
	{
		.name 		= "pm8921-dbg",
		.reg_list 	= pm8921_vreg,
		.table_size	= ARRAY_SIZE(pm8921_vreg),
	},

	
	{
		.name 		= NULL,
		.table_size 	= -1,
	},

};

static struct pm8xxx_vreg_table *pm8xxx_current_table;

static struct dentry *debugfs_base;
static struct pm8xxx_debug_device *gdebugdev;

static int pm8xxx_table_lookup(const char *name)
{
	struct pm8xxx_vreg_table *table = &pm8xxx_table[0];

	for( ;table->table_size != -1; table++) {
		if(!strcmp(table->name, name)) {
			pm8xxx_current_table = table;
			pr_info("PM debug device %s, table size: %d found!\n"
				, name, table->table_size);
			return 0;
		}
	}
	pr_err("Can not find reg table for %s\n", name);
	return 1;
}

static int pm8xxx_vreg_is_enabled(struct pm8xxx_vreg *vreg)
{
	int rc = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_FTSMPS:
		if ((vreg->ctrl_reg & FTSMPS_VCTRL_BAND_MASK)
		    != FTSMPS_VCTRL_BAND_OFF)
			rc = 1;
		break;
	case REGULATOR_TYPE_VS300:
		if ((vreg->ctrl_reg & VS300_CTRL_ENABLE_MASK)
		    != VS300_CTRL_DISABLE)
			rc = 1;
		break;
	case REGULATOR_TYPE_SMPS:
		if (SMPS_IN_ADVANCED_MODE(vreg)) {
			if ((vreg->ctrl_reg & SMPS_ADVANCED_BAND_MASK)
			    != SMPS_ADVANCED_BAND_OFF)
				rc = 1;
			break;
		}
		
	default:
		if ((vreg->ctrl_reg & REGULATOR_ENABLE_MASK)
		    == REGULATOR_ENABLE)
			rc = 1;
	}

	return rc;
}

static int pm8xxx_vreg_is_pulldown(struct pm8xxx_vreg *vreg)
{
	int rc = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_PLDO:
	case REGULATOR_TYPE_NLDO:
		return (vreg->ctrl_reg & LDO_PULL_DOWN_ENABLE_MASK)?1:0;

	case REGULATOR_TYPE_NLDO1200:
		return (vreg->test_reg[1] & NLDO1200_PULL_DOWN_ENABLE_MASK)?1:0;

	case REGULATOR_TYPE_SMPS:
		if (!SMPS_IN_ADVANCED_MODE(vreg))
			return  (vreg->ctrl_reg & SMPS_LEGACY_PULL_DOWN_ENABLE)?1:0;
		else
			return (vreg->test_reg[6] & SMPS_ADVANCED_PULL_DOWN_ENABLE)?1:0;

	case REGULATOR_TYPE_FTSMPS:
		return (vreg->pwr_cnfg_reg & FTSMPS_PULL_DOWN_ENABLE_MASK)?1:0;

	case REGULATOR_TYPE_VS:
		return (vreg->ctrl_reg & VS_PULL_DOWN_DISABLE)?0:1;

	case REGULATOR_TYPE_VS300:
		return  (vreg->ctrl_reg & VS300_PULL_DOWN_ENABLE)?1:0;

	default:
		return rc = -EINVAL;
	}

	return rc;
}

static int _pm8xxx_nldo_get_voltage(struct pm8xxx_vreg *vreg)
{
	u8 vprog, fine_step_reg;

	fine_step_reg = vreg->test_reg[2] & LDO_TEST_FINE_STEP_MASK;
	vprog = vreg->ctrl_reg & LDO_CTRL_VPROG_MASK;

	vprog = (vprog << 1) | (fine_step_reg >> LDO_TEST_FINE_STEP_SHIFT);

	return NLDO_UV_FINE_STEP * vprog + NLDO_UV_MIN;
}

static int _pm8xxx_pldo_get_voltage(struct pm8xxx_vreg *vreg)
{
	int vmin, fine_step;
	u8 range_ext, range_sel, vprog, fine_step_reg;

	fine_step_reg = vreg->test_reg[2] & LDO_TEST_FINE_STEP_MASK;
	range_sel = vreg->test_reg[2] & LDO_TEST_RANGE_SEL_MASK;
	range_ext = vreg->test_reg[4] & LDO_TEST_RANGE_EXT_MASK;
	vprog = vreg->ctrl_reg & LDO_CTRL_VPROG_MASK;

	vprog = (vprog << 1) | (fine_step_reg >> LDO_TEST_FINE_STEP_SHIFT);

	if (range_sel) {
		
		fine_step = PLDO_LOW_UV_FINE_STEP;
		vmin = PLDO_LOW_UV_MIN;
	} else if (!range_ext) {
		
		fine_step = PLDO_NORM_UV_FINE_STEP;
		vmin = PLDO_NORM_UV_MIN;
	} else {
		
		fine_step = PLDO_HIGH_UV_FINE_STEP;
		vmin = PLDO_HIGH_UV_MIN;
	}

	return fine_step * vprog + vmin;
}

static int _pm8xxx_nldo1200_get_voltage(struct pm8xxx_vreg *vreg)
{
	int uV = 0;
	int vprog;

	if (!NLDO1200_IN_ADVANCED_MODE(vreg)) {
		pr_warn("%s: currently in legacy mode; voltage unknown.\n",
			vreg->name);
		return  -EINVAL;
	}

	vprog = vreg->ctrl_reg & NLDO1200_CTRL_VPROG_MASK;

	if ((vreg->ctrl_reg & NLDO1200_CTRL_RANGE_MASK)
	    == NLDO1200_CTRL_RANGE_LOW)
		uV = vprog * NLDO1200_LOW_UV_STEP + NLDO1200_LOW_UV_MIN;
	else
		uV = vprog * NLDO1200_HIGH_UV_STEP + NLDO1200_HIGH_UV_MIN;

	return uV;
}

static int pm8xxx_smps_get_voltage_advanced(struct pm8xxx_vreg *vreg)
{
	u8 vprog, band;
	int uV = 0;

	vprog = vreg->ctrl_reg & SMPS_ADVANCED_VPROG_MASK;
	band = vreg->ctrl_reg & SMPS_ADVANCED_BAND_MASK;

	if (band == SMPS_ADVANCED_BAND_1)
		uV = vprog * SMPS_BAND1_UV_STEP + SMPS_BAND1_UV_MIN;
	else if (band == SMPS_ADVANCED_BAND_2)
		uV = vprog * SMPS_BAND2_UV_STEP + SMPS_BAND2_UV_MIN;
	else if (band == SMPS_ADVANCED_BAND_3)
		uV = vprog * SMPS_BAND3_UV_STEP + SMPS_BAND3_UV_MIN;
	else
		uV = - EINVAL;

	return uV;
}

static int pm8xxx_smps_get_voltage_legacy(struct pm8xxx_vreg *vreg)
{
	u8 vlow, vref, vprog;
	int uV;

	vlow = vreg->test_reg[1] & SMPS_LEGACY_VLOW_SEL_MASK;
	vref = vreg->ctrl_reg & SMPS_LEGACY_VREF_SEL_MASK;
	vprog = vreg->ctrl_reg & SMPS_LEGACY_VPROG_MASK;

	if (vlow && vref) {
		
		uV = vprog * SMPS_MODE3_UV_STEP + SMPS_MODE3_UV_MIN;
	} else if (vref) {
		
		uV = vprog * SMPS_MODE2_UV_STEP + SMPS_MODE2_UV_MIN;
	} else {
		
		uV = vprog * SMPS_MODE1_UV_STEP + SMPS_MODE1_UV_MIN;
	}

	return uV;
}

static int _pm8xxx_smps_get_voltage(struct pm8xxx_vreg *vreg)
{
	if (SMPS_IN_ADVANCED_MODE(vreg))
		return pm8xxx_smps_get_voltage_advanced(vreg);

	return pm8xxx_smps_get_voltage_legacy(vreg);
}

static int _pm8xxx_ftsmps_get_voltage(struct pm8xxx_vreg *vreg)
{
	u8 vprog, band;
	int uV = 0;

	if ((vreg->test_reg[0] & FTSMPS_CNFG1_PM_MASK) == FTSMPS_CNFG1_PM_PFM) {
		vprog = vreg->pfm_ctrl_reg & FTSMPS_VCTRL_VPROG_MASK;
		band = vreg->pfm_ctrl_reg & FTSMPS_VCTRL_BAND_MASK;
		if (band == FTSMPS_VCTRL_BAND_OFF && vprog == 0) {
			
			vprog = vreg->ctrl_reg & FTSMPS_VCTRL_VPROG_MASK;
			band = vreg->ctrl_reg & FTSMPS_VCTRL_BAND_MASK;
		}
	} else {
		vprog = vreg->ctrl_reg & FTSMPS_VCTRL_VPROG_MASK;
		band = vreg->ctrl_reg & FTSMPS_VCTRL_BAND_MASK;
	}

	if (band == FTSMPS_VCTRL_BAND_1)
		uV = vprog * FTSMPS_BAND1_UV_PHYS_STEP + FTSMPS_BAND1_UV_MIN;
	else if (band == FTSMPS_VCTRL_BAND_2)
		uV = vprog * FTSMPS_BAND2_UV_STEP + FTSMPS_BAND2_UV_MIN;
	else if (band == FTSMPS_VCTRL_BAND_3)
		uV = vprog * FTSMPS_BAND3_UV_STEP + FTSMPS_BAND3_UV_MIN;
	else
		uV = -EINVAL;

	return uV;
}

static int pm8xxx_vreg_get_voltage(struct pm8xxx_vreg *vreg)
{
	int rc = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_PLDO:
		return _pm8xxx_pldo_get_voltage(vreg);

	case REGULATOR_TYPE_NLDO:
		return _pm8xxx_nldo_get_voltage(vreg);

	case REGULATOR_TYPE_NLDO1200:
		return _pm8xxx_nldo1200_get_voltage(vreg);

	case REGULATOR_TYPE_SMPS:
		return _pm8xxx_smps_get_voltage(vreg);

	case REGULATOR_TYPE_FTSMPS:
		 return _pm8xxx_ftsmps_get_voltage(vreg);

	default:
		return rc = -EINVAL;
	}

	return rc;
}

static int pm8xxx_vreg_get_mode(struct pm8xxx_vreg *vreg)
{
	int rc = 0;
	unsigned int mode = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_PLDO:
	case REGULATOR_TYPE_NLDO:
		return ((vreg->ctrl_reg & LDO_CTRL_PM_MASK)
					== LDO_CTRL_PM_LPM ?
				MODE_LDO_LPM: MODE_LDO_HPM);

	case REGULATOR_TYPE_NLDO1200:
		if (NLDO1200_IN_ADVANCED_MODE(vreg)) {
			
			if ((vreg->test_reg[2] & NLDO1200_ADVANCED_PM_MASK)
			    == NLDO1200_ADVANCED_PM_LPM)
				mode = MODE_NLDO1200_LPM;
			else
				mode = MODE_NLDO1200_HPM;
		} else {
			
			if ((vreg->ctrl_reg & NLDO1200_LEGACY_PM_MASK)
			    == NLDO1200_LEGACY_PM_LPM)
				mode = MODE_NLDO1200_LPM;
			else
				mode = MODE_NLDO1200_HPM;
		}
		return mode;

	case REGULATOR_TYPE_SMPS:
		switch (vreg->clk_ctrl_reg & SMPS_CLK_CTRL_MASK) {
			case 0x0:
				mode = MODE_SMPS_FOLLOW_TCXO;
				break;
			case 0x10:
				mode = MODE_SMPS_PWM;
				break;
			case 0x20:
				mode = MODE_SMPS_PFM;
				break;
			case 0x30:
				mode = MODE_SMPS_AUTO;
				break;
		}
		return mode;
	case REGULATOR_TYPE_FTSMPS:
		switch (vreg->test_reg[0] & FTSMPS_CNFG1_PM_MASK) {
			case 0x0:
				mode = MODE_FTSMPS_PWM;
				break;
			case 0x40:
				mode = MODE_FTSMPS_1;
				break;
			case 0x80:
				mode = MODE_FTSMPS_PFM;
				break;
			case 0xC0:
				mode = MODE_FTSMPS_3;
				break;
		}
		return mode;

	default:
		return rc = -EINVAL;
	}

	return rc;
}

static int pm8xxx_init_ldo(struct pm8xxx_vreg *vreg, bool is_real)
{
	int rc = 0;
	int i;
	u8 bank;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc)
		goto bail;

	
	for (i = 0; i < LDO_TEST_BANKS; i++) {
		bank = REGULATOR_BANK_SEL(i);
		rc = pm8xxx_writeb(gdebugdev->parent, vreg->test_addr, bank);
		if (rc)
			goto bail;

		rc = pm8xxx_readb(gdebugdev->parent, vreg->test_addr,
				  &vreg->test_reg[i]);
		if (rc)
			goto bail;
		vreg->test_reg[i] |= REGULATOR_BANK_WRITE;
	}

bail:
	if (rc)
		vreg_err(vreg, "pm8xxx_readb/writeb failed, rc=%d\n", rc);

	return rc;
}

static int pm8xxx_init_nldo1200(struct pm8xxx_vreg *vreg)
{
	int rc = 0;
	int i;
	u8 bank;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc)
		goto bail;

	
	for (i = 0; i < LDO_TEST_BANKS; i++) {
		bank = REGULATOR_BANK_SEL(i);
		rc = pm8xxx_writeb(gdebugdev->parent, vreg->test_addr, bank);
		if (rc)
			goto bail;

		rc = pm8xxx_readb(gdebugdev->parent, vreg->test_addr,
				  &vreg->test_reg[i]);
		if (rc)
			goto bail;
		vreg->test_reg[i] |= REGULATOR_BANK_WRITE;
	}
bail:
	if (rc)
		vreg_err(vreg, "pm8xxx_readb/writeb failed, rc=%d\n", rc);

	return rc;
}

static int pm8xxx_init_smps(struct pm8xxx_vreg *vreg, bool is_real)
{
	int rc = 0;
	int i;
	u8 bank;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc)
		goto bail;

	
	for (i = 0; i < SMPS_TEST_BANKS; i++) {
		bank = REGULATOR_BANK_SEL(i);
		rc = pm8xxx_writeb(gdebugdev->parent, vreg->test_addr, bank);
		if (rc)
			goto bail;

		rc = pm8xxx_readb(gdebugdev->parent, vreg->test_addr,
				  &vreg->test_reg[i]);
		if (rc)
			goto bail;
		vreg->test_reg[i] |= REGULATOR_BANK_WRITE;
	}

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->clk_ctrl_addr,
			  &vreg->clk_ctrl_reg);
	if (rc)
		goto bail;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->sleep_ctrl_addr,
			  &vreg->sleep_ctrl_reg);
	if (rc)
		goto bail;

bail:
	if (rc)
		vreg_err(vreg, "pm8xxx_readb/writeb failed, rc=%d\n", rc);

	return rc;
}

static int pm8xxx_init_ftsmps(struct pm8xxx_vreg *vreg)
{
	int rc, i;
	u8 bank;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc)
		goto bail;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->pfm_ctrl_addr,
			  &vreg->pfm_ctrl_reg);
	if (rc)
		goto bail;

	rc = pm8xxx_readb(gdebugdev->parent, vreg->pwr_cnfg_addr,
			  &vreg->pwr_cnfg_reg);
	if (rc)
		goto bail;

	
	for (i = 0; i < SMPS_TEST_BANKS; i++) {
		bank = REGULATOR_BANK_SEL(i);
		rc = pm8xxx_writeb(gdebugdev->parent, vreg->test_addr, bank);
		if (rc)
			goto bail;

		rc = pm8xxx_readb(gdebugdev->parent, vreg->test_addr,
				  &vreg->test_reg[i]);
		if (rc)
			goto bail;
		vreg->test_reg[i] |= REGULATOR_BANK_WRITE;
	}

bail:
	if (rc)
		vreg_err(vreg, "pm8xxx_readb/writeb failed, rc=%d\n", rc);

	return rc;
}

static int pm8xxx_init_vs(struct pm8xxx_vreg *vreg, bool is_real)
{
	int rc = 0;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc) {
		vreg_err(vreg, "pm8xxx_readb failed, rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int pm8xxx_init_vs300(struct pm8xxx_vreg *vreg)
{
	int rc;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc) {
		vreg_err(vreg, "pm8xxx_readb failed, rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int pm8xxx_init_ncp(struct pm8xxx_vreg *vreg)
{
	int rc;

	
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc) {
		vreg_err(vreg, "pm8xxx_readb failed, rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int pm8xxx_init_vreg(struct pm8xxx_vreg *vreg)
{
	int rc = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_PLDO:
	case REGULATOR_TYPE_NLDO:
		rc = pm8xxx_init_ldo(vreg, true);
		break;
	case REGULATOR_TYPE_NLDO1200:
		rc = pm8xxx_init_nldo1200(vreg);
		break;
	case REGULATOR_TYPE_SMPS:
		rc = pm8xxx_init_smps(vreg, true);
		break;
	case REGULATOR_TYPE_FTSMPS:
		rc = pm8xxx_init_ftsmps(vreg);
		break;
	case REGULATOR_TYPE_VS:
		rc = pm8xxx_init_vs(vreg, true);
		break;
	case REGULATOR_TYPE_VS300:
		rc = pm8xxx_init_vs300(vreg);
		break;
	case REGULATOR_TYPE_NCP:
		rc = pm8xxx_init_ncp(vreg);
		break;
	}
	return rc;
}

#define PMIC_REG_BUF_SIZE   128
int pmic8921_regs_dump(int id, struct seq_file *m, char *pmic_reg_buffer, int curr_len)
{
	u8 rc, val;
	u8 bank;
	int i, j;
	char pmic_reg_buf[PMIC_REG_BUF_SIZE];

	i = id;
	memset(pmic_reg_buf, 0, PMIC_REG_BUF_SIZE);

	if (pm8921_regs[i].multi_bank > 0)
	{
		for (j = 0, bank = 0; j < pm8921_regs[i].multi_bank; j++)
		{
			bank = REGULATOR_BANK_SEL(j);
			rc = pm8xxx_writeb(gdebugdev->parent, pm8921_regs[i].addr, bank);
			if(rc) {
				if (m)
					seq_printf(m, "[%03d] Register write fail on 0x%03X\n", i, pm8921_regs[i].addr);
				else
					pr_info("[%03d] Register write fail on 0x%03X\n", i, pm8921_regs[i].addr);

				if (pmic_reg_buffer)
				{
					sprintf(pmic_reg_buf, "[%03d] Register write fail on 0x%03X\n", i, pm8921_regs[i].addr);
					pmic_reg_buf[PMIC_REG_BUF_SIZE - 1] = '\0';
					curr_len += sprintf(pmic_reg_buffer + curr_len, pmic_reg_buf);
				}
			}

			rc = pm8xxx_readb(gdebugdev->parent, pm8921_regs[i].addr, &val);
			if(rc) {
				if (m)
					seq_printf(m, "[%03d] Register read fail on 0x%03X\n", i, pm8921_regs[i].addr);
				else
					pr_info("[%03d] Register read fail on 0x%03X\n", i, pm8921_regs[i].addr);

				if (pmic_reg_buffer)
				{
					sprintf(pmic_reg_buf, "[%03d] Register read fail on 0x%03X\n", i, pm8921_regs[i].addr);
					pmic_reg_buf[PMIC_REG_BUF_SIZE - 1] = '\0';
					curr_len += sprintf(pmic_reg_buffer + curr_len, pmic_reg_buf);
				}
			}
			else
			{
				if (m)
					seq_printf(m, "[%03d] Register 0x%03X (Bank 0x%X) = 0x%02X\n", i, pm8921_regs[i].addr, j, val);
				else
					pr_info("[%03d] Register 0x%03X (Bank 0x%X) = 0x%02X\n", i, pm8921_regs[i].addr, j, val);

				if (pmic_reg_buffer)
				{
					sprintf(pmic_reg_buf, "[%03d] Register 0x%03X (Bank 0x%X) = 0x%02X\n", i, pm8921_regs[i].addr, j, val);
					pmic_reg_buf[PMIC_REG_BUF_SIZE - 1] = '\0';
					curr_len += sprintf(pmic_reg_buffer + curr_len, pmic_reg_buf);
				}
			}
		}
	}
	else{
		rc = pm8xxx_readb(gdebugdev->parent, pm8921_regs[i].addr, &val);
		if(rc) {
			if (m)
				seq_printf(m, "[%03d] Register read fail on 0x%03X\n", i, pm8921_regs[i].addr);
			else
				pr_info("[%03d] Register read fail on 0x%03X\n", i, pm8921_regs[i].addr);

			if (pmic_reg_buffer)
			{
				sprintf(pmic_reg_buf, "[%03d] Register read fail on 0x%03X\n", i, pm8921_regs[i].addr);
				pmic_reg_buf[PMIC_REG_BUF_SIZE - 1] = '\0';
				curr_len += sprintf(pmic_reg_buffer + curr_len, pmic_reg_buf);
			}
		}
		else
		{
			if (m)
				seq_printf(m, "[%03d] Register 0x%03X = 0x%02X\n", i, pm8921_regs[i].addr, val);
			else
				pr_info("[%03d] Register 0x%03X = 0x%02X\n", i, pm8921_regs[i].addr, val);

			if (pmic_reg_buffer)
			{
				sprintf(pmic_reg_buf, "[%03d] Register 0x%03X = 0x%02X\n", i, pm8921_regs[i].addr, val);
				pmic_reg_buf[PMIC_REG_BUF_SIZE - 1] = '\0';
				curr_len += sprintf(pmic_reg_buffer + curr_len, pmic_reg_buf);
			}
		}
	}

	return curr_len;
} 


int pm8xxx_vreg_dump(int id, struct seq_file *m, char *vreg_buffer, int curr_len)
{
	int rc = 0;
	int len = 0;
	int mode = 0;
	int enable = 0;
	int voltage = 0;
	int pd = 0;
	char nam_buf[20];
	char en_buf[10];
	char mod_buf[10];
	char vol_buf[20];
	char pd_buf[10];
	char vreg_buf[128];

	struct pm8xxx_vreg *vreg;

	if (!IS_REAL_REGULATOR(id) || id == PM8xxx_VREG_ID_L5)
		return curr_len;

	memset(nam_buf,  ' ', sizeof(nam_buf));
	nam_buf[19] = 0;
	memset(en_buf, 0, sizeof(en_buf));
	memset(mod_buf, 0, sizeof(mod_buf));
	memset(vol_buf, 0, sizeof(vol_buf));
	memset(pd_buf, 0, sizeof(pd_buf));
	memset(vreg_buf, 0, sizeof(vreg_buf));

	vreg = &pm8xxx_current_table->reg_list[id];

	if (!vreg->name)
		return curr_len;

	rc = pm8xxx_init_vreg(vreg);
	if (rc)
		return curr_len;

	len = strlen(vreg->name);
	if (len > 19)
		len = 19;
	memcpy(nam_buf, vreg->name, len);

	enable =  pm8xxx_vreg_is_enabled(vreg);
	if (enable == -EINVAL)
		sprintf(en_buf, "NULL");
	else if (enable)
		sprintf(en_buf, "YES ");
	else
		sprintf(en_buf, "NO  ");

	mode = pm8xxx_vreg_get_mode(vreg);
	switch (mode) {
		case MODE_LDO_HPM:
		case MODE_NLDO1200_HPM:
			sprintf(mod_buf, "HPM ");
			break;
		case MODE_NLDO1200_LPM:
		case MODE_LDO_LPM:
			sprintf(mod_buf, "LPM ");
			break;
		case MODE_SMPS_PWM:
		case MODE_FTSMPS_PWM:
			sprintf(mod_buf, "PWM ");
			break;
		case MODE_SMPS_PFM:
		case MODE_FTSMPS_PFM:
			sprintf(mod_buf, "PFM ");
			break;
		case MODE_SMPS_FOLLOW_TCXO:
			sprintf(mod_buf, "TCXO");
			break;
		case MODE_SMPS_AUTO:
			sprintf(mod_buf, "AUTO");
			break;
		case MODE_FTSMPS_1:
			sprintf(mod_buf, "0x40");
			break;
		case MODE_FTSMPS_3:
			sprintf(mod_buf, "0x80");
			break;
		default:
			sprintf(mod_buf, "NULL");
	}

	voltage = pm8xxx_vreg_get_voltage(vreg);
	if (voltage == -EINVAL)
		sprintf(vol_buf, "NULL");
	else
		sprintf(vol_buf, "%d uV", voltage);

	pd = pm8xxx_vreg_is_pulldown(vreg);
	if (pd == -EINVAL)
		sprintf(pd_buf, "NULL");
	else if (pd)
		sprintf(pd_buf, "YES ");
	else
		sprintf(pd_buf, "NO  ");

	if (m)
		seq_printf(m, "VREG %s: [Enable]%s, [Mode]%s, [PD]%s, [Vol]%s\n", nam_buf, en_buf,  mod_buf, pd_buf, vol_buf);
	else
		pr_info("VREG %s: [Enable]%s, [Mode]%s, [PD]%s, [Vol]%s\n", nam_buf, en_buf,  mod_buf, pd_buf, vol_buf);

	if (vreg_buffer)
	{
		sprintf(vreg_buf, "VREG %s: [Enable]%s, [Mode]%s, [PD]%s, [Vol]%s\n", nam_buf, en_buf,  mod_buf, pd_buf, vol_buf);
		vreg_buf[127] = '\0';
		curr_len += sprintf(vreg_buffer + curr_len, vreg_buf);
	}
	return curr_len;
}

int pmic_vreg_dump(char *vreg_buffer, int curr_len)
{
	int i;
	char *title_msg = "------------ PMIC VREG -------------\n";

	if (vreg_buffer)
		curr_len += sprintf(vreg_buffer + curr_len,
			"%s\n", title_msg);

	pr_info("%s", title_msg);

	for (i = 0; i < pm8xxx_current_table->table_size; i++)
		curr_len = pm8xxx_vreg_dump(i, NULL, vreg_buffer, curr_len);

	return curr_len;
}

static int list_vregs_show(struct seq_file *m, void *unused)
{
	int i;
	char *title_msg = "------------ PMIC VREG -------------\n";

	if (m)
		seq_printf(m, title_msg);

	for (i = 0; i < pm8xxx_current_table->table_size; i++)
		pm8xxx_vreg_dump(i, m, NULL, 0);

	return 0;
}

int pmic_suspend_reg_dump(char *pmic_reg_buffer, int curr_len)
{
	int i;
	char *title_msg = "------------ PMIC REGS -------------\n";

	if (pmic_reg_buffer)
		curr_len += sprintf(pmic_reg_buffer + curr_len,
			"%s\n", title_msg);

	pr_info("%s", title_msg);

	for (i = 0; pm8921_regs[i].multi_bank >= 0; i++) {
		curr_len = pmic8921_regs_dump(i, NULL, pmic_reg_buffer, curr_len);
	}

	return curr_len;
}

static int list_pmic_regs_show(struct seq_file *m, void *unused)
{
	int i;
	char *title_msg = "------------ PMIC REGS -------------\n";

	if (m)
		seq_printf(m, title_msg);

	for (i = 0; pm8921_regs[i].multi_bank >= 0; i++) {
		pmic8921_regs_dump(i, m, NULL, 0);
	}

	return 0;
} 

extern int print_vreg_buffer(struct seq_file *m);
extern int free_vreg_buffer(void);

static int list_sleep_vregs_show(struct seq_file *m, void *unused)
{
	print_vreg_buffer(m);
	return 0;
}

static int list_vregs_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_vregs_show, inode->i_private);
}

static int list_sleep_vregs_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_sleep_vregs_show, inode->i_private);
}

static int list_sleep_vregs_release(struct inode *inode, struct file *file)
{
	free_vreg_buffer();
	return single_release(inode, file);
}

extern int print_pmic_reg_buffer(struct seq_file *m);
extern int free_pmic_reg_buffer(void);

static int list_sleep_pmic_regs_show(struct seq_file *m, void *unused)
{
	print_pmic_reg_buffer(m);
	return 0;
}

static int list_pmic_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_pmic_regs_show, inode->i_private);
}

static int list_sleep_pmic_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_sleep_pmic_regs_show, inode->i_private);
}

static int list_sleep_pmic_regs_release(struct inode *inode, struct file *file)
{
	free_pmic_reg_buffer();
	return single_release(inode, file);
}

static const struct file_operations list_vregs_fops = {
	.open		= list_vregs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static const struct file_operations list_sleep_vregs_fops = {
	.open		= list_sleep_vregs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= list_sleep_vregs_release,
};

static const struct file_operations list_pmic_regs_fops = {
	.open		= list_pmic_regs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static const struct file_operations list_sleep_pmic_regs_fops = {
	.open		= list_sleep_pmic_regs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= list_sleep_pmic_regs_release,
};

int pm8xxx_vreg_status_init(struct pm8xxx_debug_device *dev)
{
	int err = 0;

	debugfs_base = debugfs_create_dir("pm8xxx-vreg", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	if (!debugfs_create_file("list_vregs", S_IRUGO, debugfs_base,
				pm8xxx_current_table->reg_list, &list_vregs_fops))
		return -ENOMEM;

	if (!debugfs_create_file("list_sleep_vregs", S_IRUGO, debugfs_base,
				pm8xxx_current_table->reg_list, &list_sleep_vregs_fops))
		return -ENOMEM;

	
	if (!debugfs_create_file("list_pmic_regs", S_IRUGO, debugfs_base,
				pm8xxx_current_table->reg_list, &list_pmic_regs_fops))
		return -ENOMEM;

	if (!debugfs_create_file("list_sleep_pmic_regs", S_IRUGO, debugfs_base,
				pm8xxx_current_table->reg_list, &list_sleep_pmic_regs_fops))
		return -ENOMEM;
	

	gdebugdev = dev;
	return err;
}

static int __devinit pm8xxx_debug_probe(struct platform_device *pdev)
{
	char *name = pdev->dev.platform_data;
	struct pm8xxx_debug_device *debugdev;
#if 0 
	struct dentry *dir;
	struct dentry *temp;
#endif
	int rc;

	if (name == NULL) {
		pr_err("debugfs directory name must be specified in "
			"platform_data pointer\n");
		return -EINVAL;
	}

	debugdev = kzalloc(sizeof(struct pm8xxx_debug_device), GFP_KERNEL);
	if (debugdev == NULL) {
		pr_err("kzalloc failed\n");
		return -ENOMEM;
	}

	rc = pm8xxx_table_lookup(name);
	if (rc) {
		pr_err("Can not find pm8xxx table(%s)\n", name);
		goto dir_error;
	}

	debugdev->parent = pdev->dev.parent;
	debugdev->addr = -1;
#if 0 
	dir = debugfs_create_dir(name, NULL);
	if (dir == NULL || IS_ERR(dir)) {
		pr_err("debugfs_create_dir failed: rc=%ld\n", PTR_ERR(dir));
		rc = PTR_ERR(dir);
		goto dir_error;
	}

	temp = debugfs_create_file("addr", S_IRUSR | S_IWUSR, dir, debugdev,
				   &debug_addr_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		rc = PTR_ERR(temp);
		goto file_error;
	}

	temp = debugfs_create_file("data", S_IRUSR | S_IWUSR, dir, debugdev,
				   &debug_data_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		rc = PTR_ERR(temp);
		goto file_error;
	}
#endif
	pm8xxx_vreg_status_init(debugdev);
	mutex_init(&debugdev->debug_mutex);
#if 0 
	debugdev->dir = dir;
#endif
	platform_set_drvdata(pdev, debugdev);

	return 0;
#if 0 
file_error:
	debugfs_remove_recursive(dir);
#endif
dir_error:
	kfree(debugdev);

	return rc;
}

static int __devexit pm8xxx_debug_remove(struct platform_device *pdev)
{
	struct pm8xxx_debug_device *debugdev = platform_get_drvdata(pdev);

	if (debugdev) {
#if 0 
		debugfs_remove_recursive(debugdev->dir);
#endif
		mutex_destroy(&debugdev->debug_mutex);
		kfree(debugdev);
	}

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver pm8xxx_debug_driver = {
	.probe		= pm8xxx_debug_probe,
	.remove		= __devexit_p(pm8xxx_debug_remove),
	.driver		= {
		.name	= PM8XXX_DEBUG_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8xxx_debug_init(void)
{
	return platform_driver_register(&pm8xxx_debug_driver);
}
subsys_initcall(pm8xxx_debug_init);

static void __exit pm8xxx_debug_exit(void)
{
	platform_driver_unregister(&pm8xxx_debug_driver);
}
module_exit(pm8xxx_debug_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PM8XXX Debug driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:" PM8XXX_DEBUG_DEV_NAME);
