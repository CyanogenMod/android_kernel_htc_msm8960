/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#include "yushanII.h"
#include "rawchip_spi.h"
#include "ilp0100_ST_debugging.h"
#include "file_operation.h"
#include "ilp0100_ST_definitions.h"
#include "tone_map.h"
#include "ilp0100_customer_iq_settings.h"
#include <linux/errno.h>

#define MSM_YUSHANII_NAME "rawchip"

#undef HDR_COLOR_BAR
#undef DUMP_LOGGING_FILE
#undef BYPASS_MODE
#define YUSHANII_CONFIG_DUMP

static int reload_firmware=1;
static int first_init_2nd_cam = 1;
static uint32_t pInterruptId, pInterruptId2;
struct yushanii_hist hist;

#if 1
static Ilp0100_structGlaceStatsData glace_long;
static Ilp0100_structHistStatsData Hist_long;
static Ilp0100_structGlaceStatsData glace_short;
static Ilp0100_structHistStatsData Hist_short;
#endif

static struct YushanII_id_info_t YushanII_id_info = {
	.YushanII_id_reg_addr = 0x03c4,
	.YushanII_id = 0x4100110c,
};

static struct YushanII_info_t yushanII_info = {
	.yushanII_id_info = &YushanII_id_info,
};

static struct class *yushanII_class;
static dev_t yushanII_devno;

static struct msm_sensor_ctrl_t *g_sensor = NULL;	

int yushanII_intr0, yushanII_intr1;
atomic_t interrupt, interrupt2;

struct yushanII_int_t yushanII_int;

struct yushanII_int_t {
	spinlock_t yushan_spin_lock;
	wait_queue_head_t yushan_wait;
};

static struct YushanII_ctrl *YushanIICtrl = NULL;

int YushanII_Get_reloadInfo(void){
	return reload_firmware;
}

void YushanII_reload_firmware(void){
	reload_firmware = 1;
	first_init_2nd_cam = 1;
}


void dump_logging_file(uint8_t *buf, uint32_t size){
	uint8_t *path= "/data/ST_logging.xml";
	struct file *pFile;
	
	pFile = file_open(path, O_CREAT|O_RDWR,0666);
	if(pFile == NULL){
		pr_err("open file %s fail",path);
		return;
	}
	file_write(pFile, 0, buf, size);
	file_close(pFile);
}


void YushanII_login_start(void){
       #ifdef DUMP_LOGGING_FILE
       pr_info("[CAM] %s",__func__);
       Ilp0100_loggingOpen();
       Ilp0100_loggingStart(1);
       #endif
       return;
}

void YushanII_login_stop(void){
       #ifdef DUMP_LOGGING_FILE
       uint32_t LogSize;
       uint8_t *pLog;
       uint16_t error;
       pr_info("[CAM] %s",__func__);
       Ilp0100_loggingStop();
       Ilp0100_loggingGetSize(&LogSize);
       pLog = kmalloc(LogSize,GFP_KERNEL);
       memset(pLog,0x00,LogSize);
       pr_info("[CAM] read back LogSize:%d",LogSize);
       error = Ilp0100_loggingReadBack(pLog,&LogSize);
       pr_info("[CAM] read back error no:%d",error);
       dump_logging_file(pLog,LogSize);
       Ilp0100_loggingClose();
       #endif
       return;
}



void YushanII_default_exp(void){
	Ilp0100_structFrameParams long_exp,short_exp;
	
	
	long_exp.AnalogGainCodeBlue = 128;
	long_exp.AnalogGainCodeGreen = 128;
	long_exp.AnalogGainCodeRed = 128;
	long_exp.ExposureTime = 400;
	long_exp.DigitalGainCodeBlue = 256;
	long_exp.DigitalGainCodeGreen = 256;
	long_exp.DigitalGainCodeRed = 256;

	Ilp0100_updateSensorParamsLong(long_exp);

	
	short_exp.AnalogGainCodeBlue = 128;
	short_exp.AnalogGainCodeGreen = 128;
	short_exp.AnalogGainCodeRed = 128;
	short_exp.ExposureTime = 128;
	short_exp.DigitalGainCodeBlue = 256;
	short_exp.DigitalGainCodeGreen = 256;
	short_exp.DigitalGainCodeRed = 256;

	Ilp0100_updateSensorParamsShortOrNormal(short_exp);
}


int  YushanII_set_default_IQ2(void)
{
	
	int channel_offset = 0;
	YushanII_set_channel_offset(channel_offset);

	return 0;
}

int  YushanII_set_default_IQ(struct msm_sensor_ctrl_t *sensor)
{
	
	int channel_offset = 64;
	
	int tone_map = 46;
	
	int disable_defcor = 0;
	
	struct yushanii_cls cls;
	cls.cls_enable = 0;
	cls.color_temp = 5000;

	if (sensor->func_tbl->sensor_yushanII_set_IQ)
	    sensor->func_tbl->sensor_yushanII_set_IQ (sensor,&channel_offset,&tone_map,&disable_defcor,&cls);

    if (sensor->is_black_level_calibration_ongoing) {
        channel_offset=0;
        pr_info("[CAM] %s: force channel offset to 0 in black level ongoing mode.",__func__);
    }

	YushanII_set_channel_offset(channel_offset);
	YushanII_set_tone_mapping(tone_map);
	YushanII_set_defcor(disable_defcor);
	YushanII_set_cls(&cls);
	return 0;
}


void YushanII_set_pixel_order(
	struct msm_sensor_ctrl_t *sensor,
	Ilp0100_structSensorParams *YushanII_sensor)
{
	switch (sensor->sensordata->sensor_platform_info->pixel_order_default) {
	    case MSM_CAMERA_PIXEL_ORDER_GR:
	        YushanII_sensor->PixelOrder = GR;
	        break;
	    case MSM_CAMERA_PIXEL_ORDER_RG:
	        YushanII_sensor->PixelOrder = RG;
	        break;
	    case MSM_CAMERA_PIXEL_ORDER_BG:
	        YushanII_sensor->PixelOrder = BG;
	        break;
	    case MSM_CAMERA_PIXEL_ORDER_GB:
	        YushanII_sensor->PixelOrder = GB;
	        break;
	    default:
	        YushanII_sensor->PixelOrder = GR;
	        break;
	}

	pr_info("[CAM]%s,pixel_order_default=%d,  PixelOrder=%d", 
		__func__, 
		sensor->sensordata->sensor_platform_info->pixel_order_default, 
		YushanII_sensor->PixelOrder);

}
void YushanII_set_parm(
	struct msm_sensor_ctrl_t *sensor, int res,
	Ilp0100_structInit *YushanII_init,
	Ilp0100_structSensorParams *YushanII_sensor, int UsedSensorInterface)
{
	int pixel_format;
	uint32_t op_pixel_clk;
	pr_info("[CAM]%s",__func__);
	op_pixel_clk = 
		sensor->msm_sensor_reg->output_settings[res].op_pixel_clk;
	pixel_format =
		sensor->curr_csi_params->csid_params.lut_params.vc_cfg->dt;

	YushanII_init->NumberOfLanes =
		sensor->curr_csi_params->csid_params.lane_cnt;

	switch(pixel_format){
		case CSI_RAW8:
			YushanII_init->uwPixelFormat = RAW_8;
			break;
		case CSI_RAW10:
			YushanII_init->uwPixelFormat = RAW_10;
			break;
		case CSI_RAW12:
			YushanII_init->uwPixelFormat = RAW_12;
			break;
	}

	YushanII_init->BitRate = 
		(op_pixel_clk*YushanII_init->uwPixelFormat)/YushanII_init->NumberOfLanes;

	YushanII_init->ExternalClock = 24000000;
	YushanII_init->ClockUsed = ILP0100_CLOCK;

	
	YushanII_init->UsedSensorInterface = UsedSensorInterface;
	if(sensor->sensordata->hdr_mode){
		YushanII_init->IntrEnablePin1 = ENABLE_HTC_INTR;
		YushanII_init->IntrEnablePin2 = ENABLE_RECOMMENDED_DEBUG_INTR_PIN1;
	}else{ 
		YushanII_init->IntrEnablePin1 = ENABLE_NO_INTR;
		YushanII_init->IntrEnablePin2 = ENABLE_NO_INTR;
	}

	if (sensor->func_tbl->sensor_yushanII_set_parm)
	    sensor->func_tbl->sensor_yushanII_set_parm (sensor,res,YushanII_sensor);

	
	YushanII_set_pixel_order(sensor,YushanII_sensor);	


	#ifdef YUSHANII_CONFIG_DUMP
	pr_info("[CAM]%s,pixel_format=%d, NumberOfLanes=%d,camera_type=%d",
		__func__,
		pixel_format,
		YushanII_init->NumberOfLanes,
		sensor->sensordata->camera_type);

	pr_info("[CAM]%s,bitrate:%d",__func__, YushanII_init->BitRate);

	pr_info("[CAM]%s,UsedSensorInterface=%d, IntrEnablePin1=%d, IntrEnablePin2=%d",
		__func__,
		YushanII_init->UsedSensorInterface,
		YushanII_init->IntrEnablePin1,
		YushanII_init->IntrEnablePin2);
	pr_info("[CAM]%s,FullActiveLines=%d, FullActivePixels=%d, MinLineLength=%d",
		__func__,
		YushanII_sensor->FullActiveLines,
		YushanII_sensor->FullActivePixels,
		YushanII_sensor->MinLineLength);

	pr_info("[CAM]%s,PixelOrder=%d",
		__func__,
		YushanII_sensor->PixelOrder);
	#endif
}

