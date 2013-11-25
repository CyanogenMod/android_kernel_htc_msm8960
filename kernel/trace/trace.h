#ifndef _LINUX_KERNEL_TRACE_H
#define _LINUX_KERNEL_TRACE_H

#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#include <linux/clocksource.h>
#include <linux/ring_buffer.h>
#include <linux/mmiotrace.h>
#include <linux/tracepoint.h>
#include <linux/ftrace.h>
#include <linux/hw_breakpoint.h>
#include <linux/trace_seq.h>
#include <linux/ftrace_event.h>

enum trace_type {
	__TRACE_FIRST_TYPE = 0,

	TRACE_FN,
	TRACE_CTX,
	TRACE_WAKE,
	TRACE_STACK,
	TRACE_PRINT,
	TRACE_BPRINT,
	TRACE_MMIO_RW,
	TRACE_MMIO_MAP,
	TRACE_BRANCH,
	TRACE_GRAPH_RET,
	TRACE_GRAPH_ENT,
	TRACE_USER_STACK,
	TRACE_BLK,

	__TRACE_LAST_TYPE,
};


#undef __field
#define __field(type, item)		type	item;

#undef __field_struct
#define __field_struct(type, item)	__field(type, item)

#undef __field_desc
#define __field_desc(type, container, item)

#undef __array
#define __array(type, item, size)	type	item[size];

#undef __array_desc
#define __array_desc(type, container, item, size)

#undef __dynamic_array
#define __dynamic_array(type, item)	type	item[];

#undef F_STRUCT
#define F_STRUCT(args...)		args

#undef FTRACE_ENTRY
#define FTRACE_ENTRY(name, struct_name, id, tstruct, print, filter)	\
	struct struct_name {						\
		struct trace_entry	ent;				\
		tstruct							\
	}

#undef TP_ARGS
#define TP_ARGS(args...)	args

#undef FTRACE_ENTRY_DUP
#define FTRACE_ENTRY_DUP(name, name_struct, id, tstruct, printk, filter)

#undef FTRACE_ENTRY_REG
#define FTRACE_ENTRY_REG(name, struct_name, id, tstruct, print,	\
			 filter, regfn) \
	FTRACE_ENTRY(name, struct_name, id, PARAMS(tstruct), PARAMS(print), \
		     filter)

#include "trace_entries.h"

struct syscall_trace_enter {
	struct trace_entry	ent;
	int			nr;
	unsigned long		args[];
};

struct syscall_trace_exit {
	struct trace_entry	ent;
	int			nr;
	long			ret;
};

struct kprobe_trace_entry_head {
	struct trace_entry	ent;
	unsigned long		ip;
};

struct kretprobe_trace_entry_head {
	struct trace_entry	ent;
	unsigned long		func;
	unsigned long		ret_ip;
};

enum trace_flag_type {
	TRACE_FLAG_IRQS_OFF		= 0x01,
	TRACE_FLAG_IRQS_NOSUPPORT	= 0x02,
	TRACE_FLAG_NEED_RESCHED		= 0x04,
	TRACE_FLAG_HARDIRQ		= 0x08,
	TRACE_FLAG_SOFTIRQ		= 0x10,
};

#define TRACE_BUF_SIZE		1024

struct trace_array_cpu {
	atomic_t		disabled;
	void			*buffer_page;	

	unsigned long		saved_latency;
	unsigned long		critical_start;
	unsigned long		critical_end;
	unsigned long		critical_sequence;
	unsigned long		nice;
	unsigned long		policy;
	unsigned long		rt_priority;
	unsigned long		skipped_entries;
	cycle_t			preempt_timestamp;
	pid_t			pid;
	uid_t			uid;
	char			comm[TASK_COMM_LEN];
};

struct trace_array {
	struct ring_buffer	*buffer;
	unsigned long		entries;
	int			cpu;
	int			buffer_disabled;
	cycle_t			time_start;
	struct task_struct	*waiter;
	struct trace_array_cpu	*data[NR_CPUS];
};

#define FTRACE_CMP_TYPE(var, type) \
	__builtin_types_compatible_p(typeof(var), type *)

