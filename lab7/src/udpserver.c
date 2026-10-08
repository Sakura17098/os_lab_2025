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
  int sockfd, port, bufsize;
  char *mesg;
  char ipadr[INET_ADDRSTRLEN];
  ssize_t n;
  struct sockaddr_in servaddr, cliaddr;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <port> <buffer_size>\n", argv[0]);
    return EXIT_FAILURE;
  }

  port = parse_number(argv[1], 1, 65535, "port");
  bufsize = parse_number(argv[2], 1, 65535, "buffer_size");
  mesg = malloc((size_t)bufsize + 1);
  if (mesg == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    free(mesg);
    return EXIT_FAILURE;
  }

  servaddr = (struct sockaddr_in){0};
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons((unsigned short)port);

  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind");
    close(sockfd);
    free(mesg);
    return EXIT_FAILURE;
  }

  printf("UDP server listening on port %d\n", port);
  fflush(stdout);

  while (1) {
    socklen_t len = sizeof(cliaddr);
    n = recvfrom(sockfd, mesg, (size_t)bufsize, 0,
                 (struct sockaddr *)&cliaddr, &len);
    if (n < 0) {
      perror("recvfrom");
      continue;
    }

    mesg[n] = '\0';
    const char *ip = inet_ntop(AF_INET, &cliaddr.sin_addr,
                               ipadr, sizeof(ipadr));
    printf("REQUEST %s FROM %s:%u\n", mesg,
           ip != NULL ? ip : "unknown", ntohs(cliaddr.sin_port));
    fflush(stdout);

    if (sendto(sockfd, mesg, (size_t)n, 0,
               (struct sockaddr *)&cliaddr, len) < 0) {
      perror("sendto");
    }
  }
}
