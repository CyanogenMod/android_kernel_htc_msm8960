
#include <linux/sched.h>
#include <linux/posix-timers.h>
#include <linux/errno.h>
#include <linux/math64.h>
#include <asm/uaccess.h>
#include <linux/kernel_stat.h>
#include <trace/events/timer.h>

void update_rlimit_cpu(struct task_struct *task, unsigned long rlim_new)
{
	cputime_t cputime = secs_to_cputime(rlim_new);

	spin_lock_irq(&task->sighand->siglock);
	set_process_cpu_timer(task, CPUCLOCK_PROF, &cputime, NULL);
	spin_unlock_irq(&task->sighand->siglock);
}

static int check_clock(const clockid_t which_clock)
{
	int error = 0;
	struct task_struct *p;
	const pid_t pid = CPUCLOCK_PID(which_clock);

	if (CPUCLOCK_WHICH(which_clock) >= CPUCLOCK_MAX)
		return -EINVAL;

	if (pid == 0)
		return 0;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (!p || !(CPUCLOCK_PERTHREAD(which_clock) ?
		   same_thread_group(p, current) : has_group_leader_pid(p))) {
		error = -EINVAL;
	}
	rcu_read_unlock();

	return error;
}

static inline union cpu_time_count
timespec_to_sample(const clockid_t which_clock, const struct timespec *tp)
{
	union cpu_time_count ret;
	ret.sched = 0;		
	if (CPUCLOCK_WHICH(which_clock) == CPUCLOCK_SCHED) {
		ret.sched = (unsigned long long)tp->tv_sec * NSEC_PER_SEC + tp->tv_nsec;
	} else {
		ret.cpu = timespec_to_cputime(tp);
	}
	return ret;
}

static void sample_to_timespec(const clockid_t which_clock,
			       union cpu_time_count cpu,
			       struct timespec *tp)
{
	if (CPUCLOCK_WHICH(which_clock) == CPUCLOCK_SCHED)
		*tp = ns_to_timespec(cpu.sched);
	else
		cputime_to_timespec(cpu.cpu, tp);
}

static inline int cpu_time_before(const clockid_t which_clock,
				  union cpu_time_count now,
				  union cpu_time_count then)
{
	if (CPUCLOCK_WHICH(which_clock) == CPUCLOCK_SCHED) {
		return now.sched < then.sched;
	}  else {
		return now.cpu < then.cpu;
	}
}
static inline void cpu_time_add(const clockid_t which_clock,
				union cpu_time_count *acc,
			        union cpu_time_count val)
{
	if (CPUCLOCK_WHICH(which_clock) == CPUCLOCK_SCHED) {
		acc->sched += val.sched;
	}  else {
		acc->cpu += val.cpu;
	}
}
static inline union cpu_time_count cpu_time_sub(const clockid_t which_clock,
						union cpu_time_count a,
						union cpu_time_count b)
{
	if (CPUCLOCK_WHICH(which_clock) == CPUCLOCK_SCHED) {
		a.sched -= b.sched;
	}  else {
		a.cpu -= b.cpu;
	}
	return a;
}

static void bump_cpu_timer(struct k_itimer *timer,
				  union cpu_time_count now)
{
	int i;

	if (timer->it.cpu.incr.sched == 0)
		return;

	if (CPUCLOCK_WHICH(timer->it_clock) == CPUCLOCK_SCHED) {
		unsigned long long delta, incr;

		if (now.sched < timer->it.cpu.expires.sched)
			return;
		incr = timer->it.cpu.incr.sched;
		delta = now.sched + incr - timer->it.cpu.expires.sched;
		
		for (i = 0; incr < delta - incr; i++)
			incr = incr << 1;
		for (; i >= 0; incr >>= 1, i--) {
			if (delta < incr)
				continue;
			timer->it.cpu.expires.sched += incr;
			timer->it_overrun += 1 << i;
			delta -= incr;
		}
	} else {
		cputime_t delta, incr;

		if (now.cpu < timer->it.cpu.expires.cpu)
			return;
		incr = timer->it.cpu.incr.cpu;
		delta = now.cpu + incr - timer->it.cpu.expires.cpu;
		
		for (i = 0; incr < delta - incr; i++)
			     incr += incr;
		for (; i >= 0; incr = incr >> 1, i--) {
			if (delta < incr)
				continue;
			timer->it.cpu.expires.cpu += incr;
			timer->it_overrun += 1 << i;
			delta -= incr;
		}
	}
}

static inline cputime_t prof_ticks(struct task_struct *p)
{
	return p->utime + p->stime;
}
static inline cputime_t virt_ticks(struct task_struct *p)
{
	return p->utime;
}

static int
posix_cpu_clock_getres(const clockid_t which_clock, struct timespec *tp)
{
	int error = check_clock(which_clock);
	if (!error) {
		tp->tv_sec = 0;
		tp->tv_nsec = ((NSEC_PER_SEC + HZ - 1) / HZ);
		if (CPUCLOCK_WHICH(which_clock) == CPUCLOCK_SCHED) {
			tp->tv_nsec = 1;
		}
	}
	return error;
}

