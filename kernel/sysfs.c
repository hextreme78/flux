#include <kernel/sysfs.h>
#include <kernel/ext2.h>
#include <kernel/klib.h>
#include <kernel/alloc.h>
#include <kernel/dev.h>

ssize_t sys_read(int fd, void *buf, size_t count)
{
	size_t roffset, woffset, upperbound = PIPEBUF_NPAGES * PAGESZ;
	void *kbuf, *pipebuf;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}
	if ((*curproc()->filetable[fd].status_flags & O_ACCMODE) == O_WRONLY) {
		return -EBADFD;
	}

	if (curproc()->filetable[fd].ftype == S_IFCHR) {
		return character_device_driver_read(
				&curproc()->filetable[fd], buf, count);
	} else if (curproc()->filetable[fd].ftype == S_IFBLK) {
		return block_device_driver_read(
				&curproc()->filetable[fd], buf, count);
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

		*curproc()->filetable[fd].roffset += count;

		break;

	case S_IFIFO:
		if (curproc()->filetable[fd].ondisk) {
			spinlock_acquire(&fifodescs_lock);
			roffset = curproc()->filetable[fd].fifodesc->roffset;
			woffset = curproc()->filetable[fd].fifodesc->woffset;
			pipebuf = curproc()->filetable[fd].fifodesc->pipebuf;
		} else {
			roffset = *curproc()->filetable[fd].roffset;
			woffset = *curproc()->filetable[fd].woffset;
			pipebuf = curproc()->filetable[fd].pipebuf;
		}

		if (roffset <= woffset && roffset + count > woffset) {
			count = woffset - roffset;
		} else if (roffset > woffset && roffset + count >= upperbound &&
				(roffset + count) % upperbound > woffset) {
			count = upperbound - roffset + woffset;
		}

		if (roffset + count > upperbound) {
			if (copy_to_user(buf, pipebuf + roffset,
						upperbound - roffset)) {
				if (curproc()->filetable[fd].ondisk) {
					spinlock_release(&fifodescs_lock);
				}
				return -EFAULT;
			}
			if (copy_to_user(buf + upperbound - roffset, pipebuf,
						count - upperbound + roffset)) {
				if (curproc()->filetable[fd].ondisk) {
					spinlock_release(&fifodescs_lock);
				}
				return -EFAULT;
			}
		} else {
			if (copy_to_user(buf, pipebuf + roffset, count)) {
				if (curproc()->filetable[fd].ondisk) {
					spinlock_release(&fifodescs_lock);
				}
				return -EFAULT;
			}
		}

		if (curproc()->filetable[fd].ondisk) {
			curproc()->filetable[fd].fifodesc->roffset =
				(roffset + count) % upperbound;
			spinlock_release(&fifodescs_lock);
		} else {
			*curproc()->filetable[fd].roffset =
				(roffset + count) % upperbound;
		}

		break;

	default:
		return -EINVAL;
	}

	return count;
}

