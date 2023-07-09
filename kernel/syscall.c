#include <kernel/syscall.h>
#include <kernel/riscv64.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/sysproc.h>

void debug_printint(i64 a0)
{
	kprintf_s("debug_printint %d\n", a0);
	
	/*
	char data[512] = "qwerty3210\n";
	virtio_blk_write(7, 0, data);
	*/
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
		ret = sys_wait((int *) tf->a0);
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

	default:
		kprintf_s("unknown syscall %u\n", sysnum);
	}

	/* return result in a0 register */
	tf->a0 = ret;

	/* increment epc to return after syscall */
	tf->epc += 4;	
}

