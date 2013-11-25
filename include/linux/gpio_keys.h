#ifndef _GPIO_KEYS_H
#define _GPIO_KEYS_H

struct device;

struct gpio_keys_button {
	
	unsigned int code;	
	int gpio;		
	int active_low;
	const char *desc;
	unsigned int type;	
	int wakeup;		
	int debounce_interval;	
	bool can_disable;
	int value;		
	unsigned int irq;	
};

struct gpio_keys_platform_data {
	struct gpio_keys_button *buttons;
	int nbuttons;
	unsigned int poll_interval;	
	unsigned int rep:1;		
	int (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	const char *name;		
};

#endif
