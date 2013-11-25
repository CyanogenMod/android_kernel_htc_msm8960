/*
 * User-mode machine state access
 *
 * Copyright (C) 2007 Red Hat, Inc.  All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * Red Hat Author: Roland McGrath.
 */

#ifndef _LINUX_REGSET_H
#define _LINUX_REGSET_H	1

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/bug.h>
#include <linux/uaccess.h>
struct task_struct;
struct user_regset;


typedef int user_regset_active_fn(struct task_struct *target,
				  const struct user_regset *regset);

typedef int user_regset_get_fn(struct task_struct *target,
			       const struct user_regset *regset,
			       unsigned int pos, unsigned int count,
			       void *kbuf, void __user *ubuf);

typedef int user_regset_set_fn(struct task_struct *target,
			       const struct user_regset *regset,
			       unsigned int pos, unsigned int count,
			       const void *kbuf, const void __user *ubuf);

/**
 * user_regset_writeback_fn - type of @writeback function in &struct user_regset
 * @target:	thread being examined
 * @regset:	regset being examined
 * @immediate:	zero if writeback at completion of next context switch is OK
 *
 * This call is optional; usually the pointer is %NULL.  When
 * provided, there is some user memory associated with this regset's
 * hardware, such as memory backing cached register data on register
 * window machines; the regset's data controls what user memory is
 * used (e.g. via the stack pointer value).
 *
 * Write register data back to user memory.  If the @immediate flag
 * is nonzero, it must be written to the user memory so uaccess or
 * access_process_vm() can see it when this call returns; if zero,
 * then it must be written back by the time the task completes a
 * context switch (as synchronized with wait_task_inactive()).
 * Return %0 on success or if there was nothing to do, -%EFAULT for
 * a memory problem (bad stack pointer or whatever), or -%EIO for a
 * hardware problem.
 */
typedef int user_regset_writeback_fn(struct task_struct *target,
				     const struct user_regset *regset,
				     int immediate);

struct user_regset {
	user_regset_get_fn		*get;
	user_regset_set_fn		*set;
	user_regset_active_fn		*active;
	user_regset_writeback_fn	*writeback;
	unsigned int			n;
	unsigned int 			size;
	unsigned int 			align;
	unsigned int 			bias;
	unsigned int 			core_note_type;
};

/**
 * struct user_regset_view - available regsets
 * @name:	Identifier, e.g. UTS_MACHINE string.
 * @regsets:	Array of @n regsets available in this view.
 * @n:		Number of elements in @regsets.
 * @e_machine:	ELF header @e_machine %EM_* value written in core dumps.
 * @e_flags:	ELF header @e_flags value written in core dumps.
 * @ei_osabi:	ELF header @e_ident[%EI_OSABI] value written in core dumps.
 *
 * A regset view is a collection of regsets (&struct user_regset,
 * above).  This describes all the state of a thread that can be seen
 * from a given architecture/ABI environment.  More than one view might
 * refer to the same &struct user_regset, or more than one regset
 * might refer to the same machine-specific state in the thread.  For
 * example, a 32-bit thread's state could be examined from the 32-bit
 * view or from the 64-bit view.  Either method reaches the same thread
 * register state, doing appropriate widening or truncation.
 */
struct user_regset_view {
	const char *name;
	const struct user_regset *regsets;
	unsigned int n;
	u32 e_flags;
	u16 e_machine;
	u8 ei_osabi;
};

const struct user_regset_view *task_user_regset_view(struct task_struct *tsk);



static inline int user_regset_copyout(unsigned int *pos, unsigned int *count,
				      void **kbuf,
				      void __user **ubuf, const void *data,
				      const int start_pos, const int end_pos)
{
	if (*count == 0)
		return 0;
	BUG_ON(*pos < start_pos);
	if (end_pos < 0 || *pos < end_pos) {
		unsigned int copy = (end_pos < 0 ? *count
				     : min(*count, end_pos - *pos));
		data += *pos - start_pos;
		if (*kbuf) {
			memcpy(*kbuf, data, copy);
			*kbuf += copy;
		} else if (__copy_to_user(*ubuf, data, copy))
			return -EFAULT;
		else
			*ubuf += copy;
		*pos += copy;
		*count -= copy;
	}
	return 0;
}

static inline int user_regset_copyin(unsigned int *pos, unsigned int *count,
				     const void **kbuf,
				     const void __user **ubuf, void *data,
				     const int start_pos, const int end_pos)
{
	if (*count == 0)
		return 0;
	BUG_ON(*pos < start_pos);
	if (end_pos < 0 || *pos < end_pos) {
		unsigned int copy = (end_pos < 0 ? *count
				     : min(*count, end_pos - *pos));
		data += *pos - start_pos;
		if (*kbuf) {
			memcpy(data, *kbuf, copy);
			*kbuf += copy;
		} else if (__copy_from_user(data, *ubuf, copy))
			return -EFAULT;
		else
			*ubuf += copy;
		*pos += copy;
		*count -= copy;
	}
	return 0;
}

static inline int user_regset_copyout_zero(unsigned int *pos,
					   unsigned int *count,
					   void **kbuf, void __user **ubuf,
					   const int start_pos,
					   const int end_pos)
{
	if (*count == 0)
		return 0;
	BUG_ON(*pos < start_pos);
	if (end_pos < 0 || *pos < end_pos) {
		unsigned int copy = (end_pos < 0 ? *count
				     : min(*count, end_pos - *pos));
		if (*kbuf) {
			memset(*kbuf, 0, copy);
			*kbuf += copy;
		} else if (__clear_user(*ubuf, copy))
			return -EFAULT;
		else
			*ubuf += copy;
		*pos += copy;
		*count -= copy;
	}
	return 0;
}

static inline int user_regset_copyin_ignore(unsigned int *pos,
					    unsigned int *count,
					    const void **kbuf,
					    const void __user **ubuf,
					    const int start_pos,
					    const int end_pos)
{
	if (*count == 0)
		return 0;
	BUG_ON(*pos < start_pos);
	if (end_pos < 0 || *pos < end_pos) {
		unsigned int copy = (end_pos < 0 ? *count
				     : min(*count, end_pos - *pos));
		if (*kbuf)
			*kbuf += copy;
		else
			*ubuf += copy;
		*pos += copy;
		*count -= copy;
	}
	return 0;
}

static inline int copy_regset_to_user(struct task_struct *target,
				      const struct user_regset_view *view,
				      unsigned int setno,
				      unsigned int offset, unsigned int size,
				      void __user *data)
{
	const struct user_regset *regset = &view->regsets[setno];

	if (!regset->get)
		return -EOPNOTSUPP;

	if (!access_ok(VERIFY_WRITE, data, size))
		return -EFAULT;

	return regset->get(target, regset, offset, size, NULL, data);
}

static inline int copy_regset_from_user(struct task_struct *target,
					const struct user_regset_view *view,
					unsigned int setno,
					unsigned int offset, unsigned int size,
					const void __user *data)
{
	const struct user_regset *regset = &view->regsets[setno];

	if (!regset->set)
		return -EOPNOTSUPP;

	if (!access_ok(VERIFY_READ, data, size))
		return -EFAULT;

	return regset->set(target, regset, offset, size, NULL, data);
}


#endif	
