#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace DynamicIsland {

// Taskbar docking position
enum class TaskbarPosition {
    Top,
    Bottom,
    Left,
    Right,
    Unknown
};

// Placement mode preference
enum class PlacementMode {
    Auto,
    TopEmbed,
    Floating
};

// Island visual layout state
enum class IslandState {
    Hidden,
    Compact,
    Expanded
};

// Event types supported by the Dynamic Island
enum class EventType {
    None,
    // Persistent Live Activities
    Media,
    Timer,
    MicStatus,
    // Transient HUD Notifications
    Volume,
    CapsLock,
    Power,
    Bluetooth,
    LowBattery,
    TimerDone
};

// Transient HUD duration constants (in milliseconds)
constexpr DWORD DURATION_VOLUME_MS     = 1800;
constexpr DWORD DURATION_CAPS_LOCK_MS  = 1500;
constexpr DWORD DURATION_POWER_MS      = 2400;
constexpr DWORD DURATION_BLUETOOTH_MS  = 2600;
constexpr DWORD DURATION_LOW_BATT_MS   = 2800;
constexpr DWORD DURATION_TIMER_DONE_MS = 3200;

// Configuration settings container
struct ModSettings {
    PlacementMode placement = PlacementMode::Auto;
    bool enableMedia = true;
    bool enableTimer = true;
    bool enableMicStatus = true;
    bool enableVolumeHUD = true;
    bool enableCapsLockHUD = true;
    bool enablePowerHUD = true;
    bool enableBluetoothHUD = true;
    int autoCollapseSeconds = 5;
    float animationSpeed = 1.0f;
};

// Global mod settings instance
extern ModSettings g_settings;

// Theme color definitions (ARGB / hex)
namespace Colors {
    constexpr COLORREF BACKGROUND_DARK       = RGB(11, 11, 13);
    constexpr COLORREF EMBEDDED_DARK         = RGB(30, 31, 36);
    constexpr COLORREF ACCENT_BLUE           = RGB(47, 107, 255);
    constexpr COLORREF ACCENT_GREEN          = RGB(48, 209, 88);
    constexpr COLORREF ACCENT_ORANGE         = RGB(255, 159, 10);
    constexpr COLORREF ACCENT_RED            = RGB(255, 69, 58);
    constexpr COLORREF TEXT_PRIMARY          = RGB(255, 255, 255);
    constexpr COLORREF TEXT_MUTED            = RGB(168, 171, 181);
    constexpr COLORREF BORDER_COLOR          = RGB(44, 47, 55);
}

} // namespace DynamicIsland