static int
posix_cpu_clock_set(const clockid_t which_clock, const struct timespec *tp)
{
	int error = check_clock(which_clock);
	if (error == 0) {
		error = -EPERM;
	}
	return error;
}


static int cpu_clock_sample(const clockid_t which_clock, struct task_struct *p,
			    union cpu_time_count *cpu)
{
	switch (CPUCLOCK_WHICH(which_clock)) {
	default:
		return -EINVAL;
	case CPUCLOCK_PROF:
		cpu->cpu = prof_ticks(p);
		break;
	case CPUCLOCK_VIRT:
		cpu->cpu = virt_ticks(p);
		break;
	case CPUCLOCK_SCHED:
		cpu->sched = task_sched_runtime(p);
		break;
	}
	return 0;
}

void thread_group_cputime(struct task_struct *tsk, struct task_cputime *times)
{
	struct signal_struct *sig = tsk->signal;
	struct task_struct *t;

	times->utime = sig->utime;
	times->stime = sig->stime;
	times->sum_exec_runtime = sig->sum_sched_runtime;

	rcu_read_lock();
	
	if (!likely(pid_alive(tsk)))
		goto out;

	t = tsk;
	do {
		times->utime += t->utime;
		times->stime += t->stime;
		times->sum_exec_runtime += task_sched_runtime(t);
	} while_each_thread(tsk, t);
out:
	rcu_read_unlock();
}

static void update_gt_cputime(struct task_cputime *a, struct task_cputime *b)
{
	if (b->utime > a->utime)
		a->utime = b->utime;

	if (b->stime > a->stime)
		a->stime = b->stime;

	if (b->sum_exec_runtime > a->sum_exec_runtime)
		a->sum_exec_runtime = b->sum_exec_runtime;
}

void thread_group_cputimer(struct task_struct *tsk, struct task_cputime *times)
{
	struct thread_group_cputimer *cputimer = &tsk->signal->cputimer;
	struct task_cputime sum;
	unsigned long flags;

	if (!cputimer->running) {
		thread_group_cputime(tsk, &sum);
		raw_spin_lock_irqsave(&cputimer->lock, flags);
		cputimer->running = 1;
		update_gt_cputime(&cputimer->cputime, &sum);
	} else
		raw_spin_lock_irqsave(&cputimer->lock, flags);
	*times = cputimer->cputime;
	raw_spin_unlock_irqrestore(&cputimer->lock, flags);
}

static int cpu_clock_sample_group(const clockid_t which_clock,
				  struct task_struct *p,
				  union cpu_time_count *cpu)
{
	struct task_cputime cputime;

	switch (CPUCLOCK_WHICH(which_clock)) {
	default:
		return -EINVAL;
	case CPUCLOCK_PROF:
		thread_group_cputime(p, &cputime);
		cpu->cpu = cputime.utime + cputime.stime;
		break;
	case CPUCLOCK_VIRT:
		thread_group_cputime(p, &cputime);
		cpu->cpu = cputime.utime;
		break;
	case CPUCLOCK_SCHED:
		thread_group_cputime(p, &cputime);
		cpu->sched = cputime.sum_exec_runtime;
		break;
	}
	return 0;
}


static int posix_cpu_clock_get(const clockid_t which_clock, struct timespec *tp)
{
	const pid_t pid = CPUCLOCK_PID(which_clock);
	int error = -EINVAL;
	union cpu_time_count rtn;

	if (pid == 0) {
		if (CPUCLOCK_PERTHREAD(which_clock)) {
			error = cpu_clock_sample(which_clock,
						 current, &rtn);
		} else {
			read_lock(&tasklist_lock);
			error = cpu_clock_sample_group(which_clock,
						       current, &rtn);
			read_unlock(&tasklist_lock);
		}
	} else {
		struct task_struct *p;
		rcu_read_lock();
		p = find_task_by_vpid(pid);
		if (p) {
			if (CPUCLOCK_PERTHREAD(which_clock)) {
				if (same_thread_group(p, current)) {
					error = cpu_clock_sample(which_clock,
								 p, &rtn);
				}
			} else {
				read_lock(&tasklist_lock);
				if (thread_group_leader(p) && p->sighand) {
					error =
					    cpu_clock_sample_group(which_clock,
							           p, &rtn);
				}
				read_unlock(&tasklist_lock);
			}
		}
		rcu_read_unlock();
	}

	if (error)
		return error;
	sample_to_timespec(which_clock, rtn, tp);
	return 0;
}


