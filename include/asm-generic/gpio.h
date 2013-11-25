#ifndef _ASM_GENERIC_GPIO_H
#define _ASM_GENERIC_GPIO_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/of.h>

#ifdef CONFIG_GPIOLIB

#include <linux/compiler.h>


#ifndef ARCH_NR_GPIOS
#define ARCH_NR_GPIOS		256
#endif


static inline bool gpio_is_valid(int number)
{
	return number >= 0 && number < ARCH_NR_GPIOS;
}

struct device;
struct gpio;
struct seq_file;
struct module;
struct device_node;

struct gpio_chip {
	const char		*label;
	struct device		*dev;
	struct module		*owner;

	int			(*request)(struct gpio_chip *chip,
						unsigned offset);
	void			(*free)(struct gpio_chip *chip,
						unsigned offset);

	int			(*direction_input)(struct gpio_chip *chip,
						unsigned offset);
	int			(*get)(struct gpio_chip *chip,
						unsigned offset);
	int			(*direction_output)(struct gpio_chip *chip,
						unsigned offset, int value);
	int			(*set_debounce)(struct gpio_chip *chip,
						unsigned offset, unsigned debounce);

	void			(*set)(struct gpio_chip *chip,
						unsigned offset, int value);

	int			(*to_irq)(struct gpio_chip *chip,
						unsigned offset);

	void			(*dbg_show)(struct seq_file *s,
						struct gpio_chip *chip);
	int			base;
	u16			ngpio;
	const char		*const *names;
	unsigned		can_sleep:1;
	unsigned		exported:1;

#if defined(CONFIG_OF_GPIO)
	struct device_node *of_node;
	int of_gpio_n_cells;
	int (*of_xlate)(struct gpio_chip *gc,
		        const struct of_phandle_args *gpiospec, u32 *flags);
#endif
};

extern const char *gpiochip_is_requested(struct gpio_chip *chip,
			unsigned offset);
extern struct gpio_chip *gpio_to_chip(unsigned gpio);
extern int __must_check gpiochip_reserve(int start, int ngpio);

extern int gpiochip_add(struct gpio_chip *chip);
extern int __must_check gpiochip_remove(struct gpio_chip *chip);
extern struct gpio_chip *gpiochip_find(const void *data,
					int (*match)(struct gpio_chip *chip,
						     const void *data));


extern int gpio_request(unsigned gpio, const char *label);
extern void gpio_free(unsigned gpio);

extern int gpio_direction_input(unsigned gpio);
extern int gpio_direction_output(unsigned gpio, int value);

extern int gpio_set_debounce(unsigned gpio, unsigned debounce);

extern int gpio_get_value_cansleep(unsigned gpio);
extern void gpio_set_value_cansleep(unsigned gpio, int value);


extern int __gpio_get_value(unsigned gpio);
extern void __gpio_set_value(unsigned gpio, int value);

extern int __gpio_cansleep(unsigned gpio);

extern int __gpio_to_irq(unsigned gpio);

extern int gpio_request_one(unsigned gpio, unsigned long flags, const char *label);
extern int gpio_request_array(const struct gpio *array, size_t num);
extern void gpio_free_array(const struct gpio *array, size_t num);

int devm_gpio_request(struct device *dev, unsigned gpio, const char *label);
void devm_gpio_free(struct device *dev, unsigned int gpio);

#ifdef CONFIG_GPIO_SYSFS

extern int gpio_export(unsigned gpio, bool direction_may_change);
extern int gpio_export_link(struct device *dev, const char *name,
			unsigned gpio);
extern int gpio_sysfs_set_active_low(unsigned gpio, int value);
extern void gpio_unexport(unsigned gpio);

#endif	

#else	

static inline bool gpio_is_valid(int number)
{
	
	return number >= 0;
}


static inline int gpio_cansleep(unsigned gpio)
{
	return 0;
}

static inline int gpio_get_value_cansleep(unsigned gpio)
{
	might_sleep();
	return __gpio_get_value(gpio);
}

static inline void gpio_set_value_cansleep(unsigned gpio, int value)
{
	might_sleep();
	__gpio_set_value(gpio, value);
}

#endif 

#ifndef CONFIG_GPIO_SYSFS

struct device;


static inline int gpio_export(unsigned gpio, bool direction_may_change)
{
	return -ENOSYS;
}

static inline int gpio_export_link(struct device *dev, const char *name,
				unsigned gpio)
{
	return -ENOSYS;
}

static inline int gpio_sysfs_set_active_low(unsigned gpio, int value)
{
	return -ENOSYS;
}

static inline void gpio_unexport(unsigned gpio)
{
}
#endif	

#endif 
