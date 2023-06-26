#ifndef KERNEL_ASM_H
#define KERNEL_ASM_H

#include <kernel/types.h>

static inline u64 r_mstatus(void)
{
	u64 mstatus;
	asm volatile("csrr %0, mstatus" : "=r" (mstatus));
	return mstatus;
}

static inline void w_mstatus(u64 mstatus)
{
	asm volatile("csrw mstatus, %0" : : "r" (mstatus));
}

static inline void w_medeleg(u64 medeleg)
{
	asm volatile("csrw medeleg, %0" : : "r" (medeleg));
}

static inline void w_mideleg(u64 mideleg)
{
	asm volatile("csrw mideleg, %0" : : "r" (mideleg));
}

static inline void w_mepc(u64 mepc)
{
	asm volatile("csrw mepc, %0" : : "r" (mepc));
}

static inline void w_pmpcfg0(u64 pmpcfg0)
{
	asm volatile("csrw pmpcfg0, %0" : : "r" (pmpcfg0));
}

static inline void w_pmpaddr0(u64 pmpaddr0)
{
	asm volatile("csrw pmpaddr0, %0" : : "r" (pmpaddr0));
}

static inline u64 r_mhartid(void)
{
	u64 mhartid;
	asm volatile("csrr %0, mhartid" : "=r" (mhartid));
	return mhartid;
}

static inline u64 r_tp(void)
{
	u64 tp;
	asm volatile("mv %0, tp" : "=r" (tp));
	return tp;
}

static inline void w_tp(u64 tp)
{
	asm volatile("mv tp, %0" : : "r" (tp));
}

static inline void mret(void)
{
	asm volatile("mret");
}

static inline u64 r_sstatus(void)
{
	u64 sstatus;
	asm volatile("csrr %0, sstatus" : "=r" (sstatus));
	return sstatus;
}

static inline void w_sstatus(u64 sstatus)
{
	asm volatile("csrw sstatus, %0" : : "r" (sstatus));
}

static inline u64 r_sie(void)
{
	u64 sie;
	asm volatile("csrr %0, sie" : "=r" (sie));
	return sie;
}

static inline void w_sie(u64 sie)
{
	asm volatile("csrw sie, %0" : : "r" (sie));
}

static inline void w_sip(u64 sip)
{
	asm volatile("csrw sip, %0" : : "r" (sip));
}

static inline void w_mie(u64 mie)
{
	asm volatile("csrw mie, %0" : : "r" (mie));
}

static inline void w_mip(u64 mip)
{
	asm volatile("csrw mip, %0" : : "r" (mip));
}

static inline void w_stvec(u64 stvec)
{
	asm volatile("csrw stvec, %0" : : "r" (stvec));
}

static inline void w_mtvec(u64 mtvec)
{
	asm volatile("csrw mtvec, %0" : : "r" (mtvec));
}

static inline u64 r_satp(void)
{
	u64 satp;
	asm volatile("csrr %0, satp" : "=r" (satp));
	return satp;
}

static inline void w_satp(u64 satp)
{
	asm volatile("csrw satp, %0" : : "r" (satp));
}

static inline u64 r_sepc(void)
{
	u64 sepc;
	asm volatile("csrr %0, sepc" : "=r" (sepc));
	return sepc;
}

static inline void w_sepc(u64 sepc)
{
	asm volatile("csrw sepc, %0" : : "r" (sepc));
}

static inline u64 r_scause(void)
{
	u64 scause;
	asm volatile("csrr %0, scause" : "=r" (scause));
	return scause;
}

static inline void w_mscratch(u64 mscratch)
{
	asm volatile("csrw mscratch, %0" : : "r" (mscratch));
}

static inline void w_sscratch(u64 sscratch)
{
	asm volatile("csrw sscratch, %0" : : "r" (sscratch));
}

static inline void wfi(void)
{
	asm volatile("wfi");
}

static inline void nop(void)
{
	asm volatile("nop");
}

#endif

