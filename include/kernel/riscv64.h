#ifndef KERNEL_RISCV64_H
#define KERNEL_RISCV64_H

#include <stdint.h>
#include <kernel/asm.h>
#include <kernel/atomic.h>
#include <kernel/tlb.h>
#include <kernel/platform-virt.h>

#define MSTATUS_MPP_MASK (3 << 11)
#define MSTATUS_MPP_S (1 << 11)

#define SSTATUS_SPP_S (1 << 8)
#define SSTATUS_SPIE (1 << 5)

#define PMPXCFG(pmpnum, flags) ((flags) << (pmpnum) * 8)
#define PMPXCFG_R 1
#define PMPXCFG_W (1 << 1)
#define PMPXCFG_X (1 << 2)
#define PMPXCFG_A_OFF 0
#define PMPXCFG_A_TOR (1 << 3)
#define PMPXCFG_A_NA4 (2 << 3)
#define PMPXCFG_A_NAPOT (3 << 3)
#define PMPXCFG_L (1 << 7)

#define SSTATUS_SIE (1 << 1)

#define MSTATUS_MIE (1 << 3)
#define MSTATUS_MPIE (1 << 7)

#define STVEC_MODE_DIRECT 0
#define STVEC_MODE_VECTORED 1

#define MTVEC_MODE_DIRECT 0
#define MTVEC_MODE_VECTORED 1

#define SIE_SSIE (1 << 1)
#define SIE_STIE (1 << 5)
#define SIE_SEIE (1 << 9)

#define SIP_SSIP (1 << 1)

#define MIP_STIP (1 << 5)

#define MIE_MTIE (1 << 7)

#define SCAUSE_INTERRUPT_MASK (1ul << 63)
#define SCAUSE_EXCEPTION_CODE_MASK (~(1ul << 63))

#define SCAUSE_SOFTWARE_IRQ 1
#define SCAUSE_TIMER_IRQ 5
#define SCAUSE_EXTERNAL_IRQ 9

#define SCAUSE_EXCEPTION_INSTURCTION_ADDRESS_MISALIGNED 0
#define SCAUSE_EXCEPTION_INSTURCTION_ACCESSS_FAULT 1
#define SCAUSE_EXCEPTION_ILLEGAL_INSTURCTION 2
#define SCAUSE_EXCEPTION_BREAKPOINT 3
#define SCAUSE_EXCEPTION_LOAD_ADDRESS_MISALIGNED 4
#define SCAUSE_EXCEPTION_LOAD_ACCESS_FAULT 5
#define SCAUSE_EXCEPTION_STORE_ADDRESS_MISALIGNED 6
#define SCAUSE_EXCEPTION_STORE_ACCESS_FAULT 7
#define SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_UMODE 8
#define SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_SMODE 9
#define SCAUSE_EXCEPTION_INSTURCTION_PAGE_FAULT 12
#define SCAUSE_EXCEPTION_LOAD_PAGE_FAULT 13
#define SCAUSE_EXCEPTION_STORE_PAGE_FAULT 15

#define SATP_MODE_MASK (0xful << 60)
#define SATP_ASID_MASK (0xfffful << 44)
#define SATP_PPN_MASK (~(0xffffful << 44))

#define SATP_MODE_BARE 0
#define SATP_MODE_SV39 (8ull << 60)
#define SATP_MODE_SV48 (9ull << 60)
#define SATP_MODE_SV57 (10ull << 60)
#define SATP_MODE_SV64 (11ull << 60)

#endif

