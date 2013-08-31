#ifndef _YUSHAN_API_H_
#define _YUSHAN_API_H_


#ifndef WIN32
#include <media/linux_rawchip.h>
#include "yushan_registermap.h"
#include "DxODOP_regMap.h"
#include "DxODPP_regMap.h"
#include "DxOPDP_regMap.h"
#include "rawchip_spi.h"
#endif
#ifdef __cplusplus
extern "C"{
#endif   





#define YUSHAN_DEBUG 0
#if YUSHAN_DEBUG
	#ifdef WIN32
		#include <stdio.h>
		extern	FILE	*fLogFile;
		extern	FILE	*HtmlFileLog;
		#define __func__  __FUNCTION__
		
		#define DEBUGLOG(...)  fprintf(HtmlFileLog, __VA_ARGS__); fprintf(HtmlFileLog, "<hr>\n")
		
	#else
		#define DEBUGLOG  pr_info
	#endif
#else
	#define DEBUGLOG(...)	
#endif

#define YUSHAN_VERBOSE 0
#if YUSHAN_VERBOSE
	#ifdef WIN32
		#define VERBOSELOG(...)   fprintf(HtmlFileLog, __VA_ARGS__)
	#else
		#define VERBOSELOG  pr_info
	#endif
#else
	#define VERBOSELOG(...)	
#endif

#ifdef WIN32
	#if YUSHAN_DEBUG == 0
		#include <stdio.h>
		extern	FILE	*fLogFile;
		extern	FILE	*HtmlFileLog;
		
		#define __func__  __FUNCTION__
	#endif
	#define ERRORLOG(...)   fprintf(HtmlFileLog, __VA_ARGS__)
#else
	#define ERRORLOG  pr_err
#endif



typedef unsigned char bool_t;
#ifdef WIN32
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif


#define API_MAJOR_VERSION					14
#define API_MINOR_VERSION				2

#define TRUE								1
#define SUCCESS								1
#define FALSE								0
#define FAILURE								0
#define MIPI_CLK_LANE					  1
#define MIPI_DATA_LANE1					2
#define MIPI_DATA_LANE2					3
#define MIPI_DATA_LANE3					4
#define MIPI_DATA_LANE4					5
#define MIPI_STOP_STATE					1
#define MIPI_ULP_STATE					2
#define MIPI_ACTIVE_STATE				3



#define RAW8								8
#define	RAW10								10
#define	RAW10_8								108		


#define IDP_GEN_PIX_WIDTH					10
#define DXO_CLK_LIMIT						0x12C0000	
#define SYS_CLK_LIMIT						0x0C80000	

#define PLL_CLK_INDEX				    	0
#define IDIV_INDEX				    		1
#define ODIV_INDEX		          			2
#define LOOP_INDEX	     					3

#define DXO_MIN_LINEBLANKING				150
#define DXO_MIN_LINELENGTH					1000
#define DXO_ACTIVE_FRAMELENGTH				500000
#define DXO_FULL_FRAMELENGTH				900000


#define DXO_PDP_HW_MICROCODE_ID				((DxOPDP_dfltVal_ucode_id_15_8<<8)|			\
												(DxOPDP_dfltVal_hw_id_7_0<<16)|(DxOPDP_dfltVal_hw_id_15_8<<24))
#define DXO_DOP_HW_MICROCODE_ID				((DxODOP_dfltVal_ucode_id_15_8<<8)|			\
												(DxODOP_dfltVal_hw_id_7_0<<16)|(DxODOP_dfltVal_hw_id_15_8<<24))
#define DXO_DPP_HW_MICROCODE_ID				((DxODPP_dfltVal_ucode_id_15_8<<8)|			\
												(DxODPP_dfltVal_hw_id_7_0<<16)|(DxODPP_dfltVal_hw_id_15_8<<24))

#define DXO_PDP_CALIBRATIONCODE_ID			((DxOPDP_dfltVal_calib_id_0_7_0)|(DxOPDP_dfltVal_calib_id_1_7_0<<8)|	\
												(DxOPDP_dfltVal_calib_id_2_7_0<<16)|(DxOPDP_dfltVal_calib_id_3_7_0<<24))
