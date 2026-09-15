#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

bool ConvertStringToUI64(const char *str, uint64_t *value);

int SendAll(int socket_fd, const void *buffer, size_t size);

int RecvAll(int socket_fd, void *buffer, size_t size);

#endif
