#include <mach/board.h>

#include "kgsl_htc.h"

void
kgsl_dump_contextpid_locked(struct idr *context_idr)
{
	int i = 0;
	struct kgsl_context *context;
	struct task_struct *task;
	struct task_struct *parent_task;
	char task_name[TASK_COMM_LEN+1];
	char task_parent_name[TASK_COMM_LEN+1];
	pid_t ppid;

	printk(" == [KGSL] context maximal count is %d, dump context id, pid, name, group leader name ==\n",KGSL_MEMSTORE_MAX);
	for (i = 0; i <KGSL_MEMSTORE_MAX; i++) {

		context = idr_find(context_idr, i);

		if (context  && context->dev_priv &&  context->dev_priv->process_priv) {
			task = find_task_by_pid_ns(context->dev_priv->process_priv->pid, &init_pid_ns);
			if (!task) {
				printk("can't find context's task: context id %d\n", context->id);
				continue;
			}
			parent_task = task->group_leader;
			get_task_comm(task_name, task);

			if (parent_task) {
				get_task_comm(task_parent_name, parent_task);
				ppid = context->dev_priv->process_priv->pid;
			} else {
				task_parent_name[0] = '\0';
				ppid = 0;
			}

			printk("context id=%d\t\t pid=%d\t\t %s\t\t %s\n", context->id, ppid, task_name, task_parent_name);
		}
	}
}

