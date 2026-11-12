#define _GNU_SOURCE
#include "1-6_mythread.h"

#include <errno.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int answer = 69;

static void *exit_worker(void *arg)
{
    mythread_t self = mythread_self();

    printf("exit_worker: self=%p, returning through mythread_exit()\n",
           (void *)self);

    mythread_exit(arg);
    return NULL;
}

static void *cancel_worker(void *arg)
{
    (void)arg;

    printf("cancel_worker: started, waiting for cancellation\n");

    while (1) {
        mythread_testcancel();
    }
}

static void *detached_worker(void *arg)
{
    (void)arg;

    printf("detached_worker: started\n");
    usleep(100000);
    printf("detached_worker: finished\n");

    return NULL;
}

int main(void)
{
    int err;
    mythread_t thread;
    void *retval = NULL;

    printf("main: mythread_self() = %p (ожидается NULL)\n",
           (void *)mythread_self());

    err = mythread_create(&thread, exit_worker, &answer);
    if (err != 0) {
        fprintf(stderr, "mythread_create: %s\n", strerror(err));
        return 1;
    }

    err = mythread_join(thread, &retval);
    if (err != 0) {
        fprintf(stderr, "mythread_join: %s\n", strerror(err));
        return 1;
    }

    printf("join result: %s, value=%d\n",
           retval == &answer ? "OK" : "FAIL",
           retval == &answer ? *(int *)retval : -1);

    err = mythread_create(&thread, cancel_worker, NULL);
    if (err != 0) {
        fprintf(stderr, "mythread_create: %s\n", strerror(err));
        return 1;
    }

    err = mythread_cancel(thread);
    if (err != 0) {
        fprintf(stderr, "mythread_cancel: %s\n", strerror(err));
        return 1;
    }

    retval = NULL;
    err = mythread_join(thread, &retval);
    if (err != 0) {
        fprintf(stderr, "mythread_join (canceled): %s\n", strerror(err));
        return 1;
    }

    printf("cancel result: %s\n",
           retval == MYTHREAD_CANCELED ? "OK" : "FAIL");

    err = mythread_create(&thread, detached_worker, NULL);
    if (err != 0) {
        fprintf(stderr, "mythread_create: %s\n", strerror(err));
        return 1;
    }

    err = mythread_detach(thread);
    if (err != 0) {
        fprintf(stderr, "mythread_detach: %s\n", strerror(err));
        return 1;
    }

    printf("main: detached successfully\n");

    printf("all checks finished\n");
    return 0;
}