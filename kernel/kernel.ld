ENTRY(kinit)

. = 0x80000000;

SECTIONS
{
	.text : ALIGN(4K) {
		PROVIDE(ktext = .);
		*(.kinit)
		*(.text*)

		. = ALIGN(4K);
		PROVIDE(trampoline = .);
		*(.trampoline)
		ASSERT(. - trampoline <= 4K,
		"error: trampoline must be less than one page");
	}

	.rodata : ALIGN(4K) {
		PROVIDE(krodata = .);
		*(.rodata*)
		*(.srodata*)

		/* include elf binaries for elf loader test */
		PROVIDE(elfbin_test1 = .);
		*(.elfbin_test1)
		PROVIDE(elfbin_test2 = .);
		*(.elfbin_test2)
		PROVIDE(elfbin_end = .);
	}
	
	.data : ALIGN(4K) {
		PROVIDE(kdata = .);
		*(.data*)
		*(.sdata*)
	}

	.bss : ALIGN(4K) {
		PROVIDE(kbss = .);
		*(.bss*)
		*(.sbss*)
	}

	PROVIDE(kend = ALIGN(4K));
}

