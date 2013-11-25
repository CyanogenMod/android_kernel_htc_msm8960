/*
 $License:
    Copyright (C) 2010 InvenSense Corporation, All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
  $
 */

#ifndef __MPU3050_H_
#define __MPU3050_H_

#ifdef __KERNEL__
#include <linux/types.h>
#endif

#ifdef M_HW
#error MPU6000 build including MPU3050 header
#endif

#define MPU_NAME "mpu3050"
#define DEFAULT_MPU_SLAVEADDR       0x68

enum mpu_register {
	MPUREG_WHO_AM_I = 0,	
	MPUREG_PRODUCT_ID,	
	MPUREG_02_RSVD,		
	MPUREG_03_RSVD,		
	MPUREG_04_RSVD,		
	MPUREG_XG_OFFS_TC,	
	MPUREG_06_RSVD,		
	MPUREG_07_RSVD,		
	MPUREG_YG_OFFS_TC,	
	MPUREG_09_RSVD,		
	MPUREG_0A_RSVD,		
	MPUREG_ZG_OFFS_TC,	
	MPUREG_X_OFFS_USRH,	
	MPUREG_X_OFFS_USRL,	
	MPUREG_Y_OFFS_USRH,	
	MPUREG_Y_OFFS_USRL,	
	MPUREG_Z_OFFS_USRH,	
	MPUREG_Z_OFFS_USRL,	
	MPUREG_FIFO_EN1,	
	MPUREG_FIFO_EN2,	
	MPUREG_AUX_SLV_ADDR,	
	MPUREG_SMPLRT_DIV,	
	MPUREG_DLPF_FS_SYNC,	
	MPUREG_INT_CFG,		
	MPUREG_ACCEL_BURST_ADDR,
	MPUREG_19_RSVD,		
	MPUREG_INT_STATUS,	
	MPUREG_TEMP_OUT_H,	
	MPUREG_TEMP_OUT_L,	
	MPUREG_GYRO_XOUT_H,	
	MPUREG_GYRO_XOUT_L,	
	MPUREG_GYRO_YOUT_H,	
	MPUREG_GYRO_YOUT_L,	
	MPUREG_GYRO_ZOUT_H,	
	MPUREG_GYRO_ZOUT_L,	
	MPUREG_23_RSVD,		
	MPUREG_24_RSVD,		
	MPUREG_25_RSVD,		
	MPUREG_26_RSVD,		
	MPUREG_27_RSVD,		
	MPUREG_28_RSVD,		
	MPUREG_29_RSVD,		
	MPUREG_2A_RSVD,		
	MPUREG_2B_RSVD,		
	MPUREG_2C_RSVD,		
	MPUREG_2D_RSVD,		
	MPUREG_2E_RSVD,		
	MPUREG_2F_RSVD,		
	MPUREG_30_RSVD,		
	MPUREG_31_RSVD,		
	MPUREG_32_RSVD,		
	MPUREG_33_RSVD,		
	MPUREG_34_RSVD,		
	MPUREG_DMP_CFG_1,	
	MPUREG_DMP_CFG_2,	
	MPUREG_BANK_SEL,	
	MPUREG_MEM_START_ADDR,	
	MPUREG_MEM_R_W,		
	MPUREG_FIFO_COUNTH,	
	MPUREG_FIFO_COUNTL,	
	MPUREG_FIFO_R_W,	
	MPUREG_USER_CTRL,	
	MPUREG_PWR_MGM,		
	MPUREG_3F_RSVD,		
	NUM_OF_MPU_REGISTERS	
};


