#ifndef _LINUX_TTY_H
#define _LINUX_TTY_H


#define NR_LDISCS		30

#define N_TTY		0
#define N_SLIP		1
#define N_MOUSE		2
#define N_PPP		3
#define N_STRIP		4
#define N_AX25		5
#define N_X25		6	
#define N_6PACK		7
#define N_MASC		8	
#define N_R3964		9	
#define N_PROFIBUS_FDL	10	
#define N_IRDA		11	
#define N_SMSBLOCK	12	
				
#define N_HDLC		13	
#define N_SYNC_PPP	14	
#define N_HCI		15	
#define N_GIGASET_M101	16	
#define N_SLCAN		17	
#define N_PPS		18	
#define N_V253		19	
#define N_CAIF		20      
#define N_GSM0710	21	
#define N_TI_WL		22	
#define N_TRACESINK	23	
#define N_TRACEROUTER	24	
#define N_SMUX		25	

#ifdef __KERNEL__
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/termios.h>
#include <linux/workqueue.h>
#include <linux/tty_driver.h>
#include <linux/tty_ldisc.h>
#include <linux/mutex.h>



#define NR_UNIX98_PTY_DEFAULT	4096      
#define NR_UNIX98_PTY_RESERVE	1024	  
#define NR_UNIX98_PTY_MAX	(1 << MINORBITS) 

#define __DISABLED_CHAR '\0'

#if defined(CONFIG_MSM_SMD0_WQ)
extern struct workqueue_struct *tty_wq;
#endif

struct tty_buffer {
	struct tty_buffer *next;
	char *char_buf_ptr;
	unsigned char *flag_buf_ptr;
	int used;
	int size;
	int commit;
	int read;
	
	unsigned long data[0];
};


#define TTY_BUFFER_PAGE	(((PAGE_SIZE - sizeof(struct tty_buffer)) / 2) & ~0xFF)


struct tty_bufhead {
	struct work_struct work;
	spinlock_t lock;
	struct tty_buffer *head;	
	struct tty_buffer *tail;	
	struct tty_buffer *free;	
	int memory_used;		
};
#define TTY_NORMAL	0
#define TTY_BREAK	1
#define TTY_FRAME	2
#define TTY_PARITY	3
#define TTY_OVERRUN	4

#define INTR_CHAR(tty) ((tty)->termios->c_cc[VINTR])
#define QUIT_CHAR(tty) ((tty)->termios->c_cc[VQUIT])
#define ERASE_CHAR(tty) ((tty)->termios->c_cc[VERASE])
#define KILL_CHAR(tty) ((tty)->termios->c_cc[VKILL])
#define EOF_CHAR(tty) ((tty)->termios->c_cc[VEOF])
#define TIME_CHAR(tty) ((tty)->termios->c_cc[VTIME])
#define MIN_CHAR(tty) ((tty)->termios->c_cc[VMIN])
#define SWTC_CHAR(tty) ((tty)->termios->c_cc[VSWTC])
#define START_CHAR(tty) ((tty)->termios->c_cc[VSTART])
#define STOP_CHAR(tty) ((tty)->termios->c_cc[VSTOP])
#define SUSP_CHAR(tty) ((tty)->termios->c_cc[VSUSP])
#define EOL_CHAR(tty) ((tty)->termios->c_cc[VEOL])
#define REPRINT_CHAR(tty) ((tty)->termios->c_cc[VREPRINT])
#define DISCARD_CHAR(tty) ((tty)->termios->c_cc[VDISCARD])
#define WERASE_CHAR(tty) ((tty)->termios->c_cc[VWERASE])
#define LNEXT_CHAR(tty)	((tty)->termios->c_cc[VLNEXT])
#define EOL2_CHAR(tty) ((tty)->termios->c_cc[VEOL2])

#define _I_FLAG(tty, f)	((tty)->termios->c_iflag & (f))
#define _O_FLAG(tty, f)	((tty)->termios->c_oflag & (f))
#define _C_FLAG(tty, f)	((tty)->termios->c_cflag & (f))
#define _L_FLAG(tty, f)	((tty)->termios->c_lflag & (f))

