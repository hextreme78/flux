#include <kernel/syscall.h>
#include <kernel/riscv64.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/sysproc.h>
#include <kernel/sysfs.h>

void debug_print(const char *str)
{
	size_t len;
	len = strlen_user(str);
	char strbuf[len];
	if (copy_from_user(strbuf, str, len)) {
		return;
	}
	kprintf_s(strbuf);
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
	case SYS_exit:
		sys_exit(tf->a0);

	case SYS_read:
		ret = sys_read(tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_write:
		ret = sys_write(tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_lseek:
		ret = sys_lseek(tf->a0, tf->a1, tf->a2);
		break;

	case SYS_close:
		ret = sys_close(tf->a0);
		break;

	case SYS_faccessat:
		ret = sys_faccessat(tf->a0, (void *) tf->a1, tf->a2, tf->a3);
		break;

	case SYS_dup:
		ret = sys_dup(tf->a0);
		break;

	case SYS_dup2:
		ret = sys_dup2(tf->a0, tf->a1);
		break;

	case SYS_fstatat:
		ret = sys_fstatat(tf->a0, (void *) tf->a1, (void *) tf->a2, tf->a3);
		break;

	case SYS_readlink:
		ret = sys_readlink((void *) tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_truncate:
		ret = sys_truncate(tf->a0, (void *) tf->a1, tf->a2, tf->a3);
		break;

	case SYS_unlinkat:
		ret = sys_unlinkat(tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_openat:
		ret = sys_openat(tf->a0, (void *) tf->a1, tf->a2, tf->a3);
		break;

	case SYS_getpid:
		ret = sys_getpid();
		break;

	case SYS_getppid:
		ret = sys_getppid();
		break;

	case SYS_sleep:
		ret = sys_sleep((void *) tf->a0, (void *) tf->a1);
		break;

	case SYS_fork:
		ret = sys_fork();
		break;

	case SYS_execve:
		ret = sys_execve((void *) tf->a0, (void *) tf->a1, (void *) tf->a2);
		break;

	case SYS_getcwd:
		ret = sys_getcwd((void *) tf->a0, tf->a1);
		break;

	case SYS_chdir:
		ret = sys_chdir(tf->a0, (void *) tf->a1, tf->a2);
		break;

	case SYS_linkat:
		ret = sys_linkat(tf->a0, (void *) tf->a1,
				tf->a2, (void *) tf->a3, tf->a4);
		break;

	case SYS_renameat2:
		ret = sys_renameat2(tf->a0, (void *) tf->a1,
				tf->a2, (void *) tf->a3, tf->a4);
		break;

	case SYS_fcntl:
		ret = sys_fcntl(tf->a0, tf->a1, tf->a2);
		break;

	case SYS_fchmodat:
		ret = sys_fchmodat(tf->a0, (void *) tf->a1, tf->a2, tf->a3);
		break;

	case SYS_pipe2:
		ret = sys_pipe2((void *) tf->a0, tf->a1);
		break;

	case SYS_mknodat:
		ret = sys_mknodat(tf->a0, (void *) tf->a1, tf->a2, tf->a3,
				(void *) tf->a4);
		break;

	case SYS_umask:
		ret = sys_umask(tf->a0);
		break;

	case SYS_fchownat:
		ret = sys_fchownat(tf->a0, (void *) tf->a1, tf->a2, tf->a3, tf->a4);
		break;

	case SYS_uname:
		ret = sys_uname((void *) tf->a0);
		break;

	case SYS_wait4:
		ret = sys_wait4(tf->a0, (void *) tf->a1, tf->a2, (void *) tf->a3);
		break;

	case SYS_brk:
		ret = sys_brk((void *) tf->a0);
		break;

	case SYS_getuid:
		ret = sys_getuid();
		break;

	case SYS_geteuid:
		ret = sys_geteuid();
		break;

	case SYS_getgid:
		ret = sys_getgid();
		break;

	case SYS_getegid:
		ret = sys_getegid();
		break;

	case 0:
		debug_print((void *) tf->a0);
		break;

	default:
		kprintf_s("unknown syscall %u\n", sysnum);
	}

	/* return result in a0 register */
	tf->a0 = ret;

	/* increment epc to return after syscall */
	tf->epc += 4;	
}

