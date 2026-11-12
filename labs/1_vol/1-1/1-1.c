#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h> 

#define THREADS_NUM 5

pthread_t THREADS_IDs[THREADS_NUM];
int global_var = 10;

void *mythread(void *arg) {
    int local_var = 9;
    const int constl_var = 8;
    static int static_var = 7;

    int index = (int)(intptr_t)arg; 

    printf("mythread [%d %d %d %ld]: Hello from mythread!\n", getpid(), getppid(), gettid(), pthread_self());
    printf("global_var adress = %p, val = %d\n", (void*) &global_var, global_var);
    printf("static_var adress = %p, val = %d\n", (void*) &static_var, static_var);
    printf("local_var adress = %p, val = %d\n", (void*) &local_var, local_var);
    printf("constl_var adress = %p, val = %d\n", (void*) &constl_var, constl_var);

    int res = (pthread_equal(THREADS_IDs[index], pthread_self())) ? 1 : 0;
    printf("comparison res = %d\n", res);

    global_var += 1;
    local_var += 1;
    return NULL;
}

int main() {
    int err;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    for (int i = 0; i < THREADS_NUM; i++) {

        err = pthread_create(&THREADS_IDs[i], NULL, mythread, (void *)(intptr_t)i);

        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            return -1;
        }

        sleep(30);

    }

    for (int i = 0; i < THREADS_NUM; i++){
        pthread_join(THREADS_IDs[i], NULL);
    }

    return 0;
}
