#!/usr/bin/env bash
# import-mother3-symbols.sh — build the pinned MOTHER 3 decomp and import its
# symbols into variants/mother3 (JP) and variants/mother3en (EN fan patch).
#
# Run under WSL (or any Linux). Needs one of:
#   * an arm-none-eabi toolchain with $DEVKITARM/bin/arm-none-eabi-{cpp,as,ld,objcopy}
#     (Ubuntu: apt install gcc-arm-none-eabi binutils-arm-none-eabi; DEVKITARM=/usr,
#      or devkitPro's devkitARM), or
#   * docker (the decomp's own build.sh uses the devkitpro/devkitarm image).
#
# third_party/mother3 is the PROVENANCE record: its pinned commit is the decomp
# revision we trust. Point it at your fork with
#   git submodule set-url third_party/mother3 https://github.com/<you>/mother3
# and bump it with a normal submodule update. The build itself happens in a
# Linux-side clone ($SANDBOX), because agbcc installs into the tree and builds
# over /mnt/c are slow.
#
# Gate: the built mother3.gba must hash to the JP ROM's SHA-1, or nothing is
# imported (symbols from a non-matching build would be wrong for our ROM).
#
# Usage: tools/decomp/import-mother3-symbols.sh [--force-clone]
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SUB="third_party/mother3"
JP_SHA1="4f0f493e12c2a8c61b2d809af03f7abf87a85776"
EN_SHA1="306fb8874533b0fd0796208faa8bed2db4ae77fb"
ID="mother3_jpn"
SANDBOX="${DECOMP_SANDBOX:-$HOME/mother3-build}"
J="${J:-$(nproc)}"
IMPORTER="$REPO/gbarecomp/tools/symbol_import/import_decomp_symbols.py"
AGBCC_URL="https://github.com/notyourav/agbcc/releases/download/cp/agbcc.tar.gz"

say() { printf '==> %s\n' "$*"; }
die() { echo "error: $*" >&2; exit 1; }

[ -f "$IMPORTER" ] || die "missing $IMPORTER (is the gbarecomp submodule checked out?)"

URL="$(git -C "$REPO" config -f .gitmodules "submodule.$SUB.url" || true)"
[ -n "$URL" ] || die "$SUB is not registered in .gitmodules"
# Pinned revision: committed gitlink if present, else the staged one.
PINNED="$(git -C "$REPO" ls-tree HEAD "$SUB" | awk '{print $3}')"
[ -n "$PINNED" ] || PINNED="$(git -C "$REPO" ls-files -s "$SUB" | awk '{print $2}')"
[ -n "$PINNED" ] || die "$SUB has no pinned revision"
say "decomp: $URL @ $PINNED"

if ! { [ -n "${DEVKITARM:-}" ] && [ -x "$DEVKITARM/bin/arm-none-eabi-as" ]; } && ! docker info >/dev/null 2>&1; then
    die "no working toolchain: \$DEVKITARM/bin/arm-none-eabi-as not found and docker
is not usable. Easiest on Ubuntu (the Makefile only needs cpp/as/ld/objcopy):
  sudo apt install -y gcc-arm-none-eabi binutils-arm-none-eabi
  echo 'export DEVKITARM=/usr' >> ~/.bashrc && source ~/.bashrc
(devkitARM from devkitPro also works: DEVKITARM=/opt/devkitpro/devkitARM)"
fi

# ── 1. sandbox clone at the pinned revision ─────────────────────────
[ "${1:-}" = "--force-clone" ] && rm -rf "$SANDBOX"
if [ ! -d "$SANDBOX/.git" ]; then
    say "cloning into $SANDBOX"
    git clone --quiet "$URL" "$SANDBOX"
fi
git -C "$SANDBOX" remote set-url origin "$URL"
git -C "$SANDBOX" fetch --quiet origin
git -C "$SANDBOX" checkout --quiet --detach "$PINNED"