#define DXO_DOP_CALIBRATIONCODE_ID			((DxODOP_dfltVal_calib_id_0_7_0)|(DxODOP_dfltVal_calib_id_1_7_0<<8)|	\
												(DxODOP_dfltVal_calib_id_2_7_0<<16)|(DxODOP_dfltVal_calib_id_3_7_0<<24))
#define DXO_DPP_CALIBRATIONCODE_ID			((DxODPP_dfltVal_calib_id_0_7_0)|(DxODPP_dfltVal_calib_id_1_7_0<<8)|	\
												(DxODPP_dfltVal_calib_id_2_7_0<<16)|(DxODPP_dfltVal_calib_id_3_7_0<<24))




#define ADD_INTR_TO_LIST							0x01
#define	DEL_INTR_FROM_LIST							0x00

#define	INTERRUPT_PAD_0								0x00
#define	INTERRUPT_PAD_1								0x01

#define TOTAL_INTERRUPT_SETS						13
#define TOTAL_INTERRUPT_COUNT						85
#define INTERRUPT_SET_SIZE							0x18
#define FALSE_ALARM									0xFF

#define YUSHAN_INTR_BASE_ADDR						0x0C00
#define	YUSHAN_OFFSET_INTR_STATUS					0x00	
#define	YUSHAN_OFFSET_INTR_ENABLE_STATUS			0x04
#define YUSHAN_OFFSET_INTR_STATUS_CLEAR				0x08
#define YUSHAN_OFFSET_INTR_DISABLE					0x10
#define YUSHAN_OFFSET_INTR_ENABLE					0x14

#define DXO_DOP_BASE_ADDR							0x8000
#define DXO_DPP_BASE_ADDR							0x10000
#define DXO_PDP_BASE_ADDR							0x6000


#define DFT_RESET									1
#define T1_DMA										2
#define CSI2_RX										3
#define IT_POINT									4
#define IDP_GEN										5
#define MIPI_RX_DTCHK								6
#define SMIA_PATTERN_GEN							7
#define SMIA_DCPX									8
#define P2W_FIFO_WR									9
#define P2W_FIFO_RD									10
#define CSI2_TX_WRAPPER								11
#define CSI2_TX										12
#define LINE_FILTER_BYPASS							13
#define DT_FILTER_BYPASS							14
#define LINE_FILTER_DXO								15
#define DT_FILTER_DXO								16
#define EOF_RESIZE_PRE_DXO							17
#define LBE_PRE_DXO									18
#define EOF_RESIZE_POST_DXO							19
#define LECCI_RESET									20
#define LBE_POST_DXO								21

#define TOTAL_MODULES								21



#define EVENT_CSI2RX_ECC_ERR						1
#define EVENT_CSI2RX_CHKSUM_ERR						2
#define EVENT_CSI2RX_SHORTPACKET					3
#define EVENT_CSI2RX_SYNCPULSE_MISSED				4

#define EVENT_PDP_BOOT							    5
#define EVENT_PDP_EOF_EXECCMD				  		6
#define EVENT_DXOPDP_EOP						  	7
#define EVENT_DXOPDP_NEWFRAMECMD_ACK				8
#define EVENT_DXOPDP_NEWFRAMEPROC_ACK				9
#define EVENT_DXOPDP_NEWFRAME_ERR					10
	
#define EVENT_DPP_BOOT							  	11
#define EVENT_DPP_EOF_EXECCMD						12
#define EVENT_DXODPP_EOP							13
#define EVENT_DXODPP_NEWFRAMECMD_ACK				14
#define EVENT_DXODPP_NEWFRAMEPROC_ACK				15
#define EVENT_DXODPP_NEWFRAME_ERR					16

#define EVENT_DOP7_BOOT							  	17
#define EVENT_DOP7_EOF_EXECCMD						18
#define EVENT_DXODOP7_EOP						  	19
#define EVENT_DXODOP7_NEWFRAMECMD_ACK				20
#define EVENT_DXODOP7_NEWFRAMEPROC_ACK				21
#define EVENT_DXODOP7_NEWFRAME_ERR					22

