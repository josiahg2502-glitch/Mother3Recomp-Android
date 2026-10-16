# Mother3Recomp — MOTHER 3, Recompiled

Static recompilation of **MOTHER 3** (Game Boy Advance) to native PC, built on
the [`gbarecomp`](https://github.com/mstan/gbarecomp) framework. Forked from
[EmeraldRecomp](https://github.com/mstan/EmeraldRecomp); the Emerald sources
and variant still in this tree are inherited scaffolding.

Goals: a low-latency native build (battle combos are timed to the music),
a widescreen presentation, and a moddable game through function-level hooks.

## What "static recompilation" means here

The ROM's ARM7TDMI machine code is statically translated to native C — every
function the game runs becomes a generated C function, and the GBA BIOS is
recompiled and executed too. The `gbarecomp` runtime models the rest of the
console: PPU, APU + sound engine, DMA, timers, save chip and I/O.

Any code path the static recompiler missed runs through the built-in
interpreter the first time it's hit, is JIT-compiled to native where possible,
and is cached per ROM in `recomp_cache/<rom-sha1>/`. Misses are also written
to `recomp_master_misses_<code>.toml.frag` for review.

Only symbol metadata from the
[Kurausukun/mother3](https://github.com/Kurausukun/mother3) decompilation
(function names, addresses, sizes) enters the build. The ROM is never
redistributed; supply your own legally-dumped copy.

## Targets

| Target            | Game                          | ROM SHA-1                                  | Debug port |
|-------------------|-------------------------------|--------------------------------------------|------------|
| `Mother3Recomp`   | MOTHER 3 (Japan)              | `4f0f493e12c2a8c61b2d809af03f7abf87a85776` | 19893      |
| `Mother3RecompEN` | MOTHER 3 (English fan patch)  | `306fb8874533b0fd0796208faa8bed2db4ae77fb` | 19894      |

The runtime refuses to launch on an unrecognized ROM.

ROM locations: `variants/mother3/roms/mother3_jpn.gba` and
`variants/mother3en/roms/mother3_en.gba` (gitignored).

## Controls

Keyboard and controller bindings are in `keybinds.ini`.
Save states: **Shift+F1–F9** save to a slot, **F1–F9** load it.

## Audio latency

Audio output uses a small cushion by default so the music stays close to the
game's timing clock. Tune with environment variables:

| Variable                     | Default | Meaning |
|------------------------------|---------|---------|
| `GBARECOMP_AUDIO_TARGET_MS`  | 32      | Steady-state buffer fill (12–250 ms) |
| `GBARECOMP_AUDIO_SAMPLES`    | 512     | Device callback size in frames (128–4096) |
| `GBARECOMP_AUDIO_PREROLL_MS` | 0       | Extra startup cushion (0 = start at target) |
| `GBARECOMP_AUDIO_MAX_MS`     | target+40 | Latency ceiling; a backlog above it is skipped back to target (0 = off) |
| `GBARECOMP_AUDIO_PROBE`      | off     | `1` logs underruns and buffer fill every 2 s |

If audio crackles, raise `GBARECOMP_AUDIO_TARGET_MS` (try 40–48). The previous
defaults were 60 / 1024 / 250.

## Android

The `android-port` branch contains a native Android target built on the same
`gbarecomp` runtime. It is **not a GBA emulator wrapper**: the Android app
loads `libmain.so`, SDL2 and the shared GBA runtime directly through the NDK.

The public APK contains no Nintendo ROM or BIOS. On first launch it opens the
Android setup screen, where the player selects their own files. Both are
verified by size and SHA-1 before they are copied into app-private storage.

### Public/bootstrap APK

The repository intentionally does not commit `variants/*/generated/*.cpp`
because those files are generated from a ROM. A clean checkout therefore builds
a bootstrap APK whose missing static dispatch entries use gbarecomp's reference
interpreter bridge. This is useful for bring-up and diagnostics but is not the
performance target.

English fan-translation build (default):

```bash
cd android
gradle :app:assembleDebug -Pmother3Variant=en -PgbaAbis=arm64-v8a
```

Japanese build:

```bash
cd android
gradle :app:assembleDebug -Pmother3Variant=jpn -PgbaAbis=arm64-v8a
```

### Private native-recompiled APK

For a personal build using your own verified ROM and BIOS, the helper script
regenerates the MOTHER 3 and BIOS C/C++ corpora locally, then compiles them into
the ARM64 APK:

```bash
tools/build-android.sh \
  --variant en \
  --rom /path/to/mother3_en.gba \
  --bios /path/to/gba_bios.bin
```

The generated sources, ROM, BIOS and private APK remain ignored/local. The
Android shared shell also provides touch controls, immersive fullscreen,
safe-area handling, suspend/resume state support and external gamepad input.

## Building from source

**Prerequisites (Windows):** [MSYS2](https://www.msys2.org/) with the mingw64
toolchain, CMake 3.20+, Ninja, and SDL2 (mingw64 package). Run builds from
PowerShell with mingw64 on `PATH`.

```
git clone --recurse-submodules <this repo> Mother3Recomp
cd Mother3Recomp
cmake -S . -B build -G Ninja
cmake --build build --target Mother3RecompEN     # or Mother3Recomp
```

Regenerating the recompiled C (only after recompiler or config changes):

```
cd variants/mother3en
gba_recompile --rom roms/mother3_en.gba \
    --config game.toml \
    --config symbols/mother3_jpn.toml \
    --symbols symbols/imported_symbols.tsv \
    --data-symbols symbols/imported_data_symbols.tsv \
    --out generated --max-functions 65536
```

`--config` order matters: `game.toml` is the base and wins every conflict
(see `gbarecomp/docs/SYMBOL_OVERLAY.md`).

`gba_recompile` is built from the `gbarecomp` submodule. The generated corpus
is large — expect a multi-minute compile.

## License

PolyForm Noncommercial 1.0.0 — see [`LICENSE`](LICENSE). Third-party
components retain their own licenses.

## Legal

This project contains no copyrighted ROM data, no Nintendo BIOS, and no decomp
source — only recompiler/runtime code and symbol metadata. You must supply your
own legally-dumped ROM and BIOS. MOTHER 3 is a trademark of Nintendo / HAL
Laboratory / Shigesato Itoi; this is an unaffiliated, non-commercial
preservation and research project.