#undef IF_ASSIGN
#define IF_ASSIGN(var, entry, etype, id)		\
	if (FTRACE_CMP_TYPE(var, etype)) {		\
		var = (typeof(var))(entry);		\
		WARN_ON(id && (entry)->type != id);	\
		break;					\
	}

extern void __ftrace_bad_type(void);

#define trace_assign_type(var, ent)					\
	do {								\
		IF_ASSIGN(var, ent, struct ftrace_entry, TRACE_FN);	\
		IF_ASSIGN(var, ent, struct ctx_switch_entry, 0);	\
		IF_ASSIGN(var, ent, struct stack_entry, TRACE_STACK);	\
		IF_ASSIGN(var, ent, struct userstack_entry, TRACE_USER_STACK);\
		IF_ASSIGN(var, ent, struct print_entry, TRACE_PRINT);	\
		IF_ASSIGN(var, ent, struct bprint_entry, TRACE_BPRINT);	\
		IF_ASSIGN(var, ent, struct trace_mmiotrace_rw,		\
			  TRACE_MMIO_RW);				\
		IF_ASSIGN(var, ent, struct trace_mmiotrace_map,		\
			  TRACE_MMIO_MAP);				\
		IF_ASSIGN(var, ent, struct trace_branch, TRACE_BRANCH); \
		IF_ASSIGN(var, ent, struct ftrace_graph_ent_entry,	\
			  TRACE_GRAPH_ENT);		\
		IF_ASSIGN(var, ent, struct ftrace_graph_ret_entry,	\
			  TRACE_GRAPH_RET);		\
		__ftrace_bad_type();					\
	} while (0)

struct tracer_opt {
	const char	*name; 
	u32		bit; 
};

struct tracer_flags {
	u32			val;
	struct tracer_opt	*opts;
};

#define TRACER_OPT(s, b)	.name = #s, .bit = b


struct tracer {
	const char		*name;
	int			(*init)(struct trace_array *tr);
	void			(*reset)(struct trace_array *tr);
	void			(*start)(struct trace_array *tr);
	void			(*stop)(struct trace_array *tr);
	void			(*open)(struct trace_iterator *iter);
	void			(*pipe_open)(struct trace_iterator *iter);
	void			(*wait_pipe)(struct trace_iterator *iter);
	void			(*close)(struct trace_iterator *iter);
	void			(*pipe_close)(struct trace_iterator *iter);
	ssize_t			(*read)(struct trace_iterator *iter,
					struct file *filp, char __user *ubuf,
					size_t cnt, loff_t *ppos);
	ssize_t			(*splice_read)(struct trace_iterator *iter,
					       struct file *filp,
					       loff_t *ppos,
					       struct pipe_inode_info *pipe,
					       size_t len,
					       unsigned int flags);
#ifdef CONFIG_FTRACE_STARTUP_TEST
	int			(*selftest)(struct tracer *trace,
					    struct trace_array *tr);
#endif
	void			(*print_header)(struct seq_file *m);
	enum print_line_t	(*print_line)(struct trace_iterator *iter);
	
	int			(*set_flag)(u32 old_flags, u32 bit, int set);
	struct tracer		*next;
	struct tracer_flags	*flags;
	int			print_max;
	int			use_max_tr;
};


#define trace_recursion_inc() do { (current)->trace_recursion++; } while (0)
#define trace_recursion_dec() do { (current)->trace_recursion--; } while (0)

#define trace_recursion_buffer() ((current)->trace_recursion & 0x3ff)

#define TRACE_INTERNAL_BIT		(1<<11)
#define TRACE_GLOBAL_BIT		(1<<12)
#define TRACE_CONTROL_BIT		(1<<13)

#define TRACE_IRQ_BIT			(1<<13)

#define trace_recursion_set(bit)	do { (current)->trace_recursion |= (bit); } while (0)
#define trace_recursion_clear(bit)	do { (current)->trace_recursion &= ~(bit); } while (0)
#define trace_recursion_test(bit)	((current)->trace_recursion & (bit))

#define TRACE_PIPE_ALL_CPU	-1

int tracer_init(struct tracer *t, struct trace_array *tr);
int tracing_is_enabled(void);
void trace_wake_up(void);
void tracing_reset(struct trace_array *tr, int cpu);
void tracing_reset_online_cpus(struct trace_array *tr);
void tracing_reset_current(int cpu);
void tracing_reset_current_online_cpus(void);
int tracing_open_generic(struct inode *inode, struct file *filp);
struct dentry *trace_create_file(const char *name,
				 umode_t mode,
				 struct dentry *parent,
				 void *data,
				 const struct file_operations *fops);