#define EVENT_CSI2TX_SP_ERR							23
#define EVENT_CSI2TX_LP_ERR							24
#define EVENT_CSI2TX_DATAINDEX_ERR					25
#define EVENT_CSI2TX_FRAMEFINISH					26
#define	EVENT_RX_PHY_ERR_SOT_SOFT_DL4				27
#define	EVENT_RX_PHY_ERR_SOT_HARD_DL4				28
#define	EVENT_RX_PHY_ERR_EOT_DL4					29
#define	EVENT_RX_PHY_ERR_ESC_DL4					30
#define	EVENT_RX_PHY_ERR_CTRL_DL4					31
#define	EVENT_RX_PHY_ULPM_EXIT_DL4					32
#define	EVENT_RX_PHY_ULPM_ENTER_DL4					33

#define	EVENT_RESERVED								34

#define	EVENT_RX_PHY_ERR_SOT_SOFT_DL3				35
#define	EVENT_RX_PHY_ERR_SOT_HARD_DL3				36
#define	EVENT_RX_PHY_ERR_EOT_DL3					37
#define	EVENT_RX_PHY_ERR_ESC_DL3					38
#define	EVENT_RX_PHY_ERR_CTRL_DL3					39
#define	EVENT_RX_PHY_ULPM_EXIT_DL3					40
#define	EVENT_RX_PHY_ULPM_ENTER_DL3					41

#define	EVENT_RESERVED2								42

#define	EVENT_RX_PHY_ERR_SOT_SOFT_DL2				43
#define	EVENT_RX_PHY_ERR_SOT_HARD_DL2				44
#define	EVENT_RX_PHY_ERR_EOT_DL2					45
#define	EVENT_RX_PHY_ERR_ESC_DL2					46
#define	EVENT_RX_PHY_ERR_CTRL_DL2					47
#define	EVENT_RX_PHY_ULPM_EXIT_DL2					48
#define	EVENT_RX_PHY_ULPM_ENTER_DL2					49

#define	EVENT_RX_PHY_ULPM_EXIT_CLK					50

#define	EVENT_RX_PHY_ERR_SOT_SOFT_DL1				51
#define	EVENT_RX_PHY_ERR_SOT_HARD_DL1				52
#define	EVENT_RX_PHY_ERR_EOT_DL1					53
#define	EVENT_RX_PHY_ERR_ESC_DL1					54
#define	EVENT_RX_PHY_ERR_CTRL_DL1					55
#define	EVENT_RX_PHY_ULPM_EXIT_DL1					56
#define	EVENT_RX_PHY_ULPM_ENTER_DL1					57

#define	EVENT_RX_PHY_ULPM_ENTER_CLK					58
#define EVENT_TXPHY_CTRL_ERR_D1						59
#define EVENT_TXPHY_CTRL_ERR_D2						60
#define EVENT_TXPHY_CTRL_ERR_D3						61
#define EVENT_TXPHY_CTRL_ERR_D4						62

#define EVENT_UNMATCHED_IMAGE_SIZE_ERROR			63
#define	EVENT_UNMANAGED_DATA_TYPE					64
#define	PRE_DXO_WRAPPER_PROTOCOL_ERR				65
#define	PRE_DXO_WRAPPER_FIFO_OVERFLOW				66
#define EVENT_BAD_FRAME_DETECTION					67
#define EVENT_TX_DATA_FIFO_OVERFLOW					68
#define EVENT_TX_INDEX_FIFO_OVERFLOW				69
#define EVENT_RX_CHAR_COLOR_BAR_0_ERR				70
#define EVENT_RX_CHAR_COLOR_BAR_1_ERR				71
#define EVENT_RX_CHAR_COLOR_BAR_2_ERR				72
#define EVENT_RX_CHAR_COLOR_BAR_3_ERR				73
#define EVENT_RX_CHAR_COLOR_BAR_4_ERR				74
#define EVENT_RX_CHAR_COLOR_BAR_5_ERR				75
#define EVENT_RX_CHAR_COLOR_BAR_6_ERR				76
#define EVENT_RX_CHAR_COLOR_BAR_7_ERR				77
#define	EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR			78
#define	EVENT_POST_DXO_WRAPPER_FIFO_OVERFLOW		79
#define EVENT_LINESIZE_REPROGRAM_DONE				80
#define EVENT_PLL_STABLE							81
#define EVENT_LDO_STABLE							82
#define EVENT_LINE_POSITION_INTR					83	
#define EVENT_TX_DATA_UNDERFLOW						84	
#define EVENT_TX_INDEX_UNDERFLOW					85	

