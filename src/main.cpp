// MOTHER 3 recomp runner â€” desktop and Android entry points.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "runtime.h"
#include "mobile_platform.h"
#include "mother3_runtime_hooks.h"
#if defined(__ANDROID__)
#include "ips_patch.h"
#include <jni.h>
#endif

#ifndef GBARECOMP_BUILTIN_NAME
#define GBARECOMP_BUILTIN_NAME "MOTHER 3"
#endif
#ifndef GBARECOMP_BUILTIN_SHA1
#define GBARECOMP_BUILTIN_SHA1 ""
#endif
#ifndef GBARECOMP_BUILTIN_CRC32
#define GBARECOMP_BUILTIN_CRC32 0
#endif
#ifndef GBARECOMP_BUILTIN_REGION
#define GBARECOMP_BUILTIN_REGION ""
#endif
#ifndef GBARECOMP_WINDOW_TITLE
#define GBARECOMP_WINDOW_TITLE "Mother3Recomp"
#endif
#ifndef GBARECOMP_PROGRAM_NAME
#define GBARECOMP_PROGRAM_NAME "./Mother3Recomp"
#endif
#ifndef GBARECOMP_DEFAULT_GAME_CONFIG
#define GBARECOMP_DEFAULT_GAME_CONFIG "variants/mother3en/game.toml"
#endif
#ifndef GBARECOMP_BOXART
#define GBARECOMP_BOXART ""
#endif

#if defined(GBAGAME_RECOMP_UI)
#include "game_launcher_boot.h"
#endif

namespace {

void print_usage() {
    std::printf(
        "%s [--bios <path>] [--rom <path>] [game.toml]\n"
        "\n"
        "A matching MOTHER 3 ROM and GBA BIOS are required.\n"
        "Android imports both through the setup screen; desktop may use\n"
        "--rom / --bios or game.toml.\n",
        GBARECOMP_WINDOW_TITLE);
}

int mother3_main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 ||
            std::strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        }
    }

    std::vector<std::string> args(argv, argv + argc);

    // Android switches the working directory to app-private storage, redirects
    // diagnostics to android-runtime.log and appends the packaged game.toml.
    // This is a no-op on desktop.
    gbarecomp::MobileProcessOptions mobile;
    mobile.game_config = GBARECOMP_DEFAULT_GAME_CONFIG;
    mobile.program_name = GBARECOMP_PROGRAM_NAME;
    const bool on_mobile = gbarecomp::mobile_prepare_process(args, mobile);

#if defined(__ANDROID__)
    // The Android shell in this gbarecomp revision appends --no-launcher, but
    // this runtime revision does not expose that CLI option. Android already
    // bypasses the desktop preboot launcher below, so simply remove the stale
    // compatibility flag before run_game() parses argv.
    args.erase(std::remove(args.begin(), args.end(), std::string("--no-launcher")),
               args.end());
#endif

#if defined(__ANDROID__) && defined(MOTHER3_BOOTSTRAP_INTERP)
    // Android's interpreter build supports language patches that alter guest
    // code, regardless of whether this checkout contains generated code.
    setenv("GBARECOMP_FORCE_INTERP", "1", 1);
