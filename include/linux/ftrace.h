
#ifndef _LINUX_FTRACE_H
#define _LINUX_FTRACE_H

#include <linux/trace_clock.h>
#include <linux/kallsyms.h>
#include <linux/linkage.h>
#include <linux/bitops.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>

#include <asm/ftrace.h>

struct module;
struct ftrace_hash;

#ifdef CONFIG_FUNCTION_TRACER

extern int ftrace_enabled;
extern int
ftrace_enable_sysctl(struct ctl_table *table, int write,
		     void __user *buffer, size_t *lenp,
		     loff_t *ppos);

typedef void (*ftrace_func_t)(unsigned long ip, unsigned long parent_ip);

enum {
	FTRACE_OPS_FL_ENABLED		= 1 << 0,
	FTRACE_OPS_FL_GLOBAL		= 1 << 1,
	FTRACE_OPS_FL_DYNAMIC		= 1 << 2,
	FTRACE_OPS_FL_CONTROL		= 1 << 3,
};

struct ftrace_ops {
	ftrace_func_t			func;
	struct ftrace_ops		*next;
	unsigned long			flags;
	int __percpu			*disabled;
#ifdef CONFIG_DYNAMIC_FTRACE
	struct ftrace_hash		*notrace_hash;
	struct ftrace_hash		*filter_hash;
#endif
};

extern int function_trace_stop;

enum ftrace_tracing_type_t {
	FTRACE_TYPE_ENTER = 0, 
	FTRACE_TYPE_RETURN,	
};

extern enum ftrace_tracing_type_t ftrace_tracing_type;

static inline void ftrace_stop(void)
{
	function_trace_stop = 1;
}

static inline void ftrace_start(void)
{
	function_trace_stop = 0;
}

int register_ftrace_function(struct ftrace_ops *ops);
int unregister_ftrace_function(struct ftrace_ops *ops);
void clear_ftrace_function(void);

static inline void ftrace_function_local_enable(struct ftrace_ops *ops)
{
	if (WARN_ON_ONCE(!(ops->flags & FTRACE_OPS_FL_CONTROL)))
		return;

	(*this_cpu_ptr(ops->disabled))--;
}

static inline void ftrace_function_local_disable(struct ftrace_ops *ops)
{
	if (WARN_ON_ONCE(!(ops->flags & FTRACE_OPS_FL_CONTROL)))
		return;

	(*this_cpu_ptr(ops->disabled))++;
}

static inline int ftrace_function_local_disabled(struct ftrace_ops *ops)
{
	WARN_ON_ONCE(!(ops->flags & FTRACE_OPS_FL_CONTROL));
	return *this_cpu_ptr(ops->disabled);
}

extern void ftrace_stub(unsigned long a0, unsigned long a1);

#else 
#define register_ftrace_function(ops) ({ 0; })
#define unregister_ftrace_function(ops) ({ 0; })
static inline void clear_ftrace_function(void) { }
static inline void ftrace_kill(void) { }
static inline void ftrace_stop(void) { }
static inline void ftrace_start(void) { }
#endif 

#ifdef CONFIG_STACK_TRACER
extern int stack_tracer_enabled;
int
stack_trace_sysctl(struct ctl_table *table, int write,
		   void __user *buffer, size_t *lenp,
		   loff_t *ppos);
#endif

struct ftrace_func_command {
	struct list_head	list;
	char			*name;
	int			(*func)(struct ftrace_hash *hash,
					char *func, char *cmd,
					char *params, int enable);
};

#ifdef CONFIG_DYNAMIC_FTRACE

int ftrace_arch_code_modify_prepare(void);
int ftrace_arch_code_modify_post_process(void);

void ftrace_bug(int err, unsigned long ip);

struct seq_file;

struct ftrace_probe_ops {
	void			(*func)(unsigned long ip,
					unsigned long parent_ip,
					void **data);
	int			(*callback)(unsigned long ip, void **data);
	void			(*free)(void **data);
	int			(*print)(struct seq_file *m,
					 unsigned long ip,
					 struct ftrace_probe_ops *ops,
					 void *data);
};

extern int
register_ftrace_function_probe(char *glob, struct ftrace_probe_ops *ops,
			      void *data);
extern void
unregister_ftrace_function_probe(char *glob, struct ftrace_probe_ops *ops,
				void *data);
extern void
unregister_ftrace_function_probe_func(char *glob, struct ftrace_probe_ops *ops);
extern void unregister_ftrace_function_probe_all(char *glob);

extern int ftrace_text_reserved(void *start, void *end);

