#include <kernel/syscall.h>
#include <kernel/riscv64.h>
#include <kernel/proc.h>
#include <kernel/kprintf.h>
#include <kernel/sysproc.h>

#include <kernel/ext2.h>

int ext2_create_test(void)
{
	int err;
	extern ext2_blkdev_t ext2_dev_list;
	ext2_blkdev_t *dev = list_next_entry(&ext2_dev_list, devlist);
	u16 mode = EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IXUSR;
	u16 uid = 1000;
	u16 gid = 1000;

	err = ext2_creat(dev, "/regular", mode, uid, gid, 2);
	if (err) {
		kprintf_s("ext2_creat() err %d\n", err);
		return err;
	}

	err = ext2_link(dev, "/regular", "/hardlink", 2);
	if (err) {
		kprintf_s("ext2_link() err %d\n", err);
		return err;
	}

	err = ext2_mkdir(dev, "/directory", mode, uid, gid, 2);
	if (err) {
		kprintf_s("ext2_mkdir() err %d\n", err);
		return err;
	}

	err = ext2_symlink(dev, "/symlink", mode, uid, gid, "directory", 2);
	if (err) {
		kprintf_s("ext2_symlink() err %d\n", err);
		return err;
	}

	return 0;
}

int ext2_delete_test(void)
{
	int err;
	extern ext2_blkdev_t ext2_dev_list;
	ext2_blkdev_t *dev = list_next_entry(&ext2_dev_list, devlist);

	err = ext2_unlink(dev, "/regular", 2);
	if (err) {
		kprintf_s("1 -- ext2_unlink() err %d\n", err);
		return err;
	}

	err = ext2_unlink(dev, "/hardlink", 2);
	if (err) {
		kprintf_s("2 -- ext2_unlink() err %d\n", err);
		return err;
	}

	err = ext2_rmdir(dev, "/directory", 2);
	if (err) {
		kprintf_s("ext2_rmdir() err %d\n", err);
		return err;
	}

	err = ext2_unlink(dev, "/symlink", 2);
	if (err) {
		kprintf_s("3 -- ext2_unlink() err %d\n", err);
		return err;
	}

	return 0;
}

int ext2_move_test(void)
{
	int err;
	extern ext2_blkdev_t ext2_dev_list;
	ext2_blkdev_t *dev = list_next_entry(&ext2_dev_list, devlist);

	err = ext2_rename(dev, "/regular", "/directory/", 2);
	if (err) {
		kprintf_s("ext2_rename() err %d\n", err);
		return err;
	}

	return 0;
}

void debug_printint(i64 a0)
{
	ext2_create_test();
	ext2_move_test();

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

