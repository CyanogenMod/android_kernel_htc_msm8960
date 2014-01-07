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



#include <stddef.h>

#include "mldl_cfg.h"
#include "mpu.h"

#include "mlsl.h"
#include "mlos.h"

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "mldl_cfg:"

#ifdef M_HW
#define SLEEP   0
#define WAKE_UP 7
#define RESET   1
#define STANDBY 1
#else
#define SLEEP   1
#define WAKE_UP 0
#define RESET   1
#define STANDBY 1
#endif

#define D(x...) printk(KERN_DEBUG "[GYRO][MPU3050] " x)
#define I(x...) printk(KERN_INFO "[GYRO][MPU3050] " x)
#define E(x...) printk(KERN_ERR "[GYRO][MPU3050 ERROR] " x)



static int dmp_stop(struct mldl_cfg *mldl_cfg, void *gyro_handle)
{
	unsigned char userCtrlReg = 0;
	int result;

	if (!mldl_cfg->dmp_is_running)
		return ML_SUCCESS;

	result = MLSLSerialRead(gyro_handle, mldl_cfg->addr,
				MPUREG_USER_CTRL, 1, &userCtrlReg);
	ERROR_CHECK(result);
	userCtrlReg = (userCtrlReg & (~BIT_FIFO_EN)) | BIT_FIFO_RST;
	userCtrlReg = (userCtrlReg & (~BIT_DMP_EN)) | BIT_DMP_RST;

	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				       MPUREG_USER_CTRL, userCtrlReg);
	ERROR_CHECK(result);
	mldl_cfg->dmp_is_running = 0;

	return result;

}
static int dmp_start(struct mldl_cfg *pdata, void *mlsl_handle)
{
	unsigned char userCtrlReg = 0;
	int result;

	if (pdata->dmp_is_running == pdata->dmp_enable)
		return ML_SUCCESS;

	result = MLSLSerialRead(mlsl_handle, pdata->addr,
				MPUREG_USER_CTRL, 1, &userCtrlReg);
	ERROR_CHECK(result);

	result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
				       MPUREG_USER_CTRL,
				       ((userCtrlReg & (~BIT_FIFO_EN))
						|   BIT_FIFO_RST));
	ERROR_CHECK(result);

	result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
				       MPUREG_USER_CTRL, userCtrlReg);
	ERROR_CHECK(result);

	result = MLSLSerialRead(mlsl_handle, pdata->addr,
				MPUREG_USER_CTRL, 1, &userCtrlReg);
	ERROR_CHECK(result);

	if (pdata->dmp_enable)
		userCtrlReg |= BIT_DMP_EN;
	else
		userCtrlReg &= ~BIT_DMP_EN;

	if (pdata->fifo_enable)
		userCtrlReg |= BIT_FIFO_EN;
	else
		userCtrlReg &= ~BIT_FIFO_EN;

	userCtrlReg |= BIT_DMP_RST;

	result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
				       MPUREG_USER_CTRL, userCtrlReg);
	ERROR_CHECK(result);
	pdata->dmp_is_running = pdata->dmp_enable;

	return result;
}

static int MLDLSetI2CBypass(struct mldl_cfg *mldl_cfg,
			    void *mlsl_handle,
			    unsigned char enable)
{
	unsigned char b = 0;
	int result;

	if ((mldl_cfg->gyro_is_bypassed && enable) ||
	    (!mldl_cfg->gyro_is_bypassed && !enable))
		return ML_SUCCESS;

	
	result = MLSLSerialRead(mlsl_handle, mldl_cfg->addr,
				MPUREG_USER_CTRL, 1, &b);
	ERROR_CHECK(result);

	b &= ~BIT_AUX_IF_EN;

	if (!enable) {
		result = MLSLSerialWriteSingle(mlsl_handle, mldl_cfg->addr,
					       MPUREG_USER_CTRL,
					       (b | BIT_AUX_IF_EN));
		ERROR_CHECK(result);
	} else {
		result = MLSLSerialWriteSingle(mlsl_handle, mldl_cfg->addr,
					       MPUREG_AUX_SLV_ADDR, 0x7F);
		ERROR_CHECK(result);
		MLOSSleep(2);
		result = MLSLSerialWriteSingle(mlsl_handle, mldl_cfg->addr,
					       MPUREG_USER_CTRL, (b));
		ERROR_CHECK(result);
		MLOSSleep(SAMPLING_PERIOD_US(mldl_cfg) / 1000);
		result = MLSLSerialWriteSingle(mlsl_handle, mldl_cfg->addr,
					       MPUREG_AUX_SLV_ADDR,
					       mldl_cfg->pdata->
					       accel.address);
		ERROR_CHECK(result);

#ifdef M_HW
		result = MLSLSerialWriteSingle(mlsl_handle, mldl_cfg->addr,
					       MPUREG_USER_CTRL,
					       (b | BIT_I2C_MST_RST));

#else
		result = MLSLSerialWriteSingle(mlsl_handle, mldl_cfg->addr,
					       MPUREG_USER_CTRL,
					       (b | BIT_AUX_IF_RST));
#endif
		ERROR_CHECK(result);
		MLOSSleep(2);
	}
	mldl_cfg->gyro_is_bypassed = enable;

	return result;
}

struct tsProdRevMap {
	unsigned char siliconRev;
	unsigned short sensTrim;
};

#define NUM_OF_PROD_REVS (DIM(prodRevsMap))

#ifdef M_HW
#define OLDEST_PROD_REV_SUPPORTED 1
static struct tsProdRevMap prodRevsMap[] = {
	{0, 0},
	{MPU_SILICON_REV_A1, 131},	
	{MPU_SILICON_REV_A1, 131},	
	{MPU_SILICON_REV_A1, 131},	
	{MPU_SILICON_REV_A1, 131},	
	{MPU_SILICON_REV_A1, 131},	
	{MPU_SILICON_REV_A1, 131},	
	{MPU_SILICON_REV_A1, 131},	
	{MPU_SILICON_REV_A1, 131},	
};

#else				
#define OLDEST_PROD_REV_SUPPORTED 11

static struct tsProdRevMap prodRevsMap[] = {
	{0, 0},
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_A4, 131},	
	{MPU_SILICON_REV_B1, 131},	
	{MPU_SILICON_REV_B1, 131},	
	{MPU_SILICON_REV_B1, 131},	
	{MPU_SILICON_REV_B1, 131},	
	{MPU_SILICON_REV_B4, 131},	
	{MPU_SILICON_REV_B4, 131},	
	{MPU_SILICON_REV_B4, 131},	
	{MPU_SILICON_REV_B4, 131},	
	{MPU_SILICON_REV_B4, 115},	
	{MPU_SILICON_REV_B4, 115},	
	{MPU_SILICON_REV_B6, 131},	
	{MPU_SILICON_REV_B4, 115},	
	{MPU_SILICON_REV_B6, 0},	
	{MPU_SILICON_REV_B6, 0},	
	{MPU_SILICON_REV_B6, 0},	
	{MPU_SILICON_REV_B6, 131},	
};
#endif				

