#include <kernel/memlayout.h>
#define SIE_STIE (1 << 5)

.section .text

# mscratch contains addr of tscratch in ram
# tscratch[0] contains mtime addr
# tscratch[1] contains hart's mtimecmp addr
# tscratch[2] contains NCYCLE
# tscratch[3, 4, 5, 6] for saving a1, a2, a3, a4 registers

# we should update mtimecmp by adding NCYCLE
# to mtime to schedule next timer interrupt

# we should also set stie bit to trigger s-mode 
# timer interrupt handler immediately after mret
.global timertrap
.align 4
timertrap:
	csrrw a0, mscratch, a0

	sd a1, 24(a0)
	sd a2, 32(a0)
	sd a3, 40(a0)
	sd a4, 48(a0)

	ld a1, 0(a0)
	ld a2, 8(a0)
	ld a3, 16(a0)

	ld a4, 0(a1)
	add a4, a4, a3
	sd a4, 0(a2)

	csrr a1, sie
	ori a1, a1, SIE_STIE
	csrw sie, a1

	ld a1, 24(a0)
	ld a2, 32(a0)
	ld a3, 40(a0)
	ld a4, 48(a0)

	csrrw a0, mscratch, a0

	mret

# we want to stop ram probing because of load fault
# just set raminitstop to 1 and increment sepc
.global ramtrap
.align 4
ramtrap:
	addi sp, sp, -16
	sd a0, 0(sp)
	sd a1, 8(sp)

	la a0, raminitstop
	addi a1, x0, 1
	sd a1, (a0)

	csrr a0, sepc
	addi a0, a0, 4
	csrw sepc, a0

	ld a0, 0(sp)
	ld a1, 8(sp)
	addi sp, sp, 16

	sret

.global kerneltrap
.align 4
kerneltrap:
	# save caller-saved registers
	addi sp, sp, -128
	sd ra, 0(sp)
	sd a0, 8(sp)
	sd a1, 16(sp)
	sd a2, 24(sp)
	sd a3, 32(sp)
	sd a4, 40(sp)
	sd a5, 48(sp)
	sd a6, 56(sp)
	sd a7, 64(sp)
	sd t0, 72(sp)
	sd t1, 80(sp)
	sd t2, 88(sp)
	sd t3, 96(sp)
	sd t4, 104(sp)
	sd t5, 112(sp)
	sd t6, 120(sp)

	# handle interrupt
	call kernel_irq_handler

	# restore caller-saved registers
	ld ra, 0(sp)
	ld a0, 8(sp)
	ld a1, 16(sp)
	ld a2, 24(sp)
	ld a3, 32(sp)
	ld a4, 40(sp)
	ld a5, 48(sp)
	ld a6, 56(sp)
	ld a7, 64(sp)
	ld t0, 72(sp)
	ld t1, 80(sp)
	ld t2, 88(sp)
	ld t3, 96(sp)
	ld t4, 104(sp)
	ld t5, 112(sp)
	ld t6, 120(sp)
	addi sp, sp, 128

	sret

.section .trampoline

.global usertrap
.align 4
usertrap:
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
	csrr a1, sscratch
	sd a1, 72(a0)

	# set kernel stack
	ld sp, 248(a0)

	# restore cpuid in tp
	ld tp, 256(a0)

	# load addr of kernel pagetable
	ld a1, 264(a0)

	# load addr of user_irq_handler()
	ld a2, 272(a0)

	# set kernel pagetable
	sfence.vma x0, x0
	csrw satp, a1

	# jump to user_irq_handler()
	jalr a2

	# restore process pagetable
	csrr a0, sscratch
	sfence.vma x0, x0
	csrw satp, a0

	# restore process registers from trapframe
	li a0, VA_TRAPFRAME
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

	# return to userspace
	sret

