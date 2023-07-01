#include <kernel/riscv64_defs.h>

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
	# save registers
	addi sp, sp, -256
	sd ra, 0(sp)
	sd sp, 8(sp)
	sd gp, 16(sp)
	# tp contains cpuid
	# we do not need to save it
	#sd tp, 24(sp)
	sd t0, 32(sp)
	sd t1, 40(sp)
	sd t2, 48(sp)
	sd fp, 56(sp)
	sd s1, 64(sp)
	sd a0, 72(sp)
	sd a1, 80(sp)
	sd a2, 88(sp)
	sd a3, 96(sp)
	sd a4, 104(sp)
	sd a5, 112(sp)
	sd a6, 120(sp)
	sd a7, 128(sp)
	sd s2, 136(sp)
	sd s3, 144(sp)
	sd s4, 152(sp)
	sd s5, 160(sp)
	sd s6, 168(sp)
	sd s7, 176(sp)
	sd s8, 184(sp)
	sd s9, 192(sp)
	sd s10, 200(sp)
	sd s11, 208(sp)
	sd t3, 216(sp)
	sd t4, 224(sp)
	sd t5, 232(sp)
	sd t6, 240(sp)

	# save epc on stack
	csrr t0, sepc
	sd t0, 248(sp)

	# handle interrupt
	call kernel_irq_handler

	# restore epc from stack
	ld t0, 248(sp)
	csrw sepc, t0

	# restore registers
	ld ra, 0(sp)
	ld sp, 8(sp)
	ld gp, 16(sp)
	# tp contains cpuid
	# we do not need to restore it
	#ld tp, 24(sp)
	ld t0, 32(sp)
	ld t1, 40(sp)
	ld t2, 48(sp)
	ld fp, 56(sp)
	ld s1, 64(sp)
	ld a0, 72(sp)
	ld a1, 80(sp)
	ld a2, 88(sp)
	ld a3, 96(sp)
	ld a4, 104(sp)
	ld a5, 112(sp)
	ld a6, 120(sp)
	ld a7, 128(sp)
	ld s2, 136(sp)
	ld s3, 144(sp)
	ld s4, 152(sp)
	ld s5, 160(sp)
	ld s6, 168(sp)
	ld s7, 176(sp)
	ld s8, 184(sp)
	ld s9, 192(sp)
	ld s10, 200(sp)
	ld s11, 208(sp)
	ld t3, 216(sp)
	ld t4, 224(sp)
	ld t5, 232(sp)
	ld t6, 240(sp)
	addi sp, sp, 256

	sret