void YushanII_set_outputformat(struct msm_sensor_ctrl_t *sensor,int res,
	Ilp0100_structFrameFormat *output_format)
{

	int pixel_format = sensor->curr_csi_params->csid_params.lut_params.vc_cfg->dt;
	switch(pixel_format){
		case CSI_RAW8:
			output_format->uwOutputPixelFormat = RAW_8;
			break;
		case CSI_RAW10:
			output_format->uwOutputPixelFormat = RAW_10;
			break;
		case CSI_RAW12:
			output_format->uwOutputPixelFormat = RAW_12;
			break;
	}
	if (sensor->func_tbl->sensor_yushanII_set_output_format)
        sensor->func_tbl->sensor_yushanII_set_output_format (sensor,res,output_format);
	#ifdef YUSHANII_CONFIG_DUMP
	pr_info("[CAM]%s Hoffset:%d", __func__, output_format->Hoffset);
	pr_info("[CAM]%s Voffset:%d", __func__,output_format->Voffset);
	pr_info("[CAM]%s ActiveLineLengthPixels:%d", __func__, output_format->ActiveLineLengthPixels);
	pr_info("[CAM]%s ActiveFrameLengthLines:%d", __func__,output_format->ActiveFrameLengthLines);
	pr_info("[CAM]%s LineLengthPixels:%d", __func__,output_format->LineLengthPixels);
	pr_info("[CAM]%s FrameLengthLines:%d", __func__,output_format->FrameLengthLines);
	pr_info("[CAM]%s StatusLinesOutputted:%d", __func__,output_format->StatusLinesOutputted);
	pr_info("[CAM]%s StatusNbLines:%d", __func__,output_format->StatusNbLines);
	pr_info("[CAM]%s ImageOrientation:%d", __func__,output_format->ImageOrientation);
	pr_info("[CAM]%s StatusLineLengthPixels:%d", __func__,output_format->StatusLineLengthPixels);
	pr_info("[CAM]%s MinInterframe:%d", __func__,output_format->MinInterframe);
	pr_info("[CAM]%s HDRMode:%d", __func__,output_format->HDRMode);
	#endif
	return;
}

void YushanII_set_frontcam_parm(
	struct msm_sensor_ctrl_t *sensor, int res,
	Ilp0100_structInit *YushanII_init,
	Ilp0100_structSensorParams *YushanII_sensor)
{
	int pixel_format;
	uint32_t op_pixel_clk;
	op_pixel_clk = 
		sensor->msm_sensor_reg->output_settings[res].op_pixel_clk;

	pr_info("[CAM]%s",__func__);
	pixel_format =
		sensor->curr_csi_params->csid_params.lut_params.vc_cfg->dt;

	YushanII_init->NumberOfLanes =
		sensor->curr_csi_params->csid_params.lane_cnt;

	switch(pixel_format){
		case CSI_RAW8:
			YushanII_init->uwPixelFormat = RAW_8;
			break;
		case CSI_RAW10:
			YushanII_init->uwPixelFormat = RAW_10;
			break;
		case CSI_RAW12:
			YushanII_init->uwPixelFormat = RAW_12;
			break;
	}

	YushanII_init->BitRate = 
		(op_pixel_clk*YushanII_init->uwPixelFormat)/YushanII_init->NumberOfLanes;


	YushanII_init->ExternalClock = 24000000;
	YushanII_init->ClockUsed = ILP0100_CLOCK;

	
	YushanII_init->UsedSensorInterface = SENSOR_1;
	YushanII_init->IntrEnablePin1 = ENABLE_NO_INTR;
	YushanII_init->IntrEnablePin2 = ENABLE_NO_INTR;
	

	
	YushanII_sensor->FullActiveLines =
		sensor->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
	
	YushanII_sensor->FullActivePixels =
		sensor->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
	
	YushanII_sensor->MinLineLength =
		sensor->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk;
	
	YushanII_set_pixel_order(sensor,YushanII_sensor);	
	YushanII_sensor->StatusNbLines = 0;

	#ifdef YUSHANII_CONFIG_DUMP
	pr_info("[CAM]%s,bitrate:%d",__func__, YushanII_init->BitRate);

	pr_info("[CAM]%s,pixel_format=%d, NumberOfLanes=%d,camera_type=%d",
		__func__,
		pixel_format,
		YushanII_init->NumberOfLanes,
		sensor->sensordata->camera_type);

	pr_info("[CAM]%s,UsedSensorInterface=%d, IntrEnablePin1=%d, IntrEnablePin2=%d",
		__func__,
		YushanII_init->UsedSensorInterface,
		YushanII_init->IntrEnablePin1,
		YushanII_init->IntrEnablePin2);
	pr_info("[CAM]%s,FullActiveLines=%d, FullActivePixels=%d, MinLineLength=%d",
		__func__,
		YushanII_sensor->FullActiveLines,
		YushanII_sensor->FullActivePixels,
		YushanII_sensor->MinLineLength);

	pr_info("[CAM]%s,PixelOrder=%d",
		__func__,
		YushanII_sensor->PixelOrder);

	pr_info("[CAM]%s, StatusLine:%d",
		__func__,
		YushanII_sensor->StatusNbLines);
	#endif
}

