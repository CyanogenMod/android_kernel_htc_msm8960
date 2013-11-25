/***************************************************************************
 *
 *   SiI9234 - MHL Transmitter Driver
 *
 * Copyright (C) 2011 SiliconImage, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *****************************************************************************/

#define	MHL_DEV_LD_DISPLAY					(0x01 << 0)
#define	MHL_DEV_LD_VIDEO					(0x01 << 1)
#define	MHL_DEV_LD_AUDIO					(0x01 << 2)
#define	MHL_DEV_LD_MEDIA					(0x01 << 3)
#define	MHL_DEV_LD_TUNER					(0x01 << 4)
#define	MHL_DEV_LD_RECORD					(0x01 << 5)
#define	MHL_DEV_LD_SPEAKER					(0x01 << 6)
#define	MHL_DEV_LD_GUI						(0x01 << 7)
#define	MHL_LOGICAL_DEVICE_MAP			(MHL_DEV_LD_AUDIO | MHL_DEV_LD_VIDEO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_GUI)

#define	MHL_VER_MAJOR						(0x01 << 4)
#define	MHL_VER_MINOR						0x02
#define	MHL_VERSION							(MHL_VER_MAJOR | MHL_VER_MINOR)


#define MHL_STATUS_REG_CONNECTED_RDY        0x30
#define MHL_STATUS_REG_LINK_MODE            0x31


#define	MHL_STATUS_DCAP_RDY					BIT_0

#define MHL_STATUS_CLK_MODE_MASK            0x07
#define MHL_STATUS_CLK_MODE_PACKED_PIXEL    0x02
#define MHL_STATUS_CLK_MODE_NORMAL          0x03
#define MHL_STATUS_PATH_EN_MASK             0x08
#define MHL_STATUS_PATH_ENABLED             0x08
#define MHL_STATUS_PATH_DISABLED            0x00
#define MHL_STATUS_MUTED_MASK               0x10

#define MHL_RCHANGE_INT                     0x20
#define MHL_DCHANGE_INT                     0x21

#define	MHL_INT_DCAP_CHG					BIT_0
#define MHL_INT_DSCR_CHG                    BIT_1
#define MHL_INT_REQ_WRT                     BIT_2
#define MHL_INT_GRT_WRT                     BIT_3


enum {
    MHL_MSC_MSG_RCP             = 0x10,     
    MHL_MSC_MSG_RCPK            = 0x11,     
    MHL_MSC_MSG_RCPE            = 0x12,     
    MHL_MSC_MSG_RAP             = 0x20,     
    MHL_MSC_MSG_RAPK            = 0x21,     
};

enum {
	MHL_ACK						= 0x33,	
	MHL_NACK					= 0x34,	
	MHL_ABORT					= 0x35,	
	MHL_WRITE_STAT				= 0x60 | 0x80,	
	MHL_SET_INT					= 0x60,	
	MHL_READ_DEVCAP				= 0x61,	
	MHL_GET_STATE				= 0x62,	
	MHL_GET_VENDOR_ID			= 0x63,	
	MHL_SET_HPD					= 0x64,	
	MHL_CLR_HPD					= 0x65,	
	MHL_SET_CAP_ID				= 0x66,	
	MHL_GET_CAP_ID				= 0x67,	
	MHL_MSC_MSG					= 0x68,	
	MHL_GET_SC1_ERRORCODE		= 0x69,	
	MHL_GET_DDC_ERRORCODE		= 0x6A,	
	MHL_GET_MSC_ERRORCODE		= 0x6B,	
	MHL_WRITE_BURST				= 0x6C,	
	MHL_GET_SC3_ERRORCODE		= 0x6D,	
};

#define        RCPE_NO_ERROR                           0x00
#define        RCPE_INEEFECTIVE_KEY_CODE       0x01
#define        RCPE_BUSY                                       0x02

#define	T_SRC_VBUS_CBUS_TO_STABLE	(200)
#define	T_SRC_WAKE_PULSE_WIDTH_1	(20)
#define	T_SRC_WAKE_PULSE_WIDTH_2	(60)

#define	T_SRC_WAKE_TO_DISCOVER		(500)

#define T_SRC_VBUS_CBUS_T0_STABLE 	(500)

#define	T_SRC_RSEN_DEGLITCH			(100)

#define	T_SRC_RXSENSE_CHK			(400)


