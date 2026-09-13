#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;

void *thread_one(void *arg)
{
    (void)arg;

    printf("Thread 1: locking mutex A\n");
    fflush(stdout);
    pthread_mutex_lock(&mutex_a);

    sleep(1);

    printf("Thread 1: trying to lock mutex B\n");
    fflush(stdout);
    pthread_mutex_lock(&mutex_b);

    printf("Thread 1: acquired mutex B\n");
    fflush(stdout);

    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);

    return NULL;
}

void *thread_two(void *arg)
{
    (void)arg;

    printf("Thread 2: locking mutex B\n");
    fflush(stdout);
    pthread_mutex_lock(&mutex_b);

    sleep(1);

    printf("Thread 2: trying to lock mutex A\n");
    fflush(stdout);
    pthread_mutex_lock(&mutex_a);

    printf("Thread 2: acquired mutex A\n");
    fflush(stdout);

    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);

    return NULL;
}

int main(void)
{
    pthread_t first_thread;
    pthread_t second_thread;

    if (pthread_create(&first_thread, NULL, thread_one, NULL) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    if (pthread_create(&second_thread, NULL, thread_two, NULL) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    pthread_join(first_thread, NULL);
    pthread_join(second_thread, NULL);

    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);

    return EXIT_SUCCESS;
}