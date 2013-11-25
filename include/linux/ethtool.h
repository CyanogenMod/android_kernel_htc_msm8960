/*
 * ethtool.h: Defines for Linux ethtool.
 *
 * Copyright (C) 1998 David S. Miller (davem@redhat.com)
 * Copyright 2001 Jeff Garzik <jgarzik@pobox.com>
 * Portions Copyright 2001 Sun Microsystems (thockin@sun.com)
 * Portions Copyright 2002 Intel (eli.kupermann@intel.com,
 *                                christopher.leech@intel.com,
 *                                scott.feldman@intel.com)
 * Portions Copyright (C) Sun Microsystems 2008
 */

#ifndef _LINUX_ETHTOOL_H
#define _LINUX_ETHTOOL_H

#ifdef __KERNEL__
#include <linux/compat.h>
#endif
#include <linux/types.h>
#include <linux/if_ether.h>

struct ethtool_cmd {
	__u32	cmd;
	__u32	supported;	
	__u32	advertising;	
	__u16	speed;	        
	__u8	duplex;		
	__u8	port;		
	__u8	phy_address;	
	__u8	transceiver;	
	__u8	autoneg;	
	__u8	mdio_support;	
	__u32	maxtxpkt;	
	__u32	maxrxpkt;	
	__u16	speed_hi;       
	__u8	eth_tp_mdix;
	__u8	reserved2;
	__u32	lp_advertising;	
	__u32	reserved[2];
};

static inline void ethtool_cmd_speed_set(struct ethtool_cmd *ep,
					 __u32 speed)
{

	ep->speed = (__u16)speed;
	ep->speed_hi = (__u16)(speed >> 16);
}

static inline __u32 ethtool_cmd_speed(const struct ethtool_cmd *ep)
{
	return (ep->speed_hi << 16) | ep->speed;
}

#define ETH_MDIO_SUPPORTS_C22	1

#define ETH_MDIO_SUPPORTS_C45	2

#define ETHTOOL_FWVERS_LEN	32
#define ETHTOOL_BUSINFO_LEN	32
struct ethtool_drvinfo {
	__u32	cmd;
	char	driver[32];	
	char	version[32];	
	char	fw_version[ETHTOOL_FWVERS_LEN];	
	char	bus_info[ETHTOOL_BUSINFO_LEN];	
				
	char	reserved1[32];
	char	reserved2[12];
	__u32	n_priv_flags;	
	__u32	n_stats;	
	__u32	testinfo_len;
	__u32	eedump_len;	
	__u32	regdump_len;	
};

#define SOPASS_MAX	6
struct ethtool_wolinfo {
	__u32	cmd;
	__u32	supported;
	__u32	wolopts;
	__u8	sopass[SOPASS_MAX]; 
};

struct ethtool_value {
	__u32	cmd;
	__u32	data;
};

struct ethtool_regs {
	__u32	cmd;
	__u32	version; 
	__u32	len; 
	__u8	data[0];
};

struct ethtool_eeprom {
	__u32	cmd;
	__u32	magic;
	__u32	offset; 
	__u32	len; 
	__u8	data[0];
};

struct ethtool_coalesce {
	__u32	cmd;
	__u32	rx_coalesce_usecs;
	__u32	rx_max_coalesced_frames;
	__u32	rx_coalesce_usecs_irq;
	__u32	rx_max_coalesced_frames_irq;
	__u32	tx_coalesce_usecs;
	__u32	tx_max_coalesced_frames;
	__u32	tx_coalesce_usecs_irq;
	__u32	tx_max_coalesced_frames_irq;
	__u32	stats_block_coalesce_usecs;
	__u32	use_adaptive_rx_coalesce;
	__u32	use_adaptive_tx_coalesce;
	__u32	pkt_rate_low;
	__u32	rx_coalesce_usecs_low;
	__u32	rx_max_coalesced_frames_low;
	__u32	tx_coalesce_usecs_low;
	__u32	tx_max_coalesced_frames_low;
	__u32	pkt_rate_high;
	__u32	rx_coalesce_usecs_high;
	__u32	rx_max_coalesced_frames_high;
	__u32	tx_coalesce_usecs_high;
	__u32	tx_max_coalesced_frames_high;
	__u32	rate_sample_interval;
};

