/*
    V4L2 controls framework implementation.

    Copyright (C) 2010  Hans Verkuil <hverkuil@xs4all.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-dev.h>

struct ctrl_helper {
	
	struct v4l2_ctrl *ctrl;
	bool handled;
};

const char * const *v4l2_ctrl_get_menu(u32 id)
{
	static const char * const mpeg_audio_sampling_freq[] = {
		"44.1 kHz",
		"48 kHz",
		"32 kHz",
		NULL
	};
	static const char * const mpeg_audio_encoding[] = {
		"MPEG-1/2 Layer I",
		"MPEG-1/2 Layer II",
		"MPEG-1/2 Layer III",
		"MPEG-2/4 AAC",
		"AC-3",
		NULL
	};
	static const char * const mpeg_audio_l1_bitrate[] = {
		"32 kbps",
		"64 kbps",
		"96 kbps",
		"128 kbps",
		"160 kbps",
		"192 kbps",
		"224 kbps",
		"256 kbps",
		"288 kbps",
		"320 kbps",
		"352 kbps",
		"384 kbps",
		"416 kbps",
		"448 kbps",
		NULL
	};
	static const char * const mpeg_audio_l2_bitrate[] = {
		"32 kbps",
		"48 kbps",
		"56 kbps",
		"64 kbps",
		"80 kbps",
		"96 kbps",
		"112 kbps",
		"128 kbps",
		"160 kbps",
		"192 kbps",
		"224 kbps",
		"256 kbps",
		"320 kbps",
		"384 kbps",
		NULL
	};
	static const char * const mpeg_audio_l3_bitrate[] = {
		"32 kbps",
		"40 kbps",
		"48 kbps",
		"56 kbps",
		"64 kbps",
		"80 kbps",
		"96 kbps",
		"112 kbps",
		"128 kbps",
		"160 kbps",
		"192 kbps",
		"224 kbps",
		"256 kbps",
		"320 kbps",
		NULL
	};
	static const char * const mpeg_audio_ac3_bitrate[] = {
		"32 kbps",
		"40 kbps",
		"48 kbps",
		"56 kbps",
		"64 kbps",
		"80 kbps",
		"96 kbps",
		"112 kbps",
		"128 kbps",
		"160 kbps",
		"192 kbps",
		"224 kbps",
		"256 kbps",
		"320 kbps",
		"384 kbps",
		"448 kbps",
		"512 kbps",
		"576 kbps",
		"640 kbps",
		NULL
	};
	static const char * const mpeg_audio_mode[] = {
		"Stereo",
		"Joint Stereo",
		"Dual",
		"Mono",
		NULL
	};
	static const char * const mpeg_audio_mode_extension[] = {
		"Bound 4",
		"Bound 8",
		"Bound 12",
		"Bound 16",
		NULL
	};
	static const char * const mpeg_audio_emphasis[] = {
		"No Emphasis",
		"50/15 us",
		"CCITT J17",
		NULL
	};
	static const char * const mpeg_audio_crc[] = {
		"No CRC",
		"16-bit CRC",
		NULL
	};
	static const char * const mpeg_video_encoding[] = {
		"MPEG-1",
		"MPEG-2",
		"MPEG-4 AVC",
		NULL
	};
	static const char * const mpeg_video_aspect[] = {
		"1x1",
		"4x3",
		"16x9",
		"2.21x1",
		NULL
	};
	static const char * const mpeg_video_bitrate_mode[] = {
		"Variable Bitrate",
		"Constant Bitrate",
		NULL
	};
	static const char * const mpeg_stream_type[] = {
		"MPEG-2 Program Stream",
		"MPEG-2 Transport Stream",
		"MPEG-1 System Stream",
		"MPEG-2 DVD-compatible Stream",
		"MPEG-1 VCD-compatible Stream",
		"MPEG-2 SVCD-compatible Stream",
		NULL
	};
	static const char * const mpeg_stream_vbi_fmt[] = {
		"No VBI",
		"Private packet, IVTV format",
		NULL
	};
	static const char * const camera_power_line_frequency[] = {
		"Disabled",
		"50 Hz",
		"60 Hz",
		NULL
	};
	static const char * const camera_exposure_auto[] = {
		"Auto Mode",
		"Manual Mode",
		"Shutter Priority Mode",
		"Aperture Priority Mode",
		NULL
	};
	static const char * const colorfx[] = {
		"None",
		"Black & White",
		"Sepia",
		"Negative",
		"Emboss",
		"Sketch",
		"Sky blue",
		"Grass green",
		"Skin whiten",
		"Vivid",
		NULL
	};
	static const char * const tune_preemphasis[] = {
		"No preemphasis",
		"50 useconds",
		"75 useconds",
		NULL,
	};

	switch (id) {
	case V4L2_CID_MPEG_AUDIO_SAMPLING_FREQ:
		return mpeg_audio_sampling_freq;
	case V4L2_CID_MPEG_AUDIO_ENCODING:
		return mpeg_audio_encoding;
	case V4L2_CID_MPEG_AUDIO_L1_BITRATE:
		return mpeg_audio_l1_bitrate;
	case V4L2_CID_MPEG_AUDIO_L2_BITRATE:
		return mpeg_audio_l2_bitrate;
	case V4L2_CID_MPEG_AUDIO_L3_BITRATE:
		return mpeg_audio_l3_bitrate;
	case V4L2_CID_MPEG_AUDIO_AC3_BITRATE:
		return mpeg_audio_ac3_bitrate;
	case V4L2_CID_MPEG_AUDIO_MODE:
		return mpeg_audio_mode;
	case V4L2_CID_MPEG_AUDIO_MODE_EXTENSION:
		return mpeg_audio_mode_extension;
	case V4L2_CID_MPEG_AUDIO_EMPHASIS:
		return mpeg_audio_emphasis;
	case V4L2_CID_MPEG_AUDIO_CRC:
		return mpeg_audio_crc;
	case V4L2_CID_MPEG_VIDEO_ENCODING:
		return mpeg_video_encoding;
	case V4L2_CID_MPEG_VIDEO_ASPECT:
		return mpeg_video_aspect;
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
		return mpeg_video_bitrate_mode;
	case V4L2_CID_MPEG_STREAM_TYPE:
		return mpeg_stream_type;
	case V4L2_CID_MPEG_STREAM_VBI_FMT:
		return mpeg_stream_vbi_fmt;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return camera_power_line_frequency;
	case V4L2_CID_EXPOSURE_AUTO:
		return camera_exposure_auto;
	case V4L2_CID_COLORFX:
		return colorfx;
	case V4L2_CID_TUNE_PREEMPHASIS:
		return tune_preemphasis;
	default:
		return NULL;
	}
}
EXPORT_SYMBOL(v4l2_ctrl_get_menu);

const char *v4l2_ctrl_get_name(u32 id)
{
	switch (id) {
	
	
	case V4L2_CID_USER_CLASS:		return "User Controls";
	case V4L2_CID_BRIGHTNESS:		return "Brightness";
	case V4L2_CID_CONTRAST:			return "Contrast";
	case V4L2_CID_SATURATION:		return "Saturation";
	case V4L2_CID_HUE:			return "Hue";
	case V4L2_CID_AUDIO_VOLUME:		return "Volume";
	case V4L2_CID_AUDIO_BALANCE:		return "Balance";
	case V4L2_CID_AUDIO_BASS:		return "Bass";
	case V4L2_CID_AUDIO_TREBLE:		return "Treble";
	case V4L2_CID_AUDIO_MUTE:		return "Mute";
	case V4L2_CID_AUDIO_LOUDNESS:		return "Loudness";
	case V4L2_CID_BLACK_LEVEL:		return "Black Level";
	case V4L2_CID_AUTO_WHITE_BALANCE:	return "White Balance, Automatic";
	case V4L2_CID_DO_WHITE_BALANCE:		return "Do White Balance";
	case V4L2_CID_RED_BALANCE:		return "Red Balance";
	case V4L2_CID_BLUE_BALANCE:		return "Blue Balance";
	case V4L2_CID_GAMMA:			return "Gamma";
	case V4L2_CID_EXPOSURE:			return "Exposure";
	case V4L2_CID_AUTOGAIN:			return "Gain, Automatic";
	case V4L2_CID_GAIN:			return "Gain";
	case V4L2_CID_HFLIP:			return "Horizontal Flip";
	case V4L2_CID_VFLIP:			return "Vertical Flip";
	case V4L2_CID_HCENTER:			return "Horizontal Center";
	case V4L2_CID_VCENTER:			return "Vertical Center";
	case V4L2_CID_POWER_LINE_FREQUENCY:	return "Power Line Frequency";
	case V4L2_CID_HUE_AUTO:			return "Hue, Automatic";
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE: return "White Balance Temperature";
	case V4L2_CID_SHARPNESS:		return "Sharpness";
	case V4L2_CID_BACKLIGHT_COMPENSATION:	return "Backlight Compensation";
	case V4L2_CID_CHROMA_AGC:		return "Chroma AGC";
	case V4L2_CID_COLOR_KILLER:		return "Color Killer";
	case V4L2_CID_COLORFX:			return "Color Effects";
	case V4L2_CID_AUTOBRIGHTNESS:		return "Brightness, Automatic";
	case V4L2_CID_BAND_STOP_FILTER:		return "Band-Stop Filter";
	case V4L2_CID_ROTATE:			return "Rotate";
	case V4L2_CID_BG_COLOR:			return "Background Color";
	case V4L2_CID_CHROMA_GAIN:		return "Chroma Gain";
	case V4L2_CID_ILLUMINATORS_1:		return "Illuminator 1";
	case V4L2_CID_ILLUMINATORS_2:		return "Illuminator 2";

	
	
	case V4L2_CID_MPEG_CLASS:		return "MPEG Encoder Controls";
	case V4L2_CID_MPEG_STREAM_TYPE:		return "Stream Type";
	case V4L2_CID_MPEG_STREAM_PID_PMT:	return "Stream PMT Program ID";
	case V4L2_CID_MPEG_STREAM_PID_AUDIO:	return "Stream Audio Program ID";
	case V4L2_CID_MPEG_STREAM_PID_VIDEO:	return "Stream Video Program ID";
	case V4L2_CID_MPEG_STREAM_PID_PCR:	return "Stream PCR Program ID";
	case V4L2_CID_MPEG_STREAM_PES_ID_AUDIO: return "Stream PES Audio ID";
	case V4L2_CID_MPEG_STREAM_PES_ID_VIDEO: return "Stream PES Video ID";
	case V4L2_CID_MPEG_STREAM_VBI_FMT:	return "Stream VBI Format";
	case V4L2_CID_MPEG_AUDIO_SAMPLING_FREQ: return "Audio Sampling Frequency";
	case V4L2_CID_MPEG_AUDIO_ENCODING:	return "Audio Encoding";
	case V4L2_CID_MPEG_AUDIO_L1_BITRATE:	return "Audio Layer I Bitrate";
	case V4L2_CID_MPEG_AUDIO_L2_BITRATE:	return "Audio Layer II Bitrate";
	case V4L2_CID_MPEG_AUDIO_L3_BITRATE:	return "Audio Layer III Bitrate";
	case V4L2_CID_MPEG_AUDIO_MODE:		return "Audio Stereo Mode";
	case V4L2_CID_MPEG_AUDIO_MODE_EXTENSION: return "Audio Stereo Mode Extension";
	case V4L2_CID_MPEG_AUDIO_EMPHASIS:	return "Audio Emphasis";
	case V4L2_CID_MPEG_AUDIO_CRC:		return "Audio CRC";
	case V4L2_CID_MPEG_AUDIO_MUTE:		return "Audio Mute";
	case V4L2_CID_MPEG_AUDIO_AAC_BITRATE:	return "Audio AAC Bitrate";
	case V4L2_CID_MPEG_AUDIO_AC3_BITRATE:	return "Audio AC-3 Bitrate";
	case V4L2_CID_MPEG_VIDEO_ENCODING:	return "Video Encoding";
	case V4L2_CID_MPEG_VIDEO_ASPECT:	return "Video Aspect";
	case V4L2_CID_MPEG_VIDEO_B_FRAMES:	return "Video B Frames";
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:	return "Video GOP Size";
	case V4L2_CID_MPEG_VIDEO_GOP_CLOSURE:	return "Video GOP Closure";
	case V4L2_CID_MPEG_VIDEO_PULLDOWN:	return "Video Pulldown";
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:	return "Video Bitrate Mode";
	case V4L2_CID_MPEG_VIDEO_BITRATE:	return "Video Bitrate";
	case V4L2_CID_MPEG_VIDEO_BITRATE_PEAK:	return "Video Peak Bitrate";
	case V4L2_CID_MPEG_VIDEO_TEMPORAL_DECIMATION: return "Video Temporal Decimation";
	case V4L2_CID_MPEG_VIDEO_MUTE:		return "Video Mute";
	case V4L2_CID_MPEG_VIDEO_MUTE_YUV:	return "Video Mute YUV";

	
	
	case V4L2_CID_CAMERA_CLASS:		return "Camera Controls";
	case V4L2_CID_EXPOSURE_AUTO:		return "Auto Exposure";
	case V4L2_CID_EXPOSURE_ABSOLUTE:	return "Exposure Time, Absolute";
	case V4L2_CID_EXPOSURE_AUTO_PRIORITY:	return "Exposure, Dynamic Framerate";
	case V4L2_CID_PAN_RELATIVE:		return "Pan, Relative";
	case V4L2_CID_TILT_RELATIVE:		return "Tilt, Relative";
	case V4L2_CID_PAN_RESET:		return "Pan, Reset";
	case V4L2_CID_TILT_RESET:		return "Tilt, Reset";
	case V4L2_CID_PAN_ABSOLUTE:		return "Pan, Absolute";
	case V4L2_CID_TILT_ABSOLUTE:		return "Tilt, Absolute";
	case V4L2_CID_FOCUS_ABSOLUTE:		return "Focus, Absolute";
	case V4L2_CID_FOCUS_RELATIVE:		return "Focus, Relative";
	case V4L2_CID_FOCUS_AUTO:		return "Focus, Automatic";
	case V4L2_CID_ZOOM_ABSOLUTE:		return "Zoom, Absolute";
	case V4L2_CID_ZOOM_RELATIVE:		return "Zoom, Relative";
	case V4L2_CID_ZOOM_CONTINUOUS:		return "Zoom, Continuous";
	case V4L2_CID_PRIVACY:			return "Privacy";
	case V4L2_CID_IRIS_ABSOLUTE:		return "Iris, Absolute";
	case V4L2_CID_IRIS_RELATIVE:		return "Iris, Relative";

	
	
	case V4L2_CID_FM_TX_CLASS:		return "FM Radio Modulator Controls";
	case V4L2_CID_RDS_TX_DEVIATION:		return "RDS Signal Deviation";
	case V4L2_CID_RDS_TX_PI:		return "RDS Program ID";
	case V4L2_CID_RDS_TX_PTY:		return "RDS Program Type";
	case V4L2_CID_RDS_TX_PS_NAME:		return "RDS PS Name";
	case V4L2_CID_RDS_TX_RADIO_TEXT:	return "RDS Radio Text";
	case V4L2_CID_AUDIO_LIMITER_ENABLED:	return "Audio Limiter Feature Enabled";
	case V4L2_CID_AUDIO_LIMITER_RELEASE_TIME: return "Audio Limiter Release Time";
	case V4L2_CID_AUDIO_LIMITER_DEVIATION:	return "Audio Limiter Deviation";
	case V4L2_CID_AUDIO_COMPRESSION_ENABLED: return "Audio Compression Feature Enabled";
	case V4L2_CID_AUDIO_COMPRESSION_GAIN:	return "Audio Compression Gain";
	case V4L2_CID_AUDIO_COMPRESSION_THRESHOLD: return "Audio Compression Threshold";
	case V4L2_CID_AUDIO_COMPRESSION_ATTACK_TIME: return "Audio Compression Attack Time";
	case V4L2_CID_AUDIO_COMPRESSION_RELEASE_TIME: return "Audio Compression Release Time";
	case V4L2_CID_PILOT_TONE_ENABLED:	return "Pilot Tone Feature Enabled";
	case V4L2_CID_PILOT_TONE_DEVIATION:	return "Pilot Tone Deviation";
	case V4L2_CID_PILOT_TONE_FREQUENCY:	return "Pilot Tone Frequency";
	case V4L2_CID_TUNE_PREEMPHASIS:		return "Pre-emphasis settings";
	case V4L2_CID_TUNE_POWER_LEVEL:		return "Tune Power Level";
	case V4L2_CID_TUNE_ANTENNA_CAPACITOR:	return "Tune Antenna Capacitor";

	default:
		return NULL;
	}
}
EXPORT_SYMBOL(v4l2_ctrl_get_name);

void v4l2_ctrl_fill(u32 id, const char **name, enum v4l2_ctrl_type *type,
		    s32 *min, s32 *max, s32 *step, s32 *def, u32 *flags)
{
	*name = v4l2_ctrl_get_name(id);
	*flags = 0;

	switch (id) {
	case V4L2_CID_AUDIO_MUTE:
	case V4L2_CID_AUDIO_LOUDNESS:
	case V4L2_CID_AUTO_WHITE_BALANCE:
	case V4L2_CID_AUTOGAIN:
	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP:
	case V4L2_CID_HUE_AUTO:
	case V4L2_CID_CHROMA_AGC:
	case V4L2_CID_COLOR_KILLER:
	case V4L2_CID_MPEG_AUDIO_MUTE:
	case V4L2_CID_MPEG_VIDEO_MUTE:
	case V4L2_CID_MPEG_VIDEO_GOP_CLOSURE:
	case V4L2_CID_MPEG_VIDEO_PULLDOWN:
	case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
	case V4L2_CID_FOCUS_AUTO:
	case V4L2_CID_PRIVACY:
	case V4L2_CID_AUDIO_LIMITER_ENABLED:
	case V4L2_CID_AUDIO_COMPRESSION_ENABLED:
	case V4L2_CID_PILOT_TONE_ENABLED:
	case V4L2_CID_ILLUMINATORS_1:
	case V4L2_CID_ILLUMINATORS_2:
		*type = V4L2_CTRL_TYPE_BOOLEAN;
		*min = 0;
		*max = *step = 1;
		break;
	case V4L2_CID_PAN_RESET:
	case V4L2_CID_TILT_RESET:
		*type = V4L2_CTRL_TYPE_BUTTON;
		*flags |= V4L2_CTRL_FLAG_WRITE_ONLY;
		*min = *max = *step = *def = 0;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
	case V4L2_CID_MPEG_AUDIO_SAMPLING_FREQ:
	case V4L2_CID_MPEG_AUDIO_ENCODING:
	case V4L2_CID_MPEG_AUDIO_L1_BITRATE:
	case V4L2_CID_MPEG_AUDIO_L2_BITRATE:
	case V4L2_CID_MPEG_AUDIO_L3_BITRATE:
	case V4L2_CID_MPEG_AUDIO_AC3_BITRATE:
	case V4L2_CID_MPEG_AUDIO_MODE:
	case V4L2_CID_MPEG_AUDIO_MODE_EXTENSION:
	case V4L2_CID_MPEG_AUDIO_EMPHASIS:
	case V4L2_CID_MPEG_AUDIO_CRC:
	case V4L2_CID_MPEG_VIDEO_ENCODING:
	case V4L2_CID_MPEG_VIDEO_ASPECT:
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
	case V4L2_CID_MPEG_STREAM_TYPE:
	case V4L2_CID_MPEG_STREAM_VBI_FMT:
	case V4L2_CID_EXPOSURE_AUTO:
	case V4L2_CID_COLORFX:
	case V4L2_CID_TUNE_PREEMPHASIS:
		*type = V4L2_CTRL_TYPE_MENU;
		break;
	case V4L2_CID_RDS_TX_PS_NAME:
	case V4L2_CID_RDS_TX_RADIO_TEXT:
		*type = V4L2_CTRL_TYPE_STRING;
		break;
	case V4L2_CID_USER_CLASS:
	case V4L2_CID_CAMERA_CLASS:
	case V4L2_CID_MPEG_CLASS:
	case V4L2_CID_FM_TX_CLASS:
		*type = V4L2_CTRL_TYPE_CTRL_CLASS;
		
		*flags |= V4L2_CTRL_FLAG_READ_ONLY | V4L2_CTRL_FLAG_WRITE_ONLY;
		*min = *max = *step = *def = 0;
		break;
	case V4L2_CID_BG_COLOR:
		*type = V4L2_CTRL_TYPE_INTEGER;
		*step = 1;
		*min = 0;
		
		*max = 0xFFFFFF;
		break;
	default:
		*type = V4L2_CTRL_TYPE_INTEGER;
		break;
	}
	switch (id) {
	case V4L2_CID_MPEG_AUDIO_ENCODING:
	case V4L2_CID_MPEG_AUDIO_MODE:
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
	case V4L2_CID_MPEG_VIDEO_B_FRAMES:
	case V4L2_CID_MPEG_STREAM_TYPE:
		*flags |= V4L2_CTRL_FLAG_UPDATE;
		break;
	case V4L2_CID_AUDIO_VOLUME:
	case V4L2_CID_AUDIO_BALANCE:
	case V4L2_CID_AUDIO_BASS:
	case V4L2_CID_AUDIO_TREBLE:
	case V4L2_CID_BRIGHTNESS:
	case V4L2_CID_CONTRAST:
	case V4L2_CID_SATURATION:
	case V4L2_CID_HUE:
	case V4L2_CID_RED_BALANCE:
	case V4L2_CID_BLUE_BALANCE:
	case V4L2_CID_GAMMA:
	case V4L2_CID_SHARPNESS:
	case V4L2_CID_CHROMA_GAIN:
	case V4L2_CID_RDS_TX_DEVIATION:
	case V4L2_CID_AUDIO_LIMITER_RELEASE_TIME:
	case V4L2_CID_AUDIO_LIMITER_DEVIATION:
	case V4L2_CID_AUDIO_COMPRESSION_GAIN:
	case V4L2_CID_AUDIO_COMPRESSION_THRESHOLD:
	case V4L2_CID_AUDIO_COMPRESSION_ATTACK_TIME:
	case V4L2_CID_AUDIO_COMPRESSION_RELEASE_TIME:
	case V4L2_CID_PILOT_TONE_DEVIATION:
	case V4L2_CID_PILOT_TONE_FREQUENCY:
	case V4L2_CID_TUNE_POWER_LEVEL:
	case V4L2_CID_TUNE_ANTENNA_CAPACITOR:
		*flags |= V4L2_CTRL_FLAG_SLIDER;
		break;
	case V4L2_CID_PAN_RELATIVE:
	case V4L2_CID_TILT_RELATIVE:
	case V4L2_CID_FOCUS_RELATIVE:
	case V4L2_CID_IRIS_RELATIVE:
	case V4L2_CID_ZOOM_RELATIVE:
		*flags |= V4L2_CTRL_FLAG_WRITE_ONLY;
		break;
	}
}
EXPORT_SYMBOL(v4l2_ctrl_fill);

static bool type_is_int(const struct v4l2_ctrl *ctrl)
{
	switch (ctrl->type) {
	case V4L2_CTRL_TYPE_INTEGER64:
	case V4L2_CTRL_TYPE_STRING:
		
		return false;
	default:
		return true;
	}
}

static int cur_to_user(struct v4l2_ext_control *c,
		       struct v4l2_ctrl *ctrl)
{
	u32 len;

	switch (ctrl->type) {
	case V4L2_CTRL_TYPE_STRING:
		len = strlen(ctrl->cur.string);
		if (c->size < len + 1) {
			c->size = len + 1;
			return -ENOSPC;
		}
		return copy_to_user(c->string, ctrl->cur.string,
						len + 1) ? -EFAULT : 0;
	case V4L2_CTRL_TYPE_INTEGER64:
		c->value64 = ctrl->cur.val64;
		break;
	default:
		c->value = ctrl->cur.val;
		break;
	}
	return 0;
}

static int user_to_new(struct v4l2_ext_control *c,
		       struct v4l2_ctrl *ctrl)
{
	int ret;
	u32 size;

	ctrl->is_new = 1;
	switch (ctrl->type) {
	case V4L2_CTRL_TYPE_INTEGER64:
		ctrl->val64 = c->value64;
		break;
	case V4L2_CTRL_TYPE_STRING:
		size = c->size;
		if (size == 0)
			return -ERANGE;
		if (size > ctrl->maximum + 1)
			size = ctrl->maximum + 1;
		ret = copy_from_user(ctrl->string, c->string, size);
		if (!ret) {
			char last = ctrl->string[size - 1];

			ctrl->string[size - 1] = 0;
			if (strlen(ctrl->string) == ctrl->maximum && last)
				return -ERANGE;
		}
		return ret ? -EFAULT : 0;
	default:
		ctrl->val = c->value;
		break;
	}
	return 0;
}

static int new_to_user(struct v4l2_ext_control *c,
		       struct v4l2_ctrl *ctrl)
{
	u32 len;

	switch (ctrl->type) {
	case V4L2_CTRL_TYPE_STRING:
		len = strlen(ctrl->string);
		if (c->size < len + 1) {
			c->size = ctrl->maximum + 1;
			return -ENOSPC;
		}
		return copy_to_user(c->string, ctrl->string,
						len + 1) ? -EFAULT : 0;
	case V4L2_CTRL_TYPE_INTEGER64:
		c->value64 = ctrl->val64;
		break;
	default:
		c->value = ctrl->val;
		break;
	}
	return 0;
}

static void new_to_cur(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL)
		return;
	switch (ctrl->type) {
	case V4L2_CTRL_TYPE_STRING:
		
		strcpy(ctrl->cur.string, ctrl->string);
		break;
	case V4L2_CTRL_TYPE_INTEGER64:
		ctrl->cur.val64 = ctrl->val64;
		break;
	default:
		ctrl->cur.val = ctrl->val;
		break;
	}
}

static void cur_to_new(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL)
		return;
	switch (ctrl->type) {
	case V4L2_CTRL_TYPE_STRING:
		
		strcpy(ctrl->string, ctrl->cur.string);
		break;
	case V4L2_CTRL_TYPE_INTEGER64:
		ctrl->val64 = ctrl->cur.val64;
		break;
	default:
		ctrl->val = ctrl->cur.val;
		break;
	}
}

static int cluster_changed(struct v4l2_ctrl *master)
{
	int diff = 0;
	int i;

	for (i = 0; !diff && i < master->ncontrols; i++) {
		struct v4l2_ctrl *ctrl = master->cluster[i];

		if (ctrl == NULL)
			continue;
		switch (ctrl->type) {
		case V4L2_CTRL_TYPE_BUTTON:
			
			return 1;
		case V4L2_CTRL_TYPE_STRING:
			
			diff = strcmp(ctrl->string, ctrl->cur.string);
			break;
		case V4L2_CTRL_TYPE_INTEGER64:
			diff = ctrl->val64 != ctrl->cur.val64;
			break;
		default:
			diff = ctrl->val != ctrl->cur.val;
			break;
		}
	}
	return diff;
}

static int validate_new(struct v4l2_ctrl *ctrl)
{
	s32 val = ctrl->val;
	char *s = ctrl->string;
	u32 offset;
	size_t len;

	switch (ctrl->type) {
	case V4L2_CTRL_TYPE_INTEGER:
		
		val += ctrl->step / 2;
		if (val < ctrl->minimum)
			val = ctrl->minimum;
		if (val > ctrl->maximum)
			val = ctrl->maximum;
		offset = val - ctrl->minimum;
		offset = ctrl->step * (offset / ctrl->step);
		val = ctrl->minimum + offset;
		ctrl->val = val;
		return 0;

	case V4L2_CTRL_TYPE_BOOLEAN:
		ctrl->val = !!ctrl->val;
		return 0;

	case V4L2_CTRL_TYPE_MENU:
		if (val < ctrl->minimum || val > ctrl->maximum)
			return -ERANGE;
		if (ctrl->qmenu[val][0] == '\0' ||
		    (ctrl->menu_skip_mask & (1 << val)))
			return -EINVAL;
		return 0;

	case V4L2_CTRL_TYPE_BUTTON:
	case V4L2_CTRL_TYPE_CTRL_CLASS:
		ctrl->val64 = 0;
		return 0;

	case V4L2_CTRL_TYPE_INTEGER64:
		return 0;

	case V4L2_CTRL_TYPE_STRING:
		len = strlen(s);
		if (len < ctrl->minimum)
			return -ERANGE;
		if ((len - ctrl->minimum) % ctrl->step)
			return -ERANGE;
		return 0;

	default:
		return -EINVAL;
	}
}

static inline u32 node2id(struct list_head *node)
{
	return list_entry(node, struct v4l2_ctrl_ref, node)->ctrl->id;
}

static inline int handler_set_err(struct v4l2_ctrl_handler *hdl, int err)
{
	if (hdl->error == 0)
		hdl->error = err;
	return err;
}

int v4l2_ctrl_handler_init(struct v4l2_ctrl_handler *hdl,
			   unsigned nr_of_controls_hint)
{
	mutex_init(&hdl->lock);
	INIT_LIST_HEAD(&hdl->ctrls);
	INIT_LIST_HEAD(&hdl->ctrl_refs);
	hdl->nr_of_buckets = 1 + nr_of_controls_hint / 8;
	hdl->buckets = kzalloc(sizeof(hdl->buckets[0]) * hdl->nr_of_buckets,
								GFP_KERNEL);
	hdl->error = hdl->buckets ? 0 : -ENOMEM;
	return hdl->error;
}
EXPORT_SYMBOL(v4l2_ctrl_handler_init);

void v4l2_ctrl_handler_free(struct v4l2_ctrl_handler *hdl)
{
	struct v4l2_ctrl_ref *ref, *next_ref;
	struct v4l2_ctrl *ctrl, *next_ctrl;

	if (hdl == NULL || hdl->buckets == NULL)
		return;

	mutex_lock(&hdl->lock);
	
	list_for_each_entry_safe(ref, next_ref, &hdl->ctrl_refs, node) {
		list_del(&ref->node);
		kfree(ref);
	}
	
	list_for_each_entry_safe(ctrl, next_ctrl, &hdl->ctrls, node) {
		list_del(&ctrl->node);
		kfree(ctrl);
	}
	kfree(hdl->buckets);
	hdl->buckets = NULL;
	hdl->cached = NULL;
	hdl->error = 0;
	mutex_unlock(&hdl->lock);
}
EXPORT_SYMBOL(v4l2_ctrl_handler_free);

static struct v4l2_ctrl_ref *find_private_ref(
		struct v4l2_ctrl_handler *hdl, u32 id)
{
	struct v4l2_ctrl_ref *ref;

	id -= V4L2_CID_PRIVATE_BASE;
	list_for_each_entry(ref, &hdl->ctrl_refs, node) {
		if (V4L2_CTRL_ID2CLASS(ref->ctrl->id) == V4L2_CTRL_CLASS_USER &&
		    V4L2_CTRL_DRIVER_PRIV(ref->ctrl->id)) {
			if (!type_is_int(ref->ctrl))
				continue;
			if (id == 0)
				return ref;
			id--;
		}
	}
	return NULL;
}

static struct v4l2_ctrl_ref *find_ref(struct v4l2_ctrl_handler *hdl, u32 id)
{
	struct v4l2_ctrl_ref *ref;
	int bucket;

	id &= V4L2_CTRL_ID_MASK;

	
	if (id >= V4L2_CID_PRIVATE_BASE)
		return find_private_ref(hdl, id);
	bucket = id % hdl->nr_of_buckets;

	
	if (hdl->cached && hdl->cached->ctrl->id == id)
		return hdl->cached;

	
	ref = hdl->buckets ? hdl->buckets[bucket] : NULL;
	while (ref && ref->ctrl->id != id)
		ref = ref->next;

	if (ref)
		hdl->cached = ref; 
	return ref;
}

static struct v4l2_ctrl_ref *find_ref_lock(
		struct v4l2_ctrl_handler *hdl, u32 id)
{
	struct v4l2_ctrl_ref *ref = NULL;

	if (hdl) {
		mutex_lock(&hdl->lock);
		ref = find_ref(hdl, id);
		mutex_unlock(&hdl->lock);
	}
	return ref;
}

struct v4l2_ctrl *v4l2_ctrl_find(struct v4l2_ctrl_handler *hdl, u32 id)
{
	struct v4l2_ctrl_ref *ref = find_ref_lock(hdl, id);

	return ref ? ref->ctrl : NULL;
}
EXPORT_SYMBOL(v4l2_ctrl_find);

static int handler_new_ref(struct v4l2_ctrl_handler *hdl,
			   struct v4l2_ctrl *ctrl)
{
	struct v4l2_ctrl_ref *ref;
	struct v4l2_ctrl_ref *new_ref;
	u32 id = ctrl->id;
	u32 class_ctrl = V4L2_CTRL_ID2CLASS(id) | 1;
	int bucket = id % hdl->nr_of_buckets;	

	
	if (id != class_ctrl && find_ref_lock(hdl, class_ctrl) == NULL)
		if (!v4l2_ctrl_new_std(hdl, NULL, class_ctrl, 0, 0, 0, 0))
			return hdl->error;

	if (hdl->error)
		return hdl->error;

	new_ref = kzalloc(sizeof(*new_ref), GFP_KERNEL);
	if (!new_ref)
		return handler_set_err(hdl, -ENOMEM);
	new_ref->ctrl = ctrl;
	if (ctrl->handler == hdl) {
		ctrl->cluster = &new_ref->ctrl;
		ctrl->ncontrols = 1;
	}

	INIT_LIST_HEAD(&new_ref->node);

	mutex_lock(&hdl->lock);

	if (list_empty(&hdl->ctrl_refs) || id > node2id(hdl->ctrl_refs.prev)) {
		list_add_tail(&new_ref->node, &hdl->ctrl_refs);
		goto insert_in_hash;
	}

	
	list_for_each_entry(ref, &hdl->ctrl_refs, node) {
		if (ref->ctrl->id < id)
			continue;
		
		if (ref->ctrl->id == id) {
			kfree(new_ref);
			goto unlock;
		}
		list_add(&new_ref->node, ref->node.prev);
		break;
	}

insert_in_hash:
	
	new_ref->next = hdl->buckets[bucket];
	hdl->buckets[bucket] = new_ref;

unlock:
	mutex_unlock(&hdl->lock);
	return 0;
}

static struct v4l2_ctrl *v4l2_ctrl_new(struct v4l2_ctrl_handler *hdl,
			const struct v4l2_ctrl_ops *ops,
			u32 id, const char *name, enum v4l2_ctrl_type type,
			s32 min, s32 max, u32 step, s32 def,
			u32 flags, const char * const *qmenu, void *priv)
{
	struct v4l2_ctrl *ctrl;
	unsigned sz_extra = 0;

	if (hdl->error)
		return NULL;

	
	if (id == 0 || name == NULL || id >= V4L2_CID_PRIVATE_BASE ||
	    max < min ||
	    (type == V4L2_CTRL_TYPE_INTEGER && step == 0) ||
	    (type == V4L2_CTRL_TYPE_MENU && qmenu == NULL) ||
	    (type == V4L2_CTRL_TYPE_STRING && max == 0)) {
		handler_set_err(hdl, -ERANGE);
		return NULL;
	}
	if ((type == V4L2_CTRL_TYPE_INTEGER ||
	     type == V4L2_CTRL_TYPE_MENU ||
	     type == V4L2_CTRL_TYPE_BOOLEAN) &&
	    (def < min || def > max)) {
		handler_set_err(hdl, -ERANGE);
		return NULL;
	}

	if (type == V4L2_CTRL_TYPE_BUTTON)
		flags |= V4L2_CTRL_FLAG_WRITE_ONLY;
	else if (type == V4L2_CTRL_TYPE_CTRL_CLASS)
		flags |= V4L2_CTRL_FLAG_READ_ONLY;
	else if (type == V4L2_CTRL_TYPE_STRING)
		sz_extra += 2 * (max + 1);

	ctrl = kzalloc(sizeof(*ctrl) + sz_extra, GFP_KERNEL);
	if (ctrl == NULL) {
		handler_set_err(hdl, -ENOMEM);
		return NULL;
	}

	INIT_LIST_HEAD(&ctrl->node);
	ctrl->handler = hdl;
	ctrl->ops = ops;
	ctrl->id = id;
	ctrl->name = name;
	ctrl->type = type;
	ctrl->flags = flags;
	ctrl->minimum = min;
	ctrl->maximum = max;
	ctrl->step = step;
	ctrl->qmenu = qmenu;
	ctrl->priv = priv;
	ctrl->cur.val = ctrl->val = ctrl->default_value = def;

	if (ctrl->type == V4L2_CTRL_TYPE_STRING) {
		ctrl->cur.string = (char *)&ctrl[1] + sz_extra - (max + 1);
		ctrl->string = (char *)&ctrl[1] + sz_extra - 2 * (max + 1);
		if (ctrl->minimum)
			memset(ctrl->cur.string, ' ', ctrl->minimum);
	}
	if (handler_new_ref(hdl, ctrl)) {
		kfree(ctrl);
		return NULL;
	}
	mutex_lock(&hdl->lock);
	list_add_tail(&ctrl->node, &hdl->ctrls);
	mutex_unlock(&hdl->lock);
	return ctrl;
}

struct v4l2_ctrl *v4l2_ctrl_new_custom(struct v4l2_ctrl_handler *hdl,
			const struct v4l2_ctrl_config *cfg, void *priv)
{
	bool is_menu;
	struct v4l2_ctrl *ctrl;
	const char *name = cfg->name;
	const char * const *qmenu = cfg->qmenu;
	enum v4l2_ctrl_type type = cfg->type;
	u32 flags = cfg->flags;
	s32 min = cfg->min;
	s32 max = cfg->max;
	u32 step = cfg->step;
	s32 def = cfg->def;

	if (name == NULL)
		v4l2_ctrl_fill(cfg->id, &name, &type, &min, &max, &step,
								&def, &flags);

	is_menu = (cfg->type == V4L2_CTRL_TYPE_MENU);
	if (is_menu)
		WARN_ON(step);
	else
		WARN_ON(cfg->menu_skip_mask);
	if (is_menu && qmenu == NULL)
		qmenu = v4l2_ctrl_get_menu(cfg->id);

	ctrl = v4l2_ctrl_new(hdl, cfg->ops, cfg->id, name,
			type, min, max,
			is_menu ? cfg->menu_skip_mask : step,
			def, flags, qmenu, priv);
	if (ctrl) {
		ctrl->is_private = cfg->is_private;
		ctrl->is_volatile = cfg->is_volatile;
	}
	return ctrl;
}
EXPORT_SYMBOL(v4l2_ctrl_new_custom);

struct v4l2_ctrl *v4l2_ctrl_new_std(struct v4l2_ctrl_handler *hdl,
			const struct v4l2_ctrl_ops *ops,
			u32 id, s32 min, s32 max, u32 step, s32 def)
{
	const char *name;
	enum v4l2_ctrl_type type;
	u32 flags;

	v4l2_ctrl_fill(id, &name, &type, &min, &max, &step, &def, &flags);
	if (type == V4L2_CTRL_TYPE_MENU) {
		handler_set_err(hdl, -EINVAL);
		return NULL;
	}
	return v4l2_ctrl_new(hdl, ops, id, name, type,
				    min, max, step, def, flags, NULL, NULL);
}
EXPORT_SYMBOL(v4l2_ctrl_new_std);

struct v4l2_ctrl *v4l2_ctrl_new_std_menu(struct v4l2_ctrl_handler *hdl,
			const struct v4l2_ctrl_ops *ops,
			u32 id, s32 max, s32 mask, s32 def)
{
	const char * const *qmenu = v4l2_ctrl_get_menu(id);
	const char *name;
	enum v4l2_ctrl_type type;
	s32 min;
	s32 step;
	u32 flags;

	v4l2_ctrl_fill(id, &name, &type, &min, &max, &step, &def, &flags);
	if (type != V4L2_CTRL_TYPE_MENU) {
		handler_set_err(hdl, -EINVAL);
		return NULL;
	}
	return v4l2_ctrl_new(hdl, ops, id, name, type,
				    0, max, mask, def, flags, qmenu, NULL);
}
EXPORT_SYMBOL(v4l2_ctrl_new_std_menu);

struct v4l2_ctrl *v4l2_ctrl_add_ctrl(struct v4l2_ctrl_handler *hdl,
					  struct v4l2_ctrl *ctrl)
{
	if (hdl == NULL || hdl->error)
		return NULL;
	if (ctrl == NULL) {
		handler_set_err(hdl, -EINVAL);
		return NULL;
	}
	if (ctrl->handler == hdl)
		return ctrl;
	return handler_new_ref(hdl, ctrl) ? NULL : ctrl;
}
EXPORT_SYMBOL(v4l2_ctrl_add_ctrl);

int v4l2_ctrl_add_handler(struct v4l2_ctrl_handler *hdl,
			  struct v4l2_ctrl_handler *add)
{
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	
	if (!hdl || !add || hdl == add)
		return 0;
	if (hdl->error)
		return hdl->error;
	mutex_lock(&add->lock);
	list_for_each_entry(ctrl, &add->ctrls, node) {
		
		if (ctrl->is_private)
			continue;
		ret = handler_new_ref(hdl, ctrl);
		if (ret)
			break;
	}
	mutex_unlock(&add->lock);
	return ret;
}
EXPORT_SYMBOL(v4l2_ctrl_add_handler);

void v4l2_ctrl_cluster(unsigned ncontrols, struct v4l2_ctrl **controls)
{
	int i;

	
	BUG_ON(controls[0] == NULL);

	for (i = 0; i < ncontrols; i++) {
		if (controls[i]) {
			controls[i]->cluster = controls;
			controls[i]->ncontrols = ncontrols;
		}
	}
}
EXPORT_SYMBOL(v4l2_ctrl_cluster);

void v4l2_ctrl_activate(struct v4l2_ctrl *ctrl, bool active)
{
	if (ctrl == NULL)
		return;

	if (!active)
		
		set_bit(4, &ctrl->flags);
	else
		
		clear_bit(4, &ctrl->flags);
}
EXPORT_SYMBOL(v4l2_ctrl_activate);

void v4l2_ctrl_grab(struct v4l2_ctrl *ctrl, bool grabbed)
{
	if (ctrl == NULL)
		return;

	if (grabbed)
		
		set_bit(1, &ctrl->flags);
	else
		
		clear_bit(1, &ctrl->flags);
}
EXPORT_SYMBOL(v4l2_ctrl_grab);

static void log_ctrl(const struct v4l2_ctrl *ctrl,
		     const char *prefix, const char *colon)
{
	int fl_inact = ctrl->flags & V4L2_CTRL_FLAG_INACTIVE;
	int fl_grabbed = ctrl->flags & V4L2_CTRL_FLAG_GRABBED;

	if (ctrl->flags & (V4L2_CTRL_FLAG_DISABLED | V4L2_CTRL_FLAG_WRITE_ONLY))
		return;
	if (ctrl->type == V4L2_CTRL_TYPE_CTRL_CLASS)
		return;

	printk(KERN_INFO "%s%s%s: ", prefix, colon, ctrl->name);

	switch (ctrl->type) {
	case V4L2_CTRL_TYPE_INTEGER:
		printk(KERN_CONT "%d", ctrl->cur.val);
		break;
	case V4L2_CTRL_TYPE_BOOLEAN:
		printk(KERN_CONT "%s", ctrl->cur.val ? "true" : "false");
		break;
	case V4L2_CTRL_TYPE_MENU:
		printk(KERN_CONT "%s", ctrl->qmenu[ctrl->cur.val]);
		break;
	case V4L2_CTRL_TYPE_INTEGER64:
		printk(KERN_CONT "%lld", ctrl->cur.val64);
		break;
	case V4L2_CTRL_TYPE_STRING:
		printk(KERN_CONT "%s", ctrl->cur.string);
		break;
	default:
		printk(KERN_CONT "unknown type %d", ctrl->type);
		break;
	}
	if (fl_inact && fl_grabbed)
		printk(KERN_CONT " (inactive, grabbed)\n");
	else if (fl_inact)
		printk(KERN_CONT " (inactive)\n");
	else if (fl_grabbed)
		printk(KERN_CONT " (grabbed)\n");
	else
		printk(KERN_CONT "\n");
}

void v4l2_ctrl_handler_log_status(struct v4l2_ctrl_handler *hdl,
				  const char *prefix)
{
	struct v4l2_ctrl *ctrl;
	const char *colon = "";
	int len;

	if (hdl == NULL)
		return;
	if (prefix == NULL)
		prefix = "";
	len = strlen(prefix);
	if (len && prefix[len - 1] != ' ')
		colon = ": ";
	mutex_lock(&hdl->lock);
	list_for_each_entry(ctrl, &hdl->ctrls, node)
		if (!(ctrl->flags & V4L2_CTRL_FLAG_DISABLED))
			log_ctrl(ctrl, prefix, colon);
	mutex_unlock(&hdl->lock);
}
EXPORT_SYMBOL(v4l2_ctrl_handler_log_status);

int v4l2_ctrl_handler_setup(struct v4l2_ctrl_handler *hdl)
{
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	if (hdl == NULL)
		return 0;
	mutex_lock(&hdl->lock);
	list_for_each_entry(ctrl, &hdl->ctrls, node)
		ctrl->done = false;

	list_for_each_entry(ctrl, &hdl->ctrls, node) {
		struct v4l2_ctrl *master = ctrl->cluster[0];
		int i;

		
		if (ctrl->done)
			continue;

		for (i = 0; i < master->ncontrols; i++) {
			if (master->cluster[i]) {
				cur_to_new(master->cluster[i]);
				master->cluster[i]->is_new = 1;
			}
		}

		
		if (ctrl->type == V4L2_CTRL_TYPE_BUTTON ||
		    (ctrl->flags & V4L2_CTRL_FLAG_READ_ONLY))
			continue;
		ret = master->ops->s_ctrl(master);
		if (ret)
			break;
		for (i = 0; i < master->ncontrols; i++)
			if (master->cluster[i])
				master->cluster[i]->done = true;
	}
	mutex_unlock(&hdl->lock);
	return ret;
}
EXPORT_SYMBOL(v4l2_ctrl_handler_setup);

int v4l2_queryctrl(struct v4l2_ctrl_handler *hdl, struct v4l2_queryctrl *qc)
{
	u32 id = qc->id & V4L2_CTRL_ID_MASK;
	struct v4l2_ctrl_ref *ref;
	struct v4l2_ctrl *ctrl;

	if (hdl == NULL)
		return -EINVAL;

	mutex_lock(&hdl->lock);

	
	ref = find_ref(hdl, id);

	if ((qc->id & V4L2_CTRL_FLAG_NEXT_CTRL) && !list_empty(&hdl->ctrl_refs)) {
		

		
		if (id >= node2id(hdl->ctrl_refs.prev)) {
			ref = NULL; 
		} else if (ref) {
			ref = list_entry(ref->node.next, typeof(*ref), node);
		} else {
			list_for_each_entry(ref, &hdl->ctrl_refs, node)
				if (id < ref->ctrl->id)
					break;
		}
	}
	mutex_unlock(&hdl->lock);
	if (!ref)
		return -EINVAL;

	ctrl = ref->ctrl;
	memset(qc, 0, sizeof(*qc));
	if (id >= V4L2_CID_PRIVATE_BASE)
		qc->id = id;
	else
		qc->id = ctrl->id;
	strlcpy(qc->name, ctrl->name, sizeof(qc->name));
	qc->minimum = ctrl->minimum;
	qc->maximum = ctrl->maximum;
	qc->default_value = ctrl->default_value;
	if (ctrl->type == V4L2_CTRL_TYPE_MENU)
		qc->step = 1;
	else
		qc->step = ctrl->step;
	qc->flags = ctrl->flags;
	qc->type = ctrl->type;
	return 0;
}
EXPORT_SYMBOL(v4l2_queryctrl);

int v4l2_subdev_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	if (qc->id & V4L2_CTRL_FLAG_NEXT_CTRL)
		return -EINVAL;
	return v4l2_queryctrl(sd->ctrl_handler, qc);
}
EXPORT_SYMBOL(v4l2_subdev_queryctrl);

int v4l2_querymenu(struct v4l2_ctrl_handler *hdl, struct v4l2_querymenu *qm)
{
	struct v4l2_ctrl *ctrl;
	u32 i = qm->index;

	ctrl = v4l2_ctrl_find(hdl, qm->id);
	if (!ctrl)
		return -EINVAL;

	qm->reserved = 0;
	
	if (ctrl->qmenu == NULL ||
	    i < ctrl->minimum || i > ctrl->maximum)
		return -EINVAL;
	
	if (ctrl->menu_skip_mask & (1 << i))
		return -EINVAL;
	
	if (ctrl->qmenu[i] == NULL || ctrl->qmenu[i][0] == '\0')
		return -EINVAL;
	strlcpy(qm->name, ctrl->qmenu[i], sizeof(qm->name));
	return 0;
}
EXPORT_SYMBOL(v4l2_querymenu);

int v4l2_subdev_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	return v4l2_querymenu(sd->ctrl_handler, qm);
}
EXPORT_SYMBOL(v4l2_subdev_querymenu);




static int prepare_ext_ctrls(struct v4l2_ctrl_handler *hdl,
			     struct v4l2_ext_controls *cs,
			     struct ctrl_helper *helpers,
			     bool try)
{
	u32 i;

	for (i = 0; i < cs->count; i++) {
		struct v4l2_ext_control *c = &cs->controls[i];
		struct v4l2_ctrl *ctrl;
		u32 id = c->id & V4L2_CTRL_ID_MASK;

		if (try)
			cs->error_idx = i;

		if (cs->ctrl_class && V4L2_CTRL_ID2CLASS(id) != cs->ctrl_class)
			return -EINVAL;

		if (id >= V4L2_CID_PRIVATE_BASE)
			return -EINVAL;
		ctrl = v4l2_ctrl_find(hdl, id);
		if (ctrl == NULL)
			return -EINVAL;
		if (ctrl->flags & V4L2_CTRL_FLAG_DISABLED)
			return -EINVAL;

		helpers[i].ctrl = ctrl;
		helpers[i].handled = false;
	}
	return 0;
}

typedef int (*cluster_func)(struct v4l2_ext_control *c,
			    struct v4l2_ctrl *ctrl);

static int cluster_walk(unsigned from,
			struct v4l2_ext_controls *cs,
			struct ctrl_helper *helpers,
			cluster_func f)
{
	struct v4l2_ctrl **cluster = helpers[from].ctrl->cluster;
	int ret = 0;
	int i;

	
	for (i = from; !ret && i < cs->count; i++) {
		struct v4l2_ctrl *ctrl = helpers[i].ctrl;

		if (!helpers[i].handled && ctrl->cluster == cluster)
			ret = f(&cs->controls[i], ctrl);
	}
	return ret;
}

static void cluster_done(unsigned from,
			 struct v4l2_ext_controls *cs,
			 struct ctrl_helper *helpers)
{
	struct v4l2_ctrl **cluster = helpers[from].ctrl->cluster;
	int i;

	
	for (i = from; i < cs->count; i++)
		if (helpers[i].ctrl->cluster == cluster)
			helpers[i].handled = true;
}

static int class_check(struct v4l2_ctrl_handler *hdl, u32 ctrl_class)
{
	if (ctrl_class == 0)
		return list_empty(&hdl->ctrl_refs) ? -EINVAL : 0;
	return find_ref_lock(hdl, ctrl_class | 1) ? 0 : -EINVAL;
}



int v4l2_g_ext_ctrls(struct v4l2_ctrl_handler *hdl, struct v4l2_ext_controls *cs)
{
	struct ctrl_helper helper[4];
	struct ctrl_helper *helpers = helper;
	int ret;
	int i;

	cs->error_idx = cs->count;
	cs->ctrl_class = V4L2_CTRL_ID2CLASS(cs->ctrl_class);

	if (hdl == NULL)
		return -EINVAL;

	if (cs->count == 0)
		return class_check(hdl, cs->ctrl_class);

	if (cs->count > ARRAY_SIZE(helper)) {
		helpers = kmalloc(sizeof(helper[0]) * cs->count, GFP_KERNEL);
		if (helpers == NULL)
			return -ENOMEM;
	}

	ret = prepare_ext_ctrls(hdl, cs, helpers, false);

	for (i = 0; !ret && i < cs->count; i++)
		if (helpers[i].ctrl->flags & V4L2_CTRL_FLAG_WRITE_ONLY)
			ret = -EACCES;

	for (i = 0; !ret && i < cs->count; i++) {
		struct v4l2_ctrl *ctrl = helpers[i].ctrl;
		struct v4l2_ctrl *master = ctrl->cluster[0];

		if (helpers[i].handled)
			continue;

		cs->error_idx = i;

		v4l2_ctrl_lock(master);
		
		if (ctrl->is_volatile && master->ops->g_volatile_ctrl)
			ret = master->ops->g_volatile_ctrl(master);
		
		if (!ret)
			ret = cluster_walk(i, cs, helpers, cur_to_user);
		v4l2_ctrl_unlock(master);
		cluster_done(i, cs, helpers);
	}

	if (cs->count > ARRAY_SIZE(helper))
		kfree(helpers);
	return ret;
}
EXPORT_SYMBOL(v4l2_g_ext_ctrls);

int v4l2_subdev_g_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *cs)
{
	return v4l2_g_ext_ctrls(sd->ctrl_handler, cs);
}
EXPORT_SYMBOL(v4l2_subdev_g_ext_ctrls);

static int get_ctrl(struct v4l2_ctrl *ctrl, s32 *val)
{
	struct v4l2_ctrl *master = ctrl->cluster[0];
	int ret = 0;

	if (ctrl->flags & V4L2_CTRL_FLAG_WRITE_ONLY)
		return -EACCES;

	v4l2_ctrl_lock(master);
	
	if (ctrl->is_volatile && master->ops->g_volatile_ctrl)
		ret = master->ops->g_volatile_ctrl(master);
	*val = ctrl->cur.val;
	v4l2_ctrl_unlock(master);
	return ret;
}

int v4l2_g_ctrl(struct v4l2_ctrl_handler *hdl, struct v4l2_control *control)
{
	struct v4l2_ctrl *ctrl = v4l2_ctrl_find(hdl, control->id);

	if (ctrl == NULL || !type_is_int(ctrl))
		return -EINVAL;
	return get_ctrl(ctrl, &control->value);
}
EXPORT_SYMBOL(v4l2_g_ctrl);

int v4l2_subdev_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	return v4l2_g_ctrl(sd->ctrl_handler, control);
}
EXPORT_SYMBOL(v4l2_subdev_g_ctrl);

s32 v4l2_ctrl_g_ctrl(struct v4l2_ctrl *ctrl)
{
	s32 val = 0;

	
	WARN_ON(!type_is_int(ctrl));
	get_ctrl(ctrl, &val);
	return val;
}
EXPORT_SYMBOL(v4l2_ctrl_g_ctrl);


static int try_or_set_control_cluster(struct v4l2_ctrl *master, bool set)
{
	bool try = !set;
	int ret = 0;
	int i;

	for (i = 0; !ret && i < master->ncontrols; i++) {
		struct v4l2_ctrl *ctrl = master->cluster[i];

		if (ctrl == NULL)
			continue;

		if (ctrl->is_new) {
			if (set && (ctrl->flags & V4L2_CTRL_FLAG_GRABBED))
				return -EBUSY;

			
			if (!set)
				ret = validate_new(ctrl);
			continue;
		}
		try = true;
		cur_to_new(ctrl);
	}

	if (!ret && master->ops->try_ctrl && try)
		ret = master->ops->try_ctrl(master);

	
	if (!ret && set && cluster_changed(master)) {
		ret = master->ops->s_ctrl(master);
		
		if (!ret)
			for (i = 0; i < master->ncontrols; i++)
				new_to_cur(master->cluster[i]);
	}
	return ret;
}

static int try_or_set_ext_ctrls(struct v4l2_ctrl_handler *hdl,
				struct v4l2_ext_controls *cs,
				struct ctrl_helper *helpers,
				bool set)
{
	unsigned i, j;
	int ret = 0;

	cs->error_idx = cs->count;
	for (i = 0; i < cs->count; i++) {
		struct v4l2_ctrl *ctrl = helpers[i].ctrl;

		if (!set)
			cs->error_idx = i;

		if (ctrl->flags & V4L2_CTRL_FLAG_READ_ONLY)
			return -EACCES;
		if (set && (ctrl->flags & V4L2_CTRL_FLAG_GRABBED))
			return -EBUSY;
	}

	for (i = 0; !ret && i < cs->count; i++) {
		struct v4l2_ctrl *ctrl = helpers[i].ctrl;
		struct v4l2_ctrl *master = ctrl->cluster[0];

		cs->error_idx = i;

		if (helpers[i].handled)
			continue;

		v4l2_ctrl_lock(ctrl);

		
		for (j = 0; j < master->ncontrols; j++)
			if (master->cluster[j])
				master->cluster[j]->is_new = 0;

		ret = cluster_walk(i, cs, helpers, user_to_new);

		if (!ret)
			ret = try_or_set_control_cluster(master, set);

		
		if (!ret)
			ret = cluster_walk(i, cs, helpers, new_to_user);

		v4l2_ctrl_unlock(ctrl);
		cluster_done(i, cs, helpers);
	}
	return ret;
}

static int try_set_ext_ctrls(struct v4l2_ctrl_handler *hdl,
			     struct v4l2_ext_controls *cs,
			     bool set)
{
	struct ctrl_helper helper[4];
	struct ctrl_helper *helpers = helper;
	int ret;
	int i;

	cs->error_idx = cs->count;
	cs->ctrl_class = V4L2_CTRL_ID2CLASS(cs->ctrl_class);

	if (hdl == NULL)
		return -EINVAL;

	if (cs->count == 0)
		return class_check(hdl, cs->ctrl_class);

	if (cs->count > ARRAY_SIZE(helper)) {
		helpers = kmalloc(sizeof(helper[0]) * cs->count, GFP_KERNEL);
		if (!helpers)
			return -ENOMEM;
	}
	ret = prepare_ext_ctrls(hdl, cs, helpers, !set);
	if (ret)
		goto free;

	
	ret = try_or_set_ext_ctrls(hdl, cs, helpers, false);
	if (set)
		cs->error_idx = cs->count;
	if (!ret && set) {
		
		for (i = 0; i < cs->count; i++)
			helpers[i].handled = false;
		ret = try_or_set_ext_ctrls(hdl, cs, helpers, true);
	}

free:
	if (cs->count > ARRAY_SIZE(helper))
		kfree(helpers);
	return ret;
}

int v4l2_try_ext_ctrls(struct v4l2_ctrl_handler *hdl, struct v4l2_ext_controls *cs)
{
	return try_set_ext_ctrls(hdl, cs, false);
}
EXPORT_SYMBOL(v4l2_try_ext_ctrls);

int v4l2_s_ext_ctrls(struct v4l2_ctrl_handler *hdl, struct v4l2_ext_controls *cs)
{
	return try_set_ext_ctrls(hdl, cs, true);
}
EXPORT_SYMBOL(v4l2_s_ext_ctrls);

int v4l2_subdev_try_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *cs)
{
	return try_set_ext_ctrls(sd->ctrl_handler, cs, false);
}
EXPORT_SYMBOL(v4l2_subdev_try_ext_ctrls);

int v4l2_subdev_s_ext_ctrls(struct v4l2_subdev *sd, struct v4l2_ext_controls *cs)
{
	return try_set_ext_ctrls(sd->ctrl_handler, cs, true);
}
EXPORT_SYMBOL(v4l2_subdev_s_ext_ctrls);

static int set_ctrl(struct v4l2_ctrl *ctrl, s32 *val)
{
	struct v4l2_ctrl *master = ctrl->cluster[0];
	int ret;
	int i;

	if (ctrl->flags & V4L2_CTRL_FLAG_READ_ONLY)
		return -EACCES;

	v4l2_ctrl_lock(ctrl);

	
	for (i = 0; i < master->ncontrols; i++)
		if (master->cluster[i])
			master->cluster[i]->is_new = 0;

	ctrl->val = *val;
	ctrl->is_new = 1;
	ret = try_or_set_control_cluster(master, false);
	if (!ret)
		ret = try_or_set_control_cluster(master, true);
	*val = ctrl->cur.val;
	v4l2_ctrl_unlock(ctrl);
	return ret;
}

int v4l2_s_ctrl(struct v4l2_ctrl_handler *hdl, struct v4l2_control *control)
{
	struct v4l2_ctrl *ctrl = v4l2_ctrl_find(hdl, control->id);

	if (ctrl == NULL || !type_is_int(ctrl))
		return -EINVAL;

	return set_ctrl(ctrl, &control->value);
}
EXPORT_SYMBOL(v4l2_s_ctrl);

int v4l2_subdev_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *control)
{
	return v4l2_s_ctrl(sd->ctrl_handler, control);
}
EXPORT_SYMBOL(v4l2_subdev_s_ctrl);

int v4l2_ctrl_s_ctrl(struct v4l2_ctrl *ctrl, s32 val)
{
	
	WARN_ON(!type_is_int(ctrl));
	return set_ctrl(ctrl, &val);
}
EXPORT_SYMBOL(v4l2_ctrl_s_ctrl);
