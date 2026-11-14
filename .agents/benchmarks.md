# Benchmarks

Each row says what was measured, on what, and the command that measured
it, so it can be run again. Numbers come from one machine and one run.
Read them as ratios, not as absolutes.

## mir_compile() compile time, by route (Stage 7, D17)

`Rscript tools/bench-mir-compile-routes.R 200`, from the package root with
the package installed. It uses 200 random functions from the tests'
generator: median 156 lines of MIR text, at most 617. The time is
`rf_context()` alone: the module built by the emitter, or read by MIR's
scanner, then linked. It is the median of 5 rounds, per function.

| Date | Build | Machine | Tier | Emitter | Text | Emitter / text |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-11-14 | lines split in R | x86_64 macOS, R 4.6.2 | native | 1.90 ms | 1.93 ms | 0.99 |
| 2026-11-14 | lines split in R | x86_64 macOS, R 4.6.2 | interp | 0.38 ms | 0.46 ms | 0.84 |
| 2026-11-14 | lines split in C | x86_64 macOS, R 4.6.2 | native | 1.94 ms | 2.04 ms | 0.95 |
| 2026-11-14 | lines split in C | x86_64 macOS, R 4.6.2 | interp | 0.41 ms | 0.45 ms | 0.90 |

On the native tier, MIR's generator and linking take almost all the time,
so the route changes little there: 1% to 5% between runs. On the
interpreter, skipping the scanner saves 10% to 16%. Between the two
builds the times differ by about as much as between runs, so the move to
C is not measured to change speed.
