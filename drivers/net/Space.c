/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Holds initial configuration information for devices.
 *
 * Version:	@(#)Space.c	1.0.7	08/12/93
 *
 * Authors:	Ross Biro
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Donald J. Becker, <becker@scyld.com>
 *
 * Changelog:
 *		Stephen Hemminger (09/2003)
 *		- get rid of pre-linked dev list, dynamic device allocation
 *		Paul Gortmaker (03/2002)
 *		- struct init cleanup, enable multiple ISA autoprobes.
 *		Arnaldo Carvalho de Melo <acme@conectiva.com.br> - 09/1999
 *		- fix sbni: s/device/net_device/
 *		Paul Gortmaker (06/98):
 *		 - sort probes in a sane way, make sure all (safe) probes
 *		   get run once & failed autoprobes don't autoprobe again.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/trdevice.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/netlink.h>


extern struct net_device *ne2_probe(int unit);
extern struct net_device *hp100_probe(int unit);
extern struct net_device *ultra_probe(int unit);
extern struct net_device *ultra32_probe(int unit);
extern struct net_device *wd_probe(int unit);
extern struct net_device *el2_probe(int unit);
extern struct net_device *ne_probe(int unit);
extern struct net_device *hp_probe(int unit);
extern struct net_device *hp_plus_probe(int unit);
extern struct net_device *express_probe(int unit);
extern struct net_device *eepro_probe(int unit);
extern struct net_device *at1700_probe(int unit);
extern struct net_device *fmv18x_probe(int unit);
extern struct net_device *eth16i_probe(int unit);
extern struct net_device *i82596_probe(int unit);
extern struct net_device *ewrk3_probe(int unit);
extern struct net_device *el1_probe(int unit);
extern struct net_device *el16_probe(int unit);
extern struct net_device *elmc_probe(int unit);
extern struct net_device *elplus_probe(int unit);
extern struct net_device *ac3200_probe(int unit);
extern struct net_device *es_probe(int unit);
extern struct net_device *lne390_probe(int unit);
extern struct net_device *e2100_probe(int unit);
extern struct net_device *ni5010_probe(int unit);
extern struct net_device *ni52_probe(int unit);
extern struct net_device *ni65_probe(int unit);
extern struct net_device *sonic_probe(int unit);
extern struct net_device *seeq8005_probe(int unit);
extern struct net_device *smc_init(int unit);
extern struct net_device *atarilance_probe(int unit);
extern struct net_device *sun3lance_probe(int unit);
extern struct net_device *sun3_82586_probe(int unit);
extern struct net_device *apne_probe(int unit);
extern struct net_device *cs89x0_probe(int unit);
extern struct net_device *mvme147lance_probe(int unit);
extern struct net_device *tc515_probe(int unit);
extern struct net_device *lance_probe(int unit);
extern struct net_device *mac8390_probe(int unit);
extern struct net_device *mac89x0_probe(int unit);
extern struct net_device *mc32_probe(int unit);
extern struct net_device *cops_probe(int unit);
extern struct net_device *ltpc_probe(void);

extern struct net_device *de620_probe(int unit);

extern int iph5526_probe(struct net_device *dev);

extern int sbni_probe(int unit);

struct devprobe2 {
	struct net_device *(*probe)(int unit);
	int status;	
};

static int __init probe_list2(int unit, struct devprobe2 *p, int autoprobe)
{
	struct net_device *dev;
	for (; p->probe; p++) {
		if (autoprobe && p->status)
			continue;
		dev = p->probe(unit);
		if (!IS_ERR(dev))
			return 0;
		if (autoprobe)
			p->status = PTR_ERR(dev);
	}
	return -ENODEV;
}


static struct devprobe2 eisa_probes[] __initdata = {
#ifdef CONFIG_ULTRA32
	{ultra32_probe, 0},
#endif
#ifdef CONFIG_AC3200
	{ac3200_probe, 0},
#endif
#ifdef CONFIG_ES3210
	{es_probe, 0},
#endif
#ifdef CONFIG_LNE390
	{lne390_probe, 0},
#endif
	{NULL, 0},
};

static struct devprobe2 mca_probes[] __initdata = {
#ifdef CONFIG_NE2_MCA
	{ne2_probe, 0},
#endif
#ifdef CONFIG_ELMC		
	{elmc_probe, 0},
#endif
#ifdef CONFIG_ELMC_II		
	{mc32_probe, 0},
#endif
	{NULL, 0},
};

static struct devprobe2 isa_probes[] __initdata = {
#if defined(CONFIG_HP100) && defined(CONFIG_ISA)	
	{hp100_probe, 0},
#endif
#ifdef CONFIG_3C515
	{tc515_probe, 0},
#endif
#ifdef CONFIG_ULTRA
	{ultra_probe, 0},
#endif
#ifdef CONFIG_WD80x3
	{wd_probe, 0},
#endif
#ifdef CONFIG_EL2 		
	{el2_probe, 0},
#endif
#ifdef CONFIG_HPLAN
	{hp_probe, 0},
#endif
#ifdef CONFIG_HPLAN_PLUS
	{hp_plus_probe, 0},
#endif
#ifdef CONFIG_E2100		
	{e2100_probe, 0},
#endif
#if defined(CONFIG_NE2000) || \
    defined(CONFIG_NE_H8300)  
	{ne_probe, 0},
#endif
#ifdef CONFIG_LANCE		
	{lance_probe, 0},
#endif
#ifdef CONFIG_SMC9194
	{smc_init, 0},
#endif
#ifdef CONFIG_SEEQ8005
	{seeq8005_probe, 0},
#endif
#ifdef CONFIG_CS89x0
#ifndef CONFIG_CS89x0_PLATFORM
 	{cs89x0_probe, 0},
#endif
#endif
#ifdef CONFIG_AT1700
	{at1700_probe, 0},
#endif
#ifdef CONFIG_ETH16I
	{eth16i_probe, 0},	
#endif
#ifdef CONFIG_EEXPRESS		
	{express_probe, 0},
#endif
#ifdef CONFIG_EEXPRESS_PRO	
	{eepro_probe, 0},
#endif
#ifdef CONFIG_EWRK3             
    	{ewrk3_probe, 0},
#endif
#if defined(CONFIG_APRICOT) || defined(CONFIG_MVME16x_NET) || defined(CONFIG_BVME6000_NET)	
	{i82596_probe, 0},
#endif
#ifdef CONFIG_EL1		
	{el1_probe, 0},
#endif
#ifdef CONFIG_EL16		
	{el16_probe, 0},
#endif
#ifdef CONFIG_ELPLUS		
	{elplus_probe, 0},
#endif
#ifdef CONFIG_NI5010
	{ni5010_probe, 0},
#endif
#ifdef CONFIG_NI52
	{ni52_probe, 0},
#endif
#ifdef CONFIG_NI65
	{ni65_probe, 0},
#endif
	{NULL, 0},
};

static struct devprobe2 parport_probes[] __initdata = {
#ifdef CONFIG_DE620		
	{de620_probe, 0},
#endif
	{NULL, 0},
};

static struct devprobe2 m68k_probes[] __initdata = {
#ifdef CONFIG_ATARILANCE	
	{atarilance_probe, 0},
#endif
#ifdef CONFIG_SUN3LANCE         
	{sun3lance_probe, 0},
#endif
#ifdef CONFIG_SUN3_82586        
	{sun3_82586_probe, 0},
#endif
#ifdef CONFIG_APNE		
	{apne_probe, 0},
#endif
#ifdef CONFIG_MVME147_NET	
	{mvme147lance_probe, 0},
#endif
#ifdef CONFIG_MAC8390           
	{mac8390_probe, 0},
#endif
#ifdef CONFIG_MAC89x0
 	{mac89x0_probe, 0},
#endif
	{NULL, 0},
};


static void __init ethif_probe2(int unit)
{
	unsigned long base_addr = netdev_boot_base("eth", unit);

	if (base_addr == 1)
		return;

	(void)(	probe_list2(unit, m68k_probes, base_addr == 0) &&
		probe_list2(unit, eisa_probes, base_addr == 0) &&
		probe_list2(unit, mca_probes, base_addr == 0) &&
		probe_list2(unit, isa_probes, base_addr == 0) &&
		probe_list2(unit, parport_probes, base_addr == 0));
}

#ifdef CONFIG_TR
extern int ibmtr_probe_card(struct net_device *);
extern struct net_device *smctr_probe(int unit);

static struct devprobe2 tr_probes2[] __initdata = {
#ifdef CONFIG_SMCTR
	{smctr_probe, 0},
#endif
	{NULL, 0},
};

static __init int trif_probe(int unit)
{
	int err = -ENODEV;
#ifdef CONFIG_IBMTR
	struct net_device *dev = alloc_trdev(0);
	if (!dev)
		return -ENOMEM;

	sprintf(dev->name, "tr%d", unit);
	netdev_boot_setup_check(dev);
	err = ibmtr_probe_card(dev);
	if (err)
		free_netdev(dev);
#endif
	return err;
}

static void __init trif_probe2(int unit)
{
	unsigned long base_addr = netdev_boot_base("tr", unit);

	if (base_addr == 1)
		return;
	probe_list2(unit, tr_probes2, base_addr == 0);
}
#endif


static int __init net_olddevs_init(void)
{
	int num;

#ifdef CONFIG_SBNI
	for (num = 0; num < 8; ++num)
		sbni_probe(num);
#endif
#ifdef CONFIG_TR
	for (num = 0; num < 8; ++num)
		if (!trif_probe(num))
			trif_probe2(num);
#endif
	for (num = 0; num < 8; ++num)
		ethif_probe2(num);

#ifdef CONFIG_COPS
	cops_probe(0);
	cops_probe(1);
	cops_probe(2);
#endif
#ifdef CONFIG_LTPC
	ltpc_probe();
#endif

	return 0;
}

device_initcall(net_olddevs_init);
