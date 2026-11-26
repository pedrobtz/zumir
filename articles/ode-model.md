# A model from zupmxode

``` r

library(zumir)
```

[zupmxode](https://pedrobtz.github.io/zupmxode/) solves pharmacometric
models with code that zumir compiles. Its Get started article begins
with an oral dose: a depot that empties into a central compartment,
which is cleared.

``` r

m <- pmx_ode_model({
  ka <- exp(tka + eta.ka); cl <- exp(tcl + eta.cl); v <- exp(tv + eta.v)
  d/dt(depot)   <- -ka * depot
  d/dt(central) <-  ka * depot - cl / v * central
  cp <- central / v
})
```

zupmxode compiles that for you, and picks the solver. This article
solves the same two equations by hand, to show what zumir’s two
compilers do with a loop like that one: once in R through
[`mir_compile()`](https://pedrobtz.github.io/zumir/reference/mir_compile.md),
then in C through
[`cfunction()`](https://pedrobtz.github.io/zumir/reference/cfunction.md).

## The model in R

A fixed-step fourth-order Runge–Kutta solve of the two equations, after
one dose into the depot at time 0. Each step needs the one before it, so
R cannot vectorise it.

``` r

pk1_r <- function(ka, cl, v, dose, h, n) {
  cp <- numeric(n + 1)
  k <- cl / v
  depot <- dose
  central <- 0
  for (i in seq_len(n)) {
    d1 <- -ka * depot
    c1 <- ka * depot - k * central
    a <- depot + h / 2 * d1
    b <- central + h / 2 * c1
    d2 <- -ka * a
    c2 <- ka * a - k * b
    a <- depot + h / 2 * d2
    b <- central + h / 2 * c2
    d3 <- -ka * a
    c3 <- ka * a - k * b
    a <- depot + h * d3
    b <- central + h * c3
    d4 <- -ka * a
    c4 <- ka * a - k * b
    depot <- depot + h / 6 * (d1 + 2 * d2 + 2 * d3 + d4)
    central <- central + h / 6 * (c1 + 2 * c2 + 2 * c3 + c4)
    cp[i + 1] <- central / v
  }
  cp
}
```

The parameters are zupmxode’s example values, with no random effect: 100
mg, over a day, in steps of 0.01 h.

``` r

ka <- exp(0.5)
cl <- exp(1)
v <- exp(3)
h <- 0.01
n <- 2400
```

## Compile it with mir_compile()

The function is already in the subset that
[`mir_compile()`](https://pedrobtz.github.io/zumir/reference/mir_compile.md)
accepts: scalars, one vector made by
[`numeric()`](https://rdrr.io/r/base/numeric.html), a `for` loop and
arithmetic. The compiled function returns exactly what R does.

``` r

pk1 <- mir_compile(pk1_r)
cp <- pk1(ka, cl, v, dose = 100, h = h, n = n)
identical(cp, pk1_r(ka, cl, v, dose = 100, h = h, n = n))
#> [1] TRUE
```

This model has a closed form, so the solve can be checked against it.

``` r

t <- h * (0:n)
k <- cl / v
exact <- 100 * ka / (v * (ka - k)) * (exp(-k * t) - exp(-ka * t))
max(abs(cp - exact)) / max(exact)
#> [1] 3.128671e-10
```

``` r

plot(t, cp, type = "l", xlab = "time (h)", ylab = "concentration (mg/L)")
```

![Concentration rising to a peak near two hours, then falling for the
rest of the day.](ode-model_files/figure-html/unnamed-chunk-6-1.png)

## The same in C with cfunction()

The same steps in C give the same doubles. The concentrations are an
output named in `out`, and `n` is their length.

``` r

pk1_c <- cfunction("
  void pk1_c(int n, double ka, double cl, double v, double dose, double h,
             double *cp) {
    double k = cl / v, depot = dose, central = 0;
    cp[0] = 0;
    for (int i = 1; i < n; i++) {
      double d1 = -ka * depot, c1 = ka * depot - k * central;
      double a = depot + h / 2 * d1, b = central + h / 2 * c1;
      double d2 = -ka * a, c2 = ka * a - k * b;
      a = depot + h / 2 * d2;
      b = central + h / 2 * c2;
      double d3 = -ka * a, c3 = ka * a - k * b;
      a = depot + h * d3;
      b = central + h * c3;
      double d4 = -ka * a, c4 = ka * a - k * b;
      depot += h / 6 * (d1 + 2 * d2 + 2 * d3 + d4);
      central += h / 6 * (c1 + 2 * c2 + 2 * c3 + c4);
      cp[i] = central / v;
    }
  }", out = "cp", n = "n")

identical(pk1_c(n = n + 1, ka = ka, cl = cl, v = v, dose = 100, h = h), cp)
#> [1] TRUE
```

## How fast

Both compiled versions run the loop far faster than R does. The C one is
faster again, because
[`mir_compile()`](https://pedrobtz.github.io/zumir/reference/mir_compile.md)
also does what R does on every step, such as checking for an interrupt.
Timings vary by machine.

``` r

bench::mark(
  R = pk1_r(ka, cl, v, 100, h, n),
  mir_compile = pk1(ka, cl, v, 100, h, n),
  cfunction = pk1_c(n = n + 1, ka = ka, cl = cl, v = v, dose = 100, h = h),
  min_iterations = 20
)[, 1:5]
#> # A tibble: 3 × 5
#>   expression       min   median `itr/sec` mem_alloc
#>   <bch:expr>  <bch:tm> <bch:tm>     <dbl> <bch:byt>
#> 1 R             1.31ms   1.33ms      750.    18.8KB
#> 2 mir_compile  74.95µs  79.39µs    12376.    18.8KB
#> 3 cfunction    41.85µs   42.7µs    22469.    18.8KB
```

## Where zupmxode goes further

A real model has doses at many times, observations, random effects, and
equations that are stiff or have no closed form. zupmxode reads the
model in rxode2’s syntax, picks a solver (the closed form, a matrix
exponential, LSODA or Dormand–Prince), and compiles the model’s blocks
through zumir’s C API, not through these R functions. Its [Get
started](https://pedrobtz.github.io/zupmxode/articles/zupmxode.html)
article solves this model with doses every day for a week.
