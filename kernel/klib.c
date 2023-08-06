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

int copy_to_user(void *to, const void *from, size_t n)
{
	size_t firstvpage = PA_TO_PN(to);
	size_t lastvpage = PA_TO_PN((u8 *) to + n);
	size_t ncopied = 0;
	size_t inpage_off, inpage_len;

	for (size_t curvpage = firstvpage; curvpage <= lastvpage; curvpage++) {
		u8 *curppage_addr;
		pte_t *pte = vm_getpte(curproc()->upagetable, curvpage);	
		if (!pte || !pte->u || !pte->w) {
			return -1;
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

int copy_from_user(void *to, const void *from, size_t n)
{
	size_t firstvpage = PA_TO_PN(from);
	size_t lastvpage = PA_TO_PN((u8 *) from + n);
	size_t ncopied = 0;
	size_t inpage_off, inpage_len;

	for (size_t curvpage = firstvpage; curvpage <= lastvpage; curvpage++) {
		u8 *curppage_addr;
		pte_t *pte = vm_getpte(curproc()->upagetable, curvpage);	
		if (!pte || !pte->u || !pte->r) {
			return -1;
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

size_t strlen(const char *s)
{
	size_t len = 0;
	while (*s++) len++;
	return len;
}