static int MLDLGetSiliconRev(struct mldl_cfg *pdata,
			     void *mlsl_handle)
{
	int result;
	unsigned char index = 0x00;
	unsigned char bank =
	    (BIT_PRFTCH_EN | BIT_CFG_USER_BANK | MPU_MEM_OTP_BANK_0);
	unsigned short memAddr = ((bank << 8) | 0x06);

	result = MLSLSerialReadMem(mlsl_handle, pdata->addr,
				   memAddr, 1, &index);
	ERROR_CHECK(result)
	if (result)
		return result;
	index >>= 2;

	
	result =
	    MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
				  MPUREG_BANK_SEL, 0);
	ERROR_CHECK(result)
	if (result)
		return result;

	if (index < OLDEST_PROD_REV_SUPPORTED || NUM_OF_PROD_REVS <= index) {
		pdata->silicon_revision = 0;
		pdata->trim = 0;
		MPL_LOGE("Unsupported Product Revision Detected : %d\n", index);
		return ML_ERROR_INVALID_MODULE;
	}

	pdata->silicon_revision = prodRevsMap[index].siliconRev;
	pdata->trim = prodRevsMap[index].sensTrim;

	if (pdata->trim == 0) {
		MPL_LOGE("sensitivity trim is 0"
			 " - unsupported non production part.\n");
		return ML_ERROR_INVALID_MODULE;
	}

	return result;
}

static int MLDLSetLevelShifterBit(struct mldl_cfg *pdata,
				  void *mlsl_handle,
				  unsigned char enable)
{
#ifndef M_HW
	int result;
	unsigned char reg;
	unsigned char mask;
	unsigned char regval = 0;

	if (0 == pdata->silicon_revision)
		return ML_ERROR_INVALID_PARAMETER;

	if ((pdata->silicon_revision & 0xf) < MPU_SILICON_REV_B6) {
		reg = MPUREG_ACCEL_BURST_ADDR;
		mask = 0x80;
	} else {
		reg = MPUREG_FIFO_EN2;
		mask = 0x04;
	}

	result = MLSLSerialRead(mlsl_handle, pdata->addr, reg, 1, &regval);
	if (result)
		return result;

	if (enable)
		regval |= mask;
	else
		regval &= ~mask;

	result =
	    MLSLSerialWriteSingle(mlsl_handle, pdata->addr, reg, regval);

	return result;
#else
	return ML_SUCCESS;
#endif
}


#ifdef M_HW
static tMLError mpu60xx_pwr_mgmt(struct mldl_cfg *pdata,
				 void *mlsl_handle,
				 unsigned char reset,
				 unsigned char powerselection)
{
	unsigned char b;
	tMLError result;

	if (powerselection < 0 || powerselection > 7)
		return ML_ERROR_INVALID_PARAMETER;

	result =
	    MLSLSerialRead(mlsl_handle, pdata->addr, MPUREG_PWR_MGMT_1, 1,
			   &b);
	ERROR_CHECK(result);

	b &= ~(BITS_PWRSEL);

	if (reset) {
		result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
					       MPUREG_PWR_MGM, b | BIT_H_RESET);
#define M_HW_RESET_ERRATTA
#ifndef M_HW_RESET_ERRATTA
		ERROR_CHECK(result);
#else
		MLOSSleep(50);
#endif
	}

	b |= (powerselection << 4);

	if (b & BITS_PWRSEL)
		pdata->gyro_is_suspended = FALSE;
	else
		pdata->gyro_is_suspended = TRUE;

	result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
				       MPUREG_PWR_MGM, b);
	ERROR_CHECK(result);

	return ML_SUCCESS;
}

static tMLError MLDLStandByGyros(struct mldl_cfg *pdata,
				 void *mlsl_handle,
				 unsigned char disable_gx,
				 unsigned char disable_gy,
				 unsigned char disable_gz)
{
	unsigned char b;
	tMLError result;

	result =
	    MLSLSerialRead(mlsl_handle, pdata->addr, MPUREG_PWR_MGMT_2, 1,
			   &b);
	ERROR_CHECK(result);

	b &= ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);
	b |= (disable_gx << 2 | disable_gy << 1 | disable_gz);

	result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
				       MPUREG_PWR_MGMT_2, b);
	ERROR_CHECK(result);

	return ML_SUCCESS;
}

static tMLError MLDLStandByAccels(struct mldl_cfg *pdata,
				  void *mlsl_handle,
				  unsigned char disable_ax,
				  unsigned char disable_ay,
				  unsigned char disable_az)
{
	unsigned char b;
	tMLError result;

	result =
	    MLSLSerialRead(mlsl_handle, pdata->addr, MPUREG_PWR_MGMT_2, 1,
			   &b);
	ERROR_CHECK(result);

	b &= ~(BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA);
	b |= (disable_ax << 2 | disable_ay << 1 | disable_az);

	result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
				       MPUREG_PWR_MGMT_2, b);
	ERROR_CHECK(result);

	return ML_SUCCESS;
}

#else				

