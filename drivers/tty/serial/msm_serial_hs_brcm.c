/* drivers/serial/msm_serial_hs.c
 *
 * MSM 7k High speed uart driver
 *
 * Copyright (c) 2008 Google Inc.
 * Copyright (c) 2007-2012, Code Aurora Forum. All rights reserved.
 * Modified: Nick Pelly <npelly@google.com>
 *
 * All source code in this file is licensed under the following license
 * except where indicated.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Has optional support for uart power management independent of linux
 * suspend/resume:
 *
 * RX wakeup.
 * UART wakeup can be triggered by RX activity (using a wakeup GPIO on the
 * UART RX pin). This should only be used if there is not a wakeup
 * GPIO on the UART CTS, and the first RX byte is known (for example, with the
 * Bluetooth Texas Instruments HCILL protocol), since the first RX byte will
 * always be lost. RTS will be asserted even while the UART is off in this mode
 * of operation. See msm_serial_hs_platform_data.rx_wakeup_irq.
 */

#include <linux/module.h>

#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/tty_flip.h>
#include <linux/wait.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/device.h>
#include <linux/wakelock.h>
#include <asm/atomic.h>
#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/msm_serial_hs.h>

#include "msm_serial_hs_hwreg.h"

#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

static int hs_serial_debug_mask = 1;
module_param_named(debug_mask, hs_serial_debug_mask,
		   int, S_IRUGO | S_IWUSR | S_IWGRP);

#define USE_BCM_BT_CHIP
#define T_LOW 0
#define T_HIGH 1
#define BT_SERIAL_DBG


enum flush_reason {
	FLUSH_NONE,
	FLUSH_DATA_READY,
	FLUSH_DATA_INVALID,  
	FLUSH_IGNORE = FLUSH_DATA_INVALID,
	FLUSH_STOP,
	FLUSH_SHUTDOWN,
};

enum msm_hs_clk_states_e {
	MSM_HS_CLK_PORT_OFF,     
	MSM_HS_CLK_OFF,          
	MSM_HS_CLK_REQUEST_OFF,  
	MSM_HS_CLK_ON,           
};

enum msm_hs_clk_req_off_state_e {
	CLK_REQ_OFF_START,
	CLK_REQ_OFF_RXSTALE_ISSUED,
	CLK_REQ_OFF_FLUSH_ISSUED,
	CLK_REQ_OFF_RXSTALE_FLUSHED,
};

struct msm_hs_tx {
	unsigned int tx_ready_int_en;  
	unsigned int dma_in_flight;    
	struct msm_dmov_cmd xfer;
	dmov_box *command_ptr;
	u32 *command_ptr_ptr;
	dma_addr_t mapped_cmd_ptr;
	dma_addr_t mapped_cmd_ptr_ptr;
	int tx_count;
	dma_addr_t dma_base;
	struct tasklet_struct tlet;
#ifdef USE_BCM_BT_CHIP	
	struct wake_lock brcm_tx_wake_lock;
#endif
};

struct msm_hs_rx {
	enum flush_reason flush;
	struct msm_dmov_cmd xfer;
	dma_addr_t cmdptr_dmaaddr;
	dmov_box *command_ptr;
	u32 *command_ptr_ptr;
	dma_addr_t mapped_cmd_ptr;
	wait_queue_head_t wait;
	dma_addr_t rbuffer;
	unsigned char *buffer;
	unsigned int buffer_pending;
	struct dma_pool *pool;
	struct wake_lock wake_lock;
	struct delayed_work flip_insert_work;
	struct tasklet_struct tlet;
#ifdef USE_BCM_BT_CHIP	
	struct wake_lock brcm_rx_wake_lock;
	unsigned int is_brcm_rx_wake_locked;
#endif
};

enum buffer_states {
	NONE_PENDING = 0x0,
	FIFO_OVERRUN = 0x1,
	PARITY_ERROR = 0x2,
	CHARS_NORMAL = 0x4,
};

struct msm_hs_wakeup {
	int irq;  
	unsigned char ignore;  

	
	unsigned char inject_rx;
	char rx_to_inject;
};

struct msm_hs_port {
	struct uart_port uport;
	unsigned long imr_reg;  
	struct clk *clk;
	struct clk *pclk;
	struct msm_hs_tx tx;
	struct msm_hs_rx rx;
	
	
	
	unsigned char __iomem	*mapped_gsbi;
	int dma_tx_channel;
	int dma_rx_channel;
	int dma_tx_crci;
	int dma_rx_crci;
	struct hrtimer clk_off_timer;  
	ktime_t clk_off_delay;
	enum msm_hs_clk_states_e clk_state;
	enum msm_hs_clk_req_off_state_e clk_req_off_state;

	struct msm_hs_wakeup wakeup;
	struct wake_lock dma_wake_lock;  

	
	struct work_struct clock_off_w; 
	struct workqueue_struct *hsuart_wq; 
	struct mutex clk_mutex; 

#ifdef USE_BCM_BT_CHIP	
	unsigned char bt_wakeup_pin;
	unsigned char bt_wakeup_level;
	unsigned char bt_wakeup_assert_inadvance;
	unsigned char host_wakeup_pin;
	unsigned char host_wakeup_level;
	unsigned char host_want_sleep;
	
#endif
};

#define MSM_UARTDM_BURST_SIZE 16   
#define UARTDM_TX_BUF_SIZE UART_XMIT_SIZE
#define UARTDM_RX_BUF_SIZE 512
#define RETRY_TIMEOUT 5
#define UARTDM_NR 2

static struct msm_hs_port q_uart_port[UARTDM_NR];
static struct platform_driver msm_serial_hs_platform_driver;
static struct uart_driver msm_hs_driver;
static struct uart_ops msm_hs_ops;

#define UARTDM_TO_MSM(uart_port) \
	container_of((uart_port), struct msm_hs_port, uport)

static ssize_t show_clock(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
#if 0
	int state = 1;
	enum msm_hs_clk_states_e clk_state;
	unsigned long flags;

	struct platform_device *pdev = container_of(dev, struct
						    platform_device, dev);
	struct msm_hs_port *msm_uport = &q_uart_port[pdev->id];

	spin_lock_irqsave(&msm_uport->uport.lock, flags);
	clk_state = msm_uport->clk_state;
	spin_unlock_irqrestore(&msm_uport->uport.lock, flags);

	if (clk_state <= MSM_HS_CLK_OFF)
		state = 0;

	return snprintf(buf, PAGE_SIZE, "%d\n", state);
#else
	return 0;
#endif
}

static ssize_t set_clock(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
#if 0
	int state;
	struct platform_device *pdev = container_of(dev, struct
						    platform_device, dev);
	struct msm_hs_port *msm_uport = &q_uart_port[pdev->id];

	state = buf[0] - '0';
	switch (state) {
	case 0: {
		msm_hs_request_clock_off(&msm_uport->uport);
		break;
	}
	case 1: {
		msm_hs_request_clock_on(&msm_uport->uport);
		break;
	}
	default: {
		return -EINVAL;
	}
	}
	return count;
#else
	return 0;
#endif
}

static DEVICE_ATTR(clock, S_IWUSR | S_IRUGO, show_clock, set_clock);

static inline unsigned int use_low_power_wakeup(struct msm_hs_port *msm_uport)
{
	return (msm_uport->wakeup.irq > 0);
}

static inline int is_gsbi_uart(struct msm_hs_port *msm_uport)
{
	
	return ((msm_uport->mapped_gsbi != NULL));
}

static inline unsigned int msm_hs_read(struct uart_port *uport,
				       unsigned int offset)
{
	return readl_relaxed(uport->membase + offset);
}

static inline void msm_hs_write(struct uart_port *uport, unsigned int offset,
				 unsigned int value)
{
	writel_relaxed(value, uport->membase + offset);
}

static void msm_hs_release_port(struct uart_port *port)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(port);
	struct platform_device *pdev = to_platform_device(port->dev);
	struct resource *gsbi_resource;
	resource_size_t size;

	if (is_gsbi_uart(msm_uport)) {
		iowrite32(GSBI_PROTOCOL_IDLE, msm_uport->mapped_gsbi +
			  GSBI_CONTROL_ADDR);
		gsbi_resource = platform_get_resource_byname(pdev,
							     IORESOURCE_MEM,
							     "gsbi_resource");
		if (unlikely(!gsbi_resource))
			return;

		size = resource_size(gsbi_resource);
		release_mem_region(gsbi_resource->start, size);
		iounmap(msm_uport->mapped_gsbi);
		msm_uport->mapped_gsbi = NULL;
	}
}

static int msm_hs_request_port(struct uart_port *port)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(port);
	struct platform_device *pdev = to_platform_device(port->dev);
	struct resource *gsbi_resource;
	resource_size_t size;

	gsbi_resource = platform_get_resource_byname(pdev,
						     IORESOURCE_MEM,
						     "gsbi_resource");
	if (gsbi_resource) {
		size = resource_size(gsbi_resource);
		if (unlikely(!request_mem_region(gsbi_resource->start, size,
						 "msm_serial_hs")))
			return -EBUSY;
		msm_uport->mapped_gsbi = ioremap(gsbi_resource->start,
						 size);
		if (!msm_uport->mapped_gsbi) {
			release_mem_region(gsbi_resource->start, size);
			return -EBUSY;
		}
	}
	
	return 0;
}

#if 0
static int msm_serial_loopback_enable_set(void *data, u64 val)
{
	struct msm_hs_port *msm_uport = data;
	struct uart_port *uport = &(msm_uport->uport);
	unsigned long flags;
	int ret = 0;

	clk_prepare_enable(msm_uport->clk);
	if (msm_uport->pclk)
		clk_prepare_enable(msm_uport->pclk);

	if (val) {
		spin_lock_irqsave(&uport->lock, flags);
		ret = msm_hs_read(uport, UARTDM_MR2_ADDR);
		ret |= UARTDM_MR2_LOOP_MODE_BMSK;
		msm_hs_write(uport, UARTDM_MR2_ADDR, ret);
		spin_unlock_irqrestore(&uport->lock, flags);
	} else {
		spin_lock_irqsave(&uport->lock, flags);
		ret = msm_hs_read(uport, UARTDM_MR2_ADDR);
		ret &= ~UARTDM_MR2_LOOP_MODE_BMSK;
		msm_hs_write(uport, UARTDM_MR2_ADDR, ret);
		spin_unlock_irqrestore(&uport->lock, flags);
	}
	
	mb();
	clk_disable_unprepare(msm_uport->clk);
	if (msm_uport->pclk)
		clk_disable_unprepare(msm_uport->pclk);

	return 0;
}

