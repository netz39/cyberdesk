#pragma once
#include <cstdint>
#include <limits>
#include <type_traits>

struct __attribute((packed)) BgrColor
{
    uint8_t blue = 0;
    uint8_t green = 0;
    uint8_t red = 0;

    constexpr BgrColor &operator*=(const float &val)
    {
        using T = std::remove_const<std::remove_reference<decltype(val)>::type>::type;
        blue = static_cast<decltype(blue)>(static_cast<T>(blue) * val);
        green = static_cast<decltype(green)>(static_cast<T>(green) * val);
        red = static_cast<decltype(red)>(static_cast<T>(red) * val);
        return *this;
    }
};

inline constexpr BgrColor operator*(BgrColor lhs, const float val)
{
    lhs *= val;
    return lhs;
}

inline constexpr BgrColor operator*(const float val, const BgrColor rhs)
{
    return rhs * val;
}

inline constexpr BgrColor operator+(const BgrColor &lhs, const BgrColor &rhs)
{
    BgrColor result;
    result.blue = lhs.blue * rhs.blue;
    result.green = lhs.green * rhs.green;
    result.red = lhs.red * rhs.red;
    return result;
}

constexpr BgrColor ColorOff{0, 0, 0};

constexpr BgrColor Red{0, 0, 255};
constexpr BgrColor Green{0, 255, 0};
constexpr BgrColor Blue{255, 0, 0};

constexpr BgrColor Yellow{0, 190, 255};
constexpr BgrColor Orange{0, 100, 255};
constexpr BgrColor Magenta{150, 0, 255};
constexpr BgrColor Turquoise{140, 255, 0};
constexpr BgrColor NeutralWhite{255, 255, 255};
constexpr BgrColor ColdWhite{255, 250, 230};

struct __attribute((packed)) BgrColorDiff
{
    int16_t blue = 0;
    int16_t green = 0;
    int16_t red = 0;

    constexpr BgrColorDiff &operator*=(const float &val)
    {
        using T = std::remove_const<std::remove_reference<decltype(val)>::type>::type;
        blue = static_cast<decltype(blue)>(static_cast<T>(blue) * val);
        green = static_cast<decltype(green)>(static_cast<T>(green) * val);
        red = static_cast<decltype(red)>(static_cast<T>(red) * val);
        return *this;
    }
};

inline constexpr BgrColor operator+(const BgrColor &lhs, const BgrColorDiff &rhs)
{
    BgrColor result;
    result.blue = lhs.blue + rhs.blue;
    result.green = lhs.green + rhs.green;
    result.red = lhs.red + rhs.red;
    return result;
}

inline constexpr BgrColorDiff operator*(BgrColorDiff lhs, const float val)
{
    lhs *= val;
    return lhs;
}

inline constexpr BgrColorDiff operator*(const float val, const BgrColorDiff rhs)
{
    return rhs * val;
}
inline constexpr BgrColorDiff operator/(BgrColorDiff lhs, const float val)
{
    using T = std::remove_const<std::remove_reference<decltype(val)>::type>::type;

    BgrColorDiff result;
    result.blue = static_cast<decltype(result.blue)>(static_cast<T>(lhs.blue) / val);
    result.green = static_cast<decltype(result.green)>(static_cast<T>(lhs.green) / val);
    result.red = static_cast<decltype(result.red)>(static_cast<T>(lhs.red) / val);
    return result;
}

inline constexpr BgrColorDiff operator-(const BgrColor &lhs, const BgrColor &rhs)
{
    BgrColorDiff result;
    using T = decltype(result.blue);
    static_assert(std::is_same<T, decltype(result.blue)>::value &&
                      std::is_same<T, decltype(result.green)>::value &&
                      std::is_same<T, decltype(result.red)>::value,
                  "Code assumes equality of types");

    result.blue = static_cast<T>(lhs.blue) - static_cast<T>(rhs.blue);
    result.green = static_cast<T>(lhs.green) - static_cast<T>(rhs.green);
    result.red = static_cast<T>(lhs.red) - static_cast<T>(rhs.red);
    return result;
}