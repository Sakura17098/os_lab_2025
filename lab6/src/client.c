#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#define MAX_IP_LENGTH INET_ADDRSTRLEN
#define MAX_LINE_LENGTH 256

struct Server {
  char ip[MAX_IP_LENGTH];
  uint16_t port;
};

struct ClientTask {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  int status;
};

static void PrintUsage(const char *program) {
  fprintf(stderr,
          "Usage: %s --k NUMBER --mod NUMBER --servers PATH_TO_FILE\n",
          program);
}

static void TrimLine(char *line) {
  size_t length = strlen(line);

  while (length > 0 && isspace((unsigned char)line[length - 1])) {
    line[length - 1] = '\0';
    --length;
  }

  size_t first = 0;
  while (line[first] != '\0' && isspace((unsigned char)line[first])) {
    ++first;
  }

  if (first > 0) {
    memmove(line, line + first, strlen(line + first) + 1);
  }
}

static bool ParsePort(const char *text, uint16_t *port) {
  char *end = NULL;

  errno = 0;
  long parsed = strtol(text, &end, 10);

  if (errno == ERANGE || end == text || *end != '\0' || parsed <= 0 ||
      parsed > 65535) {
    return false;
  }

  *port = (uint16_t)parsed;
  return true;
}

static bool ParseServerLine(const char *line, struct Server *server) {
  char copy[MAX_LINE_LENGTH];
  char *separator = NULL;

  if (strlen(line) >= sizeof(copy)) {
    return false;
  }

  strcpy(copy, line);
  separator = strrchr(copy, ':');

  if (separator == NULL || separator == copy || separator[1] == '\0') {
    return false;
  }

  *separator = '\0';

  if (inet_pton(AF_INET, copy, &server->ip) != 1) {
    return false;
  }

  if (!ParsePort(separator + 1, &server->port)) {
    return false;
  }

  strcpy(server->ip, copy);
  return true;
}

static bool ReadServers(const char *path, struct Server **servers,
                        size_t *servers_count) {
  FILE *file = fopen(path, "r");
  char line[MAX_LINE_LENGTH];
  struct Server *items = NULL;
  size_t count = 0;

  if (file == NULL) {
    perror("fopen servers file");
    return false;
  }

  while (fgets(line, sizeof(line), file) != NULL) {
    struct Server server;
    TrimLine(line);

    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }

    if (!ParseServerLine(line, &server)) {
      fprintf(stderr, "Invalid server line: %s\n", line);
      free(items);
      fclose(file);
      return false;
    }

    struct Server *resized = realloc(items, (count + 1) * sizeof(*items));
    if (resized == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      free(items);
      fclose(file);
      return false;
    }

    items = resized;
    items[count] = server;
    ++count;
  }

  if (ferror(file)) {
    perror("fgets servers file");
    free(items);
    fclose(file);
    return false;
  }

  fclose(file);

  if (count == 0) {
    fprintf(stderr, "Servers file is empty\n");
    free(items);
    return false;
  }

  *servers = items;
  *servers_count = count;
  return true;
}