static int msm_serial_loopback_enable_get(void *data, u64 *val)
{
	struct msm_hs_port *msm_uport = data;
	struct uart_port *uport = &(msm_uport->uport);
	unsigned long flags;
	int ret = 0;

	clk_prepare_enable(msm_uport->clk);
	if (msm_uport->pclk)
		clk_prepare_enable(msm_uport->pclk);

	spin_lock_irqsave(&uport->lock, flags);
	ret = msm_hs_read(&msm_uport->uport, UARTDM_MR2_ADDR);
	spin_unlock_irqrestore(&uport->lock, flags);

	clk_disable_unprepare(msm_uport->clk);
	if (msm_uport->pclk)
		clk_disable_unprepare(msm_uport->pclk);

	*val = (ret & UARTDM_MR2_LOOP_MODE_BMSK) ? 1 : 0;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(loopback_enable_fops, msm_serial_loopback_enable_get,
			msm_serial_loopback_enable_set, "%llu\n");

static void __devinit msm_serial_debugfs_init(struct msm_hs_port *msm_uport,
					   int id)
{
	char node_name[15];
	snprintf(node_name, sizeof(node_name), "loopback.%d", id);
	msm_uport->loopback_dir = debugfs_create_file(node_name,
						S_IRUGO | S_IWUSR,
						debug_base,
						msm_uport,
						&loopback_enable_fops);

	if (IS_ERR_OR_NULL(msm_uport->loopback_dir))
		pr_err("%s(): Cannot create loopback.%d debug entry",
							__func__, id);
}
#endif

static int __devexit msm_hs_remove(struct platform_device *pdev)
{

	struct msm_hs_port *msm_uport;
	struct device *dev;
	struct msm_serial_hs_platform_data *pdata = pdev->dev.platform_data;


	if (pdev->id < 0 || pdev->id >= UARTDM_NR) {
		printk(KERN_ERR "[BT]Invalid plaform device ID = %d\n",
				pdev->id);
		return -EINVAL;
	}

	msm_uport = &q_uart_port[pdev->id];
	dev = msm_uport->uport.dev;

	if (pdata && pdata->gpio_config)
		if (pdata->gpio_config(0))
			dev_err(dev, "GPIO config error\n");

	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_clock.attr);
	

	dma_unmap_single(dev, msm_uport->rx.mapped_cmd_ptr, sizeof(dmov_box),
			 DMA_TO_DEVICE);
	dma_pool_free(msm_uport->rx.pool, msm_uport->rx.buffer,
		      msm_uport->rx.rbuffer);
	dma_pool_destroy(msm_uport->rx.pool);

	dma_unmap_single(dev, msm_uport->rx.cmdptr_dmaaddr, sizeof(u32),
			 DMA_TO_DEVICE);
	dma_unmap_single(dev, msm_uport->tx.mapped_cmd_ptr_ptr, sizeof(u32),
			 DMA_TO_DEVICE);
	dma_unmap_single(dev, msm_uport->tx.mapped_cmd_ptr, sizeof(dmov_box),
			 DMA_TO_DEVICE);

#ifdef USE_BCM_BT_CHIP
	
	wake_lock_destroy(&msm_uport->tx.brcm_tx_wake_lock);
	
	wake_lock_destroy(&msm_uport->rx.brcm_rx_wake_lock);
#endif

	wake_lock_destroy(&msm_uport->rx.wake_lock);
	wake_lock_destroy(&msm_uport->dma_wake_lock);
	destroy_workqueue(msm_uport->hsuart_wq);
	mutex_destroy(&msm_uport->clk_mutex);

	uart_remove_one_port(&msm_hs_driver, &msm_uport->uport);
	clk_put(msm_uport->clk);
	if (msm_uport->pclk)
		clk_put(msm_uport->pclk);

	
	kfree(msm_uport->tx.command_ptr);
	kfree(msm_uport->tx.command_ptr_ptr);

	
	kfree(msm_uport->rx.command_ptr);
	kfree(msm_uport->rx.command_ptr_ptr);

	iounmap(msm_uport->uport.membase);

	return 0;
}

static int msm_hs_init_clk(struct uart_port *uport)
{
	int ret;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	
	ret = clk_set_rate(msm_uport->clk, uport->uartclk);
	if (ret) {
		printk(KERN_WARNING "[BT]Error setting clock rate on UART\n");
		return ret;
	}

	ret = clk_prepare_enable(msm_uport->clk);
	if (ret) {
		printk(KERN_ERR "[BT]Error could not turn on UART clk\n");
		return ret;
	}
	if (msm_uport->pclk) {
		ret = clk_prepare_enable(msm_uport->pclk);
		if (ret) {
			clk_disable_unprepare(msm_uport->clk);
			dev_err(uport->dev,
				"[BT]Error could not turn on UART pclk\n");
			return ret;
		}
	}

	msm_uport->clk_state = MSM_HS_CLK_ON;
	return 0;
}

static void msm_hs_set_bps_locked(struct uart_port *uport,
			       unsigned int bps)
{
	unsigned long rxstale;
	unsigned long data;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	switch (bps) {
	case 300:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x00);
		rxstale = 1;
		break;
	case 600:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x11);
		rxstale = 1;
		break;
	case 1200:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x22);
		rxstale = 1;
		break;
	case 2400:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x33);
		rxstale = 1;
		break;
	case 4800:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x44);
		rxstale = 1;
		break;
	case 9600:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x55);
		rxstale = 2;
		break;
	case 14400:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x66);
		rxstale = 3;
		break;
	case 19200:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x77);
		rxstale = 4;
		break;
	case 28800:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x88);
		rxstale = 6;
		break;
	case 38400:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x99);
		rxstale = 8;
		break;
	case 57600:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xaa);
		rxstale = 16;
		break;
	case 76800:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xbb);
		rxstale = 16;
		break;
	case 115200:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xcc);
		rxstale = 31;
		break;
	case 230400:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xee);
		rxstale = 31;
		break;
	case 460800:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xff);
		rxstale = 31;
		break;
	case 4000000:
	case 3686400:
	case 3200000:
	case 3500000:
	case 3000000:
	case 2500000:
	case 1500000:
	case 1152000:
	case 1000000:
	case 921600:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xff);
		rxstale = 31;
		break;
	default:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xff);
		
		bps = 9600;
		rxstale = 2;
		break;
	}
	mb();
	if (bps > 460800)
		uport->uartclk = bps * 16;
	else
		uport->uartclk = 7372800;

	if (clk_set_rate(msm_uport->clk, uport->uartclk)) {
		printk(KERN_WARNING "[BT]Error setting clock rate on UART\n");
		return;
	}

	data = rxstale & UARTDM_IPR_STALE_LSB_BMSK;
	data |= UARTDM_IPR_STALE_TIMEOUT_MSB_BMSK & (rxstale << 2);

	msm_hs_write(uport, UARTDM_IPR_ADDR, data);
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_TX);
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_RX);
}


static void msm_hs_set_std_bps_locked(struct uart_port *uport,
			       unsigned int bps)
{
	unsigned long rxstale;
	unsigned long data;

	switch (bps) {
	case 9600:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x99);
		rxstale = 2;
		break;
	case 14400:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xaa);
		rxstale = 3;
		break;
	case 19200:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xbb);
		rxstale = 4;
		break;
	case 28800:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xcc);
		rxstale = 6;
		break;
	case 38400:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xdd);
		rxstale = 8;
		break;
	case 57600:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xee);
		rxstale = 16;
		break;
	case 115200:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0xff);
		rxstale = 31;
		break;
	default:
		msm_hs_write(uport, UARTDM_CSR_ADDR, 0x99);
		
		bps = 9600;
		rxstale = 2;
		break;
	}

	data = rxstale & UARTDM_IPR_STALE_LSB_BMSK;
	data |= UARTDM_IPR_STALE_TIMEOUT_MSB_BMSK & (rxstale << 2);

	msm_hs_write(uport, UARTDM_IPR_ADDR, data);
}

static void msm_hs_set_termios(struct uart_port *uport,
				   struct ktermios *termios,
				   struct ktermios *oldtermios)
{
	unsigned int bps;
	unsigned long data;
	unsigned long flags;
	unsigned int c_cflag = termios->c_cflag;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	spin_lock_irqsave(&uport->lock, flags);

	data = msm_hs_read(uport, UARTDM_DMEN_ADDR);
	data &= ~UARTDM_RX_DM_EN_BMSK;
	msm_hs_write(uport, UARTDM_DMEN_ADDR, data);

	
	bps = uart_get_baud_rate(uport, termios, oldtermios, 200, 4000000);

	
	if (bps == 200)
		bps = 3200000;

	uport->uartclk = clk_get_rate(msm_uport->clk);
	if (!uport->uartclk)
		msm_hs_set_std_bps_locked(uport, bps);
	else
		msm_hs_set_bps_locked(uport, bps);

	data = msm_hs_read(uport, UARTDM_MR2_ADDR);
	data &= ~UARTDM_MR2_PARITY_MODE_BMSK;
	
	if (PARENB == (c_cflag & PARENB)) {
		if (PARODD == (c_cflag & PARODD))
			data |= ODD_PARITY;
		else if (CMSPAR == (c_cflag & CMSPAR))
			data |= SPACE_PARITY;
		else
			data |= EVEN_PARITY;
	}

	
	data &= ~UARTDM_MR2_BITS_PER_CHAR_BMSK;

