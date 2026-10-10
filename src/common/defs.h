#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <vector>

// Fallback declarations for syntax checking outside Windhawk environment
#ifndef WH_MOD
inline void Wh_Log(const wchar_t* format, ...) { (void)format; }
inline const wchar_t* Wh_GetStringSetting(const wchar_t* name, ...) { (void)name; return nullptr; }
inline void Wh_FreeStringSetting(const wchar_t* string) { (void)string; }
inline int Wh_GetIntSetting(const wchar_t* name, ...) { (void)name; return 0; }
#endif

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

// Idle visibility mode preference
enum class IdleVisibilityMode {
    AlwaysVisible,
    EventsOnly
};

// Media playback visibility policy
enum class MediaVisibilityPolicy {
    AlwaysVisible,
    TrackChangeOnly
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

// Persistent & Transient state data containers
struct MediaThumbnail {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixels; // 32bpp PBGRA
    uint64_t version = 0;
};

struct MediaState {
    std::wstring title;
    std::wstring artist;
    float progress = 0.0f;
    bool isPlaying = false;
};

struct TimerState {
    int remainingSeconds = 0;
    int totalSeconds = 0;
    bool isPaused = false;
    std::wstring label = L"Fokus";
    std::wstring session = L"Sesi 1 dari 4";
};

struct MicState {
    std::wstring title = L"Mikrofon lagi dipakai";
    std::wstring appName;
    bool isActive = false;
};

struct TransientState {
    EventType type = EventType::None;
    std::wstring title;
    std::wstring subtitle;
    int value = 0;
    float progressFraction = 0.0f;
};

// Configuration settings container
struct ModSettings {
    PlacementMode placement = PlacementMode::Auto;
    IdleVisibilityMode idleVisibilityMode = IdleVisibilityMode::AlwaysVisible;
    MediaVisibilityPolicy mediaVisibilityPolicy = MediaVisibilityPolicy::AlwaysVisible;
    bool enableMedia = true;
    bool enableTimer = true;
    bool enableMicStatus = true;
    bool enableVolumeHUD = true;
    bool enableCapsLockHUD = true;
    bool enablePowerHUD = true;
    bool enableBluetoothHUD = true;
    bool enableDebugHotkeys = true;
    int autoCollapseDelayMs = 1500; // 1.5 seconds default inactivity timeout after hover exit
    float animationSpeed = 1.0f;
};

// Global mod settings instance
extern ModSettings g_settings;

// Theme color definitions (ARGB / hex)
namespace Colors {
    inline constexpr COLORREF BACKGROUND_DARK       = RGB(11, 11, 13);
    inline constexpr COLORREF EMBEDDED_DARK         = RGB(30, 31, 36);
    inline constexpr COLORREF ACCENT_BLUE           = RGB(47, 107, 255);
    inline constexpr COLORREF ACCENT_GREEN          = RGB(48, 209, 88);
    inline constexpr COLORREF ACCENT_ORANGE         = RGB(255, 159, 10);
    inline constexpr COLORREF ACCENT_RED            = RGB(255, 69, 58);
    inline constexpr COLORREF TEXT_PRIMARY          = RGB(255, 255, 255);
    inline constexpr COLORREF TEXT_MUTED            = RGB(168, 171, 181);
    inline constexpr COLORREF BORDER_COLOR          = RGB(44, 47, 55);
}

} // namespace DynamicIsland