void YushanII_set_frontcam_outputformat(struct msm_sensor_ctrl_t *sensor,int res,
	Ilp0100_structFrameFormat *output_format)
{

	int pixel_format ;
	pixel_format =
		sensor->curr_csi_params->csid_params.lut_params.vc_cfg->dt;

	pr_info("[CAM]%s",__func__);
	output_format->ActiveLineLengthPixels =
		sensor->msm_sensor_reg->output_settings[res].x_output;
	output_format->ActiveFrameLengthLines =
		sensor->msm_sensor_reg->output_settings[res].y_output;
	output_format->LineLengthPixels =
		sensor->msm_sensor_reg->output_settings[res].line_length_pclk;

	output_format->FrameLengthLines =
		sensor->msm_sensor_reg->output_settings[res].frame_length_lines;

	switch(pixel_format){
		case CSI_RAW8:
			output_format->uwOutputPixelFormat = RAW_8;
			break;
		case CSI_RAW10:
			output_format->uwOutputPixelFormat = RAW_10;
			break;
		case CSI_RAW12:
			output_format->uwOutputPixelFormat = RAW_12;
			break;
	}

	
	
	output_format->StatusLinesOutputted = FALSE;
	output_format->StatusNbLines = 0;
	output_format->ImageOrientation = sensor->sensordata->sensor_platform_info->mirror_flip;
	output_format->FrameLengthLines = sensor->msm_sensor_reg->output_settings[res].frame_length_lines;

	output_format->StatusLineLengthPixels =
		sensor->msm_sensor_reg->output_settings[res].x_output;
	

	output_format->MinInterframe =
		(output_format->FrameLengthLines -
		output_format->ActiveFrameLengthLines -
		output_format->StatusNbLines);

	output_format->AutomaticFrameParamsUpdate = TRUE;
	output_format->HDRMode = HDR_NONE;
	output_format->Hoffset = 0;
	output_format->Voffset = 0;

	
	output_format->HScaling = 1;
	output_format->VScaling = 1;
	output_format->Binning = 0x11;
	#ifdef YUSHANII_CONFIG_DUMP
	pr_info("[CAM]%s Hoffset:%d", __func__, output_format->Hoffset);
	pr_info("[CAM]%s Voffset:%d", __func__,output_format->Voffset);
	pr_info("[CAM]%s ActiveLineLengthPixels:%d", __func__, output_format->ActiveLineLengthPixels);
	pr_info("[CAM]%s ActiveFrameLengthLines:%d", __func__,output_format->ActiveFrameLengthLines);
	pr_info("[CAM]%s LineLengthPixels:%d", __func__,output_format->LineLengthPixels);
	pr_info("[CAM]%s FrameLengthLines:%d", __func__,output_format->FrameLengthLines);
	pr_info("[CAM]%s StatusLinesOutputted:%d", __func__,output_format->StatusLinesOutputted);
	pr_info("[CAM]%s StatusNbLines:%d", __func__,output_format->StatusNbLines);
	pr_info("[CAM]%s ImageOrientation:%d", __func__,output_format->ImageOrientation);
	pr_info("[CAM]%s StatusLineLengthPixels:%d", __func__,output_format->StatusLineLengthPixels);
	pr_info("[CAM]%s MinInterframe:%d", __func__,output_format->MinInterframe);
	pr_info("[CAM]%s HDRMode:%d", __func__,output_format->HDRMode);
	#endif
	return;
};



void YushanII_init_backcam(struct msm_sensor_ctrl_t *sensor,int res){
	Ilp0100_structInitFirmware YushanII_InitFirmware, YushanII_emptyFirmware;
	Ilp0100_structInit YushanII_init;
	Ilp0100_structSensorParams YushanII_sensor;
	uint8_t pMajorNumber,pMinorNumber;
	uint8_t pMajorAPINum,pMinorAPINum;
	Ilp0100_structFrameFormat output_format;
	static uint32_t Previous_MIPIclock = 0, reinit = false;

	pr_info("[CAM]%s,res=%d,is_hdr=%d",
		__func__, res,sensor->msm_sensor_reg->output_settings[res].is_hdr);
	YushanII_login_start();

	g_sensor = sensor;	

	
	if(sensor->msm_sensor_reg->output_settings[res].is_hdr)
		sensor->sensordata->hdr_mode = 1;
	else
		sensor->sensordata->hdr_mode = 0;

	YushanII_set_parm(sensor,res,&YushanII_init,&YushanII_sensor,SENSOR_0);

	if(reload_firmware){
		Ilp0100_readFirmware(sensor, &YushanII_InitFirmware);
		Ilp0100_init(YushanII_init, YushanII_InitFirmware, YushanII_sensor);
		Ilp0100_getFirmwareVersionumber(&pMajorNumber, &pMinorNumber);
		pr_info("[CAM]%s, firmware version major:%d minor:%d", __func__, pMajorNumber,pMinorNumber);
		Ilp0100_getApiVersionNumber(&pMajorAPINum, &pMinorAPINum);
		pr_info("[CAM]%s, API version major:%d minor:%d", __func__, pMajorAPINum,pMinorAPINum);
		reload_firmware = 0;
		Previous_MIPIclock =
			sensor->msm_sensor_reg->output_settings[res].op_pixel_clk;
	}

	
	if(sensor->msm_sensor_reg->output_settings[res].op_pixel_clk != Previous_MIPIclock){
		reinit = true;
		Previous_MIPIclock =
			sensor->msm_sensor_reg->output_settings[res].op_pixel_clk;
	}
	if(reinit) {
		memset(&YushanII_emptyFirmware, 0x00, sizeof(Ilp0100_structInitFirmware));
		pr_info("[CAM]%s, change MIPI clock rate , reinit YushanII",__func__);
		reinit = false;
		Ilp0100_stop();
		Ilp0100_init(YushanII_init, YushanII_emptyFirmware, YushanII_sensor);
	}

	YushanII_set_outputformat(sensor,res,&output_format);

	
	Ilp0100_interruptDisable(0xFFFFFFFF, INTR_PIN_0);
	Ilp0100_interruptDisable(0xFFFFFFFF, INTR_PIN_1);

	if(sensor->sensordata->hdr_mode){
		
		Ilp0100_interruptEnable(ENABLE_HTC_INTR, INTR_PIN_0);
		Ilp0100_interruptEnable(ENABLE_RECOMMENDED_DEBUG_INTR_PIN1, INTR_PIN_1);
		if (sensor->yushanII_switch_virtual_channel) {
		    Ilp0100_setVirtualChannelShortOrNormal(1);
		    Ilp0100_setVirtualChannelLong(0);
		}
		pr_info("[CAM]%s, YushanII HDR mode", __func__);
		Ilp0100_defineMode(output_format);
		YushanII_default_exp();
		
		#ifdef HDR_COLOR_BAR
		Ilp0100_startTestMode(BYPASS_NO_BYPASS, TEST_COLORBAR);
		#endif

	}else{
		
		Ilp0100_interruptEnable(ENABLE_NO_INTR, INTR_PIN_0);
		Ilp0100_interruptEnable(ENABLE_RECOMMENDED_DEBUG_INTR_PIN1, INTR_PIN_1);
		if (sensor->yushanII_switch_virtual_channel) {
	        Ilp0100_setVirtualChannelShortOrNormal(0);
	        Ilp0100_setVirtualChannelLong(1);
	    }
		pr_info(" [CAM]%s, YushanII NON HDR mode", __func__);
		Ilp0100_defineMode(output_format);
		#ifdef HDR_COLOR_BAR
		Ilp0100_startTestMode(BYPASS_NO_BYPASS, TEST_COLORBAR);
		#endif
	}

	#ifdef BYPASS_MODE
	Ilp0100_startTestMode(BYPASS_ALL, TEST_NO_TEST_MODE);  
	#endif

	YushanII_set_default_IQ(sensor);	
}

void YushanII_init_frontcam(struct msm_sensor_ctrl_t *sensor,int res){
	Ilp0100_structInitFirmware YushanII_InitFirmware;
	Ilp0100_structInit YushanII_init;
	Ilp0100_structSensorParams YushanII_sensor;
	uint8_t pMajorNumber,pMinorNumber;
	uint8_t pMajorAPINum,pMinorAPINum;
	Ilp0100_structFrameFormat output_format;

	pr_info("[CAM]%s,res=%d,is_hdr=%d",
		__func__, res,sensor->msm_sensor_reg->output_settings[res].is_hdr);

	YushanII_login_start();

	g_sensor = sensor;	

	sensor->sensordata->hdr_mode = 0;

	YushanII_set_frontcam_parm(sensor,res,&YushanII_init,&YushanII_sensor);
	

	if(reload_firmware){
		Ilp0100_readFirmware(sensor, &YushanII_InitFirmware);
		Ilp0100_init(YushanII_init, YushanII_InitFirmware, YushanII_sensor);
		Ilp0100_getFirmwareVersionumber(&pMajorNumber, &pMinorNumber);
		pr_info("[CAM]%s, firmware version major:%d minor:%d", __func__, pMajorNumber,pMinorNumber);
		Ilp0100_getApiVersionNumber(&pMajorAPINum, &pMinorAPINum);
		pr_info("[CAM]%s, API version major:%d minor:%d", __func__, pMajorAPINum,pMinorAPINum);
		
		Ilp0100_interruptDisable(0xFFFFFFFF, INTR_PIN_0);
		Ilp0100_interruptDisable(0xFFFFFFFF, INTR_PIN_1);
		
		Ilp0100_interruptEnable(ENABLE_NO_INTR, INTR_PIN_0);
		Ilp0100_interruptEnable(ENABLE_ALL_ERROR_INTR, INTR_PIN_1);
		reload_firmware = 0;
	}
	YushanII_set_frontcam_outputformat(sensor,res,&output_format);

	pr_info("[CAM]%s, camera_type = %d, first_init_2nd_cam=%d",
		__func__,
		sensor->sensordata->camera_type,
		first_init_2nd_cam);

	if (first_init_2nd_cam) {
		Ilp0100_defineMode(output_format);
		#ifdef HDR_COLOR_BAR
		Ilp0100_startTestMode(BYPASS_NO_BYPASS, TEST_COLORBAR);
		#else
		Ilp0100_startTestMode(ENABLE_CHANNEL_OFFSET_ONLY, TEST_NO_TEST_MODE);
		#endif
		first_init_2nd_cam = 0;
	}
	YushanII_set_default_IQ2();

}