static int MLDLPowerMgmtMPU(struct mldl_cfg *pdata,
			    void *mlsl_handle,
			    unsigned char reset,
			    unsigned char sleep,
			    unsigned char disable_gx,
			    unsigned char disable_gy,
			    unsigned char disable_gz)
{
	unsigned char b = 0;
	int result;

	result =
	    MLSLSerialRead(mlsl_handle, pdata->addr, MPUREG_PWR_MGM, 1,
			   &b);
	ERROR_CHECK(result);

	
	if ((!(b & BIT_SLEEP)) && reset)
		result = MLDLSetI2CBypass(pdata, mlsl_handle, 1);

	
	if ((!(b & BIT_SLEEP)) && sleep)
		dmp_stop(pdata, mlsl_handle);

	
	if (reset) {
		MPL_LOGV("Reset MPU3050\n");
		result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
					MPUREG_PWR_MGM, b | BIT_H_RESET);
		ERROR_CHECK(result);
		MLOSSleep(5);
		pdata->gyro_needs_reset = FALSE;
		result = MLSLSerialRead(mlsl_handle, pdata->addr,
					MPUREG_PWR_MGM, 1, &b);
		ERROR_CHECK(result);
	}

	
	if (b & BIT_SLEEP)
		pdata->gyro_is_suspended = TRUE;
	else
		pdata->gyro_is_suspended = FALSE;

	
	if ((b & (BIT_SLEEP | BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG)) ==
		(((sleep != 0) * BIT_SLEEP) |
		((disable_gx != 0) * BIT_STBY_XG) |
		((disable_gy != 0) * BIT_STBY_YG) |
		((disable_gz != 0) * BIT_STBY_ZG))) {
		return ML_SUCCESS;
	}

	if ((b & (BIT_SLEEP | BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG)) ==
		 (BIT_SLEEP | BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG)
		&& ((!sleep) && disable_gx && disable_gy && disable_gz)) {
		result = MLDLPowerMgmtMPU(pdata, mlsl_handle, 0, 1, 0, 0, 0);
		if (result)
			return result;
		b |= BIT_SLEEP;
		b &= ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);
	}

	if ((b & BIT_SLEEP) != ((sleep != 0) * BIT_SLEEP)) {
		if (sleep) {
			result = MLDLSetI2CBypass(pdata, mlsl_handle, 1);
			ERROR_CHECK(result);
			b |= BIT_SLEEP;
			result =
			    MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
						  MPUREG_PWR_MGM, b);
			ERROR_CHECK(result);
			pdata->gyro_is_suspended = TRUE;
		} else {
			b &= ~BIT_SLEEP;
			result =
			    MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
						  MPUREG_PWR_MGM, b);
			ERROR_CHECK(result);
			pdata->gyro_is_suspended = FALSE;
			MLOSSleep(5);
		}
	}
	if ((b & BIT_STBY_XG) != ((disable_gx != 0) * BIT_STBY_XG)) {
		b ^= BIT_STBY_XG;
		result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
						MPUREG_PWR_MGM, b);
		ERROR_CHECK(result);
	}
	if ((b & BIT_STBY_YG) != ((disable_gy != 0) * BIT_STBY_YG)) {
		b ^= BIT_STBY_YG;
		result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
					       MPUREG_PWR_MGM, b);
		ERROR_CHECK(result);
	}
	if ((b & BIT_STBY_ZG) != ((disable_gz != 0) * BIT_STBY_ZG)) {
		b ^= BIT_STBY_ZG;
		result = MLSLSerialWriteSingle(mlsl_handle, pdata->addr,
					       MPUREG_PWR_MGM, b);
		ERROR_CHECK(result);
	}

	return ML_SUCCESS;
}
#endif				


