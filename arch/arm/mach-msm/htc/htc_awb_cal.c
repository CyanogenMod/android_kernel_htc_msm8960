/* Code to extract Camera AWB calibration information from ATAG
set up by the bootloader.

Copyright (C) 2008 Google, Inc.
Author: Dmitry Shmidt <dimitrysh@google.com>

This software is licensed under the terms of the GNU General Public
License version 2, as published by the Free Software Foundation, and
may be copied, distributed, and modified under those terms.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <asm/setup.h>

#include <linux/fs.h>
#include <linux/syscalls.h>

#define ATAG_MSM_AWB_CAL	0x59504550 

#define AWB_CAL_MAX_SIZE	0x2000U     

struct qct_lsc_struct{
	unsigned long int	lsc_verify;
	unsigned long int	lsc_fuseid[4];
	float 			pCalcParam[17*13*4];
	unsigned long int	lsc_checksum;
};

struct qct_awb_lsc_struct{
	unsigned long int caBuff[8];
	struct qct_lsc_struct qct_lsc_data;
	
	unsigned long int flashcaBuff[8];  
	
       
       unsigned long int ois_data[8];
       

};

static unsigned char cam_awb_ram[AWB_CAL_MAX_SIZE];

int gCAM_AWB_CAL_LEN;

unsigned char *get_cam_awb_cal(void)
{
	return cam_awb_ram;
}

EXPORT_SYMBOL(get_cam_awb_cal);
unsigned char *dummy(unsigned char *p)
{
    return p;
}
static int __init parse_tag_cam_awb_cal(const struct tag *tag)
{
	unsigned char *dptr = (unsigned char *)(&tag->u);
	unsigned size;

	size = min((tag->hdr.size - 2) * sizeof(__u32), AWB_CAL_MAX_SIZE);

	printk(KERN_INFO "CAM_AWB_CAL Data size = %d , 0x%x, size = %d (%d,%d)\n",
			tag->hdr.size, tag->hdr.tag, size,
			((tag->hdr.size - 2) * sizeof(__u32)), (AWB_CAL_MAX_SIZE));

	gCAM_AWB_CAL_LEN = size;
	memcpy(cam_awb_ram, dummy(dptr), size); 


#ifdef ATAG_CAM_AWB_CAL_DEBUG
   {
	 int *pint, i;

	 printk(KERN_INFO "parse_tag_cam_awb_cal():\n");

	 pint = (int *)cam_awb_ram;

	 for (i = 0; i < 1024; i++)
	   printk(KERN_INFO "%x\n", pint[i]);

   }
#endif

	return 0;
}

__tagtable(ATAG_MSM_AWB_CAL, parse_tag_cam_awb_cal);


static ssize_t awb_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned char *ptr;

	ptr = get_cam_awb_cal();
	

	ret = sizeof(struct qct_awb_lsc_struct);
	
	printk(KERN_INFO "awb_calibration_show(%d)\n", ret);
	memcpy(buf, ptr, ret);

#ifdef ATAG_CAM_AWB_CAL_DEBUG
   {
	 int i, *pint;
	 printk(KERN_INFO "awb_calibration_show():\n");
	 pint = (int *)buf;
	 for (i = 0; i < 914; i++)
	   printk(KERN_INFO "%d-%x\n", i, pint[i]);

   }
#endif

	return ret;
}

static ssize_t awb_calibration_front_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned char *ptr;

	if (gCAM_AWB_CAL_LEN < AWB_CAL_MAX_SIZE)
		return 0;

	ptr = get_cam_awb_cal();
	

	ret = sizeof(struct qct_awb_lsc_struct);
	
	printk(KERN_INFO "awb_calibration_front_show(%d)\n", ret);
	memcpy(buf, ptr + 0x1000U, ret);


#ifdef ATAG_CAM_AWB_CAL_DEBUG
   {
	 int i, *pint;
	 printk(KERN_INFO "awb_calibration_front_show():\n");
	 pint = (int *)buf;
	 for (i = 0; i < 898; i++)
	   printk(KERN_INFO "%x\n", pint[i]);

   }
#endif

	return ret;
}


static DEVICE_ATTR(awb_cal, 0444, awb_calibration_show, NULL);
static DEVICE_ATTR(awb_cal_front, 0444, awb_calibration_front_show, NULL);


static struct kobject *cam_awb_cal;

static int cam_get_awb_cal(void)
{
	int ret ;

	
	cam_awb_cal = kobject_create_and_add("android_camera_awb_cal", NULL);
	if (cam_awb_cal == NULL) {
		pr_info("cam_get_awb_cal: subsystem_register failed\n");
		ret = -ENOMEM;
		return ret ;
	}

	ret = sysfs_create_file(cam_awb_cal, &dev_attr_awb_cal.attr);
	if (ret) {
		pr_info("cam_get_awb_cal:: sysfs_create_file failed\n");
		kobject_del(cam_awb_cal);
		goto end;
	}

	if (gCAM_AWB_CAL_LEN < AWB_CAL_MAX_SIZE)
		goto end;

	ret = sysfs_create_file(cam_awb_cal, &dev_attr_awb_cal_front.attr);
	if (ret) {
		pr_info("cam_get_awb_cal_front:: sysfs_create_file failed\n");
		kobject_del(cam_awb_cal);
		goto end;
	}

end:
	return 0 ;
}

late_initcall(cam_get_awb_cal);
