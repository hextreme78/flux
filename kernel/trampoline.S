#include <kernel/riscv64_defs.h>
#include <kernel/memlayout.h>

.section .trampoline

# return from context_switch here
# when entering process first time
.global __trampoline_userret
__trampoline_userret:
	# load trapframe address
	li a0, VA_TRAPFRAME

	# set user stack
	ld sp, 8(a0)

	# make satp from userpagetable
	ld t0, 304(a0)
	srli t0, t0, 12
	li t1, SATP_MODE_SV39
	or t0, t0, t1

	# set userpagetable
	sfence.vma x0, x0
	csrw satp, t0
	sfence.vma x0, x0

	sret

.global __trampoline_usertrap
.align RISCV64_ISR_ALIGN
__trampoline_usertrap:
	# save registers in trapframe
	csrw sscratch, a0
	li a0, VA_TRAPFRAME
	sd ra, 0(a0)
	sd sp, 8(a0)
	sd gp, 16(a0)
	sd tp, 24(a0)
	sd t0, 32(a0)
	sd t1, 40(a0)
	sd t2, 48(a0)
	sd fp, 56(a0)
	sd s1, 64(a0)
	sd a1, 80(a0)
	sd a2, 88(a0)
	sd a3, 96(a0)
	sd a4, 104(a0)
	sd a5, 112(a0)
	sd a6, 120(a0)
	sd a7, 128(a0)
	sd s2, 136(a0)
	sd s3, 144(a0)
	sd s4, 152(a0)
	sd s5, 160(a0)
	sd s6, 168(a0)
	sd s7, 176(a0)
	sd s8, 184(a0)
	sd s9, 192(a0)
	sd s10, 200(a0)
	sd s11, 208(a0)
	sd t3, 216(a0)
	sd t4, 224(a0)
	sd t5, 232(a0)
	sd t6, 240(a0)
	csrr t0, sscratch
	sd t0, 72(a0)

	# save epc address
	csrr t0, sepc
	sd t0, 248(a0)

	# restore cpuid
	ld tp, 256(a0)

	# set kstack
	ld sp, 264(a0)

	# set kerneltrap
	ld t0, 272(a0)
	csrw stvec, t0

	# make satp from kpagetable address
	ld t0, 280(a0)
	srli t0, t0, 12
	li t1, SATP_MODE_SV39
	or t0, t0, t1

	# set kpagetable
	sfence.vma x0, x0
	csrw satp, t0
	sfence.vma x0, x0

	# load address of user_irq_handler
	ld a0, 288(a0)

	# jump in user_irq_handler
	jalr a0

	# set sstatus.spie and reset sstatus.spp
	csrr a0, sstatus
	ori a0, a0, SSTATUS_SPIE
	li a1, SSTATUS_SPP
	not a1, a1
	and a0, a0, a1
	csrw sstatus, a0

	# load trapframe address
	li a0, VA_TRAPFRAME

	# set usertrap
	ld t0, 296(a0)
	csrw stvec, t0

	# make satp from userpagetable
	ld t0, 304(a0)
	srli t0, t0, 12
	li t1, SATP_MODE_SV39
	or t0, t0, t1

	# set userpagetable
	sfence.vma x0, x0
	csrw satp, t0
	sfence.vma x0, x0

	# set epc address
	ld t0, 248(a0)
	csrw sepc, t0

	# save cpuid
	sd tp, 256(a0)

	# restore process registers from trapframe
	ld ra, 0(a0)
	ld sp, 8(a0)
	ld gp, 16(a0)
	ld tp, 24(a0)
	ld t0, 32(a0)
	ld t1, 40(a0)
	ld t2, 48(a0)
	ld fp, 56(a0)
	ld s1, 64(a0)
	ld a1, 80(a0)
	ld a2, 88(a0)
	ld a3, 96(a0)
	ld a4, 104(a0)
	ld a5, 112(a0)
	ld a6, 120(a0)
	ld a7, 128(a0)
	ld s2, 136(a0)
	ld s3, 144(a0)
	ld s4, 152(a0)
	ld s5, 160(a0)
	ld s6, 168(a0)
	ld s7, 176(a0)
	ld s8, 184(a0)
	ld s9, 192(a0)
	ld s10, 200(a0)
	ld s11, 208(a0)
	ld t3, 216(a0)
	ld t4, 224(a0)
	ld t5, 232(a0)
	ld t6, 240(a0)
	ld a0, 72(a0)

	sret

