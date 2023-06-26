#include <kernel/syscon-sifive.h>
#include <kernel/riscv64.h>

void syscon_poweroff(void)
{
	*((volatile u16 *) VIRT_TEST) = SYSCON_POWEROFF;
}

void syscon_reboot(void)
{
	*((volatile u16 *) VIRT_TEST) = SYSCON_REBOOT;
}

