// MOTHER 3 recomp runner — desktop and Android entry points.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "runtime.h"
#include "runtime_arm.h"
#include "mobile_platform.h"

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

// MOTHER 3's FLASH512_V131 library runs its flash-reading routines from a
// buffer on the stack (code that reads flash may not run from the cartridge
// bus), copying them there on every call:
//   ReadFlash1       0x080911D4, 4 bytes (ldrb r0,[r0]; bx lr), copied by
//                    0x080911D8 and called through the pointer at 0x02015E14
//                    by the write/erase status polls, thousands of times per save
//   ReadFlash_Core   0x08091218, 0x24 bytes, copied by 0x0809123C
//   VerifyFlash_Core 0x080912D8, 0x30 bytes, copied by 0x0809130A, 0x080913A2
// The slot moves with the call depth and is reused for other data, so fixed
// RAM entries would be unsafe, and every call ran on the interpreter bridge,
// as did every IRQ return into a loop (the library times flash operations
// with a timer IRQ): ~20 PCs at 0x03006C1C..0x03006D80 and about a million
// interpreted instructions per save. The routines are position independent,
// so a RAM PC inside a byte-exact copy runs the static translation of the ROM
// routine at the same offset (static_resume_all gives every instruction an
// entry). ReadFlash1 is too short to identify by its bytes alone and must
// also be the copy the library's pointer names. Anything else falls through
// to normal dispatch. Same idea as EmeraldRecomp's ReadFlash1/ReadFlash_Core
// hook, plus mid-routine resumes.
struct StackRoutine {
    uint32_t rom;
    uint32_t size;
    uint32_t live_pointer;  // holds (copy | 1) while the copy is live; 0 = none
};
constexpr StackRoutine kFlashStackRoutines[] = {
    {0x080911D4u, 0x04u, 0x02015E14u},  // ReadFlash1
    {0x08091218u, 0x24u, 0u},           // ReadFlash_Core
    {0x080912D8u, 0x30u, 0u},           // VerifyFlash_Core
};

bool ram_matches_rom(uint32_t ram, uint32_t rom, uint32_t size) {
    for (uint32_t i = 0; i < size; i += 2) {
        if (bus_read_u16(ram + i) != bus_read_u16(rom + i)) return false;
    }
    return true;
}

int mother3_ram_dispatch(uint32_t pc, int thumb) {
    pc &= ~1u;
    if (!thumb || pc < 0x03000000u || pc >= 0x03008000u) return 0;
    const uint16_t opcode = bus_read_u16(pc);
    for (const StackRoutine& routine : kFlashStackRoutines) {
        for (uint32_t offset = 0; offset < routine.size; offset += 2) {
            const uint32_t copy = pc - offset;
            if (opcode != bus_read_u16(routine.rom + offset) ||
                (routine.live_pointer &&
                 bus_read_u32(routine.live_pointer) != (copy | 1u)) ||
                !ram_matches_rom(copy, routine.rom, routine.size))
                continue;
            runtime_dispatch(routine.rom + offset);
            return 1;
        }
    }
    return 0;
}

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
    // Public APKs intentionally omit ROM-derived generated C/C++. With no
    // static cart dispatch table, drive the main guest CPU through gbarecomp's
    // reference interpreter instead of entering runtime_dispatch() at PC=0.
    // A private build that regenerated generated/dispatch_table.cpp does not
    // define MOTHER3_BOOTSTRAP_INTERP and therefore uses the native recompiler.
    setenv("GBARECOMP_FORCE_INTERP", "1", 1);
#endif

    gbarecomp::RunOptions opts;
    opts.builtin_game_name = GBARECOMP_BUILTIN_NAME;
    opts.builtin_rom_sha1 =
        (sizeof(GBARECOMP_BUILTIN_SHA1) > 1) ? GBARECOMP_BUILTIN_SHA1 : nullptr;
    opts.builtin_rom_crc32 = GBARECOMP_BUILTIN_CRC32;
    opts.launcher_region =
        (sizeof(GBARECOMP_BUILTIN_REGION) > 1) ? GBARECOMP_BUILTIN_REGION : nullptr;
    opts.launcher_boxart =
        (sizeof(GBARECOMP_BOXART) > 1) ? GBARECOMP_BOXART : nullptr;
    opts.launcher_game_config = GBARECOMP_DEFAULT_GAME_CONFIG;

    // MOTHER 3 is currently rendered at the native GBA 240x160 logical view.
    // Host scaling is handled by SDL without changing guest state.
    opts.max_view_width = 240;
    opts.max_resize_view_width = 240;
    opts.max_resize_view_height = 160;
    opts.resize_driven_view = false;
    opts.freely_resizable_window = true;
    opts.launcher_expose_widescreen = false;
    opts.launcher_expose_adaptive_view = false;

    if (on_mobile) {
        opts.orientation = gbarecomp::RunOptions::Orientation::Landscape;
        opts.resume_suspend_state_on_launch = true;
        opts.ui_touch_friendly = true;
        opts.touch_pad_default = 1;
    }

#if defined(GBAGAME_RECOMP_UI)
    // Android has its own ROM/BIOS setup Activity, so the desktop pre-boot
    // launcher must not be shown there.
    if (!on_mobile && game_launcher_preboot(args, opts)) return 0;
#endif

#if !defined(MOTHER3_BOOTSTRAP_INTERP)
    // Needs the generated corpus; bootstrap builds interpret everything.
    g_runtime_ram_dispatch_hook = &mother3_ram_dispatch;
#endif

    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    return gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
}

}  // namespace

#if defined(__ANDROID__)
extern "C" int SDL_main(int argc, char** argv) {
    // The recompiled corpus can consume a much deeper host stack than a normal
    // SDLActivity thread. gbarecomp creates a dedicated, large-stack game thread.
    return gbarecomp::mobile_run_with_stack(mother3_main, argc, argv);
}
#else
int main(int argc, char** argv) {
    return mother3_main(argc, argv);
}
#endif
