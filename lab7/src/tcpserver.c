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
  int lfd, cfd;
  int port, bufsize;
  char *buf;
  ssize_t nread;
  struct sockaddr_in servaddr, cliaddr;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <port> <buffer_size>\n", argv[0]);
    return EXIT_FAILURE;
  }

  port = parse_number(argv[1], 1, 65535, "port");
  bufsize = parse_number(argv[2], 1, 65535, "buffer_size");
  buf = malloc((size_t)bufsize);
  if (buf == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd < 0) {
    perror("socket");
    free(buf);
    return EXIT_FAILURE;
  }

  int reuse = 1;
  if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    perror("setsockopt");
    close(lfd);
    free(buf);
    return EXIT_FAILURE;
  }

  servaddr = (struct sockaddr_in){0};
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons((unsigned short)port);

  if (bind(lfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind");
    close(lfd);
    free(buf);
    return EXIT_FAILURE;
  }

  if (listen(lfd, 5) < 0) {
    perror("listen");
    close(lfd);
    free(buf);
    return EXIT_FAILURE;
  }

  printf("TCP server listening on port %d\n", port);
  fflush(stdout);

  while (1) {
    socklen_t clilen = sizeof(cliaddr);
    cfd = accept(lfd, (struct sockaddr *)&cliaddr, &clilen);
    if (cfd < 0) {
      perror("accept");
      continue;
    }
    puts("connection established");
    fflush(stdout);

    while ((nread = read(cfd, buf, (size_t)bufsize)) > 0) {
      ssize_t written = 0;
      while (written < nread) {
        ssize_t result = write(STDOUT_FILENO, buf + written,
                               (size_t)(nread - written));
        if (result <= 0) {
          perror("write stdout");
          close(cfd);
          close(lfd);
          free(buf);
          return EXIT_FAILURE;
        }
        written += result;
      }
    }

    if (nread < 0) {
      perror("read");
    }
    close(cfd);
  }
}