ssize_t sys_write(int fd, const void *buf, size_t count)
{
	size_t roffset, woffset, upperbound = PIPEBUF_NPAGES * PAGESZ;
	void *kbuf, *pipebuf;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}
	if ((*curproc()->filetable[fd].status_flags & O_ACCMODE) == O_RDONLY) {
		return -EBADFD;
	}

	if (curproc()->filetable[fd].ftype == S_IFCHR) {
		return character_device_driver_write(
				&curproc()->filetable[fd], buf, count);
	} else if (curproc()->filetable[fd].ftype == S_IFBLK) {
		return block_device_driver_write(
				&curproc()->filetable[fd], buf, count);
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
		mutex_unlock(&rootblkdev->lock);
		kfree(kbuf);
		if (count < 0) {
			return -EINVAL;
		}

		*curproc()->filetable[fd].woffset += count;

		break;

	case S_IFIFO:
		if (curproc()->filetable[fd].ondisk) {
			spinlock_acquire(&fifodescs_lock);
			roffset = curproc()->filetable[fd].fifodesc->roffset;
			woffset = curproc()->filetable[fd].fifodesc->woffset;
			pipebuf = curproc()->filetable[fd].fifodesc->pipebuf;
		} else {
			roffset = *curproc()->filetable[fd].roffset;
			woffset = *curproc()->filetable[fd].woffset;
			pipebuf = curproc()->filetable[fd].pipebuf;
		}

		if (woffset < roffset && woffset + count >= roffset) {
			count = roffset - woffset - 1;
		} else if (woffset >= roffset && woffset + count >= upperbound &&
				(woffset + count) % upperbound >= roffset) {
			count = upperbound - woffset + roffset - 1;
		}

		if (woffset + count > upperbound) {
			if (copy_from_user(pipebuf + woffset, buf,
						upperbound - woffset)) {
				if (curproc()->filetable[fd].ondisk) {
					spinlock_release(&fifodescs_lock);
				}
				return -EFAULT;
			}
			if (copy_from_user(pipebuf, buf + upperbound - woffset,
						count - upperbound + woffset)) {
				if (curproc()->filetable[fd].ondisk) {
					spinlock_release(&fifodescs_lock);
				}
				return -EFAULT;
			}
		} else {
			if (copy_from_user(pipebuf + woffset, buf, count)) {
				if (curproc()->filetable[fd].ondisk) {
					spinlock_release(&fifodescs_lock);
				}
				return -EFAULT;
			}
		}

		if (curproc()->filetable[fd].ondisk) {
			curproc()->filetable[fd].fifodesc->woffset =
				(woffset + count) % upperbound;
			spinlock_release(&fifodescs_lock);
		} else {
			*curproc()->filetable[fd].woffset =
				(woffset + count) % upperbound;
		}

		break;

	default:
		return -EINVAL;
	}

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
	} else if (curproc()->filetable[fd].ftype == S_IFCHR) {
		return character_device_driver_lseek(
				&curproc()->filetable[fd], offset, whence);
	} else if (curproc()->filetable[fd].ftype == S_IFBLK) {
		return block_device_driver_lseek(
				&curproc()->filetable[fd], offset, whence);
	}

	if (whence == SEEK_SET) {
		*curproc()->filetable[fd].roffset = offset;
	} else if (whence == SEEK_CUR) {
		*curproc()->filetable[fd].roffset += offset;
	} else if (whence == SEEK_END) {
		struct stat st;
		mutex_lock(&rootblkdev->lock);
		err = ext2_stat(rootblkdev, curproc()->filetable[fd].inum, &st);
		mutex_unlock(&rootblkdev->lock);
		if (err) {
			return err;
		}
		*curproc()->filetable[fd].roffset = st.st_size + offset;
	} else {
		return -EINVAL;
	}

	return *curproc()->filetable[fd].roffset;
}

int sys_close(int fd)
{
	int err;
	bool deletemark = false;
	if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
		return -EBADFD;
	}

	if (curproc()->filetable[fd].ftype == S_IFCHR) {
		err = character_device_driver_close(&curproc()->filetable[fd]);
		if (err) {
			return err;	
		}
		curproc()->filetable[fd].alloc = false;
		--*curproc()->filetable[fd].refcnt;
		if (!*curproc()->filetable[fd].refcnt) {
			kfree(curproc()->filetable[fd].refcnt);
			kfree(curproc()->filetable[fd].status_flags);
		}
		return 0;
	} else if (curproc()->filetable[fd].ftype == S_IFBLK) {
		err = block_device_driver_close(&curproc()->filetable[fd]);
		if (err) {
			return err;	
		}
		curproc()->filetable[fd].alloc = false;
		--*curproc()->filetable[fd].refcnt;
		if (!*curproc()->filetable[fd].refcnt) {
			kfree(curproc()->filetable[fd].refcnt);
			kfree(curproc()->filetable[fd].status_flags);
		}
		return 0;
	}

	curproc()->filetable[fd].alloc = false;
	--*curproc()->filetable[fd].refcnt;
	if (!*curproc()->filetable[fd].refcnt) {
		kfree(curproc()->filetable[fd].refcnt);
		kfree(curproc()->filetable[fd].status_flags);
		kfree(curproc()->filetable[fd].roffset);

		if (curproc()->filetable[fd].ftype == S_IFIFO &&
				!curproc()->filetable[fd].ondisk) {
			kfree(curproc()->filetable[fd].woffset);
			kpage_free(curproc()->filetable[fd].pipebuf);
		}
		spinlock_acquire(&opened_inodes_lock);
		curproc()->filetable[fd].opened_inode->refcnt--;
		if (!curproc()->filetable[fd].opened_inode->refcnt) {
			deletemark = curproc()->filetable[fd].opened_inode->deletemark;
			list_del(&curproc()->filetable[fd].opened_inode->
					opened_inodes_list);
			kfree(curproc()->filetable[fd].opened_inode);
		}
		spinlock_release(&opened_inodes_lock);
	}
	
	if (deletemark) {
		mutex_lock(&rootblkdev->lock);
		err = ext2_unlink_file(rootblkdev, curproc()->filetable[fd].inum);
		mutex_unlock(&rootblkdev->lock);
		if (err) {
			return err;
		}
		if (curproc()->filetable[fd].ftype == S_IFIFO &&
				curproc()->filetable[fd].ondisk) {
			spinlock_acquire(&fifodescs_lock);
			list_del(&curproc()->filetable[fd].fifodesc->fifodescs_list);
			kpage_free(curproc()->filetable[fd].fifodesc->pipebuf);
			kfree(curproc()->filetable[fd].fifodesc);
			spinlock_release(&fifodescs_lock);
		}
	}

	return 0;
}

