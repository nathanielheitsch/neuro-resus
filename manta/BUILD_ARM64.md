# Manta 1.6.0 on Apple Silicon (arm64) — local build notes

Upstream Manta (2016, Python 2 + CMake 2.8) does not build or run on Apple Silicon.
Patches applied in this tree:

**CMake/build**
- CMakeLists.txt: cmake_minimum_required 2.8.12 → 3.10; PythonInterp 2 → 3;
  arm64 target detection for Apple Silicon.
- src/cmake/boost.cmake: Homebrew boost (1.74+) instead of vendored 1.58; dropped
  `timer`/`unit_test_framework` components (removed in boost 1.75+); shared libs.
- src/cmake/cxxConfigure.cmake: `-std=c++14` for Apple clang (boost 1.75+ math
  headers require it).
- Unit tests (Boost.Test) gated behind BUILD_MANTA_TESTS=OFF (not ported).

**C++**
- src/c++/lib/blt_util/time_util.hpp: removed boost::timer (deleted in boost 1.75)
  → std::chrono + getrusage.

**Python 3 port (scripts + vendored pyflow in redist/)**
- Shebangs python2 → python3; removed Python-3 refusals in configManta.py /
  makeRunScript.py; basestring/iteritems/xrange/ConfigParser/`except X, e`/octal
  literals; pickle binary modes; subprocess text mode; pyflow 1.1.20 repacked
  (isAlive/isSet/_stop/_stopEvent, thread API, line-buffered text stderr).

Build: `cmake -S manta -B manta-build` then `make -j8 install` in manta-build
(requires Xcode CLT, cmake, boost: `brew install cmake boost`).

Validated on macOS arm64 (Apple clang 21): tumor/normal HCC1954 demo run reproduces
Illumina's expectedResults/somaticSV.vcf.gz exactly (6 SV calls).

Upstream: https://github.com/Illumina/manta (Illumina, Inc.)
Original build docs: docs/userGuide/installation.md