void mpu_print_cfg(struct mldl_cfg *mldl_cfg)
{
	struct mpu3050_platform_data *pdata = mldl_cfg->pdata;
	struct ext_slave_platform_data *accel = &mldl_cfg->pdata->accel;
	struct ext_slave_platform_data *compass =
	    &mldl_cfg->pdata->compass;
	struct ext_slave_platform_data *pressure =
	    &mldl_cfg->pdata->pressure;

	MPL_LOGD("mldl_cfg.addr             = %02x\n", mldl_cfg->addr);
	MPL_LOGD("mldl_cfg.int_config       = %02x\n",
		 mldl_cfg->int_config);
	MPL_LOGD("mldl_cfg.ext_sync         = %02x\n", mldl_cfg->ext_sync);
	MPL_LOGD("mldl_cfg.full_scale       = %02x\n",
		 mldl_cfg->full_scale);
	MPL_LOGD("mldl_cfg.lpf              = %02x\n", mldl_cfg->lpf);
	MPL_LOGD("mldl_cfg.clk_src          = %02x\n", mldl_cfg->clk_src);
	MPL_LOGD("mldl_cfg.divider          = %02x\n", mldl_cfg->divider);
	MPL_LOGD("mldl_cfg.dmp_enable       = %02x\n",
		 mldl_cfg->dmp_enable);
	MPL_LOGD("mldl_cfg.fifo_enable      = %02x\n",
		 mldl_cfg->fifo_enable);
	MPL_LOGD("mldl_cfg.dmp_cfg1         = %02x\n", mldl_cfg->dmp_cfg1);
	MPL_LOGD("mldl_cfg.dmp_cfg2         = %02x\n", mldl_cfg->dmp_cfg2);
	MPL_LOGD("mldl_cfg.offset_tc[0]     = %02x\n",
		 mldl_cfg->offset_tc[0]);
	MPL_LOGD("mldl_cfg.offset_tc[1]     = %02x\n",
		 mldl_cfg->offset_tc[1]);
	MPL_LOGD("mldl_cfg.offset_tc[2]     = %02x\n",
		 mldl_cfg->offset_tc[2]);
	MPL_LOGD("mldl_cfg.silicon_revision = %02x\n",
		 mldl_cfg->silicon_revision);
	MPL_LOGD("mldl_cfg.product_id       = %02x\n",
		 mldl_cfg->product_id);
	MPL_LOGD("mldl_cfg.trim             = %02x\n", mldl_cfg->trim);
	MPL_LOGD("mldl_cfg.requested_sensors= %04lx\n",
		 mldl_cfg->requested_sensors);

	if (mldl_cfg->accel) {
		MPL_LOGD("slave_accel->suspend      = %02x\n",
			 (int) mldl_cfg->accel->suspend);
		MPL_LOGD("slave_accel->resume       = %02x\n",
			 (int) mldl_cfg->accel->resume);
		MPL_LOGD("slave_accel->read         = %02x\n",
			 (int) mldl_cfg->accel->read);
		MPL_LOGD("slave_accel->type         = %02x\n",
			 mldl_cfg->accel->type);
		MPL_LOGD("slave_accel->reg          = %02x\n",
			 mldl_cfg->accel->reg);
		MPL_LOGD("slave_accel->len          = %02x\n",
			 mldl_cfg->accel->len);
		MPL_LOGD("slave_accel->endian       = %02x\n",
			 mldl_cfg->accel->endian);
		MPL_LOGD("slave_accel->range.mantissa= %02lx\n",
			 mldl_cfg->accel->range.mantissa);
		MPL_LOGD("slave_accel->range.fraction= %02lx\n",
			 mldl_cfg->accel->range.fraction);
	} else {
		MPL_LOGD("slave_accel               = NULL\n");
	}

	if (mldl_cfg->compass) {
		MPL_LOGD("slave_compass->suspend    = %02x\n",
			 (int) mldl_cfg->compass->suspend);
		MPL_LOGD("slave_compass->resume     = %02x\n",
			 (int) mldl_cfg->compass->resume);
		MPL_LOGD("slave_compass->read       = %02x\n",
			 (int) mldl_cfg->compass->read);
		MPL_LOGD("slave_compass->type       = %02x\n",
			 mldl_cfg->compass->type);
		MPL_LOGD("slave_compass->reg        = %02x\n",
			 mldl_cfg->compass->reg);
		MPL_LOGD("slave_compass->len        = %02x\n",
			 mldl_cfg->compass->len);
		MPL_LOGD("slave_compass->endian     = %02x\n",
			 mldl_cfg->compass->endian);
		MPL_LOGD("slave_compass->range.mantissa= %02lx\n",
			 mldl_cfg->compass->range.mantissa);
		MPL_LOGD("slave_compass->range.fraction= %02lx\n",
			 mldl_cfg->compass->range.fraction);

	} else {
		MPL_LOGD("slave_compass             = NULL\n");
	}

	if (mldl_cfg->pressure) {
		MPL_LOGD("slave_pressure->suspend    = %02x\n",
			 (int) mldl_cfg->pressure->suspend);
		MPL_LOGD("slave_pressure->resume     = %02x\n",
			 (int) mldl_cfg->pressure->resume);
		MPL_LOGD("slave_pressure->read       = %02x\n",
			 (int) mldl_cfg->pressure->read);
		MPL_LOGD("slave_pressure->type       = %02x\n",
			 mldl_cfg->pressure->type);
		MPL_LOGD("slave_pressure->reg        = %02x\n",
			 mldl_cfg->pressure->reg);
		MPL_LOGD("slave_pressure->len        = %02x\n",
			 mldl_cfg->pressure->len);
		MPL_LOGD("slave_pressure->endian     = %02x\n",
			 mldl_cfg->pressure->endian);
		MPL_LOGD("slave_pressure->range.mantissa= %02lx\n",
			 mldl_cfg->pressure->range.mantissa);
		MPL_LOGD("slave_pressure->range.fraction= %02lx\n",
			 mldl_cfg->pressure->range.fraction);

	} else {
		MPL_LOGD("slave_pressure             = NULL\n");
	}
	MPL_LOGD("accel->get_slave_descr    = %x\n",
		 (unsigned int) accel->get_slave_descr);
	MPL_LOGD("accel->irq                = %02x\n", accel->irq);
	MPL_LOGD("accel->adapt_num          = %02x\n", accel->adapt_num);
	MPL_LOGD("accel->bus                = %02x\n", accel->bus);
	MPL_LOGD("accel->address            = %02x\n", accel->address);
	MPL_LOGD("accel->orientation        =\n"
		 "                            %2d %2d %2d\n"
		 "                            %2d %2d %2d\n"
		 "                            %2d %2d %2d\n",
		 accel->orientation[0], accel->orientation[1],
		 accel->orientation[2], accel->orientation[3],
		 accel->orientation[4], accel->orientation[5],
		 accel->orientation[6], accel->orientation[7],
		 accel->orientation[8]);
	MPL_LOGD("compass->get_slave_descr  = %x\n",
		 (unsigned int) compass->get_slave_descr);
	MPL_LOGD("compass->irq              = %02x\n", compass->irq);
	MPL_LOGD("compass->adapt_num        = %02x\n", compass->adapt_num);
	MPL_LOGD("compass->bus              = %02x\n", compass->bus);
	MPL_LOGD("compass->address          = %02x\n", compass->address);
	MPL_LOGD("compass->orientation      =\n"
		 "                            %2d %2d %2d\n"
		 "                            %2d %2d %2d\n"
		 "                            %2d %2d %2d\n",
		 compass->orientation[0], compass->orientation[1],
		 compass->orientation[2], compass->orientation[3],
		 compass->orientation[4], compass->orientation[5],
		 compass->orientation[6], compass->orientation[7],
		 compass->orientation[8]);
	MPL_LOGD("pressure->get_slave_descr  = %x\n",
		 (unsigned int) pressure->get_slave_descr);
	MPL_LOGD("pressure->irq             = %02x\n", pressure->irq);
	MPL_LOGD("pressure->adapt_num       = %02x\n", pressure->adapt_num);
	MPL_LOGD("pressure->bus             = %02x\n", pressure->bus);
	MPL_LOGD("pressure->address         = %02x\n", pressure->address);
	MPL_LOGD("pressure->orientation     =\n"
		 "                            %2d %2d %2d\n"
		 "                            %2d %2d %2d\n"
		 "                            %2d %2d %2d\n",
		 pressure->orientation[0], pressure->orientation[1],
		 pressure->orientation[2], pressure->orientation[3],
		 pressure->orientation[4], pressure->orientation[5],
		 pressure->orientation[6], pressure->orientation[7],
		 pressure->orientation[8]);

	MPL_LOGD("pdata->int_config         = %02x\n", pdata->int_config);
	MPL_LOGD("pdata->level_shifter      = %02x\n",
		 pdata->level_shifter);
	MPL_LOGD("pdata->orientation        =\n"
		 "                            %2d %2d %2d\n"
		 "                            %2d %2d %2d\n"
		 "                            %2d %2d %2d\n",
		 pdata->orientation[0], pdata->orientation[1],
		 pdata->orientation[2], pdata->orientation[3],
		 pdata->orientation[4], pdata->orientation[5],
		 pdata->orientation[6], pdata->orientation[7],
		 pdata->orientation[8]);

	MPL_LOGD("Struct sizes: mldl_cfg: %d, "
		 "ext_slave_descr:%d, "
		 "mpu3050_platform_data:%d: RamOffset: %d\n",
		 sizeof(struct mldl_cfg), sizeof(struct ext_slave_descr),
		 sizeof(struct mpu3050_platform_data),
		 offsetof(struct mldl_cfg, ram));
}

int mpu_set_slave(struct mldl_cfg *mldl_cfg,
		void *gyro_handle,
		struct ext_slave_descr *slave,
		struct ext_slave_platform_data *slave_pdata)
{
	int result;
	unsigned char reg = 0;
	unsigned char slave_reg;
	unsigned char slave_len;
	unsigned char slave_endian;
	unsigned char slave_address;

	result = MLDLSetI2CBypass(mldl_cfg, gyro_handle, TRUE);

	if (NULL == slave || NULL == slave_pdata) {
		slave_reg = 0;
		slave_len = 0;
		slave_endian = 0;
		slave_address = 0;
	} else {
		slave_reg = slave->reg;
		slave_len = slave->len;
		slave_endian = slave->endian;
		slave_address = slave_pdata->address;
	}

	
	result = MLSLSerialWriteSingle(gyro_handle,
				mldl_cfg->addr,
				MPUREG_AUX_SLV_ADDR,
				slave_address);
	ERROR_CHECK(result);
	
