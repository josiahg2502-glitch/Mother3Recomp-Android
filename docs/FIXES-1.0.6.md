# 1.0.6 candidate fixes

- Correct the packaged runtime ROM path to match the file imported by setup.
- Native execution for the matching generated corpus, including recognized HQ Audio; interpreter fallback for other patch outputs.
- Remove unvalidated expanded scenery. Offer a labeled Fill display (stretch) switch while retaining system bar and touch visibility controls.
- Report failed native startup once and allow diagnostic text to be copied from setup.
- Keep generated code, build caches and signing files outside Git.

Validated: release build, signature and APK configuration checks, Windows software/D3D11 menu/input tests.
Still requires the affected Snapdragon/Redmi hardware for performance and first-launch confirmation.
