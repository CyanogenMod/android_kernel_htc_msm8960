/*! @file vfsSpiDrv.c
*******************************************************************************
**  SPI Driver Interface Functions
**
**  This file contains the SPI driver interface functions.
**
**  Copyright 2011 Validity Sensors, Inc. All Rights Reserved.
*/

#include "vfsSpiDrv.h"

static int vfsspi_open(struct inode *inode, struct file *filp);

static int vfsspi_release(struct inode *inode, struct file *filp);

static int vfsspi_probe(struct spi_device *spi);

static int vfsspi_remove(struct spi_device *spi);

static ssize_t vfsspi_read(struct file *filp, char __user *buf,
					 size_t count, loff_t *fPos);

static ssize_t vfsspi_write(struct file *filp, const char __user *buf,
						 size_t count, loff_t *fPos);

static int vfsspi_xfer(struct vfsspi_devData *vfsSpiDev,
				struct vfsspi_iocTransfer *tr);

static inline ssize_t vfsspi_readSync(struct vfsspi_devData *vfsSpiDev,
					unsigned char *buf, size_t len);

static inline ssize_t vfsspi_writeSync(struct vfsspi_devData *vfsSpiDev,
					unsigned char *buf, size_t len);

static long vfsspi_ioctl(struct file *filp, unsigned int cmd,
						 unsigned long arg);

static int vfsspi_sendDrdySignal(struct vfsspi_devData *vfsSpiDev);

static void vfsspi_hardReset(struct vfsspi_devData *vfsSpiDev);

static void vfsspi_suspend(struct vfsspi_devData *vfsSpiDev);

static inline void shortToLittleEndian(char *buf, size_t len);

static int  vfsspi_platformInit(struct vfsspi_devData *vfsSpiDev);

static void vfsspi_platformUninit(struct vfsspi_devData *vfsSpiDev);

static irqreturn_t vfsspi_irq(int irq, void *context);

#if 0
unsigned int freqTable[] = {
	960000,
	4800000,
	9600000,
	15000000,
	19200000,
	25000000,
	50000000,
};
#else
unsigned int freqTable[] = {
	1100000,
	5400000,
	10800000,
	15060000,
	24000000,
	25600000,
	27000000,
	48000000,
	51200000,
};
#endif

#define DISABLE_FP 0

int fpr_sensor = 0; 
int COF_enable = 0;
int fp_mount = 0;
module_param(fp_mount,int,0444);

struct fingerprint_pdata_struct {
    int (*set_sleep_pin)(int, int);
    int (*set_power_control)(int);
    unsigned int (*read_engineerid)(void);
};

struct fingerprint_pdata_struct *fingerprint_pdata;

struct spi_driver vfsspi_spi = {
	.driver = {
		.name  = "validity_fingerprint",
		.owner = THIS_MODULE,
	},
	.probe  = vfsspi_probe,
	.remove = __devexit_p(vfsspi_remove),
};

static const struct file_operations vfsspi_fops = {
	.owner   = THIS_MODULE,
	.write   = vfsspi_write,
	.read    = vfsspi_read,
	.unlocked_ioctl   = vfsspi_ioctl,
	.open    = vfsspi_open,
	.release = vfsspi_release,
};

struct spi_device *gDevSpi;
struct class *vfsSpiDevClass;
#ifdef CONFIG_HAS_EARLYSUSPEND
struct vfsspi_devData *vfsSpiDevTmp = NULL;
struct early_suspend early_suspend;
#endif
int gpio_irq;

static DECLARE_WAIT_QUEUE_HEAD(wq);
static LIST_HEAD(deviceList);
static DEFINE_MUTEX(deviceListMutex);
static int dataToRead;
static int suspend = 0;
static int hasfp = 0;

static int __init fingerprint_mount(char *str)
{

	printk("[FP] hasfp=%s", str);
	hasfp = simple_strtol(str, NULL, 0);
	return 1;
}

__setup("androidboot.hasfp=", fingerprint_mount);

static ssize_t fpr_read(struct device *dev,
                    struct device_attribute *attr, char *buf)
{
    return 0;
}

static ssize_t fpr_write(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t count)
{
    int retVal;
    switch(buf[0])
    {
        case '0':
		    printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_OFF]\n");
            retVal = fingerprint_pdata->set_power_control(0);
            if(retVal)
		        printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_OFF] success\n");
            else
            {
		        printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_OFF] fail\n");
                retVal = -EFAULT;
            }

            break;

        case '1':
		    printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_ON]\n");
            retVal = fingerprint_pdata->set_power_control(1);
            if(retVal)
		        printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_ON] success\n");
            else
            {
		        printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_ON] fail\n");
                retVal = -EFAULT;
            }

            break;

        case '2':
		    printk("[fp]vfsspi: Enable COF navigation\n");
            COF_enable = 1;
            break;

        case '3':
		    printk("[fp]vfsspi: Disable COF navigation\n");
            COF_enable = 0;
            break;
        case '4':
            fingerprint_pdata->read_engineerid();
            break;
    }

    return count;
}
static DEVICE_ATTR(fpr, 0644, fpr_read, fpr_write);

inline void shortToLittleEndian(char *buf, size_t len)
{
	int i = 0;
	int j = 0;
	for (i = 0; i < len; i++, j++) {
		char LSB, MSB;
		LSB = buf[i];
		i++;
		MSB = buf[i];
		buf[j] = MSB;
		j++;
		buf[j] = LSB;
	}
} 


irqreturn_t vfsspi_irq(int irq, void *context)
{
	struct vfsspi_devData *vfsSpiDev = context;
	dataToRead = 1;
	wake_up_interruptible(&wq);
	vfsspi_sendDrdySignal(vfsSpiDev);
	return IRQ_HANDLED;
}

