#pragma once

#include <optional>
#include <stdint.h>

namespace can_id
{
static constexpr uint8_t LightDriverOffset = 0x10;

enum class IdBase : uint8_t
{
    Status = 0x10,
    Brightness = 0x11,
    ColorTemperature = 0x12
};

enum class LedType : uint8_t
{
    LongSide = 0,
    ShortSide = 0x05,
};

constexpr uint8_t buildGlobalId(IdBase command)
{
    // ledDriver index is set to 0 and led type is not relevant for global commands
    return static_cast<uint8_t>(command);
}

/// Builds the CAN identifier for a given command, light driver index and led type
constexpr uint8_t buildId(IdBase command, uint8_t ledDriverIndex, LedType ledType)
{
    return static_cast<uint8_t>(command) + ledDriverIndex * LightDriverOffset + static_cast<uint8_t>(ledType);
}

constexpr uint8_t buildBrightnessId(uint8_t driver, LedType led)
{
    return buildId(IdBase::Brightness, driver, led);
}

constexpr uint8_t buildColorTemperatureId(uint8_t driver, LedType led)
{
    return buildId(IdBase::ColorTemperature, driver, led);
}

struct DecodedId
{
    IdBase command;
    uint8_t ledDriverIndex;
    LedType ledType;
    bool isGlobal;
};

constexpr std::optional<DecodedId> decodeId(uint8_t canId)
{
    if (canId < static_cast<uint8_t>(IdBase::Status))
        // ID is out of valid range
        return std::nullopt;

    DecodedId result{};

    result.ledDriverIndex = (canId - static_cast<uint8_t>(IdBase::Status)) / LightDriverOffset;
    result.isGlobal = (result.ledDriverIndex == 0); // global if no driver offset applied

    // Remainder contains base command + LED type
    const uint8_t remainder = canId % LightDriverOffset;

    if (remainder >= static_cast<uint8_t>(LedType::ShortSide))
    {
        // command related to short side strip
        result.ledType = LedType::ShortSide;
        result.command = static_cast<IdBase>(remainder - static_cast<uint8_t>(LedType::ShortSide));
    }
    else
    {
        // command related to long side strip
        result.ledType = LedType::LongSide;
        result.command = static_cast<IdBase>(remainder);
    }

    return result;
}

} // namespace can_id