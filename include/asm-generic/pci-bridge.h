/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef _ASM_GENERIC_PCI_BRIDGE_H
#define _ASM_GENERIC_PCI_BRIDGE_H

#ifdef __KERNEL__

enum {
	PCI_REASSIGN_ALL_RSRC	= 0x00000001,

	
	PCI_REASSIGN_ALL_BUS	= 0x00000002,

	
	PCI_PROBE_ONLY		= 0x00000004,

	PCI_CAN_SKIP_ISA_ALIGN	= 0x00000008,

	
	PCI_ENABLE_PROC_DOMAINS	= 0x00000010,
	
	PCI_COMPAT_DOMAIN_0	= 0x00000020,
};

#ifdef CONFIG_PCI
extern unsigned int pci_flags;

static inline void pci_set_flags(int flags)
{
	pci_flags = flags;
}

static inline void pci_add_flags(int flags)
{
	pci_flags |= flags;
}

static inline void pci_clear_flags(int flags)
{
	pci_flags &= ~flags;
}

static inline int pci_has_flag(int flag)
{
	return pci_flags & flag;
}
#else
static inline void pci_set_flags(int flags) { }
static inline void pci_add_flags(int flags) { }
static inline void pci_clear_flags(int flags) { }
static inline int pci_has_flag(int flag)
{
	return 0;
}
#endif	

#endif	
#endif	
