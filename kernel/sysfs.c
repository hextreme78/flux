#include <kernel/sysfs.h>
#include <kernel/ext2.h>
#include <kernel/klib.h>

int sys_creat(const char *path, mode_t mode)
{
	int err;
	size_t pathlen;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EINVAL;
	}

	mutex_lock(&rootblkdev->lock);

	err = ext2_creat(rootblkdev, pathbuf, mode, curproc()->uid, curproc()->gid,
			curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}

	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_link(const char *path1, const char *path2)
{
	int err;
	size_t pathlen1, pathlen2;

	pathlen1 = strnlen_user(path1, PATH_MAX);
	if (pathlen1 > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf1[pathlen1];
	if (copy_from_user(pathbuf1, path1, pathlen1)) {
		return -EINVAL;
	}

	pathlen2 = strnlen_user(path2, PATH_MAX);
	if (pathlen2 > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf2[pathlen2];
	if (copy_from_user(pathbuf2, path2, pathlen2)) {
		return -EINVAL;
	}

	mutex_lock(&rootblkdev->lock);

	err = ext2_link(rootblkdev, pathbuf1, pathbuf2, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}

	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_mkdir(const char *path, mode_t mode)
{

	return 0;
}

int sys_readlink(const char *restrict path, char *restrict buf,
		size_t bufsize)
{

	return 0;
}

int sys_rename(const char *old, const char *new)
{

	return 0;
}

int sys_rmdir(const char *path)
{

	return 0;
}

int sys_unlink(const char *path)
{

	return 0;
}

int sys_read(const char *path)
{

	return 0;
}

int sys_write(const char *path)
{

	return 0;
}

int sys_stat(const char *restrict path, struct stat *restrict buf)
{

	return 0;
}

