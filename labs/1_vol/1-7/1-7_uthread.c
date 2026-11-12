#define _GNU_SOURCE
#include "1-7_uthread.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACK_SIZE (256 * 1024)

struct uthread_ctl {
    ucontext_t ctx;
    ucontext_t caller;
    void *(*start_routine)(void *);
    void *arg;
};

static uthread_t next_id = 1;

static void thread_start(unsigned int hi, unsigned int lo)
{
    struct uthread_ctl *t = (struct uthread_ctl *)(((uintptr_t)hi << 32) | (uintptr_t)lo);
    t->start_routine(t->arg);
}

int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg)
{
    if (!start_routine)
        return EINVAL;

    struct uthread_ctl *t = calloc(1, sizeof *t);
    if (!t)
        return ENOMEM;
    void *stack = malloc(STACK_SIZE);
    if (!stack) {
        free(t);
        return ENOMEM;
    }
    if (getcontext(&t->ctx) == -1) {
        int err = errno;
        free(stack);
        free(t);
        return err;
    }

    t->start_routine = start_routine;
    t->arg = arg;
    t->ctx.uc_stack.ss_sp = stack;
    t->ctx.uc_stack.ss_size = STACK_SIZE;
    t->ctx.uc_link = &t->caller;

    uintptr_t p = (uintptr_t)t;
    makecontext(&t->ctx, (void (*)(void))thread_start, 2,
                (unsigned int)(p >> 32), (unsigned int)(p & 0xffffffffu));

    if (thread)
        *thread = next_id;
    next_id++;

    if (swapcontext(&t->caller, &t->ctx) == -1) {
        int err = errno;
        free(stack);
        free(t);
        return err;
    }

    free(stack);
    free(t);
    return 0;
}