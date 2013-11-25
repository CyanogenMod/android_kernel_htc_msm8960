/*
 * This provides the callbacks and functions that KGDB needs to share between
 * the core, I/O and arch-specific portions.
 *
 * Author: Amit Kale <amitkale@linsyssoft.com> and
 *         Tom Rini <trini@kernel.crashing.org>
 *
 * 2001-2004 (c) Amit S. Kale and 2003-2005 (c) MontaVista Software, Inc.
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#ifndef _KGDB_H_
#define _KGDB_H_

#include <linux/serial_8250.h>
#include <linux/linkage.h>
#include <linux/init.h>
#include <linux/atomic.h>
#ifdef CONFIG_HAVE_ARCH_KGDB
#include <asm/kgdb.h>
#endif

#ifdef CONFIG_KGDB
struct pt_regs;

extern int kgdb_skipexception(int exception, struct pt_regs *regs);

struct tasklet_struct;
struct task_struct;
struct uart_port;

void kgdb_breakpoint(void);

extern int kgdb_connected;
extern int kgdb_io_module_registered;

extern atomic_t			kgdb_setting_breakpoint;
extern atomic_t			kgdb_cpu_doing_single_step;

extern struct task_struct	*kgdb_usethread;
extern struct task_struct	*kgdb_contthread;

enum kgdb_bptype {
	BP_BREAKPOINT = 0,
	BP_HARDWARE_BREAKPOINT,
	BP_WRITE_WATCHPOINT,
	BP_READ_WATCHPOINT,
	BP_ACCESS_WATCHPOINT,
	BP_POKE_BREAKPOINT,
};

enum kgdb_bpstate {
	BP_UNDEFINED = 0,
	BP_REMOVED,
	BP_SET,
	BP_ACTIVE
};

struct kgdb_bkpt {
	unsigned long		bpt_addr;
	unsigned char		saved_instr[BREAK_INSTR_SIZE];
	enum kgdb_bptype	type;
	enum kgdb_bpstate	state;
};

struct dbg_reg_def_t {
	char *name;
	int size;
	int offset;
};

#ifndef DBG_MAX_REG_NUM
#define DBG_MAX_REG_NUM 0
#else
extern struct dbg_reg_def_t dbg_reg_def[];
extern char *dbg_get_reg(int regno, void *mem, struct pt_regs *regs);
extern int dbg_set_reg(int regno, void *mem, struct pt_regs *regs);
#endif
#ifndef KGDB_MAX_BREAKPOINTS
# define KGDB_MAX_BREAKPOINTS	1000
#endif

#define KGDB_HW_BREAKPOINT	1


extern int kgdb_arch_init(void);

extern void kgdb_arch_exit(void);

extern void pt_regs_to_gdb_regs(unsigned long *gdb_regs, struct pt_regs *regs);

extern void
sleeping_thread_to_gdb_regs(unsigned long *gdb_regs, struct task_struct *p);

extern void gdb_regs_to_pt_regs(unsigned long *gdb_regs, struct pt_regs *regs);

extern int
kgdb_arch_handle_exception(int vector, int signo, int err_code,
			   char *remcom_in_buffer,
			   char *remcom_out_buffer,
			   struct pt_regs *regs);

extern void kgdb_roundup_cpus(unsigned long flags);

extern void kgdb_arch_set_pc(struct pt_regs *regs, unsigned long pc);


extern int kgdb_validate_break_address(unsigned long addr);
extern int kgdb_arch_set_breakpoint(struct kgdb_bkpt *bpt);
extern int kgdb_arch_remove_breakpoint(struct kgdb_bkpt *bpt);

extern void kgdb_arch_late(void);


struct kgdb_arch {
	unsigned char		gdb_bpt_instr[BREAK_INSTR_SIZE];
	unsigned long		flags;

	int	(*set_breakpoint)(unsigned long, char *);
	int	(*remove_breakpoint)(unsigned long, char *);
	int	(*set_hw_breakpoint)(unsigned long, int, enum kgdb_bptype);
	int	(*remove_hw_breakpoint)(unsigned long, int, enum kgdb_bptype);
	void	(*disable_hw_break)(struct pt_regs *regs);
	void	(*remove_all_hw_break)(void);
	void	(*correct_hw_break)(void);
};

struct kgdb_io {
	const char		*name;
	int			(*read_char) (void);
	void			(*write_char) (u8);
	void			(*flush) (void);
	int			(*init) (void);
	void			(*pre_exception) (void);
	void			(*post_exception) (void);
	int			is_console;
};

extern struct kgdb_arch		arch_kgdb_ops;

extern unsigned long __weak kgdb_arch_pc(int exception, struct pt_regs *regs);

extern int kgdb_register_io_module(struct kgdb_io *local_kgdb_io_ops);
extern void kgdb_unregister_io_module(struct kgdb_io *local_kgdb_io_ops);
extern struct kgdb_io *dbg_io_ops;

extern int kgdb_hex2long(char **ptr, unsigned long *long_val);
extern char *kgdb_mem2hex(char *mem, char *buf, int count);
extern int kgdb_hex2mem(char *buf, char *mem, int count);

extern int kgdb_isremovedbreak(unsigned long addr);
extern void kgdb_schedule_breakpoint(void);

extern int
kgdb_handle_exception(int ex_vector, int signo, int err_code,
		      struct pt_regs *regs);
extern int kgdb_nmicallback(int cpu, void *regs);
extern void gdbstub_exit(int status);

extern int			kgdb_single_step;
extern atomic_t			kgdb_active;
#define in_dbg_master() \
	(raw_smp_processor_id() == atomic_read(&kgdb_active))
extern bool dbg_is_early;
extern void __init dbg_late_init(void);
#else 
#define in_dbg_master() (0)
#define dbg_late_init()
#endif 
#endif 