	switch (c_cflag & CSIZE) {
	case CS5:
		data |= FIVE_BPC;
		break;
	case CS6:
		data |= SIX_BPC;
		break;
	case CS7:
		data |= SEVEN_BPC;
		break;
	default:
		data |= EIGHT_BPC;
		break;
	}
	
	if (c_cflag & CSTOPB) {
		data |= STOP_BIT_TWO;
	} else {
		
		data |= STOP_BIT_ONE;
	}
	data |= UARTDM_MR2_ERROR_MODE_BMSK;
	
	msm_hs_write(uport, UARTDM_MR2_ADDR, data);

	
	data = msm_hs_read(uport, UARTDM_MR1_ADDR);

	data &= ~(UARTDM_MR1_CTS_CTL_BMSK | UARTDM_MR1_RX_RDY_CTL_BMSK);

	if (c_cflag & CRTSCTS) {
		data |= UARTDM_MR1_CTS_CTL_BMSK;
		data |= UARTDM_MR1_RX_RDY_CTL_BMSK;
	}

	msm_hs_write(uport, UARTDM_MR1_ADDR, data);

	uport->ignore_status_mask = termios->c_iflag & INPCK;
	uport->ignore_status_mask |= termios->c_iflag & IGNPAR;
	uport->read_status_mask = (termios->c_cflag & CREAD);

	msm_hs_write(uport, UARTDM_IMR_ADDR, 0);

	
	uart_update_timeout(uport, c_cflag, bps);

	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_RX);
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_TX);

	if (msm_uport->rx.flush == FLUSH_NONE) {
		wake_lock(&msm_uport->rx.wake_lock);
		msm_uport->rx.flush = FLUSH_IGNORE;
		mb();
		
		msm_dmov_flush(msm_uport->dma_rx_channel, 0);
	}

	msm_hs_write(uport, UARTDM_IMR_ADDR, msm_uport->imr_reg);
	mb();
	spin_unlock_irqrestore(&uport->lock, flags);
}

unsigned int brcm_msm_hs_tx_empty(struct uart_port *uport)
{
	unsigned int data;
	unsigned int ret = 0;

	data = msm_hs_read(uport, UARTDM_SR_ADDR);
	if (data & UARTDM_SR_TXEMT_BMSK)
		ret = TIOCSER_TEMT;

	return ret;
}

static void msm_hs_stop_tx_locked(struct uart_port *uport)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	msm_uport->tx.tx_ready_int_en = 0;
}

static void msm_hs_stop_rx_locked(struct uart_port *uport)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	unsigned int data;

	
	data = msm_hs_read(uport, UARTDM_DMEN_ADDR);
	data &= ~UARTDM_RX_DM_EN_BMSK;
	msm_hs_write(uport, UARTDM_DMEN_ADDR, data);

	
	mb();
	
	if (msm_uport->rx.flush == FLUSH_NONE) {
		wake_lock(&msm_uport->rx.wake_lock);
		
		msm_dmov_flush(msm_uport->dma_rx_channel, 0);
	}
	if (msm_uport->rx.flush != FLUSH_SHUTDOWN)
		msm_uport->rx.flush = FLUSH_STOP;
}

static void msm_hs_submit_tx_locked(struct uart_port *uport)
{
	int left;
	int tx_count;
	int aligned_tx_count;
	dma_addr_t src_addr;
	dma_addr_t aligned_src_addr;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	struct msm_hs_tx *tx = &msm_uport->tx;
	struct circ_buf *tx_buf = &msm_uport->uport.state->xmit;

	if (uart_circ_empty(tx_buf) || uport->state->port.tty->stopped) {
		msm_hs_stop_tx_locked(uport);
		return;
	}

	tx->dma_in_flight = 1;

	tx_count = uart_circ_chars_pending(tx_buf);

	if (UARTDM_TX_BUF_SIZE < tx_count)
		tx_count = UARTDM_TX_BUF_SIZE;

	left = UART_XMIT_SIZE - tx_buf->tail;

	if (tx_count > left)
		tx_count = left;

	src_addr = tx->dma_base + tx_buf->tail;
	aligned_src_addr = src_addr & ~(dma_get_cache_alignment() - 1);
	aligned_tx_count = tx_count + src_addr - aligned_src_addr;

	dma_sync_single_for_device(uport->dev, aligned_src_addr,
			aligned_tx_count, DMA_TO_DEVICE);

	tx->command_ptr->num_rows = (((tx_count + 15) >> 4) << 16) |
				     ((tx_count + 15) >> 4);
	tx->command_ptr->src_row_addr = src_addr;

	dma_sync_single_for_device(uport->dev, tx->mapped_cmd_ptr,
				   sizeof(dmov_box), DMA_TO_DEVICE);

	*tx->command_ptr_ptr = CMD_PTR_LP | DMOV_CMD_ADDR(tx->mapped_cmd_ptr);

	
	tx->tx_count = tx_count;
	msm_hs_write(uport, UARTDM_NCF_TX_ADDR, tx_count);

	
	msm_uport->imr_reg &= ~UARTDM_ISR_TX_READY_BMSK;
	msm_hs_write(uport, UARTDM_IMR_ADDR, msm_uport->imr_reg);
	
	mb();

	dma_sync_single_for_device(uport->dev, tx->mapped_cmd_ptr_ptr,
				   sizeof(u32), DMA_TO_DEVICE);

	msm_dmov_enqueue_cmd(msm_uport->dma_tx_channel, &tx->xfer);
}

static void msm_hs_start_rx_locked(struct uart_port *uport)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	unsigned int buffer_pending = msm_uport->rx.buffer_pending;
	unsigned int data;

	msm_uport->rx.buffer_pending = 0;
	if (buffer_pending && hs_serial_debug_mask)
		printk(KERN_ERR "[BT]Error: rx started in buffer state = %x",
		       buffer_pending);

	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_STALE_INT);
	msm_hs_write(uport, UARTDM_DMRX_ADDR, UARTDM_RX_BUF_SIZE);
	msm_hs_write(uport, UARTDM_CR_ADDR, STALE_EVENT_ENABLE);
	msm_uport->imr_reg |= UARTDM_ISR_RXLEV_BMSK;

	data = msm_hs_read(uport, UARTDM_DMEN_ADDR);
	data |= UARTDM_RX_DM_EN_BMSK;
	msm_hs_write(uport, UARTDM_DMEN_ADDR, data);
	msm_hs_write(uport, UARTDM_IMR_ADDR, msm_uport->imr_reg);
	
	mb();

	msm_uport->rx.flush = FLUSH_NONE;
	msm_dmov_enqueue_cmd(msm_uport->dma_rx_channel, &msm_uport->rx.xfer);

}

static void flip_insert_work(struct work_struct *work)
{
	unsigned long flags;
	int retval;
	struct msm_hs_port *msm_uport =
		container_of(work, struct msm_hs_port,
			     rx.flip_insert_work.work);
	struct tty_struct *tty = msm_uport->uport.state->port.tty;

	spin_lock_irqsave(&msm_uport->uport.lock, flags);
	if (msm_uport->rx.buffer_pending == NONE_PENDING) {
		if (hs_serial_debug_mask)
			printk(KERN_ERR "[BT]Error: No buffer pending in %s",
			       __func__);
		return;
	}
	if (msm_uport->rx.buffer_pending & FIFO_OVERRUN) {
		retval = tty_insert_flip_char(tty, 0, TTY_OVERRUN);
		if (retval)
			msm_uport->rx.buffer_pending &= ~FIFO_OVERRUN;
	}
	if (msm_uport->rx.buffer_pending & PARITY_ERROR) {
		retval = tty_insert_flip_char(tty, 0, TTY_PARITY);
		if (retval)
			msm_uport->rx.buffer_pending &= ~PARITY_ERROR;
	}
	if (msm_uport->rx.buffer_pending & CHARS_NORMAL) {
		int rx_count, rx_offset;
		rx_count = (msm_uport->rx.buffer_pending & 0xFFFF0000) >> 16;
		rx_offset = (msm_uport->rx.buffer_pending & 0xFFD0) >> 5;
		retval = tty_insert_flip_string(tty, msm_uport->rx.buffer +
						rx_offset, rx_count);
		msm_uport->rx.buffer_pending &= (FIFO_OVERRUN |
						 PARITY_ERROR);
		if (retval != rx_count)
			msm_uport->rx.buffer_pending |= CHARS_NORMAL |
				retval << 8 | (rx_count - retval) << 16;
	}
	if (msm_uport->rx.buffer_pending)
		schedule_delayed_work(&msm_uport->rx.flip_insert_work,
				      msecs_to_jiffies(RETRY_TIMEOUT));
	else
		if ((msm_uport->clk_state == MSM_HS_CLK_ON) &&
		    (msm_uport->rx.flush <= FLUSH_IGNORE)) {
			if (hs_serial_debug_mask)
				printk(KERN_WARNING
				       "[BT]msm_serial_hs: "
				       "Pending buffers cleared. "
				       "Restarting\n");
			msm_hs_start_rx_locked(&msm_uport->uport);
		}
	spin_unlock_irqrestore(&msm_uport->uport.lock, flags);
	tty_flip_buffer_push(tty);
}

