#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

void cleanup_handler(void *arg) {
    char *message = (char *)arg;
    printf("cleanup: freeing message\n");
    free(message);
}

void *mythread_return_string(void *arg) {
    // int count = 0;
    char* message = malloc(sizeof("hello world"));
    pthread_cleanup_push(cleanup_handler, message);
    if (!message) {
        printf("Sorry mem wasnt initialized\n");
        return NULL;
    }
    memcpy(message, "hello world", sizeof("hello world"));

    while(1) {
        printf("%s\n", message);
        // printf("Hello from mythread!\n");
        // count++;
        // sleep(1);
    }

    pthread_cleanup_pop(1);
    return NULL;
}

int main() {

    pthread_t thread;
    void *res;

    int err = pthread_create(&thread, NULL, mythread_return_string, NULL);

    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_cancel(thread);

    if (err) {
        printf("main: pthread_cancel() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_cancel(thread);

    if (err) {
        printf("main: pthread_cancel() failed: %s\n", strerror(err));
        return -1;
    }

    pthread_join(thread, &res);

    if (res == PTHREAD_CANCELED) {
        printf("Thread was canceled\n");
    }
    else {
        printf("Thread was not canceled\n");
    }

    return 0;

}
