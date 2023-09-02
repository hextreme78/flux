#include "syscall.h"

int main(void);

void _start(void)
{
	int status = main();
	_exit(status);
}

