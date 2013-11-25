#ifndef _ARCH_ARM_GPIO_H
#define _ARCH_ARM_GPIO_H

#if CONFIG_ARCH_NR_GPIO > 0
#define ARCH_NR_GPIOS CONFIG_ARCH_NR_GPIO
#endif

#include <mach/gpio.h>

#ifndef __ARM_GPIOLIB_COMPLEX
#include <asm-generic/gpio.h>

#define gpio_get_value  __gpio_get_value
#define gpio_set_value  __gpio_set_value
#define gpio_cansleep   __gpio_cansleep
#endif

#ifndef gpio_to_irq
#define gpio_to_irq	__gpio_to_irq
#endif

#endif 
