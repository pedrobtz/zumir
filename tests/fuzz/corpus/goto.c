int collatz(unsigned long n) {
  int steps = 0;
again:
  if (n == 1) return steps;
  n = (n % 2) ? 3 * n + 1 : n / 2;
  steps++;
  goto again;
}
