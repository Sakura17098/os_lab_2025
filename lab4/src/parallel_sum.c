#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <getopt.h>

#include "sum.h"
#include "utils.h"

struct SumArgs {
  const int *array;
  size_t begin;
  size_t end;
  long long partial_sum;
};

static void PrintUsage(const char *program_name) {
  printf("Usage: %s --threads_num \"num\" --seed \"num\" "
         "--array_size \"num\"\n",
         program_name);
}

static int ParsePositiveInt(const char *value, const char *option_name) {
  char *end_pointer = NULL;
  errno = 0;

  long parsed_value = strtol(value, &end_pointer, 10);

  if (errno != 0 || *value == '\0' || *end_pointer != '\0' ||
      parsed_value <= 0 || parsed_value > INT32_MAX) {
    printf("%s is a positive number\n", option_name);
    exit(EXIT_FAILURE);
  }

  return (int)parsed_value;
}

static void *ThreadSum(void *arguments) {
  struct SumArgs *sum_args = arguments;

  sum_args->partial_sum =
      SumRange(sum_args->array, sum_args->begin, sum_args->end);

  return NULL;
}

static double GetElapsedTimeMilliseconds(const struct timespec *start_time,
                                         const struct timespec *finish_time) {
  double seconds = (double)(finish_time->tv_sec - start_time->tv_sec);
  double nanoseconds =
      (double)(finish_time->tv_nsec - start_time->tv_nsec) / 1000000.0;

  return seconds * 1000.0 + nanoseconds;
}

int main(int argc, char **argv) {
  int threads_num = -1;
  int array_size = -1;
  int seed = -1;

  while (true) {
    static struct option options[] = {
        {"threads_num", required_argument, 0, 0},
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {0, 0, 0, 0}};

    int option_index = 0;
    int option = getopt_long(argc, argv, "", options, &option_index);

    if (option == -1) {
      break;
    }

    switch (option) {
      case 0:
        switch (option_index) {
          case 0:
            threads_num = ParsePositiveInt(optarg, "threads_num");
            break;

          case 1:
            seed = ParsePositiveInt(optarg, "seed");
            break;

          case 2:
            array_size = ParsePositiveInt(optarg, "array_size");
            break;

          default:
            printf("Unknown option index\n");
            return EXIT_FAILURE;
        }
        break;

      case '?':
        printf("Unknown option\n");
        PrintUsage(argv[0]);
        return EXIT_FAILURE;

      default:
        printf("Error while parsing arguments\n");
        return EXIT_FAILURE;
    }
  }

  if (optind < argc) {
    printf("Has at least one non-option argument\n");
    PrintUsage(argv[0]);
    return EXIT_FAILURE;
  }

  if (threads_num == -1 || seed == -1 || array_size == -1) {
    PrintUsage(argv[0]);
    return EXIT_FAILURE;
  }

  if (threads_num > array_size) {
    threads_num = array_size;
  }

  int *array = malloc(sizeof(int) * (size_t)array_size);
  if (array == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  GenerateArray(array, (unsigned int)array_size, (unsigned int)seed);

  pthread_t *threads = malloc(sizeof(pthread_t) * (size_t)threads_num);
  struct SumArgs *args =
      malloc(sizeof(struct SumArgs) * (size_t)threads_num);

  if (threads == NULL || args == NULL) {
    perror("malloc");
    free(args);
    free(threads);
    free(array);
    return EXIT_FAILURE;
  }

  struct timespec start_time;
  struct timespec finish_time;

  if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
    perror("clock_gettime");
    free(args);
    free(threads);
    free(array);
    return EXIT_FAILURE;
  }

  for (int i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = (size_t)i * (size_t)array_size / (size_t)threads_num;
    args[i].end =
        (size_t)(i + 1) * (size_t)array_size / (size_t)threads_num;
    args[i].partial_sum = 0;

    int result = pthread_create(&threads[i], NULL, ThreadSum, &args[i]);

    if (result != 0) {
      fprintf(stderr, "pthread_create: %s\n", strerror(result));

      for (int j = 0; j < i; j++) {
        pthread_join(threads[j], NULL);
      }

      free(args);
      free(threads);
      free(array);
      return EXIT_FAILURE;
    }
  }

  long long total_sum = 0;

  for (int i = 0; i < threads_num; i++) {
    int result = pthread_join(threads[i], NULL);

    if (result != 0) {
      fprintf(stderr, "pthread_join: %s\n", strerror(result));
      free(args);
      free(threads);
      free(array);
      return EXIT_FAILURE;
    }

    total_sum += args[i].partial_sum;
  }

  if (clock_gettime(CLOCK_MONOTONIC, &finish_time) == -1) {
    perror("clock_gettime");
    free(args);
    free(threads);
    free(array);
    return EXIT_FAILURE;
  }

  double elapsed_time =
      GetElapsedTimeMilliseconds(&start_time, &finish_time);

  printf("Threads number: %d\n", threads_num);
  printf("Seed: %d\n", seed);
  printf("Array size: %d\n", array_size);
  printf("Total: %lld\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);

  free(args);
  free(threads);
  free(array);

  return EXIT_SUCCESS;
}
