#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 50
#define DELAY_CYCLES 500000UL

int common = 0;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void *do_one_thing(void *arg);
void *do_another_thing(void *arg);
void do_wrap_up(int counter);

int main(void)
{
    pthread_t thread1;
    pthread_t thread2;

    if (pthread_create(&thread1, NULL, do_one_thing, &common) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    if (pthread_create(&thread2, NULL, do_another_thing, &common) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    if (pthread_join(thread1, NULL) != 0) {
        perror("pthread_join");
        return EXIT_FAILURE;
    }

    if (pthread_join(thread2, NULL) != 0) {
        perror("pthread_join");
        return EXIT_FAILURE;
    }

    do_wrap_up(common);
    pthread_mutex_destroy(&mut);

    return EXIT_SUCCESS;
}

void *do_one_thing(void *arg)
{
    int *pnum_times = arg;

    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mut);

        printf("doing one thing\n");
        int work = *pnum_times;
        printf("counter = %d\n", work);
        work++;

        for (volatile unsigned long k = 0; k < DELAY_CYCLES; k++) {
        }

        *pnum_times = work;

        pthread_mutex_unlock(&mut);
    }

    return NULL;
}

void *do_another_thing(void *arg)
{
    int *pnum_times = arg;

    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mut);

        printf("doing another thing\n");
        int work = *pnum_times;
        printf("counter = %d\n", work);
        work++;

        for (volatile unsigned long k = 0; k < DELAY_CYCLES; k++) {
        }

        *pnum_times = work;

        pthread_mutex_unlock(&mut);
    }

    return NULL;
}

void do_wrap_up(int counter)
{
    printf("All done, counter = %d\n", counter);
}