struct dentry *tracing_init_dentry(void);

struct ring_buffer_event;

struct ring_buffer_event *
trace_buffer_lock_reserve(struct ring_buffer *buffer,
			  int type,
			  unsigned long len,
			  unsigned long flags,
			  int pc);
void trace_buffer_unlock_commit(struct ring_buffer *buffer,
				struct ring_buffer_event *event,
				unsigned long flags, int pc);

struct trace_entry *tracing_get_trace_entry(struct trace_array *tr,
						struct trace_array_cpu *data);

struct trace_entry *trace_find_next_entry(struct trace_iterator *iter,
					  int *ent_cpu, u64 *ent_ts);

int trace_empty(struct trace_iterator *iter);

void *trace_find_next_entry_inc(struct trace_iterator *iter);

void trace_init_global_iter(struct trace_iterator *iter);

void tracing_iter_reset(struct trace_iterator *iter, int cpu);

void default_wait_pipe(struct trace_iterator *iter);
void poll_wait_pipe(struct trace_iterator *iter);

void ftrace(struct trace_array *tr,
			    struct trace_array_cpu *data,
			    unsigned long ip,
			    unsigned long parent_ip,
			    unsigned long flags, int pc);
void tracing_sched_switch_trace(struct trace_array *tr,
				struct task_struct *prev,
				struct task_struct *next,
				unsigned long flags, int pc);

void tracing_sched_wakeup_trace(struct trace_array *tr,
				struct task_struct *wakee,
				struct task_struct *cur,
				unsigned long flags, int pc);
void trace_function(struct trace_array *tr,
		    unsigned long ip,
		    unsigned long parent_ip,
		    unsigned long flags, int pc);
void trace_graph_function(struct trace_array *tr,
		    unsigned long ip,
		    unsigned long parent_ip,
		    unsigned long flags, int pc);
void trace_latency_header(struct seq_file *m);
void trace_default_header(struct seq_file *m);
void print_trace_header(struct seq_file *m, struct trace_iterator *iter);
int trace_empty(struct trace_iterator *iter);

void trace_graph_return(struct ftrace_graph_ret *trace);
int trace_graph_entry(struct ftrace_graph_ent *trace);
void set_graph_array(struct trace_array *tr);

void tracing_start_cmdline_record(void);
void tracing_stop_cmdline_record(void);
void tracing_sched_switch_assign_trace(struct trace_array *tr);
void tracing_stop_sched_switch_record(void);
void tracing_start_sched_switch_record(void);
int register_tracer(struct tracer *type);
void unregister_tracer(struct tracer *type);
int is_tracing_stopped(void);
enum trace_file_type {
	TRACE_FILE_LAT_FMT	= 1,
	TRACE_FILE_ANNOTATE	= 2,
};

extern cpumask_var_t __read_mostly tracing_buffer_mask;

#define for_each_tracing_cpu(cpu)	\
	for_each_cpu(cpu, tracing_buffer_mask)

extern unsigned long nsecs_to_usecs(unsigned long nsecs);

extern unsigned long tracing_thresh;

#ifdef CONFIG_TRACER_MAX_TRACE
extern unsigned long tracing_max_latency;

void update_max_tr(struct trace_array *tr, struct task_struct *tsk, int cpu);
void update_max_tr_single(struct trace_array *tr,
			  struct task_struct *tsk, int cpu);
#endif 

#ifdef CONFIG_STACKTRACE
void ftrace_trace_stack(struct ring_buffer *buffer, unsigned long flags,
			int skip, int pc);

void ftrace_trace_stack_regs(struct ring_buffer *buffer, unsigned long flags,
			     int skip, int pc, struct pt_regs *regs);

void ftrace_trace_userstack(struct ring_buffer *buffer, unsigned long flags,
			    int pc);

void __trace_stack(struct trace_array *tr, unsigned long flags, int skip,
		   int pc);
#else
static inline void ftrace_trace_stack(struct ring_buffer *buffer,
				      unsigned long flags, int skip, int pc)
{
}

