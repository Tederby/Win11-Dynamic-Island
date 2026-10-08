#pragma once

#include <windows.h>
#include <string>

namespace DynamicIsland {
namespace Utils {

// Scale an integer value based on current DPI
int ScaleDpi(int value, UINT dpi);

// Scale a float value based on current DPI
float ScaleDpiF(float value, UINT dpi);

// Formats a duration in seconds into MM:SS
std::wstring FormatDuration(int totalSeconds);

// Clamp helper
template <typename T>
constexpr const T& Clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

// Linear interpolation
inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

} // namespace Utils
} // namespace DynamicIsland
