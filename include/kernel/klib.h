#ifndef KERNEL_KLIB_H
#define KERNEL_KLIB_H

#include <kernel/types.h>

void *memmove(void *dest, const void *src, size_t n);
void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void bzero(void *s, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

int copy_to_user(void *to, const void *from, size_t n);
int copy_from_user(void *to, const void *from, size_t n);

#endif