	result = MLSLSerialRead(gyro_handle, mldl_cfg->addr,
				MPUREG_ACCEL_BURST_ADDR, 1,
				&reg);
	ERROR_CHECK(result);
	reg = ((reg & 0x80) | slave_reg);
	result = MLSLSerialWriteSingle(gyro_handle,
				mldl_cfg->addr,
				MPUREG_ACCEL_BURST_ADDR,
				reg);
	ERROR_CHECK(result);

#ifdef M_HW
	
	if (slave_len > BITS_SLV_LENG) {
		MPL_LOGW("Limiting slave burst read length to "
			"the allowed maximum (15B, req. %d)\n",
			slave_len);
		slave_len = BITS_SLV_LENG;
	}
	reg = slave_len;
	if (slave_endian == EXT_SLAVE_LITTLE_ENDIAN)
		reg |= BIT_SLV_BYTE_SW;
	reg |= BIT_SLV_GRP;
	reg |= BIT_SLV_ENABLE;

	result = MLSLSerialWriteSingle(gyro_handle,
				mldl_cfg->addr,
				MPUREG_I2C_SLV0_CTRL,
				reg);
#else
	
	result = MLSLSerialRead(gyro_handle, mldl_cfg->addr,
				MPUREG_USER_CTRL, 1, &reg);
	ERROR_CHECK(result);
	reg = (reg & ~BIT_AUX_RD_LENG);
	result = MLSLSerialWriteSingle(gyro_handle,
				mldl_cfg->addr,
				MPUREG_USER_CTRL, reg);
	ERROR_CHECK(result);
#endif

	if (slave_address) {
		result = MLDLSetI2CBypass(mldl_cfg, gyro_handle, FALSE);
		ERROR_CHECK(result);
	}
	return result;
}

static int mpu_was_reset(struct mldl_cfg *mldl_cfg, void *gyro_handle)
{
	int result = ML_SUCCESS;
	unsigned char reg = 0;

	result = MLSLSerialRead(gyro_handle, mldl_cfg->addr,
				MPUREG_DMP_CFG_2, 1, &reg);
	ERROR_CHECK(result);

	if (mldl_cfg->dmp_cfg2 != reg)
		return TRUE;

	if (0 != mldl_cfg->dmp_cfg1)
		return FALSE;

	result = MLSLSerialRead(gyro_handle, mldl_cfg->addr,
				MPUREG_SMPLRT_DIV, 1, &reg);
	ERROR_CHECK(result);
	if (reg != mldl_cfg->divider)
		return TRUE;

	if (0 != mldl_cfg->divider)
		return FALSE;

	
	return TRUE;
}

static int gyro_resume(struct mldl_cfg *mldl_cfg, void *gyro_handle)
{
	int result;
	int ii;
	int jj;
	unsigned char reg = 0;
	unsigned char regs[7];

	
#ifdef M_HW
	result = mpu60xx_pwr_mgmt(mldl_cfg, gyro_handle, RESET,
				WAKE_UP);
	ERROR_CHECK(result);

	
	result = MLDLSetI2CBypass(mldl_cfg, gyro_handle, 1);
	ERROR_CHECK(result);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_INT_PIN_CFG,
				(mldl_cfg->pdata->int_config |
					BIT_BYPASS_EN));
	ERROR_CHECK(result);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_INT_ENABLE,
				(mldl_cfg->int_config));
	ERROR_CHECK(result);
#else
	result = MLDLPowerMgmtMPU(mldl_cfg, gyro_handle, 0, 0,
				mldl_cfg->gyro_power & BIT_STBY_XG,
				mldl_cfg->gyro_power & BIT_STBY_YG,
				mldl_cfg->gyro_power & BIT_STBY_ZG);

	if (!mldl_cfg->gyro_needs_reset &&
	    !mpu_was_reset(mldl_cfg, gyro_handle)) {
		return ML_SUCCESS;
	}

	result = MLDLPowerMgmtMPU(mldl_cfg, gyro_handle, 1, 0,
				mldl_cfg->gyro_power & BIT_STBY_XG,
				mldl_cfg->gyro_power & BIT_STBY_YG,
				mldl_cfg->gyro_power & BIT_STBY_ZG);
	ERROR_CHECK(result);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_INT_CFG,
				(mldl_cfg->int_config |
					mldl_cfg->pdata->int_config));
	ERROR_CHECK(result);
#endif

	result = MLSLSerialRead(gyro_handle, mldl_cfg->addr,
				MPUREG_PWR_MGM, 1, &reg);
	ERROR_CHECK(result);
	reg &= ~BITS_CLKSEL;
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_PWR_MGM,
				mldl_cfg->clk_src | reg);
	ERROR_CHECK(result);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_SMPLRT_DIV,
				mldl_cfg->divider);
	ERROR_CHECK(result);

#ifdef M_HW
	reg = DLPF_FS_SYNC_VALUE(0, mldl_cfg->full_scale, 0);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_GYRO_CONFIG, reg);
	reg = DLPF_FS_SYNC_VALUE(mldl_cfg->ext_sync, 0, mldl_cfg->lpf);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_CONFIG, reg);
#else
	reg = DLPF_FS_SYNC_VALUE(mldl_cfg->ext_sync,
				mldl_cfg->full_scale, mldl_cfg->lpf);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_DLPF_FS_SYNC, reg);
