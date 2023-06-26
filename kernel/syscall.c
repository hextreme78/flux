#include <kernel/syscall.h>
#include <kernel/riscv64.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/sysproc.h>

void debug_print0(void)
{
	kprintf_s("debug_print0\n");
}

void debug_print1(void)
{
	kprintf_s("debug_print1\n");
}

void syscall(void)
{
	trapframe_t *tf = curproc()->trapframe;
	u64 callnum = tf->a7;

	switch (callnum) {
	case 0:
		debug_print0();
		break;
	case 1:
		debug_print1();
		break;

	case SYS_EXIT:
		sys_exit(tf->a0);
		break;

	case SYS_WAITPID:
		//sys_waitpid();
		break;
	default:
		kprintf_s("unknown syscall %u\n", callnum);
	}

	/* increment sepc to return after ecall */
	w_sepc(r_sepc() + 4);
}

