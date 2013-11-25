/*
 * nop tracer
 *
 * Copyright (C) 2008 Steven Noonan <steven@uplinklabs.net>
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/ftrace.h>

#include "trace.h"

enum {
	TRACE_NOP_OPT_ACCEPT = 0x1,
	TRACE_NOP_OPT_REFUSE = 0x2
};

static struct tracer_opt nop_opts[] = {
	
	{ TRACER_OPT(test_nop_accept, TRACE_NOP_OPT_ACCEPT) },
	
	{ TRACER_OPT(test_nop_refuse, TRACE_NOP_OPT_REFUSE) },
	{ } 
};

static struct tracer_flags nop_flags = {
	
	.val = 0, 
	.opts = nop_opts
};

static struct trace_array	*ctx_trace;

static void start_nop_trace(struct trace_array *tr)
{
	
}

static void stop_nop_trace(struct trace_array *tr)
{
	
}

static int nop_trace_init(struct trace_array *tr)
{
	ctx_trace = tr;
	start_nop_trace(tr);
	return 0;
}

static void nop_trace_reset(struct trace_array *tr)
{
	stop_nop_trace(tr);
}

static int nop_set_flag(u32 old_flags, u32 bit, int set)
{
	if (bit == TRACE_NOP_OPT_ACCEPT) {
		printk(KERN_DEBUG "nop_test_accept flag set to %d: we accept."
			" Now cat trace_options to see the result\n",
			set);
		return 0;
	}

	if (bit == TRACE_NOP_OPT_REFUSE) {
		printk(KERN_DEBUG "nop_test_refuse flag set to %d: we refuse."
			"Now cat trace_options to see the result\n",
			set);
		return -EINVAL;
	}

	return 0;
}


struct tracer nop_trace __read_mostly =
{
	.name		= "nop",
	.init		= nop_trace_init,
	.reset		= nop_trace_reset,
	.wait_pipe	= poll_wait_pipe,
#ifdef CONFIG_FTRACE_SELFTEST
	.selftest	= trace_selftest_startup_nop,
#endif
	.flags		= &nop_flags,
	.set_flag	= nop_set_flag
};

