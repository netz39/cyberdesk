#pragma once
#include <stdint.h>

namespace can_id
{
static constexpr uint8_t LightDriverOffset = 0x10;
static constexpr uint8_t LedStripOffset = 0x05;

enum class IdBase : uint8_t
{
    Status = 0x10,
    Brightness = 0x11,
    ColorTemperature = 0x12,
    Reserved = 0x13,
    Reserved2 = 0x14,
};
} // namespace can_id

// can id examples:
//
// brightness message to LightDriver1 and LedStrip0 (long side)
// 0x11 + 1*0x10 + 0*0x05 = 0x21
//
// color temperature message to LightDriver2 and LedStrip1 (short side)
// 0x12 + 2*0x10 + 1*0x05 = 0x37
//
// global control - set all strips to same brightness
// 0x11