#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

void sigint_handler(int signo)
{
    (void)signo;
    const char msg[] = "\nПолучен SIGINT в обработчике\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

void *thread_block_all(void *arg)
{
    while(1) {
        pthread_testcancel();
    }              
    return NULL;
}

void *thread_sigint(void *arg)
{
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &s, NULL);
    while(1) {
        pthread_testcancel();
    }
    return NULL;
}

void *thread_sigwait_quit(void *arg)
{
    sigset_t s;
    int sig;
    sigemptyset(&s);
    sigaddset(&s, SIGQUIT);       
    pthread_sigmask(SIG_UNBLOCK, &s, NULL);  
    if (sigwait(&s, &sig) == 0)
        printf("Получен SIGQUIT через sigwait()\n");
    return NULL;
}

int main(void)
{
    sigset_t all;
    sigfillset(&all);
    pthread_sigmask(SIG_BLOCK, &all, NULL);   

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, thread_block_all, NULL);
    pthread_create(&t2, NULL, thread_sigint, NULL);
    pthread_create(&t3, NULL, thread_sigwait_quit, NULL);

    pthread_join(t3, NULL);         
    pthread_cancel(t1);
    pthread_cancel(t2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}