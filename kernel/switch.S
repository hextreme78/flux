.section .text

.global switch_to_user
switch_to_user:
	# save scheduler context
	sd ra, 0(a0)
	sd sp, 8(a0)
	sd gp, 16(a0)
	sd tp, 24(a0)
	sd t0, 32(a0)
	sd t1, 40(a0)
	sd t2, 48(a0)
	sd fp, 56(a0)
	sd s1, 64(a0)
	sd a0, 72(a0)
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

	# prepare for userret
	call switch_userret_prepare

.global switch_to_scheduler_and_save
switch_to_scheduler_and_save:
	# save irq context
	sd ra, 0(a1)
	sd sp, 8(a1)
	sd gp, 16(a1)
	sd tp, 24(a1)
	sd t0, 32(a1)
	sd t1, 40(a1)
	sd t2, 48(a1)
	sd fp, 56(a1)
	sd s1, 64(a1)
	sd a0, 72(a1)
	sd a1, 80(a1)
	sd a2, 88(a1)
	sd a3, 96(a1)
	sd a4, 104(a1)
	sd a5, 112(a1)
	sd a6, 120(a1)
	sd a7, 128(a1)
	sd s2, 136(a1)
	sd s3, 144(a1)
	sd s4, 152(a1)
	sd s5, 160(a1)
	sd s6, 168(a1)
	sd s7, 176(a1)
	sd s8, 184(a1)
	sd s9, 192(a1)
	sd s10, 200(a1)
	sd s11, 208(a1)
	sd t3, 216(a1)
	sd t4, 224(a1)
	sd t5, 232(a1)
	sd t6, 240(a1)
	# save ra in process context epc
	sd ra, 248(a1)

	# restore scheduler context
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

	# return to scheduler
	sret

.global switch_to_scheduler
switch_to_scheduler:
	# restore scheduler context
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

	# return to scheduler
	sret

.global switch_irqret
switch_irqret:
	# restore process context
	ld ra, 0(a0)
	ld sp, 8(a0)
	ld gp, 16(a0)

	# we must not restore tp because
	# it contains new hartid
	# ld tp, 24(a0)

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

	# return to irq handler
	sret

.section .trampoline

.global switch_userret
switch_userret:
	# save process pagetable addr in sscratch
	csrw sscratch, a1

	# restore process context
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

	# swap satp value in sscratch and a0
	csrrw a0, sscratch, a0
	
	# write a0 to satp
	sfence.vma x0, x0
	csrw satp, a0

	# swap again to restore a0
	csrrw a0, sscratch, a0

	# return to userspace
	sret