int vfsspi_sendDrdySignal(struct vfsspi_devData *vfsSpiDev)
{
	struct task_struct *t;
	int ret = 0;
	if (vfsSpiDev->userPID != 0) {
		rcu_read_lock();
		
		t = pid_task(find_pid_ns(vfsSpiDev->userPID, &init_pid_ns),
								 PIDTYPE_PID);
		if (t == NULL) {
			printk("[fp]vfsspi_sendDrdySignal: No such pid\n");
			rcu_read_unlock();
			return -ENODEV;
		}
		rcu_read_unlock();
		
		ret = send_sig_info(vfsSpiDev->signalID,
					 (struct siginfo *)1, t);
		if (ret < 0)
			printk("[fp]vfsspi_sendDrdySignal: Error sending signal\n");
	} else {
		printk("[fp]vfsspi_sendDrdySignal: pid not received yet\n");
	}
	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void vfsspi_early_suspend(struct early_suspend *h)
{
	struct task_struct *t;
	int ret = 0;

	printk("[fp]vfsspi_early_suspend\n");
	suspend = 1;
	if ( vfsSpiDevTmp != NULL ) {
		if (vfsSpiDevTmp->eUserPID != 0) {
			rcu_read_lock();
			
			printk("[fp]Searching task with PID=%08x\n", vfsSpiDevTmp->eUserPID);
			t = pid_task(find_pid_ns(vfsSpiDevTmp->eUserPID, &init_pid_ns),
					 PIDTYPE_PID);
			if (t == NULL) {
				printk("[fp]No such pid\n");
				rcu_read_unlock();
				return;
			}
			rcu_read_unlock();
			
			ret =
				send_sig_info(vfsSpiDevTmp->eSignalID, (struct siginfo *)1,
					  t);
			if (ret < 0)
				printk("[fp]Error sending suspendResumesignal\n");
			else
				printk("[fp]pid not received yet\n");
		}
	}
}

void vfsspi_late_resume(struct early_suspend *h)
{
	struct task_struct *t;
	int ret = 0;

	printk("[fp]vfsspi_late_resume\n");
	suspend = 0;
	if ( vfsSpiDevTmp != NULL ) {
		if (vfsSpiDevTmp->eUserPID != 0) {
			rcu_read_lock();
			
			printk("[fp]Searching task with PID=%08x\n", vfsSpiDevTmp->eUserPID);
			t = pid_task(find_pid_ns(vfsSpiDevTmp->eUserPID, &init_pid_ns),
					 PIDTYPE_PID);
			if (t == NULL) {
				printk("[fp]No such pid\n");
				rcu_read_unlock();
				return;
			}
			rcu_read_unlock();
			
			ret =
				send_sig_info(vfsSpiDevTmp->eSignalID, (struct siginfo *)1,
					  t);
			if (ret < 0)
				printk("[fp]Error sending suspendResumesignal\n");
			else
				printk("[fp]pid not received yet\n");
		}
	}
}
#endif

/* Return no.of bytes written to device. Negative number for errors */
static inline ssize_t vfsspi_writeSync(struct vfsspi_devData *vfsSpiDev,
					unsigned char *buf,size_t len)
{
	int    status = 0;
	struct spi_message m;
	struct spi_transfer t;
	DPRINTK("[fp]vfsspi_writeSync\n");
	spi_message_init(&m);
	memset(&t, 0, sizeof(t));
	t.rx_buf = NULL;
	t.tx_buf = buf;
	t.len = len;
    t.speed_hz = vfsSpiDev->curSpiSpeed;
	spi_message_add_tail(&t, &m);
	status = spi_sync(vfsSpiDev->spi, &m);
	if (status == 0)
		status = m.actual_length;
    else
	    printk("[fp]vfsspi_writeSync, spi_sync fail\n");

	DPRINTK("[fp]vfsspi_writeSync,length=%d\n", m.actual_length);
	return status;
}

inline ssize_t vfsspi_readSync(struct vfsspi_devData *vfsSpiDev,
				unsigned char *buf, size_t len)
{
	int    status = 0;
	struct spi_message m;
	struct spi_transfer t;
	DPRINTK("[fp]vfsspi_readSync\n");
	spi_message_init(&m);
	memset(&t, 0x0, sizeof(t));
	t.tx_buf = NULL;
	t.rx_buf = buf;
	t.len = len;
	t.speed_hz = vfsSpiDev->curSpiSpeed;
	spi_message_add_tail(&t, &m);
	status = spi_sync(vfsSpiDev->spi, &m);
	if (status == 0) {
		status = m.actual_length;
		  status = len;
	}
    else
	    printk("[fp]vfsspi_readSync, spi_sync fail\n");
	return status;
}

ssize_t vfsspi_write(struct file *filp, const char __user *buf, size_t count,
								 loff_t *fPos)
{
	struct vfsspi_devData *vfsSpiDev = NULL;
	ssize_t               status = 0;
	DPRINTK("[fp]vfsspi_write\n");
	if (count > DEFAULT_BUFFER_SIZE || count <= 0)
    {
        printk("[fp]vfsspi_write: count(%d) > DEFAULT_BUFFER_SIZE(%d) || count <= 0", count, DEFAULT_BUFFER_SIZE);
		return -EMSGSIZE;
    }
	vfsSpiDev = filp->private_data;
	mutex_lock(&vfsSpiDev->bufferMutex);
	if (vfsSpiDev->buffer) {
		unsigned long missing = 0;
		missing = copy_from_user(vfsSpiDev->buffer, buf, count);
		shortToLittleEndian((char *)vfsSpiDev->buffer, count);
		if (missing == 0)
			status = vfsspi_writeSync(vfsSpiDev, vfsSpiDev->buffer,
                                                            count);
		else
        {
             printk("[fp]vfsspi_write: missing != 0\n");
			status = -EFAULT;
        }
	}
	mutex_unlock(&vfsSpiDev->bufferMutex);
	return status;
}

ssize_t vfsspi_read(struct file *filp, char __user *buf, size_t count,
								loff_t *fPos)
{
	struct vfsspi_devData *vfsSpiDev = NULL;
	unsigned char * readBuf = NULL;
	ssize_t                status    = 0;
	DPRINTK("[fp]vfsspi_read\n");
	vfsSpiDev = filp->private_data;
	if (vfsSpiDev->streamBuffer != NULL && count <= vfsSpiDev->streamBufSize) {
		readBuf = vfsSpiDev->streamBuffer;
	} else if (count <= DEFAULT_BUFFER_SIZE) {
		readBuf = vfsSpiDev->buffer;
	} else {
        printk("[fp]vfsspi_read: count(%d) > DEFAULT_BUFFER_SIZE (%d)\n", count, DEFAULT_BUFFER_SIZE);
		return -EMSGSIZE;
	}
	if (buf == NULL)
    {
        printk("[fp]vfsspi_read: buf == NULL\n");
		return -EFAULT;
    }

	mutex_lock(&vfsSpiDev->bufferMutex);
	status  = vfsspi_readSync(vfsSpiDev, readBuf, count);
    if (status > 400000) {
        printk("[fp]vfsspi_read, wrong status = %d\n",status);
        status = -EFAULT;
    } else if (status > DEFAULT_BUFFER_SIZE) {
        printk("[fp]vfsspi_read, status > DEFAULT_BUFFER_SIZE; status = %d\n",status);
    } else if (status > vfsSpiDev->streamBufSize) {
        
    }
	if (status > 0) {
		unsigned long missing = 0;
		
		shortToLittleEndian((char *)readBuf, status);
		missing = copy_to_user(buf, readBuf, status);
		if (missing == status) {
			printk("[fp]vfsspi_read: copy_to_user failed\n");
			
			status = -EFAULT;
		} else {
			status = status - missing;
		}
	}
    else
        printk("[fp]vfsspi_read: vfsspi_readSync is fail, status = %d\n", status);
	mutex_unlock(&vfsSpiDev->bufferMutex);
	return status;
}

int vfsspi_xfer(struct vfsspi_devData *vfsSpiDev, struct vfsspi_iocTransfer *tr)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;
	DPRINTK("[fp]vfsspi_xfer\n");
	if (vfsSpiDev == NULL || tr == NULL)
    {
        printk("[fp]vfsspi_xfer: vfsSpiDev == NULL(%d) || tr == NULL(%d)\n", (vfsSpiDev == NULL), (tr == NULL));
		return -EFAULT;
    }
	if (tr->len > DEFAULT_BUFFER_SIZE || tr->len <= 0)
    {
        printk("[fp]vfsspi_xfer: tr->len(%d) > DEFAULT_BUFFER_SIZE(%d) || tr->len(%d) <= 0\n", tr->len, DEFAULT_BUFFER_SIZE, tr->len);
		return -EMSGSIZE;
    }
	if (tr->txBuffer != NULL) {
		if (copy_from_user(vfsSpiDev->nullBuffer, tr->txBuffer,
							tr->len) != 0)
        {
            printk("[fp]vfsspi_xfer: copy from user fail\n");
			return -EFAULT;
        }
		shortToLittleEndian((char *)vfsSpiDev->nullBuffer, tr->len);
	}
	spi_message_init(&m);
	memset(&t, 0, sizeof(t));
	t.tx_buf = vfsSpiDev->nullBuffer;
	t.rx_buf = vfsSpiDev->buffer;
	t.len = tr->len;
	t.speed_hz = vfsSpiDev->curSpiSpeed;
	spi_message_add_tail(&t, &m);
	status = spi_sync(vfsSpiDev->spi, &m);
	if (status == 0) {
		if (tr->rxBuffer != NULL) {
			unsigned missing = 0;
			shortToLittleEndian((char *)vfsSpiDev->buffer, tr->len);
			missing = copy_to_user(tr->rxBuffer,
						 vfsSpiDev->buffer, tr->len);
			if (missing != 0)
				tr->len = tr->len - missing;
		}
	}
    else
	    printk("[fp]vfsspi_xfer: spi_sync fail\n");
	DPRINTK("vfsspi_xfer,length=%d\n", tr->len);
	return status;
}

