/*
 *
 * /arch/arm/mach-msm/include/mach/htc_headset_pmic.h
 *
 * HTC PMIC headset driver.
 *
 * Copyright (C) 2010 HTC, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/termios.h>
#include <linux/tty.h>

#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_one_wire.h>

#define DRIVER_NAME "HS_1WIRE"
static struct htc_35mm_1wire_info *hi;
static struct file *fp;

static struct file *openFile(char *path,int flag,int mode)
{
	mm_segment_t old_fs;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp=filp_open(path, flag, mode);
	set_fs(old_fs);
	if(IS_ERR(fp))
	   HS_LOG("File Open Error:%s",path);

	if(!fp->f_op)
	   HS_LOG("File Operation Method Error!!");

	if (fp) return fp;
	else return NULL;
}

static int readFile(struct file *fp,char *buf,int readlen)
{
	mm_segment_t old_fs;
	int ret;

	if (fp && fp->f_op && fp->f_op->read) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		ret = fp->f_op->read(fp, buf, readlen, &fp->f_pos);
		set_fs(old_fs);
		return ret;
	}
	return -1;
}

static int writeFile(struct file *fp,char *buf,int readlen)
{
	mm_segment_t old_fs;
	int ret;

	if (fp && fp->f_op && fp->f_op->write) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		ret = fp->f_op->write(fp, buf, readlen, &fp->f_pos);
		set_fs(old_fs);
		return ret;
	}
	return -1;
}

static void setup_hs_tty(struct file *tty_fp)
{
	struct termios hs_termios;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	tty_ioctl(tty_fp, TCGETS, (unsigned long)&hs_termios);
	hs_termios.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	hs_termios.c_oflag &= ~OPOST;
	hs_termios.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	hs_termios.c_cflag &= ~(CSIZE|CBAUD|PARENB|CSTOPB);
	hs_termios.c_cflag |= (CREAD|CS8|CLOCAL|CRTSCTS|B38400);
	tty_ioctl(tty_fp, TCSETS, (unsigned long)&hs_termios);
	set_fs(old_fs);
}

int closeFile(struct file *fp)
{
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	filp_close(fp,NULL);
	set_fs(old_fs);
	return 0;
}

static int hs_read_aid(void)
{
	char in_buf[10];
	int read_count, i;
		read_count = readFile(fp, in_buf, 10);
		HS_LOG("[1wire]read_count = %d", read_count);
		if (read_count > 0) {
			for (i = 0; i < read_count; i++) {
				HS_LOG("[1wire]in_buf[%d] = 0x%x", i, in_buf[i]);
				if ((in_buf[i] & 0xF0) == 0x80 && in_buf[i] > 0x80) {
					hi->aid = in_buf[i];
					return 0;
				}
			}
		}
	return -1;

}

static int hs_1wire_query(int type)
{
	return 0; 
}

static int hs_1wire_read_key(void)
{
	char key_code[10];
	int read_count, i;
	HS_LOG("[1-wire]hs_1wire_read_key");
		read_count = readFile(fp, key_code, 10);
		HS_LOG("[1wire]key read_count = %d", read_count);
		if (read_count > 0) {
			for (i = 0; i < read_count; i++) {
				HS_LOG("[1wire]key_code[%d] = 0x%x", i, key_code[i]);
				if (key_code[i] == hi->pdata.one_wire_remote[0])
					return 1;
				else if (key_code[i] == hi->pdata.one_wire_remote[2])
					return 2;
				else if (key_code[i] == hi->pdata.one_wire_remote[4])
					return 3;
				else if (key_code[i] == hi->pdata.one_wire_remote[1])
					return 0;
				else
					HS_LOG("Non key data, dropped");
			}
		}
	return -1;
}

static int hs_1wire_init(void)
{
	char all_zero = 0;
	char send_data = 0x35;

	HS_LOG("[1-wire]hs_1wire_init");
	fp = openFile(hi->pdata.onewire_tty_dev,O_CREAT|O_RDWR|O_SYNC,0666);
	HS_LOG("Open %s", hi->pdata.onewire_tty_dev);
	if (!(fp->private_data)) {
		HS_LOG("No private data");
		return -1;
	}
	setup_hs_tty(fp);
	HS_LOG("Setup HS tty");
	if (hi->pdata.tx_level_shift_en) {
		gpio_set_value_cansleep(hi->pdata.tx_level_shift_en, 0); 
		HS_LOG("[HS]set tx_level_shift_en to 0");
	}
	if (hi->pdata.uart_sw)
		gpio_set_value_cansleep(hi->pdata.uart_sw, 1); 
	hi->aid = 0;
	hr_msleep(20);
	writeFile(fp,&all_zero,1);
	hr_msleep(5);
	writeFile(fp,&send_data,1);
	if (hi->pdata.remote_press) {
		while(gpio_get_value(hi->pdata.remote_press) == 1) {
			HS_LOG("[HS]Polling remote_press low");
		}
	}
	hr_msleep(1);
	if (hi->pdata.tx_level_shift_en)
		gpio_set_value(hi->pdata.tx_level_shift_en, 1);
	HS_LOG("[HS]Disable level shift");
	hr_msleep(22);
	if (hs_read_aid() == 0) {
		HS_LOG("[1-wire]Valid AID received, enter 1-wire mode");
		return 0;
	} else {
		if (hi->pdata.tx_level_shift_en)
			gpio_set_value_cansleep(hi->pdata.tx_level_shift_en, 1);
		if (hi->pdata.uart_sw)
			gpio_set_value_cansleep(hi->pdata.uart_sw, 0);
		hi->aid = 0;
		closeFile(fp);
		return -1;
	}
}

static void hs_1wire_deinit(void)
{
	char all_zero = 0xaa;
	if (fp) {
		if (hi->pdata.tx_level_shift_en)
			gpio_set_value_cansleep(hi->pdata.tx_level_shift_en, 0);
		hr_msleep(20);
		writeFile(fp, &all_zero, 1); 
		HS_LOG("Write 0xaa to unblock");
		hr_msleep(10);
		HS_LOG("close file");
		closeFile(fp);
		fp = NULL;
		if (hi->pdata.tx_level_shift_en)
			gpio_set_value_cansleep(hi->pdata.tx_level_shift_en, 1);
	}
}

static int hs_1wire_report_type(char **string)
{
	const int type_num = 3; 
	char *hs_type[] = {
		"headset_beats_20", 
		"headset_mic_midtier",
		"headset_beats_solo_20", 
	};
	hi->aid &= 0x7f;
	HS_LOG("[1wire]AID = 0x%x", hi->aid);
	if (hi->aid > type_num || hi->aid < 1) {
		*string = "1wire_unknown";
		return 14;
	}else {
		*string = hs_type[hi->aid - 1];
		HS_LOG("Report %s type, size %d", *string, sizeof(hs_type[hi->aid -1]));
		return sizeof(hs_type[hi->aid -1]);
	}
}

static void hs_1wire_register(void)
{
	struct headset_notifier notifier;

		notifier.id = HEADSET_REG_1WIRE_INIT;
		notifier.func = hs_1wire_init;
		headset_notifier_register(&notifier);

		notifier.id = HEADSET_REG_1WIRE_QUERY;
		notifier.func = hs_1wire_query;
		headset_notifier_register(&notifier);

		notifier.id = HEADSET_REG_1WIRE_READ_KEY;
		notifier.func = hs_1wire_read_key;
		headset_notifier_register(&notifier);

		notifier.id = HEADSET_REG_1WIRE_DEINIT;
		notifier.func = hs_1wire_deinit;
		headset_notifier_register(&notifier);

		notifier.id = HEADSET_REG_1WIRE_REPORT_TYPE;
		notifier.func = hs_1wire_report_type;
		headset_notifier_register(&notifier);

}

void one_wire_gpio_tx(int enable)
{
	HS_LOG("Set gpio[%d] = %d", hi->pdata.uart_tx, enable);
	gpio_set_value(hi->pdata.uart_tx, enable);
}

void one_wire_lv_en(int enable)
{
	gpio_set_value(hi->pdata.tx_level_shift_en, 0);
}

void one_wire_uart_sw(int enable)
{
	gpio_set_value(hi->pdata.uart_sw, enable);
}

static int htc_headset_1wire_probe(struct platform_device *pdev)
{
	struct htc_headset_1wire_platform_data *pdata = pdev->dev.platform_data;
	HS_LOG("1-wire probe starts");

	hi = kzalloc(sizeof(struct htc_35mm_1wire_info), GFP_KERNEL);
	if (!hi)
		return -ENOMEM;

	hi->pdata.tx_level_shift_en = pdata->tx_level_shift_en;
	hi->pdata.uart_sw = pdata->uart_sw;
	if (pdata->one_wire_remote[5])
		memcpy(hi->pdata.one_wire_remote, pdata->one_wire_remote,
		       sizeof(hi->pdata.one_wire_remote));
	hi->pdata.uart_tx = pdata->uart_tx;
	hi->pdata.uart_rx = pdata->uart_rx;
	hi->pdata.remote_press = pdata->remote_press;
	strncpy(hi->pdata.onewire_tty_dev, pdata->onewire_tty_dev, 15);
	HS_LOG("1wire tty device %s", hi->pdata.onewire_tty_dev);
	hs_1wire_register();
	hs_notify_driver_ready(DRIVER_NAME);

	HS_LOG("--------------------");

	return 0;
}

static int htc_headset_1wire_remove(struct platform_device *pdev)
{
	return 0;
}


static struct platform_driver htc_headset_1wire_driver = {
	.probe	= htc_headset_1wire_probe,
	.remove	= htc_headset_1wire_remove,
	.driver	= {
		.name	= "HTC_HEADSET_1WIRE",
		.owner	= THIS_MODULE,
	},
};

static int __init htc_headset_1wire_init(void)
{
	return platform_driver_register(&htc_headset_1wire_driver);
}

static void __exit htc_headset_1wire_exit(void)
{
	platform_driver_unregister(&htc_headset_1wire_driver);
}

module_init(htc_headset_1wire_init);
module_exit(htc_headset_1wire_exit);

MODULE_DESCRIPTION("HTC 1-wire headset driver");
MODULE_LICENSE("GPL");
