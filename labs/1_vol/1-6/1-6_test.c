#define _GNU_SOURCE
#include "1-6_mythread.h"

#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int answer = 69;

static void *joined_worker(void *arg) {
    (void)arg;

    printf("some_worker: hi, i wanna %d\n", *(int *)arg);
    printf("my tid is %d\n", gettid());

    return arg;
}

void *cancel_worker(void *arg)
{
    mythread_t *self = arg;

    printf("cancel_worker: started, waiting for cancellation\n");

    while (1) {
        mythread_testcancel(*self);
    }
}

static void *detached_worker(void *arg)
{
    (void)arg;

    printf("detached_worker: started\n");
    sleep(2);
    printf("detached_worker: finished\n");

    return NULL;
}

int main(void)
{
    int err;
    mythread_t thread;
    void *retval = NULL;

    err = mythread_create(&thread, joined_worker, &answer); // join test
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
           (int*)retval == &answer ? "OK" : "FAIL",
           (int*)retval == &answer ? *(int *)retval : -1);

    err = mythread_create(&thread, cancel_worker, &thread); // cancel test
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

    err = mythread_create(&thread, detached_worker, NULL); // detach test
    if (err != 0) {
        fprintf(stderr, "mythread_create: %s\n", strerror(err));
        return 1;
    }

    err = mythread_detach(thread);
    if (err != 0) {
        fprintf(stderr, "mythread_detach: %s\n", strerror(err));
        return 1;
    }

    sleep(3);
    printf("main: detached successfully\n");

    sleep(3);
    printf("all checks finished\n");
    return 0;
}