#define I_IGNBRK(tty)	_I_FLAG((tty), IGNBRK)
#define I_BRKINT(tty)	_I_FLAG((tty), BRKINT)
#define I_IGNPAR(tty)	_I_FLAG((tty), IGNPAR)
#define I_PARMRK(tty)	_I_FLAG((tty), PARMRK)
#define I_INPCK(tty)	_I_FLAG((tty), INPCK)
#define I_ISTRIP(tty)	_I_FLAG((tty), ISTRIP)
#define I_INLCR(tty)	_I_FLAG((tty), INLCR)
#define I_IGNCR(tty)	_I_FLAG((tty), IGNCR)
#define I_ICRNL(tty)	_I_FLAG((tty), ICRNL)
#define I_IUCLC(tty)	_I_FLAG((tty), IUCLC)
#define I_IXON(tty)	_I_FLAG((tty), IXON)
#define I_IXANY(tty)	_I_FLAG((tty), IXANY)
#define I_IXOFF(tty)	_I_FLAG((tty), IXOFF)
#define I_IMAXBEL(tty)	_I_FLAG((tty), IMAXBEL)
#define I_IUTF8(tty)	_I_FLAG((tty), IUTF8)

#define O_OPOST(tty)	_O_FLAG((tty), OPOST)
#define O_OLCUC(tty)	_O_FLAG((tty), OLCUC)
#define O_ONLCR(tty)	_O_FLAG((tty), ONLCR)
#define O_OCRNL(tty)	_O_FLAG((tty), OCRNL)
#define O_ONOCR(tty)	_O_FLAG((tty), ONOCR)
#define O_ONLRET(tty)	_O_FLAG((tty), ONLRET)
#define O_OFILL(tty)	_O_FLAG((tty), OFILL)
#define O_OFDEL(tty)	_O_FLAG((tty), OFDEL)
#define O_NLDLY(tty)	_O_FLAG((tty), NLDLY)
#define O_CRDLY(tty)	_O_FLAG((tty), CRDLY)
#define O_TABDLY(tty)	_O_FLAG((tty), TABDLY)
#define O_BSDLY(tty)	_O_FLAG((tty), BSDLY)
#define O_VTDLY(tty)	_O_FLAG((tty), VTDLY)
#define O_FFDLY(tty)	_O_FLAG((tty), FFDLY)

#define C_BAUD(tty)	_C_FLAG((tty), CBAUD)
#define C_CSIZE(tty)	_C_FLAG((tty), CSIZE)
#define C_CSTOPB(tty)	_C_FLAG((tty), CSTOPB)
#define C_CREAD(tty)	_C_FLAG((tty), CREAD)
#define C_PARENB(tty)	_C_FLAG((tty), PARENB)
#define C_PARODD(tty)	_C_FLAG((tty), PARODD)
#define C_HUPCL(tty)	_C_FLAG((tty), HUPCL)
#define C_CLOCAL(tty)	_C_FLAG((tty), CLOCAL)
#define C_CIBAUD(tty)	_C_FLAG((tty), CIBAUD)
#define C_CRTSCTS(tty)	_C_FLAG((tty), CRTSCTS)

#define L_ISIG(tty)	_L_FLAG((tty), ISIG)
#define L_ICANON(tty)	_L_FLAG((tty), ICANON)
#define L_XCASE(tty)	_L_FLAG((tty), XCASE)
#define L_ECHO(tty)	_L_FLAG((tty), ECHO)
#define L_ECHOE(tty)	_L_FLAG((tty), ECHOE)
#define L_ECHOK(tty)	_L_FLAG((tty), ECHOK)
#define L_ECHONL(tty)	_L_FLAG((tty), ECHONL)
#define L_NOFLSH(tty)	_L_FLAG((tty), NOFLSH)
#define L_TOSTOP(tty)	_L_FLAG((tty), TOSTOP)
#define L_ECHOCTL(tty)	_L_FLAG((tty), ECHOCTL)
#define L_ECHOPRT(tty)	_L_FLAG((tty), ECHOPRT)
#define L_ECHOKE(tty)	_L_FLAG((tty), ECHOKE)
#define L_FLUSHO(tty)	_L_FLAG((tty), FLUSHO)
#define L_PENDIN(tty)	_L_FLAG((tty), PENDIN)
#define L_IEXTEN(tty)	_L_FLAG((tty), IEXTEN)
#define L_EXTPROC(tty)	_L_FLAG((tty), EXTPROC)

