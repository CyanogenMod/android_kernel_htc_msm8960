#include "yushan_registermap.h"
#include "DxODOP_regMap.h"
#include "DxODPP_regMap.h"
#include "DxOPDP_regMap.h"
#include "Yushan_HTC_Functions.h"

#include <mach/board.h>

#ifdef YUSHAN_HTC_FUNCTIONS_DEBUG
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

Yushan_ImageChar_t	sImageChar_context;
struct yushan_reg_t *p_yushan_regs;

#define PDP_enable 0x01
#define black_level_enable 0x01
#define dead_pixel_enable 0x01
#define DOP_enable 0x01
#define denoise_enable 0x01
#define DPP_enable 0x01

uint8_t bPdpMode = PDP_enable ? (0x01|(((~black_level_enable)&0x01)<<3)|(((~dead_pixel_enable)&0x01)<<4)) : 0;
uint8_t bDppMode = DPP_enable ? 0x03 : 0;
uint8_t bDopMode = DOP_enable ? (0x01|(((~denoise_enable)&0x01)<<4)) : 0;

void YushanPrintDxODOPAfStrategy(void)
{
	uint8_t val ;
	SPI_Read((DXO_DOP_BASE_ADDR+0x02a4) , 1, (uint8_t *)(&val));
	pr_info("[CAM] prcDxO YushanReadDxODOPAfStrategy:0x%02x", val);
}

void YushanPrintFrameNumber(void)
{
	uint16_t val ;
	SPI_Read((0x82a2) , 2, (uint8_t *)(&val));
	pr_info("[CAM] prcDxO YushanReadFrameNumber:%d", val);
}

void YushanPrintVisibleLineSizeAndRoi(void)
{
	uint16_t val ;
	uint8_t  xRoiStart;
	uint8_t  yRoiStart;
	uint8_t  xRoiEnd;
	uint8_t  yRoiEnd;

	SPI_Read((DxOPDP_visible_line_size_7_0 + 0x8000) , 2, (uint8_t *)(&val));
	pr_info("[CAM] prcDxO YushanPrintVisibleLineSizeAndRoi:%d", val);

	SPI_Read((DxODOP_ROI_0_x_start_7_0 + DXO_DOP_BASE_ADDR) , 1, (uint8_t *)(&xRoiStart));
	SPI_Read((DxODOP_ROI_0_y_start_7_0 + DXO_DOP_BASE_ADDR) , 1, (uint8_t *)(&yRoiStart));
	SPI_Read((DxODOP_ROI_0_x_end_7_0 + DXO_DOP_BASE_ADDR) , 1, (uint8_t *)(&xRoiEnd));
	SPI_Read((DxODOP_ROI_0_y_end_7_0 + DXO_DOP_BASE_ADDR) , 1, (uint8_t *)(&yRoiEnd));
	pr_info("[CAM] prcDxO ROI [ %d %d | %d %d ]\n",xRoiStart,xRoiEnd,yRoiStart,yRoiEnd);
}

void YushanPrintImageInformation(void)
{
	static uint16_t xStart_prev = -1;
	static uint16_t xEnd_prev = -1;
	static uint16_t yStart_prev = -1;
	static uint16_t yEnd_prev = -1;
	static uint16_t xOddInc_prev = -1;
	static uint16_t yOddInc_prev = -1;
	uint16_t xStart_new ;
	uint16_t xEnd_new ;
	uint16_t yStart_new ;
	uint16_t yEnd_new ;
	uint16_t xOddInc_new ;
	uint16_t yOddInc_new ;


	SPI_Read((DxOPDP_x_addr_start_7_0 + 0x8000) , 2, (uint8_t *)(&xStart_new));
	SPI_Read((DxOPDP_y_addr_start_7_0 + 0x8000) , 2, (uint8_t *)(&yStart_new));
	SPI_Read((DxOPDP_x_addr_end_7_0 + 0x8000) , 2, (uint8_t *)(&xEnd_new));
	SPI_Read((DxOPDP_y_addr_end_7_0 + 0x8000) , 2, (uint8_t *)(&yEnd_new));
	SPI_Read((DxOPDP_x_odd_inc_7_0 + 0x8000) , 2, (uint8_t *)(&xOddInc_new));
	SPI_Read((DxOPDP_y_odd_inc_7_0 + 0x8000) , 2, (uint8_t *)(&yOddInc_new));

	if ((xStart_prev!=xStart_new)
		||	(yStart_prev!=yStart_new)
		||	(xEnd_prev!=xEnd_new)
		||	(yEnd_prev!=yEnd_new)
		||	(xOddInc_prev!=xOddInc_new)
		||	(yOddInc_prev!=yOddInc_new) ) {
			pr_err("[CAM] prcDxO DxOPDP_x_addr_start: %d", xStart_new);
			pr_err("[CAM] prcDxO DxOPDP_x_addr_end: %d", xEnd_new);
			pr_err("[CAM] prcDxO DxOPDP_y_addr_start: %d", yStart_new);
			pr_err("[CAM] prcDxO DxOPDP_y_addr_end: %d", yEnd_new);
			pr_err("[CAM] prcDxO DxOPDP_x_odd_inc: %d", xOddInc_new);
			pr_err("[CAM] prcDxO DxOPDP_y_odd_inc: %d", yOddInc_new);
		}
	xStart_prev = xStart_new;
	xEnd_prev = xEnd_new ;
	yStart_prev = yStart_new ;
	yEnd_prev = yEnd_new ;
	xOddInc_prev = xOddInc_new ;
	yOddInc_prev = yOddInc_new ;


}

void Reset_Yushan(void)
{
	uint8_t	bSpiData;
	Yushan_Init_Dxo_Struct_t	sDxoStruct;
	sDxoStruct.pDxoPdpRamImage[0] = (uint8_t *)p_yushan_regs->pdpcode;
	sDxoStruct.pDxoDppRamImage[0] = (uint8_t *)p_yushan_regs->dppcode;
	sDxoStruct.pDxoDopRamImage[0] = (uint8_t *)p_yushan_regs->dopcode;
	sDxoStruct.pDxoPdpRamImage[1] = (uint8_t *)p_yushan_regs->pdpclib;
	sDxoStruct.pDxoDppRamImage[1] = (uint8_t *)p_yushan_regs->dppclib;
	sDxoStruct.pDxoDopRamImage[1] = (uint8_t *)p_yushan_regs->dopclib;

	sDxoStruct.uwDxoPdpRamImageSize[0] = p_yushan_regs->pdpcode_size;
	sDxoStruct.uwDxoDppRamImageSize[0] = p_yushan_regs->dppcode_size;
	sDxoStruct.uwDxoDopRamImageSize[0] = p_yushan_regs->dopcode_size;
	sDxoStruct.uwDxoPdpRamImageSize[1] = p_yushan_regs->pdpclib_size;
	sDxoStruct.uwDxoDppRamImageSize[1] = p_yushan_regs->dppclib_size;
	sDxoStruct.uwDxoDopRamImageSize[1] = p_yushan_regs->dopclib_size;

	sDxoStruct.uwBaseAddrPdpMicroCode[0] = p_yushan_regs->pdpcode_first_addr;
	sDxoStruct.uwBaseAddrDppMicroCode[0] = p_yushan_regs->dppcode_first_addr;
	sDxoStruct.uwBaseAddrDopMicroCode[0] = p_yushan_regs->dopcode_first_addr;
	sDxoStruct.uwBaseAddrPdpMicroCode[1] = p_yushan_regs->pdpclib_first_addr;
	sDxoStruct.uwBaseAddrDppMicroCode[1] = p_yushan_regs->dppclib_first_addr;
	sDxoStruct.uwBaseAddrDopMicroCode[1] = p_yushan_regs->dopclib_first_addr;

	sDxoStruct.uwDxoPdpBootAddr = p_yushan_regs->pdpBootAddr;
	sDxoStruct.uwDxoDppBootAddr = p_yushan_regs->dppBootAddr;
	sDxoStruct.uwDxoDopBootAddr = p_yushan_regs->dopBootAddr;

	sDxoStruct.uwDxoPdpStartAddr = p_yushan_regs->pdpStartAddr;
	sDxoStruct.uwDxoDppStartAddr = p_yushan_regs->dppStartAddr;
	sDxoStruct.uwDxoDopStartAddr = p_yushan_regs->dopStartAddr;

	pr_err("[CAM] %s\n",__func__);
	
	Yushan_Assert_Reset(0x001F0F10, RESET_MODULE);
	bSpiData =1;
	Yushan_DXO_Sync_Reset_Dereset(bSpiData);
	Yushan_Assert_Reset(0x001F0F10, DERESET_MODULE);
	bSpiData = 0;
	Yushan_DXO_Sync_Reset_Dereset(bSpiData);
	Yushan_Init_Dxo(&sDxoStruct, 1);
	msleep(10);
	
	
	
}

