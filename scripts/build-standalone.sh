#!/bin/bash
# Build standalone (no Homebrew dylib deps) arm64 binaries of wham and manta,
# sign them, and package them into the tarballs the Casks (Casks/*.rb) consume.
#
# Usage: scripts/build-standalone.sh [wham|manta|all] [--notarize]
#
# Signing: uses the first "Developer ID Application" identity in the keychain by default.
#   override with SIGN_IDENTITY="..." scripts/build-standalone.sh
# Notarization requires credentials stored via:
#   xcrun notarytool store-credentials NOTARY_PROFILE ...
set -euo pipefail
cd "$(dirname "$0")/.."

TARGET="${1:-all}"
NOTARIZE="${2:-}"
IDENTITY="${SIGN_IDENTITY:-$(security find-identity -v -p codesigning | awk -F'"' '/Developer ID Application/{print $2; exit}')}"
[ -n "$IDENTITY" ] || { echo "FATAL: no Developer ID Application identity found" >&2; exit 1; }
echo "Signing identity: $IDENTITY"

sign_tree() { # all mach-o executables under $1
    find "$1" -type f -perm +111 | while read -r f; do
        file "$f" | grep -q "Mach-O" || continue
        codesign --force --timestamp --options runtime --sign "$IDENTITY" "$f"
        codesign --verify --strict --verbose=2 "$f"
    done
}

build_wham() {
    echo "== wham =="
    make -C wham -j8
    sign_tree wham/bin
    mkdir -p dist
    tar czf dist/wham-v1.0.0-macos-arm64.tar.gz -C wham/bin wham
}

build_manta() {
    echo "== manta =="
    rm -rf manta-build manta-install
    cmake -S manta -B manta-build -DTHIS_FORCE_STATIC_LINK=ON \
        -DCMAKE_INSTALL_PREFIX="$PWD/manta-install" >/dev/null
    make -C manta-build -j8 install
    sign_tree manta-install/libexec
    # rename top-level dir to 'manta' inside the tarball without BSD-tar --transform:
    rm -rf dist/.stage && mkdir -p dist/.stage
    cp -R manta-install dist/.stage/manta
    find dist/.stage -name .DS_Store -delete
    tar czf dist/manta-v1.6.0-macos-arm64.tar.gz -C dist/.stage manta
    rm -rf dist/.stage
}

do_notarize() {
    echo "== notarize =="
    # The service accepts ZIP, not tar.gz; standalone binaries cannot be stapled.
    # Package the exact final bytes so Gatekeeper can retrieve their ticket online.
    for name in wham manta; do
        archive="dist/${name}-v$([ "$name" = wham ] && echo 1.0.0 || echo 1.6.0)-macos-arm64.tar.gz"
        tmpdir="$(mktemp -d)"
        tar xzf "$archive" -C "$tmpdir"
        ditto -c -k --keepParent "$tmpdir/$name" "$tmpdir/$name.zip"
        xcrun notarytool submit "$tmpdir/$name.zip" --keychain-profile NOTARY_PROFILE --wait
        rm -rf "$tmpdir"
    done
}

case "$TARGET" in
    wham) build_wham ;;
    manta) build_manta ;;
    all) build_wham; build_manta ;;
    *) echo "usage: $0 [wham|manta|all] [--notarize]"; exit 1 ;;
esac

[ "$NOTARIZE" = "--notarize" ] && do_notarize

echo
shasum -a 256 dist/*.tar.gz
echo "Done. Update Casks/*.rb version+sha256 from the hashes above."
