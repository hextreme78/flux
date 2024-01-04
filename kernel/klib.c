#include <kernel/klib.h>
#include <kernel/vm.h>
#include <kernel/proc.h>

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

size_t strlen(const char *s)
{
	size_t len = 0;
	while (*s++) len++;
	return len;
}

size_t strnlen(const char *s, size_t n)
{
	size_t len = 0;
	while (*s++) {
		if (len >= n) {
			break;
		}
		len++;
	}
	return len;
}

int strcmp(const char *s1, const char *s2)
{
	for (size_t i = 0; s1[i] || s2[i]; i++) {
		if (s1[i] != s2[i]) {
			return s1[i] - s2[i];
		}
	}
	return 0;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	for (size_t i = 0; (s1[i] || s2[i]) && i < n; i++) {
		if (s1[i] != s2[i]) {
			return s1[i] - s2[i];
		}
	}
	return 0;
}

char *strcpy(char *restrict dst, const char *restrict src)
{
	size_t i;
	for (i = 0; src[i]; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
	return dst + i;
}

char *strncpy(char *restrict dst, const char *restrict src, size_t n)
{
	size_t i;
	for (i = 0; src[i] && i < n - 1; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
	return dst + i;
}

char *basename(char *path)
{
	size_t i = 0;
	if (!path || !path[0]) {
		return ".";
	}
	i = strlen(path) - 1;
	while (i && path[i] == '/') i--;
	path[i + 1] = '\0';
	while (i && path[i - 1] != '/') i--;
	return path + i;
}

char *dirname(char *path)
{
	size_t i;
	if (!path || !path[0]) {
		return ".";
	}
	i = strlen(path) - 1;
	while (path[i] == '/') {
		if (!i) {
			return "/";
		}
		i--;
	}
	while (path[i] != '/') {
		if (!i) {
			return ".";
		}
		i--;
	}
	while (path[i] == '/') {
		if (!i) {
			return "/";
		}
		i--;
	}
	path[i + 1] = '\0';
	return path;
}

size_t copy_to_user(void *to, const void *from, size_t n)
{
	size_t firstvpage = PA_TO_PN(to),
		lastvpage = PA_TO_PN((u8 *) to + n),
		ncopied = 0, inpage_len;
	off_t inpage_off;

	for (size_t curvpage = firstvpage; curvpage <= lastvpage; curvpage++) {
		u8 *curppage_addr;
		pte_t *pte = vm_getpte(curproc()->upagetable, curvpage);	
		if (!pte || !pte->u || !pte->w) {
			return n - ncopied;
		}
		curppage_addr = (u8 *) PN_TO_PA(pte->ppn);

		if (curvpage == firstvpage) {
			inpage_off = (size_t) to - PAGEDOWN(to);
			if (PAGEUP(to) > (size_t) to + n) {
				inpage_len = n;
			} else {
				inpage_len = PAGEUP(to) - (size_t) to;
			}
		} else if (curvpage == lastvpage) {
			inpage_off = 0;
			inpage_len = n - ncopied;
		} else {
			inpage_off = 0;
			inpage_len = PAGESZ;
		}

		memcpy(curppage_addr + inpage_off, (u8 *) from + ncopied, inpage_len);
		ncopied += inpage_len;
	}

	return 0;
}

size_t copy_from_user(void *to, const void *from, size_t n)
{
	size_t firstvpage = PA_TO_PN(from),
		lastvpage = PA_TO_PN((u8 *) from + n),
		ncopied = 0, inpage_len;
	off_t inpage_off;

	for (size_t curvpage = firstvpage; curvpage <= lastvpage; curvpage++) {
		u8 *curppage_addr;
		pte_t *pte = vm_getpte(curproc()->upagetable, curvpage);	
		if (!pte || !pte->u || !pte->r) {
			return n - ncopied;
		}
		curppage_addr = (u8 *) PN_TO_PA(pte->ppn);

		if (curvpage == firstvpage) {
			inpage_off = (size_t) from - PAGEDOWN(from);
			if (PAGEUP(from) > (size_t) from + n) {
				inpage_len = n;
			} else {
				inpage_len = PAGEUP(from) - (size_t) from;
			}
		} else if (curvpage == lastvpage) {
			inpage_off = 0;
			inpage_len = n - ncopied;
		} else {
			inpage_off = 0;
			inpage_len = PAGESZ;
		}

		memcpy((u8 *) to + ncopied, curppage_addr + inpage_off, inpage_len);
		ncopied += inpage_len;
	}

	return 0;
}

/* On success returns size of string with terminating null.
 * On error returns 0.
 */
size_t strlen_user(const char *str)
{
	size_t firstvpage = PA_TO_PN(str),
		curvpage = firstvpage,
		len = 0, inpage_len;
	off_t inpage_off;

	while (1) {
		u8 *curppage_addr;
		pte_t *pte = vm_getpte(curproc()->upagetable, curvpage);	
		if (!pte || !pte->u || !pte->r) {
			return 0;
		}
		curppage_addr = (u8 *) PN_TO_PA(pte->ppn);

		if (curvpage == firstvpage) {
			inpage_off = (size_t) str - PAGEDOWN(str);
			inpage_len = PAGEUP(str) - (size_t) str;
		} else {
			inpage_off = 0;
			inpage_len = PAGESZ;
		}

		size_t i = 0;
		char *s = (void *) curppage_addr + inpage_off;
		while (i < inpage_len) {
			if (s[i]) {
				len++;
			} else {
				return len + 1;
			}
			i++;
		}

		curvpage++;
	}

	return 0;
}

/* n is maximum length of string with terminating null.
 * On success returns size of string with terminating null
 * or if string is longer than n, returns value greater than n.
 * On error returns 0.
 */
size_t strnlen_user(const char *str, size_t n)
{
	size_t firstvpage = PA_TO_PN(str),
		curvpage = firstvpage,
		len = 0, inpage_len;
	off_t inpage_off;

	while (1) {
		u8 *curppage_addr;
		pte_t *pte = vm_getpte(curproc()->upagetable, curvpage);	
		if (!pte || !pte->u || !pte->r) {
			return 0;
		}
		curppage_addr = (u8 *) PN_TO_PA(pte->ppn);

		if (curvpage == firstvpage) {
			inpage_off = (size_t) str - PAGEDOWN(str);
			inpage_len = PAGEUP(str) - (size_t) str;
		} else {
			inpage_off = 0;
			inpage_len = PAGESZ;
		}

		size_t i = 0;
		char *s = (void *) curppage_addr + inpage_off;
		while (i < inpage_len) {
			if (len >= n) {
				return n + 1;
			}
			if (s[i]) {
				len++;
			} else {
				return len + 1;
			}
			i++;
		}

		curvpage++;
	}

	return 0;
}

size_t strncpy_from_user(char *to, const char *from, size_t n)
{
	size_t firstvpage = PA_TO_PN(from),
		lastvpage = PA_TO_PN((u8 *) from + n),
		ncopied = 0, inpage_len;
	off_t inpage_off;

	for (size_t curvpage = firstvpage; curvpage <= lastvpage; curvpage++) {
		u8 *curppage_addr;
		pte_t *pte = vm_getpte(curproc()->upagetable, curvpage);	
		if (!pte || !pte->u || !pte->r) {
			return -EFAULT;
		}
		curppage_addr = (u8 *) PN_TO_PA(pte->ppn);

		if (curvpage == firstvpage) {
			inpage_off = (size_t) from - PAGEDOWN(from);
			if (PAGEUP(from) > (size_t) from + n) {
				inpage_len = n;
			} else {
				inpage_len = PAGEUP(from) - (size_t) from;
			}
		} else if (curvpage == lastvpage) {
			inpage_off = 0;
			inpage_len = n - ncopied;
		} else {
			inpage_off = 0;
			inpage_len = PAGESZ;
		}
	
		size_t i = 0;
		char *dst = to + ncopied;
		char *src = (void *) curppage_addr + inpage_off;
		while (i < inpage_len) {
			if (src[i] || ncopied + i == n - 1) {
				dst[i] = '\0';
				return ncopied + i;
			}

			dst[i] = src[i];
			i++;
		}

		ncopied += inpage_len;
	}

	return ncopied;
}

size_t memset_user(void *to, int c, size_t n)
{
	size_t firstvpage = PA_TO_PN(to),
		lastvpage = PA_TO_PN((u8 *) to + n),
		ncopied = 0, inpage_len;
	off_t inpage_off;

	for (size_t curvpage = firstvpage; curvpage <= lastvpage; curvpage++) {
		u8 *curppage_addr;
		pte_t *pte = vm_getpte(curproc()->upagetable, curvpage);	
		if (!pte || !pte->u || !pte->w) {
			return n - ncopied;
		}
		curppage_addr = (u8 *) PN_TO_PA(pte->ppn);

		if (curvpage == firstvpage) {
			inpage_off = (size_t) to - PAGEDOWN(to);
			if (PAGEUP(to) > (size_t) to + n) {
				inpage_len = n;
			} else {
				inpage_len = PAGEUP(to) - (size_t) to;
			}
		} else if (curvpage == lastvpage) {
			inpage_off = 0;
			inpage_len = n - ncopied;
		} else {
			inpage_off = 0;
			inpage_len = PAGESZ;
		}

		memset(curppage_addr + inpage_off, c, inpage_len);

		ncopied += inpage_len;
	}

	return 0;
}

size_t bzero_user(void *to, size_t n)
{
	return memset_user(to, '\0', n);
}

