/*
 * include/linux/serial.h
 *
 * Copyright (C) 1992 by Theodore Ts'o.
 * 
 * Redistribution of this file is permitted under the terms of the GNU 
 * Public License (GPL)
 */

#ifndef _LINUX_SERIAL_H
#define _LINUX_SERIAL_H

#include <linux/types.h>

#ifdef __KERNEL__
#include <asm/page.h>


struct async_icount {
	__u32	cts, dsr, rng, dcd, tx, rx;
	__u32	frame, parity, overrun, brk;
	__u32	buf_overrun;
};

#define SERIAL_XMIT_SIZE PAGE_SIZE

#endif

struct serial_struct {
	int	type;
	int	line;
	unsigned int	port;
	int	irq;
	int	flags;
	int	xmit_fifo_size;
	int	custom_divisor;
	int	baud_base;
	unsigned short	close_delay;
	char	io_type;
	char	reserved_char[1];
	int	hub6;
	unsigned short	closing_wait; 
	unsigned short	closing_wait2; 
	unsigned char	*iomem_base;
	unsigned short	iomem_reg_shift;
	unsigned int	port_high;
	unsigned long	iomap_base;	
};

#define ASYNC_CLOSING_WAIT_INF	0
#define ASYNC_CLOSING_WAIT_NONE	65535

#define PORT_UNKNOWN	0
#define PORT_8250	1
#define PORT_16450	2
#define PORT_16550	3
#define PORT_16550A	4
#define PORT_CIRRUS     5	
#define PORT_16650	6
#define PORT_16650V2	7
#define PORT_16750	8
#define PORT_STARTECH	9	
#define PORT_16C950	10	
#define PORT_16654	11
#define PORT_16850	12
#define PORT_RSA	13	
#define PORT_MAX	13

#define SERIAL_IO_PORT	0
#define SERIAL_IO_HUB6	1
#define SERIAL_IO_MEM	2

struct serial_uart_config {
	char	*name;
	int	dfl_xmit_fifo_size;
	int	flags;
};

#define UART_CLEAR_FIFO		0x01
#define UART_USE_FIFO		0x02
#define UART_STARTECH		0x04
#define UART_NATSEMI		0x08

#define ASYNCB_HUP_NOTIFY	 0 
#define ASYNCB_FOURPORT		 1 
#define ASYNCB_SAK		 2 
#define ASYNCB_SPLIT_TERMIOS	 3 
#define ASYNCB_SPD_HI		 4 
#define ASYNCB_SPD_VHI		 5 
#define ASYNCB_SKIP_TEST	 6 
#define ASYNCB_AUTO_IRQ		 7 
#define ASYNCB_SESSION_LOCKOUT	 8 
#define ASYNCB_PGRP_LOCKOUT	 9 
#define ASYNCB_CALLOUT_NOHUP	10 
#define ASYNCB_HARDPPS_CD	11 
#define ASYNCB_SPD_SHI		12 
#define ASYNCB_LOW_LATENCY	13 
#define ASYNCB_BUGGY_UART	14 
#define ASYNCB_AUTOPROBE	15 
#define ASYNCB_LAST_USER	15

#define ASYNCB_INITIALIZED	31 
#define ASYNCB_SUSPENDED	30 
#define ASYNCB_NORMAL_ACTIVE	29 
#define ASYNCB_BOOT_AUTOCONF	28 
#define ASYNCB_CLOSING		27 
#define ASYNCB_CTS_FLOW		26 
#define ASYNCB_CHECK_CD		25 
#define ASYNCB_SHARE_IRQ	24 
#define ASYNCB_CONS_FLOW	23 
#define ASYNCB_BOOT_ONLYMCA	22 
#define ASYNCB_FIRST_KERNEL	22