struct ethtool_ringparam {
	__u32	cmd;	

	__u32	rx_max_pending;
	__u32	rx_mini_max_pending;
	__u32	rx_jumbo_max_pending;
	__u32	tx_max_pending;

	__u32	rx_pending;
	__u32	rx_mini_pending;
	__u32	rx_jumbo_pending;
	__u32	tx_pending;
};


struct ethtool_channels {
	__u32	cmd;
	__u32	max_rx;
	__u32	max_tx;
	__u32	max_other;
	__u32	max_combined;
	__u32	rx_count;
	__u32	tx_count;
	__u32	other_count;
	__u32	combined_count;
};

struct ethtool_pauseparam {
	__u32	cmd;	

	__u32	autoneg;
	__u32	rx_pause;
	__u32	tx_pause;
};

#define ETH_GSTRING_LEN		32
enum ethtool_stringset {
	ETH_SS_TEST		= 0,
	ETH_SS_STATS,
	ETH_SS_PRIV_FLAGS,
	ETH_SS_NTUPLE_FILTERS,	
	ETH_SS_FEATURES,
};

struct ethtool_gstrings {
	__u32	cmd;		
	__u32	string_set;	
	__u32	len;		
	__u8	data[0];
};

struct ethtool_sset_info {
	__u32	cmd;		
	__u32	reserved;
	__u64	sset_mask;	
				
	__u32	data[0];	
};


enum ethtool_test_flags {
	ETH_TEST_FL_OFFLINE	= (1 << 0),
	ETH_TEST_FL_FAILED	= (1 << 1),
	ETH_TEST_FL_EXTERNAL_LB	= (1 << 2),
	ETH_TEST_FL_EXTERNAL_LB_DONE	= (1 << 3),
};

struct ethtool_test {
	__u32	cmd;		
	__u32	flags;		
	__u32	reserved;
	__u32	len;		
	__u64	data[0];
};

struct ethtool_stats {
	__u32	cmd;		
	__u32	n_stats;	
	__u64	data[0];
};

struct ethtool_perm_addr {
	__u32	cmd;		
	__u32	size;
	__u8	data[0];
};

enum ethtool_flags {
	ETH_FLAG_TXVLAN		= (1 << 7),	
	ETH_FLAG_RXVLAN		= (1 << 8),	
	ETH_FLAG_LRO		= (1 << 15),	
	ETH_FLAG_NTUPLE		= (1 << 27),	
	ETH_FLAG_RXHASH		= (1 << 28),
};


struct ethtool_tcpip4_spec {
	__be32	ip4src;
	__be32	ip4dst;
	__be16	psrc;
	__be16	pdst;
	__u8    tos;
};

struct ethtool_ah_espip4_spec {
	__be32	ip4src;
	__be32	ip4dst;
	__be32	spi;
	__u8    tos;
};

#define	ETH_RX_NFC_IP4	1

struct ethtool_usrip4_spec {
	__be32	ip4src;
	__be32	ip4dst;
	__be32	l4_4_bytes;
	__u8    tos;
	__u8    ip_ver;
	__u8    proto;
};

union ethtool_flow_union {
	struct ethtool_tcpip4_spec		tcp_ip4_spec;
	struct ethtool_tcpip4_spec		udp_ip4_spec;
	struct ethtool_tcpip4_spec		sctp_ip4_spec;
	struct ethtool_ah_espip4_spec		ah_ip4_spec;
	struct ethtool_ah_espip4_spec		esp_ip4_spec;
	struct ethtool_usrip4_spec		usr_ip4_spec;
	struct ethhdr				ether_spec;
	__u8					hdata[60];
};

struct ethtool_flow_ext {
	__be16	vlan_etype;
	__be16	vlan_tci;
	__be32	data[2];
};

