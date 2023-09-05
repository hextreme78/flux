#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include <kernel/types.h>

typedef struct trapframe trapframe_t;
typedef struct context   context_t;
typedef struct cpu       cpu_t;
typedef struct segment   segment_t;
typedef struct filedesc  fd_t;
typedef struct proc      proc_t;

#include <kernel/vm.h>
#include <kernel/list.h>
#include <kernel/riscv64.h>
#include <kernel/spinlock.h>

#define PROC_STATE_KILLED    0
#define PROC_STATE_PREPARING 1
#define PROC_STATE_RUNNABLE  2
#define PROC_STATE_RUNNING   3
#define PROC_STATE_STOPPED   4
#define PROC_STATE_ZOMBIE    5

struct trapframe {
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

	u64 cpuid;
	u64 kstack;
	u64 kerneltrap;
	u64 kpagetable;
	u64 user_irq_handler;
	u64 usertrap;
	u64 upagetable;
} __attribute__((packed));

struct context {
	u64 ra;
	u64 sp;
	u64 gp;
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
	u64 kpagetable;
} __attribute__((packed));

struct cpu {
	context_t *context;
	proc_t *proc;
};

struct segment {
	u64 vstart;
	u64 vlen;
	u64 pstart;
	list_t segments;
};

struct filedesc {
	u32 inum;
	int fd_flags;
	int *status_flags;
	off_t *offset;
	size_t *refcnt;
};

struct proc {
	spinlock_t lock;
	u64 state;
	void *wchan;

	pid_t pid;
	int exit_status;

	context_t *context;
	trapframe_t *trapframe;
	void *kstack;
	void *ustack;
	pte_t *upagetable;
	pte_t *kpagetable;
	segment_t segment_list;

	proc_t *parent;
	list_t children;

	u16 uid;
	u16 gid;
	u32 cwd;

	fd_t filetable[FD_MAX];
	mode_t umask;
};

void proc_init(void);
void proc_hart_init(void);

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
	return curcpu()->proc;
}

#endif

