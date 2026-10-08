#pragma once

#include <windows.h>
#include <cwchar>

namespace DynamicIsland {

// Logging helper forwarding to Windhawk logging framework
inline void LogInfo(const wchar_t* format, ...) {
    wchar_t buffer[1024];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(buffer, sizeof(buffer) / sizeof(wchar_t), _TRUNCATE, format, args);
    va_end(args);

    Wh_Log(L"[DynamicIsland] %s", buffer);
}

inline void LogError(const wchar_t* format, ...) {
    wchar_t buffer[1024];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(buffer, sizeof(buffer) / sizeof(wchar_t), _TRUNCATE, format, args);
    va_end(args);

    Wh_Log(L"[DynamicIsland:ERROR] %s", buffer);
}

} // namespace DynamicIsland
