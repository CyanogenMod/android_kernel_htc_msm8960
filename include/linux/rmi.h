/*
 * Copyright (c) 2011 Synaptics Incorporated
 * Copyright (c) 2011 Unixphere
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _RMI_H
#define _RMI_H
#include <linux/kernel.h>
#include <linux/lockdep.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/stat.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif


#define RMI_RO_ATTR S_IRUGO
#define RMI_RW_ATTR (S_IRUGO | S_IWUGO)
#define RMI_WO_ATTR S_IWUGO

#define PDT_START_SCAN_LOCATION 0x00e9

enum rmi_irq_polarity {
	RMI_IRQ_ACTIVE_LOW = 0,
	RMI_IRQ_ACTIVE_HIGH = 1
};

struct rmi_f11_2d_axis_alignment {
	bool swap_axes;
	bool flip_x;
	bool flip_y;
	int clip_X_low;
	int clip_Y_low;
	int clip_X_high;
	int clip_Y_high;
	int offset_X;
	int offset_Y;
	int rel_report_enabled;
};

union rmi_f11_2d_ctrl0 {
	struct {
		u8 reporting_mode:3;
		u8 abs_pos_filt:1;
		u8 rel_pos_filt:1;
		u8 rel_ballistics:1;
		u8 dribble:1;
		u8 report_beyond_clip:1;
	};
	u8 reg;
};

union rmi_f11_2d_ctrl1 {
	struct {
		u8 palm_detect_thres:4;
		u8 motion_sensitivity:2;
		u8 man_track_en:1;
		u8 man_tracked_finger:1;
	};
	u8 reg;
};

union rmi_f11_2d_ctrl2__3 {
	struct {
		u8 delta_x_threshold:8;
		u8 delta_y_threshold:8;
	};
	u8 regs[2];
};

union rmi_f11_2d_ctrl4 {
	struct {
		u8 velocity:8;
	};
	u8 reg;
};

union rmi_f11_2d_ctrl5 {
	struct {
		u8 acceleration:8;
	};
	u8 reg;
};

union rmi_f11_2d_ctrl6__7 {
	struct {
		u16 sensor_max_x_pos:12;
	};
	u8 regs[2];
};

union rmi_f11_2d_ctrl8__9 {
	struct {
		u16 sensor_max_y_pos:12;
	};
	u8 regs[2];
};

union rmi_f11_2d_ctrl10 {
	struct {
		u8 single_tap_int_enable:1;
		u8 tap_n_hold_int_enable:1;
		u8 double_tap_int_enable:1;
		u8 early_tap_int_enable:1;
		u8 flick_int_enable:1;
		u8 press_int_enable:1;
		u8 pinch_int_enable:1;
	};
	u8 reg;
};

union rmi_f11_2d_ctrl11 {
	struct {
		u8 palm_detect_int_enable:1;
		u8 rotate_int_enable:1;
		u8 touch_shape_int_enable:1;
		u8 scroll_zone_int_enable:1;
		u8 multi_finger_scroll_int_enable:1;
	};
	u8 reg;
};

union rmi_f11_2d_ctrl12 {
	struct {
		u8 sensor_map:7;
		u8 xy_sel:1;
	};
	u8 reg;
};

union rmi_f11_2d_ctrl14 {
	struct {
		u8 sens_adjustment:5;
		u8 hyst_adjustment:3;
	};
	u8 reg;
};

struct  rmi_f11_2d_ctrl {
	union rmi_f11_2d_ctrl0		*ctrl0;
	union rmi_f11_2d_ctrl1		*ctrl1;
	union rmi_f11_2d_ctrl2__3	*ctrl2__3;
	union rmi_f11_2d_ctrl4		*ctrl4;
	union rmi_f11_2d_ctrl5		*ctrl5;
	union rmi_f11_2d_ctrl6__7	*ctrl6__7;
	union rmi_f11_2d_ctrl8__9	*ctrl8__9;
	union rmi_f11_2d_ctrl10		*ctrl10;
	union rmi_f11_2d_ctrl11		*ctrl11;
	union rmi_f11_2d_ctrl12		*ctrl12;
	u8				ctrl12_size;
	union rmi_f11_2d_ctrl14		*ctrl14;
	u8				*ctrl15;
	u8				*ctrl16;
	u8				*ctrl17;
	u8				*ctrl18;
	u8				*ctrl19;
};

struct rmi_f19_button_map {
	unsigned char nbuttons;
	unsigned char *map;
};

struct rmi_device_platform_data_spi {
	int block_delay_us;
	int split_read_block_delay_us;
	int byte_delay_us;
	int split_read_byte_delay_us;
	int pre_delay_us;
	int post_delay_us;

	void *cs_assert_data;
	int (*cs_assert) (const void *cs_assert_data, const bool assert);
};

struct rmi_device_platform_data {
	char *driver_name;

	int irq_no;
	int irq;
	enum rmi_irq_polarity irq_polarity;
	int (*gpio_config)(void);

	struct rmi_device_platform_data_spi spi_v2;

	
	struct rmi_f11_2d_ctrl *f11_ctrl;
	struct rmi_f11_2d_axis_alignment axis_align;
	struct rmi_f19_button_map *button_map;

#ifdef	CONFIG_PM
	void *pm_data;
	int (*pre_suspend) (const void *pm_data);
	int (*post_resume) (const void *pm_data);
#endif
};

struct rmi_function_descriptor {
	u16 query_base_addr;
	u16 command_base_addr;
	u16 control_base_addr;
	u16 data_base_addr;
	u8 interrupt_source_count;
	u8 function_number;
	u8 function_version;
};

struct rmi_function_container;
struct rmi_device;

struct rmi_function_handler {
	int func;
	int (*init)(struct rmi_function_container *fc);
	int (*attention)(struct rmi_function_container *fc, u8 *irq_bits);
#ifdef CONFIG_PM
	int (*suspend)(struct rmi_function_container *fc);
	int (*resume)(struct rmi_function_container *fc);
#endif
	void (*remove)(struct rmi_function_container *fc);
};

struct rmi_function_device {
	struct device dev;
};

struct rmi_function_container {
	struct list_head list;

	struct rmi_function_descriptor fd;
	struct rmi_device *rmi_dev;
	struct rmi_function_handler *fh;
	struct device dev;

	int num_of_irqs;
	int irq_pos;
	u8 *irq_mask;

	void *data;
};
#define to_rmi_function_container(d) \
		container_of(d, struct rmi_function_container, dev);
#define to_rmi_function_device(d) \
		container_of(d, struct rmi_function_device, dev);



#define RMI_CHAR_DEV_TMPBUF_SZ 128
#define RMI_REG_ADDR_PAGE_SELECT 0xFF

struct rmi_char_dev {
	
	struct mutex mutex_file_op;
	
	struct cdev main_dev;

	
	

	
	
	struct rmi_phys_device *phys;
	
	int ref_count;
};

int rmi_char_dev_register(void);
void rmi_char_dev_unregister(struct rmi_phys_device *phys);




struct rmi_driver {
	struct device_driver driver;

	int (*probe)(struct rmi_device *rmi_dev);
	int (*remove)(struct rmi_device *rmi_dev);
	void (*shutdown)(struct rmi_device *rmi_dev);
	int (*irq_handler)(struct rmi_device *rmi_dev, int irq);
	void (*fh_add)(struct rmi_device *rmi_dev,
		       struct rmi_function_handler *fh);
	void (*fh_remove)(struct rmi_device *rmi_dev,
			  struct rmi_function_handler *fh);
	u8* (*get_func_irq_mask)(struct rmi_device *rmi_dev,
			    struct rmi_function_container *fc);
	int (*store_irq_mask)(struct rmi_device *rmi_dev, u8* new_interupts);
	int (*restore_irq_mask)(struct rmi_device *rmi_dev);
	void *data;
};
#define to_rmi_driver(d) \
	container_of(d, struct rmi_driver, driver);

struct rmi_phys_info {
	char *proto;
	long tx_count;
	long tx_bytes;
	long tx_errs;
	long rx_count;
	long rx_bytes;
	long rx_errs;
	long attn_count;
	long attn;
};

struct rmi_phys_device {
	struct device *dev;
	struct rmi_device *rmi_dev;

	int (*write)(struct rmi_phys_device *phys, u16 addr, u8 data);
	int (*write_block)(struct rmi_phys_device *phys, u16 addr, u8 *buf,
			   int len);
	int (*read)(struct rmi_phys_device *phys, u16 addr, u8 *buf);
	int (*read_block)(struct rmi_phys_device *phys, u16 addr, u8 *buf,
			  int len);

	int (*enable_device) (struct rmi_phys_device *phys);
	void (*disable_device) (struct rmi_phys_device *phys);

	void *data;

	struct rmi_phys_info info;

	
	struct rmi_char_dev *char_dev;
	struct class *rmi_char_device_class;
};

struct rmi_device {
	struct device dev;

	struct rmi_driver *driver;
	struct rmi_phys_device *phys;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend_handler;
#endif
};
#define to_rmi_device(d) container_of(d, struct rmi_device, dev);
#define to_rmi_platform_data(d) ((d)->phys->dev->platform_data);

int i2c_rmi_read(uint16_t addr, uint8_t *data, uint16_t length);
int i2c_rmi_write(uint16_t addr, uint8_t *data, uint16_t length);

static inline void rmi_set_driverdata(struct rmi_device *d, void *data)
{
	dev_set_drvdata(&d->dev, data);
}

static inline void *rmi_get_driverdata(struct rmi_device *d)
{
	return dev_get_drvdata(&d->dev);
}

static inline int rmi_read(struct rmi_device *d, u16 addr, u8 *buf)
{
	return d->phys->read(d->phys, addr, buf);
}

static inline int rmi_read_block(struct rmi_device *d, u16 addr, u8 *buf,
				 int len)
{
	return d->phys->read_block(d->phys, addr, buf, len);
}

static inline int rmi_write(struct rmi_device *d, u16 addr, u8 data)
{
	return d->phys->write(d->phys, addr, data);
}

/**
 * rmi_write_block - RMI write block
 * @d: Pointer to an RMI device
 * @addr: The start address to write to
 * @buf: The write buffer
 * @len: Length of the write buffer
 *
 * Writes a block of byte data from buf using the underlaying physical protocol.
 * It returns the amount of bytes written or a negative error code.
 */
static inline int rmi_write_block(struct rmi_device *d, u16 addr, u8 *buf,
				  int len)
{
	return d->phys->write_block(d->phys, addr, buf, len);
}

int rmi_register_driver(struct rmi_driver *driver);

void rmi_unregister_driver(struct rmi_driver *driver);

int rmi_register_phys_device(struct rmi_phys_device *phys);

void rmi_unregister_phys_device(struct rmi_phys_device *phys);

int rmi_register_function_driver(struct rmi_function_handler *fh);

void rmi_unregister_function_driver(struct rmi_function_handler *fh);

struct rmi_function_handler *rmi_get_function_handler(int id);

ssize_t rmi_store_error(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count);

ssize_t rmi_show_error(struct device *dev,
		       struct device_attribute *attr,
		       char *buf);

void u8_set_bit(u8 *target, int pos);
void u8_clear_bit(u8 *target, int pos);
bool u8_is_set(u8 *target, int pos);
bool u8_is_any_set(u8 *target, int size);
void u8_or(u8 *dest, u8* target1, u8* target2, int size);
void u8_and(u8 *dest, u8* target1, u8* target2, int size);
#endif
