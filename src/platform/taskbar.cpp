#include "taskbar.h"
#include <shellapi.h>

namespace DynamicIsland {
namespace Platform {

HWND GetTaskbarHwnd() {
    return FindWindowW(L"Shell_TrayWnd", nullptr);
}

TaskbarInfo QueryPrimaryTaskbar() {
    TaskbarInfo info{};

    HWND hTaskbar = GetTaskbarHwnd();
    if (hTaskbar) {
        GetWindowRect(hTaskbar, &info.taskbarRect);
    }

    APPBARDATA abd{};
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hTaskbar;

    SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
    info.taskbarRect = abd.rc;

    switch (abd.uEdge) {
        case ABE_TOP:    info.position = TaskbarPosition::Top; break;
        case ABE_BOTTOM: info.position = TaskbarPosition::Bottom; break;
        case ABE_LEFT:   info.position = TaskbarPosition::Left; break;
        case ABE_RIGHT:  info.position = TaskbarPosition::Right; break;
        default:         info.position = TaskbarPosition::Bottom; break;
    }

    // Check Auto-Hide state
    UINT_PTR state = SHAppBarMessage(ABM_GETSTATE, &abd);
    info.isAutoHide = (state & ABS_AUTOHIDE) != 0;

    // Monitor rect & work area
    HMONITOR hMon = MonitorFromWindow(hTaskbar ? hTaskbar : GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{};
    mi.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoW(hMon, &mi)) {
        info.monitorRect = mi.rcMonitor;
        info.workAreaRect = mi.rcWork;
    }

    // DPI query
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
