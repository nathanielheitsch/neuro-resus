# WHAM on Apple Silicon (arm64) — local build notes

Upstream WHAM (2015) does not build or run on Apple Silicon. Patches applied in this tree:

- `Makefile`: C/C++ split rules; ssw.c must compile as C (was getting `-std=c++0x`);
  real libomp via `-Xpreprocessor -fopenmp` + `-lomp` (Homebrew libomp).
- `src/bamtools`: `cmake_minimum_required` 2.6.4 → 3.5; std::pair fixes for modern libc++.
- `src/Complete-Striped-Smith-Waterman-Library`: ssw.c/ssw.h guard SSE2 includes with
  `__aarch64__` → `sse2neon.h`; ssw_cpp.cpp replaced with a portable pure-C++ Smith-Waterman
  implementing the same API (see file header for details/limits).
- `src/bin/whamg.cpp`: init the three omp locks before `allStats` (upstream never initialized
  them — crashes under libomp); renamed global `lock` variables to avoid `std::lock` ambiguity.
- `src/bin/wham.cpp`: `rand() % (max-1)` division-by-zero when BAM header has 1 sequence.

Build: `make` (requires: Xcode CLT, cmake, libomp: `brew install cmake libomp`).
Binaries: `bin/whamg`, `bin/wham`.

Validated on macOS arm64 (Apple clang 21): whamg calls a synthetic 500bp DEL correctly
(chr1:9997 <DEL>, 30 supporting reads); wham finishes normally on the same BAM.

Determinism fix: upstream getTree() collected graph nodes into a map keyed by
heap pointers and iterated it — node order depended on ASLR, so output varied
run to run (same binary, same input). Fixed by ordering nodes by allocation
sequence instead. Output is now identical across runs and thread counts
(-x 1/2/4/8/auto, 25/25 identical in testing) and matches upstream's majority
behavior (chr1:9999 T <DEL> on the synthetic 500bp DEL test).

Threading: whamg now defaults -x to auto (all logical cores via
sysconf(_SC_NPROCESSORS_ONLN)) unless the user passes -x explicitly.
Also fixed: findPairs could dereference NULL when a graph had no connected
pair (segfault on some inputs).