void ASIC_Test(void)
{
	pr_info("[CAM] ASIC_Test E\n");
	mdelay(10);
	
	mdelay(10);
	rawchip_spi_write_2B1B(0x000c, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x000d, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x000c, 0x3f); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x000d, 0x07); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x000f, 0x00); 
	mdelay(10);

	
	rawchip_spi_write_2B1B(0x1405, 0x03);
	mdelay(10);
	rawchip_spi_write_2B1B(0x1405, 0x02);
	mdelay(10);
	rawchip_spi_write_2B1B(0x1405, 0x00);

	
	
	rawchip_spi_write_2B1B(0x0015, 0x19); 
	rawchip_spi_write_2B1B(0x0014, 0x03); 

	mdelay(10);
	rawchip_spi_write_2B1B(0x0000, 0x0a); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x0001, 0x0a); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x0002, 0x14); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x0010, 0x18); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x0009, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x1000, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2000, 0xff); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2004, 0x06); 
	mdelay(10);
	
	mdelay(10);
	
	rawchip_spi_write_2B1B(0x5004, 0x14); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2408, 0x04); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x240c, 0x0a); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2420, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2428, 0x2b); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4400, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4404, 0x0a); 
	mdelay(10);

	
	rawchip_spi_write_2B1B(0x4a05, 0x04); 

	
	rawchip_spi_write_2B1B(0x2c09, 0x10); 

	mdelay(10);
	rawchip_spi_write_2B1B(0x2c0c, 0xd0); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c0d, 0x0c); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c0e, 0xa0); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c0f, 0x0f); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c10, 0xa0); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c11, 0x09); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c12, 0xf0); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c13, 0xff); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x3400, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x3401, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x3402, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x3403, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x3408, 0x02); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x3409, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x340a, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x340b, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5880, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5888, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4400, 0x11); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4408, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x440c, 0x03); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c00, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c08, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c10, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c4c, 0x14); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c4d, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c50, 0x2b); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c51, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c5c, 0x2b); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c5d, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c58, 0x04); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x4c59, 0x10); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5828, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x582c, 0x02); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5830, 0x0d); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5834, 0x03); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5820, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5868, 0xff); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x586c, 0xff); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5870, 0xff); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5874, 0xff); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5860, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c08, 0x94); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c09, 0x02); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c0c, 0xfc); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c10, 0x90); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c11, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c14, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x5c00, 0x01); 
	mdelay(10);

	
	rawchip_spi_write_2B1B(0x5000, 0x33); 
	mdelay(100);

	rawchip_spi_write_2B1B(0x2c00, 0x01); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c01, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c02, 0x00); 
	mdelay(10);
	rawchip_spi_write_2B1B(0x2c03, 0x00); 
	msleep(2000);

	pr_info("[CAM] ASIC_Test X\n");
}

