/*******************************************************************************
################################################################################
#                             (C) STMicroelectronics 2012
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License version 2 and only version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#------------------------------------------------------------------------------
#                             Imaging Division
################################################################################
********************************************************************************/

#include "ilp0100_customer_platform.h"
#include <linux/errno.h>
#include <linux/fs.h> 
#include <linux/file.h> 
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include "file_operation.h"
#include <linux/firmware.h>


#ifdef LINUX_TEST
char gbuf[1000];
#endif

#define READ_FIRMWARE_COUNT 3
static int fw_count = 0;
static const struct firmware *fw[READ_FIRMWARE_COUNT];



ilp0100_error Ilp0100_readFirmware(struct msm_sensor_ctrl_t *s_ctrl, Ilp0100_structInitFirmware *InitFirmwareData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	
#ifdef ST_SPECIFIC
	uint8_t *pFileName[3];
#else
	uint8_t *pFileName[READ_FIRMWARE_COUNT];
#endif 

	ILP0100_LOG_FUNCTION_START((void*)&InitFirmwareData);

#ifdef ST_SPECIFIC
	
	
	pFileName[0]=".\\ILP0100_IPM_Code_out.bin";

	
	Ret=Ilp0100_readFileInBuffer(pFileName[0],&InitFirmwareData->pIlp0100Firmware, &InitFirmwareData->Ilp0100FirmwareSize );
	if(Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("\n Error in Reading Firmware code Binary");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}
	
	pFileName[1]=".\\ILP0100_IPM_Data_out.bin";

	
	Ret=Ilp0100_readFileInBuffer(pFileName[1],&InitFirmwareData->pIlp0100SensorGenericCalibData, &InitFirmwareData->Ilp0100SensorGenericCalibDataSize );
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("\n Error in Reading Calib Data");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	
	pFileName[2]=".\\ILP0100_lscbuffer_out.bin";

	
	Ret=Ilp0100_readFileInBuffer(pFileName[2],&InitFirmwareData->pIlp0100SensorRawPart2PartCalibData, &InitFirmwareData->Ilp0100SensorRawPart2PartCalibDataSize );
	if(Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("\n Error in Reading Part to Part Calib Data Binary");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}
#else 

	
	pFileName[0]="ILP0100_IPM_Code_out.bin";

	
	Ret=Ilp0100_readFileInBuffer(s_ctrl, pFileName[0],
		&InitFirmwareData->pIlp0100Firmware, 
		&(InitFirmwareData->Ilp0100FirmwareSize));
	if(Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("\n Error in Reading Firmware code Binary");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	pFileName[1]="ILP0100_IPM_Data_out.bin";

	
	Ret=Ilp0100_readFileInBuffer(s_ctrl, pFileName[1],
		&InitFirmwareData->pIlp0100SensorGenericCalibData,
		&(InitFirmwareData->Ilp0100SensorGenericCalibDataSize));
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("\n Error in Reading Calib Data");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	pFileName[2]="lscbuffer_rev2.bin";

	
	Ret=Ilp0100_readFileInBuffer(s_ctrl, pFileName[2],&InitFirmwareData->pIlp0100SensorRawPart2PartCalibData, &InitFirmwareData->Ilp0100SensorRawPart2PartCalibDataSize );
	if(Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("\n Error in Reading Part to Part Calib Data Binary");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

#endif 

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

void Ilp0100_store_firmwarwe(const struct firmware *fw_yushanII){
	fw[fw_count]=fw_yushanII;
	fw_count ++;
	return;
}

void Ilp0100_release_firmware(void){
	int i;
	for(i = 0; i< fw_count;i++){
		release_firmware(fw[i]);
	}
	fw_count = 0;
	return;
}


ilp0100_error Ilp0100_readFileInBuffer(struct msm_sensor_ctrl_t *s_ctrl, uint8_t *pFileName,uint8_t **pAdd, uint32_t* SizeOfBuffer)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;

#ifdef ST_SPECIFIC
	FILE *pFile;
	uint8_t *pBuffer;
	uint32_t Result;
	uint32_t Size;
#else
	const struct firmware *fw_rawchip2 = NULL;
	int rc = 0;
	unsigned char *rawchipII_data_fw;
	u32 rawchipII_fw_size;
#endif 

	ILP0100_LOG_FUNCTION_START((void*)&pFileName, (void*)&pAdd);

#ifdef ST_SPECIFIC
	
	pFile = fopen ( pFileName , "rb" );
	
	if (pFile==NULL)
	{
		ILP0100_ERROR_LOG("\nFile error ");
		ILP0100_LOG_FUNCTION_END(Ret);
		return ILP0100_ERROR;
	}

	
	fseek (pFile , 0 , SEEK_END);
	Size = ftell (pFile);
	rewind (pFile);

	
	pBuffer= (uint8_t*) malloc (sizeof(uint8_t)*Size);
	if (pBuffer == NULL)
	{
		ILP0100_ERROR_LOG ("\n Memory allocation failed");
		ILP0100_LOG_FUNCTION_END(Ret);
	   	return ILP0100_ERROR;
	}

	
	Result = fread (pBuffer,1,Size,pFile);
	if (Result != Size)
	{
		Ret=ILP0100_ERROR;
		ILP0100_ERROR_LOG("\n File Reading to buffer error");
		ILP0100_ERROR_LOG("\n Result read=%d", Result);
		free(pBuffer); 	
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	*pAdd= (uint8_t*)pBuffer;
	*SizeOfBuffer=Size;
	fclose(pFile);
#else
	
	rc = request_firmware(&fw_rawchip2, pFileName, &(s_ctrl->sensor_i2c_client->client->dev));
	if (rc!=0) {
		pr_info("request_firmware for error %d\n", rc);
		Ret = ILP0100_ERROR;
		return Ret;
	}
	if (!fw_rawchip2) {
		pr_info("fw_rawchip2 is null\n");
		Ret = ILP0100_ERROR;
		return Ret;
	}
	
	Ilp0100_store_firmwarwe(fw_rawchip2);
	rawchipII_data_fw = (unsigned char *)fw_rawchip2->data;
	rawchipII_fw_size = (u32) fw_rawchip2->size;

	*pAdd = (uint8_t*)rawchipII_data_fw;
	*SizeOfBuffer = rawchipII_fw_size;
	
#endif 
	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_interruptHandler()
{
	ilp0100_error Ret= ILP0100_ERROR_NONE;
	
#ifdef ST_SPECIFIC
	bool_t Pin=INTR_PIN_0;
	uint32_t InterruptReadStatusPin0, InterruptReadStatusPin1;
#else
#endif 

	ILP0100_LOG_FUNCTION_START(NULL);

#ifdef ST_SPECIFIC
	
	Pin=INTR_PIN_0;
	Ret= Ilp0100_interruptReadStatus(&InterruptReadStatusPin0, Pin);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("Ilp0100_core_interruptReadStatus failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	if(InterruptReadStatusPin0)
	{
		Ret= Ilp0100_interruptManager(InterruptReadStatusPin0, Pin);
		if (Ret!=ILP0100_ERROR_NONE)
		{
			ILP0100_ERROR_LOG("Ilp0100_interruptManagerPin0 failed");
			ILP0100_LOG_FUNCTION_END(Ret);
			return Ret;
		}
	}

	Pin=INTR_PIN_1;
	
	Ret= Ilp0100_interruptReadStatus(&InterruptReadStatusPin1, Pin);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("Ilp0100_core_interruptReadStatus failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	if(InterruptReadStatusPin1)
	{
		Ret= Ilp0100_interruptManager(InterruptReadStatusPin1, Pin);
		if (Ret!=ILP0100_ERROR_NONE)
		{
			ILP0100_ERROR_LOG("Ilp0100_interruptManagerPin1 failed");
			ILP0100_LOG_FUNCTION_END(Ret);
			return Ret;
		}
	}

#else
#endif 

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_interruptManager(uint32_t InterruptId, bool_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&InterruptId, (void*)&Pin);
#ifdef ST_SPECIFIC
	if(Pin==INTR_PIN_0)
	{
		for(i=0;i<32;i++)
		{
		
			switch (InterruptId&(0x01<<i))
		{
		case INTR_LONGEXP_GLACE_STATS_READY:
			break;
		case INTR_LONGEXP_HISTOGRAM_STATS_READY:
			break;
		case INTR_SHORTEXP_GLACE_STATS_READY:
			break;
		case INTR_SHORTEXP_HISTOGRAM_STATS_READY:
			break;

		default:
			break;


		}
	}
	}
	else
	{
		
	}

	
	Ret= Ilp0100_interruptClearStatus(InterruptId, Pin);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("Ilp0100_core_interruptClearStatus failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

#else
#endif 

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}


ilp0100_error Ilp0100_GetTimeStamp(uint32_t *pTimeStamp)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;

#ifdef ST_SPECIFIC
	*pTimeStamp=0x00;
#else
#endif 

	return Ret;
}

#if 0
ilp0100_error Ilp0100_DumpLogInFile(uint8_t *pFileName)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint8_t *pBuffer;
	FILE *pFile;

	uint32_t BufferSize;

	Ret= Ilp0100_loggingGetSize(&BufferSize);
	if(Ret==ILP0100_ERROR_NONE){
		pBuffer = kmalloc(size_t size, gfp_t flags)(sizeof(uint8_t)*BufferSize);
		Ret     = Ilp0100_loggingReadBack(pBuffer, &BufferSize);
	}
	
	
	if(Ret==ILP0100_ERROR_NONE){
		pFile = fopen ( pFileName , "wb" );
		fwrite(pBuffer, sizeof(uint8_t), BufferSize, pFile);
		fclose(pFile);
	}

	return Ret;
}
#endif
