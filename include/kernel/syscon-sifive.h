#ifndef KERNEL_SYSCON_SIFIVE_H
#define KERNEL_SYSCON_SIFIVE_H

#define SYSCON_POWEROFF 0x5555
#define SYSCON_REBOOT 0x7777

void syscon_poweroff(void);
void syscon_reboot(void);

#endif