struct ethtool_rx_flow_spec {
	__u32		flow_type;
	union ethtool_flow_union h_u;
	struct ethtool_flow_ext h_ext;
	union ethtool_flow_union m_u;
	struct ethtool_flow_ext m_ext;
	__u64		ring_cookie;
	__u32		location;
};

struct ethtool_rxnfc {
	__u32				cmd;
	__u32				flow_type;
	__u64				data;
	struct ethtool_rx_flow_spec	fs;
	__u32				rule_cnt;
	__u32				rule_locs[0];
};

#ifdef __KERNEL__
#ifdef CONFIG_COMPAT

struct compat_ethtool_rx_flow_spec {
	u32		flow_type;
	union ethtool_flow_union h_u;
	struct ethtool_flow_ext h_ext;
	union ethtool_flow_union m_u;
	struct ethtool_flow_ext m_ext;
	compat_u64	ring_cookie;
	u32		location;
};

struct compat_ethtool_rxnfc {
	u32				cmd;
	u32				flow_type;
	compat_u64			data;
	struct compat_ethtool_rx_flow_spec fs;
	u32				rule_cnt;
	u32				rule_locs[0];
};

#endif 
#endif 

struct ethtool_rxfh_indir {
	__u32	cmd;
	__u32	size;
	__u32	ring_index[0];
};

struct ethtool_rx_ntuple_flow_spec {
	__u32		 flow_type;
	union {
		struct ethtool_tcpip4_spec		tcp_ip4_spec;
		struct ethtool_tcpip4_spec		udp_ip4_spec;
		struct ethtool_tcpip4_spec		sctp_ip4_spec;
		struct ethtool_ah_espip4_spec		ah_ip4_spec;
		struct ethtool_ah_espip4_spec		esp_ip4_spec;
		struct ethtool_usrip4_spec		usr_ip4_spec;
		struct ethhdr				ether_spec;
		__u8					hdata[72];
	} h_u, m_u;

	__u16	        vlan_tag;
	__u16	        vlan_tag_mask;
	__u64		data;
	__u64		data_mask;

	__s32		action;
#define ETHTOOL_RXNTUPLE_ACTION_DROP	(-1)	
#define ETHTOOL_RXNTUPLE_ACTION_CLEAR	(-2)	
};

struct ethtool_rx_ntuple {
	__u32					cmd;
	struct ethtool_rx_ntuple_flow_spec	fs;
};

#define ETHTOOL_FLASH_MAX_FILENAME	128
enum ethtool_flash_op_type {
	ETHTOOL_FLASH_ALL_REGIONS	= 0,
};

struct ethtool_flash {
	__u32	cmd;
	__u32	region;
	char	data[ETHTOOL_FLASH_MAX_FILENAME];
};

struct ethtool_dump {
	__u32	cmd;
	__u32	version;
	__u32	flag;
	__u32	len;
	__u8	data[0];
};


struct ethtool_get_features_block {
	__u32	available;
	__u32	requested;
	__u32	active;
	__u32	never_changed;
};

struct ethtool_gfeatures {
	__u32	cmd;
	__u32	size;
	struct ethtool_get_features_block features[0];
};

struct ethtool_set_features_block {
	__u32	valid;
	__u32	requested;
};

struct ethtool_sfeatures {
	__u32	cmd;
	__u32	size;
	struct ethtool_set_features_block features[0];
};

enum ethtool_sfeatures_retval_bits {
	ETHTOOL_F_UNSUPPORTED__BIT,
	ETHTOOL_F_WISH__BIT,
	ETHTOOL_F_COMPAT__BIT,
};

#define ETHTOOL_F_UNSUPPORTED   (1 << ETHTOOL_F_UNSUPPORTED__BIT)
#define ETHTOOL_F_WISH          (1 << ETHTOOL_F_WISH__BIT)
#define ETHTOOL_F_COMPAT        (1 << ETHTOOL_F_COMPAT__BIT)

#ifdef __KERNEL__