#define DxO
int Yushan_sensor_open_init(struct rawchip_sensor_init_data data)
{
	
#ifdef COLOR_BAR
	 int32_t rc = 0;
	ASIC_Test();
	
#else
	
	

	bool_t	bBypassDxoUpload = 0;
	Yushan_Version_Info_t sYushanVersionInfo;

	uint8_t			bPixelFormat = RAW10;
	
	
	Yushan_Init_Struct_t	sInitStruct;
	Yushan_Init_Dxo_Struct_t	sDxoStruct;
	Yushan_GainsExpTime_t sGainsExpTime;
	Yushan_DXO_DPP_Tuning_t sDxoDppTuning;
	Yushan_DXO_PDP_Tuning_t sDxoPdpTuning;
	Yushan_DXO_DOP_Tuning_t sDxoDopTuning;
	Yushan_SystemStatus_t			sSystemStatus;
	uint32_t		udwIntrMask[] = {0x3DE38E3B, 0xFC3C3C3C, 0x001B7FFB};	
	uint16_t		uwAssignITRGrpToPad1 = 0x008; 

#if 0
	Yushan_AF_ROI_t					sYushanAfRoi[5];
	Yushan_DXO_ROI_Active_Number_t	sYushanDxoRoiActiveNumber;
	Yushan_AF_Stats_t				sYushanAFStats[5];
	Yushan_New_Context_Config_t		sYushanNewContextConfig;
#endif
	
	uint8_t bStatus;
	

	uint16_t uwHSize = data.width;
	uint16_t uwVSize = data.height;
	uint16_t uwBlkPixels = data.blk_pixels;
	uint16_t uwBlkLines = data.blk_lines;

 #endif

	CDBG("[CAM] Yushan API Version : %d.%d \n", API_MAJOR_VERSION, API_MINOR_VERSION);

#ifndef COLOR_BAR
	
	sInitStruct.bNumberOfLanes		=	data.lane_cnt;
	sInitStruct.fpExternalClock		=	data.ext_clk;
	sInitStruct.uwBitRate			=	data.bitrate;
	sInitStruct.uwPixelFormat = 0x0A0A;
	
	sInitStruct.bDxoSettingCmdPerFrame	=	1;

	if ((sInitStruct.uwPixelFormat&0x0F) == 0x0A)
		bPixelFormat = RAW10;
	else if ((sInitStruct.uwPixelFormat&0x0F) == 0x08) {
		if (((sInitStruct.uwPixelFormat>>8)&0x0F) == 0x08)
			bPixelFormat = RAW8;
		else 
			bPixelFormat = RAW10_8;
	}
#endif

#ifndef COLOR_BAR

	p_yushan_regs = &yushan_regs;
	if (1) {
		pr_info("[CAM] Load R2 u_code data\n");
		p_yushan_regs->pdpcode_first_addr = yushan_u_code_r2.pdpcode_first_addr;
		p_yushan_regs->pdpcode = yushan_u_code_r2.pdpcode;
		p_yushan_regs->pdpcode_size = yushan_u_code_r2.pdpcode_size;
		p_yushan_regs->pdpBootAddr = yushan_u_code_r2.pdpBootAddr;
		p_yushan_regs->pdpStartAddr = yushan_u_code_r2.pdpStartAddr;

		p_yushan_regs->dppcode_first_addr = yushan_u_code_r2.dppcode_first_addr;
		p_yushan_regs->dppcode = yushan_u_code_r2.dppcode;
		p_yushan_regs->dppcode_size = yushan_u_code_r2.dppcode_size;
		p_yushan_regs->dppBootAddr = yushan_u_code_r2.dppBootAddr;
		p_yushan_regs->dppStartAddr = yushan_u_code_r2.dppStartAddr;

		p_yushan_regs->dopcode_first_addr = yushan_u_code_r2.dopcode_first_addr;
		p_yushan_regs->dopcode = yushan_u_code_r2.dopcode;
		p_yushan_regs->dopcode_size = yushan_u_code_r2.dopcode_size;
		p_yushan_regs->dopBootAddr = yushan_u_code_r2.dopBootAddr;
		p_yushan_regs->dopStartAddr = yushan_u_code_r2.dopStartAddr;
	} else {
		pr_info("[CAM] Load R3 u_code data\n");
		p_yushan_regs->pdpcode_first_addr = yushan_u_code_r3.pdpcode_first_addr;
		p_yushan_regs->pdpcode = yushan_u_code_r3.pdpcode;
		p_yushan_regs->pdpcode_size = yushan_u_code_r3.pdpcode_size;
		p_yushan_regs->pdpBootAddr = yushan_u_code_r3.pdpBootAddr;
		p_yushan_regs->pdpStartAddr = yushan_u_code_r3.pdpStartAddr;

		p_yushan_regs->dppcode_first_addr = yushan_u_code_r3.dppcode_first_addr;
		p_yushan_regs->dppcode = yushan_u_code_r3.dppcode;
		p_yushan_regs->dppcode_size = yushan_u_code_r3.dppcode_size;
		p_yushan_regs->dppBootAddr = yushan_u_code_r3.dppBootAddr;
		p_yushan_regs->dppStartAddr = yushan_u_code_r3.dppStartAddr;

		p_yushan_regs->dopcode_first_addr = yushan_u_code_r3.dopcode_first_addr;
		p_yushan_regs->dopcode = yushan_u_code_r3.dopcode;
		p_yushan_regs->dopcode_size = yushan_u_code_r3.dopcode_size;
		p_yushan_regs->dopBootAddr = yushan_u_code_r3.dopBootAddr;
		p_yushan_regs->dopStartAddr = yushan_u_code_r3.dopStartAddr;
	}
	
	if (strcmp(data.sensor_name, "s5k3h2yx") == 0) {
		p_yushan_regs->pdpclib = yushan_regs_clib_s5k3h2yx.pdpclib;
		p_yushan_regs->dppclib = yushan_regs_clib_s5k3h2yx.dppclib;
		p_yushan_regs->dopclib = yushan_regs_clib_s5k3h2yx.dopclib;
		p_yushan_regs->pdpclib_size = yushan_regs_clib_s5k3h2yx.pdpclib_size;
		p_yushan_regs->dppclib_size = yushan_regs_clib_s5k3h2yx.dppclib_size;
		p_yushan_regs->dopclib_size = yushan_regs_clib_s5k3h2yx.dopclib_size;
		p_yushan_regs->pdpclib_first_addr = yushan_regs_clib_s5k3h2yx.pdpclib_first_addr;
		p_yushan_regs->dppclib_first_addr = yushan_regs_clib_s5k3h2yx.dppclib_first_addr;
		p_yushan_regs->dopclib_first_addr = yushan_regs_clib_s5k3h2yx.dopclib_first_addr;
	}
	else if (strcmp(data.sensor_name, "imx175") == 0) {
		p_yushan_regs->pdpclib = yushan_regs_clib_imx175.pdpclib;
		p_yushan_regs->dppclib = yushan_regs_clib_imx175.dppclib;
		p_yushan_regs->dopclib = yushan_regs_clib_imx175.dopclib;
		p_yushan_regs->pdpclib_size = yushan_regs_clib_imx175.pdpclib_size;
		p_yushan_regs->dppclib_size = yushan_regs_clib_imx175.dppclib_size;
		p_yushan_regs->dopclib_size = yushan_regs_clib_imx175.dopclib_size;
		p_yushan_regs->pdpclib_first_addr = yushan_regs_clib_imx175.pdpclib_first_addr;
		p_yushan_regs->dppclib_first_addr = yushan_regs_clib_imx175.dppclib_first_addr;
		p_yushan_regs->dopclib_first_addr = yushan_regs_clib_imx175.dopclib_first_addr;
	}
	else if (strcmp(data.sensor_name, "ov8838") == 0) {
		p_yushan_regs->pdpclib = yushan_regs_clib_ov8838.pdpclib;
		p_yushan_regs->dppclib = yushan_regs_clib_ov8838.dppclib;
		p_yushan_regs->dopclib = yushan_regs_clib_ov8838.dopclib;
		p_yushan_regs->pdpclib_size = yushan_regs_clib_ov8838.pdpclib_size;
		p_yushan_regs->dppclib_size = yushan_regs_clib_ov8838.dppclib_size;
		p_yushan_regs->dopclib_size = yushan_regs_clib_ov8838.dopclib_size;
		p_yushan_regs->pdpclib_first_addr = yushan_regs_clib_ov8838.pdpclib_first_addr;
		p_yushan_regs->dppclib_first_addr = yushan_regs_clib_ov8838.dppclib_first_addr;
		p_yushan_regs->dopclib_first_addr = yushan_regs_clib_ov8838.dopclib_first_addr;
	}
	else if (strcmp(data.sensor_name, "ar0260") == 0) {
		p_yushan_regs->pdpclib = yushan_regs_clib_ar0260.pdpclib;
		p_yushan_regs->dppclib = yushan_regs_clib_ar0260.dppclib;
		p_yushan_regs->dopclib = yushan_regs_clib_ar0260.dopclib;
		p_yushan_regs->pdpclib_size = yushan_regs_clib_ar0260.pdpclib_size;
		p_yushan_regs->dppclib_size = yushan_regs_clib_ar0260.dppclib_size;
		p_yushan_regs->dopclib_size = yushan_regs_clib_ar0260.dopclib_size;
		p_yushan_regs->pdpclib_first_addr = yushan_regs_clib_ar0260.pdpclib_first_addr;
		p_yushan_regs->dppclib_first_addr = yushan_regs_clib_ar0260.dppclib_first_addr;
		p_yushan_regs->dopclib_first_addr = yushan_regs_clib_ar0260.dopclib_first_addr;
	}
	else if (strcmp(data.sensor_name, "ov2722") == 0) {
		p_yushan_regs->pdpclib = yushan_regs_clib_ov2722.pdpclib;
		p_yushan_regs->dppclib = yushan_regs_clib_ov2722.dppclib;
		p_yushan_regs->dopclib = yushan_regs_clib_ov2722.dopclib;
		p_yushan_regs->pdpclib_size = yushan_regs_clib_ov2722.pdpclib_size;
		p_yushan_regs->dppclib_size = yushan_regs_clib_ov2722.dppclib_size;
		p_yushan_regs->dopclib_size = yushan_regs_clib_ov2722.dopclib_size;
		p_yushan_regs->pdpclib_first_addr = yushan_regs_clib_ov2722.pdpclib_first_addr;
		p_yushan_regs->dppclib_first_addr = yushan_regs_clib_ov2722.dppclib_first_addr;
		p_yushan_regs->dopclib_first_addr = yushan_regs_clib_ov2722.dopclib_first_addr;
	}

	sDxoStruct.pDxoPdpRamImage[0] = (uint8_t *)p_yushan_regs->pdpcode;
	sDxoStruct.pDxoDppRamImage[0] = (uint8_t *)p_yushan_regs->dppcode;
	sDxoStruct.pDxoDopRamImage[0] = (uint8_t *)p_yushan_regs->dopcode;
	sDxoStruct.pDxoPdpRamImage[1] = (uint8_t *)p_yushan_regs->pdpclib;
	sDxoStruct.pDxoDppRamImage[1] = (uint8_t *)p_yushan_regs->dppclib;
	sDxoStruct.pDxoDopRamImage[1] = (uint8_t *)p_yushan_regs->dopclib;

	sDxoStruct.uwDxoPdpRamImageSize[0] = p_yushan_regs->pdpcode_size;
	sDxoStruct.uwDxoDppRamImageSize[0] = p_yushan_regs->dppcode_size;
	sDxoStruct.uwDxoDopRamImageSize[0] = p_yushan_regs->dopcode_size;
	sDxoStruct.uwDxoPdpRamImageSize[1] = p_yushan_regs->pdpclib_size;
	sDxoStruct.uwDxoDppRamImageSize[1] = p_yushan_regs->dppclib_size;
	sDxoStruct.uwDxoDopRamImageSize[1] = p_yushan_regs->dopclib_size;

	sDxoStruct.uwBaseAddrPdpMicroCode[0] = p_yushan_regs->pdpcode_first_addr;
	sDxoStruct.uwBaseAddrDppMicroCode[0] = p_yushan_regs->dppcode_first_addr;
	sDxoStruct.uwBaseAddrDopMicroCode[0] = p_yushan_regs->dopcode_first_addr;
	sDxoStruct.uwBaseAddrPdpMicroCode[1] = p_yushan_regs->pdpclib_first_addr;
	sDxoStruct.uwBaseAddrDppMicroCode[1] = p_yushan_regs->dppclib_first_addr;
	sDxoStruct.uwBaseAddrDopMicroCode[1] = p_yushan_regs->dopclib_first_addr;

	sDxoStruct.uwDxoPdpBootAddr = p_yushan_regs->pdpBootAddr;
	sDxoStruct.uwDxoDppBootAddr = p_yushan_regs->dppBootAddr;
	sDxoStruct.uwDxoDopBootAddr = p_yushan_regs->dopBootAddr;

	sDxoStruct.uwDxoPdpStartAddr = p_yushan_regs->pdpStartAddr;
	sDxoStruct.uwDxoDppStartAddr = p_yushan_regs->dppStartAddr;
	sDxoStruct.uwDxoDopStartAddr = p_yushan_regs->dopStartAddr;

#if 0
	pr_info("/*---------------------------------------*\\");
	pr_info("array base ADDRs %d %d %d %d %d %d",
		sDxoStruct.uwBaseAddrPdpMicroCode[0], sDxoStruct.uwBaseAddrDppMicroCode[0], sDxoStruct.uwBaseAddrDopMicroCode[0],
		sDxoStruct.uwBaseAddrPdpMicroCode[1], sDxoStruct.uwBaseAddrDppMicroCode[1], sDxoStruct.uwBaseAddrDopMicroCode[1]);
	pr_info("array 1st values %d %d %d %d %d %d",
		*sDxoStruct.pDxoPdpRamImage[0], *sDxoStruct.pDxoDppRamImage[0], *sDxoStruct.pDxoDopRamImage[0],
		*sDxoStruct.pDxoPdpRamImage[1], *sDxoStruct.pDxoDppRamImage[1], *sDxoStruct.pDxoDopRamImage[1]);
	pr_info("array sizes %d %d %d %d %d %d",
		sDxoStruct.uwDxoPdpRamImageSize[0], sDxoStruct.uwDxoDppRamImageSize[0], sDxoStruct.uwDxoDopRamImageSize[0],
		sDxoStruct.uwDxoPdpRamImageSize[1], sDxoStruct.uwDxoDppRamImageSize[1], sDxoStruct.uwDxoDopRamImageSize[1]);
	pr_info("Boot Addr %d %d %d",
		sDxoStruct.uwDxoPdpBootAddr, sDxoStruct.uwDxoDppBootAddr, sDxoStruct.uwDxoDopBootAddr);
	pr_info("\\*---------------------------------------*/");
#endif

	
	
#if 0
	
	switch (bSpiFreq) {
	case 0x80:	
		udwSpiFreq = 8<<16;
		break;
	case 0x00:	
		udwSpiFreq = 4<<16;
		break;
	case 0x81:	
		udwSpiFreq = 2<<16;
		break;
	case 0x01:	
		udwSpiFreq = 1<<16;
		break;
	case 0x82:	
		udwSpiFreq = 1<<8;
		break;
	case 0x02:	
		udwSpiFreq = 1<<4;
		break;
	case 0x03:	
		udwSpiFreq = 1<<2;
		break;
	  }
#endif

	sInitStruct.fpSpiClock			=	data.spi_clk*(1<<16);  
	sInitStruct.fpExternalClock		=	sInitStruct.fpExternalClock << 16; 

	sInitStruct.uwActivePixels = uwHSize;
	sInitStruct.uwLineBlankStill = uwBlkPixels;
	sInitStruct.uwLineBlankVf = uwBlkPixels;
	sInitStruct.uwLines = uwVSize;
	sInitStruct.uwFrameBlank = uwBlkLines;
	sInitStruct.bUseExternalLDO = data.use_ext_1v2;
	sImageChar_context.bImageOrientation = data.orientation;
	sImageChar_context.uwXAddrStart = data.x_addr_start;
	sImageChar_context.uwYAddrStart = data.y_addr_start;
	sImageChar_context.uwXAddrEnd = data.x_addr_end;
	sImageChar_context.uwYAddrEnd = data.y_addr_end;
	sImageChar_context.uwXEvenInc = data.x_even_inc;
	sImageChar_context.uwXOddInc = data.x_odd_inc;
	sImageChar_context.uwYEvenInc = data.y_even_inc;
	sImageChar_context.uwYOddInc = data.y_odd_inc;
	sImageChar_context.bBinning = data.binning_rawchip;

	memset(sInitStruct.sFrameFormat, 0, sizeof(Yushan_Frame_Format_t)*15);
	if ((bPixelFormat == RAW8) || (bPixelFormat == RAW10_8)) {
		CDBG("[CAM] bPixelFormat==RAW8");
		sInitStruct.sFrameFormat[0].uwWordcount = (uwHSize);	
		sInitStruct.sFrameFormat[0].bDatatype = 0x2a;		  
	} else { 
		CDBG("[CAM] bPixelFormat==RAW10");
		sInitStruct.sFrameFormat[0].uwWordcount = (uwHSize*10)/8;	 
		sInitStruct.sFrameFormat[0].bDatatype = 0x2b;		  
	}
	
	if (bPixelFormat == RAW10_8) {
		sInitStruct.sFrameFormat[0].bDatatype = 0x30;
	}
	sInitStruct.sFrameFormat[0].bActiveDatatype = 1;
	sInitStruct.sFrameFormat[0].bSelectStillVfMode = YUSHAN_FRAME_FORMAT_STILL_MODE;

	sInitStruct.bValidWCEntries = 1;

	sGainsExpTime.uwAnalogGainCodeGR = 0x20; 
	sGainsExpTime.uwAnalogGainCodeR = 0x20;
	sGainsExpTime.uwAnalogGainCodeB = 0x20;
	sGainsExpTime.uwPreDigGainGR = 0x100;
	sGainsExpTime.uwPreDigGainR = 0x100;
	sGainsExpTime.uwPreDigGainB = 0x100;
	sGainsExpTime.uwExposureTime = 0x20;
	sGainsExpTime.bRedGreenRatio = 0x40;
	sGainsExpTime.bBlueGreenRatio = 0x40;

	sDxoDppTuning.bTemporalSmoothing = 0x63; 
	sDxoDppTuning.uwFlashPreflashRating = 0;
	sDxoDppTuning.bFocalInfo = 0;

	sDxoPdpTuning.bDeadPixelCorrectionLowGain = 0x80;
	sDxoPdpTuning.bDeadPixelCorrectionMedGain = 0x80;
	sDxoPdpTuning.bDeadPixelCorrectionHiGain = 0x80;

#if 0
	sDxoDopTuning.uwForceClosestDistance = 0;	
	sDxoDopTuning.uwForceFarthestDistance = 0;
#endif
	sDxoDopTuning.bEstimationMode = 1;
	sDxoDopTuning.bSharpness = 0x01; 
	sDxoDopTuning.bDenoisingLowGain = 0x1; 
	sDxoDopTuning.bNoiseVsDetailsLowGain = 0xA0;
	sDxoDopTuning.bNoiseVsDetailsMedGain = 0x80;
	sDxoDopTuning.bNoiseVsDetailsHiGain = 0x80;
	bDppMode = DPP_enable ? 0x03 : 0;

	if (strcmp(data.sensor_name, "s5k3h2yx") == 0)
	{
		sDxoDopTuning.bDenoisingMedGain = 0x60;
		sDxoDopTuning.bDenoisingHiGain = 0x40;
	}
	else if (strcmp(data.sensor_name, "imx175") == 0)
	{
	    sDxoDopTuning.bSharpness = 0x01;
	    sDxoDopTuning.bDenoisingLowGain = 0x30;
	    sDxoDopTuning.bDenoisingMedGain = 0x80;
	    sDxoDopTuning.bDenoisingHiGain =  0x60;
 
	    sDxoDopTuning.bNoiseVsDetailsLowGain = 0xD0;
	    sDxoDopTuning.bNoiseVsDetailsMedGain = 0xB0;
	    sDxoDopTuning.bNoiseVsDetailsHiGain = 0xB0;
	}
	else if (strcmp(data.sensor_name, "ar0260") == 0)
	{
		sDxoDopTuning.bDenoisingLowGain = 0x60;
		sDxoDopTuning.bDenoisingMedGain = 0x60;
		sDxoDopTuning.bDenoisingHiGain =  0x60;
	    sDxoDopTuning.bNoiseVsDetailsLowGain = 0x80;
	    sDxoDopTuning.bNoiseVsDetailsMedGain = 0x80;
	    sDxoDopTuning.bNoiseVsDetailsHiGain = 0x80;
	    sDxoDopTuning.bSharpness = 0;
		bDppMode =0;
	}
	else
	{
		sDxoDopTuning.bDenoisingMedGain = 0x60;
		sDxoDopTuning.bDenoisingHiGain = 0x40;
	}

	sDxoDopTuning.bTemporalSmoothing = 0x26; 

	gPllLocked = 0;
	CDBG("[CAM] Yushan_common_init Yushan_Init_Clocks\n");
	bStatus = Yushan_Init_Clocks(&sInitStruct, &sSystemStatus, udwIntrMask) ;
	if (bStatus != SUCCESS) {
		pr_err("[CAM] Clock Init FAILED\n");
		pr_err("[CAM] Yushan_common_init Yushan_Init_Clocks=%d\n", bStatus);
		pr_err("[CAM] Min Value Required %d\n", sSystemStatus.udwDxoConstraintsMinValue);
		pr_err("[CAM] Error Code : %d\n", sSystemStatus.bDxoConstraints);
		return -1;
	} else
		CDBG("[CAM] Clock Init Done \n");
	
	gPllLocked = 1;

	
	Yushan_AssignInterruptGroupsToPad1(uwAssignITRGrpToPad1);

	CDBG("[CAM] Yushan_common_init Yushan_Init\n");
	bStatus = Yushan_Init(&sInitStruct) ;
	CDBG("[CAM] Yushan_common_init Yushan_Init=%d\n", bStatus);

	
	if (bPixelFormat == RAW10_8)
		Yushan_DCPX_CPX_Enable();

	if (bStatus == 0) {
		pr_err("[CAM] Yushan Init FAILED\n");
		return -1;
	}
	

	if (data.use_rawchip == RAWCHIP_DXO_BYPASS) {
		
		Yushan_DXO_Lecci_Bypass();
	}

	if (data.use_rawchip == RAWCHIP_MIPI_BYPASS) {
		
		Yushan_DXO_DTFilter_Bypass();
	}

	if (data.use_rawchip == RAWCHIP_ENABLE) {
		CDBG("[CAM] Yushan_common_init Yushan_Init_Dxo\n");
		
		bStatus = Yushan_Init_Dxo(&sDxoStruct, bBypassDxoUpload);
		CDBG("[CAM] Yushan_common_init Yushan_Init_Dxo=%d\n", bStatus);
		if (bStatus == SUCCESS) {
			CDBG("[CAM] DXO Upload and Init Done\n");
		} else {
			pr_err("[CAM] DXO Upload and Init FAILED\n");
			return -1;
		}
		CDBG("[CAM] Yushan_common_init Yushan_Get_Version_Information\n");

		bStatus = Yushan_Get_Version_Information(&sYushanVersionInfo) ;
	
	#if 1 
		CDBG("Yushan_common_init Yushan_Get_Version_Information=%d\n", bStatus);

		CDBG("API Version : %d.%d \n", sYushanVersionInfo.bApiMajorVersion, sYushanVersionInfo.bApiMinorVersion);
		CDBG("DxO Pdp Version : %x \n", sYushanVersionInfo.udwPdpVersion);
		CDBG("DxO Dpp Version : %x \n", sYushanVersionInfo.udwDppVersion);
		CDBG("DxO Dop Version : %x \n", sYushanVersionInfo.udwDopVersion);
		CDBG("DxO Pdp Calibration Version : %x \n", sYushanVersionInfo.udwPdpCalibrationVersion);
		CDBG("DxO Dpp Calibration Version : %x \n", sYushanVersionInfo.udwDppCalibrationVersion);
		CDBG("DxO Dop Calibration Version : %x \n", sYushanVersionInfo.udwDopCalibrationVersion);
	#endif
	

	#if 0
		
		if (bTestPatternMode == 1) {
			
			Yushan_PatternGenerator(&sInitStruct, bPatternReq, bDxoBypassForTestPattern);
			
			return 0;
		}
	#endif

		
		Yushan_Update_ImageChar(&sImageChar_context);
	
		Yushan_Update_SensorParameters(&sGainsExpTime);
		Yushan_Update_DxoDpp_TuningParameters(&sDxoDppTuning);
		Yushan_Update_DxoDop_TuningParameters(&sDxoDopTuning);
		Yushan_Update_DxoPdp_TuningParameters(&sDxoPdpTuning);
		bStatus = Yushan_Update_Commit(bPdpMode, bDppMode, bDopMode);
		CDBG("[CAM] Yushan_common_init Yushan_Update_Commit=%d\n", bStatus);

		
		
		bStatus &= Yushan_WaitForInterruptEvent(EVENT_PDP_EOF_EXECCMD, TIME_100MS);
		if (!bStatus)
		{
			pr_err("[CAM] EVENT_PDP_EOF_EXECCMD fail\n");
			return -1;
		}

		bStatus &= Yushan_WaitForInterruptEvent2(EVENT_DOP7_EOF_EXECCMD, TIME_100MS);
		if (!bStatus)
		{
			pr_err("[CAM] EVENT_DOP7_EOF_EXECCMD fail\n");
			return -1;
		}

		bStatus &= Yushan_WaitForInterruptEvent(EVENT_DPP_EOF_EXECCMD, TIME_100MS);
		if (!bStatus)
		{
			pr_err("[CAM] EVENT_DPP_EOF_EXECCMD fail\n");
			return -1;
		}

		if (bStatus == 1)
			CDBG("[CAM] DXO Commit Done\n");
		else {
			pr_err("[CAM] DXO Commit FAILED\n");
			
		}
	}
#if 0
	
	bSpiData = 0;
	SPI_Write(YUSHAN_SMIA_FM_EOF_INT_EN, 1,  &bSpiData);
#endif

	return (bStatus == SUCCESS) ? 0 : -1;
#endif
	return 0;
}
bool_t Yushan_Dxo_Dop_Af_Run(Yushan_AF_ROI_t	*sYushanAfRoi, uint32_t *pAfStatsGreen, uint8_t	bRoiActiveNumber)
{

	uint8_t		bStatus = SUCCESS;
	
#if 1
	uint32_t		enableIntrMask[] = {0x00338E30, 0x00000000, 0x00018000};
	uint32_t		disableIntrMask[] = {0x00238E30, 0x00000000, 0x00018000};
#endif

	

	

	if (bRoiActiveNumber)
	{
		Yushan_Intr_Enable((uint8_t*)enableIntrMask);
		
		bStatus = Yushan_AF_ROI_Update(&sYushanAfRoi[0], bRoiActiveNumber);
		bStatus &= Yushan_Update_Commit(bPdpMode,bDppMode,bDopMode);

		
		bStatus &= Yushan_WaitForInterruptEvent2(EVENT_DXODOP7_NEWFRAMEPROC_ACK, TIME_100MS);
	}
	else
		Yushan_Intr_Enable((uint8_t*)disableIntrMask);

#if 0
	if (bStatus) {
		
		#if 0
		yushan_go_to_position(0, 0);
		for(i=1; i<=42; i++)
		{
			s5k3h2yx_move_focus( 1, i);
			bStatus = Yushan_Read_AF_Statistics(pAfStatsGreen);
		}
		#endif
		bStatus = Yushan_Read_AF_Statistics(pAfStatsGreen);
	}

	if (!bStatus) {
		pr_err("ROI AF Statistics read failed\n");
		return FAILURE;
	} else
		pr_err("Read ROI AF Statistics successfully\n");
#endif

	return SUCCESS;

}

