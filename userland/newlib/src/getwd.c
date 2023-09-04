#include <stddef.h>
#include <limits.h>

char *getcwd(char *buf, size_t size);

char *getwd(char *buf)
{
	return getcwd(buf, PATH_MAX);
}

