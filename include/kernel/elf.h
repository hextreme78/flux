#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include <elf.h>

#include <kernel/proc.h>
#include <kernel/errno.h>

int elf_load(proc_t *proc, void *elf, size_t elfsz);

#endif

