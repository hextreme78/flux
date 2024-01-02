#include <kernel/sysproc.h>
#include <kernel/sched.h>
#include <kernel/klib.h>
#include <kernel/errno.h>

void sys_exit(int status)
{
	curproc()->exit_status = status;
	sched_zombie();
}

pid_t sys_getpid(void)
{
	int irqflags;
	pid_t pid;
	spinlock_acquire_irqsave(&curproc()->lock, irqflags);
	pid = curproc()->pid;
	spinlock_release_irqrestore(&curproc()->lock, irqflags);
	return pid;
}

pid_t sys_getppid(void)
{
	/* not implemented */
	return 0;
}

int sys_sleep(const struct timespec *req, struct timespec *rem)
{
	/* not implemented */
	return 0;
}

pid_t sys_fork(void)
{
	/* not implemented */
	return 0;
}

int sys_execve(const char *pathname, char *const argv[], char *const envp[])
{
	/* not implemented */
	return 0;
}

int sys_uname(struct utsname *buf)
{
	/* not implemented */
	return 0;
}

pid_t sys_wait4(pid_t pid, int *status, int options, struct rusage *rusage)
{
	int irqflags;
	u64 ret;
	proc_t *child = NULL;

	/* if process does not have children return error */
	if (list_empty(&curproc()->children)) {
		return -ECHILD;
	}

	child = list_next_entry(curproc(), children);

	spinlock_acquire_irqsave(&child->lock, irqflags);	

	/* wait for zombie child */
	while (child->state != PROC_STATE_ZOMBIE) {
		spinlock_release_irqrestore(&child->lock, irqflags);

		sched();

		spinlock_acquire_irqsave(&child->lock, irqflags);	
	}

	/* copy exit status to user memory */
	if (copy_to_user(status, &child->exit_status, sizeof(int)) < 0) {
		spinlock_release_irqrestore(&child->lock, irqflags);
		return -EINVAL;
	}

	/* save child pid before proc_destroy */
	ret = child->pid;

	proc_destroy(child);

	spinlock_release_irqrestore(&child->lock, irqflags);

	return ret;

}

int sys_brk(void *addr)
{
	/* not implemented */
	return 0;
}

uid_t sys_getuid(void)
{
	return curproc()->uid;
}

uid_t sys_geteuid(void)
{
	return curproc()->euid;
}

gid_t sys_getgid(void)
{
	return curproc()->gid;
}

gid_t sys_getegid(void)
{
	return curproc()->egid;
}

