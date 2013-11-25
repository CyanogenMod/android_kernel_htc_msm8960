/*
 * include/net/dsa.h - Driver for Distributed Switch Architecture switch chips
 * Copyright (c) 2008-2009 Marvell Semiconductor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __LINUX_NET_DSA_H
#define __LINUX_NET_DSA_H

#include <linux/if_ether.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#define DSA_MAX_SWITCHES	4
#define DSA_MAX_PORTS		12

struct dsa_chip_data {
	struct device	*mii_bus;
	int		sw_addr;

	char		*port_names[DSA_MAX_PORTS];

	s8		*rtable;
};

struct dsa_platform_data {
	struct device	*netdev;

	int		nr_chips;
	struct dsa_chip_data	*chip;
};

struct dsa_switch_tree {
	struct dsa_platform_data	*pd;

	struct net_device	*master_netdev;
	__be16			tag_protocol;

	s8			cpu_switch;
	s8			cpu_port;

	int			link_poll_needed;
	struct work_struct	link_poll_work;
	struct timer_list	link_poll_timer;

	struct dsa_switch	*ds[DSA_MAX_SWITCHES];
};

struct dsa_switch {
	struct dsa_switch_tree	*dst;
	int			index;

	struct dsa_chip_data	*pd;

	struct dsa_switch_driver	*drv;

	struct mii_bus		*master_mii_bus;

	u32			dsa_port_mask;
	u32			phys_port_mask;
	struct mii_bus		*slave_mii_bus;
	struct net_device	*ports[DSA_MAX_PORTS];
};

static inline bool dsa_is_cpu_port(struct dsa_switch *ds, int p)
{
	return !!(ds->index == ds->dst->cpu_switch && p == ds->dst->cpu_port);
}

static inline u8 dsa_upstream_port(struct dsa_switch *ds)
{
	struct dsa_switch_tree *dst = ds->dst;

	if (dst->cpu_switch == ds->index)
		return dst->cpu_port;
	else
		return ds->pd->rtable[dst->cpu_switch];
}

struct dsa_switch_driver {
	struct list_head	list;

	__be16			tag_protocol;
	int			priv_size;

	char	*(*probe)(struct mii_bus *bus, int sw_addr);
	int	(*setup)(struct dsa_switch *ds);
	int	(*set_addr)(struct dsa_switch *ds, u8 *addr);

	int	(*phy_read)(struct dsa_switch *ds, int port, int regnum);
	int	(*phy_write)(struct dsa_switch *ds, int port,
			     int regnum, u16 val);

	void	(*poll_link)(struct dsa_switch *ds);

	void	(*get_strings)(struct dsa_switch *ds, int port, uint8_t *data);
	void	(*get_ethtool_stats)(struct dsa_switch *ds,
				     int port, uint64_t *data);
	int	(*get_sset_count)(struct dsa_switch *ds);
};

void register_switch_driver(struct dsa_switch_driver *type);
void unregister_switch_driver(struct dsa_switch_driver *type);

static inline bool dsa_uses_dsa_tags(struct dsa_switch_tree *dst)
{
	return !!(dst->tag_protocol == htons(ETH_P_DSA));
}

static inline bool dsa_uses_trailer_tags(struct dsa_switch_tree *dst)
{
	return !!(dst->tag_protocol == htons(ETH_P_TRAILER));
}

#endif
