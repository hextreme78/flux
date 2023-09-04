#include <kernel/sysfs.h>
#include <kernel/ext2.h>
#include <kernel/klib.h>
#include <kernel/alloc.h>

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
		return -EFAULT;
	}

	pathlen2 = strnlen_user(path2, PATH_MAX);
	if (pathlen2 > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf2[pathlen2];
	if (copy_from_user(pathbuf2, path2, pathlen2)) {
		return -EFAULT;
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
	int err;
	size_t pathlen;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_mkdir(rootblkdev, pathbuf, mode, curproc()->uid, curproc()->gid,
			curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_unlink(const char *path)
{
	int err;
	size_t pathlen;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_unlink(rootblkdev, pathbuf, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_rmdir(const char *path)
{
	int err;
	size_t pathlen;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_rmdir(rootblkdev, pathbuf, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_rename(const char *path1, const char *path2)
{
	int err;
	size_t pathlen1, pathlen2;

	pathlen1 = strnlen_user(path1, PATH_MAX);
	if (pathlen1 > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf1[pathlen1];
	if (copy_from_user(pathbuf1, path1, pathlen1)) {
		return -EFAULT;
	}

	pathlen2 = strnlen_user(path2, PATH_MAX);
	if (pathlen2 > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf2[pathlen2];
	if (copy_from_user(pathbuf2, path2, pathlen2)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_rename(rootblkdev, pathbuf1, pathbuf2, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

ssize_t sys_readlink(const char *restrict path, char *restrict buf,
		size_t bufsize)
{
	int err;
	size_t pathlen;
	char linkbuf[PATH_MAX];
	size_t linklen;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_readlink(rootblkdev, pathbuf, linkbuf, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	linklen = strlen(linkbuf);
	if (copy_to_user(buf, linkbuf, min(linklen, bufsize))) {
		return -EFAULT;
	}

	return min(linklen, bufsize);
}

int sys_open(const char *path, int flags, mode_t mode)
{
	int err;
	size_t pathlen;
	u32 inum;
	int fd = -1;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	for (size_t i = 0; i < FD_MAX; i++) {
		if (!curproc()->filetable[i].inum) {
			fd = i;
		}
	}

	if (fd < 0) {
		return -EMFILE;
	}

	mutex_lock(&rootblkdev->lock);
	if (flags & O_CREAT) {
		err = ext2_creat(rootblkdev, pathbuf, mode, curproc()->uid,
				curproc()->gid, curproc()->cwd);
		if (err && (err != -EEXIST || (err == -EEXIST && (flags & O_EXCL)))) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}
	err = ext2_file_lookup(rootblkdev, pathbuf, &inum, curproc()->cwd, true);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	if ((flags & O_TRUNC) && ((flags + 1) & (O_WRONLY + 1))) {
		err = ext2_truncate(rootblkdev, pathbuf, 0, curproc()->cwd);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}

	if (flags & O_APPEND) {
		struct stat st;
		err = ext2_stat(rootblkdev, pathbuf, &st, curproc()->cwd);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
		curproc()->filetable[fd].offset = st.st_size;
	} else {
		curproc()->filetable[fd].offset = 0;
	}
	mutex_unlock(&rootblkdev->lock);

	curproc()->filetable[fd].inum = inum;
	curproc()->filetable[fd].flags = flags;

	return fd;
}

ssize_t sys_read(int fd, void *buf, size_t count)
{
	void *kbuf;
	if (fd >= FD_MAX || !curproc()->filetable[fd].inum) {
		return -EBADFD;
	}
	if (!((curproc()->filetable[fd].flags + 1) & (O_RDONLY + 1))) {
		return -EBADF;
	}

	if (!(kbuf = kmalloc(count))) {
		return -ENOMEM;
	}

	mutex_lock(&rootblkdev->lock);
	count = ext2_regular_read(rootblkdev,
			curproc()->filetable[fd].inum, kbuf, count,
			curproc()->filetable[fd].offset);
	if (count < 0) {
		mutex_unlock(&rootblkdev->lock);
		kfree(kbuf);
		return -EINVAL;
	}
	mutex_unlock(&rootblkdev->lock);

	if (copy_to_user(buf, kbuf, count)) {
		kfree(kbuf);
		return -EFAULT;
	}

	kfree(kbuf);

	curproc()->filetable[fd].offset += count;

	return count;
}

ssize_t sys_write(int fd, const void *buf, size_t count)
{
	void *kbuf;
	if (fd >= FD_MAX || !curproc()->filetable[fd].inum) {
		return -EBADFD;
	}
	if (!((curproc()->filetable[fd].flags + 1) & (O_WRONLY + 1))) {
		return -EBADF;
	}

	if (!(kbuf = kmalloc(count))) {
		return -ENOMEM;
	}

	if (copy_from_user(kbuf, buf, count)) {
		kfree(kbuf);
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	count = ext2_regular_write(rootblkdev,
			curproc()->filetable[fd].inum, kbuf, count,
			curproc()->filetable[fd].offset);
	if (count < 0) {
		mutex_unlock(&rootblkdev->lock);
		kfree(kbuf);
		return -EINVAL;
	}
	mutex_unlock(&rootblkdev->lock);

	kfree(kbuf);

	curproc()->filetable[fd].offset += count;

	return count;
}

off_t sys_lseek(int fd, off_t offset, int whence)
{
	int err;
	if (fd >= FD_MAX || !curproc()->filetable[fd].inum) {
		return -EBADFD;
	}

	if (whence == SEEK_SET) {
		curproc()->filetable[fd].offset = offset;
	} else if (whence == SEEK_CUR) {
		curproc()->filetable[fd].offset += offset;
	} else if (whence == SEEK_END) {
		struct stat st;
		mutex_lock(&rootblkdev->lock);
		err = ext2_fstat(rootblkdev, curproc()->filetable[fd].inum, &st);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
		mutex_unlock(&rootblkdev->lock);
		curproc()->filetable[fd].offset = st.st_size + offset;
	} else {
		return -EINVAL;
	}

	return curproc()->filetable[fd].offset;
}

int sys_close(int fd)
{
	if (fd >= FD_MAX || !curproc()->filetable[fd].inum) {
		return -EBADFD;
	}
	curproc()->filetable[fd].inum = 0;

	return 0;
}

int sys_fstat(int fd, struct stat *st)
{
	int err;
	struct stat stbuf;

	if (fd >= FD_MAX || !curproc()->filetable[fd].inum) {
		return -EBADFD;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_fstat(rootblkdev, curproc()->filetable[fd].inum, &stbuf);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	if (copy_to_user(st, &stbuf, sizeof(*st))) {
		return -EFAULT;
	}

	return 0;
}

int sys_fchdir(int fd)
{
	int err;

	if (fd >= FD_MAX || !curproc()->filetable[fd].inum) {
		return -EBADFD;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_fchdir(rootblkdev, curproc()->filetable[fd].inum, &curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_symlink(const char *path1, const char *path2)
{
	int err;
	size_t pathlen1, pathlen2;

	pathlen1 = strnlen_user(path1, PATH_MAX);
	if (pathlen1 > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf1[pathlen1];
	if (copy_from_user(pathbuf1, path1, pathlen1)) {
		return -EFAULT;
	}

	pathlen2 = strnlen_user(path2, PATH_MAX);
	if (pathlen2 > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf2[pathlen2];
	if (copy_from_user(pathbuf2, path2, pathlen2)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_symlink(rootblkdev, pathbuf2, 0777, curproc()->uid,
			curproc()->gid, pathbuf1, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_ftruncate(int fd, off_t length)
{
	int err;

	if (fd >= FD_MAX || !curproc()->filetable[fd].inum) {
		return -EBADFD;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_ftruncate(rootblkdev, curproc()->filetable[fd].inum, length);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_getcwd(char *buf, size_t size)
{
	int err;
	char pathbuf[PATH_MAX];

	mutex_lock(&rootblkdev->lock);
	err = ext2_getcwd(rootblkdev, curproc()->cwd, pathbuf, size);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	if (copy_to_user(buf, pathbuf, strlen(pathbuf))) {
		return -EFAULT;
	}

	return 0;
}

