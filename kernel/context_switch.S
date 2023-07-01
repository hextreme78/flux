.section .text

.global context_switch
context_switch:
	# save previous process context
	sd ra, 0(a0)
	sd sp, 8(a0)
	sd gp, 16(a0)
	sd t0, 24(a0)
	sd t1, 32(a0)
	sd t2, 40(a0)
	sd fp, 48(a0)
	sd s1, 56(a0)
	sd a0, 64(a0)
	sd a1, 72(a0)
	sd a2, 80(a0)
	sd a3, 88(a0)
	sd a4, 96(a0)
	sd a5, 104(a0)
	sd a6, 112(a0)
	sd a7, 120(a0)
	sd s2, 128(a0)
	sd s3, 136(a0)
	sd s4, 144(a0)
	sd s5, 152(a0)
	sd s6, 160(a0)
	sd s7, 168(a0)
	sd s8, 176(a0)
	sd s9, 184(a0)
	sd s10, 192(a0)
	sd s11, 200(a0)
	sd t3, 208(a0)
	sd t4, 216(a0)
	sd t5, 224(a0)
	sd t6, 232(a0)

	csrw sscratch, a1
	call context_switch_prepare
	csrr a1, sscratch

	# restore next process context
	ld ra, 0(a1)
	ld sp, 8(a1)
	ld gp, 16(a1)
	ld t0, 24(a1)
	ld t1, 32(a1)
	ld t2, 40(a1)
	ld fp, 48(a1)
	ld s1, 56(a1)
	ld a0, 64(a1)
	ld a2, 80(a1)
	ld a3, 88(a1)
	ld a4, 96(a1)
	ld a5, 104(a1)
	ld a6, 112(a1)
	ld a7, 120(a1)
	ld s2, 128(a1)
	ld s3, 136(a1)
	ld s4, 144(a1)
	ld s5, 152(a1)
	ld s6, 160(a1)
	ld s7, 168(a1)
	ld s8, 176(a1)
	ld s9, 184(a1)
	ld s10, 192(a1)
	ld s11, 200(a1)
	ld t3, 208(a1)
	ld t4, 216(a1)
	ld t5, 224(a1)
	ld t6, 232(a1)
	ld a1, 72(a1)

	sret

