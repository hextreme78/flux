#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <kernel/types.h>

typedef u32 mode_t;

/* file permissions */
#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00030
#define S_IXGRP 00020
#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#endif

