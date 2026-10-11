# MOTHER 3 Recomp — Android

This module packages the existing MOTHER 3 recomp runtime as an Android SDL2
application. The default variant is the English fan-translation build; pass
`-Pmother3Variant=jpn` for the Japanese ROM.

A public APK does not contain a ROM or GBA BIOS. The first-launch setup Activity
imports and SHA-1 verifies the player's own files.

For a private native-recompiled build, use `../tools/build-android.sh`, which
generates the ROM/BIOS-derived C++ locally before assembling the APK.
