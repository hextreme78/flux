#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include <kernel/spinlock.h>
#include <kernel/vm.h>
#include <kernel/list.h>
#include <kernel/riscv64.h>
#include <kernel/types.h>

#define PROC_STATE_KILLED    0
#define PROC_STATE_PREPARING 1
#define PROC_STATE_RUNNABLE  2
#define PROC_STATE_RUNNING   3
#define PROC_STATE_STOPPED   4
#define PROC_STATE_ZOMBIE    5

typedef struct proc proc_t;
typedef i64 pid_t;

typedef struct {
	u64 ra;
	u64 sp;
	u64 gp;
	u64 tp;
	u64 t0;
	u64 t1;
	u64 t2;
	u64 fp;
	u64 s1;
	u64 a0;
	u64 a1;
	u64 a2;
	u64 a3;
	u64 a4;
	u64 a5;
	u64 a6;
	u64 a7;
	u64 s2;
	u64 s3;
	u64 s4;
	u64 s5;
	u64 s6;
	u64 s7;
	u64 s8;
	u64 s9;
	u64 s10;
	u64 s11;
	u64 t3;
	u64 t4;
	u64 t5;
	u64 t6;

	u64 kernel_sp;
	u64 kernel_tp;
	u64 kernel_satp;
	u64 user_irq_handler;
} __attribute__((packed)) trapframe_t;

typedef struct {
	u64 ra;
	u64 sp;
	u64 gp;
	u64 tp;
	u64 t0;
	u64 t1;
	u64 t2;
	u64 fp;
	u64 s1;
	u64 a0;
	u64 a1;
	u64 a2;
	u64 a3;
	u64 a4;
	u64 a5;
	u64 a6;
	u64 a7;
	u64 s2;
	u64 s3;
	u64 s4;
	u64 s5;
	u64 s6;
	u64 s7;
	u64 s8;
	u64 s9;
	u64 s10;
	u64 s11;
	u64 t3;
	u64 t4;
	u64 t5;
	u64 t6;

	u64 epc;
} __attribute__((packed)) ctx_t;

typedef struct {
	proc_t *proc;
	ctx_t *context;
} cpu_t;

typedef struct {
	u64 vstart;
	u64 vlen;
	u64 pstart;
	list_t segments;
} segment_t;

struct proc {
	spinlock_t lock;
	pid_t pid;
	u64 state;
	ctx_t *context;
	trapframe_t *trapframe;
	void *kstack;
	void *ustack;
	pte_t *pagetable;
	segment_t segment_list;

	int exit_status;

	proc_t *parent;
	list_t children;

	i64 irq_save;
	bool in_irq;
};

void proc_init(void);
void proc_hart_init(void);
void scheduler(void);

int proc_create(void *elf, size_t elfsz);
void proc_destroy(proc_t *proc);

static inline u64 cpuid(void)
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