static void msm_serial_hs_rx_tlet(unsigned long tlet_ptr)
{
	int retval;
	int rx_count;
	unsigned long status;
	unsigned long flags;
	unsigned int error_f = 0;
	struct uart_port *uport;
	struct msm_hs_port *msm_uport;
	unsigned int flush;
	struct tty_struct *tty;

	msm_uport = container_of((struct tasklet_struct *)tlet_ptr,
				 struct msm_hs_port, rx.tlet);
	uport = &msm_uport->uport;
	tty = uport->state->port.tty;

	status = msm_hs_read(uport, UARTDM_SR_ADDR);

	spin_lock_irqsave(&uport->lock, flags);

	msm_hs_write(uport, UARTDM_CR_ADDR, STALE_EVENT_DISABLE);

	
	if (unlikely((status & UARTDM_SR_OVERRUN_BMSK) &&
		     (uport->read_status_mask & CREAD))) {
		retval = tty_insert_flip_char(tty, 0, TTY_OVERRUN);
		if (!retval)
			msm_uport->rx.buffer_pending |= TTY_OVERRUN;
		uport->icount.buf_overrun++;
		error_f = 1;
		printk(KERN_ERR "[BT]***OVERRUN:%d\n", uport->icount.buf_overrun);
	}

	if (!(uport->ignore_status_mask & INPCK))
		status = status & ~(UARTDM_SR_PAR_FRAME_BMSK);

	if (unlikely(status & UARTDM_SR_PAR_FRAME_BMSK)) {
		
		uport->icount.parity++;
		error_f = 1;
		printk(KERN_ERR "[BT]***PARITY FRAME error:%d\n", uport->icount.parity);
		if (uport->ignore_status_mask & IGNPAR) {
			retval = tty_insert_flip_char(tty, 0, TTY_PARITY);
			if (!retval)
				msm_uport->rx.buffer_pending |= TTY_PARITY;
		}
	}

	if (error_f)
		msm_hs_write(uport, UARTDM_CR_ADDR, RESET_ERROR_STATUS);

	if (msm_uport->clk_req_off_state == CLK_REQ_OFF_FLUSH_ISSUED)
		msm_uport->clk_req_off_state = CLK_REQ_OFF_RXSTALE_FLUSHED;
	flush = msm_uport->rx.flush;
	if (flush == FLUSH_IGNORE)
		if (!msm_uport->rx.buffer_pending)
			msm_hs_start_rx_locked(uport);

	if (flush == FLUSH_STOP) {
		msm_uport->rx.flush = FLUSH_SHUTDOWN;
		wake_up(&msm_uport->rx.wait);
	}
	if (flush >= FLUSH_DATA_INVALID)
		goto out;

	rx_count = msm_hs_read(uport, UARTDM_RX_TOTAL_SNAP_ADDR);

	
	rmb();

	if (0 != (uport->read_status_mask & CREAD)) {
		retval = tty_insert_flip_string(tty, msm_uport->rx.buffer,
						rx_count);
		if (retval != rx_count) {
			msm_uport->rx.buffer_pending |= CHARS_NORMAL |
				retval << 5 | (rx_count - retval) << 16;
		}
	}

	
	wmb();

	if (!msm_uport->rx.buffer_pending)
		msm_hs_start_rx_locked(uport);

out:
	if (msm_uport->rx.buffer_pending) {
		if (hs_serial_debug_mask)
			printk(KERN_WARNING
			       "[BT]msm_serial_hs: "
			       "tty buffer exhausted. "
			       "Stalling\n");
		schedule_delayed_work(&msm_uport->rx.flip_insert_work
				      , msecs_to_jiffies(RETRY_TIMEOUT));
	}
	wake_lock_timeout(&msm_uport->rx.wake_lock, HZ / 2);
	
	spin_unlock_irqrestore(&uport->lock, flags);
	if (flush < FLUSH_DATA_INVALID)
		tty_flip_buffer_push(tty);
}

static void msm_hs_start_tx_locked(struct uart_port *uport )
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	if (msm_uport->tx.tx_ready_int_en == 0) {
		msm_uport->tx.tx_ready_int_en = 1;
		if (msm_uport->tx.dma_in_flight == 0)
			msm_hs_submit_tx_locked(uport);
	}
}

static void msm_hs_dmov_tx_callback(struct msm_dmov_cmd *cmd_ptr,
					unsigned int result,
					struct msm_dmov_errdata *err)
{
	struct msm_hs_port *msm_uport;

	WARN_ON(result != 0x80000002);  

	msm_uport = container_of(cmd_ptr, struct msm_hs_port, tx.xfer);

	tasklet_schedule(&msm_uport->tx.tlet);
}

static void msm_serial_hs_tx_tlet(unsigned long tlet_ptr)
{
	unsigned long flags;
	struct msm_hs_port *msm_uport = container_of((struct tasklet_struct *)
				tlet_ptr, struct msm_hs_port, tx.tlet);

	spin_lock_irqsave(&(msm_uport->uport.lock), flags);

	msm_uport->imr_reg |= UARTDM_ISR_TX_READY_BMSK;
	msm_hs_write(&(msm_uport->uport), UARTDM_IMR_ADDR, msm_uport->imr_reg);
	
	mb();

	spin_unlock_irqrestore(&(msm_uport->uport.lock), flags);
}

static void msm_hs_dmov_rx_callback(struct msm_dmov_cmd *cmd_ptr,
					unsigned int result,
					struct msm_dmov_errdata *err)
{
	struct msm_hs_port *msm_uport;

	msm_uport = container_of(cmd_ptr, struct msm_hs_port, rx.xfer);

	tasklet_schedule(&msm_uport->rx.tlet);
}

static unsigned int msm_hs_get_mctrl_locked(struct uart_port *uport)
{
	return TIOCM_DSR | TIOCM_CAR | TIOCM_CTS;
}

static void msm_hs_set_mctrl_locked(struct uart_port *uport,
				    unsigned int mctrl)
{
	unsigned int set_rts;
	unsigned int data;

	
	set_rts = TIOCM_RTS & mctrl ? 0 : 1;

	data = msm_hs_read(uport, UARTDM_MR1_ADDR);
	if (set_rts) {
		
		data &= ~UARTDM_MR1_RX_RDY_CTL_BMSK;
		msm_hs_write(uport, UARTDM_MR1_ADDR, data);
		
		msm_hs_write(uport, UARTDM_CR_ADDR, RFR_HIGH);
	} else {
		
		data |= UARTDM_MR1_RX_RDY_CTL_BMSK;
		msm_hs_write(uport, UARTDM_MR1_ADDR, data);
	}
	mb();
}

#if 0
void msm_hs_set_mctrl(struct uart_port *uport,
				    unsigned int mctrl)
{
	unsigned long flags;

	spin_lock_irqsave(&uport->lock, flags);
	msm_hs_set_mctrl_locked(uport, mctrl);
	spin_unlock_irqrestore(&uport->lock, flags);
}
EXPORT_SYMBOL(msm_hs_set_mctrl);
#endif

static void msm_hs_enable_ms_locked(struct uart_port *uport)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	
	msm_uport->imr_reg |= UARTDM_ISR_DELTA_CTS_BMSK;
	msm_hs_write(uport, UARTDM_IMR_ADDR, msm_uport->imr_reg);
	mb();

}

static void msm_hs_break_ctl(struct uart_port *uport, int ctl)
{
	unsigned long flags;

	spin_lock_irqsave(&uport->lock, flags);
	msm_hs_write(uport, UARTDM_CR_ADDR, ctl ? START_BREAK : STOP_BREAK);
	mb();
	spin_unlock_irqrestore(&uport->lock, flags);
}

static void msm_hs_config_port(struct uart_port *uport, int cfg_flags)
{
	unsigned long flags;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	if (cfg_flags & UART_CONFIG_TYPE) {
		uport->type = PORT_MSM;
		msm_hs_request_port(uport);
	}

	if (is_gsbi_uart(msm_uport)) {
		if (msm_uport->pclk)
			clk_prepare_enable(msm_uport->pclk);
		spin_lock_irqsave(&uport->lock, flags);
		iowrite32(GSBI_PROTOCOL_UART, msm_uport->mapped_gsbi +
			  GSBI_CONTROL_ADDR);
		spin_unlock_irqrestore(&uport->lock, flags);
		if (msm_uport->pclk)
			clk_disable_unprepare(msm_uport->pclk);
	}
}

static void msm_hs_handle_delta_cts_locked(struct uart_port *uport)
{
	
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_CTS);
	
	mb();
	uport->icount.cts++;

	
	wake_up_interruptible(&uport->state->port.delta_msr_wait);
}

#ifdef USE_BCM_BT_CHIP	
static int read_host_wake_state_locked(struct uart_port *uport)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	return gpio_get_value(msm_uport->host_wakeup_pin);
}
#endif