static inline void ftrace_trace_stack_regs(struct ring_buffer *buffer,
					   unsigned long flags, int skip,
					   int pc, struct pt_regs *regs)
{
}

static inline void ftrace_trace_userstack(struct ring_buffer *buffer,
					  unsigned long flags, int pc)
{
}

static inline void __trace_stack(struct trace_array *tr, unsigned long flags,
				 int skip, int pc)
{
}
#endif 

extern cycle_t ftrace_now(int cpu);

extern void trace_find_cmdline(int pid, char comm[]);

#ifdef CONFIG_DYNAMIC_FTRACE
extern unsigned long ftrace_update_tot_cnt;
#define DYN_FTRACE_TEST_NAME trace_selftest_dynamic_test_func
extern int DYN_FTRACE_TEST_NAME(void);
#define DYN_FTRACE_TEST_NAME2 trace_selftest_dynamic_test_func2
extern int DYN_FTRACE_TEST_NAME2(void);
#endif

extern int ring_buffer_expanded;
extern bool tracing_selftest_disabled;
DECLARE_PER_CPU(int, ftrace_cpu_disabled);

#ifdef CONFIG_FTRACE_STARTUP_TEST
extern int trace_selftest_startup_function(struct tracer *trace,
					   struct trace_array *tr);
extern int trace_selftest_startup_function_graph(struct tracer *trace,
						 struct trace_array *tr);
extern int trace_selftest_startup_irqsoff(struct tracer *trace,
					  struct trace_array *tr);
extern int trace_selftest_startup_preemptoff(struct tracer *trace,
					     struct trace_array *tr);
extern int trace_selftest_startup_preemptirqsoff(struct tracer *trace,
						 struct trace_array *tr);
extern int trace_selftest_startup_wakeup(struct tracer *trace,
					 struct trace_array *tr);
extern int trace_selftest_startup_nop(struct tracer *trace,
					 struct trace_array *tr);
extern int trace_selftest_startup_sched_switch(struct tracer *trace,
					       struct trace_array *tr);
extern int trace_selftest_startup_branch(struct tracer *trace,
					 struct trace_array *tr);
#endif 

extern void *head_page(struct trace_array_cpu *data);
extern unsigned long long ns2usecs(cycle_t nsec);
extern int
trace_vbprintk(unsigned long ip, const char *fmt, va_list args);
extern int
trace_vprintk(unsigned long ip, const char *fmt, va_list args);
extern int
trace_array_vprintk(struct trace_array *tr,
		    unsigned long ip, const char *fmt, va_list args);
int trace_array_printk(struct trace_array *tr,
		       unsigned long ip, const char *fmt, ...);
void trace_printk_seq(struct trace_seq *s);
enum print_line_t print_trace_line(struct trace_iterator *iter);

extern unsigned long trace_flags;

extern int trace_clock_id;

#ifdef CONFIG_FUNCTION_GRAPH_TRACER

#define TRACE_GRAPH_PRINT_OVERRUN       0x1
#define TRACE_GRAPH_PRINT_CPU           0x2
#define TRACE_GRAPH_PRINT_OVERHEAD      0x4
#define TRACE_GRAPH_PRINT_PROC          0x8
#define TRACE_GRAPH_PRINT_DURATION      0x10
#define TRACE_GRAPH_PRINT_ABS_TIME      0x20

extern enum print_line_t
print_graph_function_flags(struct trace_iterator *iter, u32 flags);
extern void print_graph_headers_flags(struct seq_file *s, u32 flags);
extern enum print_line_t
trace_print_graph_duration(unsigned long long duration, struct trace_seq *s);
extern void graph_trace_open(struct trace_iterator *iter);
extern void graph_trace_close(struct trace_iterator *iter);
extern int __trace_graph_entry(struct trace_array *tr,
			       struct ftrace_graph_ent *trace,
			       unsigned long flags, int pc);
extern void __trace_graph_return(struct trace_array *tr,
				 struct ftrace_graph_ret *trace,
				 unsigned long flags, int pc);


#ifdef CONFIG_DYNAMIC_FTRACE
#define FTRACE_GRAPH_MAX_FUNCS		32
extern int ftrace_graph_filter_enabled;
extern int ftrace_graph_count;
extern unsigned long ftrace_graph_funcs[FTRACE_GRAPH_MAX_FUNCS];

