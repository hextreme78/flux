#include <kernel/sysproc.h>
#include <kernel/proc.h>
#include <kernel/riscv64.h>

void sys_exit(int status)
{
	curproc()->exit_status = status;

	spinlock_acquire(&curproc()->lock);
	curproc()->state = PROC_STATE_ZOMBIE;
	spinlock_release(&curproc()->lock);
}

pid_t sys_waitpid(pid_t pid, int *status, int options)
{
	if (pid > 0) {
		proc_t *child = NULL;
		
		list_for_each_entry (child, curproc(), children) {
			if (child->pid == pid) {
				break;
			}
		}
		
		if (!child) {
			return -1;
		}

		spinlock_acquire(&child->lock);
		while (child->state != PROC_STATE_ZOMBIE) {
			spinlock_release(&child->lock);

			/* replace with switch to scheduler */
			nop();

			spinlock_acquire(&child->lock);
		}
		spinlock_release(&child->lock);
		
		proc_destroy(child);

		return pid;
	
	} else if (pid == 0) {
		return -1;
	} else {
		return -1;
	}

	return 0;
}