static int posix_cpu_timer_create(struct k_itimer *new_timer)
{
	int ret = 0;
	const pid_t pid = CPUCLOCK_PID(new_timer->it_clock);
	struct task_struct *p;

	if (CPUCLOCK_WHICH(new_timer->it_clock) >= CPUCLOCK_MAX)
		return -EINVAL;

	INIT_LIST_HEAD(&new_timer->it.cpu.entry);

	rcu_read_lock();
	if (CPUCLOCK_PERTHREAD(new_timer->it_clock)) {
		if (pid == 0) {
			p = current;
		} else {
			p = find_task_by_vpid(pid);
			if (p && !same_thread_group(p, current))
				p = NULL;
		}
	} else {
		if (pid == 0) {
			p = current->group_leader;
		} else {
			p = find_task_by_vpid(pid);
			if (p && !has_group_leader_pid(p))
				p = NULL;
		}
	}
	new_timer->it.cpu.task = p;
	if (p) {
		get_task_struct(p);
	} else {
		ret = -EINVAL;
	}
	rcu_read_unlock();

	return ret;
}

static int posix_cpu_timer_del(struct k_itimer *timer)
{
	struct task_struct *p = timer->it.cpu.task;
	int ret = 0;

	if (likely(p != NULL)) {
		read_lock(&tasklist_lock);
		if (unlikely(p->sighand == NULL)) {
			BUG_ON(!list_empty(&timer->it.cpu.entry));
		} else {
			spin_lock(&p->sighand->siglock);
			if (timer->it.cpu.firing)
				ret = TIMER_RETRY;
			else
				list_del(&timer->it.cpu.entry);
			spin_unlock(&p->sighand->siglock);
		}
		read_unlock(&tasklist_lock);

		if (!ret)
			put_task_struct(p);
	}

	return ret;
}

static void cleanup_timers(struct list_head *head,
			   cputime_t utime, cputime_t stime,
			   unsigned long long sum_exec_runtime)
{
	struct cpu_timer_list *timer, *next;
	cputime_t ptime = utime + stime;

	list_for_each_entry_safe(timer, next, head, entry) {
		list_del_init(&timer->entry);
		if (timer->expires.cpu < ptime) {
			timer->expires.cpu = 0;
		} else {
			timer->expires.cpu -= ptime;
		}
	}

	++head;
	list_for_each_entry_safe(timer, next, head, entry) {
		list_del_init(&timer->entry);
		if (timer->expires.cpu < utime) {
			timer->expires.cpu = 0;
		} else {
			timer->expires.cpu -= utime;
		}
	}

	++head;
	list_for_each_entry_safe(timer, next, head, entry) {
		list_del_init(&timer->entry);
		if (timer->expires.sched < sum_exec_runtime) {
			timer->expires.sched = 0;
		} else {
			timer->expires.sched -= sum_exec_runtime;
		}
	}
}

void posix_cpu_timers_exit(struct task_struct *tsk)
{
	cleanup_timers(tsk->cpu_timers,
		       tsk->utime, tsk->stime, tsk->se.sum_exec_runtime);

}
void posix_cpu_timers_exit_group(struct task_struct *tsk)
{
	struct signal_struct *const sig = tsk->signal;

	cleanup_timers(tsk->signal->cpu_timers,
		       tsk->utime + sig->utime, tsk->stime + sig->stime,
		       tsk->se.sum_exec_runtime + sig->sum_sched_runtime);
}

static void clear_dead_task(struct k_itimer *timer, union cpu_time_count now)
{
	put_task_struct(timer->it.cpu.task);
	timer->it.cpu.task = NULL;
	timer->it.cpu.expires = cpu_time_sub(timer->it_clock,
					     timer->it.cpu.expires,
					     now);
}

static inline int expires_gt(cputime_t expires, cputime_t new_exp)
{
	return expires == 0 || expires > new_exp;
}

static void arm_timer(struct k_itimer *timer)
{
	struct task_struct *p = timer->it.cpu.task;
	struct list_head *head, *listpos;
	struct task_cputime *cputime_expires;
	struct cpu_timer_list *const nt = &timer->it.cpu;
	struct cpu_timer_list *next;

	if (CPUCLOCK_PERTHREAD(timer->it_clock)) {
		head = p->cpu_timers;
		cputime_expires = &p->cputime_expires;
	} else {
		head = p->signal->cpu_timers;
		cputime_expires = &p->signal->cputime_expires;
	}
	head += CPUCLOCK_WHICH(timer->it_clock);

	listpos = head;
	list_for_each_entry(next, head, entry) {
		if (cpu_time_before(timer->it_clock, nt->expires, next->expires))
			break;
		listpos = &next->entry;
	}
	list_add(&nt->entry, listpos);

	if (listpos == head) {
		union cpu_time_count *exp = &nt->expires;


		switch (CPUCLOCK_WHICH(timer->it_clock)) {
		case CPUCLOCK_PROF:
			if (expires_gt(cputime_expires->prof_exp, exp->cpu))
				cputime_expires->prof_exp = exp->cpu;
			break;
		case CPUCLOCK_VIRT:
			if (expires_gt(cputime_expires->virt_exp, exp->cpu))
				cputime_expires->virt_exp = exp->cpu;
			break;
		case CPUCLOCK_SCHED:
			if (cputime_expires->sched_exp == 0 ||
			    cputime_expires->sched_exp > exp->sched)
				cputime_expires->sched_exp = exp->sched;
			break;
		}
	}
}

