#include <kernel/syscall.h>
#include <kernel/riscv64.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/sysproc.h>
#include <kernel/sysfs.h>

#include <kernel/ext2.h>

void debug_printint(i64 a0)
{
	kprintf_s("debug_printint %d\n", a0);
}

void syscall(void)
{
	u64 ret = 0;
	u64 sysnum;
	trapframe_t *tf;

	tf = curproc()->trapframe;

	/* get syscall number from a7 register */
	sysnum = tf->a7;

	switch (sysnum) {
	case SYS_DEBUG_PRINTINT:
		debug_printint(tf->a0);
		break;

	case SYS__EXIT:
		/* never returns */
		sys__exit(tf->a0);

	case SYS_WAIT:
		ret = sys_wait((void *) tf->a0);
		break;

	case SYS_KILL:

		break;

	case SYS_GETPID:
		ret = sys_getpid();
		break;

	case SYS_FORK:

		break;

	case SYS_EXECVE:

		break;

	case SYS_SBRK:

		break;

	case SYS_CREAT:
		ret = sys_creat((void *) tf->a0, tf->a1);
		break;

	case SYS_LINK:
		ret = sys_link((void *) tf->a0, (void *) tf->a1);
		break;

	case SYS_MKDIR:
		ret = sys_mkdir((void *) tf->a0, tf->a1);
		break;

	case SYS_UNLINK:
		ret = sys_unlink((void *) tf->a0);
		break;

	case SYS_RMDIR:
		ret = sys_rmdir((void *) tf->a0);
		break;

	case SYS_RENAME:
		ret = sys_rename((void *) tf->a0, (void *) tf->a1);
		break;

	case SYS_READLINK:
		ret = sys_readlink((void *) tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_OPEN:
		ret = sys_open((void *) tf->a0, tf->a1, tf->a2);
		break;

	case SYS_READ:
		ret = sys_read(tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_WRITE:
		ret = sys_write(tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_LSEEK:
		ret = sys_lseek(tf->a0, tf->a1, tf->a2);
		break;

	case SYS_CLOSE:
		ret = sys_close(tf->a0);
		break;

	case SYS_STAT:
		ret = sys_stat((void *) tf->a0, (void *) tf->a1);
		break;

	case SYS_CHDIR:
		ret = sys_chdir((void *) tf->a0);
		break;

	default:
		kprintf_s("unknown syscall %u\n", sysnum);
	}

	/* return result in a0 register */
	tf->a0 = ret;

	/* increment epc to return after syscall */
	tf->epc += 4;	
}

