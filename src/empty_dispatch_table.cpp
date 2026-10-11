// Empty cart dispatch table for public/bootstrap builds.
//
// The actual static recompilation corpus is generated locally from a user's
// legally dumped ROM and intentionally is not committed. When
// generated/dispatch_table.cpp exists CMake does not compile this file.

#include <cstdint>

struct DispatchEntry {
    std::uint32_t addr;
    std::uint8_t thumb;
    std::uint8_t resume;
    void (*fn)(void);
};

extern "C" {
extern const DispatchEntry kDispatchTable[] = {
    {0u, 0u, 0u, nullptr},
};
extern const unsigned kDispatchTableLen = 0u;
}
