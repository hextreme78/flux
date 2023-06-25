#include <kernel/syscon-sifive.h>
#include <kernel/riscv64.h>

void syscon_poweroff(void)
{
	*((volatile uint16_t *) VIRT_TEST) = SYSCON_POWEROFF;
}

void syscon_reboot(void)
{
	*((volatile uint16_t *) VIRT_TEST) = SYSCON_REBOOT;
}

