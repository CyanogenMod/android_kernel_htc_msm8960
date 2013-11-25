#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <asm/uaccess.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <scsi/scsi_dbg.h>

#include "scsi_logging.h"

#define NORMAL_RETRIES			5
#define IOCTL_NORMAL_TIMEOUT			(10 * HZ)

#define MAX_BUF PAGE_SIZE

static int ioctl_probe(struct Scsi_Host *host, void __user *buffer)
{
	unsigned int len, slen;
	const char *string;

	if (buffer) {
		if (get_user(len, (unsigned int __user *) buffer))
			return -EFAULT;

		if (host->hostt->info)
			string = host->hostt->info(host);
		else
			string = host->hostt->name;
		if (string) {
			slen = strlen(string);
			if (len > slen)
				len = slen + 1;
			if (copy_to_user(buffer, string, len))
				return -EFAULT;
		}
	}
	return 1;
}


static int ioctl_internal_command(struct scsi_device *sdev, char *cmd,
				  int timeout, int retries)
{
	int result;
	struct scsi_sense_hdr sshdr;

	SCSI_LOG_IOCTL(1, printk("Trying ioctl with scsi command %d\n", *cmd));

	result = scsi_execute_req(sdev, cmd, DMA_NONE, NULL, 0,
				  &sshdr, timeout, retries, NULL);

	SCSI_LOG_IOCTL(2, printk("Ioctl returned  0x%x\n", result));

	if ((driver_byte(result) & DRIVER_SENSE) &&
	    (scsi_sense_valid(&sshdr))) {
		switch (sshdr.sense_key) {
		case ILLEGAL_REQUEST:
			if (cmd[0] == ALLOW_MEDIUM_REMOVAL)
				sdev->lockable = 0;
			else
				printk(KERN_INFO "ioctl_internal_command: "
				       "ILLEGAL REQUEST asc=0x%x ascq=0x%x\n",
				       sshdr.asc, sshdr.ascq);
			break;
		case NOT_READY:	
			if (sdev->removable)
				break;
		case UNIT_ATTENTION:
			if (sdev->removable) {
				sdev->changed = 1;
				result = 0;	
				break;
			}
		default:	
			sdev_printk(KERN_INFO, sdev,
				    "ioctl_internal_command return code = %x\n",
				    result);
			scsi_print_sense_hdr("   ", &sshdr);
			break;
		}
	}

	SCSI_LOG_IOCTL(2, printk("IOCTL Releasing command\n"));
	return result;
}

int scsi_set_medium_removal(struct scsi_device *sdev, char state)
{
	char scsi_cmd[MAX_COMMAND_SIZE];
	int ret;

	if (!sdev->removable || !sdev->lockable)
	       return 0;

	scsi_cmd[0] = ALLOW_MEDIUM_REMOVAL;
	scsi_cmd[1] = 0;
	scsi_cmd[2] = 0;
	scsi_cmd[3] = 0;
	scsi_cmd[4] = state;
	scsi_cmd[5] = 0;

	ret = ioctl_internal_command(sdev, scsi_cmd,
			IOCTL_NORMAL_TIMEOUT, NORMAL_RETRIES);
	if (ret == 0)
		sdev->locked = (state == SCSI_REMOVAL_PREVENT);
	return ret;
}
EXPORT_SYMBOL(scsi_set_medium_removal);

static int scsi_ioctl_get_pci(struct scsi_device *sdev, void __user *arg)
{
	struct device *dev = scsi_get_device(sdev->host);
	const char *name;

        if (!dev)
		return -ENXIO;

	name = dev_name(dev);

        return copy_to_user(arg, name, min(strlen(name), (size_t)20))
		? -EFAULT: 0;
}