# ── 2. baserom (the decomp extracts assets from it) ─────────────────
find_rom() {  # $1 = variant dir, $2 = sha1
    local f
    for f in "$REPO/variants/$1/roms/"*.gba; do
        [ -f "$f" ] || continue
        [ "$(sha1sum "$f" | cut -d' ' -f1)" = "$2" ] && { echo "$f"; return; }
    done
}
JP_ROM="$(find_rom mother3 "$JP_SHA1")"
[ -n "$JP_ROM" ] || die "no JP ROM with sha1 $JP_SHA1 in variants/mother3/roms"
cp "$JP_ROM" "$SANDBOX/baserom.gba"

# ── 3. agbcc (prebuilt, per the decomp's INSTALL.md) ────────────────
if [ ! -d "$SANDBOX/tools/agbcc/bin" ]; then
    say "fetching prebuilt agbcc"
    mkdir -p "$SANDBOX/tools/agbcc"
    curl -sSL "$AGBCC_URL" | tar xz -C "$SANDBOX/tools/agbcc"
fi

# ── 4. build ────────────────────────────────────────────────────────
cd "$SANDBOX"
run_make() {
    if [ -n "${DEVKITARM:-}" ] && [ -x "$DEVKITARM/bin/arm-none-eabi-as" ]; then
        make "$@"
    elif docker info >/dev/null 2>&1; then
        docker run --platform=linux/amd64 --rm -v "$PWD":/work -w /work \
            -e DEVKITPRO=/opt/devkitpro -e DEVKITARM=/opt/devkitpro/devkitARM \
            devkitpro/devkitarm:latest bash -lc "make $*"
    else
        die "no working toolchain (see the check at the top of this script)"
    fi
}
if [ ! -f .m3recomp_setup_done ]; then
    say "make setup (asset extraction, first run only)"
    run_make setup
    touch .m3recomp_setup_done
fi
say "building mother3.gba"
run_make -j"$J" -s

got="$(sha1sum mother3.gba | cut -d' ' -f1)"
[ "$got" = "$JP_SHA1" ] || die "built sha1 $got != $JP_SHA1; refusing to import"
say "mother3.gba sha1 OK"

# ── 5. import into the JP variant ───────────────────────────────────
READELF="$(command -v arm-none-eabi-readelf || command -v readelf)"
out="$REPO/variants/mother3/symbols"
mkdir -p "$out"
"$READELF" -sW mother3.elf > "$out/mother3_syms.txt"
"$READELF" -SW mother3.elf > "$out/mother3_sections.txt"
echo "$PINNED" > "$out/mother3_revision.txt"

say "importing $ID"
python3 "$IMPORTER" \
    --id "$ID" --name "MOTHER 3 (JPN)" \
    --syms "$out/mother3_syms.txt" --sections "$out/mother3_sections.txt" \
    --rom "$JP_ROM" \
    --code-copy-pair "SoundMainRAM_Buffer=SoundMainRAM:thumb" \
    --out "$out"
# Keep the overlay name the build scripts and game.toml comments use.
mv -f "$out/${ID}_symbols.toml" "$out/${ID}.toml"

# ── 6. mirror into the EN variant (same code layout; EN identity) ───
en="$REPO/variants/mother3en/symbols"
mkdir -p "$en"
cp -f "$out"/imported_symbols.tsv "$out"/imported_data_symbols.tsv \
      "$out"/function_boundaries.tsv "$en/" 2>/dev/null || true
sed -E "s/^sha1 = \"[0-9A-Fa-f]{40}\"/sha1 = \"$(echo "$EN_SHA1" | tr a-f A-F)\"/" \
    "$out/${ID}.toml" > "$en/${ID}.toml"

say "done. Regenerate + rebuild from PowerShell:"
say "  .\\tools\\build.ps1 -Regen -Target Mother3RecompEN"
say "  .\\tools\\build.ps1 -Regen -Target Mother3Recomp"
