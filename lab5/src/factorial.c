#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    long long start;
    long long end;
    long long mod;
} thread_data_t;

long long result = 1;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s -k NUMBER --pnum=NUMBER --mod=NUMBER\n",
            program_name);
}

int parse_positive_long_long(const char *text, long long *value)
{
    char *end = NULL;
    long long parsed;

    errno = 0;
    parsed = strtoll(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' || parsed <= 0) {
        return 0;
    }

    *value = parsed;
    return 1;
}

void *calculate_partial_factorial(void *arg)
{
    thread_data_t *data = arg;
    long long partial_result = 1 % data->mod;

    for (long long i = data->start; i <= data->end; i++) {
        partial_result = (partial_result * (i % data->mod)) % data->mod;
    }

    pthread_mutex_lock(&result_mutex);
    result = (result * partial_result) % data->mod;
    pthread_mutex_unlock(&result_mutex);

    return NULL;
}

int main(int argc, char *argv[])
{
    long long k = -1;
    long long pnum = -1;
    long long mod = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0) {
            if (i + 1 >= argc ||
                !parse_positive_long_long(argv[++i], &k)) {
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            if (!parse_positive_long_long(argv[i] + 7, &pnum)) {
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            if (!parse_positive_long_long(argv[i] + 6, &mod)) {
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (k <= 0 || pnum <= 0 || mod <= 0) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (pnum > k) {
        pnum = k;
    }

    pthread_t *threads = malloc((size_t)pnum * sizeof(*threads));
    thread_data_t *thread_data = malloc((size_t)pnum * sizeof(*thread_data));

    if (threads == NULL || thread_data == NULL) {
        perror("malloc");
        free(threads);
        free(thread_data);
        return EXIT_FAILURE;
    }

    result = 1 % mod;

    long long base_count = k / pnum;
    long long remainder = k % pnum;
    long long current = 1;

    for (long long i = 0; i < pnum; i++) {
        long long count = base_count + (i < remainder ? 1 : 0);

        thread_data[i].start = current;
        thread_data[i].end = current + count - 1;
        thread_data[i].mod = mod;
        current = thread_data[i].end + 1;

        if (pthread_create(&threads[i], NULL, calculate_partial_factorial,
                           &thread_data[i]) != 0) {
            perror("pthread_create");

            for (long long j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }

            free(threads);
            free(thread_data);
            return EXIT_FAILURE;
        }
    }

    for (long long i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            free(threads);
            free(thread_data);
            return EXIT_FAILURE;
        }
    }

    printf("%lld! mod %lld = %lld\n", k, mod, result);

    pthread_mutex_destroy(&result_mutex);
    free(threads);
    free(thread_data);

    return EXIT_SUCCESS;
}