static int msm_hs_check_clock_off(struct uart_port *uport)
{
	unsigned long sr_status;
	unsigned long flags;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	struct circ_buf *tx_buf = &uport->state->xmit;

	mutex_lock(&msm_uport->clk_mutex);
	spin_lock_irqsave(&uport->lock, flags);

#if 0
	if (msm_uport->request_clk_off_delay > 0) {
		msm_uport->request_clk_off_delay--;
		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return 0;  
	}
#endif

	if (msm_uport->clk_state != MSM_HS_CLK_REQUEST_OFF ||
	    !uart_circ_empty(tx_buf) || msm_uport->tx.dma_in_flight ||
	    msm_uport->imr_reg & UARTDM_ISR_TXLEV_BMSK) {
		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return -1;
	}

	
	sr_status = msm_hs_read(uport, UARTDM_SR_ADDR);
	if (!(sr_status & UARTDM_SR_TXEMT_BMSK)) {
		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return 0;  
	}

#ifdef USE_BCM_BT_CHIP	
	
	if (msm_uport->host_want_sleep) {
		if ((msm_uport->bt_wakeup_level == 0)
			|| (msm_uport->bt_wakeup_assert_inadvance == 1)) {
			gpio_set_value(msm_uport->bt_wakeup_pin,
				T_HIGH);
			msm_uport->bt_wakeup_level = 1;
			msm_uport->bt_wakeup_assert_inadvance = 0;
			#ifdef BT_SERIAL_DBG
			printk(KERN_INFO "[BT]CHK CLK OFF, BT_WAKE=HIGH\n");
			#endif
		}
	} else {
		
		msm_uport->clk_state = MSM_HS_CLK_ON;
		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return -1;
	}

	
	
	if (msm_uport->host_wakeup_level == 0) {
		msm_uport->clk_state = MSM_HS_CLK_ON;
		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return -1;	
	} else if (!read_host_wake_state_locked(uport)) {
		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return 0;
	} else {

		if (sr_status & UARTDM_SR_RXRDY_BMSK) {
			spin_unlock_irqrestore(&uport->lock, flags);
			mutex_unlock(&msm_uport->clk_mutex);
			return 0;  
		}

		
		{
			unsigned int data;
			data = msm_hs_read(uport, UARTDM_MR1_ADDR);

			if (data & UARTDM_MR1_RX_RDY_CTL_BMSK) {
				
				data &= ~UARTDM_MR1_RX_RDY_CTL_BMSK;
				msm_hs_write(uport, UARTDM_MR1_ADDR, data);

				
				msm_hs_write(uport, UARTDM_CR_ADDR, RFR_HIGH);
				
				mb();
				#ifdef BT_SERIAL_DBG
				printk(KERN_INFO "[BT]- DIS RFR, %d -\n",
						msm_uport->clk_state);
				#endif
			}
		}
	}
#endif

	
	switch (msm_uport->clk_req_off_state) {
	case CLK_REQ_OFF_START:
		msm_uport->clk_req_off_state = CLK_REQ_OFF_RXSTALE_ISSUED;
		msm_hs_write(uport, UARTDM_CR_ADDR, FORCE_STALE_EVENT);
		mb();
		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return 0;  
	case CLK_REQ_OFF_RXSTALE_ISSUED:
	case CLK_REQ_OFF_FLUSH_ISSUED:
		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return 0;  
	case CLK_REQ_OFF_RXSTALE_FLUSHED:
		break;  
	}

	if (msm_uport->rx.flush != FLUSH_SHUTDOWN) {
		#ifdef BT_SERIAL_DBG
		printk(KERN_INFO "[BT]CHK CLK OFF rx.flush:%d\n",
				msm_uport->rx.flush);
		#endif
		if (msm_uport->rx.flush == FLUSH_NONE)
			msm_hs_stop_rx_locked(uport);

		spin_unlock_irqrestore(&uport->lock, flags);
		mutex_unlock(&msm_uport->clk_mutex);
		return 0;  
	}

	spin_unlock_irqrestore(&uport->lock, flags);

	
	clk_disable_unprepare(msm_uport->clk);
	if (msm_uport->pclk)
		clk_disable_unprepare(msm_uport->pclk);

	msm_uport->clk_state = MSM_HS_CLK_OFF;

#ifdef USE_BCM_BT_CHIP
	
	if (msm_uport->rx.is_brcm_rx_wake_locked == 1) {
		wake_unlock(&msm_uport->rx.brcm_rx_wake_lock);
		msm_uport->rx.is_brcm_rx_wake_locked = 0;
	}
#endif

#if 0
	spin_lock_irqsave(&uport->lock, flags);
	if (use_low_power_wakeup(msm_uport)) {
		msm_uport->wakeup.ignore = 1;
		enable_irq(msm_uport->wakeup.irq);
	}
#endif
	wake_unlock(&msm_uport->dma_wake_lock);

	
	mutex_unlock(&msm_uport->clk_mutex);
	return 1;
}

static void hsuart_clock_off_work(struct work_struct *w)
{
	struct msm_hs_port *msm_uport = container_of(w, struct msm_hs_port,
							clock_off_w);
	struct uart_port *uport = &msm_uport->uport;

	if (!msm_hs_check_clock_off(uport)) {
		hrtimer_start(&msm_uport->clk_off_timer,
				msm_uport->clk_off_delay,
				HRTIMER_MODE_REL);
	}
}

static enum hrtimer_restart msm_hs_clk_off_retry(struct hrtimer *timer)
{
	struct msm_hs_port *msm_uport = container_of(timer, struct msm_hs_port,
							clk_off_timer);

	queue_work(msm_uport->hsuart_wq, &msm_uport->clock_off_w);
	return HRTIMER_NORESTART;
}

static irqreturn_t msm_hs_isr(int irq, void *dev)
{
	unsigned long flags;
	unsigned long isr_status;
	struct msm_hs_port *msm_uport = (struct msm_hs_port *)dev;
	struct uart_port *uport = &msm_uport->uport;
	struct circ_buf *tx_buf = &uport->state->xmit;
	struct msm_hs_tx *tx = &msm_uport->tx;
	struct msm_hs_rx *rx = &msm_uport->rx;

	spin_lock_irqsave(&uport->lock, flags);

	isr_status = msm_hs_read(uport, UARTDM_MISR_ADDR);

	
	if (isr_status & UARTDM_ISR_RXLEV_BMSK) {
		wake_lock(&rx->wake_lock);  
		msm_uport->imr_reg &= ~UARTDM_ISR_RXLEV_BMSK;
		msm_hs_write(uport, UARTDM_IMR_ADDR, msm_uport->imr_reg);
		
		mb();
	}
	
	if (isr_status & UARTDM_ISR_RXSTALE_BMSK) {
		msm_hs_write(uport, UARTDM_CR_ADDR, STALE_EVENT_DISABLE);
		msm_hs_write(uport, UARTDM_CR_ADDR, RESET_STALE_INT);
		mb();

		if (msm_uport->clk_req_off_state == CLK_REQ_OFF_RXSTALE_ISSUED)
			msm_uport->clk_req_off_state =
				CLK_REQ_OFF_FLUSH_ISSUED;

		if (rx->flush == FLUSH_NONE) {
			rx->flush = FLUSH_DATA_READY;
			msm_dmov_flush(msm_uport->dma_rx_channel, 1);
		}
	}
	
	if (isr_status & UARTDM_ISR_TX_READY_BMSK) {
		
		msm_hs_write(uport, UARTDM_CR_ADDR, CLEAR_TX_READY);

		if (msm_uport->clk_state == MSM_HS_CLK_REQUEST_OFF) {
			msm_uport->imr_reg |= UARTDM_ISR_TXLEV_BMSK;
			msm_hs_write(uport, UARTDM_IMR_ADDR,
				     msm_uport->imr_reg);
		}
		mb();
		
		tx_buf->tail = (tx_buf->tail + tx->tx_count) & ~UART_XMIT_SIZE;

		tx->dma_in_flight = 0;

		uport->icount.tx += tx->tx_count;
		if (tx->tx_ready_int_en)
			msm_hs_submit_tx_locked(uport);

		if (uart_circ_chars_pending(tx_buf) < WAKEUP_CHARS)
			uart_write_wakeup(uport);
	}
	if (isr_status & UARTDM_ISR_TXLEV_BMSK) {
		
		msm_uport->imr_reg &= ~UARTDM_ISR_TXLEV_BMSK;
		msm_hs_write(uport, UARTDM_IMR_ADDR, msm_uport->imr_reg);
		mb();
		queue_work(msm_uport->hsuart_wq, &msm_uport->clock_off_w);
	}

	
	if (isr_status & UARTDM_ISR_DELTA_CTS_BMSK)
		msm_hs_handle_delta_cts_locked(uport);

	spin_unlock_irqrestore(&uport->lock, flags);

	return IRQ_HANDLED;
}

void brcm_msm_hs_request_clock_off(struct uart_port *uport) {
	unsigned long flags;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);

	spin_lock_irqsave(&uport->lock, flags);
	if (msm_uport->clk_state == MSM_HS_CLK_ON) {
		msm_uport->clk_state = MSM_HS_CLK_REQUEST_OFF;
		msm_uport->clk_req_off_state = CLK_REQ_OFF_START;
		msm_uport->imr_reg |= UARTDM_ISR_TXLEV_BMSK;
		msm_hs_write(uport, UARTDM_IMR_ADDR, msm_uport->imr_reg);
		mb();
	}
	spin_unlock_irqrestore(&uport->lock, flags);
}

void brcm_msm_hs_request_clock_on(struct uart_port *uport)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	unsigned long flags;
	unsigned int data;
	int ret = 0;

	mutex_lock(&msm_uport->clk_mutex);
	spin_lock_irqsave(&uport->lock, flags);

	switch (msm_uport->clk_state) {
	case MSM_HS_CLK_OFF:
		wake_lock(&msm_uport->dma_wake_lock);
		
		spin_unlock_irqrestore(&uport->lock, flags);
		ret = clk_prepare_enable(msm_uport->clk);
		if (ret) {
			dev_err(uport->dev, "Clock ON Failure"
			"For UART CLK Stalling HSUART\n");
			break;
		}

		if (msm_uport->pclk) {
			ret = clk_prepare_enable(msm_uport->pclk);
			if (unlikely(ret)) {
				clk_disable_unprepare(msm_uport->clk);
				dev_err(uport->dev, "Clock ON Failure"
				"For UART Pclk Stalling HSUART\n");
				break;
			}
		}
		spin_lock_irqsave(&uport->lock, flags);
		
	case MSM_HS_CLK_REQUEST_OFF:
		 
		if (msm_uport->rx.flush == FLUSH_STOP ||
		    msm_uport->rx.flush == FLUSH_SHUTDOWN) {
			msm_hs_write(uport, UARTDM_CR_ADDR, RESET_RX);
			data = msm_hs_read(uport, UARTDM_DMEN_ADDR);
			data |= UARTDM_RX_DM_EN_BMSK;
			msm_hs_write(uport, UARTDM_DMEN_ADDR, data);
			
			mb();
		}
		hrtimer_try_to_cancel(&msm_uport->clk_off_timer);
		if (msm_uport->rx.flush == FLUSH_SHUTDOWN)
			msm_hs_start_rx_locked(uport);
		if (msm_uport->rx.flush == FLUSH_STOP)
			msm_uport->rx.flush = FLUSH_IGNORE;
		msm_uport->clk_state = MSM_HS_CLK_ON;

#ifdef USE_BCM_BT_CHIP	
	case MSM_HS_CLK_ON:
	case MSM_HS_CLK_PORT_OFF:
		{
			unsigned int data;

			
			data = msm_hs_read(uport, UARTDM_MR1_ADDR);
			if (!(data & UARTDM_MR1_RX_RDY_CTL_BMSK)) {
				data |= UARTDM_MR1_RX_RDY_CTL_BMSK;
				msm_hs_write(uport, UARTDM_MR1_ADDR, data);
				mb();
				#ifdef BT_SERIAL_DBG
				printk(KERN_INFO "[BT]- EN RFR -\n");
				#endif
			}
		}
		break;
#else
		break;
	case MSM_HS_CLK_ON:
		break;
	case MSM_HS_CLK_PORT_OFF:
		break;
#endif
	}

	spin_unlock_irqrestore(&uport->lock, flags);
	mutex_unlock(&msm_uport->clk_mutex);
}

