#define _GNU_SOURCE
#include "1-6_mythread.h"

#include <errno.h>
#include <linux/futex.h>
#include <sched.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define STACK_SIZE ((size_t)1 << 20)  
#define GUARD_SIZE 4096

struct mythread_struct {
    volatile int tid;            
    volatile int cancel_requested;
    volatile int joined;
    void *(*start_routine)(void *);
    void *arg;
    void *retval;
    void *stack_base;            
    struct mythread_struct *next; 
};

static mythread_t detached_list;
static volatile int list_lock;

static void list_acquire(void) { while (__atomic_exchange_n(&list_lock, 1, __ATOMIC_ACQUIRE)); }
static void list_release(void) { __atomic_store_n(&list_lock, 0, __ATOMIC_RELEASE); }

static void reap_detached(void)
{
    list_acquire();
    mythread_t *pp = &detached_list;
    while (*pp) {
        mythread_t t = *pp;
        if (__atomic_load_n(&t->tid, __ATOMIC_ACQUIRE) == 0) {
            *pp = t->next;
            munmap(t->stack_base, STACK_SIZE);
        } else {
            pp = &t->next;
        }
    }
    list_release();
}

static pid_t my_gettid(void) { return (pid_t)syscall(SYS_gettid); }

static int is_main_thread(void) { return my_gettid() == getpid(); }

mythread_t mythread_self(void)
{
    if (is_main_thread())
        return NULL;
    char probe;
    uintptr_t sp = (uintptr_t)&probe;
    uintptr_t base = sp & ~((uintptr_t)STACK_SIZE - 1);
    size_t sz = (sizeof(struct mythread_struct) + 63) & ~(size_t)63;
    return (mythread_t)(base + STACK_SIZE - sz);
}

void mythread_exit(void *retval)
{
    mythread_t self = mythread_self();
    if (self == NULL)
        syscall(SYS_exit_group, 0);   
    self->retval = retval;
    syscall(SYS_exit, 0);             
}

void mythread_testcancel(void)
{
    mythread_t self = mythread_self();
    if (self && __atomic_load_n(&self->cancel_requested, __ATOMIC_ACQUIRE))
        mythread_exit(MYTHREAD_CANCELED);
}

static int thread_start(void *p)
{
    mythread_t t = p;
    t->retval = t->start_routine(t->arg);
    return 0;                         
}

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg)
{
    if (!thread || !start_routine)
        return EINVAL;

    reap_detached();

    size_t total = 2 * STACK_SIZE;
    void *raw = mmap(NULL, total, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (raw == MAP_FAILED)
        return errno;

    uintptr_t aligned = ((uintptr_t)raw + STACK_SIZE - 1) & ~((uintptr_t)STACK_SIZE - 1);
    if (aligned > (uintptr_t)raw)
        munmap(raw, aligned - (uintptr_t)raw);
    uintptr_t tail = aligned + STACK_SIZE;
    uintptr_t raw_end = (uintptr_t)raw + total;
    if (raw_end > tail)
        munmap((void *)tail, raw_end - tail);
    mprotect((void *)aligned, GUARD_SIZE, PROT_NONE);

    size_t sz = (sizeof(struct mythread_struct) + 63) & ~(size_t)63;
    mythread_t t = (mythread_t)(aligned + STACK_SIZE - sz);
    t->tid = 0;
    t->cancel_requested = 0;
    t->joined = 0;
    t->next = NULL;
    t->start_routine = start_routine;
    t->arg = arg;
    t->retval = NULL;
    t->stack_base = (void *)aligned;

    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD |
                CLONE_SYSVSEM | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID;

    int tid = clone(thread_start, (void *)t, flags, t, (pid_t *)&t->tid, NULL, (pid_t *)&t->tid);
    if (tid == -1) {
        int err = errno;
        munmap((void *)aligned, STACK_SIZE);
        return err;
    }
    *thread = t;
    return 0;
}

int mythread_join(mythread_t t, void **retval)
{
    if (!t)
        return EINVAL;
    if (t == mythread_self())
        return EDEADLK;
    if (__atomic_exchange_n(&t->joined, 1, __ATOMIC_ACQ_REL))
        return EINVAL;               

    int tid;
    while ((tid = __atomic_load_n(&t->tid, __ATOMIC_ACQUIRE)) != 0)
        syscall(SYS_futex, &t->tid, FUTEX_WAIT, tid, NULL, NULL, 0);

    if (retval)
        *retval = t->retval;
    munmap(t->stack_base, STACK_SIZE);
    return 0;
}

int mythread_detach(mythread_t t)
{
    if (!t)
        return EINVAL;
    if (__atomic_exchange_n(&t->joined, 1, __ATOMIC_ACQ_REL))
        return EINVAL;                

    list_acquire();
    t->next = detached_list;
    detached_list = t;
    list_release();

    reap_detached();
    return 0;
}

int mythread_cancel(mythread_t t)
{
    if (!t)
        return EINVAL;
    __atomic_store_n(&t->cancel_requested, 1, __ATOMIC_RELEASE);
    return 0;
}