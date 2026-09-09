#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s seed array_size\n", argv[0]);
    return 1;
  }

  int seed = atoi(argv[1]);
  if (seed <= 0) {
    printf("seed is a positive number\n");
    return 1;
  }

  int array_size = atoi(argv[2]);
  if (array_size <= 0) {
    printf("array_size is a positive number\n");
    return 1;
  }

  pid_t child_pid = fork();

  if (child_pid < 0) {
    perror("fork");
    return 1;
  }

  if (child_pid == 0) {
    execl("./sequential_min_max",
          "sequential_min_max",
          argv[1],
          argv[2],
          (char *)NULL);

    perror("execl");
    _exit(1);
  }

  int status;

  if (waitpid(child_pid, &status, 0) == -1) {
    perror("waitpid");
    return 1;
  }

  if (WIFEXITED(status)) {
    printf("Child process finished with exit code: %d\n",
           WEXITSTATUS(status));
  } else {
    printf("Child process did not finish normally\n");
  }

  return 0;
}
