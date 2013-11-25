#include <linux/export.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>

int _atomic_dec_and_lock(atomic_t *atomic, spinlock_t *lock)
{
	
	if (atomic_add_unless(atomic, -1, 1))
		return 0;

	
	spin_lock(lock);
	if (atomic_dec_and_test(atomic))
		return 1;
	spin_unlock(lock);
	return 0;
}

EXPORT_SYMBOL(_atomic_dec_and_lock);
