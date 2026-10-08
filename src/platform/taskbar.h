#pragma once

#include <windows.h>
#include "../common/defs.h"

namespace DynamicIsland {
namespace Platform {

struct TaskbarInfo {
    TaskbarPosition position = TaskbarPosition::Bottom;
    RECT taskbarRect = {0, 0, 0, 0};
    RECT monitorRect = {0, 0, 0, 0};
    RECT workAreaRect = {0, 0, 0, 0};
    bool isAutoHide = false;
    UINT dpi = 96;
};

// Query the current taskbar geometry and monitor details
TaskbarInfo QueryPrimaryTaskbar();

// Find the Shell_TrayWnd handle
HWND GetTaskbarHwnd();

} // namespace Platform
} // namespace DynamicIsland
