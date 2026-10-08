#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static int parse_number(const char *text, int min, int max, const char *name) {
  char *end;
  long value;

  errno = 0;
  value = strtol(text, &end, 10);
  if (errno != 0 || *text == '\0' || *end != '\0' ||
      value < min || value > max) {
    fprintf(stderr, "Invalid %s: %s (expected %d..%d)\n",
            name, text, min, max);
    exit(EXIT_FAILURE);
  }
  return (int)value;
}

int main(int argc, char *argv[]) {
  int fd;
  int port;
  int bufsize;
  char *buf;
  ssize_t nread;
  struct sockaddr_in servaddr;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <server_ip> <port> <buffer_size>\n", argv[0]);
    return EXIT_FAILURE;
  }

  port = parse_number(argv[2], 1, 65535, "port");
  bufsize = parse_number(argv[3], 1, 65535, "buffer_size");
  buf = malloc((size_t)bufsize);
  if (buf == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    free(buf);
    return EXIT_FAILURE;
  }

  servaddr = (struct sockaddr_in){0};
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons((unsigned short)port);

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1) {
    fprintf(stderr, "Invalid IPv4 address: %s\n", argv[1]);
    close(fd);
    free(buf);
    return EXIT_FAILURE;
  }

  if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("connect");
    close(fd);
    free(buf);
    return EXIT_FAILURE;
  }

  puts("Input message to send (Ctrl+D to finish):");
  while ((nread = read(STDIN_FILENO, buf, (size_t)bufsize)) > 0) {
    ssize_t sent = 0;
    while (sent < nread) {
      ssize_t result = write(fd, buf + sent, (size_t)(nread - sent));
      if (result <= 0) {
        perror("write");
        close(fd);
        free(buf);
        return EXIT_FAILURE;
      }
      sent += result;
    }
  }

  if (nread < 0) {
    perror("read");
    close(fd);
    free(buf);
    return EXIT_FAILURE;
  }

  close(fd);
  free(buf);
  return EXIT_SUCCESS;
}