enum {
	FTRACE_FL_ENABLED	= (1 << 30),
};

#define FTRACE_FL_MASK		(0x3UL << 30)
#define FTRACE_REF_MAX		((1 << 30) - 1)

struct dyn_ftrace {
	union {
		unsigned long		ip; 
		struct dyn_ftrace	*freelist;
	};
	unsigned long		flags;
	struct dyn_arch_ftrace		arch;
};

int ftrace_force_update(void);
int ftrace_set_filter(struct ftrace_ops *ops, unsigned char *buf,
		       int len, int reset);
int ftrace_set_notrace(struct ftrace_ops *ops, unsigned char *buf,
			int len, int reset);
void ftrace_set_global_filter(unsigned char *buf, int len, int reset);
void ftrace_set_global_notrace(unsigned char *buf, int len, int reset);
void ftrace_free_filter(struct ftrace_ops *ops);

int register_ftrace_command(struct ftrace_func_command *cmd);
int unregister_ftrace_command(struct ftrace_func_command *cmd);

enum {
	FTRACE_UPDATE_CALLS		= (1 << 0),
	FTRACE_DISABLE_CALLS		= (1 << 1),
	FTRACE_UPDATE_TRACE_FUNC	= (1 << 2),
	FTRACE_START_FUNC_RET		= (1 << 3),
	FTRACE_STOP_FUNC_RET		= (1 << 4),
};

enum {
	FTRACE_UPDATE_IGNORE,
	FTRACE_UPDATE_MAKE_CALL,
	FTRACE_UPDATE_MAKE_NOP,
};

enum {
	FTRACE_ITER_FILTER	= (1 << 0),
	FTRACE_ITER_NOTRACE	= (1 << 1),
	FTRACE_ITER_PRINTALL	= (1 << 2),
	FTRACE_ITER_DO_HASH	= (1 << 3),
	FTRACE_ITER_HASH	= (1 << 4),
	FTRACE_ITER_ENABLED	= (1 << 5),
};

void arch_ftrace_update_code(int command);

struct ftrace_rec_iter;

struct ftrace_rec_iter *ftrace_rec_iter_start(void);
struct ftrace_rec_iter *ftrace_rec_iter_next(struct ftrace_rec_iter *iter);
struct dyn_ftrace *ftrace_rec_iter_record(struct ftrace_rec_iter *iter);

int ftrace_update_record(struct dyn_ftrace *rec, int enable);
int ftrace_test_record(struct dyn_ftrace *rec, int enable);
void ftrace_run_stop_machine(int command);
int ftrace_location(unsigned long ip);

extern ftrace_func_t ftrace_trace_function;

int ftrace_regex_open(struct ftrace_ops *ops, int flag,
		  struct inode *inode, struct file *file);
ssize_t ftrace_filter_write(struct file *file, const char __user *ubuf,
			    size_t cnt, loff_t *ppos);
ssize_t ftrace_notrace_write(struct file *file, const char __user *ubuf,
			     size_t cnt, loff_t *ppos);
loff_t ftrace_regex_lseek(struct file *file, loff_t offset, int origin);
int ftrace_regex_release(struct inode *inode, struct file *file);

void __init
ftrace_set_early_filter(struct ftrace_ops *ops, char *buf, int enable);

extern int ftrace_ip_converted(unsigned long ip);
extern int ftrace_dyn_arch_init(void *data);
extern int ftrace_update_ftrace_func(ftrace_func_t func);
extern void ftrace_caller(void);
extern void ftrace_call(void);
extern void mcount_call(void);

#ifndef FTRACE_ADDR
#define FTRACE_ADDR ((unsigned long)ftrace_caller)
#endif
#ifdef CONFIG_FUNCTION_GRAPH_TRACER
extern void ftrace_graph_caller(void);
extern int ftrace_enable_ftrace_graph_caller(void);
extern int ftrace_disable_ftrace_graph_caller(void);
#else
static inline int ftrace_enable_ftrace_graph_caller(void) { return 0; }
static inline int ftrace_disable_ftrace_graph_caller(void) { return 0; }
#endif

extern int ftrace_make_nop(struct module *mod,
			   struct dyn_ftrace *rec, unsigned long addr);

extern int ftrace_make_call(struct dyn_ftrace *rec, unsigned long addr);

extern int ftrace_arch_read_dyn_info(char *buf, int size);

extern int skip_trace(unsigned long ip);

