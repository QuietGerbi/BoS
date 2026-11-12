#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h> 

// void *mythread_return_string(void *arg) {
//     printf("mythread [%d %d %d %ld]: Hello from mythread!\n", getpid(), getppid(), gettid(), pthread_self());

//     return (void *)"hello world!";
// }

// void *mythread_return_int(void *arg) {
//     printf("mythread [%ld]: Hello from mythread!\n", pthread_self());
//     return (void *)(intptr_t)42;
// }

void *mythread_print_tid(void *arg) {

    // pthread_detach(pthread_self());
    printf("mythread [%ld]: Hello from mythread!\n", pthread_self());

    return NULL;
}

int main() {

    pthread_t thread;
    pthread_attr_t attr;

    int err;
    void *res_ptr;
    int count;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    // err = pthread_create(&thread, NULL, mythread_return_string, NULL);

        // if (err) {
        //     printf("main: pthread_create() failed: %s\n", strerror(err));
        //     return -1;
        // }


    // pthread_attr_init(&attr); 
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    while(1) {
        // err = pthread_create(&thread, &attr, mythread_print_tid, NULL);
        err = pthread_create(&thread, NULL, mythread_print_tid, NULL);

        // count++;

        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            return -1;
        }

        // printf("%d\n", count);
    }

    // pthread_attr_destroy(&attr); 

    // pthread_join(thread, &res_ptr);

    // char* res = (char*)res_ptr; 
    // int res = (int)(intptr_t)res_ptr;

    // printf("res = %s\n", res);
    // printf("res = %d\n", res);

    return 0;
}
