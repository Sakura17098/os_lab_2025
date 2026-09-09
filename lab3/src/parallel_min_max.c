#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed is a positive number\n");
              return 1;
            }
            break;

          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size is a positive number\n");
              return 1;
            }
            break;

          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum is a positive number\n");
              return 1;
            }
            break;

          default:
            printf("Index %d is out of options\n", option_index);
            return 1;
        }
        break;

      case 'f':
        with_files = true;
        break;

      case '?':
        printf("Unknown option\n");
        return 1;

      default:
        printf("getopt returned character code 0%o?\n", c);
        return 1;
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" "
           "[--by_files]\n",
           argv[0]);
    return 1;
  }

  if (pnum > array_size) {
    pnum = array_size;
  }

  int *array = malloc(sizeof(int) * array_size);
  if (array == NULL) {
    perror("malloc");
    return 1;
  }

  GenerateArray(array, array_size, seed);

  int(*pipes)[2] = NULL;

  if (!with_files) {
    pipes = malloc(sizeof(*pipes) * pnum);
    if (pipes == NULL) {
      perror("malloc");
      free(array);
      return 1;
    }

    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe");

        for (int j = 0; j < i; j++) {
          close(pipes[j][0]);
          close(pipes[j][1]);
        }

        free(pipes);
        free(array);
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    unsigned int begin = (unsigned int)((long long)i * array_size / pnum);
    unsigned int end =
        (unsigned int)((long long)(i + 1) * array_size / pnum);

    pid_t child_pid = fork();

    if (child_pid < 0) {
      perror("fork");
      free(pipes);
      free(array);
      return 1;
    }

    if (child_pid == 0) {
      struct MinMax local_min_max = GetMinMax(array, begin, end);

      if (with_files) {
        char filename[64];
        snprintf(filename, sizeof(filename), "min_max_%d.txt", i);

        FILE *file = fopen(filename, "w");
        if (file == NULL) {
          perror("fopen");
          _exit(1);
        }

        fprintf(file, "%d %d\n", local_min_max.min, local_min_max.max);
        fclose(file);
      } else {
        for (int j = 0; j < pnum; j++) {
          close(pipes[j][0]);

          if (j != i) {
            close(pipes[j][1]);
          }
        }

        ssize_t written =
            write(pipes[i][1], &local_min_max, sizeof(local_min_max));

        close(pipes[i][1]);

        if (written != (ssize_t)sizeof(local_min_max)) {
          perror("write");
          _exit(1);
        }
      }

      _exit(0);
    }
  }

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);
    }
  }

  for (int i = 0; i < pnum; i++) {
    if (wait(NULL) == -1) {
      perror("wait");
      free(pipes);
      free(array);
      return 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    struct MinMax local_min_max;

    if (with_files) {
      char filename[64];
      snprintf(filename, sizeof(filename), "min_max_%d.txt", i);

      FILE *file = fopen(filename, "r");
      if (file == NULL) {
        perror("fopen");
        free(pipes);
        free(array);
        return 1;
      }

      if (fscanf(file, "%d %d", &local_min_max.min, &local_min_max.max) !=
          2) {
        printf("Error while reading %s\n", filename);
        fclose(file);
        free(pipes);
        free(array);
        return 1;
      }

      fclose(file);
      remove(filename);
    } else {
      ssize_t read_bytes =
          read(pipes[i][0], &local_min_max, sizeof(local_min_max));

      close(pipes[i][0]);

      if (read_bytes != (ssize_t)sizeof(local_min_max)) {
        perror("read");
        free(pipes);
        free(array);
        return 1;
      }
    }

    if (local_min_max.min < min_max.min) {
      min_max.min = local_min_max.min;
    }

    if (local_min_max.max > min_max.max) {
      min_max.max = local_min_max.max;
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(pipes);
  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Synchronization: %s\n", with_files ? "files" : "pipe");
  printf("Elapsed time: %fms\n", elapsed_time);

  return 0;
}