extern void ftrace_disable_daemon(void);
extern void ftrace_enable_daemon(void);
#else
static inline int skip_trace(unsigned long ip) { return 0; }
static inline int ftrace_force_update(void) { return 0; }
static inline void ftrace_disable_daemon(void) { }
static inline void ftrace_enable_daemon(void) { }
static inline void ftrace_release_mod(struct module *mod) {}
static inline int register_ftrace_command(struct ftrace_func_command *cmd)
{
	return -EINVAL;
}
static inline int unregister_ftrace_command(char *cmd_name)
{
	return -EINVAL;
}
static inline int ftrace_text_reserved(void *start, void *end)
{
	return 0;
}

#define ftrace_regex_open(ops, flag, inod, file) ({ -ENODEV; })
#define ftrace_set_early_filter(ops, buf, enable) do { } while (0)
#define ftrace_set_filter(ops, buf, len, reset) ({ -ENODEV; })
#define ftrace_set_notrace(ops, buf, len, reset) ({ -ENODEV; })
#define ftrace_free_filter(ops) do { } while (0)

static inline ssize_t ftrace_filter_write(struct file *file, const char __user *ubuf,
			    size_t cnt, loff_t *ppos) { return -ENODEV; }
static inline ssize_t ftrace_notrace_write(struct file *file, const char __user *ubuf,
			     size_t cnt, loff_t *ppos) { return -ENODEV; }
static inline loff_t ftrace_regex_lseek(struct file *file, loff_t offset, int origin)
{
	return -ENODEV;
}
static inline int
ftrace_regex_release(struct inode *inode, struct file *file) { return -ENODEV; }
#endif 

void ftrace_kill(void);

static inline void tracer_disable(void)
{
#ifdef CONFIG_FUNCTION_TRACER
	ftrace_enabled = 0;
#endif
}

static inline int __ftrace_enabled_save(void)
{
#ifdef CONFIG_FUNCTION_TRACER
	int saved_ftrace_enabled = ftrace_enabled;
	ftrace_enabled = 0;
	return saved_ftrace_enabled;
#else
	return 0;
#endif
}

static inline void __ftrace_enabled_restore(int enabled)
{
#ifdef CONFIG_FUNCTION_TRACER
	ftrace_enabled = enabled;
#endif
}

#ifndef HAVE_ARCH_CALLER_ADDR
# ifdef CONFIG_FRAME_POINTER
#  define CALLER_ADDR0 ((unsigned long)__builtin_return_address(0))
#  define CALLER_ADDR1 ((unsigned long)__builtin_return_address(1))
#  define CALLER_ADDR2 ((unsigned long)__builtin_return_address(2))
#  define CALLER_ADDR3 ((unsigned long)__builtin_return_address(3))
#  define CALLER_ADDR4 ((unsigned long)__builtin_return_address(4))
#  define CALLER_ADDR5 ((unsigned long)__builtin_return_address(5))
#  define CALLER_ADDR6 ((unsigned long)__builtin_return_address(6))
# else
#  define CALLER_ADDR0 ((unsigned long)__builtin_return_address(0))
#  define CALLER_ADDR1 0UL
#  define CALLER_ADDR2 0UL
#  define CALLER_ADDR3 0UL
#  define CALLER_ADDR4 0UL
#  define CALLER_ADDR5 0UL
#  define CALLER_ADDR6 0UL
# endif
#endif 

#ifdef CONFIG_IRQSOFF_TRACER
  extern void time_hardirqs_on(unsigned long a0, unsigned long a1);
  extern void time_hardirqs_off(unsigned long a0, unsigned long a1);
#else
  static inline void time_hardirqs_on(unsigned long a0, unsigned long a1) { }
  static inline void time_hardirqs_off(unsigned long a0, unsigned long a1) { }
#endif

#ifdef CONFIG_PREEMPT_TRACER
  extern void trace_preempt_on(unsigned long a0, unsigned long a1);
  extern void trace_preempt_off(unsigned long a0, unsigned long a1);
#else
  static inline void trace_preempt_on(unsigned long a0, unsigned long a1) { }
  static inline void trace_preempt_off(unsigned long a0, unsigned long a1) { }
#endif

#ifdef CONFIG_FTRACE_MCOUNT_RECORD
extern void ftrace_init(void);
#else
static inline void ftrace_init(void) { }
#endif

struct ftrace_graph_ent {
	unsigned long func; 
	int depth;
};

struct ftrace_graph_ret {
	unsigned long func; 
	unsigned long long calltime;
	unsigned long long rettime;
	
	unsigned long overrun;
	int depth;
};

