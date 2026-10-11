#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VARIANT="en"
ROM=""
BIOS=""
JOBS="2"

usage() {
  cat <<'EOF'
Usage:
  tools/build-android.sh --rom /path/to/mother3.gba --bios /path/to/gba_bios.bin [--variant en|jpn] [--jobs N]

This creates a PRIVATE Android build. The ROM/BIOS and generated recompilation
corpus stay local and are never committed.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rom) ROM="$2"; shift 2 ;;
    --bios) BIOS="$2"; shift 2 ;;
    --variant) VARIANT="$2"; shift 2 ;;
    --jobs) JOBS="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage; exit 2 ;;
  esac
done

[[ -n "$ROM" && -f "$ROM" ]] || { echo "--rom is required" >&2; exit 2; }
[[ -n "$BIOS" && -f "$BIOS" ]] || { echo "--bios is required" >&2; exit 2; }

ROM="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"
BIOS="$(cd "$(dirname "$BIOS")" && pwd)/$(basename "$BIOS")"

case "$VARIANT" in
  en|english|mother3en)
    VARIANT="en"
    VDIR="mother3en"
    ROM_NAME="mother3_en.gba"
    ROM_SHA="306fb8874533b0fd0796208faa8bed2db4ae77fb"
    ;;
  jp|jpn|ja|mother3)
    VARIANT="jpn"
    VDIR="mother3"
    ROM_NAME="mother3_jpn.gba"
    ROM_SHA="4f0f493e12c2a8c61b2d809af03f7abf87a85776"
    ;;
  *)
    echo "Unknown variant '$VARIANT' (use en or jpn)" >&2
    exit 2
    ;;
esac

sha1_file() {
  if command -v sha1sum >/dev/null 2>&1; then
    sha1sum "$1" | awk '{print tolower($1)}'
  else
    shasum -a 1 "$1" | awk '{print tolower($1)}'
  fi
}

[[ "$(wc -c < "$ROM" | tr -d ' ')" == "33554432" ]] || {
  echo "ROM must be exactly 33554432 bytes" >&2; exit 3;
}
ACTUAL_ROM_SHA="$(sha1_file "$ROM")"
[[ "$ACTUAL_ROM_SHA" == "$ROM_SHA" ]] || {
  echo "ROM SHA-1 mismatch: $ACTUAL_ROM_SHA" >&2
  echo "Expected: $ROM_SHA" >&2
  exit 3
}

[[ "$(wc -c < "$BIOS" | tr -d ' ')" == "16384" ]] || {
  echo "GBA BIOS must be exactly 16384 bytes" >&2; exit 3;
}
ACTUAL_BIOS_SHA="$(sha1_file "$BIOS")"
[[ "$ACTUAL_BIOS_SHA" == "300c20df6731a33952ded8c436f7f186d25d3492" ]] || {
  echo "BIOS SHA-1 mismatch: $ACTUAL_BIOS_SHA" >&2
  exit 3
}

cd "$ROOT"
git submodule update --init gbarecomp recomp-ui
git -C gbarecomp config submodule.external/arm-recomp-core.url \
  https://github.com/mstan/arm-recomp-core.git
git -C gbarecomp submodule update --init \
  external/arm-recomp-core platform/android/third_party/SDL

HOST_BUILD="$ROOT/build-android-host"
cmake -S "$ROOT" -B "$HOST_BUILD" -G Ninja -DGBAGAME_RECOMP_UI=OFF
cmake --build "$HOST_BUILD" --target gba_recompile -- -j"$JOBS"

RECOMP="$HOST_BUILD/gbarecomp_build/gba_recompile"
[[ -x "$RECOMP" ]] || { echo "gba_recompile was not produced" >&2; exit 4; }

mkdir -p "$ROOT/variants/$VDIR/roms"
cp "$ROM" "$ROOT/variants/$VDIR/roms/$ROM_NAME"
mkdir -p "$ROOT/gbarecomp/bios"
cp "$BIOS" "$ROOT/gbarecomp/bios/gba_bios.bin"

echo "Generating MOTHER 3 recompilation corpus..."
(
  cd "$ROOT/variants/$VDIR"
  "$RECOMP" \
    --rom "roms/$ROM_NAME" \
    --config game.toml \
    --config symbols/mother3_jpn.toml \
    --symbols symbols/imported_symbols.tsv \
    --data-symbols symbols/imported_data_symbols.tsv \
    --out generated \
    --max-functions 65536
)

echo "Generating native BIOS corpus..."
"$RECOMP" \
  --bios "$BIOS" \
  --out "$ROOT/gbarecomp/src/runtime/generated_bios"

command -v gradle >/dev/null 2>&1 || {
  echo "Gradle is required (8.11.x recommended)." >&2
  exit 5
}

echo "Building private Android APK..."
(
  cd "$ROOT/android"
  gradle :app:assembleRelease \
    -Pmother3Variant="$VARIANT" \
    -PgbaAbis=arm64-v8a \
    -PgbaNativeJobs="$JOBS" \
    -PprivateRom="$ROM" \
    -PprivateBios="$BIOS"
)

echo
echo "APK output:"
find "$ROOT/android/app/build/outputs/apk/release" -maxdepth 1 -name '*.apk' -print
