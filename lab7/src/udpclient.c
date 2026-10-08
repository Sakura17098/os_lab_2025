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
  char *sendline, *recvline;
  ssize_t n;
  struct sockaddr_in servaddr;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <server_ip> <port> <buffer_size>\n", argv[0]);
    return EXIT_FAILURE;
  }

  port = parse_number(argv[2], 1, 65535, "port");
  bufsize = parse_number(argv[3], 1, 65535, "buffer_size");
  sendline = malloc((size_t)bufsize);
  recvline = malloc((size_t)bufsize + 1);
  if (sendline == NULL || recvline == NULL) {
    perror("malloc");
    free(sendline);
    free(recvline);
    return EXIT_FAILURE;
  }

  servaddr = (struct sockaddr_in){0};
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons((unsigned short)port);

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1) {
    fprintf(stderr, "Invalid IPv4 address: %s\n", argv[1]);
    free(sendline);
    free(recvline);
    return EXIT_FAILURE;
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    free(sendline);
    free(recvline);
    return EXIT_FAILURE;
  }

  puts("Enter string (Ctrl+D to finish):");
  while ((n = read(STDIN_FILENO, sendline, (size_t)bufsize)) > 0) {
    ssize_t received;

    if (sendto(sockfd, sendline, (size_t)n, 0,
               (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
      perror("sendto");
      close(sockfd);
      free(sendline);
      free(recvline);
      return EXIT_FAILURE;
    }

    received = recvfrom(sockfd, recvline, (size_t)bufsize, 0, NULL, NULL);
    if (received < 0) {
      perror("recvfrom");
      close(sockfd);
      free(sendline);
      free(recvline);
      return EXIT_FAILURE;
    }

    recvline[received] = '\0';
    printf("REPLY FROM SERVER= %s\n", recvline);
    fflush(stdout);
  }

  if (n < 0) {
    perror("read");
    close(sockfd);
    free(sendline);
    free(recvline);
    return EXIT_FAILURE;
  }

  close(sockfd);
  free(sendline);
  free(recvline);
  return EXIT_SUCCESS;
}
