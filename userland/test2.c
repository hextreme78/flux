#include <stddef.h>
#include "syscall.h"

int main(void)
{
	syscall(101);

	for (size_t i = 0; i < 10000000; i++)
		asm volatile("nop;");

	syscall(101);

	return 0;
}

