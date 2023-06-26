#ifndef KERNEL_KPRINTF_H
#define KERNEL_KPRINTF_H

#include <kernel/types.h>

extern u64 paniced;

void kprintf_init(void);

/*__attribute__((format(printf, 1, 2)))*/
void kprintf(const char *fmt, ...);

/*__attribute__((format(printf, 1, 2)))*/
void kprintf_s(const char *fmt, ...);

void panic(const char *msg);

#endif