#include <linux/rculist.h>

extern int __ethtool_get_settings(struct net_device *dev,
				  struct ethtool_cmd *cmd);

enum ethtool_phys_id_state {
	ETHTOOL_ID_INACTIVE,
	ETHTOOL_ID_ACTIVE,
	ETHTOOL_ID_ON,
	ETHTOOL_ID_OFF
};

struct net_device;

u32 ethtool_op_get_link(struct net_device *dev);

static inline u32 ethtool_rxfh_indir_default(u32 index, u32 n_rx_rings)
{
	return index % n_rx_rings;
}

/**
 * struct ethtool_ops - optional netdev operations
 * @get_settings: Get various device settings including Ethernet link
 *	settings. The @cmd parameter is expected to have been cleared
 *	before get_settings is called. Returns a negative error code or
 *	zero.
 * @set_settings: Set various device settings including Ethernet link
 *	settings.  Returns a negative error code or zero.
 * @get_drvinfo: Report driver/device information.  Should only set the
 *	@driver, @version, @fw_version and @bus_info fields.  If not
 *	implemented, the @driver and @bus_info fields will be filled in
 *	according to the netdev's parent device.
 * @get_regs_len: Get buffer length required for @get_regs
 * @get_regs: Get device registers
 * @get_wol: Report whether Wake-on-Lan is enabled
 * @set_wol: Turn Wake-on-Lan on or off.  Returns a negative error code
 *	or zero.
 * @get_msglevel: Report driver message level.  This should be the value
 *	of the @msg_enable field used by netif logging functions.
 * @set_msglevel: Set driver message level
 * @nway_reset: Restart autonegotiation.  Returns a negative error code
 *	or zero.
 * @get_link: Report whether physical link is up.  Will only be called if
 *	the netdev is up.  Should usually be set to ethtool_op_get_link(),
 *	which uses netif_carrier_ok().
 * @get_eeprom: Read data from the device EEPROM.
 *	Should fill in the magic field.  Don't need to check len for zero
 *	or wraparound.  Fill in the data argument with the eeprom values
 *	from offset to offset + len.  Update len to the amount read.
 *	Returns an error or zero.
 * @set_eeprom: Write data to the device EEPROM.
 *	Should validate the magic field.  Don't need to check len for zero
 *	or wraparound.  Update len to the amount written.  Returns an error
 *	or zero.
 * @get_coalesce: Get interrupt coalescing parameters.  Returns a negative
 *	error code or zero.
 * @set_coalesce: Set interrupt coalescing parameters.  Returns a negative
 *	error code or zero.
 * @get_ringparam: Report ring sizes
 * @set_ringparam: Set ring sizes.  Returns a negative error code or zero.
 * @get_pauseparam: Report pause parameters
 * @set_pauseparam: Set pause parameters.  Returns a negative error code
 *	or zero.
 * @self_test: Run specified self-tests
 * @get_strings: Return a set of strings that describe the requested objects
 * @set_phys_id: Identify the physical devices, e.g. by flashing an LED
 *	attached to it.  The implementation may update the indicator
 *	asynchronously or synchronously, but in either case it must return
 *	quickly.  It is initially called with the argument %ETHTOOL_ID_ACTIVE,
 *	and must either activate asynchronous updates and return zero, return
 *	a negative error or return a positive frequency for synchronous
 *	indication (e.g. 1 for one on/off cycle per second).  If it returns
 *	a frequency then it will be called again at intervals with the
 *	argument %ETHTOOL_ID_ON or %ETHTOOL_ID_OFF and should set the state of
 *	the indicator accordingly.  Finally, it is called with the argument
 *	%ETHTOOL_ID_INACTIVE and must deactivate the indicator.  Returns a
 *	negative error code or zero.
 * @get_ethtool_stats: Return extended statistics about the device.
 *	This is only useful if the device maintains statistics not
 *	included in &struct rtnl_link_stats64.
 * @begin: Function to be called before any other operation.  Returns a
 *	negative error code or zero.
 * @complete: Function to be called after any other operation except
 *	@begin.  Will be called even if the other operation failed.
 * @get_priv_flags: Report driver-specific feature flags.
 * @set_priv_flags: Set driver-specific feature flags.  Returns a negative
 *	error code or zero.
 * @get_sset_count: Get number of strings that @get_strings will write.
 * @get_rxnfc: Get RX flow classification rules.  Returns a negative
 *	error code or zero.
 * @set_rxnfc: Set RX flow classification rules.  Returns a negative
 *	error code or zero.
 * @flash_device: Write a firmware image to device's flash memory.
 *	Returns a negative error code or zero.
 * @reset: Reset (part of) the device, as specified by a bitmask of
 *	flags from &enum ethtool_reset_flags.  Returns a negative
 *	error code or zero.
 * @get_rxfh_indir_size: Get the size of the RX flow hash indirection table.
 *	Returns zero if not supported for this specific device.
 * @get_rxfh_indir: Get the contents of the RX flow hash indirection table.
 *	Will not be called if @get_rxfh_indir_size returns zero.
 *	Returns a negative error code or zero.
 * @set_rxfh_indir: Set the contents of the RX flow hash indirection table.
 *	Will not be called if @get_rxfh_indir_size returns zero.
 *	Returns a negative error code or zero.
 * @get_channels: Get number of channels.
 * @set_channels: Set number of channels.  Returns a negative error code or
 *	zero.
 * @get_dump_flag: Get dump flag indicating current dump length, version,
 * 		   and flag of the device.
 * @get_dump_data: Get dump data.
 * @set_dump: Set dump specific flags to the device.
 *
 * All operations are optional (i.e. the function pointer may be set
 * to %NULL) and callers must take this into account.  Callers must
 * hold the RTNL lock.
 *
 * See the structures used by these operations for further documentation.
 *
 * See &struct net_device and &struct net_device_ops for documentation
 * of the generic netdev features interface.
 */
