#ifndef KERNEL_RISCV64_MACROS_H
#define KERNEL_RISCV64_MACROS_H

#define PMPXCFG(pmpnum, flags) ((flags) << (pmpnum) * 8)

#endif