int scsi_ioctl(struct scsi_device *sdev, int cmd, void __user *arg)
{
	char scsi_cmd[MAX_COMMAND_SIZE];

	
	if (!sdev)
		return -ENXIO;

	if (!scsi_block_when_processing_errors(sdev))
		return -ENODEV;

	switch (cmd) {
	case SCSI_IOCTL_SEND_COMMAND:
	case SCSI_IOCTL_TEST_UNIT_READY:
	case SCSI_IOCTL_BENCHMARK_COMMAND:
	case SCSI_IOCTL_SYNC:
	case SCSI_IOCTL_START_UNIT:
	case SCSI_IOCTL_STOP_UNIT:
		printk(KERN_WARNING "program %s is using a deprecated SCSI "
		       "ioctl, please convert it to SG_IO\n", current->comm);
		break;
	default:
		break;
	}

	switch (cmd) {
	case SCSI_IOCTL_GET_IDLUN:
		if (!access_ok(VERIFY_WRITE, arg, sizeof(struct scsi_idlun)))
			return -EFAULT;

		__put_user((sdev->id & 0xff)
			 + ((sdev->lun & 0xff) << 8)
			 + ((sdev->channel & 0xff) << 16)
			 + ((sdev->host->host_no & 0xff) << 24),
			 &((struct scsi_idlun __user *)arg)->dev_id);
		__put_user(sdev->host->unique_id,
			 &((struct scsi_idlun __user *)arg)->host_unique_id);
		return 0;
	case SCSI_IOCTL_GET_BUS_NUMBER:
		return put_user(sdev->host->host_no, (int __user *)arg);
	case SCSI_IOCTL_PROBE_HOST:
		return ioctl_probe(sdev->host, arg);
	case SCSI_IOCTL_SEND_COMMAND:
		if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
			return -EACCES;
		return sg_scsi_ioctl(sdev->request_queue, NULL, 0, arg);
	case SCSI_IOCTL_DOORLOCK:
		return scsi_set_medium_removal(sdev, SCSI_REMOVAL_PREVENT);
	case SCSI_IOCTL_DOORUNLOCK:
		return scsi_set_medium_removal(sdev, SCSI_REMOVAL_ALLOW);
	case SCSI_IOCTL_TEST_UNIT_READY:
		return scsi_test_unit_ready(sdev, IOCTL_NORMAL_TIMEOUT,
					    NORMAL_RETRIES, NULL);
	case SCSI_IOCTL_START_UNIT:
		scsi_cmd[0] = START_STOP;
		scsi_cmd[1] = 0;
		scsi_cmd[2] = scsi_cmd[3] = scsi_cmd[5] = 0;
		scsi_cmd[4] = 1;
		return ioctl_internal_command(sdev, scsi_cmd,
				     START_STOP_TIMEOUT, NORMAL_RETRIES);
	case SCSI_IOCTL_STOP_UNIT:
		scsi_cmd[0] = START_STOP;
		scsi_cmd[1] = 0;
		scsi_cmd[2] = scsi_cmd[3] = scsi_cmd[5] = 0;
		scsi_cmd[4] = 0;
		return ioctl_internal_command(sdev, scsi_cmd,
				     START_STOP_TIMEOUT, NORMAL_RETRIES);
        case SCSI_IOCTL_GET_PCI:
                return scsi_ioctl_get_pci(sdev, arg);
	default:
		if (sdev->host->hostt->ioctl)
			return sdev->host->hostt->ioctl(sdev, cmd, arg);
	}
	return -EINVAL;
}
EXPORT_SYMBOL(scsi_ioctl);

int scsi_nonblockable_ioctl(struct scsi_device *sdev, int cmd,
			    void __user *arg, int ndelay)
{
	int val, result;

	if (ndelay) {
		if (scsi_host_in_recovery(sdev->host))
			return -ENODEV;
	} else if (!scsi_block_when_processing_errors(sdev))
		return -ENODEV;

	switch (cmd) {
	case SG_SCSI_RESET:
		result = get_user(val, (int __user *)arg);
		if (result)
			return result;
		if (val == SG_SCSI_RESET_NOTHING)
			return 0;
		switch (val) {
		case SG_SCSI_RESET_DEVICE:
			val = SCSI_TRY_RESET_DEVICE;
			break;
		case SG_SCSI_RESET_TARGET:
			val = SCSI_TRY_RESET_TARGET;
			break;
		case SG_SCSI_RESET_BUS:
			val = SCSI_TRY_RESET_BUS;
			break;
		case SG_SCSI_RESET_HOST:
			val = SCSI_TRY_RESET_HOST;
			break;
		default:
			return -EINVAL;
		}
		if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
			return -EACCES;
		return (scsi_reset_provider(sdev, val) ==
			SUCCESS) ? 0 : -EIO;
	}
	return -ENODEV;
}
EXPORT_SYMBOL(scsi_nonblockable_ioctl);
