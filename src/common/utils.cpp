#include "utils.h"
#include <cwchar>

namespace DynamicIsland {
namespace Utils {

int ScaleDpi(int value, UINT dpi) {
    if (dpi == 0) dpi = 96;
    return MulDiv(value, dpi, 96);
}

float ScaleDpiF(float value, UINT dpi) {
    if (dpi == 0) dpi = 96;
    return (value * static_cast<float>(dpi)) / 96.0f;
}

std::wstring FormatDuration(int totalSeconds) {
    if (totalSeconds < 0) totalSeconds = 0;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    wchar_t buf[32];
    swprintf_s(buf, L"%d:%02d", minutes, seconds);
    return std::wstring(buf);
}

} // namespace Utils
} // namespace DynamicIsland