struct device;
struct signal_struct;


struct tty_port;

struct tty_port_operations {
	
	int (*carrier_raised)(struct tty_port *port);
	
	void (*dtr_rts)(struct tty_port *port, int raise);
	void (*shutdown)(struct tty_port *port);
	void (*drop)(struct tty_port *port);
	int (*activate)(struct tty_port *port, struct tty_struct *tty);
	
	void (*destruct)(struct tty_port *port);
};
	
struct tty_port {
	struct tty_struct	*tty;		
	const struct tty_port_operations *ops;	
	spinlock_t		lock;		
	int			blocked_open;	
	int			count;		
	wait_queue_head_t	open_wait;	
	wait_queue_head_t	close_wait;	
	wait_queue_head_t	delta_msr_wait;	
	unsigned long		flags;		
	unsigned char		console:1;	
	struct mutex		mutex;		
	struct mutex		buf_mutex;	
	unsigned char		*xmit_buf;	
	unsigned int		close_delay;	
	unsigned int		closing_wait;	
	int			drain_delay;	
	struct kref		kref;		
};


struct tty_operations;

struct tty_struct {
	int	magic;
	struct kref kref;
	struct device *dev;
	struct tty_driver *driver;
	const struct tty_operations *ops;
	int index;

	
	struct mutex ldisc_mutex;
	struct tty_ldisc *ldisc;

	struct mutex termios_mutex;
	spinlock_t ctrl_lock;
	
	struct ktermios *termios, *termios_locked;
	struct termiox *termiox;	
	char name[64];
	struct pid *pgrp;		
	struct pid *session;
	unsigned long flags;
	int count;
	struct winsize winsize;		
	unsigned char stopped:1, hw_stopped:1, flow_stopped:1, packet:1;
	unsigned char low_latency:1, warned:1;
	unsigned char update_room_in_ldisc:1;
	unsigned char ctrl_status;	
	unsigned int receive_room;	
	unsigned int rr_bug;

	struct tty_struct *link;
	struct fasync_struct *fasync;
	struct tty_bufhead buf;		
	int alt_speed;		
	wait_queue_head_t write_wait;
	wait_queue_head_t read_wait;
	struct work_struct hangup_work;
	void *disc_data;
	void *driver_data;
	struct list_head tty_files;

#define N_TTY_BUF_SIZE 4096

	unsigned int column;
	unsigned char lnext:1, erasing:1, raw:1, real_raw:1, icanon:1;
	unsigned char closing:1;
	unsigned char echo_overrun:1;
	unsigned short minimum_to_wake;
	unsigned long overrun_time;
	int num_overrun;
	unsigned long process_char_map[256/(8*sizeof(unsigned long))];
	char *read_buf;
	int read_head;
	int read_tail;
	int read_cnt;
	unsigned long read_flags[N_TTY_BUF_SIZE/(8*sizeof(unsigned long))];
	unsigned char *echo_buf;
	unsigned int echo_pos;
	unsigned int echo_cnt;
	int canon_data;
	unsigned long canon_head;
	unsigned int canon_column;
	struct mutex atomic_read_lock;
	struct mutex atomic_write_lock;
	struct mutex output_lock;
	struct mutex echo_lock;
	unsigned char *write_buf;
	int write_cnt;
	spinlock_t read_lock;
	
	struct work_struct SAK_work;
	struct tty_port *port;
};

struct tty_file_private {
	struct tty_struct *tty;
	struct file *file;
	struct list_head list;
};