long vfsspi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retVal = 0;
	struct vfsspi_devData *vfsSpiDev = NULL;
	DPRINTK("[fp]vfsspi_ioctl\n");
	if (_IOC_TYPE(cmd) != VFSSPI_IOCTL_MAGIC) {
		printk("[fp]vfsspi_ioctl: invalid magic. cmd= 0x%X Received= 0x%X "
				"Expected= 0x%X\n", cmd, _IOC_TYPE(cmd),
						VFSSPI_IOCTL_MAGIC);
		return -ENOTTY;
	}
	vfsSpiDev = filp->private_data;
#ifdef CONFIG_HAS_EARLYSUSPEND
	vfsSpiDevTmp = vfsSpiDev;
#endif
	mutex_lock(&vfsSpiDev->bufferMutex);
	switch (cmd) {
	case VFSSPI_IOCTL_DEVICE_SUSPEND:
	{
		printk("[fp]VFSSPI_IOCTL_DEVICE_SUSPEND:\n");
		vfsspi_suspend(vfsSpiDev);
		break;
	}
	case VFSSPI_IOCTL_DEVICE_RESET:
	{
		printk("[fp]VFSSPI_IOCTL_DEVICE_RESET:\n");
		vfsspi_hardReset(vfsSpiDev);
		break;
	}
	case VFSSPI_IOCTL_RW_SPI_MESSAGE:
	{
		struct vfsspi_iocTransfer *dup = NULL;
		DPRINTK("[fp]VFSSPI_IOCTL_RW_SPI_MESSAGE\n");
		dup = kmalloc(sizeof(struct vfsspi_iocTransfer), GFP_KERNEL);
		if (dup != NULL) {
			if (copy_from_user(dup, (void *)arg,
				sizeof(struct vfsspi_iocTransfer)) == 0) {
				retVal = vfsspi_xfer(vfsSpiDev, dup);
				if (retVal == 0) {
					if (copy_to_user((void *)arg, dup,
				sizeof(struct vfsspi_iocTransfer)) != 0) {
						printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_RW_SPI_MESSAGE] copy to user"
								 "failed\n");
						retVal = -EFAULT;
					}
				}
			} else {
				printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_RW_SPI_MESSAGE] copy from user failed\n");
				retVal = -EFAULT;
			}
			kfree(dup);
		} else {

            printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_RW_SPI_MESSAGE] dup is NULL\n");
			retVal = -ENOMEM;
		}
		break;
	}
	case VFSSPI_IOCTL_SET_CLK:
	{
		unsigned short clock = 0;
		printk("[fp]VFSSPI_IOCTL_SET_CLK\n");
		if (copy_from_user(&clock, (void *)arg,
				 sizeof(unsigned short)) == 0) {
			struct spi_device *spidev = NULL;
			spin_lock_irq(&vfsSpiDev->vfsSpiLock);
			spidev = spi_dev_get(vfsSpiDev->spi);
			spin_unlock_irq(&vfsSpiDev->vfsSpiLock);
			if (spidev != NULL) {
				switch (clock) {
				case 0:
				{
					
					DPRINTK("[fp]Running baud rate.\n");
					spidev->max_speed_hz = MAX_BAUD_RATE;
					vfsSpiDev->curSpiSpeed = MAX_BAUD_RATE;
					break;
				}
				case 0xFFFF:
				{
					
					DPRINTK("[fp]slow baud rate.\n");
					spidev->max_speed_hz = SLOW_BAUD_RATE;
					vfsSpiDev->curSpiSpeed = SLOW_BAUD_RATE;
					break;
				}
				default:
				{
					DPRINTK("[fp]baud rate is %d.\n",
								 clock);
					vfsSpiDev->curSpiSpeed =
							 clock * BAUD_RATE_COEF;
					if (vfsSpiDev->curSpiSpeed >
								 MAX_BAUD_RATE)
						vfsSpiDev->curSpiSpeed =
								 MAX_BAUD_RATE;
					if (vfsSpiDev->curSpiSpeed <
								 SLOW_BAUD_RATE)
						vfsSpiDev->curSpiSpeed =
								 SLOW_BAUD_RATE;
					spidev->max_speed_hz =
							vfsSpiDev->curSpiSpeed;
					break;
				}
				}
				spi_dev_put(spidev);
			}
		} else {
			printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_SET_CLK] Failed copy from user.\n");
			retVal = -EFAULT;
		}
		break;
	}
	case VFSSPI_IOCTL_CHECK_DRDY:
	{
		retVal = ETIMEDOUT;
		DPRINTK("[fp]VFSSPI_IOCTL_CHECK_DRDY\n");
		dataToRead = 0;

		if (gpio_get_value(vfsSpiDev->drdyPin) == DRDY_ACTIVE_STATUS) {
			retVal = 0;
		} else {
			unsigned long timeout = msecs_to_jiffies(DRDY_TIMEOUT_MS);
			unsigned long startTime = jiffies;
			unsigned long endTime = 0;
			int status;
			do
			{
				status = wait_event_interruptible_timeout(wq, dataToRead != 0,
											timeout);
				dataToRead = 0;
				if (gpio_get_value(vfsSpiDev->drdyPin) == DRDY_ACTIVE_STATUS) {
					retVal = 0;
					break;
				}
				endTime = jiffies;
				if (endTime - startTime >= timeout) {
					
					timeout = 0;
				} else {
					timeout -= endTime - startTime;
					startTime = endTime;
				}
			} while(timeout > 0);
		}
		dataToRead = 0;
		break;
	}
	case VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL:
	{
		struct vfsspi_iocRegSignal usrSignal;
		printk("[fp]VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL\n");
		if (copy_from_user(&usrSignal, (void *)arg,
					 sizeof(usrSignal)) != 0) {
			printk("[fp][VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL] Failed copy from user.\n");
			retVal = -EFAULT;
		} else {
			vfsSpiDev->userPID = usrSignal.userPID;
			vfsSpiDev->signalID = usrSignal.signalID;
		}
		break;
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	case VFSSPI_IOCTL_REGISTER_SUSPENDRESUME_SIGNAL:
	{
		struct vfsspi_iocRegSignal usrSignal;
		printk("[fp]VFSSPI_IOCTL_REGISTER_SUSPENDRESUME_SIGNAL\n");
		if (copy_from_user(&usrSignal, (void *)arg,
					 sizeof(usrSignal)) != 0) {
			printk("[fp]Failed copy from user.\n");
			retVal = -EFAULT;
		} else {
			vfsSpiDev->eUserPID = usrSignal.userPID;
			vfsSpiDev->eSignalID = usrSignal.signalID;
		}
		break;
	}
	case VFSSPI_IOCTL_GET_SYSTEM_STATUS:
	{
		if(arg != 0){
			printk("[fp]VFSSPI_IOCTL_GET_SYSTEM_STATUS\n");
			if (copy_to_user((void *)arg, &suspend,sizeof(suspend)) != 0) {
				printk("[fp]copy to userfailed\n");
				retVal = -EFAULT;
			}
		}
		break;
	}
#endif
	case VFSSPI_IOCTL_SET_USER_DATA:
	{
		struct vfsspi_iocUserData tmpUserData;
		printk("[fp]VFSSPI_IOCTL_SET_USER_DATA\n");
		if ((void *)arg == NULL) {
			printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_SET_USER_DATA] VFSSPI_IOCTL_SET_USER_DATA is failed\n");
			retVal = -EINVAL;
			break;
		}
		if (copy_from_user(&tmpUserData, (void *)arg,
				sizeof(struct vfsspi_iocUserData)) == 0) {
			if (vfsSpiDev->userInfoData.buffer != NULL)
				kfree(vfsSpiDev->userInfoData.buffer);
			vfsSpiDev->userInfoData.buffer =
				 kmalloc(tmpUserData.len, GFP_KERNEL);
			if (vfsSpiDev->userInfoData.buffer != NULL) {
				vfsSpiDev->userInfoData.len = tmpUserData.len;
				if (tmpUserData.buffer != NULL) {
					if (copy_from_user(
						vfsSpiDev->userInfoData.buffer,
						tmpUserData.buffer,
						tmpUserData.len) != 0) {
						printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_SET_USER_DATA] copy from user"
								" failed\n");
						retVal = -EFAULT;
					}
				} else {
                    printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_SET_USER_DATA] tmpUserData.buffer == NULL\n");
					retVal = -EFAULT;
				}
			} else {
                printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_SET_USER_DATA] vfsSpiDev->userInfoData.buffer == NULL\n");
				retVal = -ENOMEM;
			}
		} else {
			printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_SET_USER_DATA] copy from user failed\n");
			retVal = -EFAULT;
		}
		break;
	}
	case VFSSPI_IOCTL_GET_USER_DATA:
	{
		struct vfsspi_iocUserData tmpUserData;
		retVal = -EFAULT;
		printk("[fp]VFSSPI_IOCTL_GET_USER_DATA\n");
		if (vfsSpiDev->userInfoData.buffer != NULL &&
					(void *)arg != NULL) {
			if (copy_from_user(&tmpUserData, (void *)arg,
				sizeof(struct vfsspi_iocUserData)) == 0) {
				if (tmpUserData.len ==
						 vfsSpiDev->userInfoData.len
						&& tmpUserData.buffer != NULL) {
					if (copy_to_user(tmpUserData.buffer,
						 vfsSpiDev->userInfoData.buffer,
						tmpUserData.len) == 0) {
						if (copy_to_user((void *)arg,
							 &(tmpUserData),
				sizeof(struct vfsspi_iocUserData)) != 0) {
							printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_GET_USER_DATA] copy to user"
								" failed\n");
							break;
						}
						retVal = 0;
					} else {
						printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_GET_USER_DATA] copy to user"
								" failed\n");
					}
				} else {
					printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_GET_USER_DATA] userInfoData parameter "
						 "is incorrect\n");
				}
			} else {
				printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_GET_USER_DATA] copy from user failed\n");
			}
		} else {
			printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_GET_USER_DATA] VFSSPI_IOCTL_GET_USER_DATA failed\n");
		}
		break;
	}
	case VFSSPI_IOCTL_SET_DRDY_INT:
	{
		unsigned short drdy_enable_flag;
		printk("[fp]VFSSPI_IOCTL_SET_DRDY_INT\n");
		if (copy_from_user(&drdy_enable_flag, (void *)arg,
					 sizeof(drdy_enable_flag)) != 0) {
			printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_SET_DRDY_INT] Failed copy from user.\n");
			retVal = -EFAULT;
		} else {
			if (drdy_enable_flag == 0)
			{
				disable_irq(gpio_irq);
			}
			else {
				enable_irq(gpio_irq);
				if (gpio_get_value(vfsSpiDev->drdyPin) == DRDY_ACTIVE_STATUS)
				{
					mdelay(10);
					vfsspi_sendDrdySignal(vfsSpiDev);
				}
			}
		}
		break;
	}
	case VFSSPI_IOCTL_STREAM_READ_START:
	{
		unsigned int streamDataSize;
		printk("[fp]VFSSPI_IOCTL_STREAM_READ_START\n");
		if (copy_from_user(&streamDataSize, (void *)arg,
					sizeof(unsigned int)) != 0) {
			printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_STREAM_READ_START] Failed copy from user.\n");
			retVal = -EFAULT;
		} else {
			if (vfsSpiDev->streamBuffer != NULL) {
				kfree(vfsSpiDev->streamBuffer);
			}
			vfsSpiDev->streamBuffer =
			kmalloc(streamDataSize, GFP_KERNEL);

			if (vfsSpiDev->streamBuffer == NULL) {
                printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_STREAM_READ_START] vfsSpiDev->streamBuffer == NULL");
				retVal = -ENOMEM;
			} else {
				vfsSpiDev->streamBufSize = streamDataSize;
			}
		}
		break;
	}
	case VFSSPI_IOCTL_STREAM_READ_STOP:
	{
		printk("[fp]VFSSPI_IOCTL_STREAM_READ_STOP\n");
		if (vfsSpiDev->streamBuffer != NULL) {
			kfree(vfsSpiDev->streamBuffer);
			vfsSpiDev->streamBuffer = NULL;
			vfsSpiDev->streamBufSize = 0;
		}
		break;

	}
	case VFSSPI_IOCTL_GET_FREQ_TABLE:
	{
		vfsspi_iocFreqTable_t tmpFreqData;

		printk("[fp]VFSSPI_IOCTL_GET_FREQ_TABLE\n");

		retVal = -EINVAL;
		if (copy_from_user(&tmpFreqData, (void *)arg,
			sizeof(vfsspi_iocFreqTable_t)) != 0) {
			printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_GET_FREQ_TABLE] Failed copy from user.\n");
			break;
		}
		tmpFreqData.tblSize = 0;
		if (vfsSpiDev->freqTable != NULL) {
			tmpFreqData.tblSize = vfsSpiDev->freqTableSize;
			if (tmpFreqData.table != NULL)
			{
				if (copy_to_user(tmpFreqData.table,
					vfsSpiDev->freqTable,
					vfsSpiDev->freqTableSize) != 0) {
					printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_GET_FREQ_TABLE] Failed copy to user.\n");
					break;
				}
			}
		}
		if (copy_to_user((void *)arg, &(tmpFreqData),
			sizeof(vfsspi_iocFreqTable_t)) == 0) {
			retVal = 0;
		} else {
			printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_GET_FREQ_TABLE] copy to user failed\n");
		}
		break;
	}
    case VFSSPI_IOCTL_POWER_ON:
    {
		printk("[fp][VFSSPI_IOCTL_POWER_ON]\n");
        retVal = fingerprint_pdata->set_power_control(1);
        if(retVal)
		    printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_ON] success\n");
        else
        {
		    printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_ON] fail\n");
            retVal = -EFAULT;
        }

        break;
    }
    case VFSSPI_IOCTL_POWER_OFF:
    {
		printk("[fp][VFSSPI_IOCTL_POWER_OFF]\n");
        retVal = fingerprint_pdata->set_power_control(0);
        if(retVal)
		    printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_OFF] success\n");
        else
        {
		    printk("[fp]vfsspi_ioctl:[VFSSPI_IOCTL_POWER_OFF] fail\n");
            retVal = -EFAULT;
        }
        break;
    }
	default:
		retVal = -EFAULT;
		break;
	}
	mutex_unlock(&vfsSpiDev->bufferMutex);
	return retVal;
}

