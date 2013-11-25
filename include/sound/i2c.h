#ifndef __SOUND_I2C_H
#define __SOUND_I2C_H

/*
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *
 */

#define SND_I2C_DEVICE_ADDRTEN	(1<<0)	

struct snd_i2c_device {
	struct list_head list;
	struct snd_i2c_bus *bus;	
	char name[32];		
	unsigned short flags;	
	unsigned short addr;	
	unsigned long private_value;
	void *private_data;
	void (*private_free)(struct snd_i2c_device *device);
};

#define snd_i2c_device(n) list_entry(n, struct snd_i2c_device, list)

struct snd_i2c_bit_ops {
	void (*start)(struct snd_i2c_bus *bus);	
	void (*stop)(struct snd_i2c_bus *bus);	
	void (*direction)(struct snd_i2c_bus *bus, int clock, int data);  
	void (*setlines)(struct snd_i2c_bus *bus, int clock, int data);
	int (*getclock)(struct snd_i2c_bus *bus);
	int (*getdata)(struct snd_i2c_bus *bus, int ack);
};

struct snd_i2c_ops {
	int (*sendbytes)(struct snd_i2c_device *device, unsigned char *bytes, int count);
	int (*readbytes)(struct snd_i2c_device *device, unsigned char *bytes, int count);
	int (*probeaddr)(struct snd_i2c_bus *bus, unsigned short addr);
};

struct snd_i2c_bus {
	struct snd_card *card;	
	char name[32];		

	struct mutex lock_mutex;

	struct snd_i2c_bus *master;	
	struct list_head buses;	

	struct list_head devices; 

	union {
		struct snd_i2c_bit_ops *bit;
		void *ops;
	} hw_ops;		
	struct snd_i2c_ops *ops;	

	unsigned long private_value;
	void *private_data;
	void (*private_free)(struct snd_i2c_bus *bus);
};

#define snd_i2c_slave_bus(n) list_entry(n, struct snd_i2c_bus, buses)

int snd_i2c_bus_create(struct snd_card *card, const char *name,
		       struct snd_i2c_bus *master, struct snd_i2c_bus **ri2c);
int snd_i2c_device_create(struct snd_i2c_bus *bus, const char *name,
			  unsigned char addr, struct snd_i2c_device **rdevice);
int snd_i2c_device_free(struct snd_i2c_device *device);

static inline void snd_i2c_lock(struct snd_i2c_bus *bus)
{
	if (bus->master)
		mutex_lock(&bus->master->lock_mutex);
	else
		mutex_lock(&bus->lock_mutex);
}

static inline void snd_i2c_unlock(struct snd_i2c_bus *bus)
{
	if (bus->master)
		mutex_unlock(&bus->master->lock_mutex);
	else
		mutex_unlock(&bus->lock_mutex);
}

int snd_i2c_sendbytes(struct snd_i2c_device *device, unsigned char *bytes, int count);
int snd_i2c_readbytes(struct snd_i2c_device *device, unsigned char *bytes, int count);
int snd_i2c_probeaddr(struct snd_i2c_bus *bus, unsigned short addr);

#endif 
