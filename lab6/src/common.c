#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;

  a %= mod;
  while (b > 0) {
    if (b & 1U) {
      result = (result + a) % mod;
    }
    a = (a * 2) % mod;
    b >>= 1U;
  }

  return result;
}

bool ConvertStringToUI64(const char *str, uint64_t *value) {
  char *end = NULL;

  errno = 0;
  unsigned long long parsed = strtoull(str, &end, 10);

  if (errno == ERANGE || end == str || *end != '\0') {
    return false;
  }

  *value = (uint64_t)parsed;
  return true;
}

int SendAll(int socket_fd, const void *buffer, size_t size) {
  const char *data = buffer;
  size_t sent_total = 0;

  while (sent_total < size) {
    ssize_t sent = send(socket_fd, data + sent_total, size - sent_total, MSG_NOSIGNAL);

    if (sent < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }

    if (sent == 0) {
      return -1;
    }

    sent_total += (size_t)sent;
  }

  return 0;
}

int RecvAll(int socket_fd, void *buffer, size_t size) {
  char *data = buffer;
  size_t received_total = 0;

  while (received_total < size) {
    ssize_t received =
        recv(socket_fd, data + received_total, size - received_total, 0);

    if (received < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }

    if (received == 0) {
      return 0;
    }

    received_total += (size_t)received;
  }

  return 1;
}
