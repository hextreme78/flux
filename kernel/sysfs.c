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
	err = ext2_link(rootblkdev, pathbuf1, pathbuf2,
			curproc()->uid, curproc()->gid, curproc()->cwd);
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
	err = ext2_mkdir(rootblkdev, pathbuf, mode & ~curproc()->umask,
			curproc()->uid, curproc()->gid, curproc()->cwd);
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
	err = ext2_unlink(rootblkdev, pathbuf, curproc()->uid,
			curproc()->gid, curproc()->cwd);
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
	err = ext2_rmdir(rootblkdev, pathbuf, curproc()->uid, curproc()->gid,
			curproc()->cwd);
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
	err = ext2_rename(rootblkdev, pathbuf1, pathbuf2,
			curproc()->uid, curproc()->gid, curproc()->cwd);
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
	err = ext2_readlink(rootblkdev, pathbuf, linkbuf,
			curproc()->uid, curproc()->gid, curproc()->cwd);
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
	struct stat st;
	size_t pathlen;
	u32 inum;
	int fd = -1;
	bool r, w, x;
	r = (flags & O_ACCMODE) != O_WRONLY;
	w = (flags & O_ACCMODE) != O_RDONLY;
	x = flags & O_EXEC;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	for (size_t i = 0; i < FD_MAX; i++) {
		if (!curproc()->filetable[i].alloc) {
			fd = i;
			break;
		}
	}

	if (fd < 0) {
		return -EMFILE;
	}

	curproc()->filetable[fd].status_flags = kmalloc(sizeof(int));
	if (!curproc()->filetable[fd].status_flags) {
		return -ENOMEM;
	}
	curproc()->filetable[fd].roffset =
		curproc()->filetable[fd].woffset = kmalloc(sizeof(off_t));
	if (!curproc()->filetable[fd].roffset) {
		kfree(curproc()->filetable[fd].status_flags);
		return -ENOMEM;
	}
	curproc()->filetable[fd].refcnt = kmalloc(sizeof(size_t));
	if (!curproc()->filetable[fd].refcnt) {
		kfree(curproc()->filetable[fd].status_flags);
		kfree(curproc()->filetable[fd].roffset);
		return -ENOMEM;
	}

	mutex_lock(&rootblkdev->lock);

	if (flags & O_CREAT) {
		err = ext2_creat(rootblkdev, pathbuf, mode & ~curproc()->umask,
				curproc()->uid, curproc()->gid, curproc()->cwd);
		if (err && (err != -EEXIST || (err == -EEXIST && (flags & O_EXCL)))) {
			mutex_unlock(&rootblkdev->lock);
			kfree(curproc()->filetable[fd].status_flags);
			kfree(curproc()->filetable[fd].roffset);
			kfree(curproc()->filetable[fd].refcnt);
			return err;
		}
	}
	err = ext2_file_lookup(rootblkdev, pathbuf, &inum, curproc()->cwd, true,
			curproc()->uid, curproc()->gid, r, w, x, true);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		kfree(curproc()->filetable[fd].refcnt);
		kfree(curproc()->filetable[fd].status_flags);
		kfree(curproc()->filetable[fd].roffset);
		return err;
	}

	if ((flags & O_TRUNC) && ((flags & O_ACCMODE) != O_RDONLY)) {
		err = ext2_truncate(rootblkdev, pathbuf, 0,
				curproc()->uid, curproc()->gid, curproc()->cwd);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			kfree(curproc()->filetable[fd].refcnt);
			kfree(curproc()->filetable[fd].status_flags);
			kfree(curproc()->filetable[fd].roffset);
			return err;
		}
	}

	err = ext2_stat(rootblkdev, pathbuf, &st,
			curproc()->uid, curproc()->gid, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		kfree(curproc()->filetable[fd].refcnt);
		kfree(curproc()->filetable[fd].status_flags);
		kfree(curproc()->filetable[fd].roffset);
		return err;
	}
	if (flags & O_APPEND) {
		*curproc()->filetable[fd].roffset = st.st_size;
	} else {
		*curproc()->filetable[fd].roffset = 0;
	}
	mutex_unlock(&rootblkdev->lock);

	curproc()->filetable[fd].alloc = true;
	curproc()->filetable[fd].ondisk = true;
	curproc()->filetable[fd].ftype = st.st_mode & S_IFMT;
	curproc()->filetable[fd].inum = inum;
	*curproc()->filetable[fd].status_flags = flags;
	if (flags & O_CLOEXEC) {
		curproc()->filetable[fd].fd_flags = FD_CLOEXEC;
	} else {
		curproc()->filetable[fd].fd_flags = 0;
	}
	*curproc()->filetable[fd].refcnt = 1;

	return fd;
}

