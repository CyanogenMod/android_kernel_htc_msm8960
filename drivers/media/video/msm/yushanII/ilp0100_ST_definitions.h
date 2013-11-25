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

#ifndef ILP0100_ST_DEFINITIONS_H_
#define ILP0100_ST_DEFINITIONS_H_
#include <linux/types.h>
#include "ilp0100_customer_platform_types.h"




#define ILP0100_MAJOR_VERSION_NUMBER 0
#define ILP0100_MINOR_VERSION_NUMBER 19

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#define INTR_PIN_0 	0
#define INTR_PIN_1	1

#define ILP0100_CLOCK	0
#define EXTERNAL_CLOCK	1

#define HDR_NONE	0
#define STAGGERED	1

#define NORMAL_OR_HDR_SHORT	0
#define HDR_LONG			1

#define SENSOR_0	0
#define SENSOR_1	1
#define SENSOR_NONE	2

#define RAW_8	8
#define RAW_10	10
#define RAW_12  12

#define GR 0
#define RG 1
#define BG 2
#define GB 3

#define ORIENTATION_NORMAL	0
#define ORIENTATION_MIRROR	1
#define ORIENTATION_VFLIP	2
#define ORIENTATION_VFLIP_AND_MIRROR	ORIENTATION_MIRROR|ORIENTATION_VFLIP


#define INTR_SPI_COMMS_READY				0x00000001
#define INTR_IDLE_COMPLETE					0x00000002
#define INTR_ISP_STREAMING					0x00000004
#define INTR_MODE_CHANGE_COMPLETE			0x00000008
#define INTR_START_OF_FRAME					0x00000020
#define INTR_END_OF_FRAME					0x00000040
#define INTR_MODE_CHANGE_REQUEST			0x00000080
#define INTR_LONGEXP_GLACE_STATS_READY		0x00000100
#define INTR_LONGEXP_HISTOGRAM_STATS_READY	0x00000200
#define INTR_SHORTEXP_GLACE_STATS_READY		0x00010000
#define INTR_SHORTEXP_HISTOGRAM_STATS_READY	0x00020000
#define INTR_ITPOINT_LONG					0x00200000
#define INTR_ITPOINT_SHORT_OR_NORMAL		0x00400000
#define INTR_ITPOINT_MERGED					0x00800000

#define INTR_RXPHY_ERROR			0x00000001
#define INTR_CSI2RX_ERROR 			0x00000002
#define INTR_SMIA_UNPACK_ERROR 		0x00000004
#define INTR_P2W_FIFO_ERROR 		0x00000008
#define INTR_LTYPE_RETAG_ERROR 		0x00000010
#define INTR_HDR_MERGE_ERROR 		0x00000020
#define INTR_TONE_MAP_ERROR 		0x00000040
#define INTR_MISC_SYS_ERROR 		0x00000080
#define INTR_CSI2TX_ERROR 			0x00000100
#define INTR_TXPHY_ERROR 			0x00000200
#define INTR_INCORRECT_WORD_COUNT 	0x00000400
#define INCORRECT_LSCP2P_BUFFER 	0x00000800

#define	ENABLE_NO_INTR					0x0000
#define ENABLE_STATISTICS_INTR				INTR_LONGEXP_GLACE_STATS_READY|INTR_SHORTEXP_GLACE_STATS_READY
#define ENABLE_RECOMMENDED_DEBUG_INTR_PIN0 	ENABLE_STATISTICS_INTR
#define ENABLE_HTC_INTR	ENABLE_STATISTICS_INTR|\
							INTR_SPI_COMMS_READY|\
							INTR_IDLE_COMPLETE|\
							INTR_MODE_CHANGE_COMPLETE|\
							INTR_START_OF_FRAME|\
							INTR_END_OF_FRAME
#define ENABLE_ALL_ERROR_INTR				INTR_RXPHY_ERROR|INTR_CSI2RX_ERROR|INTR_SMIA_UNPACK_ERROR|INTR_P2W_FIFO_ERROR|INTR_LTYPE_RETAG_ERROR|INTR_HDR_MERGE_ERROR|INTR_TONE_MAP_ERROR|INTR_MISC_SYS_ERROR|INTR_CSI2TX_ERROR|INTR_TXPHY_ERROR|INTR_INCORRECT_WORD_COUNT|INCORRECT_LSCP2P_BUFFER
#define ENABLE_RECOMMENDED_DEBUG_INTR_PIN1	ENABLE_ALL_ERROR_INTR