void YushanII_Init(struct msm_sensor_ctrl_t *sensor,int res)
{
	if(sensor->sensordata->camera_type == BACK_CAMERA_2D){
		YushanII_init_backcam(sensor,res);
	}else{
		YushanII_init_frontcam(sensor,res);
	}
}


int YushanII_power_up(const struct msm_camera_rawchip_info *pdata)
{
	int rc = 0;
	CDBG("[CAM] %s\n", __func__);

	if (pdata->camera_rawchip_power_on == NULL) {
		pr_err("YushanII power on platform_data didn't register\n");
		return -EIO;
	}
	rc = pdata->camera_rawchip_power_on();
	if (rc < 0) {
		pr_err("YushanII power on failed\n");
		goto enable_power_on_failed;
	}

#ifdef CONFIG_RAWCHIP_MCLK
	rc = msm_camio_clk_enable(0,CAMIO_CAM_RAWCHIP_MCLK_CLK);
#else
	rc = msm_camio_clk_enable(0,CAMIO_CAM_MCLK_CLK);
#endif

	if (rc < 0) {
		pr_err("enable MCLK failed\n");
		goto enable_mclk_failed;
	}
	mdelay(1);

	rc = gpio_request(pdata->rawchip_reset, "YushanII");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed\n", pdata->rawchip_reset);
		goto enable_reset_failed;
	}
	gpio_direction_output(pdata->rawchip_reset, 1);
	gpio_free(pdata->rawchip_reset);
	mdelay(1);
	Ilp0100_enableIlp0100SensorClock(SENSOR_0);
	mdelay(1);

	return rc;

enable_reset_failed:
#ifdef CONFIG_RAWCHIP_MCLK
	rc = msm_camio_clk_disable(0,CAMIO_CAM_RAWCHIP_MCLK_CLK);
#else
	rc = msm_camio_clk_disable(0,CAMIO_CAM_MCLK_CLK);
#endif

enable_mclk_failed:
	if (pdata->camera_rawchip_power_off == NULL)
		pr_err("YushanII power off platform_data didn't register\n");
	else
		pdata->camera_rawchip_power_off();
enable_power_on_failed:
	return rc;
}

int YushanII_power_down(const struct msm_camera_rawchip_info *pdata)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	rc = gpio_request(pdata->rawchip_reset, "YushanII");
	if (rc < 0)
		pr_err("GPIO(%d) request failed\n", pdata->rawchip_reset);
	gpio_direction_output(pdata->rawchip_reset, 0);
	gpio_free(pdata->rawchip_reset);

	mdelay(1);

#ifdef CONFIG_RAWCHIP_MCLK
	rc = msm_camio_clk_disable(0,CAMIO_CAM_RAWCHIP_MCLK_CLK);
#else
	rc = msm_camio_clk_disable(0,CAMIO_CAM_MCLK_CLK);
#endif

	if (rc < 0)
		pr_err("disable MCLK failed\n");

	if (pdata->camera_rawchip_power_off == NULL) {
		pr_err("YushanII power off platform_data didn't register\n");
		return -EIO;
	}

	rc = pdata->camera_rawchip_power_off();
	if (rc < 0)
		pr_err("YushanII power off failed\n");

	return rc;
}

extern uint32_t YushanII_id;

int YushanII_match_id(void)
{
	int rc = 0;
	uint32_t chipid = 0;
	int retry_spi_cnt = 0, retry_readid_cnt = 0;
	uint8_t read_byte;
	int i;

	for (retry_spi_cnt = 0, retry_readid_cnt = 0; (retry_spi_cnt < 1 && retry_readid_cnt < 1); ) {
		chipid = 0;
		
		for (i = 0; i < 4; i++) {
			rc = SPI_Read((yushanII_info.yushanII_id_info->YushanII_id_reg_addr + i), 1, (&read_byte));

			if (rc < 0) {
				pr_err("%s: read id failed\n", __func__);
				retry_spi_cnt++;
				pr_info("%s: retry: %d\n", __func__, retry_spi_cnt);
				mdelay(5);
				break;
			} else
				*((uint8_t*)(&chipid) + i) = read_byte;
		}
		if (rc < 0)
			continue;

		pr_info("%s, YushanII id: 0x%x requested id: 0x%x\n",
			__func__, chipid, yushanII_info.yushanII_id_info->YushanII_id);
		if (chipid != yushanII_info.yushanII_id_info->YushanII_id) {
			pr_err("%s:YushanII_match_id chip id does not match\n", __func__);
			retry_readid_cnt++;
			pr_info("%s: retry: %d\n", __func__, retry_readid_cnt);
			mdelay(5);
			rc = -ENODEV;
			continue;
		} else
			break;
	}

	YushanII_id = chipid;
	return rc;
}

void YushanII_release(void)
{
	struct msm_camera_rawchip_info *pdata = YushanIICtrl->pdata;
	YushanII_login_stop();
	pr_info("[CAM]%s\n", __func__);
	Ilp0100_interruptDisable(0xFFFFFFFF, INTR_PIN_0);
	Ilp0100_interruptDisable(0xFFFFFFFF, INTR_PIN_0);
	CDBG("[CAM]%s: YushanII free irq", __func__);

	free_irq(pdata->rawchip_intr0, 0);
	free_irq(pdata->rawchip_intr1, 0);
	Ilp0100_release_firmware();
	YushanII_power_down(pdata);
}


static irqreturn_t yushanII_irq_handler(int irq, void *dev_id){

	unsigned long flags;
	CDBG("%s after detect INT0, interrupt:%d %d\n",
		__func__, atomic_read(&interrupt),__LINE__);
	disable_irq_nosync(YushanIICtrl->pdata->rawchip_intr0);
	spin_lock_irqsave(&yushanII_int.yushan_spin_lock,flags);
	atomic_set(&interrupt, 1);
	CDBG("%s after detect INT0, interrupt:%d\n",__func__, atomic_read(&interrupt));
	wake_up(&yushanII_int.yushan_wait);
	spin_unlock_irqrestore(&yushanII_int.yushan_spin_lock,flags);

	return IRQ_HANDLED;
}


static irqreturn_t yushanII_irq_handler2(int irq, void *dev_id){

	unsigned long flags;
	CDBG("%s after detect INT1, interrupt:%d %d\n",
		__func__, atomic_read(&interrupt2),__LINE__);
	disable_irq_nosync(YushanIICtrl->pdata->rawchip_intr1);
	spin_lock_irqsave(&yushanII_int.yushan_spin_lock,flags);
	atomic_set(&interrupt2, 1);
	CDBG("%s after detect INT1, interrupt:%d\n",__func__, atomic_read(&interrupt2));
	wake_up(&yushanII_int.yushan_wait);
	spin_unlock_irqrestore(&yushanII_int.yushan_spin_lock,flags);

	return IRQ_HANDLED;
}


