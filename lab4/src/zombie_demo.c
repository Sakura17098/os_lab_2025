#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  pid_t child_pid = fork();

  if (child_pid < 0) {
    perror("fork");
    return EXIT_FAILURE;
  }

  if (child_pid == 0) {
    printf("Child process: PID=%d, PPID=%d\n", getpid(), getppid());
    printf("Child process finishes now.\n");
    exit(EXIT_SUCCESS);
  }

  printf("Parent process: PID=%d\n", getpid());
  printf("Child PID: %d\n", child_pid);
  printf("For 30 seconds the child will be a zombie process.\n");
  printf("Open a second terminal and run:\n");
  printf("ps -o pid,ppid,stat,cmd -p %d,%d\n", getpid(), child_pid);

  sleep(30);

  int status = 0;
  pid_t result = waitpid(child_pid, &status, 0);

  if (result == -1) {
    perror("waitpid");
    return EXIT_FAILURE;
  }

  if (WIFEXITED(status)) {
    printf("Parent received child status. Exit code: %d\n",
           WEXITSTATUS(status));
  }

  printf("The zombie process has been removed.\n");

  return EXIT_SUCCESS;
}