#define TTY_MAGIC		0x5401

#define TTY_THROTTLED 		0	
#define TTY_IO_ERROR 		1	
#define TTY_OTHER_CLOSED 	2	
#define TTY_EXCLUSIVE 		3	
#define TTY_DEBUG 		4	
#define TTY_DO_WRITE_WAKEUP 	5	
#define TTY_PUSH 		6	
#define TTY_CLOSING 		7	
#define TTY_LDISC 		9	
#define TTY_LDISC_CHANGING 	10	
#define TTY_LDISC_OPEN	 	11	
#define TTY_HW_COOK_OUT 	14	
#define TTY_HW_COOK_IN 		15	
#define TTY_PTY_LOCK 		16	
#define TTY_NO_WRITE_SPLIT 	17	
#define TTY_HUPPED 		18	
#define TTY_FLUSHING		19	
#define TTY_FLUSHPENDING	20	
#define TTY_HUPPING 		21	

#define TTY_WRITE_FLUSH(tty) tty_write_flush((tty))

extern void tty_write_flush(struct tty_struct *);

extern struct ktermios tty_std_termios;

extern void console_init(void);
extern int vcs_init(void);

extern struct class *tty_class;


static inline struct tty_struct *tty_kref_get(struct tty_struct *tty)
{
	if (tty)
		kref_get(&tty->kref);
	return tty;
}
extern void tty_kref_put(struct tty_struct *tty);

extern int tty_paranoia_check(struct tty_struct *tty, struct inode *inode,
			      const char *routine);
extern char *tty_name(struct tty_struct *tty, char *buf);
extern void tty_wait_until_sent(struct tty_struct *tty, long timeout);
extern int tty_check_change(struct tty_struct *tty);
extern void stop_tty(struct tty_struct *tty);
extern void start_tty(struct tty_struct *tty);
extern int tty_register_driver(struct tty_driver *driver);
extern int tty_unregister_driver(struct tty_driver *driver);
extern struct device *tty_register_device(struct tty_driver *driver,
					  unsigned index, struct device *dev);
extern void tty_unregister_device(struct tty_driver *driver, unsigned index);
extern int tty_read_raw_data(struct tty_struct *tty, unsigned char *bufp,
			     int buflen);
extern void tty_write_message(struct tty_struct *tty, char *msg);
extern int tty_put_char(struct tty_struct *tty, unsigned char c);
extern int tty_chars_in_buffer(struct tty_struct *tty);
extern int tty_write_room(struct tty_struct *tty);
extern void tty_driver_flush_buffer(struct tty_struct *tty);
extern void tty_throttle(struct tty_struct *tty);
extern void tty_unthrottle(struct tty_struct *tty);
extern int tty_do_resize(struct tty_struct *tty, struct winsize *ws);
extern void tty_driver_remove_tty(struct tty_driver *driver,
				  struct tty_struct *tty);
extern void tty_shutdown(struct tty_struct *tty);
extern void tty_free_termios(struct tty_struct *tty);
extern int is_current_pgrp_orphaned(void);
extern struct pid *tty_get_pgrp(struct tty_struct *tty);
extern int is_ignored(int sig);
extern int tty_signal(int sig, struct tty_struct *tty);
extern void tty_hangup(struct tty_struct *tty);
extern void tty_vhangup(struct tty_struct *tty);
extern void tty_vhangup_locked(struct tty_struct *tty);
extern void tty_vhangup_self(void);
extern void tty_unhangup(struct file *filp);
extern int tty_hung_up_p(struct file *filp);
extern void do_SAK(struct tty_struct *tty);
extern void __do_SAK(struct tty_struct *tty);
extern void disassociate_ctty(int priv);
extern void no_tty(void);
extern void tty_flip_buffer_push(struct tty_struct *tty);
extern void tty_flush_to_ldisc(struct tty_struct *tty);
extern void tty_buffer_free_all(struct tty_struct *tty);
extern void tty_buffer_flush(struct tty_struct *tty);
extern void tty_buffer_init(struct tty_struct *tty);
extern speed_t tty_get_baud_rate(struct tty_struct *tty);
extern speed_t tty_termios_baud_rate(struct ktermios *termios);
extern speed_t tty_termios_input_baud_rate(struct ktermios *termios);
extern void tty_termios_encode_baud_rate(struct ktermios *termios,
						speed_t ibaud, speed_t obaud);