static void cpu_timer_fire(struct k_itimer *timer)
{
	if ((timer->it_sigev_notify & ~SIGEV_THREAD_ID) == SIGEV_NONE) {
		timer->it.cpu.expires.sched = 0;
	} else if (unlikely(timer->sigq == NULL)) {
		wake_up_process(timer->it_process);
		timer->it.cpu.expires.sched = 0;
	} else if (timer->it.cpu.incr.sched == 0) {
		posix_timer_event(timer, 0);
		timer->it.cpu.expires.sched = 0;
	} else if (posix_timer_event(timer, ++timer->it_requeue_pending)) {
		posix_cpu_timer_schedule(timer);
	}
}

static int cpu_timer_sample_group(const clockid_t which_clock,
				  struct task_struct *p,
				  union cpu_time_count *cpu)
{
	struct task_cputime cputime;

	thread_group_cputimer(p, &cputime);
	switch (CPUCLOCK_WHICH(which_clock)) {
	default:
		return -EINVAL;
	case CPUCLOCK_PROF:
		cpu->cpu = cputime.utime + cputime.stime;
		break;
	case CPUCLOCK_VIRT:
		cpu->cpu = cputime.utime;
		break;
	case CPUCLOCK_SCHED:
		cpu->sched = cputime.sum_exec_runtime + task_delta_exec(p);
		break;
	}
	return 0;
}

static int posix_cpu_timer_set(struct k_itimer *timer, int flags,
			       struct itimerspec *new, struct itimerspec *old)
{
	struct task_struct *p = timer->it.cpu.task;
	union cpu_time_count old_expires, new_expires, old_incr, val;
	int ret;

	if (unlikely(p == NULL)) {
		return -ESRCH;
	}

	new_expires = timespec_to_sample(timer->it_clock, &new->it_value);

	read_lock(&tasklist_lock);
	if (unlikely(p->sighand == NULL)) {
		read_unlock(&tasklist_lock);
		put_task_struct(p);
		timer->it.cpu.task = NULL;
		return -ESRCH;
	}

	BUG_ON(!irqs_disabled());

	ret = 0;
	old_incr = timer->it.cpu.incr;
	spin_lock(&p->sighand->siglock);
	old_expires = timer->it.cpu.expires;
	if (unlikely(timer->it.cpu.firing)) {
		timer->it.cpu.firing = -1;
		ret = TIMER_RETRY;
	} else
		list_del_init(&timer->it.cpu.entry);

	if (CPUCLOCK_PERTHREAD(timer->it_clock)) {
		cpu_clock_sample(timer->it_clock, p, &val);
	} else {
		cpu_timer_sample_group(timer->it_clock, p, &val);
	}

	if (old) {
		if (old_expires.sched == 0) {
			old->it_value.tv_sec = 0;
			old->it_value.tv_nsec = 0;
		} else {
			bump_cpu_timer(timer, val);
			if (cpu_time_before(timer->it_clock, val,
					    timer->it.cpu.expires)) {
				old_expires = cpu_time_sub(
					timer->it_clock,
					timer->it.cpu.expires, val);
				sample_to_timespec(timer->it_clock,
						   old_expires,
						   &old->it_value);
			} else {
				old->it_value.tv_nsec = 1;
				old->it_value.tv_sec = 0;
			}
		}
	}

	if (unlikely(ret)) {
		spin_unlock(&p->sighand->siglock);
		read_unlock(&tasklist_lock);
		goto out;
	}

	if (new_expires.sched != 0 && !(flags & TIMER_ABSTIME)) {
		cpu_time_add(timer->it_clock, &new_expires, val);
	}

	timer->it.cpu.expires = new_expires;
	if (new_expires.sched != 0 &&
	    cpu_time_before(timer->it_clock, val, new_expires)) {
		arm_timer(timer);
	}

	spin_unlock(&p->sighand->siglock);
	read_unlock(&tasklist_lock);

	timer->it.cpu.incr = timespec_to_sample(timer->it_clock,
						&new->it_interval);

	timer->it_requeue_pending = (timer->it_requeue_pending + 2) &
		~REQUEUE_PENDING;
	timer->it_overrun_last = 0;
	timer->it_overrun = -1;

	if (new_expires.sched != 0 &&
	    !cpu_time_before(timer->it_clock, val, new_expires)) {
		cpu_timer_fire(timer);
	}

	ret = 0;
 out:
	if (old) {
		sample_to_timespec(timer->it_clock,
				   old_incr, &old->it_interval);
	}
	return ret;
}