int YushanII_open_init(void)
{
	int rc = 0;
	struct msm_camera_rawchip_info *pdata = YushanIICtrl->pdata;
	int read_id_retry = 0;

	pr_info("[CAM]%s\n", __func__);

open_read_id_retry:
	rc = YushanII_power_up(pdata);
	if (rc < 0)
		return rc;

	rc = YushanII_match_id();
	if (rc < 0) {
		if (read_id_retry < 3) {
			pr_info("[CAM]%s, retry read YushanII ID: %d\n", __func__, read_id_retry);
			read_id_retry++;
			YushanII_power_down(pdata);
			goto open_read_id_retry;
		}
		goto open_init_failed;
	}

	init_waitqueue_head(&yushanII_int.yushan_wait);
	spin_lock_init(&yushanII_int.yushan_spin_lock);
	atomic_set(&interrupt, 0);
	atomic_set(&interrupt2, 0);

	
	rc = request_irq(pdata->rawchip_intr0, yushanII_irq_handler,
		IRQF_TRIGGER_HIGH, "yushanII_irq", 0);
	if (rc < 0) {
		pr_err("[CAM]request irq intr0 failed\n");
		goto open_init_failed;
	}

	rc = request_irq(pdata->rawchip_intr1, yushanII_irq_handler2,
		IRQF_TRIGGER_HIGH, "yushanII_irq2", 0);
	if (rc < 0) {
		pr_err("request irq intr1 failed\n");
		goto open_init_failed;
	}

	YushanIICtrl->rawchip_init = 0;
	YushanIICtrl->total_error_interrupt_times = 0;
	
		
	atomic_set(&YushanIICtrl->check_intr0, 0);
	atomic_set(&YushanIICtrl->check_intr1, 0);
	yushanII_intr0 = pdata->rawchip_intr0;
	yushanII_intr1 = pdata->rawchip_intr1;

	return rc;

open_init_failed:
	pr_err("%s: YushanII_open_init failed\n", __func__);
	YushanII_power_down(pdata);

	return rc;
}



int YushanII_probe_init(void)
{
	int rc = 0;
	struct msm_camera_rawchip_info *pdata = NULL;
	int read_id_retry = 0;

	pr_info("%s\n", __func__);

	if (YushanIICtrl == NULL) {
		pr_err("already failed in __msm_YushanII_probe\n");
		return -EINVAL;
	} else
		pdata = YushanIICtrl->pdata;
probe_read_id_retry:
	rc = YushanII_power_up(pdata);
	if (rc < 0)
		return rc;

	rc = YushanII_match_id();
	if (rc < 0) {
		if (read_id_retry < 3) {
			pr_info("%s, retry read YushanII ID: %d\n", __func__, read_id_retry);
			read_id_retry++;
			YushanII_power_down(pdata);
			goto probe_read_id_retry;
		}
		goto probe_init_fail;
	}

	pr_info("%s: probe_success\n", __func__);
	return rc;

probe_init_fail:
	pr_err("%s: YushanII_probe_init failed\n", __func__);
	YushanII_power_down(pdata);
	return rc;
}

void YushanII_probe_deinit(void)
{
	struct msm_camera_rawchip_info *pdata = YushanIICtrl->pdata;
	CDBG("%s\n", __func__);

	YushanII_power_down(pdata);
}


static int YushanII_fops_open(struct inode *inode, struct file *filp)
{
	struct YushanII_ctrl *raw_dev = container_of(inode->i_cdev,
		struct YushanII_ctrl, cdev);

	filp->private_data = raw_dev;

	return 0;
}

static unsigned int YushanII_fops_poll(struct file *filp,
	struct poll_table_struct *pll_table)
{
	int rc = 0;
	unsigned long flags;
	poll_wait(filp, &yushanII_int.yushan_wait, pll_table);
	spin_lock_irqsave(&yushanII_int.yushan_spin_lock, flags);
	if (atomic_read(&interrupt)) {
		atomic_set(&interrupt, 0);
		atomic_set(&YushanIICtrl->check_intr0, 1);
		rc = POLLIN | POLLRDNORM;
	}
	if (atomic_read(&interrupt2)) {
		atomic_set(&interrupt2, 0);
		atomic_set(&YushanIICtrl->check_intr1, 1);
		rc = POLLIN | POLLRDNORM;
	}

	spin_unlock_irqrestore(&yushanII_int.yushan_spin_lock, flags);
	return rc;
}


int YushanII_got_INT0(void __user *argp){
	struct yushanii_stats_event_ctrl event;
	
	static int got_short = 0;	
    memset(&event,0,sizeof(struct yushanii_stats_event_ctrl));

	Ilp0100_interruptReadStatus(&pInterruptId, INTR_PIN_0);
	
	event.type = pInterruptId;

	Ilp0100_interruptClearStatus(pInterruptId, INTR_PIN_0);
	enable_irq(yushanII_intr0);
	switch(pInterruptId){
		case SPI_COMMS_READY:
			pr_info("[CAM]%s, SPI_COMMS_READY", __func__);
			break;
		case IDLE_COMPLETE:
			pr_info("[CAM]%s,  IDLE_COMPLETE", __func__);
			break;
		case ISP_STREAMING:
			pr_info("[CAM]%s, ISP_STREAMING", __func__);
			break;
		case MODE_CHANGE_COMPLETE:
			pr_info("[CAM]%s, MODE_CHANGE_COMPLETE", __func__);
			break;
		case START_OF_SHORTFRAME:
			if (g_sensor->func_tbl->sensor_yushanII_ae_updated) {
				if (g_sensor->func_tbl->sensor_yushanII_ae_updated() == 0) {
					got_short++;
					if (got_short >= 2) {
						if (g_sensor->func_tbl->sensor_yushanII_active_hold)
							g_sensor->func_tbl->sensor_yushanII_active_hold();
						got_short = 0;
					}
				}
			}
			break;
	}
	if(copy_to_user((void *)argp, &event, sizeof(struct yushanii_stats_event_ctrl))){
		pr_err("[CAM]%s, copy to user error\n", __func__);
		return -EFAULT;
	}
	return 0;
}

int YushanII_got_INT1(void){
	
	Ilp0100_interruptReadStatus(&pInterruptId2, INTR_PIN_1);
	pr_info("[CAM]pInterruptId2 : 0x%x",pInterruptId2);
	Ilp0100_interruptClearStatus(pInterruptId2, INTR_PIN_1);
	enable_irq(yushanII_intr1);
	return 0;
}

int YushanII_parse_interrupt(void __user *argp){

	int rc = 0;
	if (atomic_read(&YushanIICtrl->check_intr0)) {
		atomic_set(&YushanIICtrl->check_intr0, 0);
		rc = YushanII_got_INT0(argp);
	}
	if (atomic_read(&YushanIICtrl->check_intr1)) {
		atomic_set(&YushanIICtrl->check_intr1, 0);
		rc = YushanII_got_INT1();
	}

	return rc;
}

int YushanII_set_channel_offset_debug(void __user *argp){
	int channel_offset;

	if(copy_from_user(&channel_offset, argp,
			sizeof(int))){
			pr_err("[CAM] copy from user error\n");
			return -EFAULT;
	}

	YushanII_set_channel_offset(channel_offset);
	return 0;
}

int YushanII_set_channel_offset(int channel_offset){
	Ilp0100_structChannelOffsetConfig channel_offset_config;
	Ilp0100_structChannelOffsetParams channel_parm;

	pr_info("[CAM] %s, set channel offset:%d", __func__, channel_offset);
	channel_offset_config.Enable = 1;
	channel_parm.SensorPedestalBlue = channel_offset;
	channel_parm.SensorPedestalGreenBlue = channel_offset;
	channel_parm.SensorPedestalGreenRed = channel_offset;
	channel_parm.SensorPedestalRed = channel_offset;
	Ilp0100_configChannelOffsetShortOrNormal(channel_offset_config);
	Ilp0100_configChannelOffsetLong(channel_offset_config);
	Ilp0100_updateChannelOffsetShortOrNormal(channel_parm);
	Ilp0100_updateChannelOffsetLong(channel_parm);
	return 0;
}

