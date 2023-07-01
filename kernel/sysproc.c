#include <kernel/sysproc.h>
#include <kernel/sched.h>
#include <kernel/klib.h>
#include <kernel/errno.h>

void sys__exit(int status)
{
	curproc()->exit_status = status;
	zombie();
}

u64 sys_wait(int *status)
{
	u64 ret;
	proc_t *child = NULL;

	/* if process does not have children return error */
	if (list_empty(&curproc()->children)) {
		return -ECHILD;
	}

	child = list_next_entry(curproc(), children);

	spinlock_acquire(&child->lock);	

	/* wait for zombie child */
	while (child->state != PROC_STATE_ZOMBIE) {
		spinlock_release(&child->lock);

		//wchan_sleep();

		spinlock_acquire(&child->lock);	
	}

	/* copy exit status to user memory */
	if (copy_to_user(status, &child->exit_status, sizeof(int)) < 0) {
		spinlock_release(&child->lock);
		return -EINVAL;
	}

	/* save child pid before proc_destroy */
	ret = child->pid;

	proc_destroy(child);

	spinlock_release(&child->lock);

	return ret;
}

u64 sys_getpid(void)
{
	pid_t pid;
	spinlock_acquire(&curproc()->lock);
	pid = curproc()->pid;
	spinlock_release(&curproc()->lock);
	return pid;
}

