/*
    tuner-i2c.h - i2c interface for different tuners

    Copyright (C) 2007 Michael Krufky (mkrufky@linuxtv.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __TUNER_I2C_H__
#define __TUNER_I2C_H__

#include <linux/i2c.h>
#include <linux/slab.h>

struct tuner_i2c_props {
	u8 addr;
	struct i2c_adapter *adap;

	
	int count;
	char *name;
};

static inline int tuner_i2c_xfer_send(struct tuner_i2c_props *props, char *buf, int len)
{
	struct i2c_msg msg = { .addr = props->addr, .flags = 0,
			       .buf = buf, .len = len };
	int ret = i2c_transfer(props->adap, &msg, 1);

	return (ret == 1) ? len : ret;
}

static inline int tuner_i2c_xfer_recv(struct tuner_i2c_props *props, char *buf, int len)
{
	struct i2c_msg msg = { .addr = props->addr, .flags = I2C_M_RD,
			       .buf = buf, .len = len };
	int ret = i2c_transfer(props->adap, &msg, 1);

	return (ret == 1) ? len : ret;
}

static inline int tuner_i2c_xfer_send_recv(struct tuner_i2c_props *props,
					   char *obuf, int olen,
					   char *ibuf, int ilen)
{
	struct i2c_msg msg[2] = { { .addr = props->addr, .flags = 0,
				    .buf = obuf, .len = olen },
				  { .addr = props->addr, .flags = I2C_M_RD,
				    .buf = ibuf, .len = ilen } };
	int ret = i2c_transfer(props->adap, msg, 2);

	return (ret == 2) ? ilen : ret;
}


#define tuner_printk(kernlvl, i2cprops, fmt, arg...) do {		\
	printk(kernlvl "%s %d-%04x: " fmt, i2cprops.name,		\
			i2cprops.adap ?					\
				i2c_adapter_id(i2cprops.adap) : -1,	\
			i2cprops.addr, ##arg);				\
	 } while (0)


#define __tuner_warn(i2cprops, fmt, arg...) do {			\
	tuner_printk(KERN_WARNING, i2cprops, fmt, ##arg);		\
	} while (0)

#define __tuner_info(i2cprops, fmt, arg...) do {			\
	tuner_printk(KERN_INFO, i2cprops, fmt, ##arg);			\
	} while (0)

#define __tuner_err(i2cprops, fmt, arg...) do {				\
	tuner_printk(KERN_ERR, i2cprops, fmt, ##arg);			\
	} while (0)

#define __tuner_dbg(i2cprops, fmt, arg...) do {				\
	if ((debug))							\
		tuner_printk(KERN_DEBUG, i2cprops, fmt, ##arg);		\
	} while (0)

#define tuner_warn(fmt, arg...) __tuner_warn(priv->i2c_props, fmt, ##arg)
#define tuner_info(fmt, arg...) __tuner_info(priv->i2c_props, fmt, ##arg)
#define tuner_err(fmt, arg...) __tuner_err(priv->i2c_props, fmt, ##arg)
#define tuner_dbg(fmt, arg...) __tuner_dbg(priv->i2c_props, fmt, ##arg)



#define hybrid_tuner_request_state(type, state, list, i2cadap, i2caddr, devname)\
({									\
	int __ret = 0;							\
	list_for_each_entry(state, &list, hybrid_tuner_instance_list) {	\
		if (((i2cadap) && (state->i2c_props.adap)) &&		\
		    ((i2c_adapter_id(state->i2c_props.adap) ==		\
		      i2c_adapter_id(i2cadap)) &&			\
		     (i2caddr == state->i2c_props.addr))) {		\
			__tuner_info(state->i2c_props,			\
				     "attaching existing instance\n");	\
			state->i2c_props.count++;			\
			__ret = state->i2c_props.count;			\
			break;						\
		}							\
	}								\
	if (0 == __ret) {						\
		state = kzalloc(sizeof(type), GFP_KERNEL);		\
		if (NULL == state)					\
			goto __fail;					\
		state->i2c_props.addr = i2caddr;			\
		state->i2c_props.adap = i2cadap;			\
		state->i2c_props.name = devname;			\
		__tuner_info(state->i2c_props,				\
			     "creating new instance\n");		\
		list_add_tail(&state->hybrid_tuner_instance_list, &list);\
		state->i2c_props.count++;				\
		__ret = state->i2c_props.count;				\
	}								\
__fail:									\
	__ret;								\
})

#define hybrid_tuner_release_state(state)				\
({									\
	int __ret;							\
	state->i2c_props.count--;					\
	__ret = state->i2c_props.count;					\
	if (!state->i2c_props.count) {					\
		__tuner_info(state->i2c_props, "destroying instance\n");\
		list_del(&state->hybrid_tuner_instance_list);		\
		kfree(state);						\
	}								\
	__ret;								\
})

#define hybrid_tuner_report_instance_count(state)			\
({									\
	int __ret = 0;							\
	if (state)							\
		__ret = state->i2c_props.count;				\
	__ret;								\
})

#endif 