typedef void (*trace_func_graph_ret_t)(struct ftrace_graph_ret *); 
typedef int (*trace_func_graph_ent_t)(struct ftrace_graph_ent *); 

#ifdef CONFIG_FUNCTION_GRAPH_TRACER

#define INIT_FTRACE_GRAPH		.ret_stack = NULL,

struct ftrace_ret_stack {
	unsigned long ret;
	unsigned long func;
	unsigned long long calltime;
	unsigned long long subtime;
	unsigned long fp;
};

extern void return_to_handler(void);

extern int
ftrace_push_return_trace(unsigned long ret, unsigned long func, int *depth,
			 unsigned long frame_pointer);

#define __notrace_funcgraph		notrace

#define __irq_entry		 __attribute__((__section__(".irqentry.text")))

extern char __irqentry_text_start[];
extern char __irqentry_text_end[];

#define FTRACE_RETFUNC_DEPTH 50
#define FTRACE_RETSTACK_ALLOC_SIZE 32
extern int register_ftrace_graph(trace_func_graph_ret_t retfunc,
				trace_func_graph_ent_t entryfunc);

extern void ftrace_graph_stop(void);

extern trace_func_graph_ret_t ftrace_graph_return;
extern trace_func_graph_ent_t ftrace_graph_entry;

extern void unregister_ftrace_graph(void);

extern void ftrace_graph_init_task(struct task_struct *t);
extern void ftrace_graph_exit_task(struct task_struct *t);
extern void ftrace_graph_init_idle_task(struct task_struct *t, int cpu);

static inline int task_curr_ret_stack(struct task_struct *t)
{
	return t->curr_ret_stack;
}

static inline void pause_graph_tracing(void)
{
	atomic_inc(&current->tracing_graph_pause);
}

static inline void unpause_graph_tracing(void)
{
	atomic_dec(&current->tracing_graph_pause);
}
#else 

#define __notrace_funcgraph
#define __irq_entry
#define INIT_FTRACE_GRAPH

static inline void ftrace_graph_init_task(struct task_struct *t) { }
static inline void ftrace_graph_exit_task(struct task_struct *t) { }
static inline void ftrace_graph_init_idle_task(struct task_struct *t, int cpu) { }

static inline int register_ftrace_graph(trace_func_graph_ret_t retfunc,
			  trace_func_graph_ent_t entryfunc)
{
	return -1;
}
static inline void unregister_ftrace_graph(void) { }

static inline int task_curr_ret_stack(struct task_struct *tsk)
{
	return -1;
}

static inline void pause_graph_tracing(void) { }
static inline void unpause_graph_tracing(void) { }
#endif 

#ifdef CONFIG_TRACING

enum {
	TSK_TRACE_FL_TRACE_BIT	= 0,
	TSK_TRACE_FL_GRAPH_BIT	= 1,
};
enum {
	TSK_TRACE_FL_TRACE	= 1 << TSK_TRACE_FL_TRACE_BIT,
	TSK_TRACE_FL_GRAPH	= 1 << TSK_TRACE_FL_GRAPH_BIT,
};

static inline void set_tsk_trace_trace(struct task_struct *tsk)
{
	set_bit(TSK_TRACE_FL_TRACE_BIT, &tsk->trace);
}

static inline void clear_tsk_trace_trace(struct task_struct *tsk)
{
	clear_bit(TSK_TRACE_FL_TRACE_BIT, &tsk->trace);
}

static inline int test_tsk_trace_trace(struct task_struct *tsk)
{
	return tsk->trace & TSK_TRACE_FL_TRACE;
}

static inline void set_tsk_trace_graph(struct task_struct *tsk)
{
	set_bit(TSK_TRACE_FL_GRAPH_BIT, &tsk->trace);
}

static inline void clear_tsk_trace_graph(struct task_struct *tsk)
{
	clear_bit(TSK_TRACE_FL_GRAPH_BIT, &tsk->trace);
}

static inline int test_tsk_trace_graph(struct task_struct *tsk)
{
	return tsk->trace & TSK_TRACE_FL_GRAPH;
}

enum ftrace_dump_mode;

extern enum ftrace_dump_mode ftrace_dump_on_oops;

#ifdef CONFIG_PREEMPT
#define INIT_TRACE_RECURSION		.trace_recursion = 0,
#endif

#endif 

#ifndef INIT_TRACE_RECURSION
#define INIT_TRACE_RECURSION
#endif

#ifdef CONFIG_FTRACE_SYSCALLS

unsigned long arch_syscall_addr(int nr);

#endif 

#endif 