#define BIT_TEMP_OUT                0x80
#define BIT_GYRO_XOUT               0x40
#define BIT_GYRO_YOUT               0x20
#define BIT_GYRO_ZOUT               0x10
#define BIT_ACCEL_XOUT              0x08
#define BIT_ACCEL_YOUT              0x04
#define BIT_ACCEL_ZOUT              0x02
#define BIT_AUX_1OUT                0x01
#define BIT_AUX_2OUT                0x02
#define BIT_AUX_3OUT                0x01
#define BITS_EXT_SYNC_NONE          0x00
#define BITS_EXT_SYNC_TEMP          0x20
#define BITS_EXT_SYNC_GYROX         0x40
#define BITS_EXT_SYNC_GYROY         0x60
#define BITS_EXT_SYNC_GYROZ         0x80
#define BITS_EXT_SYNC_ACCELX        0xA0
#define BITS_EXT_SYNC_ACCELY        0xC0
#define BITS_EXT_SYNC_ACCELZ        0xE0
#define BITS_EXT_SYNC_MASK          0xE0
#define BITS_FS_250DPS              0x00
#define BITS_FS_500DPS              0x08
#define BITS_FS_1000DPS             0x10
#define BITS_FS_2000DPS             0x18
#define BITS_FS_MASK                0x18
#define BITS_DLPF_CFG_256HZ_NOLPF2  0x00
#define BITS_DLPF_CFG_188HZ         0x01
#define BITS_DLPF_CFG_98HZ          0x02
#define BITS_DLPF_CFG_42HZ          0x03
#define BITS_DLPF_CFG_20HZ          0x04
#define BITS_DLPF_CFG_10HZ          0x05
#define BITS_DLPF_CFG_5HZ           0x06
#define BITS_DLPF_CFG_2100HZ_NOLPF  0x07
#define BITS_DLPF_CFG_MASK          0x07
#define BIT_ACTL                    0x80
#define BIT_ACTL_LOW                0x80
#define BIT_ACTL_HIGH               0x00
#define BIT_OPEN                    0x40
#define BIT_OPEN_DRAIN              0x40
#define BIT_PUSH_PULL               0x00
#define BIT_LATCH_INT_EN            0x20
#define BIT_LATCH_INT_EN            0x20
#define BIT_INT_PULSE_WIDTH_50US    0x00
#define BIT_INT_ANYRD_2CLEAR        0x10
#define BIT_INT_STAT_READ_2CLEAR    0x00
#define BIT_MPU_RDY_EN              0x04
#define BIT_DMP_INT_EN              0x02
#define BIT_RAW_RDY_EN              0x01
#define BIT_INT_STATUS_FIFO_OVERLOW 0x80
#define BIT_MPU_RDY                 0x04
#define BIT_DMP_INT                 0x02
#define BIT_RAW_RDY                 0x01
#define BIT_PRFTCH_EN               0x20
#define BIT_CFG_USER_BANK           0x10
#define BITS_MEM_SEL                0x0f
#define BIT_DMP_EN                  0x80
#define BIT_FIFO_EN                 0x40
#define BIT_AUX_IF_EN               0x20
#define BIT_AUX_RD_LENG             0x10
#define BIT_AUX_IF_RST              0x08
#define BIT_DMP_RST                 0x04
#define BIT_FIFO_RST                0x02
#define BIT_GYRO_RST                0x01
#define BIT_H_RESET                 0x80
#define BIT_SLEEP                   0x40
#define BIT_STBY_XG                 0x20
#define BIT_STBY_YG                 0x10
#define BIT_STBY_ZG                 0x08
#define BITS_CLKSEL                 0x07

#define MPU_SILICON_REV_A4           1	
#define MPU_SILICON_REV_B1           2	
#define MPU_SILICON_REV_B4           3	
#define MPU_SILICON_REV_B6           4	

#define MPU_MEM_BANK_SIZE           (256)
#define FIFO_HW_SIZE                (512)

enum MPU_MEMORY_BANKS {
	MPU_MEM_RAM_BANK_0 = 0,
	MPU_MEM_RAM_BANK_1,
	MPU_MEM_RAM_BANK_2,
	MPU_MEM_RAM_BANK_3,
	MPU_MEM_NUM_RAM_BANKS,
	MPU_MEM_OTP_BANK_0 = MPU_MEM_NUM_RAM_BANKS,
	
	MPU_MEM_NUM_BANKS
};

#define MPU_NUM_AXES (3)

enum mpu_filter {
	MPU_FILTER_256HZ_NOLPF2 = 0,
	MPU_FILTER_188HZ,
	MPU_FILTER_98HZ,
	MPU_FILTER_42HZ,
	MPU_FILTER_20HZ,
	MPU_FILTER_10HZ,
	MPU_FILTER_5HZ,
	MPU_FILTER_2100HZ_NOLPF,
	NUM_MPU_FILTER
};

enum mpu_fullscale {
	MPU_FS_250DPS = 0,
	MPU_FS_500DPS,
	MPU_FS_1000DPS,
	MPU_FS_2000DPS,
	NUM_MPU_FS
};

enum mpu_clock_sel {
	MPU_CLK_SEL_INTERNAL = 0,
	MPU_CLK_SEL_PLLGYROX,
	MPU_CLK_SEL_PLLGYROY,
	MPU_CLK_SEL_PLLGYROZ,
	MPU_CLK_SEL_PLLEXT32K,
	MPU_CLK_SEL_PLLEXT19M,
	MPU_CLK_SEL_RESERVED,
	MPU_CLK_SEL_STOP,
	NUM_CLK_SEL
};

enum mpu_ext_sync {
	MPU_EXT_SYNC_NONE = 0,
	MPU_EXT_SYNC_TEMP,
	MPU_EXT_SYNC_GYROX,
	MPU_EXT_SYNC_GYROY,
	MPU_EXT_SYNC_GYROZ,
	MPU_EXT_SYNC_ACCELX,
	MPU_EXT_SYNC_ACCELY,
	MPU_EXT_SYNC_ACCELZ,
	NUM_MPU_EXT_SYNC
};

#define DLPF_FS_SYNC_VALUE(ext_sync, full_scale, lpf) \
    ((ext_sync << 5) | (full_scale << 3) | lpf)

#endif				