#endif

    mother3_install_runtime_hooks();
    gbarecomp::RunOptions opts;
    opts.builtin_game_name = GBARECOMP_BUILTIN_NAME;
    opts.builtin_rom_sha1 =
        (sizeof(GBARECOMP_BUILTIN_SHA1) > 1) ? GBARECOMP_BUILTIN_SHA1 : nullptr;
    opts.builtin_rom_crc32 = GBARECOMP_BUILTIN_CRC32;
    opts.alt_rom_sha1 = "4ffe71ce5c5e1e8d9be5fbd09968b60787169b21";
    opts.launcher_enable_rom_patches = true;
    opts.launcher_source_rom_sha1 = "4f0f493e12c2a8c61b2d809af03f7abf87a85776";
    opts.launcher_accept_patched_rom = true;
    opts.rom_post_load_hook = &mother3_apply_hq_audio_patch;
    opts.launcher_region =
        (sizeof(GBARECOMP_BUILTIN_REGION) > 1) ? GBARECOMP_BUILTIN_REGION : nullptr;
    opts.launcher_boxart =
        (sizeof(GBARECOMP_BOXART) > 1) ? GBARECOMP_BOXART : nullptr;
    opts.launcher_game_config = GBARECOMP_DEFAULT_GAME_CONFIG;

    // MOTHER 3 has no validated scenery provider beyond the native viewport.
    opts.max_view_width = 240;
    opts.max_resize_view_width = 240;
    opts.max_resize_view_height = 160;
    opts.freely_resizable_window = true;
    opts.launcher_expose_widescreen = false;
    opts.launcher_expose_adaptive_view = false;
    opts.runtime_menu_scancode = 68;  // F11

    if (on_mobile) {
        opts.orientation = gbarecomp::RunOptions::Orientation::Landscape;
        opts.resume_suspend_state_on_launch = true;
        opts.ui_touch_friendly = true;
        opts.touch_pad_default = 1;
    }

#if defined(__ANDROID__)
    // Android validates imported ROMs in the setup Activity. Patches can change
    // the target hash; the runtime selects the interpreter for mismatched ROMs.
    opts.allow_rom_sha1_mismatch = true;
#endif

#if defined(GBAGAME_RECOMP_UI)
    // Android has its own ROM/BIOS setup Activity, so the desktop pre-boot
    // launcher must not be shown there.
    if (!on_mobile && game_launcher_preboot(args, opts)) return 0;
#endif

    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    return gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
}

}  // namespace

#if defined(__ANDROID__)
extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_gbarecomp_GbaNative_applyRomPatch(JNIEnv* env, jclass, jbyteArray source,
                                           jbyteArray patch) {
    if (!source || !patch) return nullptr;
    const jsize source_size = env->GetArrayLength(source);
    const jsize patch_size = env->GetArrayLength(patch);
    std::vector<uint8_t> source_bytes(static_cast<size_t>(source_size));
    std::vector<uint8_t> patch_bytes(static_cast<size_t>(patch_size));
    env->GetByteArrayRegion(source, 0, source_size,
                            reinterpret_cast<jbyte*>(source_bytes.data()));
    env->GetByteArrayRegion(patch, 0, patch_size,
                            reinterpret_cast<jbyte*>(patch_bytes.data()));
    if (env->ExceptionCheck()) return nullptr;

    uint8_t* output = nullptr;
    size_t output_size = 0;
    if (!rom_patch_apply(source_bytes.data(), source_bytes.size(),
                         patch_bytes.data(), patch_bytes.size(),
                         &output, &output_size)) return nullptr;
    if (output_size > static_cast<size_t>(std::numeric_limits<jsize>::max())) {
        std::free(output);
        return nullptr;
    }
    jbyteArray result = env->NewByteArray(static_cast<jsize>(output_size));
    if (result)
        env->SetByteArrayRegion(result, 0, static_cast<jsize>(output_size),
                                reinterpret_cast<const jbyte*>(output));
    std::free(output);
    return env->ExceptionCheck() ? nullptr : result;
}
#endif

#if defined(__ANDROID__)
extern "C" int SDL_main(int argc, char** argv) {
    // The recompiled corpus can consume a much deeper host stack than a normal
    // SDLActivity thread. gbarecomp creates a dedicated, large-stack game thread.
    const int result = gbarecomp::mobile_run_with_stack(mother3_main, argc, argv);
    if (std::FILE* status = std::fopen("android-exit-code", "w")) {
        std::fprintf(status, "%d", result);
        std::fclose(status);
    }
    return result;
}
#else
int main(int argc, char** argv) {
    return mother3_main(argc, argv);
}
#endif