static inline int ftrace_graph_addr(unsigned long addr)
{
	int i;

	if (!ftrace_graph_filter_enabled)
		return 1;

	for (i = 0; i < ftrace_graph_count; i++) {
		if (addr == ftrace_graph_funcs[i]) {
			if (in_irq())
				trace_recursion_set(TRACE_IRQ_BIT);
			else
				trace_recursion_clear(TRACE_IRQ_BIT);
			return 1;
		}
	}

	return 0;
}
#else
static inline int ftrace_graph_addr(unsigned long addr)
{
	return 1;
}
#endif 
#else 
static inline enum print_line_t
print_graph_function_flags(struct trace_iterator *iter, u32 flags)
{
	return TRACE_TYPE_UNHANDLED;
}
#endif 

extern struct list_head ftrace_pids;

#ifdef CONFIG_FUNCTION_TRACER
static inline int ftrace_trace_task(struct task_struct *task)
{
	if (list_empty(&ftrace_pids))
		return 1;

	return test_tsk_trace_trace(task);
}
extern int ftrace_is_dead(void);
#else
static inline int ftrace_trace_task(struct task_struct *task)
{
	return 1;
}
static inline int ftrace_is_dead(void) { return 0; }
#endif

int ftrace_event_is_function(struct ftrace_event_call *call);

struct trace_parser {
	bool		cont;
	char		*buffer;
	unsigned	idx;
	unsigned	size;
};

static inline bool trace_parser_loaded(struct trace_parser *parser)
{
	return (parser->idx != 0);
}

static inline bool trace_parser_cont(struct trace_parser *parser)
{
	return parser->cont;
}

static inline void trace_parser_clear(struct trace_parser *parser)
{
	parser->cont = false;
	parser->idx = 0;
}

extern int trace_parser_get_init(struct trace_parser *parser, int size);
extern void trace_parser_put(struct trace_parser *parser);
extern int trace_get_user(struct trace_parser *parser, const char __user *ubuf,
	size_t cnt, loff_t *ppos);

enum trace_iterator_flags {
	TRACE_ITER_PRINT_PARENT		= 0x01,
	TRACE_ITER_SYM_OFFSET		= 0x02,
	TRACE_ITER_SYM_ADDR		= 0x04,
	TRACE_ITER_VERBOSE		= 0x08,
	TRACE_ITER_RAW			= 0x10,
	TRACE_ITER_HEX			= 0x20,
	TRACE_ITER_BIN			= 0x40,
	TRACE_ITER_BLOCK		= 0x80,
	TRACE_ITER_STACKTRACE		= 0x100,
	TRACE_ITER_PRINTK		= 0x200,
	TRACE_ITER_PREEMPTONLY		= 0x400,
	TRACE_ITER_BRANCH		= 0x800,
	TRACE_ITER_ANNOTATE		= 0x1000,
	TRACE_ITER_USERSTACKTRACE       = 0x2000,
	TRACE_ITER_SYM_USEROBJ          = 0x4000,
	TRACE_ITER_PRINTK_MSGONLY	= 0x8000,
	TRACE_ITER_CONTEXT_INFO		= 0x10000, 
	TRACE_ITER_LATENCY_FMT		= 0x20000,
	TRACE_ITER_SLEEP_TIME		= 0x40000,
	TRACE_ITER_GRAPH_TIME		= 0x80000,
	TRACE_ITER_RECORD_CMD		= 0x100000,
	TRACE_ITER_OVERWRITE		= 0x200000,
	TRACE_ITER_STOP_ON_FREE		= 0x400000,
	TRACE_ITER_IRQ_INFO		= 0x800000,
};

#define TRACE_ITER_SYM_MASK \
	(TRACE_ITER_PRINT_PARENT|TRACE_ITER_SYM_OFFSET|TRACE_ITER_SYM_ADDR)

extern struct tracer nop_trace;

