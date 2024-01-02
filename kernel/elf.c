#include <kernel/elf.h>
#include <kernel/alloc.h>
#include <kernel/klib.h>

static Elf64_Ehdr *elf_ehdr_isvalid(void *elf, size_t elfsz)
{
	Elf64_Ehdr *ehdr = elf;

	/* check sizes */
	if (elfsz < sizeof(Elf64_Ehdr)) {
		return NULL;	
	}
	if (ehdr->e_ehsize < sizeof(Elf64_Ehdr)) {
		return NULL;
	}
	if (ehdr->e_phentsize < sizeof(Elf64_Phdr)) {
		return NULL;
	}
	if (!ehdr->e_phoff) {
		return NULL;
	}
	if (elfsz < ehdr->e_phoff) {
		return NULL;
	}
	if (!ehdr->e_entry) {
		return NULL;
	}

	/* magic check */
	if (ehdr->e_ident[EI_MAG0] != ELFMAG0) {
		return NULL;
	}
	if (ehdr->e_ident[EI_MAG1] != ELFMAG1) {
		return NULL;
	}
	if (ehdr->e_ident[EI_MAG2] != ELFMAG2) {
		return NULL;
	}
	if (ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		return NULL;
	}

	/* 64 bits check */
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
		return NULL;
	}

	/* lsb check */
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		return NULL;
	}

	/* version check */
	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
		return NULL;
	}

	/* type check */
	if (ehdr->e_type != ET_EXEC) {
		return NULL;
	}

	return ehdr;
}

static Elf64_Phdr *elf_phdr_next(Elf64_Ehdr *ehdr, Elf64_Phdr *phdr, size_t elfsz)
{
	size_t cursize, curphcount;
	if (!phdr) {
		return (Elf64_Phdr *) ((char *) ehdr + ehdr->e_phoff);
	}

	cursize = (size_t) phdr - (size_t) ehdr + ehdr->e_phentsize + 1;
	curphcount = (cursize - ehdr->e_ehsize) / ehdr->e_phentsize;
	if (curphcount >= ehdr->e_phnum) {
		return NULL;
	}
	if (elfsz < cursize + ehdr->e_phentsize) {
		return NULL;
	}

	return phdr + 1;
}

static int elf_phdr_load(Elf64_Ehdr *ehdr, Elf64_Phdr *phdr, size_t elfsz,
		proc_t *proc)
{
	int err;
	u64 vstart = PAGEDOWN(phdr->p_vaddr);
	u64 vend = PAGEROUND(phdr->p_vaddr + phdr->p_memsz);
	u64 vlen = vend - vstart;
	u64 npages = vlen / PAGESZ;

	if (phdr->p_filesz > phdr->p_memsz) {
		return -ENOEXEC;
	}

	u64 pstart = (u64) kpage_alloc(npages);
	if (!pstart) {
		return -ENOMEM;
	}

	segment_t *segment = kmalloc(sizeof(segment_t));
	if (!segment) {
		kpage_free((void *) pstart);
		return -ENOMEM;
	}

	list_add(&segment->segments, &proc->segment_list.segments);
	segment->pstart = pstart;
	segment->vstart = vstart;
	segment->vlen = vlen;

	u64 pstart_offset = pstart + phdr->p_vaddr - vstart;
	u64 fstart_offset = (u64) ehdr + phdr->p_offset;
	memcpy((void *) pstart_offset, (void *) fstart_offset, phdr->p_filesz);

	u64 flags = PTE_U;
	if (phdr->p_flags & PF_R) {
		flags |= PTE_R;
	}
	if (phdr->p_flags & PF_W) {
		flags |= PTE_W;
	}
	if (phdr->p_flags & PF_X) {
		flags |= PTE_X;
	}

	err = vm_pagemap_range(proc->upagetable, flags,
			PA_TO_PN(vstart),
			PA_TO_PN(pstart),
			npages);

	if (err) {
		kpage_free((void *) pstart);
		list_del(&segment->segments);
		kfree(segment);
		return err;
	}

	return 0;
}

int elf_load(proc_t *proc, void *elf, size_t elfsz)
{
	int err;
	Elf64_Ehdr *ehdr;
	Elf64_Phdr *curphdr = NULL;

	ehdr = elf_ehdr_isvalid(elf, elfsz);
	if (!ehdr) {
		return -ENOEXEC;
	}

	proc->trapframe->epc = ehdr->e_entry;

	while ((curphdr = elf_phdr_next(ehdr, curphdr, elfsz))) {
		switch (curphdr->p_type) {
		case PT_LOAD:
			err = elf_phdr_load(ehdr, curphdr, elfsz, proc);
			if (err) {
				return err;
			}
			break;
		default:
			continue;
		}
	}

	return 0;
}

