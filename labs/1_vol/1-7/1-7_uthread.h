#ifndef UTHREAD_H
#define UTHREAD_H

typedef unsigned long uthread_t;

int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg);

#endif