#ifdef CONFIG_BRANCH_TRACER
extern int enable_branch_tracing(struct trace_array *tr);
extern void disable_branch_tracing(void);
static inline int trace_branch_enable(struct trace_array *tr)
{
	if (trace_flags & TRACE_ITER_BRANCH)
		return enable_branch_tracing(tr);
	return 0;
}
static inline void trace_branch_disable(void)
{
	
	disable_branch_tracing();
}
#else
static inline int trace_branch_enable(struct trace_array *tr)
{
	return 0;
}
static inline void trace_branch_disable(void)
{
}
#endif 

int tracing_update_buffers(void);

enum {
	TRACE_EVENT_TYPE_PRINTF		= 1,
	TRACE_EVENT_TYPE_RAW		= 2,
};

struct ftrace_event_field {
	struct list_head	link;
	char			*name;
	char			*type;
	int			filter_type;
	int			offset;
	int			size;
	int			is_signed;
};

struct event_filter {
	int			n_preds;	
	int			a_preds;	
	struct filter_pred	*preds;
	struct filter_pred	*root;
	char			*filter_string;
};

struct event_subsystem {
	struct list_head	list;
	const char		*name;
	struct dentry		*entry;
	struct event_filter	*filter;
	int			nr_events;
	int			ref_count;
};

#define FILTER_PRED_INVALID	((unsigned short)-1)
#define FILTER_PRED_IS_RIGHT	(1 << 15)
#define FILTER_PRED_FOLD	(1 << 15)

#define MAX_FILTER_PRED		16384

struct filter_pred;
struct regex;

typedef int (*filter_pred_fn_t) (struct filter_pred *pred, void *event);

typedef int (*regex_match_func)(char *str, struct regex *r, int len);

enum regex_type {
	MATCH_FULL = 0,
	MATCH_FRONT_ONLY,
	MATCH_MIDDLE_ONLY,
	MATCH_END_ONLY,
};

struct regex {
	char			pattern[MAX_FILTER_STR_VAL];
	int			len;
	int			field_len;
	regex_match_func	match;
};

struct filter_pred {
	filter_pred_fn_t 	fn;
	u64 			val;
	struct regex		regex;
	unsigned short		*ops;
	struct ftrace_event_field *field;
	int 			offset;
	int 			not;
	int 			op;
	unsigned short		index;
	unsigned short		parent;
	unsigned short		left;
	unsigned short		right;
};

extern struct list_head ftrace_common_fields;

extern enum regex_type
filter_parse_regex(char *buff, int len, char **search, int *not);
extern void print_event_filter(struct ftrace_event_call *call,
			       struct trace_seq *s);
extern int apply_event_filter(struct ftrace_event_call *call,
			      char *filter_string);
extern int apply_subsystem_event_filter(struct event_subsystem *system,
					char *filter_string);
extern void print_subsystem_event_filter(struct event_subsystem *system,
					 struct trace_seq *s);
extern int filter_assign_type(const char *type);

struct list_head *
trace_get_fields(struct ftrace_event_call *event_call);

static inline int
filter_check_discard(struct ftrace_event_call *call, void *rec,
		     struct ring_buffer *buffer,
		     struct ring_buffer_event *event)
{
	if (unlikely(call->flags & TRACE_EVENT_FL_FILTERED) &&
	    !filter_match_preds(call->filter, rec)) {
		ring_buffer_discard_commit(buffer, event);
		return 1;
	}

	return 0;
}

extern void trace_event_enable_cmd_record(bool enable);

extern struct mutex event_mutex;
extern struct list_head ftrace_events;

extern const char *__start___trace_bprintk_fmt[];
extern const char *__stop___trace_bprintk_fmt[];

#undef FTRACE_ENTRY
#define FTRACE_ENTRY(call, struct_name, id, tstruct, print, filter)	\
	extern struct ftrace_event_call					\
	__attribute__((__aligned__(4))) event_##call;
#undef FTRACE_ENTRY_DUP
#define FTRACE_ENTRY_DUP(call, struct_name, id, tstruct, print, filter)	\
	FTRACE_ENTRY(call, struct_name, id, PARAMS(tstruct), PARAMS(print), \
		     filter)
#include "trace_entries.h"

#if defined(CONFIG_PERF_EVENTS) && defined(CONFIG_FUNCTION_TRACER)
int perf_ftrace_event_register(struct ftrace_event_call *call,
			       enum trace_reg type, void *data);
#else
#define perf_ftrace_event_register NULL
#endif

#endif 
