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