int YushanII_set_defcor_debug(void __user *argp){
	int disable_defcor;

	if(copy_from_user(&disable_defcor, argp,
			sizeof(int))){
			pr_err("[CAM] copy from user error\n");
			return -EFAULT;
	}
	YushanII_set_defcor(disable_defcor);

	return 0;
}

int YushanII_set_defcor(int disable_defcor){
	Ilp0100_structDefcorConfig DefcorConfig;
	Ilp0100_structDefcorParams DefcorParams;
	pr_info("[CAM] %s, set disable defcor correction:%d", __func__, disable_defcor);

	DefcorParams.BlackStrength = 16;
	DefcorParams.CoupletThreshold = 150;
	DefcorParams.SingletThreshold = 20;
	DefcorParams.WhiteStrength = 16;

	if (disable_defcor == 1) {
		DefcorConfig.Mode = OFF;
		Ilp0100_configDefcorShortOrNormal(DefcorConfig);
		Ilp0100_configDefcorLong(DefcorConfig);
	} else {
		DefcorConfig.Mode = SINGLET_AND_COUPLET;
		Ilp0100_configDefcorShortOrNormal(DefcorConfig);
		Ilp0100_configDefcorLong(DefcorConfig);
		Ilp0100_updateDefcorShortOrNormal(DefcorParams);
		Ilp0100_updateDefcorLong(DefcorParams);
	}
	return 0;
}

static int YushanII_set_defcor_level(void __user *argp) {
	Ilp0100_structDefcorConfig DefcorConfig;
	Ilp0100_structDefcorParams DefcorParams;

	defcor_level_t defcor_level;
	bool_t defcorParamsUpdate = FALSE;

	if(copy_from_user(&defcor_level, argp,
		sizeof(defcor_level_t))) {
		pr_err("[CAM] copy from user error\n");
		return -EFAULT;
	}

	switch(defcor_level) {
	case DEFCOR_LEVEL_0:
		DefcorConfig.Mode = OFF;
		defcorParamsUpdate = FALSE;
		break;
	case DEFCOR_LEVEL_1:
		DefcorConfig.Mode = SINGLET_AND_COUPLET;
		DefcorParams.BlackStrength = 16;
		DefcorParams.CoupletThreshold = 150;
		DefcorParams.SingletThreshold = 20;
		DefcorParams.WhiteStrength = 16;
		defcorParamsUpdate = TRUE;
		break;
	case DEFCOR_LEVEL_2:
		DefcorConfig.Mode = SINGLET_AND_COUPLET;
		DefcorParams.BlackStrength = 16;
		DefcorParams.CoupletThreshold = 200;
		DefcorParams.SingletThreshold = 12;
		DefcorParams.WhiteStrength = 16;
		defcorParamsUpdate = TRUE;
		break;
	default:
		DefcorConfig.Mode = SINGLET_AND_COUPLET;
		DefcorParams.BlackStrength = 16;
		DefcorParams.CoupletThreshold = 150;
		DefcorParams.SingletThreshold = 20;
		DefcorParams.WhiteStrength = 16;
		defcorParamsUpdate = TRUE;
		break;
	}

	
	Ilp0100_configDefcorShortOrNormal(DefcorConfig);
	Ilp0100_configDefcorLong(DefcorConfig);

	if (defcorParamsUpdate) {
		Ilp0100_updateDefcorShortOrNormal(DefcorParams);
		Ilp0100_updateDefcorLong(DefcorParams);
	}

	return 0;
}

void YushanII_correct_StaturatedPixel(
	Ilp0100_structGlaceStatsData *glace_long,GlaceStatsData *glace)
{
	int i;
	for(i=0;i<48;i++){
		glace->GlaceStatsBlueMean[i] = glace_long->GlaceStatsBlueMean[i];
	}
	for(i=0;i<48;i++){
		glace->GlaceStatsGreenMean[i] = glace_long->GlaceStatsGreenMean[i];
	}
	for(i=0;i<48;i++){
		glace->GlaceStatsRedMean[i]= glace_long->GlaceStatsRedMean[i];
	}
	for(i=0;i<48;i++){
		glace->GlaceStatsNbOfSaturatedPixels[i]= glace_long->GlaceStatsNbOfSaturatedPixels[i]*2;
	}
	return;
}



int YushanII_read_stats(void __user *argp,unsigned int cmd){
	GlaceStatsData glace;
	memset(&glace,0,sizeof(GlaceStatsData));
	switch(cmd){
		case YUSHANII_GET_LOGN_GLACE:
			Ilp0100_readBackGlaceStatisticsLong(&glace_long);
			YushanII_correct_StaturatedPixel(&glace_long,&glace);
			if(copy_to_user((void *)argp, &glace, sizeof(glace))){
				pr_err("copy to user error\n");
				return -EFAULT;
			}
			break;
		case YUSHANII_GET_LOGN_HIST:
			Ilp0100_readBackHistStatisticsLong(&Hist_long);
			if(copy_to_user((void *)argp, &Hist_long, sizeof(Hist_long))){
				pr_err("copy to user error\n");
				return -EFAULT;
			}
			break;
		case YUSHANII_GET_SHORT_GLACE:
			Ilp0100_readBackGlaceStatisticsShortOrNormal(&glace_short);
			YushanII_correct_StaturatedPixel(&glace_short,&glace);
			if(copy_to_user((void *)argp, &glace, sizeof(glace))){
				pr_err("copy to user error\n");
				return -EFAULT;
			}
			break;
		case YUSHANII_GET_SHORT_HIST:
			Ilp0100_readBackHistStatisticsShortOrNormal(&Hist_short);
			if(copy_to_user((void *)argp, &Hist_short, sizeof(Hist_short))){
				pr_err("copy to user error\n");
				return -EFAULT;
			}
			break;
		}
	return 0;
}


int YushanII_set_tone_mapping_debug(void __user *argp){
	int tone_map;

	if(copy_from_user(&tone_map, argp,
		sizeof(int))){
		pr_err("[CAM] copy from user error\n");
		return -EFAULT;
	}
	YushanII_set_tone_mapping(tone_map);

	return 0;
}


int YushanII_set_tone_mapping(int tone_map){
	Ilp0100_structToneMappingParams tone_map_parm;
	Ilp0100_structToneMappingConfig tone_map_conf;

	pr_info("[CAM] %s, set tone map:%d", __func__, tone_map);
	tone_map_conf.Enable = TRUE;
	tone_map_conf.UserDefinedCurveEnable = FALSE;
	Ilp0100_configToneMapping(tone_map_conf);
	memcpy(tone_map_parm.UserDefinedCurve,
		tone_map_user1,
		sizeof(tone_map_parm.UserDefinedCurve));
		tone_map_parm.Strength = tone_map;
	Ilp0100_updateToneMapping(tone_map_parm);

	return 0;
}

int YushanII_set_cls_debug(void __user *argp){
	struct yushanii_cls cls;

	if(copy_from_user(&cls, argp,
		sizeof(struct yushanii_cls))){
		pr_err("[CAM] copy from user error\n");
		return -EFAULT;
	}
	YushanII_set_cls(&cls);

	return 0;
}

int YushanII_set_cls(struct yushanii_cls *cls){
	Ilp0100_structClsConfig Cls_config;
	Ilp0100_structClsParams parm;

	pr_info("[CAM] %s cls_enable:%d", __func__, cls->cls_enable);
        pr_info("[CAM] %s color_temp:%d", __func__, cls->color_temp);
	Cls_config.Enable = (bool_t)cls->cls_enable;
	parm.BowlerCornerGain = 100; 
	parm.ColorTempKelvin = cls->color_temp;
	Ilp0100_configCls(Cls_config);
	Ilp0100_updateCls(parm);
	return 0;
}

