#include <kernel/klib.h>

void *memmove(void *dest, const void *src, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((char *) dest)[i] = ((char *) src)[i];
	}
	return dest;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((char *) dest)[i] = ((char *) src)[i];
	}
	return dest;
}

void *memset(void *s, int c, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((char *) s)[i] = c;
	}
	return s;
}

void bzero(void *s, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((char *) s)[i] = 0;
	}
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		char c1 = ((char *) s1)[i];
		char c2 = ((char *) s2)[i];
		if (c1 != c2) {
			return c1 - c2;
		}
	}
	return 0;
}