#define EVENT_FIRST_INDEXFORSET {EVENT_CSI2RX_ECC_ERR, EVENT_PDP_BOOT, EVENT_DPP_BOOT, EVENT_DOP7_BOOT, EVENT_CSI2TX_SP_ERR, EVENT_RX_PHY_ERR_SOT_SOFT_DL4, EVENT_TXPHY_CTRL_ERR_D1, EVENT_UNMATCHED_IMAGE_SIZE_ERROR, EVENT_RX_CHAR_COLOR_BAR_0_ERR, EVENT_POST_DXO_WRAPPER_PROTOCOL_ERR, EVENT_LINESIZE_REPROGRAM_DONE, EVENT_LINE_POSITION_INTR, EVENT_TX_DATA_UNDERFLOW, TOTAL_INTERRUPT_COUNT+1}



#define TIME_5MS				1
#define TIME_10MS				2
#define TIME_20MS				3
#define TIME_50MS				4
#define TIME_100MS				5


#define YUSHAN_FRAME_FORMAT_NORMAL_MODE		0	
#define YUSHAN_FRAME_FORMAT_VF_MODE			1
#define YUSHAN_FRAME_FORMAT_STILL_MODE		2

#define DXO_BOOT_CMD			1


#define RESET_MODULE						1
#define	DERESET_MODULE						0


extern bool_t			gPllLocked;



typedef enum {
	DXO_NO_ERR=100,
	DXO_LINEBLANK_ERR,
	DXO_FULLLINE_ERR, 
	DXO_ACTIVE_FRAMELENGTH_ERR, 
	DXO_FULLFRAMELENGTH_ERR, 
	DXO_FRAMEBLANKING_ERR,
	
	DXO_LOLIMIT_EXCEED_HILIMIT  = 200
}Yushan_DXO_Errors_e;








typedef struct {
	uint16_t 	uwWordcount; 
	
	
	uint8_t 	bDatatype;
	
	uint8_t 	bActiveDatatype;
	
	
	uint8_t 	bSelectStillVfMode;
	
	
	
	
	
	
	
} Yushan_Frame_Format_t;




typedef	struct {
	
	
	uint8_t 	bNumberOfLanes;
	
	
	
	
	uint16_t 	uwPixelFormat;
	
	uint16_t 	uwBitRate; 
	
	uint32_t 	fpExternalClock; 
	uint32_t	fpSpiClock;
	
	uint16_t	uwActivePixels;
	
	uint16_t	uwLineBlankVf;
	
	uint16_t	uwLineBlankStill;
	
	uint16_t	uwLines;
	
	uint16_t	uwFrameBlank;
	
	
	
	
	
	uint8_t		bValidWCEntries; 
	Yushan_Frame_Format_t sFrameFormat[15];
	
	uint8_t		bDxoSettingCmdPerFrame;
	
	bool_t		bUseExternalLDO;
	
	
}Yushan_Init_Struct_t;


typedef struct {
	
	uint16_t	uwActivePixels;			
	
	uint16_t	uwLineBlank;
	
	uint16_t	uwActiveFrameLength;	
	
	uint8_t		bSelectStillVfMode;
	
	uint16_t	uwPixelFormat;

}Yushan_New_Context_Config_t;



typedef	struct {
	uint8_t 	*pDxoPdpRamImage[2];
	
	uint16_t	uwDxoPdpStartAddr;
	
	uint16_t	uwDxoPdpBootAddr;
	// IP Boot address, where first add of Microcode has to be written
	uint16_t 	uwDxoPdpRamImageSize[2];
	
	uint16_t	uwBaseAddrPdpMicroCode[2];
	
	uint8_t 	*pDxoDppRamImage[2];
	
	uint16_t	uwDxoDppStartAddr;
	
	uint16_t	uwDxoDppBootAddr;
	// IP Boot address, where first add of Microcode has to be written
	uint16_t 	uwDxoDppRamImageSize[2];
	
	uint16_t	uwBaseAddrDppMicroCode[2];
	
	uint8_t 	*pDxoDopRamImage[2];
	
	uint16_t	uwDxoDopStartAddr;
	
	uint16_t	uwDxoDopBootAddr;
	// IP Boot address, where first add of Microcode has to be written
	uint16_t 	uwDxoDopRamImageSize[2];
	
	uint16_t	uwBaseAddrDopMicroCode[2];
	
} Yushan_Init_Dxo_Struct_t;



