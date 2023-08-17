#ifndef KERNEL_KLIB_H
#define KERNEL_KLIB_H

#include <kernel/types.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

void *memmove(void *dest, const void *src, size_t n);
void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void bzero(void *s, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *restrict dst, const char *restrict src);

int copy_to_user(void *to, const void *from, size_t n);
int copy_from_user(void *to, const void *from, size_t n);

#endif