ssize_t sys_read(int fd, void *buf, size_t count)
{
	void *kbuf;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}
	if ((*curproc()->filetable[fd].status_flags & O_ACCMODE) == O_WRONLY) {
		return -EBADFD;
	}

	switch (curproc()->filetable[fd].ftype) {
	case S_IFREG:
		if (!(kbuf = kmalloc(count))) {
			return -ENOMEM;
		}

		mutex_lock(&rootblkdev->lock);
		count = ext2_regular_read(rootblkdev,
				curproc()->filetable[fd].inum, kbuf, count,
				*curproc()->filetable[fd].roffset);
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

		break;

	case S_IFIFO:
		if (curproc()->filetable[fd].ondisk) {
			/* not implemented */

		} else {
			if (*curproc()->filetable[fd].roffset >=
					PIPEBUF_NPAGES * PAGESZ) {
				return -EINVAL;
			}
			count = min(count, PIPEBUF_NPAGES * PAGESZ -
					*curproc()->filetable[fd].roffset);
			if (copy_to_user(buf, curproc()->filetable[fd].pipebuf +
					*curproc()->filetable[fd].roffset, count)) {
				return -EFAULT;
			}
		}
		break;

	default:
		return -EINVAL;
	}

	*curproc()->filetable[fd].roffset += count;

	return count;
}

ssize_t sys_write(int fd, const void *buf, size_t count)
{
	void *kbuf;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}
	if ((*curproc()->filetable[fd].status_flags & O_ACCMODE) == O_RDONLY) {
		return -EBADFD;
	}

	switch (curproc()->filetable[fd].ftype) {
	case S_IFREG:
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
				*curproc()->filetable[fd].woffset);
		if (count < 0) {
			mutex_unlock(&rootblkdev->lock);
			kfree(kbuf);
			return -EINVAL;
		}
		mutex_unlock(&rootblkdev->lock);

		kfree(kbuf);

		break;

	case S_IFIFO:
		if (curproc()->filetable[fd].ondisk) {
			/* not implemented */

		} else {
			if (*curproc()->filetable[fd].woffset >=
					PIPEBUF_NPAGES * PAGESZ) {
				return -EINVAL;
			}
			count = min(count, PIPEBUF_NPAGES * PAGESZ -
					*curproc()->filetable[fd].woffset);
			if (copy_from_user(curproc()->filetable[fd].pipebuf +
					*curproc()->filetable[fd].woffset, buf, count)) {
				return -EFAULT;
			}
		}
		break;

	default:
		return -EINVAL;
	}

	*curproc()->filetable[fd].woffset += count;

	return count;
}

off_t sys_lseek(int fd, off_t offset, int whence)
{
	int err;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}

	if (curproc()->filetable[fd].ftype == S_IFIFO ||
			curproc()->filetable[fd].ftype == S_IFSOCK) {
		return -ESPIPE;
	}

	if (whence == SEEK_SET) {
		*curproc()->filetable[fd].roffset = offset;
	} else if (whence == SEEK_CUR) {
		*curproc()->filetable[fd].roffset += offset;
	} else if (whence == SEEK_END) {
		struct stat st;
		mutex_lock(&rootblkdev->lock);
		err = ext2_fstat(rootblkdev, curproc()->filetable[fd].inum, &st);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
		mutex_unlock(&rootblkdev->lock);
		*curproc()->filetable[fd].roffset = st.st_size + offset;
	} else {
		return -EINVAL;
	}

	return *curproc()->filetable[fd].roffset;
}