#if 0
bool_t Yushan_get_AFSU(Yushan_AF_Stats_t* sYushanAFStats)
{

	uint8_t		bStatus = SUCCESS, bNumOfActiveRoi = 0,  bCount = 0;
	uint32_t	udwSpiData[4];
	uint16_t val ;

#if 0
			YushanPrintFrameNumber();
			YushanPrintDxODOPAfStrategy();
			YushanPrintImageInformation();
			YushanPrintVisibleLineSizeAndRoi();
#endif

	
	bStatus &= SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_ROI_active_number_7_0, 1, (uint8_t*)(&bNumOfActiveRoi));

	if (!bNumOfActiveRoi) 
		return SUCCESS;

	SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_frame_number_7_0, 2, (uint8_t *)(&val));
	sYushanAFStats[0].frameIdx= val;

	
	
	while(bCount<bNumOfActiveRoi)
	{
		
		bStatus &= SPI_Read((uint16_t)(DXO_DOP_BASE_ADDR + DxODOP_ROI_0_stats_G_7_0 + bCount*16), 16, (uint8_t *)(&udwSpiData[0]));
		
		sYushanAFStats[bCount].udwAfStatsGreen        = udwSpiData[0];
		sYushanAFStats[bCount].udwAfStatsRed            = udwSpiData[1];
		sYushanAFStats[bCount].udwAfStatsBlue           = udwSpiData[2];
		sYushanAFStats[bCount].udwAfStatsConfidence = udwSpiData[3];
		sYushanAFStats[bCount].frameIdx                    = sYushanAFStats[0].frameIdx;
#if 0
		pr_info("[CAM]%s, G:%d, R:%d, B:%d, confidence:%d (%d) \n", __func__,
			sYushanAFStats[bCount].udwAfStatsGreen,
			sYushanAFStats[bCount].udwAfStatsRed,
		    sYushanAFStats[bCount].udwAfStatsBlue,
		    sYushanAFStats[bCount].udwAfStatsConfidence,
		    sYushanAFStats[bCount].frameIdx);
#endif
		bCount++;
	}



	return bStatus;
}
#else
int Yushan_get_AFSU(rawchip_af_stats* af_stats)
{

	uint8_t		bStatus = SUCCESS;

	bStatus = Yushan_Read_AF_Statistics(af_stats->udwAfStats, 1, &af_stats->frameIdx);
	if (bStatus == FAILURE) {
		pr_err("[CAM] Get AFSU statistic data fail\n");
		return -1;
	}
	CDBG("[CAM] GET_AFSU:G:%d, R:%d, B:%d, confi:%d, frmIdx:%d\n",
		af_stats->udwAfStats[0].udwAfStatsGreen,
		af_stats->udwAfStats[0].udwAfStatsRed,
		af_stats->udwAfStats[0].udwAfStatsBlue,
		af_stats->udwAfStats[0].udwAfStatsConfidence,
		af_stats->frameIdx);
	return 0;
}
#endif