void vfsspi_hardReset(struct vfsspi_devData *vfsSpiDev)
{
	if (vfsSpiDev != NULL) {
	    printk("[fp]vfsspi_hardReset\n");
		
		dataToRead = 0;
        fingerprint_pdata->set_sleep_pin(vfsSpiDev->sleepPin, 1);
		mdelay(1);
        fingerprint_pdata->set_sleep_pin(vfsSpiDev->sleepPin, 0);
		
		mdelay(5);
	}
}

void vfsspi_suspend(struct vfsspi_devData *vfsSpiDev)
{
	if (vfsSpiDev != NULL) {
        printk("[fp]vfsspi_suspend\n");
		
		dataToRead = 0;
        fingerprint_pdata->set_sleep_pin(vfsSpiDev->sleepPin, 1);
		
	}
}

int vfsspi_open(struct inode *inode, struct file *filp)
{
	struct vfsspi_devData *vfsSpiDev = NULL;
	int status = -ENXIO;
	printk("[fp]: vfsspi_open\n");
	mutex_lock(&deviceListMutex);
	list_for_each_entry(vfsSpiDev, &deviceList, deviceEntry)
	{
		if (vfsSpiDev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (vfsSpiDev->isOpened != 0) {
			status = -EBUSY;
		} else {
			vfsSpiDev->userPID = 0;
			if (vfsSpiDev->buffer == NULL) {
				vfsSpiDev->nullBuffer =
				kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
				vfsSpiDev->buffer =
				 kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
				if (vfsSpiDev->buffer == NULL
					|| vfsSpiDev->nullBuffer == NULL) {
					status = -ENOMEM;
				} else {
					vfsSpiDev->isOpened = 1;
					filp->private_data = vfsSpiDev;
					nonseekable_open(inode, filp);
				}
			}
		}
	}
	mutex_unlock(&deviceListMutex);
	return status;
}

int vfsspi_release(struct inode *inode, struct file *filp)
{
	struct vfsspi_devData *vfsSpiDev = NULL;
	int                   status     = 0;
	printk("[fp]: vfsspi_release\n");
	mutex_lock(&deviceListMutex);
	vfsSpiDev = filp->private_data;
	filp->private_data = NULL;
	vfsSpiDev->isOpened = 0;
	if (vfsSpiDev->buffer != NULL) {
		kfree(vfsSpiDev->buffer);
		vfsSpiDev->buffer = NULL;
	}
	if (vfsSpiDev->nullBuffer != NULL) {
		kfree(vfsSpiDev->nullBuffer);
		vfsSpiDev->nullBuffer = NULL;
	}
	if (vfsSpiDev->streamBuffer != NULL) {
		kfree(vfsSpiDev->streamBuffer);
		vfsSpiDev->streamBuffer = NULL;
		vfsSpiDev->streamBufSize = 0;
	}
	mutex_unlock(&deviceListMutex);
	return status;
}

#if 1
static int PerformReset(void)
{
  int rc;

  rc = fingerprint_pdata->set_sleep_pin(VFSSPI_SLEEP_PIN, 1);
  if(rc < 0)
      pr_err("[vfsspi] %s: io extender high failed %d\n", __func__, rc);
  mdelay(1);
  rc = fingerprint_pdata->set_sleep_pin(VFSSPI_SLEEP_PIN, 0);
  if(rc < 0)
      pr_err("[vfsspi] %s: io extender low failed %d\n", __func__, rc);
  mdelay(10);

  printk(KERN_ERR "[fp]ValiditySensor: Reset Complete\n");

  return 0;
}

void fpr_test(struct spi_device *spi)
{
  char tx_buf[64] = {0};
  char  rx_buf[64] = {0};
  struct spi_transfer t;
  struct spi_message m;
  int i = 0;
  int j = 40;
  printk(KERN_ERR "[fp]ValiditySensor: Inside spi_probe\n");

  fpr_sensor = -1;
  fp_mount = 0;
  spi->bits_per_word = 8;
  spi->max_speed_hz= SLOW_BAUD_RATE;
  spi->mode = SPI_MODE_0;
  spi_setup(spi);

  PerformReset();
  mdelay(5);

  
  memset(&t, 0, sizeof(t));
  t.tx_buf = tx_buf;
  t.rx_buf = rx_buf;
  t.len = 64;

  spi_message_init(&m);
  spi_message_add_tail(&t, &m);
  printk(KERN_ERR "[fp]ValiditySensor: spi_sync returned %d \n", spi_sync(spi, &m));
  printk(KERN_ERR "[fp]ValiditySensor: Get first 64 bytes\n");
  for(i=0;i<64;i++)
        printk(KERN_ERR "rx[%d] = 0x%0x ", i, rx_buf[i]);
  memset(tx_buf, 0, sizeof(tx_buf));
  memset(rx_buf, 0, sizeof(rx_buf));
  
  tx_buf[0] = 0x45;
  tx_buf[1] = 0x67;
  tx_buf[2] = 0x01;
  tx_buf[3] = 0x23;
  tx_buf[4] = 0x00;
  tx_buf[5] = 0x01;
  tx_buf[6] = 0x01;
  tx_buf[7] = 0x01;
  tx_buf[8] = 0x00;
  tx_buf[9] = 0x01;


  memset(&t, 0, sizeof(t));
  t.tx_buf = tx_buf;
  t.rx_buf = rx_buf;
  t.len = 64;

  spi_message_init(&m);
  spi_message_add_tail(&t, &m);
  printk(KERN_ERR "[fp]ValiditySensor: spi_sync returned %d \n", spi_sync(spi, &m));

again:
  memset(tx_buf, 0, sizeof(tx_buf));
  memset(rx_buf, 0, sizeof(rx_buf));
  memset(&t, 0, sizeof(t));
  t.tx_buf = tx_buf;
  t.rx_buf = rx_buf;
  t.len = 64;


  spi_message_init(&m);
  spi_message_add_tail(&t, &m);
  printk(KERN_ERR "[fp]ValiditySensor: spi_sync returned %d \n", spi_sync(spi, &m));
  printk(KERN_ERR "[fp]ValiditySensor: Get next 64 bytes\n");
  for(i=0;i<64;i++)
        printk(KERN_ERR "rx[%d] = 0x%0x ", i, rx_buf[i]);
   if (rx_buf[6] == 0x09 && j-- > 0)
		goto again;
   else if(rx_buf[6] == 0xff)
   {
        printk(KERN_ERR "[fp]ValiditySensor: Get firmware version fail, please check sesnor is exist or not\n");
   }
   else
   {
        printk(KERN_ERR "[fp]ValiditySensor: Get firmware version\n");
        for(i=0;i<64;i++)
            printk(KERN_ERR "  rx[%d] = 0x%0x  ", i, rx_buf[i]);

        
        if(fingerprint_pdata->read_engineerid()) 
        {
            printk(KERN_ERR "[fp]ValiditySensor COF\n");
            fpr_sensor = 1  ;
        }else
        {
            printk(KERN_ERR "[fp]ValiditySensor BGA\n");
            fpr_sensor = 0;
        }
   }

	if (fpr_sensor == 0)
		fp_mount = 1;
}
#endif


int vfsspi_probe(struct spi_device *spi)
{
	int                   status     = 0;
	struct vfsspi_devData *vfsSpiDev = NULL;
	struct device         *dev       = NULL;
	printk("[fp]vfsspi_probe ++++\n");


    fingerprint_pdata = spi->controller_data;

  if (hasfp == 0 || DISABLE_FP) {
		status = fingerprint_pdata->set_power_control(0);
		printk("[fp]vfsspi_probe: hasfp power = %d -ENODEV\n", status);
		fp_mount = 0;
		return -ENODEV;
  }
    
	fpr_test(spi);
    if(fpr_sensor == 1)
    {
        status = fingerprint_pdata->set_power_control(0);
        if(status)
            printk("[fp]vfsspi_probe:[VFSSPI_IOCTL_POWER_OFF] success\n");
        else
        {
            printk("[fp]vfsspi_probe:[VFSSPI_IOCTL_POWER_OFF] fail\n");
            status = -EFAULT;
        }
        return status;
    }
    device_create_file(&(spi->dev), &dev_attr_fpr);
	vfsSpiDev = kzalloc(sizeof(*vfsSpiDev), GFP_KERNEL);
	if (vfsSpiDev == NULL)
		return -ENOMEM;
	
	vfsSpiDev->curSpiSpeed = SLOW_BAUD_RATE;
	vfsSpiDev->userInfoData.buffer = NULL;
	vfsSpiDev->spi = spi;
	spin_lock_init(&vfsSpiDev->vfsSpiLock);
	mutex_init(&vfsSpiDev->bufferMutex);
	INIT_LIST_HEAD(&vfsSpiDev->deviceEntry);
	status = vfsspi_platformInit(vfsSpiDev);
	if (status == 0) {
		spi->bits_per_word = BITS_PER_WORD;
		spi->max_speed_hz = SLOW_BAUD_RATE;
		spi->mode = SPI_MODE_0;
		status = spi_setup(spi);
		if (status == 0) {
			mutex_lock(&deviceListMutex);
			
			vfsSpiDev->devt = MKDEV(VFSSPI_MAJOR, 0);
			dev = device_create(vfsSpiDevClass, &spi->dev,
				 vfsSpiDev->devt, vfsSpiDev, "vfsspi");
			status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
			if (status == 0)
				list_add(&vfsSpiDev->deviceEntry, &deviceList);
			mutex_unlock(&deviceListMutex);
			if (status == 0)
				spi_set_drvdata(spi, vfsSpiDev);
			else {
				printk("[fp]vfsspi_probe: device_create failed with "
						"status= %d\n", status);
				kfree(vfsSpiDev);
			}
		} else {
			gDevSpi = spi;
			printk("[fp]vfsspi_probe: spi_setup() is failed! status= %d\n", status);
			vfsspi_platformUninit(vfsSpiDev);
			kfree(vfsSpiDev);
		}
	} else {
		printk("[fp]vfsspi_probe: vfsspi_platformInit is failed! status= %d\n", status);
		vfsspi_platformUninit(vfsSpiDev);
		kfree(vfsSpiDev);
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	if (status == 0) {
		early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
		early_suspend.suspend = vfsspi_early_suspend;
		early_suspend.resume = vfsspi_late_resume;
		printk("[fp]vfsspi_probe: registering early suspend\n");
		register_early_suspend(&early_suspend);
	}
#endif
	printk("[fp]vfsspi_probe ----\n");
	return status;
}

int vfsspi_remove(struct spi_device *spi)
{
	int status = 0;
	struct vfsspi_devData *vfsSpiDev = NULL;
	printk("[fp]vfsspi_remove ++++\n");
	vfsSpiDev = spi_get_drvdata(spi);
	if (vfsSpiDev != NULL) {
		gDevSpi = spi;
		spin_lock_irq(&vfsSpiDev->vfsSpiLock);
		vfsSpiDev->spi = NULL;
		spi_set_drvdata(spi, NULL);
		spin_unlock_irq(&vfsSpiDev->vfsSpiLock);
		mutex_lock(&deviceListMutex);
		vfsspi_platformUninit(vfsSpiDev);
		if (vfsSpiDev->userInfoData.buffer != NULL)
			kfree(vfsSpiDev->userInfoData.buffer);
		
		list_del(&vfsSpiDev->deviceEntry);
		device_destroy(vfsSpiDevClass, vfsSpiDev->devt);
		kfree(vfsSpiDev);
		mutex_unlock(&deviceListMutex);
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	printk("[fp]vfsspi_remove: unregistering early suspend\n");
	unregister_early_suspend(&early_suspend);
#endif
	printk("[fp]vfsspi_remove ----\n");
	return status;
}


int vfsspi_platformInit(struct vfsspi_devData *vfsSpiDev)
{
	int status = 0;
	printk("[fp]vfsspi_platformInit ++++\n");
	if (vfsSpiDev != NULL) {
		vfsSpiDev->drdyPin = VFSSPI_DRDY_PIN;
		vfsSpiDev->sleepPin  = VFSSPI_SLEEP_PIN;
		status = gpio_direction_input(vfsSpiDev->drdyPin);
		if (status < 0) {
			DPRINTK("[fp]gpio_direction_input DRDY failed\n");
			return -EBUSY;
		}
		gpio_irq = gpio_to_irq(vfsSpiDev->drdyPin);
		if (gpio_irq < 0) {
			DPRINTK("[fp]gpio_to_irq failed\n");
			return -EBUSY;
		}
		if (request_irq(gpio_irq, vfsspi_irq, IRQF_TRIGGER_FALLING,
					 "vfsspi_irq", vfsSpiDev) < 0) {
			DPRINTK("[fp]request_irq failed\n");
			return -EBUSY;
		}
	vfsSpiDev->freqTable = freqTable;
	vfsSpiDev->freqTableSize = sizeof(freqTable);
	} else {
		status = -EFAULT;
	}
	printk("[fp]vfsspi_platofrminit ---- status=%d\n", status);
	return status;
}

void vfsspi_platformUninit(struct vfsspi_devData *vfsSpiDev)
{
	printk("[fp]vfsspi_platformUninit ++++\n");
	if (vfsSpiDev != NULL) {
		vfsSpiDev->freqTable = NULL;
		vfsSpiDev->freqTableSize = 0;
		free_irq(gpio_irq, vfsSpiDev);
		gpio_free(vfsSpiDev->drdyPin);
	}
	printk("[fp]vfsspi_platformUninit ----\n");
}

static int __init vfsspi_init(void)
{
	int status = 0;
	DPRINTK("[fp]vfsspi_init\n");
	
	status = register_chrdev(VFSSPI_MAJOR, "validity_fingerprint",
							 &vfsspi_fops);
	if (status < 0) {
		printk("[fp]vfsspi_init: register_chrdev failed\n");
		return status;
	}
	vfsSpiDevClass = class_create(THIS_MODULE, "validity_fingerprint");
	if (IS_ERR(vfsSpiDevClass)) {
		printk("[fp]vfsspi_init: class_create() is failed "
						"- unregister chrdev.\n");
		unregister_chrdev(VFSSPI_MAJOR, vfsspi_spi.driver.name);
		return PTR_ERR(vfsSpiDevClass);
	}
	status = spi_register_driver(&vfsspi_spi);
	if (status < 0) {
		printk("[fp]vfsspi_init: spi_register_driver() is failed "
						"- unregister chrdev.\n");
		unregister_chrdev(VFSSPI_MAJOR, vfsspi_spi.driver.name);
		return status;
	}
	DPRINTK("init is successful\n");
	return status;
}

static void __exit vfsspi_exit(void)
{
	DPRINTK("[fp]vfsspi_exit\n");
	spi_unregister_driver(&vfsspi_spi);
	class_destroy(vfsSpiDevClass);
	unregister_chrdev(VFSSPI_MAJOR, vfsspi_spi.driver.name);
}

module_init(vfsspi_init);
module_exit(vfsspi_exit);

MODULE_LICENSE("GPL");