int sys_close(int fd)
{
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}
	curproc()->filetable[fd].alloc = false;
	--*curproc()->filetable[fd].refcnt;
	if (!*curproc()->filetable[fd].refcnt) {
		kfree(curproc()->filetable[fd].refcnt);
		kfree(curproc()->filetable[fd].status_flags);
		kfree(curproc()->filetable[fd].roffset);
		if (curproc()->filetable[fd].ftype == S_IFIFO) {
			if (curproc()->filetable[fd].ondisk) {
				/* not implemented */

			} else {
				kfree(curproc()->filetable[fd].woffset);
				kpage_free(curproc()->filetable[fd].pipebuf);
			}
		}
	}
	return 0;
}

int sys_stat(const char *path, struct stat *st)
{
	int err;
	struct stat stbuf;
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
	err = ext2_stat(rootblkdev, pathbuf, &stbuf,
			curproc()->uid, curproc()->gid, curproc()->cwd);
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

int sys_chdir(const char *path)
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
	err = ext2_chdir(rootblkdev, pathbuf, curproc()->uid, curproc()->gid,
			&curproc()->cwd);
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
	err = ext2_symlink(rootblkdev, pathbuf2, S_IRWXU | S_IRWXG | S_IRWXO,
			curproc()->uid, curproc()->gid, pathbuf1,
			curproc()->cwd);
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
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}
	if ((*curproc()->filetable[fd].status_flags & O_ACCMODE) == O_RDONLY) {
		return -EACCES;
	}

	if (curproc()->filetable[fd].ftype != S_IFREG) {
		return -EINVAL;
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
	err = ext2_getcwd(rootblkdev, curproc()->cwd, pathbuf, size, curproc()->uid,
			curproc()->gid);
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

int sys_fcntl(int fd, int cmd, int arg)
{
	int dupfd = -1;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}

	switch (cmd) {
	case F_DUPFD:
		if (arg < 0) {
			return -EBADFD;
		}
		for (int i = arg; i < FD_MAX; i++) {
			if (!curproc()->filetable[i].alloc) {
				dupfd = i;
			}
		}
		if (dupfd < 0) {
			return -EMFILE;
		}
		curproc()->filetable[dupfd] = curproc()->filetable[fd];
		++*curproc()->filetable[dupfd].refcnt;
		return dupfd;
	case F_DUPFD_CLOEXEC:
		if (arg < 0) {
			return -EBADFD;
		}
		for (int i = arg; i < FD_MAX; i++) {
			if (!curproc()->filetable[i].alloc) {
				dupfd = i;
			}
		}
		if (dupfd < 0) {
			return -EMFILE;
		}
		curproc()->filetable[dupfd] = curproc()->filetable[fd];
		++*curproc()->filetable[dupfd].refcnt;
		curproc()->filetable[dupfd].fd_flags = FD_CLOEXEC;
		return dupfd;
	case F_GETFD:
		return curproc()->filetable[fd].fd_flags;
	case F_SETFD:
		curproc()->filetable[fd].fd_flags = arg;
		break;
	case F_SETFL:
		*curproc()->filetable[fd].status_flags &= (O_ACCMODE | O_EXEC);
		*curproc()->filetable[fd].status_flags |= arg & ~(O_ACCMODE | O_EXEC);
		break;
	case F_GETFL:
		return *curproc()->filetable[fd].status_flags;
	default:
		return -EINVAL;
	}

	return 0;
}

int sys_dup(int oldfd)
{
	int dupfd = -1;
	if (oldfd < 0 || oldfd >= FD_MAX || !curproc()->filetable[oldfd].alloc) {
		return -EBADFD;
	}
	for (int i = 0; i < FD_MAX; i++) {
		if (!curproc()->filetable[i].alloc) {
			dupfd = i;
		}
	}
	if (dupfd < 0) {
		return -EMFILE;
	}
	curproc()->filetable[dupfd] = curproc()->filetable[oldfd];
	++*curproc()->filetable[dupfd].refcnt;
	return dupfd;
}