int	Yushan_ContextUpdate_Wrapper(Yushan_New_Context_Config_t	sYushanNewContextConfig, Yushan_ImageChar_t	sImageNewChar_context)
{

	bool_t	bStatus = SUCCESS;
	

		CDBG("[CAM] Reconfiguration starts:%d,%d,%d\n",
			sYushanNewContextConfig->uwActiveFrameLength,
			sYushanNewContextConfig->uwActivePixels,
			sYushanNewContextConfig->uwLineBlank);
		bStatus = Yushan_Context_Config_Update(&sYushanNewContextConfig);
		

#if 0
	
	sImageChar_context.bImageOrientation = sYushanNewContextConfig->orientation;
	sImageChar_context.uwXAddrStart = sYushanNewContextConfig->uwXAddrStart;
	sImageChar_context.uwYAddrStart = sYushanNewContextConfig->uwYAddrStart;
	sImageChar_context.uwXAddrEnd= sYushanNewContextConfig->uwXAddrEnd;
	sImageChar_context.uwYAddrEnd= sYushanNewContextConfig->uwYAddrEnd;

	sImageChar_context.uwXEvenInc = sYushanNewContextConfig->uwXEvenInc;
	sImageChar_context.uwXOddInc = sYushanNewContextConfig->uwXOddInc;
	sImageChar_context.uwYEvenInc = sYushanNewContextConfig->uwYEvenInc;
	sImageChar_context.uwYOddInc = sYushanNewContextConfig->uwYOddInc;
	sImageChar_context.bBinning = sYushanNewContextConfig->bBinning;
#endif


	
	Yushan_Update_ImageChar(&sImageNewChar_context);
	bStatus &= Yushan_Update_Commit(bPdpMode,bDppMode,bDopMode);

	
	
	bStatus &= Yushan_WaitForInterruptEvent(EVENT_PDP_EOF_EXECCMD, TIME_100MS);
	if (!bStatus)
	{
		pr_err("[CAM] EVENT_PDP_EOF_EXECCMD fail\n");
		return -1;
	}

	bStatus &= Yushan_WaitForInterruptEvent2(EVENT_DOP7_EOF_EXECCMD, TIME_100MS);
	if (!bStatus)
	{
		pr_err("[CAM] EVENT_DOP7_EOF_EXECCMD fail\n");
		return -1;
	}

	bStatus &= Yushan_WaitForInterruptEvent(EVENT_DPP_EOF_EXECCMD, TIME_100MS);
	if (!bStatus)
	{
		pr_err("[CAM] EVENT_DPP_EOF_EXECCMD fail\n");
		return -1;
	}

	if (bStatus)
		CDBG("[CAM] DXO Commit, Post Context Reconfigration, Done\n");
	else
		pr_err("[CAM] DXO Commit, Post Context Reconfigration, FAILED\n");

	return (bStatus == SUCCESS) ? 0 : -1;

}

