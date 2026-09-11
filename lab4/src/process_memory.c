#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char etext;
extern char edata;
extern char end;

char *cptr = "This message is output by the function showit()\n";
char buffer1[25];

static void ShowAddress(const char *name, const void *address) {
  printf("%-20s is at virtual address: %p\n", name, address);
}

static void showit(const char *text) {
  char *buffer2 = malloc(strlen(text) + 1);

  ShowAddress("buffer2 variable", (const void *)&buffer2);

  if (buffer2 == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  ShowAddress("allocated heap", (const void *)buffer2);

  strcpy(buffer2, text);
  printf("%s", buffer2);

  free(buffer2);
}

int main(void) {
  int i = 0;

  printf("\nProgram memory layout\n\n");

  printf("Segment boundary addresses:\n");
  ShowAddress("etext", (const void *)&etext);
  ShowAddress("edata", (const void *)&edata);
  ShowAddress("end", (const void *)&end);

  printf("\nAddresses of program objects:\n");
  ShowAddress("main", (const void *)main);
  ShowAddress("showit", (const void *)showit);
  ShowAddress("cptr", (const void *)&cptr);
  ShowAddress("buffer1", (const void *)buffer1);
  ShowAddress("local i", (const void *)&i);

  strcpy(buffer1, "A demonstration\n");
  write(STDOUT_FILENO, buffer1, strlen(buffer1));

  showit(cptr);

  return EXIT_SUCCESS;
}
