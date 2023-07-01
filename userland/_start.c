#include "syscall.h"

int main(void);

/*
__attribute__((naked))
void _start(void)
{
	asm volatile("spin:\nj spin;");
}
*/
void _start(void)
{
	int status = main();
	_exit(status);
}
