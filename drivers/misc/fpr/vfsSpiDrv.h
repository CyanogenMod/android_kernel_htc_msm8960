/*! @file vfsSpiDrv.h
*******************************************************************************
**  SPI Driver Interface Functions
**
**  This file contains the SPI driver interface functions.
**
**  Copyright 2011-2012 Validity Sensors, Inc. All Rights Reserved.
*/

#ifndef VFSSPIDRV_H_
#define VFSSPIDRV_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/wait.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>
#include <linux/irq.h>

#include <asm-generic/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#if 0
#define DPRINTK(fmt, args...) printk(KERN_ERR "vfsspi:"fmt, ## args)
#else
#define DPRINTK(fmt, args...)
#endif

#define VFSSPI_MAJOR         (221)

#define DEFAULT_BUFFER_SIZE  (4096 * 5)

#if VCS_FEATURE_SENSOR_WINDSOR
#define DRDY_ACTIVE_STATUS      1
#define BITS_PER_WORD           8
#define DRDY_IRQ_FLAG           IRQF_TRIGGER_RISING
#else 
#define DRDY_ACTIVE_STATUS      0
#if defined(CONFIG_FPR_SPI_DMA_GSBI1) || defined(CONFIG_FPR_SPI_DMA_GSBI5)
#define BITS_PER_WORD           8
#else
#define BITS_PER_WORD           16
#endif
#define DRDY_IRQ_FLAG           IRQF_TRIGGER_FALLING
#endif 

#define DRDY_TIMEOUT_MS      40

#define VFSSPI_IOCTL_MAGIC    'k'


#define VFSSPI_IOCTL_RW_SPI_MESSAGE         _IOWR(VFSSPI_IOCTL_MAGIC,   \
							1, unsigned int)

#define VFSSPI_IOCTL_DEVICE_RESET           _IO(VFSSPI_IOCTL_MAGIC,   2)

#define VFSSPI_IOCTL_SET_CLK                _IOW(VFSSPI_IOCTL_MAGIC,    \
							3, unsigned int)

#define VFSSPI_IOCTL_CHECK_DRDY             _IO(VFSSPI_IOCTL_MAGIC,   4)

#define VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL   _IOW(VFSSPI_IOCTL_MAGIC,    \
							5, unsigned int)

#define VFSSPI_IOCTL_SET_USER_DATA          _IOW(VFSSPI_IOCTL_MAGIC,    \
							6, unsigned int)

#define VFSSPI_IOCTL_GET_USER_DATA          _IOWR(VFSSPI_IOCTL_MAGIC,   \
							7, unsigned int)

#define VFSSPI_IOCTL_SET_DRDY_INT           _IOW(VFSSPI_IOCTL_MAGIC,    \
							8, unsigned int)

#define VFSSPI_IOCTL_DEVICE_SUSPEND         _IO(VFSSPI_IOCTL_MAGIC,   9)

#define VFSSPI_IOCTL_STREAM_READ_START      _IOW(VFSSPI_IOCTL_MAGIC,	\
							10, unsigned int)
#define VFSSPI_IOCTL_STREAM_READ_STOP       _IO(VFSSPI_IOCTL_MAGIC,   11)

#define VFSSPI_IOCTL_GET_FREQ_TABLE         _IOWR(VFSSPI_IOCTL_MAGIC,	\
							12, unsigned int)

#define VFSSPI_IOCTL_POWER_ON               _IO(VFSSPI_IOCTL_MAGIC,   13)

#define VFSSPI_IOCTL_POWER_OFF              _IO(VFSSPI_IOCTL_MAGIC,   14)

#define VFSSPI_IOCTL_REGISTER_SUSPENDRESUME_SIGNAL    _IOW(VFSSPI_IOCTL_MAGIC,	\
							 19, unsigned int)

#define VFSSPI_IOCTL_GET_SYSTEM_STATUS  _IOR(VFSSPI_IOCTL_MAGIC,	\
							 20, unsigned int)

#define VFSSPI_DRDY_PIN     55 
#define VFSSPI_SLEEP_PIN    2  

#define SLOW_BAUD_RATE  5400000  
#define MAX_BAUD_RATE   15060000 

#define BAUD_RATE_COEF  1000


struct vfsspi_iocUserData {
	void *buffer;
	unsigned int len;
};

struct vfsspi_iocTransfer {
	unsigned char *rxBuffer;    
	unsigned char *txBuffer;    
	unsigned int len;   
};

struct vfsspi_iocRegSignal {
	int userPID;        
	int signalID;       
};

typedef struct vfsspi_iocFreqTable {
	unsigned int *table;
	unsigned int  tblSize;
} vfsspi_iocFreqTable_t;

struct vfsspi_devData {
	dev_t devt;
	spinlock_t vfsSpiLock;
	struct spi_device *spi;
	struct list_head deviceEntry;
	struct mutex bufferMutex;
	unsigned int isOpened;
	unsigned char *buffer;
	unsigned char *nullBuffer;
	unsigned char *streamBuffer;
	unsigned int *freqTable;
	unsigned int freqTableSize;
	size_t streamBufSize;
	struct vfsspi_iocUserData userInfoData;
	unsigned int drdyPin;
	unsigned int sleepPin;
	int userPID;
	int signalID;
	int eUserPID;
	int eSignalID;
	unsigned int curSpiSpeed;
};


#if 0
GSBI to be used : GSBI5
GPIO assignment:
	GPIO 49 - SPI_FP_MOSI
	GPIO 50 - SPI_FP_MISO
	GPIO 51 - SPI_FP_CS_N
	GPIO 52 - SPI_FP_CLK
	GPIO 154 - FP_DRDY_N
	GPIO 131 - FP_SLEEP
#endif
#endif              
