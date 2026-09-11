#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
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

static volatile sig_atomic_t timeout_expired = 0;

static void AlarmHandler(int signal_number) {
  (void)signal_number;
  timeout_expired = 1;
}

static void ClosePipes(int (*pipes)[2], int pnum) {
  if (pipes == NULL) {
    return;
  }

  for (int i = 0; i < pnum; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0;
  int child_sleep = 0;
  bool with_files = false;

  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
        {"child_sleep", required_argument, 0, 0},
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

          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout is a positive number\n");
              return 1;
            }
            break;

          case 4:
            child_sleep = atoi(optarg);
            if (child_sleep <= 0) {
              printf("child_sleep is a positive number\n");
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
           "[--by_files] [--timeout \"seconds\"] "
           "[--child_sleep \"seconds\"]\n",
           argv[0]);
    return 1;
  }

  if (pnum > array_size) {
    pnum = array_size;
  }

  int *array = malloc(sizeof(int) * (size_t)array_size);
  if (array == NULL) {
    perror("malloc");
    return 1;
  }

  GenerateArray(array, (unsigned int)array_size, (unsigned int)seed);

  int(*pipes)[2] = NULL;

  if (!with_files) {
    pipes = malloc(sizeof(*pipes) * (size_t)pnum);
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

  pid_t *pids = malloc(sizeof(pid_t) * (size_t)pnum);
  bool *child_finished_normally = calloc((size_t)pnum, sizeof(bool));

  if (pids == NULL || child_finished_normally == NULL) {
    perror("malloc");
    ClosePipes(pipes, pnum);
    free(child_finished_normally);
    free(pids);
    free(pipes);
    free(array);
    return 1;
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

      for (int j = 0; j < i; j++) {
        kill(pids[j], SIGKILL);
      }

      for (int j = 0; j < i; j++) {
        waitpid(pids[j], NULL, 0);
      }

      ClosePipes(pipes, pnum);
      free(child_finished_normally);
      free(pids);
      free(pipes);
      free(array);
      return 1;
    }

    if (child_pid == 0) {
      if (child_sleep > 0) {
        sleep((unsigned int)child_sleep);
      }

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

    pids[i] = child_pid;
  }

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);
    }
  }

  if (timeout > 0) {
    signal(SIGALRM, AlarmHandler);
    alarm((unsigned int)timeout);
    printf("Timeout: %d second(s)\n", timeout);
  }

  int alive_children = pnum;
  bool timeout_processed = false;

  while (alive_children > 0) {
    for (int i = 0; i < pnum; i++) {
      if (pids[i] <= 0) {
        continue;
      }

      int status = 0;
      pid_t result = waitpid(pids[i], &status, WNOHANG);

      if (result == pids[i]) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
          child_finished_normally[i] = true;
        } else if (WIFSIGNALED(status)) {
          printf("Child PID=%d terminated by signal %d\n",
                 pids[i],
                 WTERMSIG(status));
        }

        pids[i] = -1;
        alive_children--;
      } else if (result == -1 && errno != ECHILD) {
        perror("waitpid");
      }
    }

    if (timeout_expired && !timeout_processed) {
      printf("Timeout expired. Sending SIGKILL to unfinished children.\n");

      for (int i = 0; i < pnum; i++) {
        if (pids[i] > 0) {
          if (kill(pids[i], SIGKILL) == -1 && errno != ESRCH) {
            perror("kill");
          }
        }
      }

      timeout_processed = true;
    }

    if (alive_children > 0) {
      struct timespec delay = {0, 1000000};
      nanosleep(&delay, NULL);
    }
  }

  alarm(0);

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;
  int successful_children = 0;

  for (int i = 0; i < pnum; i++) {
    if (!child_finished_normally[i]) {
      continue;
    }

    struct MinMax local_min_max;

    if (with_files) {
      char filename[64];
      snprintf(filename, sizeof(filename), "min_max_%d.txt", i);

      FILE *file = fopen(filename, "r");
      if (file == NULL) {
        perror("fopen");
        ClosePipes(pipes, pnum);
        free(child_finished_normally);
        free(pids);
        free(pipes);
        free(array);
        return 1;
      }

      if (fscanf(file, "%d %d", &local_min_max.min, &local_min_max.max) !=
          2) {
        printf("Error while reading %s\n", filename);
        fclose(file);
        ClosePipes(pipes, pnum);
        free(child_finished_normally);
        free(pids);
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
        free(child_finished_normally);
        free(pids);
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

    successful_children++;
  }

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (!child_finished_normally[i]) {
        close(pipes[i][0]);
      }
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(child_finished_normally);
  free(pids);
  free(pipes);
  free(array);

  if (successful_children == 0) {
    printf("No child process completed successfully.\n");
    printf("Synchronization: %s\n", with_files ? "files" : "pipe");
    printf("Elapsed time: %fms\n", elapsed_time);
    return 1;
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Synchronization: %s\n", with_files ? "files" : "pipe");
  printf("Successful children: %d of %d\n", successful_children, pnum);
  printf("Elapsed time: %fms\n", elapsed_time);

  return 0;
}
