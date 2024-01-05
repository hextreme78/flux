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

int sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
	if (ruid) {
		if (copy_to_user(ruid, &curproc()->ruid, sizeof(uid_t))) {
			return -EFAULT;
		}
	}
	if (euid) {
		if (copy_to_user(euid, &curproc()->euid, sizeof(uid_t))) {
			return -EFAULT;
		}
	}
	if (suid) {
		if (copy_to_user(suid, &curproc()->suid, sizeof(uid_t))) {
			return -EFAULT;
		}
	}
	return 0;
}

int sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
	if (rgid) {
		if (copy_to_user(rgid, &curproc()->rgid, sizeof(gid_t))) {
			return -EFAULT;
		}
	}
	if (egid) {
		if (copy_to_user(egid, &curproc()->egid, sizeof(gid_t))) {
			return -EFAULT;
		}
	}
	if (sgid) {
		if (copy_to_user(sgid, &curproc()->sgid, sizeof(gid_t))) {
			return -EFAULT;
		}
	}
	return 0;
}

int sys_setuid(uid_t uid)
{
	if (!curproc()->euid || !curproc()->egid) {
		curproc()->ruid = curproc()->euid = curproc()->suid = uid;
	} else if (uid == curproc()->ruid ||
			uid == curproc()->euid ||
			uid == curproc()->suid) {
		curproc()->euid = uid;
	}
	return -EPERM;
}

int sys_setgid(gid_t gid)
{
	if (!curproc()->euid || !curproc()->egid) {
		curproc()->rgid = curproc()->egid = curproc()->sgid = gid;
	} else if (gid == curproc()->rgid ||
			gid == curproc()->egid ||
			gid == curproc()->sgid) {
		curproc()->egid = gid;
	}
	return -EPERM;
}

int sys_setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
	uid_t curruid = curproc()->ruid,
		cureuid = curproc()->euid,
		cursuid = curproc()->suid;
	if (ruid != -1) {
		if (!curproc()->euid || !curproc()->egid ||
				ruid == curruid ||
				ruid == cureuid ||
				ruid == cursuid) {
			curproc()->ruid = ruid;
		} else {
			return -EPERM;
		}
	}
	if (euid != -1) {
		if (!curproc()->euid || !curproc()->egid ||
				euid == curruid ||
				euid == cureuid ||
				euid == cursuid) {
			curproc()->euid = euid;
		} else {
			curproc()->ruid = curruid;
			return -EPERM;
		}
	}
	if (suid != -1) {
		if (!curproc()->euid || !curproc()->egid ||
				suid == curruid ||
				suid == cureuid ||
				suid == cursuid) {
			curproc()->suid = suid;
		} else {
			curproc()->ruid = curruid;
			curproc()->euid = cureuid;
			return -EPERM;
		}
	}
	return 0;
}

int sys_setresgid(gid_t rgid, gid_t egid, uid_t sgid)
{
	gid_t currgid = curproc()->rgid,
		curegid = curproc()->egid,
		cursgid = curproc()->sgid;
	if (rgid != -1) {
		if (!curproc()->euid || !curproc()->egid ||
				rgid == currgid ||
				rgid == curegid ||
				rgid == cursgid) {
			curproc()->rgid = rgid;
		} else {
			return -EPERM;
		}
	}
	if (egid != -1) {
		if (!curproc()->euid || !curproc()->egid ||
				egid == currgid ||
				egid == curegid ||
				egid == cursgid) {
			curproc()->egid = egid;
		} else {
			curproc()->rgid = currgid;
			return -EPERM;
		}
	}
	if (sgid != -1) {
		if (!curproc()->euid || !curproc()->egid ||
				sgid == currgid ||
				sgid == curegid ||
				sgid == cursgid) {
			curproc()->sgid = sgid;
		} else {
			curproc()->rgid = currgid;
			curproc()->egid = curegid;
			return -EPERM;
		}
	}
	return 0;
}

pid_t sys_getsid(pid_t pid)
{
	/* not implemented */
	return 0;
}

pid_t sys_setsid(void)
{
	int irqflags, ret;
	pid_t sid;
	spinlock_acquire_irqsave(&curproc()->lock, irqflags);
	if (curproc()->pid == curproc()->sid) {
		ret = -1;
	} else {
		ret = curproc()->sid = curproc()->pid;
	}
	spinlock_release_irqrestore(&curproc()->lock, irqflags);
	return ret;
}

pid_t sys_getpgid(pid_t pid)
{
	/* not implemented */
	return 0;
}

int sys_setpgid(pid_t pid, pid_t pgid)
{
	/* not implemented */
	return 0;
}

pid_t sys_gettid(void)
{
	return sys_getpid();
}

