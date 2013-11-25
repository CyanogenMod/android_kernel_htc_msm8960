#ifndef _KBD_KERN_H
#define _KBD_KERN_H

#include <linux/tty.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>

extern struct tasklet_struct keyboard_tasklet;

extern char *func_table[MAX_NR_FUNC];
extern char func_buf[];
extern char *funcbufptr;
extern int funcbufsize, funcbufleft;

struct kbd_struct {

	unsigned char lockstate;
#define VC_SHIFTLOCK	KG_SHIFT	
#define VC_ALTGRLOCK	KG_ALTGR	
#define VC_CTRLLOCK	KG_CTRL 	
#define VC_ALTLOCK	KG_ALT  	
#define VC_SHIFTLLOCK	KG_SHIFTL	
#define VC_SHIFTRLOCK	KG_SHIFTR	
#define VC_CTRLLLOCK	KG_CTRLL 	
#define VC_CTRLRLOCK	KG_CTRLR 	
	unsigned char slockstate; 	

	unsigned char ledmode:2; 	
#define LED_SHOW_FLAGS 0        
#define LED_SHOW_IOCTL 1        
#define LED_SHOW_MEM 2          

	unsigned char ledflagstate:4;	
	unsigned char default_ledflagstate:4;
#define VC_SCROLLOCK	0	
#define VC_NUMLOCK	1	
#define VC_CAPSLOCK	2	
#define VC_KANALOCK	3	

	unsigned char kbdmode:3;	
#define VC_XLATE	0	
#define VC_MEDIUMRAW	1	
#define VC_RAW		2	
#define VC_UNICODE	3	
#define VC_OFF		4	

	unsigned char modeflags:5;
#define VC_APPLIC	0	
#define VC_CKMODE	1	
#define VC_REPEAT	2	
#define VC_CRLF		3	
#define VC_META		4	
};

extern int kbd_init(void);

extern unsigned char getledstate(void);
extern void setledstate(struct kbd_struct *kbd, unsigned int led);

extern int do_poke_blanked_console;

extern void (*kbd_ledfunc)(unsigned int led);

extern int set_console(int nr);
extern void schedule_console_callback(void);

static inline void set_leds(void)
{
	tasklet_schedule(&keyboard_tasklet);
}

static inline int vc_kbd_mode(struct kbd_struct * kbd, int flag)
{
	return ((kbd->modeflags >> flag) & 1);
}

static inline int vc_kbd_led(struct kbd_struct * kbd, int flag)
{
	return ((kbd->ledflagstate >> flag) & 1);
}

static inline void set_vc_kbd_mode(struct kbd_struct * kbd, int flag)
{
	kbd->modeflags |= 1 << flag;
}

static inline void set_vc_kbd_led(struct kbd_struct * kbd, int flag)
{
	kbd->ledflagstate |= 1 << flag;
}

static inline void clr_vc_kbd_mode(struct kbd_struct * kbd, int flag)
{
	kbd->modeflags &= ~(1 << flag);
}

static inline void clr_vc_kbd_led(struct kbd_struct * kbd, int flag)
{
	kbd->ledflagstate &= ~(1 << flag);
}

static inline void chg_vc_kbd_lock(struct kbd_struct * kbd, int flag)
{
	kbd->lockstate ^= 1 << flag;
}

static inline void chg_vc_kbd_slock(struct kbd_struct * kbd, int flag)
{
	kbd->slockstate ^= 1 << flag;
}

static inline void chg_vc_kbd_mode(struct kbd_struct * kbd, int flag)
{
	kbd->modeflags ^= 1 << flag;
}

static inline void chg_vc_kbd_led(struct kbd_struct * kbd, int flag)
{
	kbd->ledflagstate ^= 1 << flag;
}

#define U(x) ((x) ^ 0xf000)

#define BRL_UC_ROW 0x2800


struct console;

void compute_shiftstate(void);


extern unsigned int keymap_count;


static inline void con_schedule_flip(struct tty_struct *t)
{
	unsigned long flags;
	spin_lock_irqsave(&t->buf.lock, flags);
	if (t->buf.tail != NULL)
		t->buf.tail->commit = t->buf.tail->used;
	spin_unlock_irqrestore(&t->buf.lock, flags);
	schedule_work(&t->buf.work);
}

#endif