struct ethtool_ops {
	int	(*get_settings)(struct net_device *, struct ethtool_cmd *);
	int	(*set_settings)(struct net_device *, struct ethtool_cmd *);
	void	(*get_drvinfo)(struct net_device *, struct ethtool_drvinfo *);
	int	(*get_regs_len)(struct net_device *);
	void	(*get_regs)(struct net_device *, struct ethtool_regs *, void *);
	void	(*get_wol)(struct net_device *, struct ethtool_wolinfo *);
	int	(*set_wol)(struct net_device *, struct ethtool_wolinfo *);
	u32	(*get_msglevel)(struct net_device *);
	void	(*set_msglevel)(struct net_device *, u32);
	int	(*nway_reset)(struct net_device *);
	u32	(*get_link)(struct net_device *);
	int	(*get_eeprom_len)(struct net_device *);
	int	(*get_eeprom)(struct net_device *,
			      struct ethtool_eeprom *, u8 *);
	int	(*set_eeprom)(struct net_device *,
			      struct ethtool_eeprom *, u8 *);
	int	(*get_coalesce)(struct net_device *, struct ethtool_coalesce *);
	int	(*set_coalesce)(struct net_device *, struct ethtool_coalesce *);
	void	(*get_ringparam)(struct net_device *,
				 struct ethtool_ringparam *);
	int	(*set_ringparam)(struct net_device *,
				 struct ethtool_ringparam *);
	void	(*get_pauseparam)(struct net_device *,
				  struct ethtool_pauseparam*);
	int	(*set_pauseparam)(struct net_device *,
				  struct ethtool_pauseparam*);
	void	(*self_test)(struct net_device *, struct ethtool_test *, u64 *);
	void	(*get_strings)(struct net_device *, u32 stringset, u8 *);
	int	(*set_phys_id)(struct net_device *, enum ethtool_phys_id_state);
	void	(*get_ethtool_stats)(struct net_device *,
				     struct ethtool_stats *, u64 *);
	int	(*begin)(struct net_device *);
	void	(*complete)(struct net_device *);
	u32	(*get_priv_flags)(struct net_device *);
	int	(*set_priv_flags)(struct net_device *, u32);
	int	(*get_sset_count)(struct net_device *, int);
	int	(*get_rxnfc)(struct net_device *,
			     struct ethtool_rxnfc *, u32 *rule_locs);
	int	(*set_rxnfc)(struct net_device *, struct ethtool_rxnfc *);
	int	(*flash_device)(struct net_device *, struct ethtool_flash *);
	int	(*reset)(struct net_device *, u32 *);
	u32	(*get_rxfh_indir_size)(struct net_device *);
	int	(*get_rxfh_indir)(struct net_device *, u32 *);
	int	(*set_rxfh_indir)(struct net_device *, const u32 *);
	void	(*get_channels)(struct net_device *, struct ethtool_channels *);
	int	(*set_channels)(struct net_device *, struct ethtool_channels *);
	int	(*get_dump_flag)(struct net_device *, struct ethtool_dump *);
	int	(*get_dump_data)(struct net_device *,
				 struct ethtool_dump *, void *);
	int	(*set_dump)(struct net_device *, struct ethtool_dump *);

};
#endif 

