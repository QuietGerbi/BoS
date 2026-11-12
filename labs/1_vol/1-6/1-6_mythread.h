#ifndef MYTHREAD_H
#define MYTHREAD_H

typedef struct mythread_struct *mythread_t;

#define MYTHREAD_CANCELED ((void *)-1)

int  mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg);

int  mythread_join(mythread_t thread, void **retval);

int mythread_detach(mythread_t t);

int  mythread_cancel(mythread_t thread);

void mythread_testcancel(void);

void mythread_exit(void *retval);

mythread_t mythread_self(void);

#endif