typedef struct {
	
	uint8_t		bDxoConstraints;
	
	uint32_t	udwDxoConstraintsMinValue;
	
	
	

}Yushan_SystemStatus_t;




typedef	struct {
	uint8_t 	bImageOrientation;
	uint16_t 	uwXAddrStart;
	uint16_t 	uwYAddrStart;
	uint16_t 	uwXAddrEnd;
	uint16_t 	uwYAddrEnd;
	uint16_t 	uwXEvenInc;  
	uint16_t 	uwXOddInc;
	uint16_t 	uwYEvenInc;  
	uint16_t 	uwYOddInc;
	uint8_t 	bBinning;    
}Yushan_ImageChar_t;


typedef	struct {
	uint16_t 	uwAnalogGainCodeGR;
	uint16_t 	uwAnalogGainCodeR;
	uint16_t 	uwAnalogGainCodeB;
	uint16_t 	uwPreDigGainGR;
	uint16_t 	uwPreDigGainR;
	uint16_t 	uwPreDigGainB;
	uint16_t 	uwExposureTime;    
	uint8_t	bRedGreenRatio;
	uint8_t	bBlueGreenRatio; 
}Yushan_GainsExpTime_t;



typedef	struct {
	
	uint8_t 	bTemporalSmoothing;   
	uint16_t 	uwFlashPreflashRating;
	uint8_t 	bFocalInfo;
}Yushan_DXO_DPP_Tuning_t;



typedef	struct {
	
	
	uint8_t 	bEstimationMode;
	uint8_t 	bSharpness;
	uint8_t 	bDenoisingLowGain;
	uint8_t 	bDenoisingMedGain;
	uint8_t 	bDenoisingHiGain;
	uint8_t 	bNoiseVsDetailsLowGain;
	uint8_t 	bNoiseVsDetailsMedGain;
	uint8_t 	bNoiseVsDetailsHiGain;
	uint8_t 	bTemporalSmoothing;    
}Yushan_DXO_DOP_Tuning_t;



typedef	struct {
	uint8_t 	bDeadPixelCorrectionLowGain;
	uint8_t 	bDeadPixelCorrectionMedGain;
	uint8_t 	bDeadPixelCorrectionHiGain;
}Yushan_DXO_PDP_Tuning_t;




typedef struct {
	uint8_t		bDxoDopRoiActiveNumber;
}Yushan_DXO_ROI_Active_Number_t;



typedef	struct {
	uint8_t 	bXStart;
	uint8_t 	bYStart;
	uint8_t 	bXEnd;
	uint8_t 	bYEnd;
}Yushan_AF_ROI_t;

typedef struct {
	uint32_t 	udwAfStatsGreen;
	uint32_t 	udwAfStatsRed;
	uint32_t 	udwAfStatsBlue;
	uint32_t 	udwAfStatsConfidence;
}Yushan_AF_Stats_t;



typedef struct {
	uint32_t 	udwDopVersion;
	uint32_t 	udwDppVersion;
	uint32_t 	udwPdpVersion;
	uint32_t 	udwDopCalibrationVersion;
	uint32_t 	udwDppCalibrationVersion;
	uint32_t 	udwPdpCalibrationVersion;
	uint8_t 	bApiMajorVersion;
	uint8_t 	bApiMinorVersion;
}Yushan_Version_Info_t;



#define Yushan_DXO_Sync_Reset_Dereset(bFlagResetOrDereset) 	SPI_Write(YUSHAN_RESET_CTRL+3, 1, &bFlagResetOrDereset)

#ifdef WIN32
bool_t	SPI_Read( uint16_t uwIndex , uint16_t uwCount , uint8_t * pData);
bool_t	SPI_Write( uint16_t uwIndex , uint16_t uwCount , uint8_t * pData);
#endif








bool_t	Yushan_Init_LDO(bool_t	bUseExternalLDO);
bool_t	Yushan_Init_Clocks(Yushan_Init_Struct_t *sInitStruct, Yushan_SystemStatus_t *sSystemStatus, uint32_t *udwIntrMask);
bool_t	Yushan_Init(Yushan_Init_Struct_t * sInitStruct);
bool_t	Yushan_Init_Dxo(Yushan_Init_Dxo_Struct_t * sDxoStruct, bool_t fBypassDxoUpload);