int sys_faccessat(int dirfd, const char *path, int mode, int flags)
{
	int err;
	size_t pathlen;
	ino_t inum, relinum;
	uid_t uid;
	gid_t gid;

	if (dirfd == AT_FDCWD) {
		relinum = curproc()->cwd;
	} else if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
		return -EBADFD;
	} else {
		relinum = curproc()->filetable[dirfd].inum;
	}

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	if (flags & AT_EACCESS) {
		uid = curproc()->euid;
		gid = curproc()->egid;
	} else {
		uid = curproc()->uid;
		gid = curproc()->gid;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_file_lookup(rootblkdev, path, &inum,
			relinum, !(flags & AT_SYMLINK_NOFOLLOW),
			uid, gid,
			mode & R_OK, mode & W_OK, mode & X_OK, true);
	mutex_unlock(&rootblkdev->lock);
	if (err) {
		return err;
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
			break;
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
		int ret;
		if ((ret = sys_close(newfd))) {
			return ret;
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
		int ret;
		if ((ret = sys_close(newfd))) {
			return ret;
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

int sys_fstatat(int dirfd, const char *path, struct stat *st, int flags)
{
	int err;
	ino_t inum, relinum;
	struct stat stbuf;

	if (dirfd == AT_FDCWD) {
		relinum = curproc()->cwd;
	} else if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
		return -EBADFD;
	} else {
		relinum = curproc()->filetable[dirfd].inum;
	}

	if (flags & AT_EMPTY_PATH) {
		if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
			return -EBADFD;
		}
		if (!curproc()->filetable[dirfd].ondisk) {
			return -ENOENT;
		}

		inum = curproc()->filetable[dirfd].inum;

		mutex_lock(&rootblkdev->lock);
	} else {
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

		err = ext2_file_lookup(rootblkdev, pathbuf, &inum,
				relinum, !(flags & AT_SYMLINK_NOFOLLOW),
				curproc()->euid, curproc()->egid,
				false, false, false, true);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}


	err = ext2_stat(rootblkdev, inum, &stbuf);
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

ssize_t sys_readlink(const char *path, char *buf,
		size_t bufsize)
{
	int err;
	size_t pathlen;
	char linkbuf[PATH_MAX];
	size_t linklen;
	ino_t inum;

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_file_lookup(rootblkdev, pathbuf, &inum,
			curproc()->cwd, false,
			curproc()->euid, curproc()->egid,
			false, false, false, true);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	err = ext2_readlink(rootblkdev, inum, linkbuf);
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

int sys_truncate(int fd, const char *path, off_t length, int flags)
{
	int err;
	ino_t inum;

	if (flags & AT_EMPTY_PATH) {
		if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
			return -EBADFD;
		}
		if ((*curproc()->filetable[fd].status_flags & O_ACCMODE) == O_RDONLY) {
			return -EACCES;
		}
		if (!curproc()->filetable[fd].ondisk) {
			return -ENOENT;
		}

		inum = curproc()->filetable[fd].inum;

		mutex_lock(&rootblkdev->lock);
	} else {
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

		err = ext2_file_lookup(rootblkdev, pathbuf, &inum,
				curproc()->cwd, true,
				curproc()->euid, curproc()->egid,
				false, true, false, true);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}

	err = ext2_truncate(rootblkdev, inum, length);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}

	mutex_unlock(&rootblkdev->lock);

	return 0;
}

int sys_unlinkat(int dirfd, const char *path, int flags)
{
	int err;
	size_t pathlen;
	ino_t relinum, inum;
	opened_inode_t opened_inode, *opened_inode_ptr;
	fifodesc_t fifodesc, *fifodesc_ptr;

	if (dirfd == AT_FDCWD) {
		relinum = curproc()->cwd;
	} else if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
		return -EBADFD;
	} else {
		relinum = curproc()->filetable[dirfd].inum;
	}

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_file_lookup(rootblkdev, pathbuf, &inum, relinum, false,
			curproc()->euid, curproc()->egid, false, true, false, true);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}

	err = ext2_directory_isempty(rootblkdev, inum);
	if ((err && err != -ENOTDIR) || (err == -ENOTDIR && (flags & AT_REMOVEDIR))) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}

	spinlock_acquire(&opened_inodes_lock);
	opened_inode.inum = inum;
	opened_inode_ptr = sorted_list_search(&opened_inode, &opened_inodes,
			opened_inodes_list, opened_inodes_cmp);
	if (opened_inode_ptr) {
		opened_inode_ptr->deletemark = true;
		spinlock_release(&opened_inodes_lock);
		err = ext2_unlink_direntry_delete(rootblkdev, pathbuf, relinum);
		if (err) {
			spinlock_release(&opened_inodes_lock);
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	} else {
		spinlock_release(&opened_inodes_lock);
		
		spinlock_acquire(&fifodescs_lock);
		fifodesc.inum = inum;
		fifodesc_ptr = sorted_list_search(&fifodesc, &fifodescs,
				fifodescs_list, fifodescs_cmp);
		if (fifodesc_ptr) {
			list_del(&fifodesc_ptr->fifodescs_list);
			kpage_free(fifodesc_ptr->pipebuf);
			kfree(fifodesc_ptr);	
		}
		spinlock_release(&fifodescs_lock);

		ext2_unlink_direntry_delete(rootblkdev, pathbuf, relinum);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
		ext2_unlink_file(rootblkdev, inum);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}
	mutex_unlock(&rootblkdev->lock);
	
	return 0;
}

