/* include/linux/ewtzmu2.h - EWTZMU compass driver
 *
 * Copyright (C) 2011 Prolific Technology Inc.
 * Author: Kyle Chen
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

#ifndef EWTZMU2_H
#define EWTZMU2_H

#include <linux/ioctl.h>

#define EWTZMU_I2C_ADDRESS 			0x69
#define EWTZMU_REG_GYROX_H          0xFB

#define EWTZMU_REG_PWR_MGM          0x3E
#define EWTZMU_SLEP                 0x00
#define EWTZMU_POWERON              0x40

#define EWTZMU_INT                  0x17
#define EWTZMU_WTMON	            0x08

#define EWTZMU_DLPF                 0x16
#define EWTZMU_2000ds_1khz          0x1A
#define EWTZMU_HPF		            0x80

#define EWTZMU_SMPL         		0x15
#define EWTZMU_100hz		        0x09  

#define EWTZMU_FIFO_CTR             0x14
#define EWTZMU_stream		        0x40
#define EWTZMU_SAMPLE_100HZ			0x00
#define EWTZMU_SAMPLE_50HZ			0x01   
#define EWTZMU_SAMPLE_20HZ			0x04
#define EWTZMU_SAMPLE_16HZ			0x05
#define EWTZMU_SAMPLE_5HZ			0x13

#define EWTZMU_FIFO_STS				0x13
#define EWTZMU_PNT_H				0x02
#define	EWTZMU_PNT_L				0x00

#define EWIO						        0x83
#define EW_IOCTL_SET_INIT              	    _IO(EWIO, 0x01)
#define EW_IOCTL_SET_STANDBY				_IO(EWIO, 0x02)
#define EW_IOCTL_READ_CHIPINFO         		_IOR(EWIO, 0x03, int)
#define EW_IOCTL_READ_SENSORDATA       		_IOR(EWIO, 0x04, int)
#define EW_IOCTL_READ_SENSORDATA_FIFO       _IOR(EWIO, 0x05, int)
#define EW_IOCTL_READ_POSTUREDATA      		_IOR(EWIO, 0x06, int)
#define EW_IOCTL_WRITE_POSTUREDATA     		_IOW(EWIO, 0x07, int)
#define EW_IOCTL_READ_CALIDATA         		_IOR(EWIO, 0x08, int)
#define EW_IOCTL_WRITE_CALIDATA        		_IOW(EWIO, 0x09, int)
#define EW_IOCTL_READ_GYRODATA         		_IOR(EWIO, 0x0A, int)
#define EW_IOCTL_WRITE_GYRODATA        		_IOW(EWIO, 0x0B, int)
#define EW_IOCTL_READ_PEDODATA         		_IOR(EWIO, 0x0C, long)
#define EW_IOCTL_WRITE_PEDODATA        		_IOW(EWIO, 0x0D, long)
#define EW_IOCTL_READ_PEDOPARAM        		_IOR(EWIO, 0x0E, int)
#define EW_IOCTL_WRITE_PEDOPARAM       		_IOW(EWIO, 0x0F, int)
#define EW_IOCTL_READ_CONTROL          		_IOR(EWIO, 0x10, int)
#define EW_IOCTL_WRITE_CONTROL         		_IOW(EWIO, 0x11, int)
#define EW_IOCTL_WRITE_MODE            		_IOW(EWIO, 0x12, int)
#define EW_IOCTL_WRITE_REPORT          		_IO(EWIO, 0x13)
#define EW_IOCTL_READ_WIA			   		_IOR(EWIO, 0x14, int)
#define EW_IOCTL_READ_AXISINTERFERENCE		_IOR(EWIO, 0x15, int)
#define EW_IOCTL_GET_DIRPOLARITY			_IOR(EWIO, 0x16, int)
#define EW_IOCTL_READ_ROTATION_VECTOR		_IOR(EWIO, 0x17, int)
#define EW_IOCTL_WRITE_ROTATION_VECTOR		_IOW(EWIO, 0x18, int)
#define EW_IOCTL_READ_LINEAR_ACCEL			_IOR(EWIO, 0x19, int)
#define EW_IOCTL_WRITE_LINEAR_ACCEL			_IOW(EWIO, 0x1A, int)
#define EW_IOCTL_READ_GRAVITY				_IOR(EWIO, 0x1B, int)
#define EW_IOCTL_WRITE_GRAVITY				_IOW(EWIO, 0x1C, int)
#define EW_IOCTL_SET_SAMPLERATE				_IOW(EWIO, 0x1D, int)

#define EW_IOCTL_WRITE_I2CDATA				_IOW(EWIO, 0x1E, int)
#define EW_IOCTL_WRITE_I2CADDR				_IOW(EWIO, 0x1F, int)
#define EW_IOCTL_READ_I2CDATA				_IOR(EWIO, 0x20, int)

#define EWDAEIO						       0x84
#define EWDAE_IOCTL_SET_INIT				_IO(EWDAEIO, 0x01)
#define EWDAE_IOCTL_SET_STANDBY				_IO(EWDAEIO, 0x02)
#define EWDAE_IOCTL_GET_SENSORDATA     		_IOR(EWDAEIO, 0x03, int)
#define EWDAE_IOCTL_GET_SENSORDATA_FIFO     _IOR(EWDAEIO, 0x13, int)
#define EWDAE_IOCTL_SET_POSTURE        		_IOW(EWDAEIO, 0x04, int)
#define EWDAE_IOCTL_SET_CALIDATA       		_IOW(EWDAEIO, 0x05, int)
#define EWDAE_IOCTL_SET_GYRODATA       		_IOW(EWDAEIO, 0x06, int)
#define EWDAE_IOCTL_SET_PEDODATA       		_IOW(EWDAEIO, 0x07, long)
#define EWDAE_IOCTL_GET_PEDOPARAM      		_IOR(EWDAEIO, 0x08, int)
#define EWDAE_IOCTL_SET_PEDOPARAM      		_IOR(EWDAEIO, 0x09, int)
#define EWDAE_IOCTL_SET_CONTROL        		_IOW(EWDAEIO, 0x0A, int)
#define EWDAE_IOCTL_GET_CONTROL        		_IOR(EWDAEIO, 0x0B, int)
#define EWDAE_IOCTL_SET_MODE           		_IOW(EWDAEIO, 0x0C, int)
#define EWDAE_IOCTL_SET_REPORT         		_IO(EWDAEIO, 0x0D)
#define EWDAE_IOCTL_GET_WIA			   		_IOR(EWDAEIO, 0x0E, int)
#define EWDAE_IOCTL_GET_AXISINTERFERENCE	_IOR(EWDAEIO, 0x0F, int)
#define EWDAE_IOCTL_SET_SAMPLERATE			_IOW(EWDAEIO, 0x10, int)
#define EWDAE_IOCTL_GET_DIRPOLARITY			_IOR(EWDAEIO, 0x11, int)
#define EWDAE_IOCTL_GET_AKM_DATA			_IOR(EWDAEIO, 0x12, short[12])
#define EWDAE_IOCTL_SET_ROTATION_VECTOR		_IOW(EWDAEIO, 0x14, int)
#define EWDAE_IOCTL_SET_LINEAR_ACCEL		_IOW(EWDAEIO, 0x15, int)
#define EWDAE_IOCTL_SET_GRAVITY				_IOW(EWDAEIO, 0x16, int)
#define EWDAE_IOCTL_GET_AKM_READY			_IOR(EWDAEIO, 0x17, int)
#define EWDAE_IOCTL_GET_GYRO_CAL_DATA			_IOR(EWDAEIO, 0x18, unsigned char[12])

#define EWDAE_IOCTL_WRITE_I2CDATA			_IOW(EWDAEIO, 0x19, int)
#define EWDAE_IOCTL_WRITE_I2CADDR			_IOW(EWDAEIO, 0x1A, int)
#define EWDAE_IOCTL_READ_I2CDATA			_IOR(EWDAEIO, 0x1B, int)


#define EWHALIO						   0x85
#define EWHAL_IOCTL_GET_SENSORDATA     _IOR(EWHALIO, 0x01, int)
#define EWHAL_IOCTL_GET_POSTURE        _IOR(EWHALIO, 0x02, int)
#define EWHAL_IOCTL_GET_CALIDATA       _IOR(EWHALIO, 0x03, int)
#define EWHAL_IOCTL_GET_GYRODATA       _IOR(EWHALIO, 0x04, int)
#define EWHAL_IOCTL_GET_PEDODATA       _IOR(EWHALIO, 0x05, long)
#define EWHAL_IOCTL_GET_PEDOPARAM      _IOR(EWHALIO, 0x06, int)
#define EWHAL_IOCTL_SET_PEDOPARAM      _IOW(EWHALIO, 0x07, int)
#define EWHAL_IOCTL_GET_CONTROL        _IOR(EWHALIO, 0x08, int)
#define EWHAL_IOCTL_SET_CONTROL        _IOW(EWHALIO, 0x09, int)
#define EWHAL_IOCTL_GET_WIA			   _IOR(EWHALIO, 0x0A, int)
#define EWHAL_IOCTL_GET_ROTATION_VECTOR	 _IOR(EWHALIO, 0x0B, int)
#define EWHAL_IOCTL_GET_LINEAR_ACCEL	 _IOR(EWHALIO, 0x0C, int)
#define EWHAL_IOCTL_GET_GRAVITY			 _IOR(EWHALIO, 0x0D, int)

#define EW_CHIPSET				    0
#define EW_BUFSIZE				    256
#define EW_AXIS_INTERFERENCE	    127
#define EW_NORMAL_MODE			    0
#define EW_DEFAULT_POLLING_TIME     200
#define EW_REPORT_EN_COMPASS        1
#define EW_REPORT_EN_GYROSCOPE      2

#define EW_CB_LENGTH			10
#define EW_CB_LOOPDELAY			 0
#define EW_CB_RUN				 1
#define EW_CB_ACCCALI			 2
#define EW_CB_MAGCALI			 3
#define EW_CB_ACTIVESENSORS		 4
#define EW_CB_PD_RESET			 5
#define EW_CB_PD_EN_PARAM		 6
#define EW_CB_GYROCALI		     7
#define EW_CB_ALGORITHMLOG		 8
#define EW_CB_UNDEFINE_1		 9

#define EW_DP_LENGTH			 6
#define EW_DP_ACC_DIR			 0
#define EW_DP_ACC_POLARITY		 1
#define EW_DP_MAG_DIR			 2
#define EW_DP_MAG_POLARITY		 3
#define EW_DP_GYRO_DIR			 4
#define EW_DP_GYRO_POLARITY		 5

#define EW_PD_LENGTH			10
#define EW_PD_PRARM_IIR1		 0
#define EW_PD_PRARM_IIR2		 1
#define EW_PD_PRARM_IIR3		 2
#define EW_PD_PRARM_IIR4		 3
#define EW_PD_PRARM_TH1			 4
#define EW_PD_PRARM_TH2			 5
#define EW_PD_PRARM_TH3			 6
#define EW_PD_PRARM_TH4			 7
#define EW_PD_UNDEFINE_1		 8
#define EW_PD_UNDEFINE_2		 9

#define EW_ACCELEROMETER_SENSOR	    0
#define EW_MAGNETIC_FIELD_SENSOR	1
#define EW_ORIENTATION_SENSOR		2
#define EW_ROTATION_VECTOR			3
#define EW_LINEAR_ACCELERATION		4
#define EW_GRAVITY					5
#define EW_GYROSCOPE_SENSOR	        6
#define EW_PEDOMETER_SENSOR	        9

#define EW_BIT_ACCELEROMETER		(1<<EW_ACCELEROMETER_SENSOR)
#define EW_BIT_MAGNETIC_FIELD		(1<<EW_MAGNETIC_FIELD_SENSOR)
#define EW_BIT_ORIENTATION		    (1<<EW_ORIENTATION_SENSOR)
#define EW_BIT_ROTATION_VECTOR		(1<<EW_ROTATION_VECTOR)
#define EW_BIT_LINEAR_ACCELERATION	(1<<EW_LINEAR_ACCELERATION)
#define EW_BIT_GRAVITY				(1<<EW_GRAVITY)
#define EW_BIT_GYROSCOPE			(1<<EW_GYROSCOPE_SENSOR)
#define EW_BIT_PEDOMETER			(1<<EW_PEDOMETER_SENSOR)

struct pana_gyro_platform_data {
	int reset_line;
	int reset_asserted;
	int gpio_data_ready_int;
	int acc_dir;
	int	acc_polarity;
	int gyro_dir;
	int gyro_polarity;
	int mag_dir;
	int mag_polarity;
	int sleep_pin;
	void (*config_gyro_diag_gpios)(bool enable);
};
extern unsigned char gyro_gsensor_kvalue[37];

#endif
