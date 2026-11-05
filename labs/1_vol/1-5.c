#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

sigset_t all_signals;
sigset_t int_set;      
pthread_t tid_block, tid_sigint, tid_sigwait;

void sigint_handler(int signo)
{
    if (signo == SIGINT) {
        const char msg[] = "\nПолучен SIGINT\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    }
}

void *thread_block_all(void *arg)
{
    if (pthread_sigmask(SIG_BLOCK, &all_signals, NULL) != 0) {
        perror("pthread_block_all");
        pthread_exit(NULL);
    }
    return NULL;
}

void *thread_sigint_handler(void *arg)
{
    if (pthread_sigmask(SIG_UNBLOCK, &int_set, NULL) != 0) {
        perror("pthread_sigint");
        pthread_exit(NULL);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        pthread_exit(NULL);
    }
    return NULL;
}

void *thread_sigwait_quit(void *arg)
{
    int sig;

    if (sigwait(&all_signals, &sig) == 0) {
        if (sig == SIGQUIT) {
            printf("Получен SIGQUIT через sigwait()\n");
        } else {
            printf("Получен сигнал %d\n", sig);
        }
    } else {
        perror("sigwait");
    }
    return NULL;
}

int main(void)
{
    sigfillset(&all_signals);

    sigemptyset(&int_set);
    sigaddset(&int_set, SIGINT);

    if (pthread_sigmask(SIG_BLOCK, &all_signals, NULL) != 0) {
        perror("all signals mask error");
        return 1;
    }

    if (pthread_create(&tid_block,   NULL, thread_block_all,     NULL) != 0 ||
        pthread_create(&tid_sigint,  NULL, thread_sigint_handler,NULL) != 0 ||
        pthread_create(&tid_sigwait, NULL, thread_sigwait_quit,  NULL) != 0)
    {
        perror("pthread_create");
        return 1;
    }

    pthread_join(tid_sigwait, NULL);
    pthread_join(tid_sigint,  NULL);
    pthread_join(tid_block,   NULL);

    return 0;
}