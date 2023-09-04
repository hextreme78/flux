#include <kernel/syscall.h>
#include <kernel/riscv64.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/sysproc.h>
#include <kernel/sysfs.h>

#include <kernel/ext2.h>

void syscall(void)
{
	u64 ret = 0;
	u64 sysnum;
	trapframe_t *tf;

	tf = curproc()->trapframe;

	/* get syscall number from a7 register */
	sysnum = tf->a7;

	switch (sysnum) {
	case SYS_exit:
		/* never returns */
		sys_exit(tf->a0);

	case SYS_close:
		ret = sys_close(tf->a0);
		break;

	case SYS_execve:

		break;

	case SYS_fork:
		break;

	case SYS_fstat:
		ret = sys_fstat(tf->a0, (void *) tf->a1);
		break;

	case SYS_getpid:
		ret = sys_getpid();
		break;

	case SYS_isatty:

		break;

	case SYS_kill:
		break;

	case SYS_link:
		ret = sys_link((void *) tf->a0, (void *) tf->a1);
		break;

	case SYS_lseek:
		ret = sys_lseek(tf->a0, tf->a1, tf->a2);
		break;

	case SYS_open:
		ret = sys_open((void *) tf->a0, tf->a1, tf->a2);
		break;

	case SYS_read:
		ret = sys_read(tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_sbrk:
		break;

	case SYS_times:
		break;

	case SYS_unlink:
		ret = sys_unlink((void *) tf->a0);
		break;

	case SYS_wait:
		ret = sys_wait((void *) tf->a0);
		break;

	case SYS_write:
		ret = sys_write(tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_fchdir:
		ret = sys_fchdir(tf->a0);
		break;

	case SYS_mkdir:
		ret = sys_mkdir((void *) tf->a0, tf->a1);
		break;

	case SYS_readlink:
		ret = sys_readlink((void *) tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_rename:
		ret = sys_rename((void *) tf->a0, (void *) tf->a1);
		break;

	case SYS_rmdir:
		ret = sys_rmdir((void *) tf->a0);
		break;

	case SYS_symlink:
		ret = sys_symlink((void *) tf->a0, (void *) tf->a1);
		break;

	case SYS_ftruncate:
		ret = sys_ftruncate(tf->a0, tf->a1);
		break;

	case SYS_mknod:

		break;

	case SYS_getcwd:
		ret = sys_getcwd((void *) tf->a0, tf->a1);
		break;

	case SYS_fcntl:
		ret = sys_fcntl(tf->a0, tf->a1, tf->a1);
		break;

	default:
		kprintf_s("unknown syscall %u\n", sysnum);
	}

	/* return result in a0 register */
	tf->a0 = ret;

	/* increment epc to return after syscall */
	tf->epc += 4;	
}