static void posix_cpu_timer_get(struct k_itimer *timer, struct itimerspec *itp)
{
	union cpu_time_count now;
	struct task_struct *p = timer->it.cpu.task;
	int clear_dead;

	sample_to_timespec(timer->it_clock,
			   timer->it.cpu.incr, &itp->it_interval);

	if (timer->it.cpu.expires.sched == 0) {	
		itp->it_value.tv_sec = itp->it_value.tv_nsec = 0;
		return;
	}

	if (unlikely(p == NULL)) {
	dead:
		sample_to_timespec(timer->it_clock, timer->it.cpu.expires,
				   &itp->it_value);
		return;
	}

	if (CPUCLOCK_PERTHREAD(timer->it_clock)) {
		cpu_clock_sample(timer->it_clock, p, &now);
		clear_dead = p->exit_state;
	} else {
		read_lock(&tasklist_lock);
		if (unlikely(p->sighand == NULL)) {
			put_task_struct(p);
			timer->it.cpu.task = NULL;
			timer->it.cpu.expires.sched = 0;
			read_unlock(&tasklist_lock);
			goto dead;
		} else {
			cpu_timer_sample_group(timer->it_clock, p, &now);
			clear_dead = (unlikely(p->exit_state) &&
				      thread_group_empty(p));
		}
		read_unlock(&tasklist_lock);
	}

	if (unlikely(clear_dead)) {
		clear_dead_task(timer, now);
		goto dead;
	}

	if (cpu_time_before(timer->it_clock, now, timer->it.cpu.expires)) {
		sample_to_timespec(timer->it_clock,
				   cpu_time_sub(timer->it_clock,
						timer->it.cpu.expires, now),
				   &itp->it_value);
	} else {
		itp->it_value.tv_nsec = 1;
		itp->it_value.tv_sec = 0;
	}
}

static void check_thread_timers(struct task_struct *tsk,
				struct list_head *firing)
{
	int maxfire;
	struct list_head *timers = tsk->cpu_timers;
	struct signal_struct *const sig = tsk->signal;
	unsigned long soft;

	maxfire = 20;
	tsk->cputime_expires.prof_exp = 0;
	while (!list_empty(timers)) {
		struct cpu_timer_list *t = list_first_entry(timers,
						      struct cpu_timer_list,
						      entry);
		if (!--maxfire || prof_ticks(tsk) < t->expires.cpu) {
			tsk->cputime_expires.prof_exp = t->expires.cpu;
			break;
		}
		t->firing = 1;
		list_move_tail(&t->entry, firing);
	}

	++timers;
	maxfire = 20;
	tsk->cputime_expires.virt_exp = 0;
	while (!list_empty(timers)) {
		struct cpu_timer_list *t = list_first_entry(timers,
						      struct cpu_timer_list,
						      entry);
		if (!--maxfire || virt_ticks(tsk) < t->expires.cpu) {
			tsk->cputime_expires.virt_exp = t->expires.cpu;
			break;
		}
		t->firing = 1;
		list_move_tail(&t->entry, firing);
	}

	++timers;
	maxfire = 20;
	tsk->cputime_expires.sched_exp = 0;
	while (!list_empty(timers)) {
		struct cpu_timer_list *t = list_first_entry(timers,
						      struct cpu_timer_list,
						      entry);
		if (!--maxfire || tsk->se.sum_exec_runtime < t->expires.sched) {
			tsk->cputime_expires.sched_exp = t->expires.sched;
			break;
		}
		t->firing = 1;
		list_move_tail(&t->entry, firing);
	}

	soft = ACCESS_ONCE(sig->rlim[RLIMIT_RTTIME].rlim_cur);
	if (soft != RLIM_INFINITY) {
		unsigned long hard =
			ACCESS_ONCE(sig->rlim[RLIMIT_RTTIME].rlim_max);

		if (hard != RLIM_INFINITY &&
		    tsk->rt.timeout > DIV_ROUND_UP(hard, USEC_PER_SEC/HZ)) {
			__group_send_sig_info(SIGKILL, SEND_SIG_PRIV, tsk);
			return;
		}
		if (tsk->rt.timeout > DIV_ROUND_UP(soft, USEC_PER_SEC/HZ)) {
			if (soft < hard) {
				soft += USEC_PER_SEC;
				sig->rlim[RLIMIT_RTTIME].rlim_cur = soft;
			}
			printk(KERN_INFO
				"RT Watchdog Timeout: %s[%d]\n",
				tsk->comm, task_pid_nr(tsk));
			__group_send_sig_info(SIGXCPU, SEND_SIG_PRIV, tsk);
		}
	}
}

static void stop_process_timers(struct signal_struct *sig)
{
	struct thread_group_cputimer *cputimer = &sig->cputimer;
	unsigned long flags;

	raw_spin_lock_irqsave(&cputimer->lock, flags);
	cputimer->running = 0;
	raw_spin_unlock_irqrestore(&cputimer->lock, flags);
}

static u32 onecputick;

static void check_cpu_itimer(struct task_struct *tsk, struct cpu_itimer *it,
			     cputime_t *expires, cputime_t cur_time, int signo)
{
	if (!it->expires)
		return;

	if (cur_time >= it->expires) {
		if (it->incr) {
			it->expires += it->incr;
			it->error += it->incr_error;
			if (it->error >= onecputick) {
				it->expires -= cputime_one_jiffy;
				it->error -= onecputick;
			}
		} else {
			it->expires = 0;
		}

		trace_itimer_expire(signo == SIGPROF ?
				    ITIMER_PROF : ITIMER_VIRTUAL,
				    tsk->signal->leader_pid, cur_time);
		__group_send_sig_info(signo, SEND_SIG_PRIV, tsk);
	}

	if (it->expires && (!*expires || it->expires < *expires)) {
		*expires = it->expires;
	}
}

