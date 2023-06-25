#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include <kernel/spinlock.h>
#include <kernel/vm.h>
#include <kernel/list.h>
#include <kernel/riscv64.h>
#include <stdint.h>

#define PROC_STATE_KILLED    0
#define PROC_STATE_PREPARING 1
#define PROC_STATE_RUNNABLE  2
#define PROC_STATE_RUNNING   3
#define PROC_STATE_STOPPED   4
#define PROC_STATE_ZOMBIE    5

typedef struct proc proc_t;
typedef int64_t pid_t;

typedef struct {
	uint64_t ra;
	uint64_t sp;
	uint64_t gp;
	uint64_t tp;
	uint64_t t0;
	uint64_t t1;
	uint64_t t2;
	uint64_t fp;
	uint64_t s1;
	uint64_t a0;
	uint64_t a1;
	uint64_t a2;
	uint64_t a3;
	uint64_t a4;
	uint64_t a5;
	uint64_t a6;
	uint64_t a7;
	uint64_t s2;
	uint64_t s3;
	uint64_t s4;
	uint64_t s5;
	uint64_t s6;
	uint64_t s7;
	uint64_t s8;
	uint64_t s9;
	uint64_t s10;
	uint64_t s11;
	uint64_t t3;
	uint64_t t4;
	uint64_t t5;
	uint64_t t6;

	uint64_t kernel_sp;
	uint64_t kernel_tp;
	uint64_t kernel_satp;
	uint64_t user_irq_handler;
} __attribute__((packed)) trapframe_t;

typedef struct {
	uint64_t ra;
	uint64_t sp;
	uint64_t gp;
	uint64_t tp;
	uint64_t t0;
	uint64_t t1;
	uint64_t t2;
	uint64_t fp;
	uint64_t s1;
	uint64_t a0;
	uint64_t a1;
	uint64_t a2;
	uint64_t a3;
	uint64_t a4;
	uint64_t a5;
	uint64_t a6;
	uint64_t a7;
	uint64_t s2;
	uint64_t s3;
	uint64_t s4;
	uint64_t s5;
	uint64_t s6;
	uint64_t s7;
	uint64_t s8;
	uint64_t s9;
	uint64_t s10;
	uint64_t s11;
	uint64_t t3;
	uint64_t t4;
	uint64_t t5;
	uint64_t t6;

	uint64_t epc;
} __attribute__((packed)) ctx_t;

typedef struct {
	proc_t *proc;
	ctx_t *context;
} cpu_t;

typedef struct {
	uint64_t vstart;
	uint64_t vlen;
	uint64_t pstart;
	list_t segments;
} segment_t;

struct proc {
	spinlock_t lock;
	pid_t pid;
	uint64_t state;
	ctx_t *context;
	trapframe_t *trapframe;
	void *kstack;
	void *ustack;
	pte_t *pagetable;
	segment_t segment_list;

	int exit_status;

	proc_t *parent;
	list_t children;
};

void proc_init(void);
void proc_hart_init(void);
void scheduler(void);

int proc_create(void *elf, size_t elfsz);
void proc_destroy(proc_t *proc);

static inline uint64_t cpuid(void)
{
	return r_tp();
}

static inline cpu_t *curcpu(void)
{
	extern cpu_t cpus[NCPU];
	return &cpus[cpuid()];
}

static inline proc_t *curproc(void)
{
	extern cpu_t cpus[NCPU];
	return cpus[cpuid()].proc;
}

#endif