static int
msm_uartdm_ioctl(struct uart_port *uport, unsigned int cmd, unsigned long arg)
{
#ifdef USE_BCM_BT_CHIP	
	void __user *argp = (void __user *)arg;
	unsigned long tbt_wakeup_level;
	int i = 0;

	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	struct msm_hs_tx *tx = &msm_uport->tx;
	unsigned long tflags;

	switch (cmd) {
	case 0x8003:
		
		wake_lock(&tx->brcm_tx_wake_lock);

#if 1
		
		for (i=0; i<15; i++) {
			if(msm_uport->clk_state == MSM_HS_CLK_REQUEST_OFF) {
				msleep(20);
				printk(KERN_INFO "[BT]Wait CLK off, round:%d, CLK:%d\n",
					i, msm_uport->clk_state);
			} else {
				break;
			}
		}
#endif
		spin_lock_irqsave(&uport->lock, tflags);
		#ifdef BT_SERIAL_DBG
		printk(KERN_INFO "[BT]-- HOST BT_WAKE=LOW, %d --\n",
				msm_uport->clk_state);
		#endif

		gpio_set_value(msm_uport->bt_wakeup_pin, T_LOW);
		msm_uport->bt_wakeup_level = 0;
		msm_uport->host_want_sleep = 0;

		spin_unlock_irqrestore(&uport->lock, tflags);

		brcm_msm_hs_request_clock_on(uport);
		break;

	case 0x8004:
		#ifdef BT_SERIAL_DBG
		printk(KERN_INFO "[BT]-- HOST BT_WAKE=HIGH, %d --\n",
				msm_uport->clk_state);
		#endif

		msm_uport->host_want_sleep = 1;
		if (msm_uport->host_wakeup_level == 1)
			brcm_msm_hs_request_clock_off(uport);
		#ifdef BT_SERIAL_DBG
		else
			printk(KERN_INFO "[BT]BUT HOST_WAKE==LOW\n");
		#endif

		
		wake_lock_timeout(&msm_uport->tx.brcm_tx_wake_lock, HZ / 2);
		break;

	case 0x8005:
		tbt_wakeup_level = !msm_uport->bt_wakeup_level;
		if (copy_to_user(argp, &tbt_wakeup_level,
				sizeof(tbt_wakeup_level)))
			return -EFAULT;
		break;

	default:
		return -ENOIOCTLCMD;
	}

	return 0;
#endif
}

static irqreturn_t msm_hs_wakeup_isr(int irq, void *dev)
{
#ifdef USE_BCM_BT_CHIP	
	unsigned long flags;
	struct msm_hs_port *msm_uport = (struct msm_hs_port *)dev;
	struct uart_port *uport = &msm_uport->uport;

	spin_lock_irqsave(&uport->lock, flags);

	if (msm_uport->host_wakeup_level == 0) { 
		#ifdef BT_SERIAL_DBG
		printk(KERN_INFO "[BT]-- CHIP HOST_WAKE=HIGH, %d --\n",
				msm_uport->clk_state);
		#endif
		msm_uport->host_wakeup_level = 1;
		irq_set_irq_type(msm_uport->wakeup.irq, IRQF_TRIGGER_LOW | IRQF_ONESHOT);
		if ((msm_uport->host_want_sleep)
				&& (msm_uport->clk_state == MSM_HS_CLK_ON)) {
			#if 1	

			msm_uport->clk_state = MSM_HS_CLK_REQUEST_OFF;
			msm_uport->clk_req_off_state = CLK_REQ_OFF_START;
			
			msm_uport->imr_reg |= UARTDM_ISR_TXLEV_BMSK;
			msm_hs_write(uport,
					UARTDM_IMR_ADDR, msm_uport->imr_reg);
			
			mb();
			#endif
		}

	} else {	
		#ifdef BT_SERIAL_DBG
		printk(KERN_INFO "[BT]-- CHIP HOST_WAKE=LOW, %d --\n",
				msm_uport->clk_state);
		#endif

		
		if (msm_uport->clk_state == MSM_HS_CLK_PORT_OFF )  {
			spin_unlock_irqrestore(&uport->lock, flags);
			return IRQ_HANDLED;
			
		}

		
		if (msm_uport->rx.is_brcm_rx_wake_locked == 0) {
			msm_uport->rx.is_brcm_rx_wake_locked = 1;
			wake_lock(&msm_uport->rx.brcm_rx_wake_lock);
		}

		#if 1	
		if ((msm_uport->bt_wakeup_level == 1)
			&& (msm_uport->bt_wakeup_assert_inadvance == 0)) {
			gpio_set_value(msm_uport->bt_wakeup_pin,
					T_LOW);
			msm_uport->bt_wakeup_assert_inadvance = 1;
			#ifdef BT_SERIAL_DBG
			printk(KERN_INFO "[BT]SET BT_WAKE=LOW IN ADV\n");
			#endif
		}
		#endif

		msm_uport->host_wakeup_level = 0;
		irq_set_irq_type(msm_uport->wakeup.irq, IRQF_TRIGGER_HIGH | IRQF_ONESHOT);
		spin_unlock_irqrestore(&uport->lock, flags);
		brcm_msm_hs_request_clock_on(uport);
		spin_lock_irqsave(&uport->lock, flags);
	}

	spin_unlock_irqrestore(&uport->lock, flags);

	return IRQ_HANDLED;
#else
	unsigned int wakeup = 0;
	unsigned long flags;
	struct msm_hs_port *msm_uport = (struct msm_hs_port *)dev;
	struct uart_port *uport = &msm_uport->uport;
	struct tty_struct *tty = NULL;

	spin_lock_irqsave(&uport->lock, flags);
	if (msm_uport->clk_state == MSM_HS_CLK_OFF)  {
		if (msm_uport->wakeup.ignore)
			msm_uport->wakeup.ignore = 0;
		else
			wakeup = 1;
	}

	if (wakeup) {
		spin_unlock_irqrestore(&uport->lock, flags);
		msm_hs_request_clock_on(uport);
		spin_lock_irqsave(&uport->lock, flags);
		if (msm_uport->wakeup.inject_rx) {
			tty = uport->state->port.tty;
			tty_insert_flip_char(tty,
					     msm_uport->wakeup.rx_to_inject,
					     TTY_NORMAL);
		}
	}

	spin_unlock_irqrestore(&uport->lock, flags);

	if (wakeup && msm_uport->wakeup.inject_rx)
		tty_flip_buffer_push(tty);
	return IRQ_HANDLED;
#endif
}

static const char *msm_hs_type(struct uart_port *port)
{
	return ("MSM HS UART");
}

static int msm_hs_startup(struct uart_port *uport)
{
	int ret;
	int rfr_level;
	unsigned long flags;
	unsigned int data;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	struct circ_buf *tx_buf = &uport->state->xmit;
	struct msm_hs_tx *tx = &msm_uport->tx;

	printk(KERN_INFO "[BT]== S UP ==\n");

	rfr_level = uport->fifosize;
	if (rfr_level > 16)
		rfr_level -= 16;

	tx->dma_base = dma_map_single(uport->dev, tx_buf->buf, UART_XMIT_SIZE,
				      DMA_TO_DEVICE);

	wake_lock(&msm_uport->dma_wake_lock);
	
	ret = msm_hs_init_clk(uport);
	if (unlikely(ret)) {
		pr_err("Turning ON uartclk error\n");
		wake_unlock(&msm_uport->dma_wake_lock);
		return ret;
	}

	
	data = msm_hs_read(uport, UARTDM_MR1_ADDR);
	data &= ~UARTDM_MR1_AUTO_RFR_LEVEL1_BMSK;
	data &= ~UARTDM_MR1_AUTO_RFR_LEVEL0_BMSK;
	data |= (UARTDM_MR1_AUTO_RFR_LEVEL1_BMSK & (rfr_level << 2));
	data |= (UARTDM_MR1_AUTO_RFR_LEVEL0_BMSK & rfr_level);
	msm_hs_write(uport, UARTDM_MR1_ADDR, data);

	
	data = msm_hs_read(uport, UARTDM_IPR_ADDR);
	if (!data) {
		data |= 0x1f & UARTDM_IPR_STALE_LSB_BMSK;
		msm_hs_write(uport, UARTDM_IPR_ADDR, data);
	}

	
	data = UARTDM_TX_DM_EN_BMSK | UARTDM_RX_DM_EN_BMSK;
	msm_hs_write(uport, UARTDM_DMEN_ADDR, data);

	
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_TX);
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_RX);
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_ERROR_STATUS);
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_BREAK_INT);
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_STALE_INT);
	msm_hs_write(uport, UARTDM_CR_ADDR, RESET_CTS);
	msm_hs_write(uport, UARTDM_CR_ADDR, RFR_LOW);
	
	msm_hs_write(uport, UARTDM_CR_ADDR, UARTDM_CR_RX_EN_BMSK);

	
	msm_hs_write(uport, UARTDM_CR_ADDR, UARTDM_CR_TX_EN_BMSK);

	
	tx->tx_ready_int_en = 0;
	tx->dma_in_flight = 0;

	tx->xfer.complete_func = msm_hs_dmov_tx_callback;

	tx->command_ptr->cmd = CMD_LC |
	    CMD_DST_CRCI(msm_uport->dma_tx_crci) | CMD_MODE_BOX;

	tx->command_ptr->src_dst_len = (MSM_UARTDM_BURST_SIZE << 16)
					   | (MSM_UARTDM_BURST_SIZE);

	tx->command_ptr->row_offset = (MSM_UARTDM_BURST_SIZE << 16);

	tx->command_ptr->dst_row_addr =
	    msm_uport->uport.mapbase + UARTDM_TF_ADDR;

	msm_uport->imr_reg |= UARTDM_ISR_RXSTALE_BMSK;
	
	msm_uport->imr_reg |= UARTDM_ISR_CURRENT_CTS_BMSK;

	msm_hs_write(uport, UARTDM_TFWR_ADDR, 0);  
	mb();

	if (use_low_power_wakeup(msm_uport)) {
		ret = irq_set_irq_wake(msm_uport->wakeup.irq, 1);
		if (unlikely(ret)) {
			pr_err("%s():Err setting wakeup irq\n", __func__);
			goto deinit_uart_clk;
		}
	}

	ret = request_irq(uport->irq, msm_hs_isr, IRQF_TRIGGER_HIGH,
			  "msm_hs_uart", msm_uport);
	if (unlikely(ret)) {
		pr_err("%s():Error getting uart irq\n", __func__);
		goto free_wake_irq;
	}
	if (use_low_power_wakeup(msm_uport)) {
		msm_uport->host_wakeup_level = 1; 
		msm_uport->rx.is_brcm_rx_wake_locked = 0; 

		ret = request_threaded_irq(msm_uport->wakeup.irq, NULL,
					msm_hs_wakeup_isr,
#ifdef USE_BCM_BT_CHIP
					IRQF_TRIGGER_LOW | IRQF_ONESHOT,
#else
					IRQF_TRIGGER_FALLING,
#endif
					"msm_hs_wakeup", msm_uport);

		if (unlikely(ret)) {
			pr_err("%s():Err getting uart wakeup_irq\n", __func__);
			goto free_uart_irq;
		}
		
	}

	spin_lock_irqsave(&uport->lock, flags);

	msm_hs_start_rx_locked(uport);

	spin_unlock_irqrestore(&uport->lock, flags);