static inline int task_cputime_zero(const struct task_cputime *cputime)
{
	if (!cputime->utime && !cputime->stime && !cputime->sum_exec_runtime)
		return 1;
	return 0;
}

static void check_process_timers(struct task_struct *tsk,
				 struct list_head *firing)
{
	int maxfire;
	struct signal_struct *const sig = tsk->signal;
	cputime_t utime, ptime, virt_expires, prof_expires;
	unsigned long long sum_sched_runtime, sched_expires;
	struct list_head *timers = sig->cpu_timers;
	struct task_cputime cputime;
	unsigned long soft;

	thread_group_cputimer(tsk, &cputime);
	utime = cputime.utime;
	ptime = utime + cputime.stime;
	sum_sched_runtime = cputime.sum_exec_runtime;
	maxfire = 20;
	prof_expires = 0;
	while (!list_empty(timers)) {
		struct cpu_timer_list *tl = list_first_entry(timers,
						      struct cpu_timer_list,
						      entry);
		if (!--maxfire || ptime < tl->expires.cpu) {
			prof_expires = tl->expires.cpu;
			break;
		}
		tl->firing = 1;
		list_move_tail(&tl->entry, firing);
	}

	++timers;
	maxfire = 20;
	virt_expires = 0;
	while (!list_empty(timers)) {
		struct cpu_timer_list *tl = list_first_entry(timers,
						      struct cpu_timer_list,
						      entry);
		if (!--maxfire || utime < tl->expires.cpu) {
			virt_expires = tl->expires.cpu;
			break;
		}
		tl->firing = 1;
		list_move_tail(&tl->entry, firing);
	}

	++timers;
	maxfire = 20;
	sched_expires = 0;
	while (!list_empty(timers)) {
		struct cpu_timer_list *tl = list_first_entry(timers,
						      struct cpu_timer_list,
						      entry);
		if (!--maxfire || sum_sched_runtime < tl->expires.sched) {
			sched_expires = tl->expires.sched;
			break;
		}
		tl->firing = 1;
		list_move_tail(&tl->entry, firing);
	}

	check_cpu_itimer(tsk, &sig->it[CPUCLOCK_PROF], &prof_expires, ptime,
			 SIGPROF);
	check_cpu_itimer(tsk, &sig->it[CPUCLOCK_VIRT], &virt_expires, utime,
			 SIGVTALRM);
	soft = ACCESS_ONCE(sig->rlim[RLIMIT_CPU].rlim_cur);
	if (soft != RLIM_INFINITY) {
		unsigned long psecs = cputime_to_secs(ptime);
		unsigned long hard =
			ACCESS_ONCE(sig->rlim[RLIMIT_CPU].rlim_max);
		cputime_t x;
		if (psecs >= hard) {
			__group_send_sig_info(SIGKILL, SEND_SIG_PRIV, tsk);
			return;
		}
		if (psecs >= soft) {
			__group_send_sig_info(SIGXCPU, SEND_SIG_PRIV, tsk);
			if (soft < hard) {
				soft++;
				sig->rlim[RLIMIT_CPU].rlim_cur = soft;
			}
		}
		x = secs_to_cputime(soft);
		if (!prof_expires || x < prof_expires) {
			prof_expires = x;
		}
	}

	sig->cputime_expires.prof_exp = prof_expires;
	sig->cputime_expires.virt_exp = virt_expires;
	sig->cputime_expires.sched_exp = sched_expires;
	if (task_cputime_zero(&sig->cputime_expires))
		stop_process_timers(sig);
}

void posix_cpu_timer_schedule(struct k_itimer *timer)
{
	struct task_struct *p = timer->it.cpu.task;
	union cpu_time_count now;

	if (unlikely(p == NULL))
		goto out;

	if (CPUCLOCK_PERTHREAD(timer->it_clock)) {
		cpu_clock_sample(timer->it_clock, p, &now);
		bump_cpu_timer(timer, now);
		if (unlikely(p->exit_state)) {
			clear_dead_task(timer, now);
			goto out;
		}
		read_lock(&tasklist_lock); 
		spin_lock(&p->sighand->siglock);
	} else {
		read_lock(&tasklist_lock);
		if (unlikely(p->sighand == NULL)) {
			put_task_struct(p);
			timer->it.cpu.task = p = NULL;
			timer->it.cpu.expires.sched = 0;
			goto out_unlock;
		} else if (unlikely(p->exit_state) && thread_group_empty(p)) {
			clear_dead_task(timer, now);
			goto out_unlock;
		}
		spin_lock(&p->sighand->siglock);
		cpu_timer_sample_group(timer->it_clock, p, &now);
		bump_cpu_timer(timer, now);
		
	}

	BUG_ON(!irqs_disabled());
	arm_timer(timer);
	spin_unlock(&p->sighand->siglock);

out_unlock:
	read_unlock(&tasklist_lock);

out:
	timer->it_overrun_last = timer->it_overrun;
	timer->it_overrun = -1;
	++timer->it_requeue_pending;
}

