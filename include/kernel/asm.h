#ifndef KERNEL_ASM_H
#define KERNEL_ASM_H

#include <stdint.h>

static inline uint64_t r_mstatus(void)
{
	uint64_t mstatus;
	asm volatile("csrr %0, mstatus" : "=r" (mstatus));
	return mstatus;
}

static inline void w_mstatus(uint64_t mstatus)
{
	asm volatile("csrw mstatus, %0" : : "r" (mstatus));
}

static inline void w_medeleg(uint64_t medeleg)
{
	asm volatile("csrw medeleg, %0" : : "r" (medeleg));
}

static inline void w_mideleg(uint64_t mideleg)
{
	asm volatile("csrw mideleg, %0" : : "r" (mideleg));
}

static inline void w_mepc(uint64_t mepc)
{
	asm volatile("csrw mepc, %0" : : "r" (mepc));
}

static inline void w_pmpcfg0(uint64_t pmpcfg0)
{
	asm volatile("csrw pmpcfg0, %0" : : "r" (pmpcfg0));
}

static inline void w_pmpaddr0(uint64_t pmpaddr0)
{
	asm volatile("csrw pmpaddr0, %0" : : "r" (pmpaddr0));
}

static inline uint64_t r_mhartid(void)
{
	uint64_t mhartid;
	asm volatile("csrr %0, mhartid" : "=r" (mhartid));
	return mhartid;
}

static inline uint64_t r_tp(void)
{
	uint64_t tp;
	asm volatile("mv %0, tp" : "=r" (tp));
	return tp;
}

static inline void w_tp(uint64_t tp)
{
	asm volatile("mv tp, %0" : : "r" (tp));
}

static inline void mret(void)
{
	asm volatile("mret");
}

static inline uint64_t r_sstatus(void)
{
	uint64_t sstatus;
	asm volatile("csrr %0, sstatus" : "=r" (sstatus));
	return sstatus;
}

static inline void w_sstatus(uint64_t sstatus)
{
	asm volatile("csrw sstatus, %0" : : "r" (sstatus));
}

static inline uint64_t r_sie(void)
{
	uint64_t sie;
	asm volatile("csrr %0, sie" : "=r" (sie));
	return sie;
}

static inline void w_sie(uint64_t sie)
{
	asm volatile("csrw sie, %0" : : "r" (sie));
}

static inline void w_sip(uint64_t sip)
{
	asm volatile("csrw sip, %0" : : "r" (sip));
}

static inline void w_mie(uint64_t mie)
{
	asm volatile("csrw mie, %0" : : "r" (mie));
}

static inline void w_mip(uint64_t mip)
{
	asm volatile("csrw mip, %0" : : "r" (mip));
}

static inline void w_stvec(uint64_t stvec)
{
	asm volatile("csrw stvec, %0" : : "r" (stvec));
}

static inline void w_mtvec(uint64_t mtvec)
{
	asm volatile("csrw mtvec, %0" : : "r" (mtvec));
}

static inline uint64_t r_satp(void)
{
	uint64_t satp;
	asm volatile("csrr %0, satp" : "=r" (satp));
	return satp;
}

static inline void w_satp(uint64_t satp)
{
	asm volatile("csrw satp, %0" : : "r" (satp));
}

static inline uint64_t r_sepc(void)
{
	uint64_t sepc;
	asm volatile("csrr %0, sepc" : "=r" (sepc));
	return sepc;
}

static inline void w_sepc(uint64_t sepc)
{
	asm volatile("csrw sepc, %0" : : "r" (sepc));
}

static inline uint64_t r_scause(void)
{
	uint64_t scause;
	asm volatile("csrr %0, scause" : "=r" (scause));
	return scause;
}

static inline void w_mscratch(uint64_t mscratch)
{
	asm volatile("csrw mscratch, %0" : : "r" (mscratch));
}

static inline void w_sscratch(uint64_t sscratch)
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

