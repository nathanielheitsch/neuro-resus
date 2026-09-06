# neuro-resus — Apple Silicon ports of WHAM and Manta

One-time local ports of two legacy structural-variant callers so they build and
run natively on Mac (Apple Silicon / arm64). Pre-built binaries are on the
[releases page](https://github.com/nathanielheitsch/neuro-resus/releases/tag/mac-silicon-builds)
— extract and run, no Homebrew or build steps required.

This is **not** a fork for upstream contribution — these are in-place patches
for a one-time use. All credit for the tools belongs to the original authors.

## Original repositories

| Tool | Upstream | License | Original authors |
|------|----------|---------|------------------|
| WHAM | https://github.com/zeeev/wham | See `wham/LICENSE.md` | Zachary A. Szpiech et al. |
| Manta | https://github.com/Illumina/manta | See `manta/LICENSE.txt` | Illumina, Inc. |

Both source trees are vendored **complete** in this repo (`wham/`, `manta/`),
with the nested `.git` directories absorbed so everything is browsable and
diffable in one place. Use `git log` / the patch notes below to see exactly
what changed vs upstream.

## Folders

- `wham/` — WHAM source (whamg + wham SV callers)
  - Patch notes: [`wham/BUILD_ARM64.md`](wham/BUILD_ARM64.md)
- `manta/` — Manta 1.6.0 source (Python 2 → 3, arm64)
  - Patch notes: [`manta/BUILD_ARM64.md`](manta/BUILD_ARM64.md)
  - Upstream install docs: `manta/docs/userGuide/installation.md`

## Building from source

Requires Xcode Command Line Tools and Homebrew.

```bash
# WHAM
brew install cmake libomp
cd wham && make

# Manta
brew install cmake boost
cmake -S manta -B manta-build
make -C manta-build -j8 install   # adjust prefix as needed
```

## Release binaries

From https://github.com/nathanielheitsch/neuro-resus/releases/tag/mac-silicon-builds:

- `wham-arm64-macos.tar.gz` — `whamg`, `wham` (bundled libomp, deterministic
  output, auto thread default)
- `manta-arm64-macos.tar.gz` — full Manta install, Python 3 (bundled Boost)

Both validated end-to-end on arm64 macOS: WHAM calls a synthetic 500bp deletion
identically across 25 runs/thread counts; Manta reproduces Illumina's official
demo expected results (`somaticSV.vcf.gz`) byte-for-byte.