int YushanII_set_exp(void __user *argp){
	struct yushanii_exposure exp;
	Ilp0100_structFrameParams long_exp,short_exp;
	if(copy_from_user(&exp, argp,
		sizeof(struct yushanii_exposure))){
		pr_err("[CAM] copy from user error\n");
		return -EFAULT;
	}
	
	long_exp.AnalogGainCodeBlue = exp.AnalogGain_B;
	long_exp.AnalogGainCodeGreen = exp.AnalogGain_G;
	long_exp.AnalogGainCodeRed = exp.AnalogGain_R;
	long_exp.ExposureTime = exp.long_exposure;
	long_exp.DigitalGainCodeBlue = exp.DigitalLongGain_B;
	long_exp.DigitalGainCodeGreen = exp.DigitalLongGain_G;
	long_exp.DigitalGainCodeRed = exp.DigitalLongGain_R;

	Ilp0100_updateSensorParamsLong(long_exp);

	
	short_exp.AnalogGainCodeBlue = exp.AnalogGain_B;
	short_exp.AnalogGainCodeGreen = exp.AnalogGain_G;
	short_exp.AnalogGainCodeRed = exp.AnalogGain_R;
	short_exp.ExposureTime = exp.short_exposure;
	short_exp.DigitalGainCodeBlue = exp.DigitalShortGain_B;
	short_exp.DigitalGainCodeGreen = exp.DigitalShortGain_G;
	short_exp.DigitalGainCodeRed = exp.DigitalShortGain_R;

	Ilp0100_updateSensorParamsShortOrNormal(short_exp);
	return 0;
}

void YushanII_set_hdr_exp(uint16_t gain, uint16_t dig_gain, uint32_t long_line, uint32_t short_line){
	Ilp0100_structFrameParams long_exp,short_exp;

	
	long_exp.AnalogGainCodeBlue = gain;
	long_exp.AnalogGainCodeGreen = gain;
	long_exp.AnalogGainCodeRed = gain;
	long_exp.ExposureTime = long_line;
	long_exp.DigitalGainCodeBlue = dig_gain;
	long_exp.DigitalGainCodeGreen = dig_gain;
	long_exp.DigitalGainCodeRed = dig_gain;

	Ilp0100_updateSensorParamsLong(long_exp);

	
	short_exp.AnalogGainCodeBlue = gain;
	short_exp.AnalogGainCodeGreen = gain;
	short_exp.AnalogGainCodeRed = gain;
	short_exp.ExposureTime = short_line;
	short_exp.DigitalGainCodeBlue = dig_gain;
	short_exp.DigitalGainCodeGreen = dig_gain;
	short_exp.DigitalGainCodeRed = dig_gain;

	Ilp0100_updateSensorParamsShortOrNormal(short_exp);

}

int YushanII_set_hdr_merge_debug(void __user *argp){
	struct yushanii_hdr_merge usr_hdr_merge;

	if(copy_from_user(&usr_hdr_merge, argp,
		sizeof(struct yushanii_hdr_merge))){
		pr_err("[CAM] copy from user error\n");
		return -EFAULT;
	}

	YushanII_set_hdr_merge(usr_hdr_merge);
	return 0;
}

int YushanII_set_hdr_merge(struct yushanii_hdr_merge hdr_merge){
	Ilp0100_structHdrMergeParams merge_parm;

	pr_info("[CAM] %s, set hdr merge core:%d, method:%d", __func__, hdr_merge.code, hdr_merge.method);

	merge_parm.ImageCodes = hdr_merge.code;
	merge_parm.Method = hdr_merge.method;
	Ilp0100_updateHdrMerge(merge_parm);
	return 0;
}

int YushanII_set_hdr_merge_mode(void __user *argp){
	struct yushanii_hdr_merge_mode usr_hdr_merge_mode;
	Ilp0100_structHdrMergeConfig merge_config;

	if(copy_from_user(&usr_hdr_merge_mode, argp,
		sizeof(struct yushanii_hdr_merge_mode))){
		pr_err("[CAM] copy from user error\n");
		return -EFAULT;
	}

	pr_info("[CAM] %s, set hdr merge mode:%d", __func__, usr_hdr_merge_mode.Mode);

	merge_config.Mode= usr_hdr_merge_mode.Mode;
	Ilp0100_configHdrMerge(merge_config);
	return 0;
}

void YushanII_set_tm_MergeGain(void){
	uint32_t hist_Y1 = 0x0;
	uint32_t hist_Y0 = 0x0;
	uint32_t line_Y0 = 0x0;
	uint32_t line_Y1 = 0x0;
	uint8_t check[4];
	memset(check, 0x0, sizeof(check));
	
	hist_Y1 = 0x467ffc00;
	hist_Y0 = 0x45c00000;
	
	line_Y0 = 0x467ffc00;
	line_Y1 = 0x45c00000;
	Ilp0100_writeRegister(0x0f5c, (uint8_t *) &hist_Y1);
	Ilp0100_writeRegister(0x0f58, (uint8_t *) &hist_Y0);

	Ilp0100_writeRegister(0x0f70, (uint8_t *) &line_Y0);
	Ilp0100_writeRegister(0x0f74, (uint8_t *) &line_Y1);
}

void YushanII_set_tm_LongOnlyGain(void){
	uint32_t hist_Y1 = 0x0;
	uint32_t hist_Y0 = 0x0;
	uint32_t line_Y0 = 0x0;
	uint32_t line_Y1 = 0x0;
	uint8_t check[4];
	memset(check, 0x0, sizeof(check));
	
	hist_Y1 = 0x45c00000;
	hist_Y0 = 0x45800000;
	
	line_Y0 = 0x45c00000;
	line_Y1 = 0x45800000;
	Ilp0100_writeRegister(0x0f5c, (uint8_t *) &hist_Y1);
	Ilp0100_writeRegister(0x0f58, (uint8_t *) &hist_Y0);

	Ilp0100_writeRegister(0x0f70, (uint8_t *) &line_Y0);
	Ilp0100_writeRegister(0x0f74, (uint8_t *) &line_Y1);

}


int YushanII_set_hdr_factor(void __user *argp){
       uint8_t usr_HDRFactor;

       if(copy_from_user(&usr_HDRFactor, argp,
               sizeof(uint8_t))){
               pr_err("[CAM] copy from user error\n");
               return -EFAULT;
       }

		if ((usr_HDRFactor < 0) || (usr_HDRFactor > 100)) {
			if (usr_HDRFactor < 0)
				usr_HDRFactor = 0;

			if (usr_HDRFactor > 100)
				usr_HDRFactor = 100;

			pr_info("[CAM] %s, set hdr factor: HDRFactor error and overbound", __func__);
		}
	pr_info("%s usr_HDRFactor:%d",__func__,usr_HDRFactor);

	if(usr_HDRFactor > 1)
		YushanII_set_tm_MergeGain();
	else 
		YushanII_set_tm_LongOnlyGain();

       Ilp0100_setHDRFactor(usr_HDRFactor);
       return 0;
}

void YushanII_config_glace(struct yushanii_glace_config glace_config){
	Ilp0100_structGlaceConfig long_glace_config, short_glace_config;

	
	long_glace_config.Enable                        = glace_config.long_glace_config.Enable;
	long_glace_config.RoiHStart                     = glace_config.long_glace_config.RoiHStart;
	long_glace_config.RoiVStart                     = glace_config.long_glace_config.RoiVStart;
	long_glace_config.RoiHBlockSize                 = glace_config.long_glace_config.RoiHBlockSize;
	long_glace_config.RoiVBlockSize                 = glace_config.long_glace_config.RoiVBlockSize;
	long_glace_config.RoiHNumberOfBlocks            = glace_config.long_glace_config.RoiHNumberOfBlocks;
	long_glace_config.RoiVNumberOfBlocks            = glace_config.long_glace_config.RoiVNumberOfBlocks;
	long_glace_config.SaturationLevelRed            = glace_config.long_glace_config.SaturationLevelRed;
	long_glace_config.SaturationLevelGreen          = glace_config.long_glace_config.SaturationLevelGreen;
	long_glace_config.SaturationLevelBlue           = glace_config.long_glace_config.SaturationLevelBlue;

	Ilp0100_configGlaceLong(long_glace_config);

	
	short_glace_config.Enable                        = glace_config.short_glace_config.Enable;
	short_glace_config.RoiHStart                     = glace_config.short_glace_config.RoiHStart;
	short_glace_config.RoiVStart                     = glace_config.short_glace_config.RoiVStart;
	short_glace_config.RoiHBlockSize                 = glace_config.short_glace_config.RoiHBlockSize;
	short_glace_config.RoiVBlockSize                 = glace_config.short_glace_config.RoiVBlockSize;
	short_glace_config.RoiHNumberOfBlocks            = glace_config.short_glace_config.RoiHNumberOfBlocks;
	short_glace_config.RoiVNumberOfBlocks            = glace_config.short_glace_config.RoiVNumberOfBlocks;
	short_glace_config.SaturationLevelRed            = glace_config.short_glace_config.SaturationLevelRed;
	short_glace_config.SaturationLevelGreen          = glace_config.short_glace_config.SaturationLevelGreen;
	short_glace_config.SaturationLevelBlue           = glace_config.short_glace_config.SaturationLevelBlue;

	Ilp0100_configGlaceShortOrNormal(short_glace_config);
}