static inline int task_cputime_expired(const struct task_cputime *sample,
					const struct task_cputime *expires)
{
	if (expires->utime && sample->utime >= expires->utime)
		return 1;
	if (expires->stime && sample->utime + sample->stime >= expires->stime)
		return 1;
	if (expires->sum_exec_runtime != 0 &&
	    sample->sum_exec_runtime >= expires->sum_exec_runtime)
		return 1;
	return 0;
}

static inline int fastpath_timer_check(struct task_struct *tsk)
{
	struct signal_struct *sig;

	if (!task_cputime_zero(&tsk->cputime_expires)) {
		struct task_cputime task_sample = {
			.utime = tsk->utime,
			.stime = tsk->stime,
			.sum_exec_runtime = tsk->se.sum_exec_runtime
		};

		if (task_cputime_expired(&task_sample, &tsk->cputime_expires))
			return 1;
	}

	sig = tsk->signal;
	if (sig->cputimer.running) {
		struct task_cputime group_sample;

		raw_spin_lock(&sig->cputimer.lock);
		group_sample = sig->cputimer.cputime;
		raw_spin_unlock(&sig->cputimer.lock);

		if (task_cputime_expired(&group_sample, &sig->cputime_expires))
			return 1;
	}

	return 0;
}

void run_posix_cpu_timers(struct task_struct *tsk)
{
	LIST_HEAD(firing);
	struct k_itimer *timer, *next;
	unsigned long flags;

	BUG_ON(!irqs_disabled());

	if (!fastpath_timer_check(tsk))
		return;

	if (!lock_task_sighand(tsk, &flags))
		return;
	check_thread_timers(tsk, &firing);
	if (tsk->signal->cputimer.running)
		check_process_timers(tsk, &firing);

	unlock_task_sighand(tsk, &flags);

	list_for_each_entry_safe(timer, next, &firing, it.cpu.entry) {
		int cpu_firing;

		spin_lock(&timer->it_lock);
		list_del_init(&timer->it.cpu.entry);
		cpu_firing = timer->it.cpu.firing;
		timer->it.cpu.firing = 0;
		if (likely(cpu_firing >= 0))
			cpu_timer_fire(timer);
		spin_unlock(&timer->it_lock);
	}
}

void set_process_cpu_timer(struct task_struct *tsk, unsigned int clock_idx,
			   cputime_t *newval, cputime_t *oldval)
{
	union cpu_time_count now;

	BUG_ON(clock_idx == CPUCLOCK_SCHED);
	cpu_timer_sample_group(clock_idx, tsk, &now);

	if (oldval) {
		if (*oldval) {
			if (*oldval <= now.cpu) {
				
				*oldval = cputime_one_jiffy;
			} else {
				*oldval -= now.cpu;
			}
		}

		if (!*newval)
			return;
		*newval += now.cpu;
	}

	switch (clock_idx) {
	case CPUCLOCK_PROF:
		if (expires_gt(tsk->signal->cputime_expires.prof_exp, *newval))
			tsk->signal->cputime_expires.prof_exp = *newval;
		break;
	case CPUCLOCK_VIRT:
		if (expires_gt(tsk->signal->cputime_expires.virt_exp, *newval))
			tsk->signal->cputime_expires.virt_exp = *newval;
		break;
	}
}

static int do_cpu_nanosleep(const clockid_t which_clock, int flags,
			    struct timespec *rqtp, struct itimerspec *it)
{
	struct k_itimer timer;
	int error;

	memset(&timer, 0, sizeof timer);
	spin_lock_init(&timer.it_lock);
	timer.it_clock = which_clock;
	timer.it_overrun = -1;
	error = posix_cpu_timer_create(&timer);
	timer.it_process = current;
	if (!error) {
		static struct itimerspec zero_it;

		memset(it, 0, sizeof *it);
		it->it_value = *rqtp;

		spin_lock_irq(&timer.it_lock);
		error = posix_cpu_timer_set(&timer, flags, it, NULL);
		if (error) {
			spin_unlock_irq(&timer.it_lock);
			return error;
		}

		while (!signal_pending(current)) {
			if (timer.it.cpu.expires.sched == 0) {
				spin_unlock_irq(&timer.it_lock);
				return 0;
			}

			__set_current_state(TASK_INTERRUPTIBLE);
			spin_unlock_irq(&timer.it_lock);
			schedule();
			spin_lock_irq(&timer.it_lock);
		}

		sample_to_timespec(which_clock, timer.it.cpu.expires, rqtp);
		posix_cpu_timer_set(&timer, 0, &zero_it, it);
		spin_unlock_irq(&timer.it_lock);

		if ((it->it_value.tv_sec | it->it_value.tv_nsec) == 0) {
			return 0;
		}

		error = -ERESTART_RESTARTBLOCK;
	}

	return error;
}

static long posix_cpu_nsleep_restart(struct restart_block *restart_block);

