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

#include "mlsl.h"
#include "mpu-i2c.h"




tMLError MLSLSerialOpen(char const *port, void **sl_handle)
{
	return ML_SUCCESS;
}

tMLError MLSLSerialReset(void *sl_handle)
{
	return ML_SUCCESS;
}

tMLError MLSLSerialClose(void *sl_handle)
{
	return ML_SUCCESS;
}

tMLError MLSLSerialWriteSingle(void *sl_handle,
			       unsigned char slaveAddr,
			       unsigned char registerAddr,
			       unsigned char data)
{
	return sensor_i2c_write_register((struct i2c_adapter *) sl_handle,
					 slaveAddr, registerAddr, data);
}


tMLError MLSLSerialWrite(void *sl_handle,
			 unsigned char slaveAddr,
			 unsigned short length, unsigned char const *data)
{
	tMLError result;
	const unsigned short dataLength = length - 1;
	const unsigned char startRegAddr = data[0];
	unsigned char i2cWrite[SERIAL_MAX_TRANSFER_SIZE + 1];
	unsigned short bytesWritten = 0;

	while (bytesWritten < dataLength) {
		unsigned short thisLen = min(SERIAL_MAX_TRANSFER_SIZE,
					     dataLength - bytesWritten);
		if (bytesWritten == 0) {
			result = sensor_i2c_write((struct i2c_adapter *)
						  sl_handle, slaveAddr,
						  1 + thisLen, data);
		} else {
			
			i2cWrite[0] = startRegAddr + bytesWritten;
			memcpy(&i2cWrite[1], &data[1 + bytesWritten],
			       thisLen);
			result = sensor_i2c_write((struct i2c_adapter *)
						  sl_handle, slaveAddr,
						  1 + thisLen, i2cWrite);
		}
		if (ML_SUCCESS != result)
			return result;
		bytesWritten += thisLen;
	}
	return ML_SUCCESS;
}


tMLError MLSLSerialRead(void *sl_handle,
			unsigned char slaveAddr,
			unsigned char registerAddr,
			unsigned short length, unsigned char *data)
{
	tMLError result;
	unsigned short bytesRead = 0;

	if (registerAddr == MPUREG_FIFO_R_W
	    || registerAddr == MPUREG_MEM_R_W) {
		return ML_ERROR_INVALID_PARAMETER;
	}
	while (bytesRead < length) {
		unsigned short thisLen =
		    min(SERIAL_MAX_TRANSFER_SIZE, length - bytesRead);
		result =
		    sensor_i2c_read((struct i2c_adapter *) sl_handle,
				    slaveAddr, registerAddr + bytesRead,
				    thisLen, &data[bytesRead]);
		if (ML_SUCCESS != result)
			return result;
		bytesRead += thisLen;
	}
	return ML_SUCCESS;
}


tMLError MLSLSerialWriteMem(void *sl_handle,
			    unsigned char slaveAddr,
			    unsigned short memAddr,
			    unsigned short length,
			    unsigned char const *data)
{
	tMLError result;
	unsigned short bytesWritten = 0;

	if ((memAddr & 0xFF) + length > MPU_MEM_BANK_SIZE) {
		printk
		    ("memory read length (%d B) extends beyond its limits (%d) "
		     "if started at location %d\n", length,
		     MPU_MEM_BANK_SIZE, memAddr & 0xFF);
		return ML_ERROR_INVALID_PARAMETER;
	}
	while (bytesWritten < length) {
		unsigned short thisLen =
		    min(SERIAL_MAX_TRANSFER_SIZE, length - bytesWritten);
		result =
		    mpu_memory_write((struct i2c_adapter *) sl_handle,
				     slaveAddr, memAddr + bytesWritten,
				     thisLen, &data[bytesWritten]);
		if (ML_SUCCESS != result)
			return result;
		bytesWritten += thisLen;
	}
	return ML_SUCCESS;
}


tMLError MLSLSerialReadMem(void *sl_handle,
			   unsigned char slaveAddr,
			   unsigned short memAddr,
			   unsigned short length, unsigned char *data)
{
	tMLError result;
	unsigned short bytesRead = 0;

	if ((memAddr & 0xFF) + length > MPU_MEM_BANK_SIZE) {
		printk
		    ("memory read length (%d B) extends beyond its limits (%d) "
		     "if started at location %d\n", length,
		     MPU_MEM_BANK_SIZE, memAddr & 0xFF);
		return ML_ERROR_INVALID_PARAMETER;
	}
	while (bytesRead < length) {
		unsigned short thisLen =
		    min(SERIAL_MAX_TRANSFER_SIZE, length - bytesRead);
		result =
		    mpu_memory_read((struct i2c_adapter *) sl_handle,
				    slaveAddr, memAddr + bytesRead,
				    thisLen, &data[bytesRead]);
		if (ML_SUCCESS != result)
			return result;
		bytesRead += thisLen;
	}
	return ML_SUCCESS;
}


tMLError MLSLSerialWriteFifo(void *sl_handle,
			     unsigned char slaveAddr,
			     unsigned short length,
			     unsigned char const *data)
{
	tMLError result;
	unsigned char i2cWrite[SERIAL_MAX_TRANSFER_SIZE + 1];
	unsigned short bytesWritten = 0;

	if (length > FIFO_HW_SIZE) {
		printk(KERN_ERR
		       "maximum fifo write length is %d\n", FIFO_HW_SIZE);
		return ML_ERROR_INVALID_PARAMETER;
	}
	while (bytesWritten < length) {
		unsigned short thisLen =
		    min(SERIAL_MAX_TRANSFER_SIZE, length - bytesWritten);
		i2cWrite[0] = MPUREG_FIFO_R_W;
		memcpy(&i2cWrite[1], &data[bytesWritten], thisLen);
		result = sensor_i2c_write((struct i2c_adapter *) sl_handle,
					  slaveAddr, thisLen + 1,
					  i2cWrite);
		if (ML_SUCCESS != result)
			return result;
		bytesWritten += thisLen;
	}
	return ML_SUCCESS;
}


tMLError MLSLSerialReadFifo(void *sl_handle,
			    unsigned char slaveAddr,
			    unsigned short length, unsigned char *data)
{
	tMLError result;
	unsigned short bytesRead = 0;

	if (length > FIFO_HW_SIZE) {
		printk(KERN_ERR
		       "maximum fifo read length is %d\n", FIFO_HW_SIZE);
		return ML_ERROR_INVALID_PARAMETER;
	}
	while (bytesRead < length) {
		unsigned short thisLen =
		    min(SERIAL_MAX_TRANSFER_SIZE, length - bytesRead);
		result =
		    sensor_i2c_read((struct i2c_adapter *) sl_handle,
				    slaveAddr, MPUREG_FIFO_R_W, thisLen,
				    &data[bytesRead]);
		if (ML_SUCCESS != result)
			return result;
		bytesRead += thisLen;
	}

	return ML_SUCCESS;
}

