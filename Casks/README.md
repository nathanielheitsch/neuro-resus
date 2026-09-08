# Homebrew tap

Pre-built, standalone (no Homebrew dylib or python-module dependencies) arm64
binaries of the tools in this repo, installable via Homebrew casks.

## Install

```sh
brew install --cask nathanielheitsch/neuro-resus/wham
brew install --cask nathanielheitsch/neuro-resus/manta
```

Homebrew auto-taps this repo on first use; or explicitly:

```sh
brew tap nathanielheitsch/neuro-resus https://github.com/nathanielheitsch/neuro-resus
brew install --cask wham   # or manta
```

## What gets installed

- **wham** — single `wham` executable linked against static libomp (no
  `/opt/homebrew` dylib references; only system libz/libSystem).
- **manta** — the full relocatable install tree under
  `$(brew --prefix)/Caskroom/manta/<version>/`, with `configManta.py` and
  `runMantaWorkflowDemo.py` symlinked into `$(brew --prefix)/bin/`. All C++
  binaries are linked against static boost. Requires only `/usr/bin/python3`
  (macOS system python3) — the workflow engine (pyflow) is bundled.

Both casks declare `depends_on arch: :arm64` (Apple Silicon only).

## Releasing a new version

1. Push a tag `wham-vX.Y.Z` or `manta-vX.Y.Z` — the `release` workflow builds
   the standalone artifact and attaches it to a GitHub release.
2. Copy the sha256 from the workflow log (or `shasum -a 256 <artifact>`) into
   the cask's `sha256` stanza, bump `version`, commit, push.
3. Verify locally:

```sh
brew audit --cask nathanielheitsch/neuro-resus/wham
```

## GPL note (manta)

Manta is GPLv3. Distributing its binaries via this tap requires offering the
corresponding source — this repository *is* that source (the `manta/` tree
plus `manta/BUILD_ARM64.md` build instructions), so the release page links
back to it.