#if 0
void Yushan_Write_Exp_Time_Gain(uint16_t yushan_line, uint16_t yushan_gain)
{
#if 0
	Yushan_GainsExpTime_t sGainsExpTime;
	uint32_t udwSpiBaseIndex;
	uint32_t spidata;
	float ratio = 0.019;
	pr_info("[CAM] Yushan_Write_Exp_Time_Gain, yushan_gain:%d, yushan_line:%d", yushan_gain, yushan_line);
#endif

	sGainsExpTime.uwAnalogGainCodeGR= yushan_gain;
	sGainsExpTime.uwAnalogGainCodeR=yushan_gain;
	sGainsExpTime.uwAnalogGainCodeB=yushan_gain;
	sGainsExpTime.uwPreDigGainGR= 0x100;
	sGainsExpTime.uwPreDigGainR= 0x100;
	sGainsExpTime.uwPreDigGainB= 0x100;
	sGainsExpTime.uwExposureTime= (uint16_t)((19*yushan_line/1000));

	if (sGainsExpTime.bRedGreenRatio == 0) sGainsExpTime.bRedGreenRatio=0x40;
	if (sGainsExpTime.bBlueGreenRatio == 0) sGainsExpTime.bBlueGreenRatio=0x40;

	pr_err("[CAM] uwExposureTime: %d\n", sGainsExpTime.uwExposureTime);
	Yushan_Update_SensorParameters(&sGainsExpTime);
#if 0
	pr_info("DxO Regiser Dump Start******* \n");
	SPI_Read((0x821E), 4, (uint8_t *)(&spidata));
	pr_info("DxO DOP 0x821E : %x \n",spidata);
	SPI_Read((0x8222), 4, (uint8_t *)(&spidata));
	pr_info("DxO DOP 0x8222 : %x \n",spidata);
	SPI_Read((0x8226), 4, (uint8_t *)(&spidata));
	pr_info("DxO DOP 0x8226 : %x \n",spidata);
	SPI_Read((0x822A), 4, (uint8_t *)(&spidata));
	pr_info("DxO DOP 0x822A : %x \n",spidata);
	SPI_Read((0x822E), 2, (uint8_t *)(&spidata));
	pr_info("DxO DOP 0x822E : %x \n",spidata);
	SPI_Read((0x8204), 4, (uint8_t *)(&spidata));
	pr_info("DxO DOP Cali_ID : %x \n",spidata);

	udwSpiBaseIndex = 0x010000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	SPI_Read((0x821E), 4, (uint8_t *)(&spidata));
	pr_info("DxO DPP 0x821E : %x \n",spidata);
	SPI_Read((0x8222), 4, (uint8_t *)(&spidata));
	pr_info("DxO DPP 0x8222 : %x \n",spidata);
	SPI_Read((0x8226), 4, (uint8_t *)(&spidata));
	pr_info("DxO DPP 0x8226 : %x \n",spidata);
	SPI_Read((0x822A), 2, (uint8_t *)(&spidata));
	pr_info("DxO DPP 0x822A : %x \n",spidata);
	SPI_Read((0x8204), 4, (uint8_t *)(&spidata));
	pr_info("DxO DPP Cali_ID : %x \n",spidata);

	udwSpiBaseIndex = 0x08000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	SPI_Read((0x621E), 4, (uint8_t *)(&spidata));
	pr_info("DxO PDP 0x621E : %x \n",spidata);
	SPI_Read((0x6222), 4, (uint8_t *)(&spidata));
	pr_info("DxO PDP 0x6222 : %x \n",spidata);
	SPI_Read((0x6226), 4, (uint8_t *)(&spidata));
	pr_info("DxO PDP 0x6226 : %x \n",spidata);
	SPI_Read((0x622A), 2, (uint8_t *)(&spidata));
	pr_info("DxO PDP 0x622A : %x \n",spidata);
	SPI_Read((0x6204), 4, (uint8_t *)(&spidata));
	pr_info("DxO PDP Cali_ID : %x \n",spidata);
#endif

}
#endif

int Yushan_Set_AF_Strategy(uint8_t afStrategy)
{
	uint8_t		bStatus = SUCCESS;
	CDBG("[CAM] Yushan_Set_AF_Strategy\n");
	bStatus = Yushan_Update_DxoDop_Af_Strategy(afStrategy);
	return (bStatus == SUCCESS) ? 0 : -1;
}

int Yushan_Get_Version(rawchip_dxo_version* dxo_version)
{
	uint32_t udwSpiBaseIndex = 0;
	CDBG("[CAM] Yushan_Get_Version\n");

	SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_ucode_id_7_0 , 2,(uint8_t *)(&(dxo_version->udwDOPUcodeId)));
	SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_hw_id_7_0      , 2,(uint8_t *)(&(dxo_version->udwDOPHwId    )));
	SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_calib_id_0_7_0, 4,(uint8_t *)(&(dxo_version->udwDOPCalibId )));

	udwSpiBaseIndex = 0x010000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	SPI_Read((DXO_DPP_BASE_ADDR-0x8000)+DxODPP_ucode_id_7_0 , 2, (uint8_t *)(&(dxo_version->udwDPPUcodeId)));
	SPI_Read((DXO_DPP_BASE_ADDR-0x8000)+DxODPP_hw_id_7_0      , 2, (uint8_t *)(&(dxo_version->udwDPPHwId)));
	SPI_Read((DXO_DPP_BASE_ADDR-0x8000)+DxODPP_calib_id_0_7_0, 4, (uint8_t *)(&(dxo_version->udwDPPCalibId)));
	udwSpiBaseIndex = 0x08000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

       SPI_Read(DXO_PDP_BASE_ADDR+DxOPDP_ucode_id_7_0 , 2,(uint8_t *)(&(dxo_version->udwPDPUcodeId)));
	SPI_Read(DXO_PDP_BASE_ADDR+DxOPDP_hw_id_7_0      , 2,(uint8_t *)(&(dxo_version->udwPDPHwId)));
	SPI_Read(DXO_PDP_BASE_ADDR+DxOPDP_calib_id_0_7_0, 4,(uint8_t *)(&(dxo_version->udwPDPCalibId)));

	CDBG("[CAM] Yushan_Get_Version : DOP ucodeid 0x%04x\n",dxo_version->udwDOPUcodeId);
	CDBG("[CAM] Yushan_Get_Version : DOP hwid     0x%04x\n",dxo_version->udwDOPHwId);
	CDBG("[CAM] Yushan_Get_Version : DOP calibid  0x%08x\n",dxo_version->udwDOPCalibId);
	CDBG("[CAM] Yushan_Get_Version : DPP ucodeid 0x%04x\n",dxo_version->udwDPPUcodeId);
	CDBG("[CAM] Yushan_Get_Version : DPP hwid     0x%04x\n",dxo_version->udwDPPHwId);
	CDBG("[CAM] Yushan_Get_Version : DPP calibid  0x%08x\n",dxo_version->udwDPPCalibId);
	CDBG("[CAM] Yushan_Get_Version : PDP ucodeid 0x%04x\n",dxo_version->udwPDPUcodeId);
	CDBG("[CAM] Yushan_Get_Version : PDP hwid     0x%04x\n",dxo_version->udwPDPHwId);
	CDBG("[CAM] Yushan_Get_Version : PDP calibid  0x%08x\n",dxo_version->udwPDPCalibId);

	return SUCCESS;
}
int Yushan_Update_AEC_AWB_Params(rawchip_update_aec_awb_params_t *update_aec_awb_params)
{
	uint8_t bStatus = SUCCESS;
	Yushan_GainsExpTime_t sGainsExpTime;

	sGainsExpTime.uwAnalogGainCodeGR = update_aec_awb_params->aec_params.gain;
	sGainsExpTime.uwAnalogGainCodeR = update_aec_awb_params->aec_params.gain;
	sGainsExpTime.uwAnalogGainCodeB = update_aec_awb_params->aec_params.gain;
	sGainsExpTime.uwPreDigGainGR = update_aec_awb_params->aec_params.dig_gain;
	sGainsExpTime.uwPreDigGainR = update_aec_awb_params->aec_params.dig_gain;
	sGainsExpTime.uwPreDigGainB = update_aec_awb_params->aec_params.dig_gain;
	sGainsExpTime.uwExposureTime = update_aec_awb_params->aec_params.exp;
	sGainsExpTime.bRedGreenRatio = update_aec_awb_params->awb_params.rg_ratio;
	sGainsExpTime.bBlueGreenRatio = update_aec_awb_params->awb_params.bg_ratio;
#if 0
	if (sGainsExpTime.bRedGreenRatio == 0)
		sGainsExpTime.bRedGreenRatio = 0x40;
	if (sGainsExpTime.bBlueGreenRatio == 0)
		sGainsExpTime.bBlueGreenRatio = 0x40;
#endif

	CDBG("[CAM] uwExposureTime: %d\n", sGainsExpTime.uwExposureTime);
	bStatus = Yushan_Update_SensorParameters(&sGainsExpTime);

	return (bStatus == SUCCESS) ? 0 : -1;
}

int Yushan_Update_AF_Params(rawchip_update_af_params_t *update_af_params)
{

	uint8_t bStatus = SUCCESS;
	bStatus = Yushan_AF_ROI_Update(&update_af_params->af_params.sYushanAfRoi[0],
		update_af_params->af_params.active_number);
	return (bStatus == SUCCESS) ? 0 : -1;
}

int Yushan_Update_3A_Params(rawchip_newframe_ack_enable_t enable_newframe_ack)
{
	uint8_t bStatus = SUCCESS;
	uint32_t		enableIntrMask[] = {0x3DF38E3B, 0xFC3C3C3C, 0x001B7FFB};
	uint32_t		disableIntrMask[] = {0x3DE38E3B, 0xFC3C3C3C, 0x001B7FFB};
	if (enable_newframe_ack == RAWCHIP_NEWFRAME_ACK_ENABLE)
		Yushan_Intr_Enable((uint8_t*)enableIntrMask);
	else if (enable_newframe_ack == RAWCHIP_NEWFRAME_ACK_DISABLE)
		Yushan_Intr_Enable((uint8_t*)disableIntrMask);
	bStatus = Yushan_Update_Commit(bPdpMode, bDppMode, bDopMode);
	return (bStatus == SUCCESS) ? 0 : -1;
}

void Yushan_dump_register(void)
{
	uint16_t read_data = 0;
	uint8_t i;
	for (i = 0; i < 50; i++) {
		
		rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_FRAME_NUMBER, &read_data);
		pr_info("[CAM] Yushan's in counting=%d\n", read_data);

		
		rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_0, &read_data);
		pr_info("[CAM] Yushan's out counting=%d\n", read_data);

		mdelay(30);
	}

	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_STATUS=%x\n", read_data);
}