#endif
	ERROR_CHECK(result);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_DMP_CFG_1,
				mldl_cfg->dmp_cfg1);
	ERROR_CHECK(result);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_DMP_CFG_2,
				mldl_cfg->dmp_cfg2);
	ERROR_CHECK(result);

	
	for (ii = 0; ii < MPU_MEM_NUM_RAM_BANKS; ii++) {
		unsigned char read[MPU_MEM_BANK_SIZE] = {0};

		result = MLSLSerialWriteMem(gyro_handle,
					mldl_cfg->addr,
					((ii << 8) | 0x00),
					MPU_MEM_BANK_SIZE,
					mldl_cfg->ram[ii]);
		ERROR_CHECK(result);
		result = MLSLSerialReadMem(gyro_handle, mldl_cfg->addr,
					((ii << 8) | 0x00),
					MPU_MEM_BANK_SIZE, read);
		ERROR_CHECK(result);

#ifdef M_HW
#define ML_SKIP_CHECK 38
#else
#define ML_SKIP_CHECK 20
#endif
		for (jj = 0; jj < MPU_MEM_BANK_SIZE; jj++) {
			
			if (ii == 0 && jj < ML_SKIP_CHECK)
				continue;
			if (mldl_cfg->ram[ii][jj] != read[jj]) {
				result = ML_ERROR_SERIAL_WRITE;
				break;
			}
		}
		ERROR_CHECK(result);
	}

	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_XG_OFFS_TC,
				mldl_cfg->offset_tc[0]);
	ERROR_CHECK(result);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_YG_OFFS_TC,
				mldl_cfg->offset_tc[1]);
	ERROR_CHECK(result);
	result = MLSLSerialWriteSingle(gyro_handle, mldl_cfg->addr,
				MPUREG_ZG_OFFS_TC,
				mldl_cfg->offset_tc[2]);
	ERROR_CHECK(result);

	regs[0] = MPUREG_X_OFFS_USRH;
	for (ii = 0; ii < DIM(mldl_cfg->offset); ii++) {
		regs[1 + ii * 2] =
			(unsigned char)(mldl_cfg->offset[ii] >> 8)
			& 0xff;
		regs[1 + ii * 2 + 1] =
			(unsigned char)(mldl_cfg->offset[ii] & 0xff);
	}
	result = MLSLSerialWrite(gyro_handle, mldl_cfg->addr, 7, regs);
	ERROR_CHECK(result);

	
	result = MLDLSetLevelShifterBit(mldl_cfg, gyro_handle,
					mldl_cfg->pdata->level_shifter);
	ERROR_CHECK(result);
	return result;
}

int mpu3050_open(struct mldl_cfg *mldl_cfg,
		 void *mlsl_handle,
		 void *accel_handle,
		 void *compass_handle,
		 void *pressure_handle
		 )
{
	int result;
	
	mldl_cfg->int_config = BIT_INT_ANYRD_2CLEAR | BIT_DMP_INT_EN;
	mldl_cfg->clk_src = MPU_CLK_SEL_PLLGYROZ;
	mldl_cfg->lpf = MPU_FILTER_42HZ;
	mldl_cfg->full_scale = MPU_FS_2000DPS;
	mldl_cfg->divider = 4;
	mldl_cfg->dmp_enable = 1;
	mldl_cfg->fifo_enable = 1;
	mldl_cfg->ext_sync = 0;
	mldl_cfg->dmp_cfg1 = 0;
	mldl_cfg->dmp_cfg2 = 0;
	mldl_cfg->gyro_power = 0;
	mldl_cfg->gyro_is_bypassed = TRUE;
	mldl_cfg->dmp_is_running = FALSE;
	mldl_cfg->gyro_is_suspended = TRUE;
	mldl_cfg->accel_is_suspended = TRUE;
	mldl_cfg->compass_is_suspended = TRUE;
	mldl_cfg->pressure_is_suspended = TRUE;
	mldl_cfg->gyro_needs_reset = FALSE;
	if (mldl_cfg->addr == 0) {
#ifdef __KERNEL__
		return ML_ERROR_INVALID_PARAMETER;
#else
		mldl_cfg->addr = 0x68;
#endif
	}

#ifdef M_HW
	result = mpu60xx_pwr_mgmt(mldl_cfg, mlsl_handle,
				  RESET, WAKE_UP);
#else
	result = MLDLPowerMgmtMPU(mldl_cfg, mlsl_handle, RESET, 0, 0, 0, 0);
#endif
	ERROR_CHECK(result);

	result = MLDLGetSiliconRev(mldl_cfg, mlsl_handle);
	ERROR_CHECK(result);
#ifndef M_HW
	result = MLSLSerialRead(mlsl_handle, mldl_cfg->addr,
				MPUREG_PRODUCT_ID, 1,
				&mldl_cfg->product_id);
	ERROR_CHECK(result);
#endif

	
	result = MLSLSerialRead(mlsl_handle, mldl_cfg->addr,
				MPUREG_XG_OFFS_TC, 1,
				&mldl_cfg->offset_tc[0]);
	ERROR_CHECK(result);
	result = MLSLSerialRead(mlsl_handle, mldl_cfg->addr,
				MPUREG_YG_OFFS_TC, 1,
				&mldl_cfg->offset_tc[1]);
	ERROR_CHECK(result);
	result = MLSLSerialRead(mlsl_handle, mldl_cfg->addr,
				MPUREG_ZG_OFFS_TC, 1,
				&mldl_cfg->offset_tc[2]);
	ERROR_CHECK(result);

	
#ifdef M_HW
	result = mpu60xx_pwr_mgmt(mldl_cfg, mlsl_handle,
				  FALSE, SLEEP);
#else
	result =
	    MLDLPowerMgmtMPU(mldl_cfg, mlsl_handle, 0, SLEEP, 0, 0, 0);
#endif
	ERROR_CHECK(result);

	if (mldl_cfg->accel && mldl_cfg->accel->init) {
		result = mldl_cfg->accel->init(accel_handle,
					       mldl_cfg->accel,
#ifdef CONFIG_CIR_ALWAYS_READY
					       &mldl_cfg->pdata->accel,
					       mldl_cfg->pdata->power_LPM);
#else
					       &mldl_cfg->pdata->accel);
#endif

		ERROR_CHECK(result);
	}

	if (mldl_cfg->compass && mldl_cfg->compass->init) {
		result = mldl_cfg->compass->init(compass_handle,
						 mldl_cfg->compass,
#ifdef CONFIG_CIR_ALWAYS_READY
						 &mldl_cfg->pdata->compass,
						 NULL);
#else
						 &mldl_cfg->pdata->compass);
#endif
		if (ML_SUCCESS != result) {
			MPL_LOGE("mldl_cfg->compass->init returned %d\n",
				result);
			goto out_accel;
		}
	}
	if (mldl_cfg->pressure && mldl_cfg->pressure->init) {
		result = mldl_cfg->pressure->init(pressure_handle,
						  mldl_cfg->pressure,
#ifdef CONFIG_CIR_ALWAYS_READY
						  &mldl_cfg->pdata->pressure,
						  NULL);
#else
						  &mldl_cfg->pdata->pressure);
#endif
		if (ML_SUCCESS != result) {
			MPL_LOGE("mldl_cfg->pressure->init returned %d\n",
				result);
			goto out_compass;
		}
	}

	mldl_cfg->requested_sensors = ML_THREE_AXIS_GYRO;
	if (mldl_cfg->accel && mldl_cfg->accel->resume)
		mldl_cfg->requested_sensors |= ML_THREE_AXIS_ACCEL;

	if (mldl_cfg->compass && mldl_cfg->compass->resume)
		mldl_cfg->requested_sensors |= ML_THREE_AXIS_COMPASS;

	if (mldl_cfg->pressure && mldl_cfg->pressure->resume)
		mldl_cfg->requested_sensors |= ML_THREE_AXIS_PRESSURE;

	return result;