extern void tty_encode_baud_rate(struct tty_struct *tty,
						speed_t ibaud, speed_t obaud);
extern void tty_termios_copy_hw(struct ktermios *new, struct ktermios *old);
extern int tty_termios_hw_change(struct ktermios *a, struct ktermios *b);
extern int tty_set_termios(struct tty_struct *tty, struct ktermios *kt);

extern struct tty_ldisc *tty_ldisc_ref(struct tty_struct *);
extern void tty_ldisc_deref(struct tty_ldisc *);
extern struct tty_ldisc *tty_ldisc_ref_wait(struct tty_struct *);
extern void tty_ldisc_hangup(struct tty_struct *tty);
extern const struct file_operations tty_ldiscs_proc_fops;

extern void tty_wakeup(struct tty_struct *tty);
extern void tty_ldisc_flush(struct tty_struct *tty);

extern long tty_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
extern int tty_mode_ioctl(struct tty_struct *tty, struct file *file,
			unsigned int cmd, unsigned long arg);
extern int tty_perform_flush(struct tty_struct *tty, unsigned long arg);
extern dev_t tty_devnum(struct tty_struct *tty);
extern void proc_clear_tty(struct task_struct *p);
extern struct tty_struct *get_current_tty(void);
extern void tty_default_fops(struct file_operations *fops);
extern struct tty_struct *alloc_tty_struct(void);
extern int tty_alloc_file(struct file *file);
extern void tty_add_file(struct tty_struct *tty, struct file *file);
extern void tty_free_file(struct file *file);
extern void free_tty_struct(struct tty_struct *tty);
extern void initialize_tty_struct(struct tty_struct *tty,
		struct tty_driver *driver, int idx);
extern void deinitialize_tty_struct(struct tty_struct *tty);
extern struct tty_struct *tty_init_dev(struct tty_driver *driver, int idx);
extern int tty_release(struct inode *inode, struct file *filp);
extern int tty_init_termios(struct tty_struct *tty);
extern int tty_standard_install(struct tty_driver *driver,
		struct tty_struct *tty);

extern struct tty_struct *tty_pair_get_tty(struct tty_struct *tty);
extern struct tty_struct *tty_pair_get_pty(struct tty_struct *tty);

extern struct mutex tty_mutex;
extern spinlock_t tty_files_lock;

extern void tty_write_unlock(struct tty_struct *tty);
extern int tty_write_lock(struct tty_struct *tty, int ndelay);
#define tty_is_writelocked(tty)  (mutex_is_locked(&tty->atomic_write_lock))

extern void tty_port_init(struct tty_port *port);
extern int tty_port_alloc_xmit_buf(struct tty_port *port);
extern void tty_port_free_xmit_buf(struct tty_port *port);
extern void tty_port_put(struct tty_port *port);

static inline struct tty_port *tty_port_get(struct tty_port *port)
{
	if (port)
		kref_get(&port->kref);
	return port;
}

extern struct tty_struct *tty_port_tty_get(struct tty_port *port);
extern void tty_port_tty_set(struct tty_port *port, struct tty_struct *tty);
extern int tty_port_carrier_raised(struct tty_port *port);
extern void tty_port_raise_dtr_rts(struct tty_port *port);
extern void tty_port_lower_dtr_rts(struct tty_port *port);
extern void tty_port_hangup(struct tty_port *port);
extern int tty_port_block_til_ready(struct tty_port *port,
				struct tty_struct *tty, struct file *filp);
extern int tty_port_close_start(struct tty_port *port,
				struct tty_struct *tty, struct file *filp);
