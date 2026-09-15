#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

static void PrintUsage(const char *program) {
  fprintf(stderr, "Usage: %s --port PORT --tnum THREADS\n", program);
}

static bool ParsePositiveInt(const char *text, int *value) {
  char *end = NULL;

  errno = 0;
  long parsed = strtol(text, &end, 10);

  if (errno == ERANGE || end == text || *end != '\0' || parsed <= 0 ||
      parsed > 65535) {
    return false;
  }

  *value = (int)parsed;
  return true;
}

static uint64_t FactorialPart(const struct FactorialArgs *args) {
  uint64_t answer = 1 % args->mod;

  for (uint64_t number = args->begin; number <= args->end; ++number) {
    answer = MultModulo(answer, number, args->mod);

    if (number == UINT64_MAX) {
      break;
    }
  }

  return answer;
}

static void *ThreadFactorial(void *argument) {
  struct FactorialArgs *args = argument;
  args->result = FactorialPart(args);
  return NULL;
}

static int ProcessClient(int client_fd, int tnum) {
  uint64_t request[3];

  int receive_status = RecvAll(client_fd, request, sizeof(request));
  if (receive_status == 0) {
    return 0;
  }
  if (receive_status < 0) {
    perror("RecvAll");
    return -1;
  }

  uint64_t begin = request[0];
  uint64_t end = request[1];
  uint64_t mod = request[2];

  if (mod == 0 || begin == 0 || begin > end) {
    fprintf(stderr, "Invalid task: begin=%" PRIu64 ", end=%" PRIu64
                    ", mod=%" PRIu64 "\n",
            begin, end, mod);
    return -1;
  }

  printf("Received task: [%" PRIu64 ", %" PRIu64 "], mod=%" PRIu64 "\n",
         begin, end, mod);

  uint64_t count = end - begin + 1;
  int workers = tnum;
  if (count < (uint64_t)workers) {
    workers = (int)count;
  }

  pthread_t *threads = calloc((size_t)workers, sizeof(*threads));
  struct FactorialArgs *args = calloc((size_t)workers, sizeof(*args));

  if (threads == NULL || args == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    free(threads);
    free(args);
    return -1;
  }

  uint64_t base_size = count / (uint64_t)workers;
  uint64_t remainder = count % (uint64_t)workers;
  uint64_t current_begin = begin;

  for (int i = 0; i < workers; ++i) {
    uint64_t part_size = base_size + ((uint64_t)i < remainder ? 1 : 0);

    args[i].begin = current_begin;
    args[i].end = current_begin + part_size - 1;
    args[i].mod = mod;
    args[i].result = 1 % mod;

    if (pthread_create(&threads[i], NULL, ThreadFactorial, &args[i]) != 0) {
      fprintf(stderr, "pthread_create failed\n");

      for (int j = 0; j < i; ++j) {
        pthread_join(threads[j], NULL);
      }

      free(threads);
      free(args);
      return -1;
    }

    current_begin = args[i].end + 1;
  }

  uint64_t total = 1 % mod;

  for (int i = 0; i < workers; ++i) {
    pthread_join(threads[i], NULL);
    total = MultModulo(total, args[i].result, mod);
  }

  free(threads);
  free(args);

  if (SendAll(client_fd, &total, sizeof(total)) < 0) {
    perror("SendAll");
    return -1;
  }

  printf("Sent result: %" PRIu64 "\n", total);
  return 0;
}

int main(int argc, char **argv) {
  int port = -1;
  int tnum = -1;

  static struct option options[] = {
      {"port", required_argument, NULL, 'p'},
      {"tnum", required_argument, NULL, 't'},
      {NULL, 0, NULL, 0},
  };

  while (true) {
    int option = getopt_long(argc, argv, "", options, NULL);

    if (option == -1) {
      break;
    }

    switch (option) {
    case 'p':
      if (!ParsePositiveInt(optarg, &port)) {
        fprintf(stderr, "Invalid port: %s\n", optarg);
        return 1;
      }
      break;
    case 't':
      if (!ParsePositiveInt(optarg, &tnum)) {
        fprintf(stderr, "Invalid thread count: %s\n", optarg);
        return 1;
      }
      break;
    default:
      PrintUsage(argv[0]);
      return 1;
    }
  }

  if (port == -1 || tnum == -1 || optind != argc) {
    PrintUsage(argv[0]);
    return 1;
  }

  signal(SIGPIPE, SIG_IGN);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return 1;
  }

  int reuse_address = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
                 sizeof(reuse_address)) < 0) {
    perror("setsockopt");
    close(server_fd);
    return 1;
  }

  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons((uint16_t)port);

  if (bind(server_fd, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    perror("bind");
    close(server_fd);
    return 1;
  }

  if (listen(server_fd, 128) < 0) {
    perror("listen");
    close(server_fd);
    return 1;
  }

  printf("Server started: port=%d, worker threads=%d\n", port, tnum);

  while (true) {
    struct sockaddr_in client_address;
    socklen_t client_length = sizeof(client_address);

    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_address, &client_length);

    if (client_fd < 0) {
      if (errno == EINTR) {
        continue;
      }

      perror("accept");
      continue;
    }

    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_address.sin_addr, client_ip,
                  sizeof(client_ip)) == NULL) {
      strcpy(client_ip, "unknown");
    }

    printf("Client connected: %s:%u\n", client_ip,
           (unsigned int)ntohs(client_address.sin_port));

    ProcessClient(client_fd, tnum);

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);

    printf("Client disconnected\n");
  }
}
