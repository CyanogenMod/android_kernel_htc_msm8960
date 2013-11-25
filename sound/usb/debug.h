#ifndef __USBAUDIO_DEBUG_H
#define __USBAUDIO_DEBUG_H


#ifdef HW_CONST_DEBUG
#define hwc_debug(fmt, args...) printk(KERN_DEBUG fmt, ##args)
#else
#define hwc_debug(fmt, args...) do { } while(0)
#endif

#endif 