extern void tty_port_close_end(struct tty_port *port, struct tty_struct *tty);
extern void tty_port_close(struct tty_port *port,
				struct tty_struct *tty, struct file *filp);
extern int tty_port_open(struct tty_port *port,
				struct tty_struct *tty, struct file *filp);
static inline int tty_port_users(struct tty_port *port)
{
	return port->count + port->blocked_open;
}

extern int tty_register_ldisc(int disc, struct tty_ldisc_ops *new_ldisc);
extern int tty_unregister_ldisc(int disc);
extern int tty_set_ldisc(struct tty_struct *tty, int ldisc);
extern int tty_ldisc_setup(struct tty_struct *tty, struct tty_struct *o_tty);
extern void tty_ldisc_release(struct tty_struct *tty, struct tty_struct *o_tty);
extern void tty_ldisc_init(struct tty_struct *tty);
extern void tty_ldisc_deinit(struct tty_struct *tty);
extern void tty_ldisc_begin(void);
extern void tty_ldisc_enable(struct tty_struct *tty);


extern struct tty_ldisc_ops tty_ldisc_N_TTY;
extern void n_tty_inherit_ops(struct tty_ldisc_ops *ops);

#ifdef CONFIG_AUDIT
extern void tty_audit_add_data(struct tty_struct *tty, unsigned char *data,
			       size_t size);
extern void tty_audit_exit(void);
extern void tty_audit_fork(struct signal_struct *sig);
extern void tty_audit_tiocsti(struct tty_struct *tty, char ch);
extern void tty_audit_push(struct tty_struct *tty);
extern int tty_audit_push_task(struct task_struct *tsk,
			       uid_t loginuid, u32 sessionid);
#else
static inline void tty_audit_add_data(struct tty_struct *tty,
				      unsigned char *data, size_t size)
{
}
static inline void tty_audit_tiocsti(struct tty_struct *tty, char ch)
{
}
static inline void tty_audit_exit(void)
{
}
static inline void tty_audit_fork(struct signal_struct *sig)
{
}
static inline void tty_audit_push(struct tty_struct *tty)
{
}
static inline int tty_audit_push_task(struct task_struct *tsk,
				      uid_t loginuid, u32 sessionid)
{
	return 0;
}
#endif

extern int __init tty_init(void);

extern int n_tty_ioctl_helper(struct tty_struct *tty, struct file *file,
		       unsigned int cmd, unsigned long arg);
extern long n_tty_compat_ioctl_helper(struct tty_struct *tty, struct file *file,
		       unsigned int cmd, unsigned long arg);


extern void serial_console_init(void);


extern int pcxe_open(struct tty_struct *tty, struct file *filp);


extern int vt_ioctl(struct tty_struct *tty,
		    unsigned int cmd, unsigned long arg);

extern long vt_compat_ioctl(struct tty_struct *tty,
		     unsigned int cmd, unsigned long arg);

extern void __lockfunc tty_lock(void) __acquires(tty_lock);
extern void __lockfunc tty_unlock(void) __releases(tty_lock);

static inline void tty_wait_until_sent_from_close(struct tty_struct *tty,
		long timeout)
{
	tty_unlock(); 
	tty_wait_until_sent(tty, timeout);
	tty_lock();
}

#define wait_event_interruptible_tty(wq, condition)			\
({									\
	int __ret = 0;							\
	if (!(condition)) {						\
		__wait_event_interruptible_tty(wq, condition, __ret);	\
	}								\
	__ret;								\
})

#define __wait_event_interruptible_tty(wq, condition, ret)		\
do {									\
	DEFINE_WAIT(__wait);						\
									\
	for (;;) {							\
		prepare_to_wait(&wq, &__wait, TASK_INTERRUPTIBLE);	\
		if (condition)						\
			break;						\
		if (!signal_pending(current)) {				\
			tty_unlock();					\
			schedule();					\
			tty_lock();					\
			continue;					\
		}							\
		ret = -ERESTARTSYS;					\
		break;							\
	}								\
	finish_wait(&wq, &__wait);					\
} while (0)


#endif 
#endif