out_compass:
	if (mldl_cfg->compass->init)
		mldl_cfg->compass->exit(compass_handle,
				mldl_cfg->compass,
				&mldl_cfg->pdata->compass);
out_accel:
	if (mldl_cfg->accel->init)
		mldl_cfg->accel->exit(accel_handle,
				mldl_cfg->accel,
				&mldl_cfg->pdata->accel);
	return result;

}

int mpu3050_close(struct mldl_cfg *mldl_cfg,
		  void *mlsl_handle,
		  void *accel_handle,
		  void *compass_handle,
		  void *pressure_handle)
{
	int result = ML_SUCCESS;
	int ret_result = ML_SUCCESS;

	if (mldl_cfg->accel && mldl_cfg->accel->exit) {
		result = mldl_cfg->accel->exit(accel_handle,
					mldl_cfg->accel,
					&mldl_cfg->pdata->accel);
		if (ML_SUCCESS != result)
			MPL_LOGE("Accel exit failed %d\n", result);
		ret_result = result;
	}
	if (ML_SUCCESS == ret_result)
		ret_result = result;

	if (mldl_cfg->compass && mldl_cfg->compass->exit) {
		result = mldl_cfg->compass->exit(compass_handle,
						mldl_cfg->compass,
						&mldl_cfg->pdata->compass);
		if (ML_SUCCESS != result)
			MPL_LOGE("Compass exit failed %d\n", result);
	}
	if (ML_SUCCESS == ret_result)
		ret_result = result;

	if (mldl_cfg->pressure && mldl_cfg->pressure->exit) {
		result = mldl_cfg->pressure->exit(pressure_handle,
						mldl_cfg->pressure,
						&mldl_cfg->pdata->pressure);
		if (ML_SUCCESS != result)
			MPL_LOGE("Pressure exit failed %d\n", result);
	}
	if (ML_SUCCESS == ret_result)
		ret_result = result;

	return ret_result;
}

int mpu3050_resume(struct mldl_cfg *mldl_cfg,
		   void *gyro_handle,
		   void *accel_handle,
		   void *compass_handle,
		   void *pressure_handle,
		   bool resume_gyro,
		   bool resume_accel,
		   bool resume_compass,
		   bool resume_pressure)
{
	int result = ML_SUCCESS;

#ifdef CONFIG_MPU_SENSORS_DEBUG
	mpu_print_cfg(mldl_cfg);
#endif

	if (resume_accel &&
	    ((!mldl_cfg->accel) || (!mldl_cfg->accel->resume)))
		return ML_ERROR_INVALID_PARAMETER;
	if (resume_compass &&
	    ((!mldl_cfg->compass) || (!mldl_cfg->compass->resume)))
		return ML_ERROR_INVALID_PARAMETER;
	if (resume_pressure &&
	    ((!mldl_cfg->pressure) || (!mldl_cfg->pressure->resume)))
		return ML_ERROR_INVALID_PARAMETER;

	if (resume_gyro && mldl_cfg->gyro_is_suspended) {
		result = gyro_resume(mldl_cfg, gyro_handle);
		ERROR_CHECK(result);
	}

	if (resume_accel && mldl_cfg->accel_is_suspended) {
		if (!mldl_cfg->gyro_is_suspended &&
		    EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->accel.bus) {
			result = MLDLSetI2CBypass(mldl_cfg, gyro_handle, TRUE);
			ERROR_CHECK(result);
		}
		result = mldl_cfg->accel->resume(accel_handle,
						 mldl_cfg->accel,
						 &mldl_cfg->pdata->accel);
		ERROR_CHECK(result);
		mldl_cfg->accel_is_suspended = FALSE;
	}

	if (!mldl_cfg->gyro_is_suspended && !mldl_cfg->accel_is_suspended &&
		EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->accel.bus) {
		result = mpu_set_slave(mldl_cfg,
				gyro_handle,
				mldl_cfg->accel,
				&mldl_cfg->pdata->accel);
		ERROR_CHECK(result);
	}

	if (resume_compass && mldl_cfg->compass_is_suspended) {
		if (!mldl_cfg->gyro_is_suspended &&
		    EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->compass.bus) {
			result = MLDLSetI2CBypass(mldl_cfg, gyro_handle, TRUE);
			ERROR_CHECK(result);
		}
		result = mldl_cfg->compass->resume(compass_handle,
						   mldl_cfg->compass,
						   &mldl_cfg->pdata->
						   compass);
		ERROR_CHECK(result);
		mldl_cfg->compass_is_suspended = FALSE;
	}

	if (!mldl_cfg->gyro_is_suspended && !mldl_cfg->compass_is_suspended &&
		EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->compass.bus) {
		result = mpu_set_slave(mldl_cfg,
				gyro_handle,
				mldl_cfg->compass,
				&mldl_cfg->pdata->compass);
		ERROR_CHECK(result);
	}

	if (resume_pressure && mldl_cfg->pressure_is_suspended) {
		if (!mldl_cfg->gyro_is_suspended &&
		    EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->pressure.bus) {
			result = MLDLSetI2CBypass(mldl_cfg, gyro_handle, TRUE);
			ERROR_CHECK(result);
		}
		result = mldl_cfg->pressure->resume(pressure_handle,
						    mldl_cfg->pressure,
						    &mldl_cfg->pdata->
						    pressure);
		ERROR_CHECK(result);
		mldl_cfg->pressure_is_suspended = FALSE;
	}

	if (!mldl_cfg->gyro_is_suspended && !mldl_cfg->pressure_is_suspended &&
		EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->pressure.bus) {
		result = mpu_set_slave(mldl_cfg,
				gyro_handle,
				mldl_cfg->pressure,
				&mldl_cfg->pdata->pressure);
		ERROR_CHECK(result);
	}

	
	if (resume_gyro) {
		result = dmp_start(mldl_cfg, gyro_handle);
		ERROR_CHECK(result);
	}

	return result;
}

