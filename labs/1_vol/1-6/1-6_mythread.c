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

struct mythread_struct
{
    pid_t tid;
    void *(*start_routine)(void *);
    void *arg;
    void *retval;
    void *stack_base;
    int joined;
    int detached;
    int cancel_requested;
    struct mythread_struct *next;
};

static mythread_t detached_list;

static void mylist_push(mythread_t thread)
{
    mythread_t head = __atomic_load_n(&detached_list, __ATOMIC_RELAXED);

    do 
    {
        thread->next = head;
    } 
    while (!__atomic_compare_exchange_n(&detached_list, &head, thread, 1, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
}

void mythread_reap(void)
{
    mythread_t thread = __atomic_exchange_n(&detached_list, NULL, __ATOMIC_ACQUIRE);

    while (thread)
    {
        mythread_t next = thread->next;              

        if (__atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE) == 0)
            munmap(thread->stack_base, STACK_SIZE);     
        else
            mylist_push(thread);                         

        thread = next;
    }
}

int mythread_start(void *p)
{
    mythread_t t = p;
    t->retval = t->start_routine(t->arg);
    return 0;
}

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg)
{
    if (thread == NULL || start_routine == NULL)
    {
        return EINVAL;
    }

    mythread_reap();

    void *stack_base = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

    if (stack_base == MAP_FAILED)
    {
        return errno;
    }

    if (mprotect(stack_base, GUARD_SIZE, PROT_NONE) != 0)
    {
        int err = errno; 
        munmap(stack_base, STACK_SIZE);
        return err;
    }

    uintptr_t stack_top = (uintptr_t)stack_base + STACK_SIZE;

    mythread_t mythread = (mythread_t)(stack_top - sizeof(struct mythread_struct));

    mythread->tid = 0;
    mythread->start_routine = start_routine;
    mythread->arg = arg;
    mythread->retval = NULL;
    mythread->stack_base = stack_base;
    mythread->joined = 0;
    mythread->detached = 0;
    mythread->cancel_requested = 0;
    mythread->next = NULL;

    int clone_flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD |
                      CLONE_SYSVSEM | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID;

    int tid = clone(mythread_start, (void *)mythread, clone_flags, mythread, 
        (pid_t *)&mythread->tid, NULL, (pid_t *)&mythread->tid);

    if (tid == -1) 
    {
        int err = errno;
        munmap(stack_base, STACK_SIZE);
        return err;
    }

    *thread = mythread;

    return 0;
}

int  mythread_join(mythread_t thread, void **retval)
{
    if (thread == NULL || thread->detached) 
    {
        return EINVAL;
    }

    pid_t current_tid = (pid_t)syscall(SYS_gettid);

    if (current_tid == __atomic_load_n(&thread->tid, __ATOMIC_RELAXED))
    {
        return EDEADLK;
    }

    if (__atomic_exchange_n(&thread->joined, 1, __ATOMIC_ACQ_REL) != 0)
    {
        return EINVAL;
    }

    int tid;
    while ((tid = __atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE)) != 0)
    {
        syscall(SYS_futex, &thread->tid, FUTEX_WAIT, tid, NULL, NULL, 0);
    }

    if (retval)
    {
        *retval = thread->retval;
    }
    
    munmap(thread->stack_base, STACK_SIZE);

    return 0;
}

int mythread_detach(mythread_t thread)
{
    if (thread == NULL || thread->joined) 
    {
        return EINVAL;
    }

    if (__atomic_exchange_n(&thread->detached, 1, __ATOMIC_ACQ_REL))
        return EINVAL;  

    mylist_push(thread);

    return 0;
}

int  mythread_cancel(mythread_t thread)
{
    if (thread == NULL) 
    {
        return EINVAL;
    }

    __atomic_store_n(&thread->cancel_requested, 1, __ATOMIC_RELAXED);
    return 0;
}

int mythread_testcancel(mythread_t thread)
{
    if (thread == NULL)
    {
        return EINVAL;
    }

    pid_t current_tid = (pid_t)syscall(SYS_gettid);

    if (current_tid != __atomic_load_n(&thread->tid, __ATOMIC_RELAXED))
    {
        return EINVAL;
    }

    if (__atomic_load_n(&thread->cancel_requested, __ATOMIC_RELAXED))
    {
        thread->retval = MYTHREAD_CANCELED;
        syscall(SYS_exit, 0);
    }

    return 0;
}
