#define _GNU_SOURCE
#include "1-7_uthread.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define THREADS_NUM 3

static void *my_uthread(void *arg)
{
    int index = (int)(intptr_t)arg;

    printf("uthread %d: start\n", index);

    for (int i = 1; i < 4; i++)
        printf("uthread %d: step %d\n", index, i);

    printf("uthread %d: finish\n", index);
    return NULL;
}

int main(void)
{
    uthread_t threads[THREADS_NUM];

    printf("main: start\n");

    for (int i = 0; i < THREADS_NUM; i++) {
        printf("main: before uthread_create(%d)\n", i);

        int err = uthread_create(&threads[i], my_uthread,
                                 (void *)(intptr_t)i);
        if (err != 0) {
            fprintf(stderr, "main: uthread_create(%d) failed: %s\n",
                    i, strerror(err));
            return 1;
        }

        printf("main: after uthread_create(%d), id=%lu\n",
               i, (unsigned long)threads[i]);
    }

    printf("main: finish\n");
    return 0;
}
