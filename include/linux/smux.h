/* include/linux/smux.h
 *
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef SMUX_H
#define SMUX_H

enum {
	
	SMUX_DATA_0,
	SMUX_DATA_1,
	SMUX_DATA_2,
	SMUX_DATA_3,
	SMUX_DATA_4,
	SMUX_DATA_5,
	SMUX_DATA_6,
	SMUX_DATA_7,
	SMUX_DATA_8,
	SMUX_DATA_9,
	SMUX_USB_RMNET_DATA_0,
	SMUX_USB_DUN_0,
	SMUX_USB_DIAG_0,
	SMUX_SYS_MONITOR_0,
	SMUX_CSVT_0,
	

	
	SMUX_DATA_CTL_0 = 32,
	SMUX_DATA_CTL_1,
	SMUX_DATA_CTL_2,
	SMUX_DATA_CTL_3,
	SMUX_DATA_CTL_4,
	SMUX_DATA_CTL_5,
	SMUX_DATA_CTL_6,
	SMUX_DATA_CTL_7,
	SMUX_DATA_CTL_8,
	SMUX_DATA_CTL_9,
	SMUX_USB_RMNET_CTL_0,
	SMUX_USB_DUN_CTL_0_UNUSED,
	SMUX_USB_DIAG_CTL_0,
	SMUX_SYS_MONITOR_CTL_0,
	SMUX_CSVT_CTL_0,
	

	SMUX_TEST_LCID,
	SMUX_NUM_LOGICAL_CHANNELS,
};

enum {
	SMUX_CONNECTED,       
	SMUX_DISCONNECTED,
	SMUX_READ_DONE,
	SMUX_READ_FAIL,
	SMUX_WRITE_DONE,
	SMUX_WRITE_FAIL,
	SMUX_TIOCM_UPDATE,
	SMUX_LOW_WM_HIT,      
	SMUX_HIGH_WM_HIT,     
	SMUX_RX_RETRY_HIGH_WM_HIT,  
	SMUX_RX_RETRY_LOW_WM_HIT,   
};

enum {
	SMUX_CH_OPTION_LOCAL_LOOPBACK = 1 << 0,
	SMUX_CH_OPTION_REMOTE_LOOPBACK = 1 << 1,
	SMUX_CH_OPTION_REMOTE_TX_STOP = 1 << 2,
	SMUX_CH_OPTION_AUTO_REMOTE_TX_STOP = 1 << 3,
};

struct smux_meta_disconnected {
	int is_ssr;
};

struct smux_meta_read {
	void *pkt_priv;
	void *buffer;
	int len;
};

struct smux_meta_write {
	void *pkt_priv;
	void *buffer;
	int len;
};

struct smux_meta_tiocm {
	uint32_t tiocm_old;
	uint32_t tiocm_new;
};


#ifdef CONFIG_N_SMUX
int msm_smux_open(uint8_t lcid, void *priv,
	void (*notify)(void *priv, int event_type, const void *metadata),
	int (*get_rx_buffer)(void *priv, void **pkt_priv,
					void **buffer, int size));

int msm_smux_close(uint8_t lcid);

/**
 * Write data to a logical channel.
 *
 * @lcid      Logical channel ID
 * @pkt_priv  Client data that will be returned with the SMUX_WRITE_DONE or
 *            SMUX_WRITE_FAIL notification.
 * @data      Data to write
 * @len       Length of @data
 *
 * @returns   0 for success, <0 otherwise
 *
 * Data may be written immediately after msm_smux_open() is called, but
 * the data will wait in the transmit queue until the channel has been
 * fully opened.
 *
 * Once the data has been written, the client will receive either a completion
 * (SMUX_WRITE_DONE) or a failure notice (SMUX_WRITE_FAIL).
 */
int msm_smux_write(uint8_t lcid, void *pkt_priv, const void *data, int len);

int msm_smux_is_ch_full(uint8_t lcid);

int msm_smux_is_ch_low(uint8_t lcid);

long msm_smux_tiocm_get(uint8_t lcid);

int msm_smux_tiocm_set(uint8_t lcid, uint32_t set, uint32_t clear);

int msm_smux_set_ch_option(uint8_t lcid, uint32_t set, uint32_t clear);

#else
static inline int msm_smux_open(uint8_t lcid, void *priv,
	void (*notify)(void *priv, int event_type, const void *metadata),
	int (*get_rx_buffer)(void *priv, void **pkt_priv,
					void **buffer, int size))
{
	return -ENODEV;
}

static inline int msm_smux_close(uint8_t lcid)
{
	return -ENODEV;
}

static inline int msm_smux_write(uint8_t lcid, void *pkt_priv,
				const void *data, int len)
{
	return -ENODEV;
}

static inline int msm_smux_is_ch_full(uint8_t lcid)
{
	return -ENODEV;
}

static inline int msm_smux_is_ch_low(uint8_t lcid)
{
	return -ENODEV;
}

static inline long msm_smux_tiocm_get(uint8_t lcid)
{
	return 0;
}

static inline int msm_smux_tiocm_set(uint8_t lcid, uint32_t set, uint32_t clear)
{
	return -ENODEV;
}

static inline int msm_smux_set_ch_option(uint8_t lcid, uint32_t set,
					uint32_t clear)
{
	return -ENODEV;
}

#endif 

#endif 
