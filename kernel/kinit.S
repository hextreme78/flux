.section .kinit

.global kinit
kinit:
	csrr a0, mhartid

	/* if hartid >= NCPU then spin forever */
	li a1, NCPU
	bgeu a0, a1, spin

	/* setup stack */
	la sp, kstack
	addi a0, a0, 1
	li a1, KSTACKSIZE
	mul a0, a0, a1
	add sp, sp, a0

	/* now we can call kstart C function */
	call kstart

	spin:
		j spin
