#include <stddef.h>

void _start(void)
{
	main();
}

void main(void)
{
	asm volatile("li a7, 0; ecall;" : : : "a7");

	for (size_t i = 0; i < 10000000; i++)
		asm volatile("nop;");

	asm volatile("li a7, 0; ecall;" : : : "a7");

	while (1);
}