int sys_openat(int dirfd, const char *path, int flags, mode_t mode)
{
	int err;
	opened_inode_t opened_inode;
	fifodesc_t fifodesc;
	struct stat st;
	size_t pathlen;
	ino_t inum, relinum;
	int fd = -1;
	bool r, w, x;
	r = (flags & O_ACCMODE) != O_WRONLY;
	w = (flags & O_ACCMODE) != O_RDONLY;
	x = flags & O_EXEC;

	if (dirfd == AT_FDCWD) {
		relinum = curproc()->cwd;
	} else if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
		return -EBADFD;
	} else {
		relinum = curproc()->filetable[dirfd].inum;
	}

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
	curproc()->filetable[fd].refcnt = kmalloc(sizeof(size_t));
	if (!curproc()->filetable[fd].refcnt) {
		kfree(curproc()->filetable[fd].status_flags);
		return -ENOMEM;
	}

	mutex_lock(&rootblkdev->lock);

	if (flags & O_CREAT) {
		err = ext2_mknod(rootblkdev, pathbuf,
				S_IFREG | (mode & ~curproc()->umask), 0,
				NULL, curproc()->euid, curproc()->egid, relinum);
		if (err && (err != -EEXIST || (err == -EEXIST && (flags & O_EXCL)))) {
			mutex_unlock(&rootblkdev->lock);
			kfree(curproc()->filetable[fd].status_flags);
			kfree(curproc()->filetable[fd].refcnt);
			return err;
		}
	}
	err = ext2_file_lookup(rootblkdev, pathbuf, &inum, relinum, true,
			curproc()->euid, curproc()->egid, r, w, x, true);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		kfree(curproc()->filetable[fd].status_flags);
		kfree(curproc()->filetable[fd].refcnt);
		return err;
	}

	err = ext2_stat(rootblkdev, inum, &st);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		kfree(curproc()->filetable[fd].status_flags);
		kfree(curproc()->filetable[fd].refcnt);
		return err;
	}

	if ((st.st_mode & S_IFMT) == S_IFCHR) {
		mutex_unlock(&rootblkdev->lock);
		curproc()->filetable[fd].alloc = true;
		curproc()->filetable[fd].ondisk = true;
		curproc()->filetable[fd].ftype = S_IFCHR;
		curproc()->filetable[fd].inum = inum;
		curproc()->filetable[fd].roffset = NULL;
		curproc()->filetable[fd].woffset = NULL;
		curproc()->filetable[fd].pipebuf = NULL;
		curproc()->filetable[fd].opened_inode = NULL;
		curproc()->filetable[fd].fifodesc = NULL;
		curproc()->filetable[fd].rdev = st.st_rdev;
		*curproc()->filetable[fd].status_flags = flags;
		if (flags & O_CLOEXEC) {
			curproc()->filetable[fd].fd_flags = FD_CLOEXEC;
		} else {
			curproc()->filetable[fd].fd_flags = 0;
		}
		*curproc()->filetable[fd].refcnt = 1;
		err = character_device_driver_open(
				&curproc()->filetable[fd], flags, mode);
		if (err) {
			kfree(curproc()->filetable[fd].status_flags);
			kfree(curproc()->filetable[fd].refcnt);
			curproc()->filetable[fd].alloc = false;
			return err;
		}
		return 0;
	} else if ((st.st_mode & S_IFMT) == S_IFBLK) {
		mutex_unlock(&rootblkdev->lock);
		curproc()->filetable[fd].alloc = true;
		curproc()->filetable[fd].ondisk = true;
		curproc()->filetable[fd].ftype = S_IFBLK;
		curproc()->filetable[fd].inum = inum;
		curproc()->filetable[fd].roffset = NULL;
		curproc()->filetable[fd].woffset = NULL;
		curproc()->filetable[fd].pipebuf = NULL;
		curproc()->filetable[fd].opened_inode = NULL;
		curproc()->filetable[fd].fifodesc = NULL;
		curproc()->filetable[fd].rdev = st.st_rdev;
		if (flags & O_CLOEXEC) {
			curproc()->filetable[fd].fd_flags = FD_CLOEXEC;
		} else {
			curproc()->filetable[fd].fd_flags = 0;
		}
		*curproc()->filetable[fd].refcnt = 1;
		err = block_device_driver_open(
				&curproc()->filetable[fd], flags, mode);
		if (err) {
			kfree(curproc()->filetable[fd].status_flags);
			kfree(curproc()->filetable[fd].refcnt);
			curproc()->filetable[fd].alloc = false;
			return err;
		}
		return 0;
	}

	curproc()->filetable[fd].roffset =
		curproc()->filetable[fd].woffset = kmalloc(sizeof(off_t));
	if (!curproc()->filetable[fd].roffset) {
		mutex_unlock(&rootblkdev->lock);
		kfree(curproc()->filetable[fd].status_flags);
		kfree(curproc()->filetable[fd].refcnt);
		return -ENOMEM;
	}

	spinlock_acquire(&opened_inodes_lock);
	opened_inode.inum = inum;
	curproc()->filetable[fd].opened_inode = sorted_list_search(
			&opened_inode, &opened_inodes,
			opened_inodes_list, opened_inodes_cmp);
	if (!curproc()->filetable[fd].opened_inode) {
		curproc()->filetable[fd].opened_inode = kmalloc(sizeof(opened_inode));
		if (!curproc()->filetable[fd].opened_inode) {
			spinlock_release(&opened_inodes_lock);
			mutex_unlock(&rootblkdev->lock);
			kfree(curproc()->filetable[fd].refcnt);
			kfree(curproc()->filetable[fd].status_flags);
			kfree(curproc()->filetable[fd].roffset);
			return -ENOMEM;
		}

		curproc()->filetable[fd].opened_inode->inum = inum;
		curproc()->filetable[fd].opened_inode->deletemark = false;
		curproc()->filetable[fd].opened_inode->refcnt = 1;

		list_add(&curproc()->filetable[fd].opened_inode->opened_inodes_list,
				&opened_inodes.opened_inodes_list);
	} else {
		curproc()->filetable[fd].opened_inode->refcnt++;
	}
	spinlock_release(&opened_inodes_lock);

	if ((st.st_mode & S_IFMT) == S_IFIFO) {
		spinlock_acquire(&fifodescs_lock);
		fifodesc.inum = inum;
		curproc()->filetable[fd].fifodesc = sorted_list_search(
				&fifodesc, &fifodescs,
				fifodescs_list, fifodescs_cmp);
		if (!curproc()->filetable[fd].fifodesc) {
			curproc()->filetable[fd].fifodesc = kmalloc(sizeof(fifodesc));
			if (!curproc()->filetable[fd].fifodesc) {
				spinlock_release(&fifodescs_lock);
				mutex_unlock(&rootblkdev->lock);
				kfree(curproc()->filetable[fd].refcnt);
				kfree(curproc()->filetable[fd].status_flags);
				kfree(curproc()->filetable[fd].roffset);
				spinlock_acquire(&opened_inodes_lock);
				curproc()->filetable[fd].opened_inode->refcnt--;
				if (!curproc()->filetable[fd].opened_inode->refcnt) {
					list_del(&curproc()->filetable[fd].opened_inode->
							opened_inodes_list);
					kfree(curproc()->filetable[fd].opened_inode);
				}
				spinlock_release(&opened_inodes_lock);
				return -ENOMEM;
			}

			curproc()->filetable[fd].fifodesc->inum = inum;
			curproc()->filetable[fd].fifodesc->roffset = 0;
			curproc()->filetable[fd].fifodesc->woffset = 0;
			curproc()->filetable[fd].fifodesc->pipebuf =
				kpage_alloc(PIPEBUF_NPAGES);
			if (!curproc()->filetable[fd].fifodesc->pipebuf) {
				spinlock_release(&fifodescs_lock);
				mutex_unlock(&rootblkdev->lock);
				kfree(curproc()->filetable[fd].refcnt);
				kfree(curproc()->filetable[fd].status_flags);
				kfree(curproc()->filetable[fd].roffset);
				kfree(curproc()->filetable[fd].fifodesc);
				spinlock_acquire(&opened_inodes_lock);
				curproc()->filetable[fd].opened_inode->refcnt--;
				if (!curproc()->filetable[fd].opened_inode->refcnt) {
					list_del(&curproc()->filetable[fd].opened_inode->
							opened_inodes_list);
					kfree(curproc()->filetable[fd].opened_inode);
				}
				spinlock_release(&opened_inodes_lock);
				return -ENOMEM;
			}

			list_add(&curproc()->filetable[fd].fifodesc->fifodescs_list,
					&fifodescs.fifodescs_list);
		}
		spinlock_release(&fifodescs_lock);
	}

	if (flags & O_TRUNC) {
		if ((flags & O_ACCMODE) == O_RDONLY) {
			mutex_unlock(&rootblkdev->lock);
			kfree(curproc()->filetable[fd].refcnt);
			kfree(curproc()->filetable[fd].status_flags);
			kfree(curproc()->filetable[fd].roffset);
			spinlock_acquire(&opened_inodes_lock);
			curproc()->filetable[fd].opened_inode->refcnt--;
			if (!curproc()->filetable[fd].opened_inode->refcnt) {
				list_del(&curproc()->filetable[fd].opened_inode->
						opened_inodes_list);
				kfree(curproc()->filetable[fd].opened_inode);
			}
			spinlock_release(&opened_inodes_lock);
			return -EBADFD;
		}

		err = ext2_truncate(rootblkdev, inum, 0);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			kfree(curproc()->filetable[fd].refcnt);
			kfree(curproc()->filetable[fd].status_flags);
			kfree(curproc()->filetable[fd].roffset);
			spinlock_acquire(&opened_inodes_lock);
			curproc()->filetable[fd].opened_inode->refcnt--;
			if (!curproc()->filetable[fd].opened_inode->refcnt) {
				list_del(&curproc()->filetable[fd].opened_inode->
						opened_inodes_list);
				kfree(curproc()->filetable[fd].opened_inode);
			}
			spinlock_release(&opened_inodes_lock);
			return err;
		}
	}

	if ((flags & O_APPEND) && !(flags & O_TRUNC)) {
		*curproc()->filetable[fd].roffset = st.st_size;
	} else {
		*curproc()->filetable[fd].roffset = 0;
	}
	mutex_unlock(&rootblkdev->lock);

	curproc()->filetable[fd].alloc = true;
	curproc()->filetable[fd].ondisk = true;
	curproc()->filetable[fd].ftype = st.st_mode & S_IFMT;
	curproc()->filetable[fd].rdev = st.st_rdev;
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