int mpu3050_suspend(struct mldl_cfg *mldl_cfg,
		    void *gyro_handle,
		    void *accel_handle,
		    void *compass_handle,
		    void *pressure_handle,
		    bool suspend_gyro,
		    bool suspend_accel,
		    bool suspend_compass,
		    bool suspend_pressure)
{
	int result = ML_SUCCESS;

	if (suspend_gyro && !mldl_cfg->gyro_is_suspended) {
#ifdef M_HW
		return ML_SUCCESS;
		
		result = MLDLSetI2CBypass(mldl_cfg, gyro_handle, 1);
		ERROR_CHECK(result);
		result = mpu60xx_pwr_mgmt(mldl_cfg, gyro_handle, 0, SLEEP);
#else
		result = MLDLPowerMgmtMPU(mldl_cfg, gyro_handle,
					0, SLEEP, 0, 0, 0);
#endif
		ERROR_CHECK(result);
	}

	if (!mldl_cfg->accel_is_suspended && suspend_accel &&
	    mldl_cfg->accel && mldl_cfg->accel->suspend) {
		if (!mldl_cfg->gyro_is_suspended &&
		    EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->accel.bus) {
			result = mpu_set_slave(mldl_cfg, gyro_handle,
					       NULL, NULL);
			ERROR_CHECK(result);
		}
		result = mldl_cfg->accel->suspend(accel_handle,
						  mldl_cfg->accel,
						  &mldl_cfg->pdata->accel);
		ERROR_CHECK(result);
		mldl_cfg->accel_is_suspended = TRUE;
	}

	if (!mldl_cfg->compass_is_suspended && suspend_compass &&
	    mldl_cfg->compass && mldl_cfg->compass->suspend) {
		if (!mldl_cfg->gyro_is_suspended &&
		    EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->compass.bus) {
			result = mpu_set_slave(mldl_cfg, gyro_handle,
					       NULL, NULL);
			ERROR_CHECK(result);
		}
		result = mldl_cfg->compass->suspend(compass_handle,
						    mldl_cfg->compass,
						    &mldl_cfg->
						    pdata->compass);
		ERROR_CHECK(result);
		mldl_cfg->compass_is_suspended = TRUE;
	}

	if (!mldl_cfg->pressure_is_suspended && suspend_pressure &&
	    mldl_cfg->pressure && mldl_cfg->pressure->suspend) {
		if (!mldl_cfg->gyro_is_suspended &&
		    EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->pressure.bus) {
			result = mpu_set_slave(mldl_cfg, gyro_handle,
					       NULL, NULL);
			ERROR_CHECK(result);
		}
		result = mldl_cfg->pressure->suspend(pressure_handle,
						    mldl_cfg->pressure,
						    &mldl_cfg->
						    pdata->pressure);
		ERROR_CHECK(result);
		mldl_cfg->pressure_is_suspended = TRUE;
	}
	return result;
}


int mpu3050_read_accel(struct mldl_cfg *mldl_cfg,
		       void *accel_handle, unsigned char *data)
{
	if (NULL != mldl_cfg->accel && NULL != mldl_cfg->accel->read)
		if ((EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->accel.bus)
			&& (!mldl_cfg->gyro_is_bypassed))
			return ML_ERROR_FEATURE_NOT_ENABLED;
		else
			return mldl_cfg->accel->read(accel_handle,
						     mldl_cfg->accel,
						     &mldl_cfg->pdata->accel,
						     data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;
}

int mpu3050_read_compass(struct mldl_cfg *mldl_cfg,
			 void *compass_handle, unsigned char *data)
{
	if (NULL != mldl_cfg->compass && NULL != mldl_cfg->compass->read)
		if ((EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->compass.bus)
			&& (!mldl_cfg->gyro_is_bypassed))
			return ML_ERROR_FEATURE_NOT_ENABLED;
		else
			return mldl_cfg->compass->read(compass_handle,
						mldl_cfg->compass,
						&mldl_cfg->pdata->compass,
						data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;
}

int mpu3050_read_pressure(struct mldl_cfg *mldl_cfg,
			 void *pressure_handle, unsigned char *data)
{
	if (NULL != mldl_cfg->pressure && NULL != mldl_cfg->pressure->read)
		if ((EXT_SLAVE_BUS_SECONDARY == mldl_cfg->pdata->pressure.bus)
			&& (!mldl_cfg->gyro_is_bypassed))
			return ML_ERROR_FEATURE_NOT_ENABLED;
		else
			return mldl_cfg->pressure->read(
				pressure_handle,
				mldl_cfg->pressure,
				&mldl_cfg->pdata->pressure,
				data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;
}

int mpu3050_config_accel(struct mldl_cfg *mldl_cfg,
			void *accel_handle,
			struct ext_slave_config *data)
{
	if (NULL != mldl_cfg->accel && NULL != mldl_cfg->accel->config)
		return mldl_cfg->accel->config(accel_handle,
					       mldl_cfg->accel,
					       &mldl_cfg->pdata->accel,
					       data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;

}

int mpu3050_config_compass(struct mldl_cfg *mldl_cfg,
			void *compass_handle,
			struct ext_slave_config *data)
{
	if (NULL != mldl_cfg->compass && NULL != mldl_cfg->compass->config)
		return mldl_cfg->compass->config(compass_handle,
						 mldl_cfg->compass,
						 &mldl_cfg->pdata->compass,
						 data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;

}

int mpu3050_config_pressure(struct mldl_cfg *mldl_cfg,
			void *pressure_handle,
			struct ext_slave_config *data)
{
	if (NULL != mldl_cfg->pressure && NULL != mldl_cfg->pressure->config)
		return mldl_cfg->pressure->config(pressure_handle,
						  mldl_cfg->pressure,
						  &mldl_cfg->pdata->pressure,
						  data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;
}

int mpu3050_get_config_accel(struct mldl_cfg *mldl_cfg,
			void *accel_handle,
			struct ext_slave_config *data)
{
	if (NULL != mldl_cfg->accel && NULL != mldl_cfg->accel->get_config)
		return mldl_cfg->accel->get_config(accel_handle,
						mldl_cfg->accel,
						&mldl_cfg->pdata->accel,
						data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;

}

int mpu3050_get_config_compass(struct mldl_cfg *mldl_cfg,
			void *compass_handle,
			struct ext_slave_config *data)
{
	if (NULL != mldl_cfg->compass && NULL != mldl_cfg->compass->get_config)
		return mldl_cfg->compass->get_config(compass_handle,
						mldl_cfg->compass,
						&mldl_cfg->pdata->compass,
						data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;

}

int mpu3050_get_config_pressure(struct mldl_cfg *mldl_cfg,
				void *pressure_handle,
				struct ext_slave_config *data)
{
	if (NULL != mldl_cfg->pressure &&
	    NULL != mldl_cfg->pressure->get_config)
		return mldl_cfg->pressure->get_config(pressure_handle,
						mldl_cfg->pressure,
						&mldl_cfg->pdata->pressure,
						data);
	else
		return ML_ERROR_FEATURE_NOT_IMPLEMENTED;
}