#define BYPASS_NO_BYPASS			0x0000
#define BYPASS_CHANNEL_OFFSET 		0x0001
#define BYPASS_DEFCOR				0x0002
#define BYPASS_STATS				0x0004
#define BYPASS_HDR_MERGE_KEEP_LONG	0x0008
#define BYPASS_HDR_MERGE_KEEP_SHORT	0x0010
#define BYPASS_TONE_MAPPING			0x0020  
#define BYPASS_LSC					0x0040
#define BYPASS_ISP					0x0080
#define ENABLE_CHANNEL_OFFSET_ONLY BYPASS_DEFCOR|BYPASS_STATS|BYPASS_HDR_MERGE_KEEP_SHORT|BYPASS_TONE_MAPPING|BYPASS_LSC
#define BYPASS_ALL_IPS			BYPASS_CHANNEL_OFFSET|BYPASS_DEFCOR|BYPASS_STATS|BYPASS_HDR_MERGE_KEEP_SHORT|BYPASS_TONE_MAPPING|BYPASS_LSC
#define FORCE_LSC_GRIDS0			0x0100
#define FORCE_LSC_GRIDS1			0x0200
#define FORCE_LSC_GRIDS2			0x0400
#define FORCE_LSC_GRIDS3			0x0800


#define TEST_NO_TEST_MODE		0x00
#define TEST_UNIFORM_GREY		0x01
#define	TEST_COLORBAR			0x02
#define TEST_PN29				0x04
#define TEST_SOLID				0x06
#define TEST_ADD_STATUS_LINES	0x08	




typedef uint16_t ilp0100_error;



typedef	struct{
	
	
	uint8_t 	NumberOfLanes;
	
	
	uint16_t 	uwPixelFormat;
	
	
	uint32_t 	BitRate;
	uint32_t 	ExternalClock;
	uint8_t 	ClockUsed;
	bool_t		UsedSensorInterface;
	uint32_t	IntrEnablePin1; 
	uint32_t	IntrEnablePin2; 
} Ilp0100_structInit;

typedef	struct{
	uint8_t 	*pIlp0100Firmware;
	uint32_t	Ilp0100FirmwareSize;
	uint8_t 	*pIlp0100SensorGenericCalibData;
	uint32_t	Ilp0100SensorGenericCalibDataSize;
	uint8_t 	*pIlp0100SensorRawPart2PartCalibData;
	uint32_t	Ilp0100SensorRawPart2PartCalibDataSize;
} Ilp0100_structInitFirmware;

typedef struct{
	
	uint16_t	FullActivePixels;			
	
	uint16_t	MinLineLength;
	
	uint16_t	FullActiveLines;	
	
    uint8_t     PixelOrder;
    uint8_t		StatusNbLines;
} Ilp0100_structSensorParams;

typedef struct{
	uint16_t	ActiveLineLengthPixels;
	uint16_t	ActiveFrameLengthLines;
	uint16_t	LineLengthPixels;
	uint16_t 	FrameLengthLines;
	bool_t		StatusLinesOutputted;
	uint16_t	StatusLineLengthPixels;
	uint16_t	StatusNbLines;
	uint16_t	MinInterframe;
	bool_t		AutomaticFrameParamsUpdate;
	uint8_t 	HDRMode;
	uint8_t 	uwOutputPixelFormat;
	uint8_t 	ImageOrientation;
	uint32_t 	HScaling; 
	uint32_t 	VScaling; 
	uint8_t 	Binning;
	uint16_t 	Hoffset; 
	uint16_t 	Voffset; 

} Ilp0100_structFrameFormat;


typedef enum{
  OFF = 0,
  SINGLET_ONLY = 1,
  COUPLET_ONLY = 2,
  SINGLET_AND_COUPLET =3
}Ilp0100_enumDefcorMode;

typedef struct{
  Ilp0100_enumDefcorMode Mode;
} Ilp0100_structDefcorConfig;

typedef struct{
  uint8_t SingletThreshold;     
  uint8_t CoupletThreshold; 
  uint8_t BlackStrength;    
  uint8_t WhiteStrength;    
} Ilp0100_structDefcorParams;


typedef struct{
  bool_t Enable;
} Ilp0100_structChannelOffsetConfig;

typedef struct{
  uint16_t SensorPedestalGreenRed;
  uint16_t SensorPedestalRed;
  uint16_t SensorPedestalBlue;
  uint16_t SensorPedestalGreenBlue;
} Ilp0100_structChannelOffsetParams;