int sys_getcwd(char *buf, size_t size)
{
	int err;
	char pathbuf[PATH_MAX];

	mutex_lock(&rootblkdev->lock);
	err = ext2_getcwd(rootblkdev, curproc()->cwd, pathbuf, size, curproc()->euid,
			curproc()->egid);
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

int sys_chdir(int fd, const char *path, int flags)
{
	int err;
	ino_t inum;

	if (flags & AT_EMPTY_PATH) {
		if (fd < 0 || fd >= FD_MAX || !curproc()->filetable[fd].alloc) {
			return -EBADFD;
		}
		if (!(*curproc()->filetable[fd].status_flags & O_EXEC)) {
			return -EACCES;
		}
		if (curproc()->filetable[fd].ftype != S_IFDIR) {
			return -ENOTDIR;
		}

		inum = curproc()->filetable[fd].inum;

		mutex_lock(&rootblkdev->lock);
	} else {
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

		err = ext2_file_lookup(rootblkdev, pathbuf, &inum,
				curproc()->cwd, true,
				curproc()->euid, curproc()->egid,
				false, false, true, true);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}

	err = ext2_chdir(rootblkdev, inum, &curproc()->cwd);
	mutex_unlock(&rootblkdev->lock);
	if (err) {
		return err;
	}

	return 0;
}

int sys_linkat(int olddirfd, const char *oldpath,
		int newdirfd, const char *newpath, int flags)
{
	int err;
	size_t newpathlen;
	ino_t oldrelinum, newrelinum, oldinum;
	
	if (olddirfd == AT_FDCWD) {
		oldrelinum = curproc()->cwd;
	} else if (olddirfd < 0 || olddirfd >= FD_MAX ||
			!curproc()->filetable[olddirfd].alloc) {
		return -EBADFD;
	} else {
		oldrelinum = curproc()->filetable[olddirfd].inum;
	}

	if (newdirfd == AT_FDCWD) {
		newrelinum = curproc()->cwd;
	} else if (newdirfd < 0 || newdirfd >= FD_MAX ||
			!curproc()->filetable[newdirfd].alloc) {
		return -EBADFD;
	} else {
		newrelinum = curproc()->filetable[newdirfd].inum;
	}

	if (flags & AT_EMPTY_PATH) {
		if (olddirfd < 0 || olddirfd >= FD_MAX ||
				!curproc()->filetable[olddirfd].alloc) {
			return -EBADFD;
		}
		if (!curproc()->filetable[olddirfd].ondisk) {
			return -ENOENT;
		}
		oldinum = curproc()->filetable[olddirfd].inum;

		mutex_lock(&rootblkdev->lock);
	} else {
		size_t oldpathlen;

		oldpathlen = strnlen_user(oldpath, PATH_MAX);
		if (oldpathlen > PATH_MAX) {
			return -ENAMETOOLONG;
		}
		char oldpathbuf[oldpathlen];
		if (copy_from_user(oldpathbuf, oldpath, oldpathlen)) {
			return -EFAULT;
		}

		mutex_lock(&rootblkdev->lock);

		err = ext2_file_lookup(rootblkdev, oldpathbuf, &oldinum,
				oldrelinum, flags & AT_SYMLINK_FOLLOW,
				curproc()->euid, curproc()->egid,
				false, false, false, true);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}

	newpathlen = strnlen_user(newpath, PATH_MAX);
	if (newpathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char newpathbuf[newpathlen];
	if (copy_from_user(newpathbuf, newpath, newpathlen)) {
		return -EFAULT;
	}

	err = ext2_link(rootblkdev, oldinum, newpathbuf,
			curproc()->euid, curproc()->egid, newrelinum);
	mutex_unlock(&rootblkdev->lock);
	if (err) {
		return err;
	}

	return 0;
}

int sys_renameat2(int olddirfd, const char *olddir,
		int newdirfd, const char *newdir, unsigned int flags)
{
	/*
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
			curproc()->euid, curproc()->egid, curproc()->cwd);
	if (err) {
		mutex_unlock(&rootblkdev->lock);
		return err;
	}
	mutex_unlock(&rootblkdev->lock);
	*/
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
				break;
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
				break;
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

int sys_fchmodat(int dirfd, const char *path, mode_t mode, int flags)
{
	int err;
	ino_t relinum, inum;

	if (dirfd == AT_FDCWD) {
		relinum = curproc()->cwd;
	} else if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
		return -EBADFD;
	} else {
		relinum = curproc()->filetable[dirfd].inum;
	}

	if (flags & AT_EMPTY_PATH) {
		if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
			return -EBADFD;
		}
		if (!curproc()->filetable[dirfd].ondisk) {
			return -ENOENT;
		}

		inum = curproc()->filetable[dirfd].inum;

		mutex_lock(&rootblkdev->lock);
	} else {
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

		err = ext2_file_lookup(rootblkdev, pathbuf, &inum,
				relinum, !(flags & AT_SYMLINK_NOFOLLOW),
				curproc()->euid, curproc()->egid,
				false, false, false, true);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_chmod(rootblkdev, inum, mode);
	mutex_unlock(&rootblkdev->lock);
	if (err) {
		return err;
	}

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

int sys_mknodat(int dirfd, const char *path, mode_t mode, dev_t dev,
		const char *symlink)
{
	int err;
	size_t pathlen, symlinklen;
	ino_t relinum;

	if (dirfd == AT_FDCWD) {
		relinum = curproc()->cwd;
	} else if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
		return -EBADFD;
	} else {
		relinum = curproc()->filetable[dirfd].inum;
	}

	pathlen = strnlen_user(path, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char pathbuf[pathlen];
	if (copy_from_user(pathbuf, path, pathlen)) {
		return -EFAULT;
	}

	symlinklen = strnlen_user(symlink, PATH_MAX);
	if (pathlen > PATH_MAX) {
		return -ENAMETOOLONG;
	}
	char symlinkbuf[symlinklen];
	if (copy_from_user(symlinkbuf, symlink, symlinklen)) {
		return -EFAULT;
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_mknod(rootblkdev, pathbuf,
			(mode & S_IFMT) | (mode & ~curproc()->umask), dev,
			symlinkbuf, curproc()->euid, curproc()->egid, relinum);
	mutex_unlock(&rootblkdev->lock);
	if (err) {
		return err;
	}

	return 0;
}

mode_t sys_umask(mode_t mask)
{
	mode_t oldmask = curproc()->umask;
	curproc()->umask = S_IFMT | mask;
	return oldmask;
}

int sys_fchownat(int dirfd, const char *path, uid_t uid, gid_t gid, int flags)
{
	int err;
	ino_t relinum, inum;

	if (dirfd == AT_FDCWD) {
		relinum = curproc()->cwd;
	} else if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
		return -EBADFD;
	} else {
		relinum = curproc()->filetable[dirfd].inum;
	}

	if (flags & AT_EMPTY_PATH) {
		if (dirfd < 0 || dirfd >= FD_MAX || !curproc()->filetable[dirfd].alloc) {
			return -EBADFD;
		}
		if (!curproc()->filetable[dirfd].ondisk) {
			return -ENOENT;
		}

		inum = curproc()->filetable[dirfd].inum;

		mutex_lock(&rootblkdev->lock);
	} else {
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

		err = ext2_file_lookup(rootblkdev, pathbuf, &inum,
				relinum, !(flags & AT_SYMLINK_NOFOLLOW),
				curproc()->euid, curproc()->egid,
				false, false, false, true);
		if (err) {
			mutex_unlock(&rootblkdev->lock);
			return err;
		}
	}

	mutex_lock(&rootblkdev->lock);
	err = ext2_chown(rootblkdev, inum, uid, gid);
	mutex_unlock(&rootblkdev->lock);
	if (err) {
		return err;
	}

	return 0;
}