enum {
	MHD_RCP_CMD_SELECT          = 0x00,
	MHD_RCP_CMD_UP              = 0x01,
	MHD_RCP_CMD_DOWN            = 0x02,
	MHD_RCP_CMD_LEFT            = 0x03,
	MHD_RCP_CMD_RIGHT           = 0x04,
	MHD_RCP_CMD_RIGHT_UP        = 0x05,
	MHD_RCP_CMD_RIGHT_DOWN      = 0x06,
	MHD_RCP_CMD_LEFT_UP         = 0x07,
	MHD_RCP_CMD_LEFT_DOWN       = 0x08,
	MHD_RCP_CMD_ROOT_MENU       = 0x09,
	MHD_RCP_CMD_SETUP_MENU      = 0x0A,
	MHD_RCP_CMD_CONTENTS_MENU   = 0x0B,
	MHD_RCP_CMD_FAVORITE_MENU   = 0x0C,
	MHD_RCP_CMD_EXIT            = 0x0D,

	
	MHD_RCP_CMD_NUM_0           = 0x20,
	MHD_RCP_CMD_NUM_1           = 0x21,
	MHD_RCP_CMD_NUM_2           = 0x22,
	MHD_RCP_CMD_NUM_3           = 0x23,
	MHD_RCP_CMD_NUM_4           = 0x24,
	MHD_RCP_CMD_NUM_5           = 0x25,
	MHD_RCP_CMD_NUM_6           = 0x26,
	MHD_RCP_CMD_NUM_7           = 0x27,
	MHD_RCP_CMD_NUM_8           = 0x28,
	MHD_RCP_CMD_NUM_9           = 0x29,

	MHD_RCP_CMD_DOT             = 0x2A,
	MHD_RCP_CMD_ENTER           = 0x2B,
	MHD_RCP_CMD_CLEAR           = 0x2C,

	

	MHD_RCP_CMD_CH_UP           = 0x30,
	MHD_RCP_CMD_CH_DOWN         = 0x31,
	MHD_RCP_CMD_PRE_CH          = 0x32,
	MHD_RCP_CMD_SOUND_SELECT    = 0x33,
	MHD_RCP_CMD_INPUT_SELECT    = 0x34,
	MHD_RCP_CMD_SHOW_INFO       = 0x35,
	MHD_RCP_CMD_HELP            = 0x36,
	MHD_RCP_CMD_PAGE_UP         = 0x37,
	MHD_RCP_CMD_PAGE_DOWN       = 0x38,

	

	MHD_RCP_CMD_VOL_UP	        = 0x41,
	MHD_RCP_CMD_VOL_DOWN        = 0x42,
	MHD_RCP_CMD_MUTE            = 0x43,
	MHD_RCP_CMD_PLAY            = 0x44,
	MHD_RCP_CMD_STOP            = 0x45,
	MHD_RCP_CMD_PAUSE           = 0x46,
	MHD_RCP_CMD_RECORD          = 0x47,
	MHD_RCP_CMD_REWIND          = 0x48,
	MHD_RCP_CMD_FAST_FWD        = 0x49,
	MHD_RCP_CMD_EJECT           = 0x4A,
	MHD_RCP_CMD_FWD             = 0x4B,
	MHD_RCP_CMD_BKWD            = 0x4C,

	

	MHD_RCP_CMD_ANGLE            = 0x50,
	MHD_RCP_CMD_SUBPICTURE       = 0x51,

	

	MHD_RCP_CMD_PLAY_FUNC       = 0x60,
	MHD_RCP_CMD_PAUSE_PLAY_FUNC = 0x61,
	MHD_RCP_CMD_RECORD_FUNC     = 0x62,
	MHD_RCP_CMD_PAUSE_REC_FUNC  = 0x63,
	MHD_RCP_CMD_STOP_FUNC       = 0x64,
	MHD_RCP_CMD_MUTE_FUNC       = 0x65,
	MHD_RCP_CMD_UN_MUTE_FUNC    = 0x66,
	MHD_RCP_CMD_TUNE_FUNC       = 0x67,
	MHD_RCP_CMD_MEDIA_FUNC      = 0x68,

	

	MHD_RCP_CMD_F1              = 0x71,
	MHD_RCP_CMD_F2              = 0x72,
	MHD_RCP_CMD_F3              = 0x73,
	MHD_RCP_CMD_F4              = 0x74,
	MHD_RCP_CMD_F5              = 0x75,

	

	MHD_RCP_CMD_VS              = 0x7E,
	MHD_RCP_CMD_RSVD            = 0x7F,

};
