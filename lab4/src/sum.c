#include "sum.h"

long long SumRange(const int *array, size_t begin, size_t end) {
  long long sum = 0;

  for (size_t i = begin; i < end; i++) {
    sum += array[i];
  }

  return sum;
}
