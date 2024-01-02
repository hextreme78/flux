#include <kernel/sched.h>
#include <kernel/uart-ns16550a.h>
#include <kernel/kprintf.h>
#include <kernel/ram.h>
#include <kernel/alloc.h>
#include <kernel/irq.h>
#include <kernel/plic-sifive.h>
#include <kernel/vm.h>
#include <kernel/proc.h>
#include <kernel/virtio.h>
#include <kernel/fs.h>

static u64 cpu0_init = 0;

extern u64 elfbin_test1;
extern u64 elfbin_test2;
extern u64 elfbin_end;

void __proc_test__(void)
{
	void *elf1 = &elfbin_test1;
	size_t elf1sz = (u64) &elfbin_test2 - (u64) elfbin_test1;
	void *elf2 = &elfbin_test2;
	size_t elf2sz = (u64) &elfbin_end - (u64) elfbin_test2;

	proc_create(elf1, elf1sz);
	//proc_create(elf2, elf2sz);
}

void kmain(void)
{
	if (!cpuid()) {
		uart_init();
		kprintf_init();
		ram_init();
		alloc_init();
		irq_hart_init();
		plic_init();
		plic_hart_init();
		vm_init();
		vm_hart_init();
		proc_init();
		proc_hart_init();
		virtio_init();
		fs_init();

		/* process testing function */
		__proc_test__();

		atomic_release_membar();
		atomic_set(&cpu0_init, 1);
	} else {
		while (!atomic_test(&cpu0_init));
		atomic_acquire_membar();

		irq_hart_init();
		plic_hart_init();
		vm_hart_init();
		proc_hart_init();
	}

	scheduler();
}

