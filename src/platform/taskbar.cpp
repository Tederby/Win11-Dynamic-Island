#include "taskbar.h"
#include <shellapi.h>

namespace DynamicIsland {
namespace Platform {

HWND GetTaskbarHwnd() {
    return FindWindowW(L"Shell_TrayWnd", nullptr);
}

TaskbarInfo QueryPrimaryTaskbar() {
    TaskbarInfo info{};

    // 1. Primary monitor info
    HWND hDesktop = GetDesktopWindow();
    HMONITOR hMon = MonitorFromWindow(hDesktop, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{};
    mi.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoW(hMon, &mi)) {
        info.monitorRect = mi.rcMonitor;
        info.workAreaRect = mi.rcWork;
    } else {
        info.monitorRect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        info.workAreaRect = info.monitorRect;
    }

    // 2. Query taskbar window rect
    HWND hTaskbar = GetTaskbarHwnd();
    RECT realRect{};
    bool hasHwndRect = (hTaskbar && GetWindowRect(hTaskbar, &realRect) && (realRect.right > realRect.left));

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hTaskbar;

    UINT_PTR res = SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
    if (res && (abd.rc.right > abd.rc.left)) {
        info.taskbarRect = abd.rc;
    } else if (hasHwndRect) {
        info.taskbarRect = realRect;
    } else {
        // Fallback: top edge compact
        info.taskbarRect = { info.monitorRect.left, info.monitorRect.top, info.monitorRect.right, info.monitorRect.top + 34 };
    }

    // 3. Robust geometric edge detection (immune to SHAppBarMessage reporting errors)
    int monHeight = info.monitorRect.bottom - info.monitorRect.top;

    if (info.taskbarRect.top <= info.monitorRect.top + 10 && info.taskbarRect.bottom < info.monitorRect.bottom - (monHeight / 2)) {
        info.position = TaskbarPosition::Top;
    } else if (info.taskbarRect.bottom >= info.monitorRect.bottom - 10 && info.taskbarRect.top > info.monitorRect.top + (monHeight / 2)) {
        info.position = TaskbarPosition::Bottom;
    } else if (info.taskbarRect.left <= info.monitorRect.left + 10) {
        info.position = TaskbarPosition::Left;
    } else if (info.taskbarRect.right >= info.monitorRect.right - 10) {
        info.position = TaskbarPosition::Right;
    } else {
        // Fallback to abd.uEdge if geometric was inconclusive
        switch (abd.uEdge) {
            case ABE_TOP:    info.position = TaskbarPosition::Top; break;
            case ABE_BOTTOM: info.position = TaskbarPosition::Bottom; break;
            case ABE_LEFT:   info.position = TaskbarPosition::Left; break;
            case ABE_RIGHT:  info.position = TaskbarPosition::Right; break;
            default:         info.position = TaskbarPosition::Top; break;
        }
    }

    // 4. Auto-hide check
    UINT_PTR state = SHAppBarMessage(ABM_GETSTATE, &abd);
    info.isAutoHide = (state & ABS_AUTOHIDE) != 0;

    // 5. DPI query
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        using GetDpiForWindow_t = UINT(WINAPI*)(HWND);
        auto pGetDpi = reinterpret_cast<GetDpiForWindow_t>(GetProcAddress(hUser32, "GetDpiForWindow"));
        if (pGetDpi && hTaskbar) {
            info.dpi = pGetDpi(hTaskbar);
        }
    }
    if (info.dpi == 0) {
        info.dpi = 96;
    }

    return info;
}

} // namespace Platform
} // namespace DynamicIsland