static void *RequestServer(void *argument) {
  struct ClientTask *task = argument;
  task->status = -1;

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket");
    return NULL;
  }

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(task->server.port);

  if (inet_pton(AF_INET, task->server.ip, &address.sin_addr) != 1) {
    fprintf(stderr, "Invalid IP address: %s\n", task->server.ip);
    close(socket_fd);
    return NULL;
  }

  if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    fprintf(stderr, "Connection failed for %s:%u: %s\n", task->server.ip,
            (unsigned int)task->server.port, strerror(errno));
    close(socket_fd);
    return NULL;
  }

  uint64_t request[3] = {task->begin, task->end, task->mod};

  if (SendAll(socket_fd, request, sizeof(request)) < 0) {
    fprintf(stderr, "Sending task to %s:%u failed: %s\n", task->server.ip,
            (unsigned int)task->server.port, strerror(errno));
    close(socket_fd);
    return NULL;
  }

  int receive_status = RecvAll(socket_fd, &task->result, sizeof(task->result));
  if (receive_status != 1) {
    fprintf(stderr, "Receiving result from %s:%u failed\n", task->server.ip,
            (unsigned int)task->server.port);
    close(socket_fd);
    return NULL;
  }

  printf("Server %s:%u completed [%" PRIu64 ", %" PRIu64
         "], result=%" PRIu64 "\n",
         task->server.ip, (unsigned int)task->server.port, task->begin,
         task->end, task->result);

  task->status = 0;
  shutdown(socket_fd, SHUT_RDWR);
  close(socket_fd);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0;
  uint64_t mod = 0;
  const char *servers_path = NULL;
  bool have_k = false;
  bool have_mod = false;

  static struct option options[] = {
      {"k", required_argument, NULL, 'k'},
      {"mod", required_argument, NULL, 'm'},
      {"servers", required_argument, NULL, 's'},
      {NULL, 0, NULL, 0},
  };

  while (true) {
    int option = getopt_long(argc, argv, "", options, NULL);

    if (option == -1) {
      break;
    }

    switch (option) {
    case 'k':
      if (!ConvertStringToUI64(optarg, &k)) {
        fprintf(stderr, "Invalid k: %s\n", optarg);
        return 1;
      }
      have_k = true;
      break;
    case 'm':
      if (!ConvertStringToUI64(optarg, &mod)) {
        fprintf(stderr, "Invalid mod: %s\n", optarg);
        return 1;
      }
      have_mod = true;
      break;
    case 's':
      servers_path = optarg;
      break;
    default:
      PrintUsage(argv[0]);
      return 1;
    }
  }

  if (!have_k || !have_mod || mod == 0 || servers_path == NULL ||
      optind != argc) {
    PrintUsage(argv[0]);
    return 1;
  }

  if (k == 0) {
    printf("Final answer: 1\n");
    return 0;
  }

  struct Server *servers = NULL;
  size_t servers_count = 0;

  if (!ReadServers(servers_path, &servers, &servers_count)) {
    return 1;
  }

  size_t active_servers = servers_count;
  if (k < active_servers) {
    active_servers = (size_t)k;
  }

  pthread_t *threads = calloc(active_servers, sizeof(*threads));
  struct ClientTask *tasks = calloc(active_servers, sizeof(*tasks));

  if (threads == NULL || tasks == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    free(threads);
    free(tasks);
    free(servers);
    return 1;
  }

  uint64_t base_size = k / active_servers;
  uint64_t remainder = k % active_servers;
  uint64_t current_begin = 1;

  for (size_t i = 0; i < active_servers; ++i) {
    uint64_t part_size = base_size + (i < remainder ? 1 : 0);

    tasks[i].server = servers[i];
    tasks[i].begin = current_begin;
    tasks[i].end = current_begin + part_size - 1;
    tasks[i].mod = mod;
    tasks[i].result = 0;
    tasks[i].status = -1;

    printf("Task for %s:%u: [%" PRIu64 ", %" PRIu64 "]\n",
           tasks[i].server.ip, (unsigned int)tasks[i].server.port,
           tasks[i].begin, tasks[i].end);

    if (pthread_create(&threads[i], NULL, RequestServer, &tasks[i]) != 0) {
      fprintf(stderr, "pthread_create failed\n");

      for (size_t j = 0; j < i; ++j) {
        pthread_join(threads[j], NULL);
      }

      free(threads);
      free(tasks);
      free(servers);
      return 1;
    }

    current_begin = tasks[i].end + 1;
  }

  uint64_t answer = 1 % mod;
  bool success = true;

  for (size_t i = 0; i < active_servers; ++i) {
    pthread_join(threads[i], NULL);

    if (tasks[i].status != 0) {
      success = false;
      continue;
    }

    answer = MultModulo(answer, tasks[i].result, mod);
  }

  free(threads);
  free(tasks);
  free(servers);

  if (!success) {
    fprintf(stderr, "The final result was not calculated: some servers failed\n");
    return 1;
  }

  printf("Final answer: %" PRIu64 "\n", answer);
  return 0;
}