#if 0
	ret = pm_runtime_set_active(uport->dev);
	if (ret)
		dev_err(uport->dev, "set active error:%d\n", ret);
	pm_runtime_enable(uport->dev);
#endif

	return 0;

free_uart_irq:
	free_irq(uport->irq, msm_uport);
free_wake_irq:
	irq_set_irq_wake(msm_uport->wakeup.irq, 0);
deinit_uart_clk:
	clk_disable_unprepare(msm_uport->clk);
	if (msm_uport->pclk)
		clk_disable_unprepare(msm_uport->pclk);
	wake_unlock(&msm_uport->dma_wake_lock);

	return ret;
}

static int uartdm_init_port(struct uart_port *uport)
{
	int ret = 0;
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	struct msm_hs_tx *tx = &msm_uport->tx;
	struct msm_hs_rx *rx = &msm_uport->rx;

	
	tx->command_ptr = kmalloc(sizeof(dmov_box), GFP_KERNEL | __GFP_DMA);
	if (!tx->command_ptr)
		return -ENOMEM;

	tx->command_ptr_ptr = kmalloc(sizeof(u32), GFP_KERNEL | __GFP_DMA);
	if (!tx->command_ptr_ptr) {
		ret = -ENOMEM;
		goto free_tx_command_ptr;
	}

	tx->mapped_cmd_ptr = dma_map_single(uport->dev, tx->command_ptr,
					    sizeof(dmov_box), DMA_TO_DEVICE);
	tx->mapped_cmd_ptr_ptr = dma_map_single(uport->dev,
						tx->command_ptr_ptr,
						sizeof(u32), DMA_TO_DEVICE);
	tx->xfer.cmdptr = DMOV_CMD_ADDR(tx->mapped_cmd_ptr_ptr);

#ifdef USE_BCM_BT_CHIP	
	wake_lock_init(&tx->brcm_tx_wake_lock,
			WAKE_LOCK_SUSPEND, "msm_serial_hs_brcm_tx");

	wake_lock_init(&rx->brcm_rx_wake_lock,
			WAKE_LOCK_SUSPEND, "msm_serial_hs_brcm_rx");
#endif

	init_waitqueue_head(&rx->wait);
	wake_lock_init(&rx->wake_lock, WAKE_LOCK_SUSPEND, "msm_serial_hs_rx");
	wake_lock_init(&msm_uport->dma_wake_lock, WAKE_LOCK_SUSPEND,
		       "msm_serial_hs_dma");

	tasklet_init(&rx->tlet, msm_serial_hs_rx_tlet,
			(unsigned long) &rx->tlet);
	tasklet_init(&tx->tlet, msm_serial_hs_tx_tlet,
			(unsigned long) &tx->tlet);

	rx->pool = dma_pool_create("rx_buffer_pool", uport->dev,
				   UARTDM_RX_BUF_SIZE, 16, 0);
	if (!rx->pool) {
		pr_err("%s(): cannot allocate rx_buffer_pool", __func__);
		ret = -ENOMEM;
		goto exit_tasket_init;
	}

	rx->buffer = dma_pool_alloc(rx->pool, GFP_KERNEL, &rx->rbuffer);
	if (!rx->buffer) {
		pr_err("%s(): cannot allocate rx->buffer", __func__);
		ret = -ENOMEM;
		goto free_pool;
	}

	
	rx->command_ptr = kmalloc(sizeof(dmov_box), GFP_KERNEL | __GFP_DMA);
	if (!rx->command_ptr) {
		pr_err("%s(): cannot allocate rx->command_ptr", __func__);
		ret = -ENOMEM;
		goto free_rx_buffer;
	}

	rx->command_ptr_ptr = kmalloc(sizeof(u32), GFP_KERNEL | __GFP_DMA);
	if (!rx->command_ptr_ptr) {
		pr_err("%s(): cannot allocate rx->command_ptr_ptr", __func__);
		ret = -ENOMEM;
		goto free_rx_command_ptr;
	}

	rx->command_ptr->num_rows = ((UARTDM_RX_BUF_SIZE >> 4) << 16) |
					 (UARTDM_RX_BUF_SIZE >> 4);

	rx->command_ptr->dst_row_addr = rx->rbuffer;

	
	msm_hs_write(uport, UARTDM_RFWR_ADDR, 0);

	rx->xfer.complete_func = msm_hs_dmov_rx_callback;

	rx->command_ptr->cmd = CMD_LC |
	    CMD_SRC_CRCI(msm_uport->dma_rx_crci) | CMD_MODE_BOX;

	rx->command_ptr->src_dst_len = (MSM_UARTDM_BURST_SIZE << 16)
					   | (MSM_UARTDM_BURST_SIZE);
	rx->command_ptr->row_offset =  MSM_UARTDM_BURST_SIZE;
	rx->command_ptr->src_row_addr = uport->mapbase + UARTDM_RF_ADDR;

	rx->mapped_cmd_ptr = dma_map_single(uport->dev, rx->command_ptr,
					    sizeof(dmov_box), DMA_TO_DEVICE);

	*rx->command_ptr_ptr = CMD_PTR_LP | DMOV_CMD_ADDR(rx->mapped_cmd_ptr);

	rx->cmdptr_dmaaddr = dma_map_single(uport->dev, rx->command_ptr_ptr,
					    sizeof(u32), DMA_TO_DEVICE);
	rx->xfer.cmdptr = DMOV_CMD_ADDR(rx->cmdptr_dmaaddr);

	INIT_DELAYED_WORK(&rx->flip_insert_work, flip_insert_work);

	return ret;

free_rx_command_ptr:
	kfree(rx->command_ptr);

free_rx_buffer:
	dma_pool_free(msm_uport->rx.pool, msm_uport->rx.buffer,
			msm_uport->rx.rbuffer);

free_pool:
	dma_pool_destroy(msm_uport->rx.pool);

exit_tasket_init:
	wake_lock_destroy(&msm_uport->rx.wake_lock);
	wake_lock_destroy(&msm_uport->dma_wake_lock);
	tasklet_kill(&msm_uport->tx.tlet);
	tasklet_kill(&msm_uport->rx.tlet);
	dma_unmap_single(uport->dev, msm_uport->tx.mapped_cmd_ptr_ptr,
			sizeof(u32), DMA_TO_DEVICE);
	dma_unmap_single(uport->dev, msm_uport->tx.mapped_cmd_ptr,
			sizeof(dmov_box), DMA_TO_DEVICE);
	kfree(msm_uport->tx.command_ptr_ptr);

free_tx_command_ptr:
	kfree(msm_uport->tx.command_ptr);
	return ret;
}

static int __devinit msm_hs_probe(struct platform_device *pdev)
{
	int ret;
	struct uart_port *uport;
	struct msm_hs_port *msm_uport;
	struct resource *resource;
	struct msm_serial_hs_platform_data *pdata = pdev->dev.platform_data;

	
	printk(KERN_INFO "[BT]BRCM chip\n");

	if (pdev->id < 0 || pdev->id >= UARTDM_NR) {
		printk(KERN_ERR "[BT]Invalid plaform device ID = %d\n",
				pdev->id);
		return -EINVAL;
	}

	msm_uport = &q_uart_port[pdev->id];
	uport = &msm_uport->uport;

	uport->dev = &pdev->dev;

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!resource))
		return -ENXIO;
	uport->mapbase = resource->start;  

	uport->membase = ioremap(uport->mapbase, PAGE_SIZE);
	if (unlikely(!uport->membase))
		return -ENOMEM;

	uport->irq = platform_get_irq(pdev, 0);
	if (unlikely((int)uport->irq < 0))
		return -ENXIO;

	if (pdata == NULL)
		msm_uport->wakeup.irq = -1;
	else {
		msm_uport->wakeup.irq = pdata->wakeup_irq;
		msm_uport->wakeup.ignore = 1;
		msm_uport->wakeup.inject_rx = pdata->inject_rx_on_wakeup;
		msm_uport->wakeup.rx_to_inject = pdata->rx_to_inject;

		if (unlikely(msm_uport->wakeup.irq < 0))
			return -ENXIO;

		if (pdata->gpio_config)
			if (unlikely(pdata->gpio_config(1)))
				dev_err(uport->dev, "Cannot configure"
					"gpios\n");
	}

#ifdef USE_BCM_BT_CHIP	
	if (pdata != NULL) {
		msm_uport->bt_wakeup_pin = pdata->bt_wakeup_pin;
		msm_uport->bt_wakeup_level = 1;
		msm_uport->bt_wakeup_assert_inadvance = 0;
		msm_uport->host_wakeup_pin = pdata->host_wakeup_pin;
		msm_uport->host_want_sleep = 1;
		
		msm_uport->host_wakeup_level = 1;
	}