void Yushan_dump_all_register(void)
{
	uint16_t read_data = 0;
	uint8_t i;
	for (i = 0; i < 50; i++) {
		
		rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_FRAME_NUMBER, &read_data);
		pr_info("[CAM] Yushan's in counting=%d\n", read_data);

		
		rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_0, &read_data);
		pr_info("[CAM] Yushan's out counting=%d\n", read_data);

		mdelay(30);
	}

	rawchip_spi_read_2B2B(YUSHAN_CLK_DIV_FACTOR, &read_data);
	pr_info("[CAM]YUSHAN_CLK_DIV_FACTOR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CLK_DIV_FACTOR_2, &read_data);
	pr_info("[CAM]YUSHAN_CLK_DIV_FACTOR_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CLK_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CLK_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_RESET_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_RESET_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PLL_CTRL_MAIN, &read_data);
	pr_info("[CAM]YUSHAN_PLL_CTRL_MAIN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PLL_LOOP_OUT_DF, &read_data);
	pr_info("[CAM]YUSHAN_PLL_LOOP_OUT_DF=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PLL_SSCG_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_PLL_SSCG_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_HOST_IF_SPI_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_HOST_IF_SPI_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_HOST_IF_SPI_DEVADDR, &read_data);
	pr_info("[CAM]YUSHAN_HOST_IF_SPI_DEVADDR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, &read_data);
	pr_info("[CAM]YUSHAN_HOST_IF_SPI_BASE_ADDRESS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2RX_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2RX_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_PDP_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_PDP_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DPP_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DPP_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_DOP7_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_DOP7_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_CSI2TX_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_CSI2TX_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_PHY_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_PHY_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_TX_PHY_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_TX_PHY_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_IDP_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_IDP_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_RX_CHAR_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_RX_CHAR_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_LBE_POST_DXO_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_LBE_POST_DXO_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_SYS_DOMAIN_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_SYS_DOMAIN_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_ITPOINT_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_ITPOINT_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_EN_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_EN_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_EN_STATUS_BCLR, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_EN_STATUS_BCLR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITM_P2W_UFLOW_EN_STATUS_BSET, &read_data);
	pr_info("[CAM]YUSHAN_ITM_P2W_UFLOW_EN_STATUS_BSET=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_DATA_WORD_0, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_DATA_WORD_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_DATA_WORD_1, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_DATA_WORD_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_DATA_WORD_2, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_DATA_WORD_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_DATA_WORD_3, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_DATA_WORD_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_HYST, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_HYST=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_PDN, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_PDN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_PUN, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_PUN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_LOWEMI, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_LOWEMI=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_PAD_IN, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_PAD_IN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_RATIO_PAD, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_RATIO_PAD=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_SEND_ITR_PAD1, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_SEND_ITR_PAD1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_INTR_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_INTR_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IOR_NVM_LDO_STS_REG, &read_data);
	pr_info("[CAM]YUSHAN_IOR_NVM_LDO_STS_REG=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_REFILL_ELT_NB, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_REFILL_ELT_NB=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_REFILL_ERROR, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_REFILL_ERROR=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_REG_DFV_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_REG_DFV_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_MEM_PAGE, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_MEM_PAGE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_MEM_LOWER_ELT, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_MEM_LOWER_ELT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_T1_DMA_MEM_UPPER_ELT, &read_data);
	pr_info("[CAM]YUSHAN_T1_DMA_MEM_UPPER_ELT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_UIX4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_UIX4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SWAP_PINS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SWAP_PINS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_INVERT_HS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_INVERT_HS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_STOP_STATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_STOP_STATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_ULP_STATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_ULP_STATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_CLK_ACTIVE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_CLK_ACTIVE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_FORCE_RX_MODE_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_FORCE_RX_MODE_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_TEST_RESERVED, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_TEST_RESERVED=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_ESC_DL_STS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_ESC_DL_STS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_EOT_BYPASS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_EOT_BYPASS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_HSRX_SHIFT_CL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_HSRX_SHIFT_CL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_HS_RX_SHIFT_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_HS_RX_SHIFT_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_VIL_CL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_VIL_CL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_VIL_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_VIL_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_OVERSAMPLE_BYPASS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_OVERSAMPLE_BYPASS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_OVERSAMPLE_FLAG1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_OVERSAMPLE_FLAG1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SKEW_OFFSET_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SKEW_OFFSET_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SKEW_OFFSET_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SKEW_OFFSET_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SKEW_OFFSET_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SKEW_OFFSET_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SKEW_OFFSET_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SKEW_OFFSET_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_OFFSET_CL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_OFFSET_CL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_CALIBRATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_CALIBRATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_SPECS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_SPECS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_COMP, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_COMP=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_MIPI_IN_SHORT, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_MIPI_IN_SHORT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_LANE_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_LANE_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_VER_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_VER_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_NB_DATA_LANES, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_NB_DATA_LANES=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_IMG_UNPACKING_FORMAT, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_IMG_UNPACKING_FORMAT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_WAIT_AFTER_PACKET_END, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_WAIT_AFTER_PACKET_END=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_MULTIPLE_OF_5_HSYNC_EXTENSION_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_MULTIPLE_OF_5_HSYNC_EXTENSION_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_MULTIPLE_OF_5_HSYNC_EXTENSION_PADDING_DATA, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_MULTIPLE_OF_5_HSYNC_EXTENSION_PADDING_DATA=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_CHARACTERIZATION_MODE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_CHARACTERIZATION_MODE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_BYTE2PIXEL_READ_TH, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_BYTE2PIXEL_READ_TH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_VIRTUAL_CHANNEL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_VIRTUAL_CHANNEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_DATA_TYPE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_DATA_TYPE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_FRAME_NUMBER, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_FRAME_NUMBER=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_LINE_NUMBER, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_LINE_NUMBER=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_DATA_FIELD, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_DATA_FIELD=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_WORD_COUNT, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_WORD_COUNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_ECC_ERROR_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_ECC_ERROR_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_RX_DFV, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_RX_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_PIX_POS, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_PIX_POS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_LINE_POS, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_LINE_POS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_PIX_CNT, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_PIX_CNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_LINE_CNT, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_LINE_CNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_FRAME_CNT, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_FRAME_CNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_ITPOINT_DFV, &read_data);
	pr_info("[CAM]YUSHAN_ITPOINT_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_AUTO_RUN, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_AUTO_RUN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_LINE_LENGTH, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_LINE_LENGTH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_FRAME_LENGTH, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_FRAME_LENGTH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_ERROR_LINES_EOF_GAP, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_ERROR_LINES_EOF_GAP=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_0, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_1, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_2, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_3, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_4, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_5, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_6, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_7, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_8, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_8=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_9, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_9=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_10, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_10=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_11, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_11=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_12, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_12=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_13, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_13=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_WC_DI_14, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_WC_DI_14=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_IDP_GEN_DFV, &read_data);
	pr_info("[CAM]YUSHAN_IDP_GEN_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_VERSION_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_VERSION_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLORBAR_WIDTH_BY4_M1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLORBAR_WIDTH_BY4_M1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_0, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_5, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_6, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_7, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_VAL_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_0, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_5, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_6, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_7, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_IGNORE_ERR_CNT_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_0, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_5, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_6, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_7, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERRVAL_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_0, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_5, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_6, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_7, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_COLOR_BAR_ERR_POS_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_RX_DTCHK_DFV, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_RX_DTCHK_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_PATTERN_TYPE_REQ, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_PATTERN_TYPE_REQ=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_TPAT_DATA_RG, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_TPAT_DATA_RG=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_TPAT_DATA_BG, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_TPAT_DATA_BG=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_TPAT_HCUR_WP, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_TPAT_HCUR_WP=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_TPAT_VCUR_WP, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_TPAT_VCUR_WP=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_PATTERN_GEN_PATTERN_TYPE_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_PATTERN_GEN_PATTERN_TYPE_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_ENABLE_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_ENABLE_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_MODE_REQ, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_MODE_REQ=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_DCPX_MODE_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_DCPX_MODE_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_CPX_CTRL_REQ, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_CPX_CTRL_REQ=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_CPX_MODE_REQ, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_CPX_MODE_REQ=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_CPX_CTRL_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_CPX_CTRL_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_CPX_MODE_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_CPX_MODE_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_PIX_WIDTH, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_PIX_WIDTH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_GROUPED_PARAMETER_HOLD, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_GROUPED_PARAMETER_HOLD=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_EOF_INT_EN, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_EOF_INT_EN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_SMIA_FM_EOF_INT_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_SMIA_FM_EOF_INT_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_P2W_FIFO_WR_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_P2W_FIFO_WR_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_P2W_FIFO_WR_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_P2W_FIFO_WR_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_P2W_FIFO_RD_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_P2W_FIFO_RD_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_P2W_FIFO_RD_STATUS, &read_data);
	pr_info("[CAM]YUSHAN_P2W_FIFO_RD_STATUS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_WRAPPER_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_WRAPPER_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_WRAPPER_THRESH, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_WRAPPER_THRESH=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_WRAPPER_CHAR_EN, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_WRAPPER_CHAR_EN=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VERSION_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VERSION_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_NUMBER_OF_LANES, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_NUMBER_OF_LANES=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LANE_MAPPING, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LANE_MAPPING=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_INTERPACKET_DELAY, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_INTERPACKET_DELAY=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_STATUS_LINE_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_STATUS_LINE_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_STATUS_LINE_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_STATUS_LINE_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VC_CTRL_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VC_CTRL_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VC_CTRL_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VC_CTRL_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VC_CTRL_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VC_CTRL_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_VC_CTRL_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_VC_CTRL_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_FRAME_NO_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_FRAME_NO_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_FRAME_NO_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_FRAME_NO_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_FRAME_NO_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_BYTE_COUNT, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_BYTE_COUNT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_CURRENT_DATA_IDENTIFIER, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_CURRENT_DATA_IDENTIFIER=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DFV, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_0, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_1, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_2, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_3, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_4, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_4, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_4, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_5, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_5, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_5, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_5=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_6, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_6, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_6, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_6=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_7, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_7, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_7, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_7=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_8, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_8=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_8, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_8=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_8, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_8=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_9, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_9=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_9, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_9=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_9, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_9=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_10, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_10=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_10, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_10=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_10, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_10=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_11, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_11=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_11, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_11=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_11, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_11=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_12, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_12=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_12, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_12=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_12, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_12=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_13, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_13=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_13, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_13=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_13, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_13=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_14, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_14=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_14, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_14=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_14, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_14=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_PACKET_SIZE_15, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_PACKET_SIZE_15=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_DI_INDEX_CTRL_15, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_DI_INDEX_CTRL_15=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_CSI2_TX_LINE_NO_15, &read_data);
	pr_info("[CAM]YUSHAN_CSI2_TX_LINE_NO_15=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_UIX4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_UIX4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_SWAP_PINS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_SWAP_PINS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_INVERT_HS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_INVERT_HS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_STOP_STATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_STOP_STATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_FORCE_TX_MODE_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_FORCE_TX_MODE_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_ULP_STATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_ULP_STATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_ULP_EXIT, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_ULP_EXIT=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_ESC_DL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_ESC_DL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_HSTX_SLEW, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_HSTX_SLEW=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_SKEW, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_SKEW=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_CL, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_CL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_DL1, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_DL1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_DL2, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_DL2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_DL3, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_DL3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_GPIO_DL4, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_GPIO_DL4=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_SPECS, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_SPECS=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_SLEW_RATE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_SLEW_RATE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_TEST_RESERVED, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_TEST_RESERVED=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_TCLK_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_TCLK_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_MIPI_TX_TCLK_POST_DELAY, &read_data);
	pr_info("[CAM]YUSHAN_MIPI_TX_TCLK_POST_DELAY=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_BYPASS_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_BYPASS_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_BYPASS_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_BYPASS_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_BYPASS_LSTART_LEVEL, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_BYPASS_LSTART_LEVEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_BYPASS_LSTOP_LEVEL, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_BYPASS_LSTOP_LEVEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_MATCH0, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_MATCH0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_MATCH1, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_MATCH1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_MATCH2, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_MATCH2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_BYPASS_MATCH3, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_BYPASS_MATCH3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_DXO_LSTART_LEVEL, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_DXO_LSTART_LEVEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LINE_FILTER_DXO_LSTOP_LEVEL, &read_data);
	pr_info("[CAM]YUSHAN_LINE_FILTER_DXO_LSTOP_LEVEL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_MATCH0, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_MATCH0=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_MATCH1, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_MATCH1=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_MATCH2, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_MATCH2=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_DTFILTER_DXO_MATCH3, &read_data);
	pr_info("[CAM]YUSHAN_DTFILTER_DXO_MATCH3=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_PRE_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_PRE_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_PRE_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_PRE_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_PRE_DXO_AUTOMATIC_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_PRE_DXO_AUTOMATIC_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_PRE_DXO_H_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_PRE_DXO_H_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_DFV, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_H_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_H_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_PRE_DXO_READ_START, &read_data);
	pr_info("[CAM]YUSHAN_LBE_PRE_DXO_READ_START=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_POST_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_POST_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_POST_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_POST_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_POST_DXO_AUTOMATIC_CONTROL, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_POST_DXO_AUTOMATIC_CONTROL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_EOF_RESIZE_POST_DXO_H_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_EOF_RESIZE_POST_DXO_H_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_MIN_INTERLINE, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_MIN_INTERLINE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_OUT_BURST_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_OUT_BURST_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_LINE_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_LINE_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LECCI_BYPASS_CTRL, &read_data);
	pr_info("[CAM]YUSHAN_LECCI_BYPASS_CTRL=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_ENABLE, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_ENABLE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_VERSION, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_VERSION=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_DFV, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_DFV=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_H_SIZE, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_H_SIZE=%x\n", read_data);
	rawchip_spi_read_2B2B(YUSHAN_LBE_POST_DXO_READ_START, &read_data);
	pr_info("[CAM]YUSHAN_LBE_POST_DXO_READ_START=%x\n", read_data);

}

#define YUSHAN_DOP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_DOP_BASE_ADDR, 1, (uint8_t *)(&udwSpiData)); CDBG("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_PDP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_PDP_BASE_ADDR, 1, (uint8_t *)(&udwSpiData)); CDBG("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_DPP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_DPP_BASE_ADDR-0x8000, 1, (uint8_t *)(&udwSpiData)); CDBG("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
void Yushan_dump_Dxo(void)
{
	int i;
	uint32_t	udwSpiData;
	uint32_t	udwSpiBaseIndex;
	uint8_t target_data;
	uint32_t udwDxoBaseAddress;
	int print_data;

	pr_info("[CAM] %s: Start\n", __func__);

	CDBG("[CAM] %s: **** DXO DOP CODE/CLIB CHECK ****\n", __func__);
	for (i = 0, print_data = 0; i < p_yushan_regs->dopcode_size; i++) {
		YUSHAN_DOP_REGISTER_CHECK(p_yushan_regs->dopcode_first_addr+i);
		target_data = *((uint8_t *)p_yushan_regs->dopcode+i);
		if (udwSpiData != target_data && (print_data <= 10 || print_data % 1000 == 0)) {
			pr_err("Unmatching DOP code addr=%x data=%x target_data=%x\n",
				p_yushan_regs->dopcode_first_addr+i, udwSpiData, target_data);
			print_data++;
		}
	}
	for (i = 0, print_data = 0; i < p_yushan_regs->dopclib_size; i++) {
		YUSHAN_DOP_REGISTER_CHECK(p_yushan_regs->dopclib_first_addr+i);
		target_data = *((uint8_t *)p_yushan_regs->dopclib+i);
		if (udwSpiData != target_data && (print_data <= 10 || print_data % 1000 == 0)) {
			pr_err("Unmatching DOP clib addr=%x data=%x target_data=%x\n",
				p_yushan_regs->dopclib_first_addr+i, udwSpiData, target_data);
			print_data++;
		}
	}

	CDBG("[CAM] %s: **** DXO DPP CODE/CLIB CHECK ****\n", __func__);
	udwSpiBaseIndex = 0x010000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	udwDxoBaseAddress=(0x8000 + DXO_DPP_BASE_ADDR) - udwSpiBaseIndex; 
	for (i = 0, print_data = 0; i < p_yushan_regs->dppcode_size; i++) {
		YUSHAN_DPP_REGISTER_CHECK(p_yushan_regs->dppcode_first_addr+udwDxoBaseAddress+i-DXO_DPP_BASE_ADDR+0x8000);
		target_data = *((uint8_t *)p_yushan_regs->dppcode+i);
		if (udwSpiData != target_data && (print_data <= 10 || print_data % 1000 == 0)) {
			pr_err("Unmatching DPP code addr=%x data=%x target_data=%x\n",
				p_yushan_regs->dppcode_first_addr+i, udwSpiData, target_data);
			print_data++;
		}
	}
	udwSpiBaseIndex = DXO_DPP_BASE_ADDR + p_yushan_regs->dppclib_first_addr; 
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	udwDxoBaseAddress = ((DXO_DPP_BASE_ADDR + p_yushan_regs->dppclib_first_addr) - udwSpiBaseIndex) + 0x8000; 
	for (i = 0, print_data = 0; i < p_yushan_regs->dppclib_size; i++) {
		YUSHAN_DPP_REGISTER_CHECK(udwDxoBaseAddress+i-DXO_DPP_BASE_ADDR+0x8000);
		target_data = *((uint8_t *)p_yushan_regs->dppclib+i);
		if (udwSpiData != target_data && (print_data <= 10 || print_data % 1000 == 0)) {
			pr_err("Unmatching DPP clib addr=%x data=%x target_data=%x\n",
				p_yushan_regs->dppclib_first_addr+i, udwSpiData, target_data);
			print_data++;
		}
	}
	udwSpiBaseIndex = 0x08000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	CDBG("[CAM] %s: **** DXO PDP CODE/CLIB CHECK ****\n", __func__);
	for (i = 0, print_data = 0; i < p_yushan_regs->pdpcode_size; i++) {
		YUSHAN_PDP_REGISTER_CHECK(p_yushan_regs->pdpcode_first_addr+i);
		target_data = *((uint8_t *)p_yushan_regs->pdpcode+i);
		if (udwSpiData != target_data && (print_data <= 10 || print_data % 1000 == 0)) {
			pr_err("Unmatching PDP code addr=%x data=%x target_data=%x\n",
				p_yushan_regs->pdpcode_first_addr+i, udwSpiData, target_data);
			print_data++;
		}
	}
	for (i = 0, print_data = 0; i < p_yushan_regs->pdpclib_size; i++) {
		YUSHAN_PDP_REGISTER_CHECK(p_yushan_regs->pdpclib_first_addr+i);
		target_data = *((uint8_t *)p_yushan_regs->pdpclib+i);
		if (udwSpiData != target_data && (print_data <= 10 || print_data % 1000 == 0)) {
			pr_err("Unmatching PDP clib addr=%x data=%x target_data=%x\n",
				p_yushan_regs->pdpclib_first_addr+i, udwSpiData, target_data);
			print_data++;
		}
	}

	pr_info("[CAM] %s: End\n", __func__);
}


