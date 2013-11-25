#ifndef TPA6185_H
#define TPA6185_H

#include <linux/ioctl.h>

#define TPA6185_I2C_NAME "tpa6185"
#define SPKR_OUTPUT 0
#define HEADSET_OUTPUT 1
#define DUAL_OUTPUT 2
#define HANDSET_OUTPUT 3
#define LINEOUT_OUTPUT 4
#define MODE_CMD_LEM 9
struct tpa6185_platform_data {
	uint32_t gpio_tpa6185_spk_en;
	unsigned char spkr_cmd[7];
	unsigned char hsed_cmd[7];
	unsigned char rece_cmd[7];
	
	uint32_t gpio_tpa6185_spk_en_cpu;
};

struct tpa6185_config_data {
	unsigned int data_len;
	unsigned int mode_num;
	unsigned char *cmd_data;  
};

enum TPA6185_Mode {
	TPA6185_MODE_OFF,
	TPA6185_MODE_PLAYBACK_SPKR,
	TPA6185_MODE_PLAYBACK_HEADSET,
	TPA6185_MODE_RING,
	TPA6185_MODE_VOICECALL_SPKR,
	TPA6185_MODE_VOICECALL_HEADSET,
	TPA6185_MODE_FM_SPKR,
	TPA6185_MODE_FM_HEADSET,
	TPA6185_MODE_PLAYBACK_HANDSET,
	TPA6185_MODE_VOICECALL_HANDSET,
	TPA6185_MODE_LINEOUT,
	TPA6185_MAX_MODE
};
#define TPA6185_IOCTL_MAGIC 'a'
#define TPA6185_SET_CONFIG	_IOW(TPA6185_IOCTL_MAGIC, 0x01,	unsigned)
#define TPA6185_READ_CONFIG	_IOW(TPA6185_IOCTL_MAGIC, 0x02, unsigned)
#define TPA6185_SET_MODE        _IOW(TPA6185_IOCTL_MAGIC, 0x03, unsigned)
#define TPA6185_SET_PARAM       _IOW(TPA6185_IOCTL_MAGIC, 0x04,  unsigned)
#define TPA6185_WRITE_REG       _IOW(TPA6185_IOCTL_MAGIC, 0x07,  unsigned)

int query_tpa6185(void);
void set_speaker_amp(int on);
void set_headset_amp(int on);
void set_speaker_headset_amp(int on);
void set_handset_amp(int on);
void set_usb_audio_amp(int on);
#endif

