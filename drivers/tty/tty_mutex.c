#include <linux/tty.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/semaphore.h>
#include <linux/sched.h>

static DEFINE_MUTEX(big_tty_mutex);

void __lockfunc tty_lock(void)
{
	mutex_lock(&big_tty_mutex);
}
EXPORT_SYMBOL(tty_lock);

void __lockfunc tty_unlock(void)
{
	mutex_unlock(&big_tty_mutex);
}
EXPORT_SYMBOL(tty_unlock);