bool_t	Yushan_Update_ImageChar(Yushan_ImageChar_t * sImageChar);
bool_t	Yushan_Update_SensorParameters(Yushan_GainsExpTime_t * sGainsExpInfo);
bool_t Yushan_Update_DxoPdp_TuningParameters(Yushan_DXO_PDP_Tuning_t * sDxoPdpTuning);
bool_t	Yushan_Update_DxoDpp_TuningParameters(Yushan_DXO_DPP_Tuning_t * sDxoDppTuning);
bool_t	Yushan_Update_DxoDop_TuningParameters(Yushan_DXO_DOP_Tuning_t * sDxoDopTuning);
bool_t	Yushan_Update_Commit(uint8_t  bPdpMode, uint8_t  bDppMode, uint8_t  bDopMode);




void	Yushan_AssignInterruptGroupsToPad1(uint16_t	uwAssignITRGrpToPad1);
bool_t	Yushan_Intr_Enable(uint8_t *pIntrMask);
void	Yushan_Intr_Status_Read (uint8_t *bListOfInterrupts, bool_t	fSelect_Intr_Pad);
void	Yushan_Read_IntrEvent(uint8_t bIntrSetID, uint32_t *udwListOfInterrupts);
bool_t	Yushan_Intr_Status_Clear(uint8_t *bListOfInterrupts);
bool_t	Yushan_Check_Pad_For_IntrID(uint8_t	bInterruptId);
void	Yushan_AddnRemoveIDInList(uint8_t bInterruptID, uint32_t *udwListOfInterrupts, bool_t fAddORClear);
bool_t	Yushan_CheckForInterruptIDInList(uint8_t bInterruptID, uint32_t *udwProtoInterruptList);



bool_t Yushan_Get_Version_Information(Yushan_Version_Info_t * sYushanVersionInfo );



bool_t Yushan_Context_Config_Update(Yushan_New_Context_Config_t	*sYushanNewContextConfig);

uint8_t Yushan_GetCurrentStreamingMode(void);
bool_t	Yushan_Swap_Rx_Pins(bool_t fClkLane, bool_t fDataLane1, bool_t fDataLane2, bool_t fDataLane3, bool_t fDataLane4);
bool_t	Yushan_Invert_Rx_Pins(bool_t fClkLane, bool_t fDataLane1, bool_t fDataLane2, bool_t fDataLane3, bool_t fDataLane4);
bool_t	Yushan_Enter_Standby_Mode(void);
bool_t	Yushan_Exit_Standby_Mode(Yushan_Init_Struct_t *sInitStruct);
bool_t	Yushan_Assert_Reset(uint32_t bModuleMask, uint8_t bResetORDeReset);
bool_t  Yushan_Update_DxoDop_Af_Strategy(uint8_t  bAfStrategy);
bool_t	Yushan_AF_ROI_Update(Yushan_AF_ROI_t  *sYushanAfRoi, uint8_t bNumOfActiveRoi);
bool_t	Yushan_PatternGenerator(Yushan_Init_Struct_t *sInitStruct, uint8_t	bPatternReq, bool_t	bDxoBypassForTestPattern);
void	Yushan_DCPX_CPX_Enable(void);


bool_t		Yushan_CheckDxoConstraints(uint32_t udwParameters, uint32_t udwMinLimit, uint32_t fpDxo_Clk, uint32_t fpPixel_Clk, uint16_t uwFullLine, uint32_t * pMinValue);
uint32_t	Yushan_Compute_Pll_Divs(uint32_t fpExternal_Clk, uint32_t fpTarget_PllClk);
uint32_t	Yushan_ConvertTo16p16FP(uint16_t);


bool_t Yushan_Read_AF_Statistics(Yushan_AF_Stats_t* sYushanAFStats, uint8_t	bNumOfActiveRoi, uint16_t *frameIdx);


void	Yushan_DXO_DTFilter_Bypass(void);
void	Yushan_DXO_Lecci_Bypass(void);

void	Yushan_Status_Snapshot(void);

#ifdef __cplusplus
}
#endif   


#endif 
