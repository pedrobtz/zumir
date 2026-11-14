#include <stdbool.h>
#include <limits.h>
int classify(int x) {
  switch (x) {
  case INT_MIN: return -2;
  case 0: return 0;
  case 1: case 2: case 3: return 1;
  default: return x < 0 ? -1 : 2;
  }
}
bool even(long long v) { return (v & 1) == 0; }