int sys_dup2(int oldfd, int newfd)
{
	if (oldfd < 0 || oldfd >= FD_MAX || !curproc()->filetable[oldfd].alloc) {
		return -EBADFD;
	}
	if (newfd < 0 || newfd >= FD_MAX) {
		return -EBADFD;
	}
	if (curproc()->filetable[newfd].alloc) {
		--*curproc()->filetable[newfd].refcnt;
		if (!*curproc()->filetable[newfd].refcnt) {
			kfree(curproc()->filetable[newfd].refcnt);
			kfree(curproc()->filetable[newfd].status_flags);
			kfree(curproc()->filetable[newfd].roffset);
			if (curproc()->filetable[newfd].ftype == S_IFIFO &&
					!curproc()->filetable[newfd].ondisk) {
				kfree(curproc()->filetable[newfd].woffset);
				kpage_free(curproc()->filetable[newfd].pipebuf);
			}
		}
	}
	curproc()->filetable[newfd] = curproc()->filetable[oldfd];
	++*curproc()->filetable[newfd].refcnt;
	return newfd;
}

int sys_dup3(int oldfd, int newfd, int flags)
{
	if (oldfd < 0 || oldfd >= FD_MAX || !curproc()->filetable[oldfd].alloc) {
		return -EBADFD;
	}
	if (newfd < 0 || newfd >= FD_MAX) {
		return -EBADFD;
	}
	if (newfd == oldfd) {
		return -EINVAL;
	}
	if (curproc()->filetable[newfd].alloc) {
		--*curproc()->filetable[newfd].refcnt;
		if (!*curproc()->filetable[newfd].refcnt) {
			kfree(curproc()->filetable[newfd].refcnt);
			kfree(curproc()->filetable[newfd].status_flags);
			kfree(curproc()->filetable[newfd].roffset);
			if (curproc()->filetable[newfd].ftype == S_IFIFO &&
					!curproc()->filetable[newfd].ondisk) {
				kfree(curproc()->filetable[newfd].woffset);
				kpage_free(curproc()->filetable[newfd].pipebuf);
			}
		}
	}
	curproc()->filetable[newfd] = curproc()->filetable[oldfd];
	++*curproc()->filetable[newfd].refcnt;
	if (flags & O_CLOEXEC) {
		curproc()->filetable[newfd].fd_flags = FD_CLOEXEC;
	} else {
		curproc()->filetable[newfd].fd_flags = 0;
	}
	return newfd;
}

