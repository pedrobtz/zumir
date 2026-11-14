#define SQ(x) ((x) * (x))
#define CAT(a, b) a##b
#if defined(SQ) && !defined(NOPE)
double CAT(f, 1)(double x) { return SQ(x) + M_PI; }
#else
#error not here
#endif
