#include "mod_runtime.h"
#include "runtime_arm.h"

#include <cstdint>

namespace {

constexpr std::uint32_t kRtcCalcLocalTime = 0x0802F588u;
constexpr std::uint32_t kInitClockWithRtc = 0x08135130u;

constexpr std::uint32_t kSaveBlock2Ptr = 0x03005D90u;
constexpr std::uint32_t kLocalTimeOffset = 0x98u;
constexpr std::uint32_t kTimeSize = 5u;

bool absolute_rtc_enabled = false;
void (*previous_fn_entry_hook)(std::uint32_t) = nullptr;

bool ram_range(std::uint32_t addr, std::uint32_t size) {
    const std::uint32_t end = addr + size;
    if (end < addr) return false;
    if (addr >= 0x02000000u && end <= 0x02040000u) return true;
    if (addr >= 0x03000000u && end <= 0x03008000u) return true;
    return false;
}

void clear_local_time_offset() {
    const std::uint32_t save_block2 = bus_read_u32(kSaveBlock2Ptr);
    const std::uint32_t offset = save_block2 + kLocalTimeOffset;
    if (!ram_range(save_block2, kLocalTimeOffset + kTimeSize) ||
        !ram_range(offset, kTimeSize)) {
        return;
    }

    bus_write_u16(offset, 0);       // days
    bus_write_u8(offset + 2u, 0);   // hours
    bus_write_u8(offset + 3u, 0);   // minutes
    bus_write_u8(offset + 4u, 0);   // seconds
}

void emerald_absolute_rtc_hook(std::uint32_t entry_pc) {
    if (previous_fn_entry_hook) previous_fn_entry_hook(entry_pc);
    if (!absolute_rtc_enabled) return;

    if (entry_pc == kRtcCalcLocalTime ||
        entry_pc == kInitClockWithRtc) {
        clear_local_time_offset();
    }
}

void reset_absolute_rtc() {
    absolute_rtc_enabled = false;
    if (g_runtime_fn_entry_hook == emerald_absolute_rtc_hook)
        g_runtime_fn_entry_hook = previous_fn_entry_hook;
    previous_fn_entry_hook = nullptr;
}

void activate_absolute_rtc() {
    absolute_rtc_enabled = true;
    if (g_runtime_fn_entry_hook != emerald_absolute_rtc_hook)
        previous_fn_entry_hook = g_runtime_fn_entry_hook;
    g_runtime_fn_entry_hook = emerald_absolute_rtc_hook;
}

}  // namespace

GBA_MOD_CONSTRUCTOR(emerald_register_absolute_rtc_plugin) {
    (void)gba_mod_register_reset_callback(reset_absolute_rtc);
    (void)gba_mod_register_activation_plugin(
        "pokemon-emerald.absolute-rtc", activate_absolute_rtc);
}
