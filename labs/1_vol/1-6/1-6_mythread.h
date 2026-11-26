/*
 * mythread.h
 *
 * Public interface for the custom Leshka thread library.
 * Defines the thread descriptor type and declares the functions
 * used to create, start and cleanup threads.
 */
#ifndef MYTHREAD_H
#define MYTHREAD_H
#define MYTHREAD_CANCELED ((void *)-1)
/* Thread descriptor: a pointer to the structure that stores thread data. */
typedef struct mythread_struct *mythread_t;
/* Creates a thread that runs start_routine(arg).
 * On success, stores the new thread's descriptor in *thread.
 * Returns 0 on success or an error code.
 */
int  mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg);
/* Adds a thread descriptor to the detached-thread list.
 * This function is private to the implementation.
 */
static void mylist_push(mythread_t thread);
/* Checks the detached-thread list and releases resources for threads
 * that have already terminated. Threads that are still running remain
 * on the list for a later cleanup pass.
 */
void mythread_reap(void);
/* Entry point passed to clone().
 * Receives the thread descriptor, calls the stored start_routine with its
 * argument, and saves the routine's return value in the descriptor.
 */
int mythread_start(void *p);
/* Waits for the specified thread to terminate.
 * If retval is not NULL, stores the thread's return value in *retval.
 * On success, releases the thread's stack and descriptor.
 * Returns 0 on success or an error code.
 */
int  mythread_join(mythread_t thread, void **retval);
/* Marks the specified thread as detached and adds it to the list
 * of threads whose resources will be reclaimed by mythread_reap().
 * Returns 0 on success or an error code.
 */
int mythread_detach(mythread_t t);
/* Requests cancellation of the specified thread.
 * Cancellation takes effect when the thread calls mythread_testcancel().
 * Returns 0 on success or an error code.
 */
int  mythread_cancel(mythread_t thread);
/* Checks whether cancellation was requested for the specified thread.
 * If so, terminates the calling thread with MYTHREAD_CANCELED.
 * Returns 0 if no cancellation was requested, or an error code.
 */
int mythread_testcancel(mythread_t thread);

#endif