static int posix_cpu_nsleep(const clockid_t which_clock, int flags,
			    struct timespec *rqtp, struct timespec __user *rmtp)
{
	struct restart_block *restart_block =
		&current_thread_info()->restart_block;
	struct itimerspec it;
	int error;

	if (CPUCLOCK_PERTHREAD(which_clock) &&
	    (CPUCLOCK_PID(which_clock) == 0 ||
	     CPUCLOCK_PID(which_clock) == current->pid))
		return -EINVAL;

	error = do_cpu_nanosleep(which_clock, flags, rqtp, &it);

	if (error == -ERESTART_RESTARTBLOCK) {

		if (flags & TIMER_ABSTIME)
			return -ERESTARTNOHAND;
		if (rmtp && copy_to_user(rmtp, &it.it_value, sizeof *rmtp))
			return -EFAULT;

		restart_block->fn = posix_cpu_nsleep_restart;
		restart_block->nanosleep.clockid = which_clock;
		restart_block->nanosleep.rmtp = rmtp;
		restart_block->nanosleep.expires = timespec_to_ns(rqtp);
	}
	return error;
}

static long posix_cpu_nsleep_restart(struct restart_block *restart_block)
{
	clockid_t which_clock = restart_block->nanosleep.clockid;
	struct timespec t;
	struct itimerspec it;
	int error;

	t = ns_to_timespec(restart_block->nanosleep.expires);

	error = do_cpu_nanosleep(which_clock, TIMER_ABSTIME, &t, &it);

	if (error == -ERESTART_RESTARTBLOCK) {
		struct timespec __user *rmtp = restart_block->nanosleep.rmtp;
		if (rmtp && copy_to_user(rmtp, &it.it_value, sizeof *rmtp))
			return -EFAULT;

		restart_block->nanosleep.expires = timespec_to_ns(&t);
	}
	return error;

}

#define PROCESS_CLOCK	MAKE_PROCESS_CPUCLOCK(0, CPUCLOCK_SCHED)
#define THREAD_CLOCK	MAKE_THREAD_CPUCLOCK(0, CPUCLOCK_SCHED)

static int process_cpu_clock_getres(const clockid_t which_clock,
				    struct timespec *tp)
{
	return posix_cpu_clock_getres(PROCESS_CLOCK, tp);
}
static int process_cpu_clock_get(const clockid_t which_clock,
				 struct timespec *tp)
{
	return posix_cpu_clock_get(PROCESS_CLOCK, tp);
}
static int process_cpu_timer_create(struct k_itimer *timer)
{
	timer->it_clock = PROCESS_CLOCK;
	return posix_cpu_timer_create(timer);
}
static int process_cpu_nsleep(const clockid_t which_clock, int flags,
			      struct timespec *rqtp,
			      struct timespec __user *rmtp)
{
	return posix_cpu_nsleep(PROCESS_CLOCK, flags, rqtp, rmtp);
}
static long process_cpu_nsleep_restart(struct restart_block *restart_block)
{
	return -EINVAL;
}
static int thread_cpu_clock_getres(const clockid_t which_clock,
				   struct timespec *tp)
{
	return posix_cpu_clock_getres(THREAD_CLOCK, tp);
}
static int thread_cpu_clock_get(const clockid_t which_clock,
				struct timespec *tp)
{
	return posix_cpu_clock_get(THREAD_CLOCK, tp);
}
static int thread_cpu_timer_create(struct k_itimer *timer)
{
	timer->it_clock = THREAD_CLOCK;
	return posix_cpu_timer_create(timer);
}

struct k_clock clock_posix_cpu = {
	.clock_getres	= posix_cpu_clock_getres,
	.clock_set	= posix_cpu_clock_set,
	.clock_get	= posix_cpu_clock_get,
	.timer_create	= posix_cpu_timer_create,
	.nsleep		= posix_cpu_nsleep,
	.nsleep_restart	= posix_cpu_nsleep_restart,
	.timer_set	= posix_cpu_timer_set,
	.timer_del	= posix_cpu_timer_del,
	.timer_get	= posix_cpu_timer_get,
};

static __init int init_posix_cpu_timers(void)
{
	struct k_clock process = {
		.clock_getres	= process_cpu_clock_getres,
		.clock_get	= process_cpu_clock_get,
		.timer_create	= process_cpu_timer_create,
		.nsleep		= process_cpu_nsleep,
		.nsleep_restart	= process_cpu_nsleep_restart,
	};
	struct k_clock thread = {
		.clock_getres	= thread_cpu_clock_getres,
		.clock_get	= thread_cpu_clock_get,
		.timer_create	= thread_cpu_timer_create,
	};
	struct timespec ts;

	posix_timers_register_clock(CLOCK_PROCESS_CPUTIME_ID, &process);
	posix_timers_register_clock(CLOCK_THREAD_CPUTIME_ID, &thread);

	cputime_to_timespec(cputime_one_jiffy, &ts);
	onecputick = ts.tv_nsec;
	WARN_ON(ts.tv_sec != 0);

	return 0;
}
__initcall(init_posix_cpu_timers);
