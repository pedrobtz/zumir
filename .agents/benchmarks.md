# Benchmarks

Each row says what was measured, on what, and the command that measured
it, so it can be run again. Numbers come from one machine and one run.
Read them as ratios, not as absolutes.

## rfunction() compile time, by route (Stage 7, D17)

`Rscript tools/bench-rfunction-routes.R 200`, from the package root with
the package installed. It uses 200 random functions from the tests'
generator: median 156 lines of MIR text, at most 617. The time is
`rf_context()` alone: the module built by the emitter, or read by MIR's
scanner, then linked. It is the median of 5 rounds, per function.

| Date | Machine | Tier | Emitter | Text | Emitter / text |
| --- | --- | --- | --- | --- | --- |
| 2026-11-14 | x86_64 macOS, R 4.6.2 | native | 1.90 ms | 1.93 ms | 0.99 |
| 2026-11-14 | x86_64 macOS, R 4.6.2 | interp | 0.38 ms | 0.46 ms | 0.84 |

On the native tier, MIR's generator and linking take almost all the time,
so the route makes no difference there. On the interpreter, skipping the
scanner saves about a sixth.
