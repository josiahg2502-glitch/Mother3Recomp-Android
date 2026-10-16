# MOTHER 3 for Android

Native Android frontend for Mother3Recomp, using SDL2 and gbarecomp.

## Build from source

Run `python3 tools/setup_deps.py` to fetch the exact dependency revisions and apply the source patches in `dependencies.lock.json`. Existing checkouts are preserved; a different revision fails with an explanation.

With Java 17, Android SDK 35 and NDK 27.1 installed, run `gradle -p android :app:assembleRelease -Pmother3Variant=en -PgbaAbis=arm64-v8a` (Gradle 8.11.1).

Without a private generated corpus this produces an interpreter bootstrap build. To generate the corpus from your own English v1.3 ROM and BIOS, use `tools/build-android.sh --rom <rom> --bios <bios>`. ROMs, BIOS files, generated game sources and signing keys must remain private. CI artifacts are test builds signed with a temporary debug key, not published app updates.

## Game files and display

The setup screen accepts a Japanese source ROM and supported UPS/BPS/IPS translation or mod patches. Unknown patched ROMs use the compatibility interpreter; compatibility and performance vary by patch. English v1.3 and HQ Audio use the native corpus when present. BIOS is optional.

**Hide system bars**, **Fill display (stretch)** and **Show on-screen game buttons** are independent settings. Stretch fills the drawable without generating repeated side scenery; it changes the original aspect ratio. Expanded game-world widescreen is not implemented.

Setup stays available on each launch. Failed native launches display diagnostics; successful or normal app exits do not automatically show the old log. A copy button includes device and version information for reports.

See [candidate changes](docs/FIXES-1.0.6.md) and [historical README](docs/releases/legacy-readme.md). Real Snapdragon/Redmi performance and Android 15 startup still require device confirmation.
