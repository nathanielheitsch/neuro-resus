# Homebrew tap

Pre-built, standalone (no Homebrew dylib or python-module dependencies) arm64
binaries of the tools in this repo, installable via Homebrew casks.

## Install

```sh
# Homebrew 6 requires explicit trust before it loads third-party casks.
brew trust --cask nathanielheitsch/neuro-resus/wham nathanielheitsch/neuro-resus/manta
brew tap nathanielheitsch/neuro-resus https://github.com/nathanielheitsch/neuro-resus
brew install --cask nathanielheitsch/neuro-resus/wham nathanielheitsch/neuro-resus/manta
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

1. Run `scripts/build-standalone.sh all --notarize`, then copy its SHA-256
   values into the casks' `sha256` stanzas and bump `version`.
2. Publish the tarballs with `gh release create <tag> dist/<artifact>` (or
   `gh release upload <tag> dist/<artifact> --clobber`), then commit and push.
3. Verify locally:

```sh
brew audit --cask nathanielheitsch/neuro-resus/wham
```

## GPL note (manta)

Manta is GPLv3. Distributing its binaries via this tap requires offering the
corresponding source — this repository *is* that source (the `manta/` tree
plus `manta/BUILD_ARM64.md` build instructions), so the release page links
back to it.