#endif

	resource = platform_get_resource_byname(pdev, IORESOURCE_DMA,
						"uartdm_channels");
	if (unlikely(!resource))
		return -ENXIO;
	msm_uport->dma_tx_channel = resource->start;
	msm_uport->dma_rx_channel = resource->end;

	resource = platform_get_resource_byname(pdev, IORESOURCE_DMA,
						"uartdm_crci");
	if (unlikely(!resource))
		return -ENXIO;
	msm_uport->dma_tx_crci = resource->start;
	msm_uport->dma_rx_crci = resource->end;

	uport->iotype = UPIO_MEM;
	uport->fifosize = 64;
	uport->ops = &msm_hs_ops;
	uport->flags = UPF_BOOT_AUTOCONF;
	uport->uartclk = 7372800;
	msm_uport->imr_reg = 0x0;

	msm_uport->clk = clk_get(&pdev->dev, "core_clk");
	if (IS_ERR(msm_uport->clk))
		return PTR_ERR(msm_uport->clk);

	msm_uport->pclk = clk_get(&pdev->dev, "iface_clk");
	if (IS_ERR(msm_uport->pclk))
		msm_uport->pclk = NULL;

	ret = clk_set_rate(msm_uport->clk, uport->uartclk);
	if (ret) {
		printk(KERN_WARNING "[BT]Error setting clock rate on UART\n");
		return ret;
	}

	msm_uport->hsuart_wq = alloc_workqueue("k_hsuart",
					WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (!msm_uport->hsuart_wq) {
		pr_err("%s(): Unable to create workqueue hsuart_wq\n",
								__func__);
		return -ENOMEM;
	}

	INIT_WORK(&msm_uport->clock_off_w, hsuart_clock_off_work);
	mutex_init(&msm_uport->clk_mutex);

	clk_prepare_enable(msm_uport->clk);
	if (msm_uport->pclk)
		clk_prepare_enable(msm_uport->pclk);

	ret = uartdm_init_port(uport);
	if (unlikely(ret)) {
		clk_disable_unprepare(msm_uport->clk);
		if (msm_uport->pclk)
			clk_disable_unprepare(msm_uport->pclk);
		return ret;
	}

	
	msm_hs_write(uport, UARTDM_CR_ADDR, CR_PROTECTION_EN);

	clk_disable_unprepare(msm_uport->clk);
	if (msm_uport->pclk)
		clk_disable_unprepare(msm_uport->pclk);

	mb();

	msm_uport->clk_state = MSM_HS_CLK_PORT_OFF;
	hrtimer_init(&msm_uport->clk_off_timer, CLOCK_MONOTONIC,
		     HRTIMER_MODE_REL);
	msm_uport->clk_off_timer.function = msm_hs_clk_off_retry;
#ifdef USE_BCM_BT_CHIP
	msm_uport->clk_off_delay = ktime_set(0, 100000000);  
#else
	msm_uport->clk_off_delay = ktime_set(0, 1000000);  
#endif

	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_clock.attr);
	if (unlikely(ret))
		return ret;

	

	uport->line = pdev->id;
	return uart_add_one_port(&msm_hs_driver, uport);
}

static int __init msm_serial_hs_init(void)
{
	int ret;
	int i;

	
	for (i = 0; i < UARTDM_NR; i++)
		q_uart_port[i].uport.type = PORT_UNKNOWN;

	ret = uart_register_driver(&msm_hs_driver);
	if (unlikely(ret)) {
		printk(KERN_WARNING "[BT]%s failed to load\n", __FUNCTION__);
		return ret;
	}
#if 0
	debug_base = debugfs_create_dir("msm_serial_hs", NULL);
	if (IS_ERR_OR_NULL(debug_base))
		pr_info("msm_serial_hs: Cannot create debugfs dir\n");
#endif

	ret = platform_driver_register(&msm_serial_hs_platform_driver);
	if (ret) {
		printk(KERN_WARNING "[BT]%s failed to load\n", __FUNCTION__);
		uart_unregister_driver(&msm_hs_driver);
		return ret;
	}

	printk(KERN_INFO "[BT]msm_serial_hs module loaded\n");
	return ret;
}

static void msm_hs_shutdown(struct uart_port *uport)
{
	struct msm_hs_port *msm_uport = UARTDM_TO_MSM(uport);
	unsigned long flags; 

	printk(KERN_INFO "[BT]=+ S DN +=\n");

	
	spin_lock_irqsave(&uport->lock, flags);
	if (use_low_power_wakeup(msm_uport))
		irq_set_irq_wake(msm_uport->wakeup.irq, 0);
	spin_unlock_irqrestore(&uport->lock, flags);

	BUG_ON(msm_uport->rx.flush < FLUSH_STOP);
	tasklet_kill(&msm_uport->tx.tlet);
#if 1 
	wait_event_timeout(msm_uport->rx.wait, msm_uport->rx.flush == FLUSH_SHUTDOWN,
			msecs_to_jiffies(20000));
#else 
	wait_event(msm_uport->rx.wait, msm_uport->rx.flush == FLUSH_SHUTDOWN);
#endif
	tasklet_kill(&msm_uport->rx.tlet);
	cancel_delayed_work_sync(&msm_uport->rx.flip_insert_work);

	flush_workqueue(msm_uport->hsuart_wq);
#if 0
	pm_runtime_disable(uport->dev);
	pm_runtime_set_suspended(uport->dev);
#endif

	mutex_lock(&msm_uport->clk_mutex);
	
	msm_hs_write(uport, UARTDM_CR_ADDR, UARTDM_CR_TX_DISABLE_BMSK);
	
	msm_hs_write(uport, UARTDM_CR_ADDR, UARTDM_CR_RX_DISABLE_BMSK);

#ifdef USE_BCM_BT_CHIP
	
	wake_lock_timeout(&msm_uport->tx.brcm_tx_wake_lock, HZ / 2);
	wake_lock_timeout(&msm_uport->rx.brcm_rx_wake_lock, HZ / 2);
	
	wake_lock_timeout(&msm_uport->rx.wake_lock, HZ / 10);
#endif

	msm_uport->imr_reg = 0;
	msm_hs_write(uport, UARTDM_IMR_ADDR, msm_uport->imr_reg);
	mb();

	if (msm_uport->clk_state != MSM_HS_CLK_OFF) {
		
		clk_disable_unprepare(msm_uport->clk);
		if (msm_uport->pclk)
			clk_disable_unprepare(msm_uport->pclk);
		wake_unlock(&msm_uport->dma_wake_lock);
	}
	msm_uport->clk_state = MSM_HS_CLK_PORT_OFF;

	dma_unmap_single(uport->dev, msm_uport->tx.dma_base,
			 UART_XMIT_SIZE, DMA_TO_DEVICE);

	
	free_irq(uport->irq, msm_uport);
	if (use_low_power_wakeup(msm_uport))
		free_irq(msm_uport->wakeup.irq, msm_uport);
	mutex_unlock(&msm_uport->clk_mutex);
	

	printk(KERN_INFO "[BT]== S DN ==\n");
}

static void __exit msm_serial_hs_exit(void)
{
	printk(KERN_INFO "[BT]msm_serial_hs module removed\n");
	platform_driver_unregister(&msm_serial_hs_platform_driver);
	uart_unregister_driver(&msm_hs_driver);
}

static int msm_hs_runtime_idle(struct device *dev)
{
	return 0;
}

static int msm_hs_runtime_resume(struct device *dev)
{
#if 0
	struct platform_device *pdev = container_of(dev, struct
						    platform_device, dev);
	struct msm_hs_port *msm_uport = &q_uart_port[pdev->id];
	msm_hs_request_clock_on(&msm_uport->uport);
#endif
	return 0;
}

static int msm_hs_runtime_suspend(struct device *dev)
{
#if 0
	struct platform_device *pdev = container_of(dev, struct
						    platform_device, dev);
	struct msm_hs_port *msm_uport = &q_uart_port[pdev->id];
	msm_hs_request_clock_off(&msm_uport->uport);
#endif
	return 0;
}

static const struct dev_pm_ops msm_hs_dev_pm_ops = {
	.runtime_suspend = msm_hs_runtime_suspend,
	.runtime_resume  = msm_hs_runtime_resume,
	.runtime_idle    = msm_hs_runtime_idle,
};

static struct platform_driver msm_serial_hs_platform_driver = {
	.probe	= msm_hs_probe,
	.remove = __devexit_p(msm_hs_remove),
	.driver = {
		.name = "msm_serial_hs_brcm",
		.pm   = &msm_hs_dev_pm_ops,
	},
};

static struct uart_driver msm_hs_driver = {
	.owner = THIS_MODULE,
	.driver_name = "msm_serial_hs_brcm",
	.dev_name = "ttyHS",
	.nr = UARTDM_NR,
	.cons = 0,
};

static struct uart_ops msm_hs_ops = {
	.tx_empty = brcm_msm_hs_tx_empty,
	.set_mctrl = msm_hs_set_mctrl_locked,
	.get_mctrl = msm_hs_get_mctrl_locked,
	.stop_tx = msm_hs_stop_tx_locked,
	.start_tx = msm_hs_start_tx_locked,
	.stop_rx = msm_hs_stop_rx_locked,
	.enable_ms = msm_hs_enable_ms_locked,
	.break_ctl = msm_hs_break_ctl,
	.startup = msm_hs_startup,
	.shutdown = msm_hs_shutdown,
	.set_termios = msm_hs_set_termios,
	.type = msm_hs_type,
	.config_port = msm_hs_config_port,
	.ioctl = msm_uartdm_ioctl,
	.release_port = msm_hs_release_port,
	.request_port = msm_hs_request_port,
};

module_init(msm_serial_hs_init);
module_exit(msm_serial_hs_exit);
MODULE_DESCRIPTION("High Speed UART Driver for the MSM chipset");
MODULE_VERSION("1.2");
MODULE_LICENSE("GPL v2");
