/* drivers/power/ncp6924.c
 *
 * Copyright (C) 2008-2009 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <mach/ncp6924.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/mfd/pm8xxx/gpio.h>
#include <linux/uaccess.h>

#define NCP6924_FMT(fmt) "[NCP6924] " fmt
#define NCP6924_ERR_FMT(fmt) "[NCP6924] err:" fmt
#define NCP6924_INFO(fmt, ...) \
	printk(KERN_INFO NCP6924_FMT(fmt), ##__VA_ARGS__)
#define NCP6924_ERR(fmt, ...) \
	printk(KERN_ERR NCP6924_ERR_FMT(fmt), ##__VA_ARGS__)

static int ncp6924_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_client *this_client;
static struct mutex ncp6924_mutex;
static int en_gpio = 0;

static struct dentry *debugfs;

struct pm_gpio ncp6924_gpio_en = {
	.direction      = PM_GPIO_DIR_OUT,
	.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
	.output_value   = 0,
	.pull           = PM_GPIO_PULL_DN,
	.vin_sel        = PM_GPIO_VIN_S4,
	.out_strength   = PM_GPIO_STRENGTH_HIGH,
	.function       = PM_GPIO_FUNC_NORMAL,
};

static int ncp6924_en(bool onoff)
{
	int ret = 0;

	if (onoff) {
		ncp6924_gpio_en.output_value = 1;
		ncp6924_gpio_en.pull = PM_GPIO_PULL_UP_30;
		ret = pm8xxx_gpio_config(en_gpio, &ncp6924_gpio_en);
	} else {
		ncp6924_gpio_en.output_value = 0;
		ncp6924_gpio_en.pull = PM_GPIO_PULL_DN;
		ret = pm8xxx_gpio_config(en_gpio, &ncp6924_gpio_en);
	}

	return ret;
}

static int ncp6924_i2c_write(u8 reg, u8 data)
{
	int res;
	int curr_state;
	u8 values[] = {reg, data};

	struct i2c_msg msg[] = {
		{
		  .addr  = this_client->addr,
		  .flags = 0,
		  .len   = 2,
		  .buf   = values,
		},
	};

	mutex_lock(&ncp6924_mutex);
	curr_state = ncp6924_gpio_en.output_value;
	if (curr_state == 0)
		ncp6924_en(true);
	res = i2c_transfer(this_client->adapter, msg, 1);
	if (curr_state == 0) {
		if (reg == NCP6924_ENABLE && data != 0x00)
			ncp6924_en(true);
		else
			ncp6924_en(false);
	} else {
		if (reg == NCP6924_ENABLE && data == 0x00)
			ncp6924_en(false);
	}
	mutex_unlock(&ncp6924_mutex);
	if (res >=0)
		res = 0;

	return res;

}

static int ncp6924_i2c_read(u8 reg, u8 *data)
{
	int res;
	int curr_state;

	struct i2c_msg msg[] = {
		{
		  .addr  = this_client->addr,
		  .flags = 0,
		  .len   = 1,
		  .buf   = &reg,
		},
		{
		  .addr  = this_client->addr,
		  .flags = I2C_M_RD,
		  .len   = 1,
		  .buf   = data,
		},
	};

	mutex_lock(&ncp6924_mutex);
	curr_state = ncp6924_gpio_en.output_value;
	if (curr_state == 0)
		ncp6924_en(true);
	res = i2c_transfer(this_client->adapter, msg, 2);
	if (curr_state == 0)
		ncp6924_en(false);
	mutex_unlock(&ncp6924_mutex);
	if (res >=0)
		res = 0;

	return res;

}

u8 ncp6924_read_reg(u8 reg)
{
	u8 data = 0;
	ncp6924_i2c_read(reg, &data);
	return data;
}
EXPORT_SYMBOL(ncp6924_read_reg);

void ncp6924_dump_regs(void)
{
	int i = 0;
	u8 data = 0;
	u8 regs[] = {
		0x14, 0x20, 0x22, 0x24, 0x25, 0x26, 0x27
	};
	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		data = 0;
		data = ncp6924_read_reg(regs[i]);
		NCP6924_INFO("reg: 0x%x, value: 0x%x\n", regs[i], data);
	}
}
EXPORT_SYMBOL(ncp6924_dump_regs);

int ncp6924_config_enable(u8 data)
{
	int err = -EINVAL;
	err = ncp6924_i2c_write(NCP6924_ENABLE, data);
	NCP6924_INFO("config enable to 0x%x (gpio: %d)\n", data, ncp6924_gpio_en.output_value);
	return err;
}

int ncp6924_config_dcdc(u8 id, u8 data)
{
	int err = -EINVAL;

	switch(id)
	{
		case NCP6924_ID_DCDC1:
			err = ncp6924_i2c_write(NCP6924_VPROGDCDC1, data);
			break;
		case NCP6924_ID_DCDC2:
			err = ncp6924_i2c_write(NCP6924_VPROGDCDC2, data);
			break;
	}

	return err;
}
EXPORT_SYMBOL(ncp6924_config_dcdc);

int ncp6924_config_ldo(u8 id, u8 data)
{
	int err = -EINVAL;

	switch(id)
	{
		case NCP6924_ID_LDO1:
			err = ncp6924_i2c_write(NCP6924_VPROGLDO1, data);
			break;
		case NCP6924_ID_LDO2:
			err = ncp6924_i2c_write(NCP6924_VPROGLDO2, data);
			break;
		case NCP6924_ID_LDO3:
			err = ncp6924_i2c_write(NCP6924_VPROGLDO3, data);
			break;
		case NCP6924_ID_LDO4:
			err = ncp6924_i2c_write(NCP6924_VPROGLDO4, data);
			break;
	}

	return err;
}
EXPORT_SYMBOL(ncp6924_config_ldo);

int ncp6924_enable_dcdc(u8 id, bool onoff)
{
	int ret = 0;
	u8 reg = ncp6924_read_reg(NCP6924_ENABLE);
	u8 id_bit = 0;

	switch(id) {
		case NCP6924_ID_DCDC1:
			id_bit = NCP6924_ENABLE_BIT_DCDC1;
			break;
		case NCP6924_ID_DCDC2:
			id_bit = NCP6924_ENABLE_BIT_DCDC2;
			break;
		default:
			NCP6924_INFO("INVALID DCDC Id: %d\n", id);
			return -EINVAL;
	}

	if (onoff) {
		reg |= (0x1 << id_bit);
	} else {
		reg &= ~(0x1 << id_bit);
	}

	ret = ncp6924_config_enable(reg);

	return ret;
}
EXPORT_SYMBOL(ncp6924_enable_dcdc);

int ncp6924_enable_ldo(u8 id, bool onoff)
{
	int ret = 0;
	u8 reg = ncp6924_read_reg(NCP6924_ENABLE);
	u8 id_bit = 0;

	switch(id) {
		case NCP6924_ID_LDO1:
			id_bit = NCP6924_ENABLE_BIT_LDO1;
			break;
		case NCP6924_ID_LDO2:
			id_bit = NCP6924_ENABLE_BIT_LDO2;
			break;
		case NCP6924_ID_LDO3:
			id_bit = NCP6924_ENABLE_BIT_LDO3;
			break;
		case NCP6924_ID_LDO4:
			id_bit = NCP6924_ENABLE_BIT_LDO4;
			break;
		default:
			NCP6924_INFO("INVALID LDO Id: %d\n", id);
			return -EINVAL;
	}

	if (onoff) {
		reg |= (0x1 << id_bit);
	} else {
		reg &= ~(0x1 << id_bit);
	}

	ret = ncp6924_config_enable(reg);

	return ret;
}
EXPORT_SYMBOL(ncp6924_enable_ldo);

#ifdef CONFIG_DEBUG_FS

#define MAX_DEBUG_BUF_LEN 50
static char debug_buf[MAX_DEBUG_BUF_LEN];

static int ncp6924_enable_open(struct inode *inode, struct file *filp)
{
	if (IS_ERR(filp) || filp == NULL) {
		NCP6924_ERR("[DEBUGFS] Enable Register Open Error %ld\n", PTR_ERR(filp));
		return -ENOMEM;
	}

	filp->private_data = inode->i_private;
	return 0;
}

static ssize_t ncp6924_enable_read(struct file *filp, char __user *buffer,
		size_t count, loff_t *ppos)
{
	u8 en_reg;
	int output, rc;
	if (IS_ERR(filp) || filp == NULL) {
		NCP6924_ERR("[DEBUGFS] Enable Register Read Error %ld\n", PTR_ERR(filp));
		return -ENOMEM;
	}

	en_reg = ncp6924_read_reg(NCP6924_ENABLE);

	output = snprintf(debug_buf, MAX_DEBUG_BUF_LEN-1, "0x%x (gpio:%d)\n", en_reg, ncp6924_gpio_en.output_value);
	rc = simple_read_from_buffer((void __user *) buffer, output, ppos,
					(void *) debug_buf, output);
	return rc;
}

static ssize_t ncp6924_enable_write(struct file *filp, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	unsigned int val = 0;
	int rc = 0;
	if (IS_ERR(filp) || filp == NULL) {
		NCP6924_ERR("[DEBUGFS] Enable Register Write Error %ld\n", PTR_ERR(filp));
		return -ENOMEM;
	}

	if (count < MAX_DEBUG_BUF_LEN) {
		if (copy_from_user(debug_buf, (void __user *) buffer, count))
			return -EFAULT;

		debug_buf[count] = '\0';
		rc = sscanf(debug_buf, "0x%x", &val);
		if (rc == 1) {
			ncp6924_config_enable((u8)val);
			NCP6924_INFO("[DEBUGFS] Set ENABLE to 0x%x (read: 0x%x)\n", val, ncp6924_read_reg(NCP6924_ENABLE));
		} else
			NCP6924_INFO("[DEBUGFS] Usage: echo 0xNN > reg_enable\n");
	}

	return count;
}

struct file_operations ncp6924_enable_fops = {
	.owner  = THIS_MODULE,
	.open   = ncp6924_enable_open,
	.write  = ncp6924_enable_write,
	.read   = ncp6924_enable_read,
};

static int ncp6924_init_debugfs(void)
{
	struct dentry *en_reg;
	debugfs = debugfs_create_dir("ncp6924", NULL);
	if (!debugfs)
		return -ENOENT;

	en_reg = debugfs_create_file("reg_enable", 0644, debugfs, NULL, &ncp6924_enable_fops);
	if (!en_reg)
		goto Fail;

	return 0;

Fail:
	debugfs_remove_recursive(debugfs);
	debugfs = NULL;
	return -ENOENT;
}
#endif

static int ncp6924_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ncp6924_platform_data *pdata;
	int err = 0;

	NCP6924_INFO("%s\n", __func__);
	pdata = client->dev.platform_data;
	if (!pdata) {
		NCP6924_INFO("%s: Assign platform data error\n", __func__);
		return -EINVAL;
	}

	if(pdata->voltage_init)
		pdata->voltage_init();

	this_client = client;
	mutex_init(&ncp6924_mutex);

	en_gpio = pdata->en_gpio;

	ncp6924_en(true);
	ncp6924_config_enable(pdata->en_init_val | 0x80);
	ncp6924_config_dcdc(NCP6924_ID_DCDC1, pdata->dc1_init_val);
	ncp6924_config_dcdc(NCP6924_ID_DCDC2, pdata->dc2_init_val);
	ncp6924_config_ldo(NCP6924_ID_LDO1, pdata->ldo1_init_val);
	ncp6924_config_ldo(NCP6924_ID_LDO2, pdata->ldo2_init_val);
	ncp6924_config_ldo(NCP6924_ID_LDO3, pdata->ldo3_init_val);
	ncp6924_config_ldo(NCP6924_ID_LDO4, pdata->ldo4_init_val);
	ncp6924_dump_regs();


#ifdef CONFIG_DEBUG_FS
	ncp6924_init_debugfs();
#endif

	return err;
}

static int ncp6924_remove(struct i2c_client *client)
{
	mutex_destroy(&ncp6924_mutex);
	if (debugfs)
		debugfs_remove_recursive(debugfs);
	NCP6924_INFO("%s:\n", __func__);
	return 0;
}

static int ncp6924_suspend(struct i2c_client *client, pm_message_t state)
{
	u8 data = ncp6924_read_reg(NCP6924_ENABLE);
	if (data != 0x00) {
		NCP6924_ERR("ENABLE_REGISTER still keeps value=0x%x before suspend\n", data);
		ncp6924_config_dcdc(NCP6924_ID_DCDC1, 0);
		ncp6924_config_dcdc(NCP6924_ID_DCDC2, 0);
		ncp6924_config_ldo(NCP6924_ID_LDO1, 0);
		ncp6924_config_ldo(NCP6924_ID_LDO2, 0);
		ncp6924_config_ldo(NCP6924_ID_LDO3, 0);
		ncp6924_config_ldo(NCP6924_ID_LDO4, 0);
	}

	return ncp6924_en(false);
}

static int ncp6924_resume(struct i2c_client *client)
{
	u8 data = 0;

	data = ncp6924_read_reg(NCP6924_ENABLE);
	if (data == 0x00) {
		return ncp6924_en(false);;
	} else {
		NCP6924_ERR("ENABLE_REGISTER=0x%x after resume\n", data);
	}

	return 0;
}

static const struct i2c_device_id ncp6924_id[] = {
	{ "ncp6924", 0 },
	{ }
};

static struct i2c_driver ncp6924_driver = {
	.driver.name  =	"ncp6924",
	.probe		  = ncp6924_probe,
	.remove		  = ncp6924_remove,
	.suspend	  =	ncp6924_suspend,
	.resume		  = ncp6924_resume,
	.id_table	  = ncp6924_id,
};

static int __init regulator_ncp6924_init(void)
{
	int res = 0;

	res = i2c_add_driver(&ncp6924_driver);
	if(res)
		NCP6924_ERR("Driver registration failed \n");

	return res;
}

static void __exit regulator_ncp6924_exit(void)
{
	i2c_del_driver(&ncp6924_driver);
}

MODULE_AUTHOR("Jim Hsia <Jim_Hsia@htc.com>");
MODULE_DESCRIPTION("ncp6924 driver");
MODULE_LICENSE("GPL");

fs_initcall(regulator_ncp6924_init);
module_exit(regulator_ncp6924_exit);