int YushanII_set_glace_config(void __user *argp){
	struct yushanii_glace_config usr_glace_config;

	if(copy_from_user(&usr_glace_config, argp,
		sizeof(struct yushanii_glace_config))){
		pr_err("[CAM] copy from user error\n");
		return -EFAULT;
	}

	YushanII_config_glace(usr_glace_config);
	return 0;
}

static long YushanII_fops_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	int rc = 0;
	struct YushanII_ctrl *raw_dev = filp->private_data;
	void __user *argp = (void __user *)arg;

	
	mutex_lock(&raw_dev->raw_ioctl_lock);
	switch (cmd) {
	case YUSHANII_GET_INT:
		rc = YushanII_parse_interrupt(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_GET_LOGN_HIST:
	case YUSHANII_GET_SHORT_HIST:
	case YUSHANII_GET_LOGN_GLACE:
	case YUSHANII_GET_SHORT_GLACE:
		rc = YushanII_read_stats(argp,cmd);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_CHANNEL_OFFSET:
		rc = YushanII_set_channel_offset_debug(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_TONE_MAPPING:
		rc = YushanII_set_tone_mapping_debug(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_DISABLE_DEFCOR:
		rc = YushanII_set_defcor_debug(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_CLS:
		rc = YushanII_set_cls_debug(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_EXP:
		rc = YushanII_set_exp(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_HDR_MERGE:
		rc = YushanII_set_hdr_merge_debug(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_GLACE_CONFIG:
		rc = YushanII_set_glace_config(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_HDR_MERGE_MODE:
		rc = YushanII_set_hdr_merge_mode(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_HDR_FACTOR:
		rc = YushanII_set_hdr_factor(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	case YUSHANII_SET_DEFCOR_LEVEL:
		rc = YushanII_set_defcor_level(argp);
		if(rc < 0)
			pr_info("%s:%d cmd = %d, rc =%d\n", __func__, __LINE__, _IOC_NR(cmd), rc);
		break;
	}
	mutex_unlock(&raw_dev->raw_ioctl_lock);
	return rc;
}

static  const struct  file_operations yushanII_fops = {
	.owner	  = THIS_MODULE,
	.open	   = YushanII_fops_open,
	.unlocked_ioctl = YushanII_fops_ioctl,
	.poll  = YushanII_fops_poll,
};

static int setup_YushanII_cdev(void)
{
	int rc = 0;
	struct device *dev;

	pr_info("%s\n", __func__);

	rc = alloc_chrdev_region(&yushanII_devno, 0, 1, MSM_YUSHANII_NAME);
	if (rc < 0) {
		pr_err("%s: failed to allocate chrdev\n", __func__);
		goto alloc_chrdev_region_failed;
	}

	if (!yushanII_class) {
		yushanII_class = class_create(THIS_MODULE, MSM_YUSHANII_NAME);
		if (IS_ERR(yushanII_class)) {
			rc = PTR_ERR(yushanII_class);
			pr_err("%s: create device class failed\n",
				__func__);
			goto class_create_failed;
		}
	}

	dev = device_create(yushanII_class, NULL,
		MKDEV(MAJOR(yushanII_devno), MINOR(yushanII_devno)), NULL,
		"%s%d", MSM_YUSHANII_NAME, 0);
	if (IS_ERR(dev)) {
		pr_err("%s: error creating device\n", __func__);
		rc = -ENODEV;
		goto device_create_failed;
	}

	cdev_init(&YushanIICtrl->cdev, &yushanII_fops);
	YushanIICtrl->cdev.owner = THIS_MODULE;
	YushanIICtrl->cdev.ops   =
		(const struct file_operations *) &yushanII_fops;
	rc = cdev_add(&YushanIICtrl->cdev, yushanII_devno, 1);
	if (rc < 0) {
		pr_err("%s: error adding cdev\n", __func__);
		rc = -ENODEV;
		goto cdev_add_failed;
	}

	return rc;

cdev_add_failed:
	device_destroy(yushanII_class, yushanII_devno);
device_create_failed:
	class_destroy(yushanII_class);
class_create_failed:
	unregister_chrdev_region(yushanII_devno, 1);
alloc_chrdev_region_failed:
	return rc;
}

static void YushanII_tear_down_cdev(void)
{
	cdev_del(&YushanIICtrl->cdev);
	device_destroy(yushanII_class, yushanII_devno);
	class_destroy(yushanII_class);
	unregister_chrdev_region(yushanII_devno, 1);
}


#ifdef CONFIG_RAWCHIPII
static struct kobject *YushanII_status_obj;

uint32_t YushanII_id;

static ssize_t probed_YushanII_id_get(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t length;
	length = sprintf(buf, "0x%x\n", YushanII_id);
	return length;
}

static DEVICE_ATTR(probed_YushanII_id, 0444,
	probed_YushanII_id_get,
	NULL);

int msm_YushanII_attr_node(void)
{
	int ret = 0;

	YushanII_status_obj = kobject_create_and_add("camera_YushanII_status", NULL);
	if (YushanII_status_obj == NULL) {
		pr_err("msm_camera: create camera_YushanII_status failed\n");
		ret = -ENOMEM;
		goto error;
	}

	ret = sysfs_create_file(YushanII_status_obj,
		&dev_attr_probed_YushanII_id.attr);
	if (ret) {
		pr_info("[CAM]%s, msm_camera: sysfs_create_file dev_attr_probed_YushanII_id failed\n", __func__);
		ret = -EFAULT;
		goto error;
	}

	return ret;

error:
	kobject_del(YushanII_status_obj);
	return ret;
}
#endif


static int YushanII_driver_probe(struct platform_device *pdev)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	YushanIICtrl = kzalloc(sizeof(struct YushanII_ctrl), GFP_ATOMIC);
	if (!YushanIICtrl) {
		pr_err("%s: could not allocate mem for yushanII_dev\n", __func__);
		return -ENOMEM;
	}

	rc = setup_YushanII_cdev();
	if (rc < 0) {
		kfree(YushanIICtrl);
		return rc;
	}

	mutex_init(&YushanIICtrl->raw_ioctl_lock);
	YushanIICtrl->pdata = pdev->dev.platform_data;

	rc = rawchip_spi_init();
	if (rc < 0) {
		pr_err("%s: failed to register spi driver\n", __func__);
		return rc;
	}

	msm_YushanII_attr_node();

	return rc;
}

static int YushanII_driver_remove(struct platform_device *pdev)
{
	YushanII_tear_down_cdev();

	mutex_destroy(&YushanIICtrl->raw_ioctl_lock);

	kfree(YushanIICtrl);

	return 0;
}

static struct  platform_driver YushanII_driver = {
	.probe  = YushanII_driver_probe,
	.remove =YushanII_driver_remove,
	.driver = {
		.name = "yushanII",
		.owner = THIS_MODULE,
	},
};

static int __init YushanII_driver_init(void)
{
	int rc;
	rc = platform_driver_register(&YushanII_driver);
	return rc;
}

static void __exit YushanII_driver_exit(void)
{
	platform_driver_unregister(&YushanII_driver);
}

MODULE_DESCRIPTION("YushanII driver");
MODULE_VERSION("YushanII 0.1");

module_init(YushanII_driver_init);
module_exit(YushanII_driver_exit);