int sys_lstat(const char *path, struct stat *st)
{
	int err;
	size_t pathlen;
	struct stat stbuf;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_lstat(rootblkdev, pathbuf, &stbuf,
			curproc()->uid, curproc()->gid, curproc()->cwd);
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

mode_t sys_umask(mode_t mask)
{
	mode_t oldmask = curproc()->umask;
	curproc()->umask = mask;
	return oldmask;
}

int sys_chmod(const char *path, mode_t mode)
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
	err = ext2_chmod(rootblkdev, pathbuf, mode,
			curproc()->uid, curproc()->gid, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_chown(const char *path, uid_t uid, gid_t gid)
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
	err = ext2_chown(rootblkdev, pathbuf,
			curproc()->uid, curproc()->gid, curproc()->cwd,
			uid, gid);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_lchown(const char *path, uid_t uid, gid_t gid)
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
	err = ext2_lchown(rootblkdev, pathbuf,
			curproc()->uid, curproc()->gid, curproc()->cwd,
			uid, gid);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_fstat(int fd, struct stat *st)
{
	int err;
	struct stat stbuf;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
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
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}
	if (!(*curproc()->filetable[fd].status_flags & O_EXEC)) {
		return -EACCES;
	}

	if (curproc()->filetable[fd].ftype != S_IFDIR) {
		return -ENOTDIR;
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

int sys_fchmod(int fd, mode_t mode)
{
	int err;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}

	if (!curproc()->filetable[fd].ondisk) {
		return -ENOENT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_fchmod(rootblkdev, curproc()->filetable[fd].inum, mode);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_fchown(int fd, uid_t uid, gid_t gid)
{
	int err;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}

	if (!curproc()->filetable[fd].ondisk) {
		return -ENOENT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_fchown(rootblkdev, curproc()->filetable[fd].inum, uid, gid);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_truncate(const char *path, off_t length)
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
	err = ext2_truncate(rootblkdev, pathbuf, length,
			curproc()->uid, curproc()->gid, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

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
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_creat(rootblkdev, pathbuf, mode & ~curproc()->umask,
				curproc()->uid, curproc()->gid, curproc()->cwd);
	if (err && (err != -EEXIST)) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	} else if (err == -EEXIST) {
		err = ext2_truncate(rootblkdev, pathbuf, 0,
				curproc()->uid, curproc()->gid, curproc()->cwd);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_access(const char *path, int mode)
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
	err = ext2_access(rootblkdev, pathbuf, mode,
				curproc()->uid, curproc()->gid, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_pipe2(int *pipefd, int flags)
{
	int fds[2] = {-1, -1};
	for (size_t i = 0; i < FD_MAX; i++) {
		if (!curproc()->filetable[i].alloc) {
			if (fds[0] < 0) {
				fds[0] = i;
			} else {
				fds[1] = i;
				break;
			}
		}
	}
	if (fds[1] < 0) {
		return -EMFILE;
	}

	if (copy_to_user(pipefd, fds, sizeof(fds))) {
		return -EFAULT;
	}

	curproc()->filetable[fds[0]].status_flags = kmalloc(sizeof(int));
	if (!curproc()->filetable[fds[0]].status_flags) {
		return -ENOMEM;
	}
	curproc()->filetable[fds[0]].roffset = kmalloc(sizeof(off_t));
	if (!curproc()->filetable[fds[0]].roffset) {
		kfree(curproc()->filetable[fds[0]].status_flags);
		return -ENOMEM;
	}
	curproc()->filetable[fds[0]].woffset = kmalloc(sizeof(off_t));
	if (!curproc()->filetable[fds[0]].woffset) {
		kfree(curproc()->filetable[fds[0]].status_flags);
		kfree(curproc()->filetable[fds[0]].roffset);
		return -ENOMEM;
	}
	curproc()->filetable[fds[0]].refcnt = kmalloc(sizeof(size_t));
	if (!curproc()->filetable[fds[0]].refcnt) {
		kfree(curproc()->filetable[fds[0]].status_flags);
		kfree(curproc()->filetable[fds[0]].roffset);
		kfree(curproc()->filetable[fds[0]].woffset);
		return -ENOMEM;
	}
	curproc()->filetable[fds[0]].pipebuf = kpage_alloc(PIPEBUF_NPAGES);
	if (!curproc()->filetable[fds[0]].pipebuf) {
		kfree(curproc()->filetable[fds[0]].status_flags);
		kfree(curproc()->filetable[fds[0]].roffset);
		kfree(curproc()->filetable[fds[0]].woffset);
		kfree(curproc()->filetable[fds[0]].refcnt);
		return -ENOMEM;
	}

	curproc()->filetable[fds[0]].alloc = true;
	curproc()->filetable[fds[0]].ondisk = false;
	curproc()->filetable[fds[0]].ftype = S_IFIFO;
	curproc()->filetable[fds[0]].inum = 0;

	if (flags & O_CLOEXEC) {
		curproc()->filetable[fds[0]].fd_flags = FD_CLOEXEC;
	} else {
		curproc()->filetable[fds[0]].fd_flags = 0;
	}

	*curproc()->filetable[fds[0]].status_flags = flags | O_RDWR;
	*curproc()->filetable[fds[0]].roffset = 0;
	*curproc()->filetable[fds[0]].woffset = 0;
	*curproc()->filetable[fds[0]].refcnt = 2;

	curproc()->filetable[fds[1]] = curproc()->filetable[fds[0]];

	return 0;
}

int sys_mkfifo(const char *path, mode_t mode)
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
	err = ext2_mkfifo(rootblkdev, pathbuf, mode & ~curproc()->umask,
			curproc()->uid, curproc()->gid, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);

	return 0;
}