#define ETHTOOL_GSET		0x00000001 
#define ETHTOOL_SSET		0x00000002 
#define ETHTOOL_GDRVINFO	0x00000003 
#define ETHTOOL_GREGS		0x00000004 
#define ETHTOOL_GWOL		0x00000005 
#define ETHTOOL_SWOL		0x00000006 
#define ETHTOOL_GMSGLVL		0x00000007 
#define ETHTOOL_SMSGLVL		0x00000008 
#define ETHTOOL_NWAY_RST	0x00000009 
#define ETHTOOL_GLINK		0x0000000a
#define ETHTOOL_GEEPROM		0x0000000b 
#define ETHTOOL_SEEPROM		0x0000000c 
#define ETHTOOL_GCOALESCE	0x0000000e 
#define ETHTOOL_SCOALESCE	0x0000000f 
#define ETHTOOL_GRINGPARAM	0x00000010 
#define ETHTOOL_SRINGPARAM	0x00000011 
#define ETHTOOL_GPAUSEPARAM	0x00000012 
#define ETHTOOL_SPAUSEPARAM	0x00000013 
#define ETHTOOL_GRXCSUM		0x00000014 
#define ETHTOOL_SRXCSUM		0x00000015 
#define ETHTOOL_GTXCSUM		0x00000016 
#define ETHTOOL_STXCSUM		0x00000017 
#define ETHTOOL_GSG		0x00000018 
#define ETHTOOL_SSG		0x00000019 
#define ETHTOOL_TEST		0x0000001a 
#define ETHTOOL_GSTRINGS	0x0000001b 
#define ETHTOOL_PHYS_ID		0x0000001c 
#define ETHTOOL_GSTATS		0x0000001d 
#define ETHTOOL_GTSO		0x0000001e 
#define ETHTOOL_STSO		0x0000001f 
#define ETHTOOL_GPERMADDR	0x00000020 
#define ETHTOOL_GUFO		0x00000021 
#define ETHTOOL_SUFO		0x00000022 
#define ETHTOOL_GGSO		0x00000023 
#define ETHTOOL_SGSO		0x00000024 
#define ETHTOOL_GFLAGS		0x00000025 
#define ETHTOOL_SFLAGS		0x00000026 
#define ETHTOOL_GPFLAGS		0x00000027 
#define ETHTOOL_SPFLAGS		0x00000028 

