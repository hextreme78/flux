#include <stddef.h>
#include "syscall.h"

int main(void)
{
	syscall(100);

	for (size_t i = 0; i < 10000000; i++)
		asm volatile("nop;");

	syscall(100);

	return 0;
}

