#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

typedef struct
{
    int num;
    char* some_text;

} example;

void *mythread(void *arg) {
    example *thing_p = (example *)arg;

    printf("num = %d, some_text = %s\n", thing_p->num, thing_p->some_text);

    free(thing_p);

    return NULL;
}

int main() {
    pthread_t tid; 
    example *thing_p = (example *)(malloc(sizeof(example)));

    thing_p->num = 10;
    thing_p->some_text = "hello";

    pthread_attr_t attr;

    pthread_attr_init(&attr); 
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int err = pthread_create(&tid, &attr, mythread, thing_p);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        free(thing_p); 
        pthread_attr_destroy(&attr);
        return -1;
    }

    sleep(1);

    // pthread_join(tid, NULL);

    pthread_attr_destroy(&attr); 

    return 0;

}
