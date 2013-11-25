/* arch/arm/mach-msm/panel_id.h
 * Copyright (C) 2010 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef __PANEL_ID_H
#define __PANEL_ID_H

extern int panel_type;


#define BL_SHIFT	16
#define BL_MASK		(0x7 << BL_SHIFT)

#define BL_SPI		(0 << BL_SHIFT)
#define BL_MDDI		(1 << BL_SHIFT)
#define BL_I2C		(2 << BL_SHIFT)
#define BL_UP		(3 << BL_SHIFT)
#define BL_MIPI		(4 << BL_SHIFT)
#define BL_PMIC		(5 << BL_SHIFT)

#define IF_SHIFT	19
#define IF_MASK		(0x7 << IF_SHIFT)

#define IF_LCDC		(0 << IF_SHIFT)
#define IF_MDDI		(1 << IF_SHIFT)
#define IF_MIPI		(2 << IF_SHIFT)

#define DEPTH_SHIFT	22
#define DEPTH_MASK	(0x7 << DEPTH_SHIFT)

#define DEPTH_RGB565	(0 << DEPTH_SHIFT)
#define DEPTH_RGB666	(1 << DEPTH_SHIFT)
#define DEPTH_RGB888	(2 << DEPTH_SHIFT)


#define REV_SHIFT	25
#define REV_MASK	(0x7 << REV_SHIFT)

#define REV_0		(0 << REV_SHIFT)
#define REV_1		(1 << REV_SHIFT)
#define REV_2		(2 << REV_SHIFT)
#define REV_3		(3 << REV_SHIFT)
#define REV_4		(4 << REV_SHIFT)

#define MIPI_MODE_SHIFT	28
#define MIPI_MODE_MASK	(0x3 << MIPI_MODE_SHIFT)

#define MIPI_CMD	(0 << MIPI_MODE_SHIFT)
#define MIPI_VIDEO_ONLY	(1 << MIPI_MODE_SHIFT)

#define PANEL_ID_NONE		(0x0)

#define	PANEL_ID_START		0x0F

#define PANEL_ID_DLXJ_SHARP_RENESAS  (0x63 | BL_MIPI | IF_MIPI | DEPTH_RGB888)
#define PANEL_ID_DLXJ_SONY_RENESAS  (0x64 | BL_MIPI | IF_MIPI | DEPTH_RGB888)
#define PANEL_ID_M7_JDI_SAMSUNG  (0x65 | BL_MIPI | IF_MIPI | DEPTH_RGB888)
#define PANEL_ID_M7_JDI_SAMSUNG_C2  (0x65 | BL_MIPI | IF_MIPI | DEPTH_RGB888 | REV_1)
#define PANEL_ID_M7_JDI_SAMSUNG_C2_1  (0x65 | BL_MIPI | IF_MIPI | DEPTH_RGB888 | REV_2)
#define PANEL_ID_M7_JDI_SAMSUNG_C2_2  (0x65 | BL_MIPI | IF_MIPI | DEPTH_RGB888 | REV_3)
#define PANEL_ID_M7_SHARP_NT  (0x66 | BL_MIPI | IF_MIPI | DEPTH_RGB888)
#define PANEL_ID_M7_SHARP_RENESAS  (0x67 | BL_MIPI | IF_MIPI | DEPTH_RGB888)
#define PANEL_ID_M7_SHARP_RENESAS_C1  (0X69 | BL_MIPI | IF_MIPI | DEPTH_RGB888)
#define PANEL_ID_M7_JDI_RENESAS (0x74 | BL_MIPI | IF_MIPI | DEPTH_RGB888)

#define	PANEL_ID_END		0xFFFF

#endif	