#define ETHTOOL_GRXFH		0x00000029 
#define ETHTOOL_SRXFH		0x0000002a 
#define ETHTOOL_GGRO		0x0000002b 
#define ETHTOOL_SGRO		0x0000002c 
#define ETHTOOL_GRXRINGS	0x0000002d 
#define ETHTOOL_GRXCLSRLCNT	0x0000002e 
#define ETHTOOL_GRXCLSRULE	0x0000002f 
#define ETHTOOL_GRXCLSRLALL	0x00000030 
#define ETHTOOL_SRXCLSRLDEL	0x00000031 
#define ETHTOOL_SRXCLSRLINS	0x00000032 
#define ETHTOOL_FLASHDEV	0x00000033 
#define ETHTOOL_RESET		0x00000034 
#define ETHTOOL_SRXNTUPLE	0x00000035 
#define ETHTOOL_GRXNTUPLE	0x00000036 
#define ETHTOOL_GSSET_INFO	0x00000037 
#define ETHTOOL_GRXFHINDIR	0x00000038 
#define ETHTOOL_SRXFHINDIR	0x00000039 

#define ETHTOOL_GFEATURES	0x0000003a 
#define ETHTOOL_SFEATURES	0x0000003b 
#define ETHTOOL_GCHANNELS	0x0000003c 
#define ETHTOOL_SCHANNELS	0x0000003d 
#define ETHTOOL_SET_DUMP	0x0000003e 
#define ETHTOOL_GET_DUMP_FLAG	0x0000003f 
#define ETHTOOL_GET_DUMP_DATA	0x00000040 

#define SPARC_ETH_GSET		ETHTOOL_GSET
#define SPARC_ETH_SSET		ETHTOOL_SSET

#define SUPPORTED_10baseT_Half		(1 << 0)
#define SUPPORTED_10baseT_Full		(1 << 1)
#define SUPPORTED_100baseT_Half		(1 << 2)
#define SUPPORTED_100baseT_Full		(1 << 3)
#define SUPPORTED_1000baseT_Half	(1 << 4)
#define SUPPORTED_1000baseT_Full	(1 << 5)
#define SUPPORTED_Autoneg		(1 << 6)
#define SUPPORTED_TP			(1 << 7)
#define SUPPORTED_AUI			(1 << 8)
#define SUPPORTED_MII			(1 << 9)
#define SUPPORTED_FIBRE			(1 << 10)
#define SUPPORTED_BNC			(1 << 11)
#define SUPPORTED_10000baseT_Full	(1 << 12)
#define SUPPORTED_Pause			(1 << 13)
#define SUPPORTED_Asym_Pause		(1 << 14)
#define SUPPORTED_2500baseX_Full	(1 << 15)
#define SUPPORTED_Backplane		(1 << 16)
#define SUPPORTED_1000baseKX_Full	(1 << 17)
#define SUPPORTED_10000baseKX4_Full	(1 << 18)
#define SUPPORTED_10000baseKR_Full	(1 << 19)
#define SUPPORTED_10000baseR_FEC	(1 << 20)
#define SUPPORTED_20000baseMLD2_Full	(1 << 21)
#define SUPPORTED_20000baseKR2_Full	(1 << 22)

#define ADVERTISED_10baseT_Half		(1 << 0)
#define ADVERTISED_10baseT_Full		(1 << 1)
#define ADVERTISED_100baseT_Half	(1 << 2)
#define ADVERTISED_100baseT_Full	(1 << 3)
#define ADVERTISED_1000baseT_Half	(1 << 4)
#define ADVERTISED_1000baseT_Full	(1 << 5)
#define ADVERTISED_Autoneg		(1 << 6)
#define ADVERTISED_TP			(1 << 7)
#define ADVERTISED_AUI			(1 << 8)
#define ADVERTISED_MII			(1 << 9)
#define ADVERTISED_FIBRE		(1 << 10)
#define ADVERTISED_BNC			(1 << 11)
#define ADVERTISED_10000baseT_Full	(1 << 12)
#define ADVERTISED_Pause		(1 << 13)
#define ADVERTISED_Asym_Pause		(1 << 14)
#define ADVERTISED_2500baseX_Full	(1 << 15)
#define ADVERTISED_Backplane		(1 << 16)
#define ADVERTISED_1000baseKX_Full	(1 << 17)
#define ADVERTISED_10000baseKX4_Full	(1 << 18)
#define ADVERTISED_10000baseKR_Full	(1 << 19)
#define ADVERTISED_10000baseR_FEC	(1 << 20)
#define ADVERTISED_20000baseMLD2_Full	(1 << 21)
#define ADVERTISED_20000baseKR2_Full	(1 << 22)