#define ASYNC_HUP_NOTIFY	(1U << ASYNCB_HUP_NOTIFY)
#define ASYNC_SUSPENDED		(1U << ASYNCB_SUSPENDED)
#define ASYNC_FOURPORT		(1U << ASYNCB_FOURPORT)
#define ASYNC_SAK		(1U << ASYNCB_SAK)
#define ASYNC_SPLIT_TERMIOS	(1U << ASYNCB_SPLIT_TERMIOS)
#define ASYNC_SPD_HI		(1U << ASYNCB_SPD_HI)
#define ASYNC_SPD_VHI		(1U << ASYNCB_SPD_VHI)
#define ASYNC_SKIP_TEST		(1U << ASYNCB_SKIP_TEST)
#define ASYNC_AUTO_IRQ		(1U << ASYNCB_AUTO_IRQ)
#define ASYNC_SESSION_LOCKOUT	(1U << ASYNCB_SESSION_LOCKOUT)
#define ASYNC_PGRP_LOCKOUT	(1U << ASYNCB_PGRP_LOCKOUT)
#define ASYNC_CALLOUT_NOHUP	(1U << ASYNCB_CALLOUT_NOHUP)
#define ASYNC_HARDPPS_CD	(1U << ASYNCB_HARDPPS_CD)
#define ASYNC_SPD_SHI		(1U << ASYNCB_SPD_SHI)
#define ASYNC_LOW_LATENCY	(1U << ASYNCB_LOW_LATENCY)
#define ASYNC_BUGGY_UART	(1U << ASYNCB_BUGGY_UART)
#define ASYNC_AUTOPROBE		(1U << ASYNCB_AUTOPROBE)

#define ASYNC_FLAGS		((1U << (ASYNCB_LAST_USER + 1)) - 1)
#define ASYNC_USR_MASK		(ASYNC_SPD_MASK|ASYNC_CALLOUT_NOHUP| \
		ASYNC_LOW_LATENCY)
#define ASYNC_SPD_CUST		(ASYNC_SPD_HI|ASYNC_SPD_VHI)
#define ASYNC_SPD_WARP		(ASYNC_SPD_HI|ASYNC_SPD_SHI)
#define ASYNC_SPD_MASK		(ASYNC_SPD_HI|ASYNC_SPD_VHI|ASYNC_SPD_SHI)

#define ASYNC_INITIALIZED	(1U << ASYNCB_INITIALIZED)
#define ASYNC_NORMAL_ACTIVE	(1U << ASYNCB_NORMAL_ACTIVE)
#define ASYNC_BOOT_AUTOCONF	(1U << ASYNCB_BOOT_AUTOCONF)
#define ASYNC_CLOSING		(1U << ASYNCB_CLOSING)
#define ASYNC_CTS_FLOW		(1U << ASYNCB_CTS_FLOW)
#define ASYNC_CHECK_CD		(1U << ASYNCB_CHECK_CD)
#define ASYNC_SHARE_IRQ		(1U << ASYNCB_SHARE_IRQ)
#define ASYNC_CONS_FLOW		(1U << ASYNCB_CONS_FLOW)
#define ASYNC_BOOT_ONLYMCA	(1U << ASYNCB_BOOT_ONLYMCA)
#define ASYNC_INTERNAL_FLAGS	(~((1U << ASYNCB_FIRST_KERNEL) - 1))

struct serial_multiport_struct {
	int		irq;
	int		port1;
	unsigned char	mask1, match1;
	int		port2;
	unsigned char	mask2, match2;
	int		port3;
	unsigned char	mask3, match3;
	int		port4;
	unsigned char	mask4, match4;
	int		port_monitor;
	int	reserved[32];
};

struct serial_icounter_struct {
	int cts, dsr, rng, dcd;
	int rx, tx;
	int frame, overrun, parity, brk;
	int buf_overrun;
	int reserved[9];
};


struct serial_rs485 {
	__u32	flags;			
#define SER_RS485_ENABLED		(1 << 0)	
#define SER_RS485_RTS_ON_SEND		(1 << 1)	
#define SER_RS485_RTS_AFTER_SEND	(1 << 2)	
#define SER_RS485_RX_DURING_TX		(1 << 4)
	__u32	delay_rts_before_send;	
	__u32	delay_rts_after_send;	
	__u32	padding[5];		
};

#ifdef __KERNEL__
#include <linux/compiler.h>

#endif 
#endif 