typedef struct{
  bool_t Enable;
  uint16_t RoiHStart;                                                             
  uint16_t RoiVStart;                                                             
  uint16_t RoiHBlockSize;      
  uint16_t RoiVBlockSize;      
  uint8_t RoiHNumberOfBlocks; 
  uint8_t RoiVNumberOfBlocks; 
  uint8_t SaturationLevelRed; 
  uint8_t SaturationLevelGreen;
  uint8_t SaturationLevelBlue; 
} Ilp0100_structGlaceConfig;

typedef enum{
  HIST_OFF = 0,
  REDGREENBLUE = 1,
  LUMINANCE = 2
}Ilp0100_enumHistMode;

typedef struct{
  Ilp0100_enumHistMode Mode;
  uint16_t RoiXOffset;
  uint16_t RoiYOffset;
  uint16_t RoiXSize;
  uint16_t RoiYSize;
  uint8_t YConversionFactorGreenRed; 
  uint8_t YConversionFactorRed; 
  uint8_t YConversionFactorBlue; 
  uint8_t YConversionFactorGreenBlue; 
} Ilp0100_structHistConfig;

typedef struct{
	uint8_t 	GlaceStatsRedMean[48];
	uint8_t 	GlaceStatsGreenMean[48];
	uint8_t 	GlaceStatsBlueMean[48];
	uint16_t    GlaceStatsNbOfSaturatedPixels[48];
} Ilp0100_structGlaceStatsData;


typedef struct{
	uint32_t	HistStatsRedBin[64];
	uint32_t 	HistStatsGreenBin[64];
	uint32_t	HistStatsBlueBin[64];
	uint16_t	HistStatsRedDarkestBin;
	uint32_t	HistStatsRedDarkestCount;
	uint16_t 	HistStatsRedBrightestBin;
	uint32_t 	HistStatsRedBrightestCount;
	uint16_t	HistStatsRedHighestBin;
	uint32_t	HistStatsRedHighestCount;
	uint16_t	HistStatsGreenDarkestBin;
	uint32_t	HistStatsGreenDarkestCount;
	uint16_t 	HistStatsGreenBrightestBin;
	uint32_t 	HistStatsGreenBrightestCount;
	uint16_t	HistStatsGreenHighestBin;
	uint32_t	HistStatsGreenHighestCount;
	uint16_t	HistStatsBlueDarkestBin;
	uint32_t	HistStatsBlueDarkestCount;
	uint16_t 	HistStatsBlueBrightestBin;
	uint32_t 	HistStatsBlueBrightestCount;
	uint16_t	HistStatsBlueHighestBin;
	uint32_t	HistStatsBlueHighestCount;
} Ilp0100_structHistStatsData;

typedef enum{
  ON = 0,
  OUTPUT_LONG_ONLY  = 1,
  OUTPUT_SHORT_ONLY = 2
} Ilp0100_enumHdrMergeMode;

typedef struct{
	Ilp0100_enumHdrMergeMode Mode;
} Ilp0100_structHdrMergeConfig;

typedef enum{
  USE_KNEE_POINTS = 0,
  USE_AVERAGE  = 1,
  USE_AVERAGE_AND_KNEE_POINTS = 2
} Ilp0100_enumHdrMergeMethod;

typedef enum{
MAX_MACRO_PIXEL = 0,
LUMA = 1
} Ilp0100_enumHdrMergeImageCodes;

typedef struct{
 Ilp0100_enumHdrMergeMethod Method;
 Ilp0100_enumHdrMergeImageCodes ImageCodes;
} Ilp0100_structHdrMergeParams;

typedef struct{
  bool_t Enable;
} Ilp0100_structClsConfig;

typedef struct{
  uint8_t 	BowlerCornerGain; 
  
  uint16_t 	ColorTempKelvin;
} Ilp0100_structClsParams;

typedef struct{
  bool_t Enable;
  bool_t UserDefinedCurveEnable;
} Ilp0100_structToneMappingConfig; 

typedef struct{
  uint8_t Strength; 
  uint16_t UserDefinedCurve[256];
} Ilp0100_structToneMappingParams;

typedef struct{
	uint16_t    ExposureTime;
	uint16_t    AnalogGainCodeGreen;
	uint16_t    AnalogGainCodeRed;
	uint16_t    AnalogGainCodeBlue;
	uint16_t    DigitalGainCodeGreen;
	uint16_t    DigitalGainCodeRed;
	uint16_t    DigitalGainCodeBlue;
} Ilp0100_structFrameParams;






#endif 