#define SPEED_10		10
#define SPEED_100		100
#define SPEED_1000		1000
#define SPEED_2500		2500
#define SPEED_10000		10000
#define SPEED_UNKNOWN		-1

#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01
#define DUPLEX_UNKNOWN		0xff

#define PORT_TP			0x00
#define PORT_AUI		0x01
#define PORT_MII		0x02
#define PORT_FIBRE		0x03
#define PORT_BNC		0x04
#define PORT_DA			0x05
#define PORT_NONE		0xef
#define PORT_OTHER		0xff

#define XCVR_INTERNAL		0x00
#define XCVR_EXTERNAL		0x01
#define XCVR_DUMMY1		0x02
#define XCVR_DUMMY2		0x03
#define XCVR_DUMMY3		0x04

#define AUTONEG_DISABLE		0x00
#define AUTONEG_ENABLE		0x01

#define ETH_TP_MDI_INVALID	0x00
#define ETH_TP_MDI		0x01
#define ETH_TP_MDI_X		0x02

#define WAKE_PHY		(1 << 0)
#define WAKE_UCAST		(1 << 1)
#define WAKE_MCAST		(1 << 2)
#define WAKE_BCAST		(1 << 3)
#define WAKE_ARP		(1 << 4)
#define WAKE_MAGIC		(1 << 5)
#define WAKE_MAGICSECURE	(1 << 6) 

#define	TCP_V4_FLOW	0x01	
#define	UDP_V4_FLOW	0x02	
#define	SCTP_V4_FLOW	0x03	
#define	AH_ESP_V4_FLOW	0x04	
#define	TCP_V6_FLOW	0x05	
#define	UDP_V6_FLOW	0x06	
#define	SCTP_V6_FLOW	0x07	
#define	AH_ESP_V6_FLOW	0x08	
#define	AH_V4_FLOW	0x09	
#define	ESP_V4_FLOW	0x0a	
#define	AH_V6_FLOW	0x0b	
#define	ESP_V6_FLOW	0x0c	
#define	IP_USER_FLOW	0x0d	
#define	IPV4_FLOW	0x10	
#define	IPV6_FLOW	0x11	
#define	ETHER_FLOW	0x12	
#define	FLOW_EXT	0x80000000

#define	RXH_L2DA	(1 << 1)
#define	RXH_VLAN	(1 << 2)
#define	RXH_L3_PROTO	(1 << 3)
#define	RXH_IP_SRC	(1 << 4)
#define	RXH_IP_DST	(1 << 5)
#define	RXH_L4_B_0_1	(1 << 6) 
#define	RXH_L4_B_2_3	(1 << 7) 
#define	RXH_DISCARD	(1 << 31)

#define	RX_CLS_FLOW_DISC	0xffffffffffffffffULL

#define RX_CLS_LOC_SPECIAL	0x80000000	
#define RX_CLS_LOC_ANY		0xffffffff
#define RX_CLS_LOC_FIRST	0xfffffffe
#define RX_CLS_LOC_LAST		0xfffffffd

enum ethtool_reset_flags {
	ETH_RESET_MGMT		= 1 << 0,	
	ETH_RESET_IRQ		= 1 << 1,	
	ETH_RESET_DMA		= 1 << 2,	
	ETH_RESET_FILTER	= 1 << 3,	
	ETH_RESET_OFFLOAD	= 1 << 4,	
	ETH_RESET_MAC		= 1 << 5,	
	ETH_RESET_PHY		= 1 << 6,	
	ETH_RESET_RAM		= 1 << 7,	

	ETH_RESET_DEDICATED	= 0x0000ffff,	
	ETH_RESET_ALL		= 0xffffffff,	
};
#define ETH_RESET_SHARED_SHIFT	16

#endif 
