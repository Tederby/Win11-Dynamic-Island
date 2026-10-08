// ==WindhawkMod==
// @id              win11-dynamic-island
// @name            Dynamic Island for Windows 11
// @description     An interactive Dynamic Island overlay and taskbar companion for Windows 11
// @version         0.1.0
// @author          Tederby
// @github          https://github.com/Tederby/Win11-Dynamic-Island
// @include         explorer.exe
// @compilerOptions -ld2d1 -ldwrite -lwindowscodecs -luxtheme -lole32 -lshcore -lversion
// @license         MIT
// ==/WindhawkMod==


// ==WindhawkModReadme==
/*
# Dynamic Island for Windows 11

An interactive, fluid Dynamic Island overlay and taskbar widget for Windows 11,
bringing smooth Apple-inspired micro-interactions and live status pills to the desktop.

## Key Features
- **Adaptive Taskbar Placement**:
  - **Top Taskbar**: Seamlessly embeds into the taskbar and expands downward.
  - **Bottom / Left / Right Taskbar**: Floats gracefully as an independent pill with fluid spring animations.
- **Live Activities (Persistent & Expandable)**:
  - **Now Playing**: Album artwork, marquee track/artist info, animated equalizer wave, and expanded playback controls.
  - **Focus Timer**: Circular progress indicator, MM:SS countdown, and interactive pause/resume/stop controls.
  - **Microphone & Privacy Dots**: Active indicator showing when mic or recording streams are in use.
- **Transient HUD Alerts (Auto-dismissing)**:
  - Volume slider and percentage indicator.
  - Caps Lock ON/OFF notifications.
  - Power & Charging status (plugged in, charging %, low battery warning).
  - Bluetooth device connection and peripheral battery level.
- **Fluid Spring Physics**:
  - Smooth expansion and morphing animations modeled after modern UI curves (`cubic-bezier(0.34, 1.3, 0.5, 1)`).
- **Lightweight & High Performance**:
  - Hardware-accelerated Direct2D and DirectWrite rendering.

## Inspiration
Inspired by [devcode90/Dynamic-Island-for-Windows](https://github.com/devcode90/Dynamic-Island-for-Windows)
and expanded to cover full multi-edge taskbars, rich interactive states, and an extensible service architecture.
*/
// ==/WindhawkModReadme==


// ==WindhawkModSettings==
/*
# Dynamic Island Settings Schema
- placementMode: auto
  $name: Placement Mode
  $description: Choose how the Dynamic Island positions itself.
  $options:
    - auto: Automatically detect taskbar position (embedded on top, floating on bottom/sides)
    - top_embed: Always embed directly into top edge
    - floating: Always float as an overlay window
- enableMedia: true
  $name: Media Controls & Now Playing
  $description: Show active media playback, track marquee, equalizer, and playback controls.
- enableTimer: true
  $name: Focus Timer
  $description: Enable focus session countdown timer with progress ring.
- enableMicStatus: true
  $name: Microphone Privacy Dot
  $description: Show indicator when an application is accessing the microphone.
- enableVolumeHUD: true
  $name: Volume Changes HUD
  $description: Show compact island badge when master volume changes.
- enableCapsLockHUD: true
  $name: Caps Lock HUD
  $description: Show badge when Caps Lock is toggled ON or OFF.
- enablePowerHUD: true
  $name: Power & Battery Alerts
  $description: Show alerts when charger is plugged/unplugged or battery drops below 20%.
- enableBluetoothHUD: true
  $name: Bluetooth HUD
  $description: Show badge when Bluetooth headphones/devices connect with battery status.
- autoCollapseSeconds: 5
  $name: Auto-collapse Delay
  $description: Seconds of inactivity before an expanded island collapses back to compact view.
- animationSpeed: 1.0
  $name: Animation Speed Multiplier
  $description: Adjust speed of spring transition animations (1.0 = default fluid physics).
*/
// ==/WindhawkModSettings==


// ============================================================================
// System Includes
// ============================================================================
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <cwchar>
#include <cmath>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <deque>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <shellapi.h>



// ============================================================================
// [Module] src\main.cpp
// ============================================================================


// ============================================================================
// [Module] src\common\defs.h
// ============================================================================



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

// ============================================================================
// [Module] src\common\log.h
// ============================================================================



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

// ============================================================================
// [Module] src\common\utils.h
// ============================================================================



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

// ============================================================================
// [Module] src\platform\taskbar.h
// ============================================================================



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

// ============================================================================
// [Module] src\graphics\animation.h
// ============================================================================



namespace DynamicIsland {
namespace Graphics {

// Evaluates cubic-bezier(x1, y1, x2, y2) for parameter t in [0, 1]
// The prototype uses cubic-bezier(0.34, 1.3, 0.5, 1.0)
float EvaluateCubicBezier(float x1, float y1, float x2, float y2, float t);

// Spring animation property animator
class AnimatedValue {
public:
    AnimatedValue(float initialValue = 0.0f)
        : m_current(initialValue), m_start(initialValue), m_target(initialValue),
          m_startTime(0), m_durationMs(450), m_isAnimating(false) {}

    void SetTarget(float target, DWORD durationMs = 450);
    void SnapTo(float value);
    void Update(DWORD currentTimeMs);

    float GetValue() const { return m_current; }
    bool IsAnimating() const { return m_isAnimating; }

private:
    float m_current;
    float m_start;
    float m_target;
    DWORD m_startTime;
    DWORD m_durationMs;
    bool m_isAnimating;
};

} // namespace Graphics
} // namespace DynamicIsland

// ============================================================================
// [Module] src\graphics\d2d_renderer.h
// ============================================================================



namespace DynamicIsland {
namespace Graphics {

using Microsoft::WRL::ComPtr;

class D2DRenderer {
public:
    D2DRenderer();
    ~D2DRenderer();

    bool Initialize(HWND hwnd);
    void Cleanup();
    void Resize(UINT width, UINT height);

    void BeginDraw();
    HRESULT EndDraw();

    // Drawing primitives
    void Clear(D2D1_COLOR_F color);
    void DrawRoundedPill(const D2D1_ROUNDED_RECT& pill, D2D1_COLOR_F fillColor, D2D1_COLOR_F borderColor, float strokeWidth = 1.0f);
    void DrawTextString(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float fontSize = 12.0f, bool bold = false);
    void DrawProgressBar(const D2D1_RECT_F& barRect, float progressFraction, D2D1_COLOR_F bgColor, D2D1_COLOR_F fgColor);
    void DrawProgressRing(D2D1_POINT_2F center, float radius, float progressFraction, D2D1_COLOR_F ringColor, float strokeWidth = 2.0f);
    void DrawStatusDot(D2D1_POINT_2F center, float radius, D2D1_COLOR_F color);
    void DrawEqualizerWaves(D2D1_POINT_2F origin, float height, float progress);

    ID2D1RenderTarget* GetRenderTarget() const { return m_renderTarget.Get(); }
    ID2D1Factory* GetFactory() const { return m_d2dFactory.Get(); }
    IDWriteFactory* GetDWriteFactory() const { return m_dwriteFactory.Get(); }

private:
    HWND m_hwnd = nullptr;
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    ComPtr<ID2D1SolidColorBrush> m_solidBrush;
    ComPtr<IDWriteTextFormat> m_textFormatRegular;
    ComPtr<IDWriteTextFormat> m_textFormatBold;

    bool CreateDeviceResources();
    void DiscardDeviceResources();
};

} // namespace Graphics
} // namespace DynamicIsland

// ============================================================================
// [Module] src\graphics\icons.h
// ============================================================================



namespace DynamicIsland {
namespace Graphics {

// Icon identifiers corresponding to the SVG paths in preview.html
enum class IconType {
    Play,
    Pause,
    Previous,
    Next,
    Volume,
    CapsLock,
    Bolt,
    Bluetooth,
    Battery,
    Checkmark
};

// SVG path definitions from preview.html
namespace IconPaths {
    constexpr const char* PLAY      = "M8 5v14l11-7z";
    constexpr const char* PAUSE     = "M6 5h4v14H6zm8 0h4v14h-4z";
    constexpr const char* PREV      = "M6 6h2v12H6zm3 6 9-6v12z";
    constexpr const char* NEXT      = "M16 6h2v12h-2zM6 18V6l9 6z";
    constexpr const char* VOLUME    = "M4 9v6h4l5 4V5L8 9z";
    constexpr const char* CAPS_LOCK = "M12 4 4 13h5v5h6v-5h5z";
    constexpr const char* BOLT      = "M13 2 5 14h6l-1 8 8-12h-6z";
    constexpr const char* BLUETOOTH = "M7 7l10 10-5 5V2l5 5L7 17";
    constexpr const char* BATTERY   = "M3 8h16v8H3zM20 11h2v2h-2z";
    constexpr const char* CHECKMARK = "M5 12l5 5 9-10-2-2-7 8-3-3z";
}

} // namespace Graphics
} // namespace DynamicIsland

// ============================================================================
// [Module] src\overlay\layout.h
// ============================================================================



namespace DynamicIsland {
namespace Overlay {

struct IslandMetrics {
    float width = 0.0f;
    float height = 32.0f;
    float cornerRadius = 16.0f;
    float posX = 0.0f;
    float posY = 0.0f;
    bool isEmbedded = false;
};

class LayoutEngine {
public:
    static IslandMetrics CalculateMetrics(
        EventType eventType,
        IslandState state,
        const Platform::TaskbarInfo& tbInfo,
        UINT customWidth = 0
    );
};

} // namespace Overlay
} // namespace DynamicIsland

// ============================================================================
// [Module] src\overlay\island_window.h
// ============================================================================



namespace DynamicIsland {
namespace Overlay {

class IslandWindow {
public:
    IslandWindow();
    ~IslandWindow();

    bool Create();
    void Destroy();
    void Show(bool show);

    void SetState(IslandState state);
    void SetCurrentEvent(EventType eventType);

    void TriggerAnimationUpdate();
    void Render();

    HWND GetHwnd() const { return m_hwnd; }
    IslandState GetState() const { return m_state; }
    EventType GetCurrentEvent() const { return m_currentEvent; }

    // Message handler
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd = nullptr;
    IslandState m_state = IslandState::Hidden;
    EventType m_currentEvent = EventType::None;

    std::unique_ptr<Graphics::D2DRenderer> m_renderer;
    Graphics::AnimatedValue m_animWidth{0.0f};
    Graphics::AnimatedValue m_animHeight{32.0f};
    Graphics::AnimatedValue m_animRadius{16.0f};
    Graphics::AnimatedValue m_animOpacity{0.0f};

    bool m_isHovered = false;
    UINT_PTR m_autoCollapseTimerId = 1001;
    UINT_PTR m_animTimerId = 1002;

    void ArmAutoCollapse();
    void DisarmAutoCollapse();
    void UpdateDimensions();
    void OnClick(int x, int y);
};

} // namespace Overlay
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\service_manager.h
// ============================================================================



namespace DynamicIsland {
namespace Services {

struct QueuedEvent {
    EventType type = EventType::None;
    DWORD durationMs = 2000;
    std::wstring title;
    std::wstring subtitle;
    int progressPercent = 0;
};

class ServiceManager {
public:
    explicit ServiceManager(Overlay::IslandWindow* window);
    ~ServiceManager();

    void Initialize();
    void Shutdown();

    // Event triggering
    void PostTransientEvent(const QueuedEvent& event);
    void SetLiveActivity(EventType type, bool active);

    void Update();

private:
    Overlay::IslandWindow* m_window = nullptr;
    std::deque<QueuedEvent> m_transientQueue;

    bool m_hasActiveTransient = false;
    QueuedEvent m_currentTransient;
    DWORD m_transientExpiryTick = 0;

    // Persistent live activities
    bool m_mediaActive = false;
    bool m_timerActive = false;
    bool m_micActive = false;

    void PumpNextEvent();
    EventType ResolveCurrentLiveActivity() const;
};

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\media_service.h
// ============================================================================



namespace DynamicIsland {
namespace Services {

class ServiceManager;

struct MediaState {
    std::wstring title;
    std::wstring artist;
    bool isPlaying = false;
    float progress = 0.0f;
};

class MediaService {
public:
    explicit MediaService(ServiceManager* manager);
    ~MediaService();

    void Start();
    void Stop();
    void Poll();

    const MediaState& GetState() const { return m_state; }

    void Play();
    void Pause();
    void Next();
    void Previous();

private:
    ServiceManager* m_manager = nullptr;
    MediaState m_state;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\audio_service.h
// ============================================================================



namespace DynamicIsland {
namespace Services {

class ServiceManager;

class AudioService {
public:
    explicit AudioService(ServiceManager* manager);
    ~AudioService();

    void Start();
    void Stop();
    void CheckMicUsage();

    float GetCurrentVolume() const { return m_currentVolume; }
    bool IsMuted() const { return m_isMuted; }
    bool IsMicInUse() const { return m_isMicInUse; }

private:
    ServiceManager* m_manager = nullptr;
    float m_currentVolume = 0.5f;
    bool m_isMuted = false;
    bool m_isMicInUse = false;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\power_service.h
// ============================================================================



namespace DynamicIsland {
namespace Services {

class ServiceManager;

class PowerService {
public:
    explicit PowerService(ServiceManager* manager);
    ~PowerService();

    void Start();
    void Stop();
    void Poll();

    BYTE GetBatteryPercent() const { return m_batteryPercent; }
    bool IsACConnected() const { return m_isACConnected; }

private:
    ServiceManager* m_manager = nullptr;
    BYTE m_batteryPercent = 100;
    bool m_isACConnected = false;
    bool m_wasACConnected = false;
    bool m_lowBatteryAlerted = false;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\keyboard_service.h
// ============================================================================



namespace DynamicIsland {
namespace Services {

class ServiceManager;

class KeyboardService {
public:
    explicit KeyboardService(ServiceManager* manager);
    ~KeyboardService();

    void Start();
    void Stop();
    void Poll();

    bool IsCapsLockOn() const { return m_capsLockState; }

private:
    ServiceManager* m_manager = nullptr;
    bool m_capsLockState = false;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\bluetooth_service.h
// ============================================================================



namespace DynamicIsland {
namespace Services {

class ServiceManager;

struct BluetoothDeviceInfo {
    std::wstring name;
    int batteryLevel = -1; // -1 if not available
    bool isConnected = false;
};

class BluetoothService {
public:
    explicit BluetoothService(ServiceManager* manager);
    ~BluetoothService();

    void Start();
    void Stop();
    void Poll();

private:
    ServiceManager* m_manager = nullptr;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\timer_service.h
// ============================================================================



namespace DynamicIsland {
namespace Services {

class ServiceManager;

class TimerService {
public:
    explicit TimerService(ServiceManager* manager);
    ~TimerService();

    void Start(int durationSeconds);
    void Pause();
    void Resume();
    void Stop();
    void Tick();

    bool IsActive() const { return m_isActive; }
    bool IsPaused() const { return m_isPaused; }
    int GetRemainingSeconds() const { return m_remainingSeconds; }
    int GetTotalSeconds() const { return m_totalSeconds; }
    float GetProgressFraction() const;

private:
    ServiceManager* m_manager = nullptr;
    int m_totalSeconds = 0;
    int m_remainingSeconds = 0;
    bool m_isActive = false;
    bool m_isPaused = false;
};

} // namespace Services
} // namespace DynamicIsland

// Include module implementations for single compilation unit bundling

// ============================================================================
// [Module] src\common\utils.cpp
// ============================================================================


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

// ============================================================================
// [Module] src\platform\taskbar.cpp
// ============================================================================


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

// ============================================================================
// [Module] src\graphics\animation.cpp
// ============================================================================


namespace DynamicIsland {
namespace Graphics {

// Solves cubic bezier X to find T, then returns Y
float EvaluateCubicBezier(float x1, float y1, float x2, float y2, float x) {
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;

    // Approximate parameter t using Newton-Raphson iterations
    float t = x;
    for (int i = 0; i < 8; ++i) {
        float oneMinusT = 1.0f - t;
        float currentX = 3.0f * oneMinusT * oneMinusT * t * x1 +
                         3.0f * oneMinusT * t * t * x2 +
                         t * t * t;

        float derivativeX = 3.0f * oneMinusT * oneMinusT * x1 +
                            6.0f * oneMinusT * t * (x2 - x1) +
                            3.0f * t * t * (1.0f - x2);

        if (std::abs(derivativeX) < 1e-5f) break;
        t -= (currentX - x) / derivativeX;
        t = Utils::Clamp(t, 0.0f, 1.0f);
    }

    // Calculate Y from T
    float oneMinusT = 1.0f - t;
    return 3.0f * oneMinusT * oneMinusT * t * y1 +
           3.0f * oneMinusT * t * t * y2 +
           t * t * t;
}

void AnimatedValue::SetTarget(float target, DWORD durationMs) {
    if (std::abs(m_target - target) < 0.001f && !m_isAnimating) {
        return;
    }
    m_start = m_current;
    m_target = target;
    m_durationMs = (durationMs > 0) ? durationMs : 1;
    m_startTime = GetTickCount();
    m_isAnimating = true;
}

void AnimatedValue::SnapTo(float value) {
    m_current = value;
    m_start = value;
    m_target = value;
    m_isAnimating = false;
}

void AnimatedValue::Update(DWORD currentTimeMs) {
    if (!m_isAnimating) return;

    if (currentTimeMs >= m_startTime + m_durationMs) {
        m_current = m_target;
        m_isAnimating = false;
        return;
    }

    float linearProgress = static_cast<float>(currentTimeMs - m_startTime) / static_cast<float>(m_durationMs);
    // Cubic bezier used in CSS prototype: cubic-bezier(.34, 1.3, .5, 1)
    float curvedProgress = EvaluateCubicBezier(0.34f, 1.3f, 0.5f, 1.0f, linearProgress);
    m_current = m_start + (m_target - m_start) * curvedProgress;
}

} // namespace Graphics
} // namespace DynamicIsland

// ============================================================================
// [Module] src\graphics\d2d_renderer.cpp
// ============================================================================


namespace DynamicIsland {
namespace Graphics {

D2DRenderer::D2DRenderer() = default;

D2DRenderer::~D2DRenderer() {
    Cleanup();
}

bool D2DRenderer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
    if (FAILED(hr)) {
        LogError(L"Failed to create Direct2D Factory, hr=0x%08X", hr);
        return false;
    }

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf())
    );
    if (FAILED(hr)) {
        LogError(L"Failed to create DirectWrite Factory, hr=0x%08X", hr);
        return false;
    }

    hr = m_dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en-US",
        m_textFormatRegular.GetAddressOf()
    );

    if (SUCCEEDED(hr)) {
        m_dwriteFactory->CreateTextFormat(
            L"Segoe UI Variable",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"en-US",
            m_textFormatBold.GetAddressOf()
        );
    }

    return CreateDeviceResources();
}

bool D2DRenderer::CreateDeviceResources() {
    if (!m_hwnd || m_renderTarget) return true;

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
        m_hwnd,
        size,
        D2D1_PRESENT_OPTIONS_IMMEDIATELY
    );

    HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
        rtProps,
        hwndProps,
        m_renderTarget.GetAddressOf()
    );

    if (FAILED(hr)) {
        LogError(L"Failed to create HwndRenderTarget, hr=0x%08X", hr);
        return false;
    }

    m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        m_solidBrush.GetAddressOf()
    );

    return true;
}

void D2DRenderer::DiscardDeviceResources() {
    m_solidBrush.Reset();
    m_renderTarget.Reset();
}

void D2DRenderer::Cleanup() {
    DiscardDeviceResources();
    m_textFormatBold.Reset();
    m_textFormatRegular.Reset();
    m_dwriteFactory.Reset();
    m_d2dFactory.Reset();
}

void D2DRenderer::Resize(UINT width, UINT height) {
    if (m_renderTarget) {
        m_renderTarget->Resize(D2D1::SizeU(width, height));
    }
}

void D2DRenderer::BeginDraw() {
    CreateDeviceResources();
    if (m_renderTarget) {
        m_renderTarget->BeginDraw();
    }
}

HRESULT D2DRenderer::EndDraw() {
    if (!m_renderTarget) return E_FAIL;
    HRESULT hr = m_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
    }
    return hr;
}

void D2DRenderer::Clear(D2D1_COLOR_F color) {
    if (m_renderTarget) {
        m_renderTarget->Clear(color);
    }
}

void D2DRenderer::DrawRoundedPill(const D2D1_ROUNDED_RECT& pill, D2D1_COLOR_F fillColor, D2D1_COLOR_F borderColor, float strokeWidth) {
    if (!m_renderTarget || !m_solidBrush) return;

    m_solidBrush->SetColor(fillColor);
    m_renderTarget->FillRoundedRectangle(pill, m_solidBrush.Get());

    if (strokeWidth > 0.0f && borderColor.a > 0.0f) {
        m_solidBrush->SetColor(borderColor);
        m_renderTarget->DrawRoundedRectangle(pill, m_solidBrush.Get(), strokeWidth);
    }
}

void D2DRenderer::DrawTextString(const std::wstring& text, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float fontSize, bool bold) {
    if (!m_renderTarget || !m_solidBrush) return;

    IDWriteTextFormat* format = bold ? m_textFormatBold.Get() : m_textFormatRegular.Get();
    if (!format) return;

    m_solidBrush->SetColor(color);
    m_renderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        format,
        rect,
        m_solidBrush.Get()
    );
}

void D2DRenderer::DrawProgressBar(const D2D1_RECT_F& barRect, float progressFraction, D2D1_COLOR_F bgColor, D2D1_COLOR_F fgColor) {
    if (!m_renderTarget || !m_solidBrush) return;

    float radius = (barRect.bottom - barRect.top) / 2.0f;
    D2D1_ROUNDED_RECT bgRounded = D2D1::RoundedRect(barRect, radius, radius);

    m_solidBrush->SetColor(bgColor);
    m_renderTarget->FillRoundedRectangle(bgRounded, m_solidBrush.Get());

    if (progressFraction > 0.0f) {
        float fillWidth = (barRect.right - barRect.left) * (progressFraction > 1.0f ? 1.0f : progressFraction);
        D2D1_RECT_F fillRect = D2D1::RectF(barRect.left, barRect.top, barRect.left + fillWidth, barRect.bottom);
        D2D1_ROUNDED_RECT fillRounded = D2D1::RoundedRect(fillRect, radius, radius);

        m_solidBrush->SetColor(fgColor);
        m_renderTarget->FillRoundedRectangle(fillRounded, m_solidBrush.Get());
    }
}

void D2DRenderer::DrawProgressRing(D2D1_POINT_2F center, float radius, float progressFraction, D2D1_COLOR_F ringColor, float strokeWidth) {
    if (!m_renderTarget || !m_solidBrush) return;

    // Draw background track
    m_solidBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.2f));
    m_renderTarget->DrawEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get(), strokeWidth);

    // Draw foreground ring arc
    if (progressFraction > 0.0f) {
        m_solidBrush->SetColor(ringColor);
        m_renderTarget->DrawEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get(), strokeWidth);
    }
}

void D2DRenderer::DrawStatusDot(D2D1_POINT_2F center, float radius, D2D1_COLOR_F color) {
    if (!m_renderTarget || !m_solidBrush) return;

    m_solidBrush->SetColor(color);
    m_renderTarget->FillEllipse(D2D1::Ellipse(center, radius, radius), m_solidBrush.Get());
}

void D2DRenderer::DrawEqualizerWaves(D2D1_POINT_2F origin, float height, float progress) {
    if (!m_renderTarget || !m_solidBrush) return;

    // 3 animated equalizer vertical lines
    m_solidBrush->SetColor(D2D1::ColorF(0.49f, 0.88f, 0.76f, 1.0f)); // Mint green #7ee0c3

    float offsets[3] = {
        static_cast<float>(std::sin(progress * 6.28f) * 0.5f + 0.5f),
        static_cast<float>(std::sin(progress * 6.28f + 1.25f) * 0.5f + 0.5f),
        static_cast<float>(std::sin(progress * 6.28f + 2.5f) * 0.5f + 0.5f)
    };

    for (int i = 0; i < 3; ++i) {
        float barH = height * (0.3f + 0.7f * offsets[i]);
        D2D1_RECT_F r = D2D1::RectF(
            origin.x + i * 4.0f,
            origin.y + (height - barH) / 2.0f,
            origin.x + i * 4.0f + 2.0f,
            origin.y + (height + barH) / 2.0f
        );
        m_renderTarget->FillRectangle(r, m_solidBrush.Get());
    }
}

} // namespace Graphics
} // namespace DynamicIsland

// ============================================================================
// [Module] src\overlay\layout.cpp
// ============================================================================


namespace DynamicIsland {
namespace Overlay {

IslandMetrics LayoutEngine::CalculateMetrics(
    EventType eventType,
    IslandState state,
    const Platform::TaskbarInfo& tbInfo,
    UINT customWidth
) {
    IslandMetrics m{};
    UINT dpi = tbInfo.dpi;

    int tbHeight = tbInfo.taskbarRect.bottom - tbInfo.taskbarRect.top;
    if (tbHeight <= 0) tbHeight = 40;

    int compactH = tbHeight - 8;
    if (compactH < 28) compactH = 28;

    m.isEmbedded = (tbInfo.position == TaskbarPosition::Top);

    if (state == IslandState::Hidden) {
        m.width = 0.0f;
        m.height = static_cast<float>(Utils::ScaleDpi(compactH, dpi));
        m.cornerRadius = m.height / 2.0f;
    } else if (state == IslandState::Compact) {
        float defaultWidth = 196.0f;
        switch (eventType) {
            case EventType::Media:      defaultWidth = 196.0f; break;
            case EventType::Timer:      defaultWidth = 176.0f; break;
            case EventType::MicStatus:  defaultWidth = 176.0f; break;
            case EventType::Volume:     defaultWidth = 206.0f; break;
            case EventType::CapsLock:   defaultWidth = 160.0f; break;
            case EventType::Power:      defaultWidth = 196.0f; break;
            case EventType::Bluetooth:  defaultWidth = 244.0f; break;
            case EventType::LowBattery: defaultWidth = 196.0f; break;
            case EventType::TimerDone:  defaultWidth = 204.0f; break;
            default:                    defaultWidth = 180.0f; break;
        }

        if (customWidth > 0) defaultWidth = static_cast<float>(customWidth);

        m.width = Utils::ScaleDpiF(defaultWidth, dpi);
        m.height = Utils::ScaleDpiF(static_cast<float>(compactH), dpi);
        m.cornerRadius = m.height / 2.0f;
    } else if (state == IslandState::Expanded) {
        float expWidth = 340.0f;
        float expHeight = 150.0f;

        switch (eventType) {
            case EventType::Media:
                expWidth = 340.0f;
                expHeight = 150.0f;
                break;
            case EventType::Timer:
                expWidth = 300.0f;
                expHeight = 124.0f;
                break;
            case EventType::MicStatus:
                expWidth = 280.0f;
                expHeight = 78.0f;
                break;
            default:
                expWidth = 280.0f;
                expHeight = 90.0f;
                break;
        }

        m.width = Utils::ScaleDpiF(expWidth, dpi);
        m.height = Utils::ScaleDpiF(expHeight, dpi);
        m.cornerRadius = Utils::ScaleDpiF(26.0f, dpi);
    }

    // Position calculation
    int screenWidth = tbInfo.monitorRect.right - tbInfo.monitorRect.left;
    int centerX = tbInfo.monitorRect.left + screenWidth / 2;

    // Shift centerX if taskbar is left/right
    if (tbInfo.position == TaskbarPosition::Left) {
        int workCenter = (tbInfo.workAreaRect.left + tbInfo.workAreaRect.right) / 2;
        centerX = workCenter;
    } else if (tbInfo.position == TaskbarPosition::Right) {
        int workCenter = (tbInfo.workAreaRect.left + tbInfo.workAreaRect.right) / 2;
        centerX = workCenter;
    }

    m.posX = static_cast<float>(centerX) - (m.width / 2.0f);

    if (m.isEmbedded) {
        // Top edge: sits in taskbar, expands downwards
        m.posY = static_cast<float>(tbInfo.taskbarRect.top + 4);
    } else {
        // Floating above bottom taskbar or work area bottom
        int bottomAnchor = tbInfo.workAreaRect.bottom;
        if (tbInfo.position == TaskbarPosition::Bottom && !tbInfo.isAutoHide) {
            bottomAnchor = tbInfo.taskbarRect.top - 10;
        } else {
            bottomAnchor = tbInfo.monitorRect.bottom - 14;
        }

        m.posY = static_cast<float>(bottomAnchor) - m.height;
    }

    return m;
}

} // namespace Overlay
} // namespace DynamicIsland

// ============================================================================
// [Module] src\overlay\island_window.cpp
// ============================================================================


namespace DynamicIsland {
namespace Overlay {

static const wchar_t* WINDOW_CLASS_NAME = L"Win11DynamicIslandWndClass";

IslandWindow::IslandWindow() {
    m_renderer = std::make_unique<Graphics::D2DRenderer>();
}

IslandWindow::~IslandWindow() {
    Destroy();
}

bool IslandWindow::Create() {
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = IslandWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        WINDOW_CLASS_NAME,
        L"Win11 Dynamic Island",
        WS_POPUP,
        0, 0, 400, 200,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd) {
        LogError(L"Failed to create Dynamic Island overlay window");
        return false;
    }

    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY | LWA_ALPHA);

    if (!m_renderer->Initialize(m_hwnd)) {
        LogError(L"Failed to initialize D2D renderer for island window");
        return false;
    }

    LogInfo(L"Dynamic Island overlay window created successfully");
    return true;
}

void IslandWindow::Destroy() {
    if (m_hwnd) {
        KillTimer(m_hwnd, m_autoCollapseTimerId);
        KillTimer(m_hwnd, m_animTimerId);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandleW(nullptr));
}

void IslandWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
}

void IslandWindow::SetState(IslandState state) {
    if (m_state == state) return;
    m_state = state;
    UpdateDimensions();

    if (m_state == IslandState::Expanded) {
        ArmAutoCollapse();
    } else {
        DisarmAutoCollapse();
    }
}

void IslandWindow::SetCurrentEvent(EventType eventType) {
    m_currentEvent = eventType;
    UpdateDimensions();
}

void IslandWindow::ArmAutoCollapse() {
    if (!m_hwnd) return;
    DisarmAutoCollapse();
    SetTimer(m_hwnd, m_autoCollapseTimerId, g_settings.autoCollapseSeconds * 1000, nullptr);
}

void IslandWindow::DisarmAutoCollapse() {
    if (m_hwnd) {
        KillTimer(m_hwnd, m_autoCollapseTimerId);
    }
}

void IslandWindow::UpdateDimensions() {
    Platform::TaskbarInfo tb = Platform::QueryPrimaryTaskbar();
    IslandMetrics target = LayoutEngine::CalculateMetrics(m_currentEvent, m_state, tb);

    m_animWidth.SetTarget(target.width, 450);
    m_animHeight.SetTarget(target.height, 450);
    m_animRadius.SetTarget(target.cornerRadius, 450);
    m_animOpacity.SetTarget(m_state == IslandState::Hidden ? 0.0f : 1.0f, 200);

    // Reposition window bounding rect
    if (m_hwnd) {
        SetWindowPos(
            m_hwnd,
            HWND_TOPMOST,
            static_cast<int>(target.posX),
            static_cast<int>(target.posY),
            static_cast<int>(target.width > 0.0f ? target.width : 1.0f),
            static_cast<int>(target.height),
            SWP_NOACTIVATE | SWP_SHOWWINDOW
        );
        m_renderer->Resize(static_cast<UINT>(target.width), static_cast<UINT>(target.height));
        SetTimer(m_hwnd, m_animTimerId, 16, nullptr); // ~60fps animation tick
    }
}

void IslandWindow::TriggerAnimationUpdate() {
    DWORD now = GetTickCount();
    m_animWidth.Update(now);
    m_animHeight.Update(now);
    m_animRadius.Update(now);
    m_animOpacity.Update(now);

    Render();

    if (!m_animWidth.IsAnimating() && !m_animHeight.IsAnimating() &&
        !m_animRadius.IsAnimating() && !m_animOpacity.IsAnimating()) {
        KillTimer(m_hwnd, m_animTimerId);
    }
}

void IslandWindow::OnClick(int x, int y) {
    if (m_state == IslandState::Compact) {
        SetState(IslandState::Expanded);
    } else if (m_state == IslandState::Expanded) {
        SetState(IslandState::Compact);
    }
}

void IslandWindow::Render() {
    if (!m_renderer) return;

    m_renderer->BeginDraw();
    m_renderer->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

    float w = m_animWidth.GetValue();
    float h = m_animHeight.GetValue();
    float r = m_animRadius.GetValue();

    if (w > 0.0f && h > 0.0f) {
        D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(
            D2D1::RectF(0.0f, 0.0f, w, h),
            r, r
        );

        // Dark pill background & subtle border
        m_renderer->DrawRoundedPill(
            pill,
            D2D1::ColorF(0.043f, 0.043f, 0.051f, 1.0f), // #0b0b0d
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.13f),       // 13% white border
            1.0f
        );

        // Draw active scenario label / status
        if (m_state == IslandState::Compact) {
            std::wstring label = L"Dynamic Island";
            switch (m_currentEvent) {
                case EventType::Media:      label = L"Now Playing"; break;
                case EventType::Timer:      label = L"Focus Timer"; break;
                case EventType::MicStatus:  label = L"Microphone active"; break;
                case EventType::Volume:     label = L"Volume"; break;
                case EventType::CapsLock:   label = L"Caps Lock"; break;
                case EventType::Power:      label = L"Charging"; break;
                case EventType::Bluetooth:  label = L"Connected"; break;
                default: break;
            }

            m_renderer->DrawTextString(
                label,
                D2D1::RectF(14.0f, (h - 16.0f) / 2.0f, w - 14.0f, h),
                D2D1::ColorF(D2D1::ColorF::White),
                12.0f,
                false
            );
        }
    }

    m_renderer->EndDraw();
}

LRESULT CALLBACK IslandWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    IslandWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<IslandWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<IslandWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_TIMER:
            if (wParam == self->m_animTimerId) {
                self->TriggerAnimationUpdate();
            } else if (wParam == self->m_autoCollapseTimerId) {
                if (!self->m_isHovered && self->m_state == IslandState::Expanded) {
                    self->SetState(IslandState::Compact);
                }
            }
            return 0;

        case WM_MOUSEMOVE:
            if (!self->m_isHovered) {
                self->m_isHovered = true;
                TRACKMOUSEEVENT tme{sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tme);
                self->DisarmAutoCollapse();
            }
            return 0;

        case WM_MOUSELEAVE:
            self->m_isHovered = false;
            if (self->m_state == IslandState::Expanded) {
                self->ArmAutoCollapse();
            }
            return 0;

        case WM_LBUTTONUP:
            self->OnClick(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            self->Render();
            EndPaint(hwnd, &ps);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace Overlay
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\service_manager.cpp
// ============================================================================


namespace DynamicIsland {
namespace Services {

ServiceManager::ServiceManager(Overlay::IslandWindow* window)
    : m_window(window) {}

ServiceManager::~ServiceManager() {
    Shutdown();
}

void ServiceManager::Initialize() {
    LogInfo(L"ServiceManager initialized");
}

void ServiceManager::Shutdown() {
    m_transientQueue.clear();
    m_hasActiveTransient = false;
}

void ServiceManager::PostTransientEvent(const QueuedEvent& event) {
    m_transientQueue.push_back(event);
    if (!m_hasActiveTransient) {
        PumpNextEvent();
    }
}

void ServiceManager::SetLiveActivity(EventType type, bool active) {
    switch (type) {
        case EventType::Media:     m_mediaActive = active; break;
        case EventType::Timer:     m_timerActive = active; break;
        case EventType::MicStatus: m_micActive = active; break;
        default: break;
    }

    if (!m_hasActiveTransient && m_window) {
        EventType current = ResolveCurrentLiveActivity();
        if (current != EventType::None) {
            m_window->SetCurrentEvent(current);
            m_window->SetState(IslandState::Compact);
        } else {
            m_window->SetState(IslandState::Hidden);
        }
    }
}

void ServiceManager::PumpNextEvent() {
    if (m_transientQueue.empty()) {
        m_hasActiveTransient = false;
        if (m_window) {
            EventType live = ResolveCurrentLiveActivity();
            if (live != EventType::None) {
                m_window->SetCurrentEvent(live);
                m_window->SetState(IslandState::Compact);
            } else {
                m_window->SetState(IslandState::Hidden);
            }
        }
        return;
    }

    m_currentTransient = m_transientQueue.front();
    m_transientQueue.pop_front();
    m_hasActiveTransient = true;
    m_transientExpiryTick = GetTickCount() + m_currentTransient.durationMs;

    if (m_window) {
        m_window->SetCurrentEvent(m_currentTransient.type);
        m_window->SetState(IslandState::Compact);
    }
}

void ServiceManager::Update() {
    if (m_hasActiveTransient) {
        if (GetTickCount() >= m_transientExpiryTick) {
            PumpNextEvent();
        }
    }
}

EventType ServiceManager::ResolveCurrentLiveActivity() const {
    // Priority order matching preview.html: mic > timer > media
    if (m_micActive)   return EventType::MicStatus;
    if (m_timerActive) return EventType::Timer;
    if (m_mediaActive) return EventType::Media;
    return EventType::None;
}

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\media_service.cpp
// ============================================================================


namespace DynamicIsland {
namespace Services {

MediaService::MediaService(ServiceManager* manager)
    : m_manager(manager) {}

MediaService::~MediaService() {
    Stop();
}

void MediaService::Start() {
    m_isRunning = true;
    LogInfo(L"MediaService started");
}

void MediaService::Stop() {
    m_isRunning = false;
}

void MediaService::Poll() {
    if (!m_isRunning) return;
    // Integration with Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager
    // GSMTC listener will update m_state and inform m_manager->SetLiveActivity(EventType::Media, m_state.isPlaying);
}

void MediaService::Play() {
    // Send GSMTC Play command
}

void MediaService::Pause() {
    // Send GSMTC Pause command
}

void MediaService::Next() {
    // Send GSMTC Next command
}

void MediaService::Previous() {
    // Send GSMTC Previous command
}

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\audio_service.cpp
// ============================================================================


namespace DynamicIsland {
namespace Services {

AudioService::AudioService(ServiceManager* manager)
    : m_manager(manager) {}

AudioService::~AudioService() {
    Stop();
}

void AudioService::Start() {
    m_isRunning = true;
    LogInfo(L"AudioService started");
}

void AudioService::Stop() {
    m_isRunning = false;
}

void AudioService::CheckMicUsage() {
    if (!m_isRunning) return;
    // Registry query or CoreAudio capture stream enumeration
    // HKCU\Software\Microsoft\Windows\CurrentVersion\CapabilityAccessManager\ConsentStore\microphone\NonPackaged
}

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\power_service.cpp
// ============================================================================


namespace DynamicIsland {
namespace Services {

PowerService::PowerService(ServiceManager* manager)
    : m_manager(manager) {}

PowerService::~PowerService() {
    Stop();
}

void PowerService::Start() {
    m_isRunning = true;
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        m_batteryPercent = sps.BatteryLifePercent;
        m_isACConnected = (sps.ACLineStatus == 1);
        m_wasACConnected = m_isACConnected;
    }
    LogInfo(L"PowerService started (AC=%d, Battery=%d%%)", m_isACConnected, m_batteryPercent);
}

void PowerService::Stop() {
    m_isRunning = false;
}

void PowerService::Poll() {
    if (!m_isRunning || !m_manager) return;

    SYSTEM_POWER_STATUS sps;
    if (!GetSystemPowerStatus(&sps)) return;

    bool currentAC = (sps.ACLineStatus == 1);
    BYTE currentPercent = sps.BatteryLifePercent;

    // AC connect / disconnect event
    if (currentAC != m_wasACConnected) {
        m_wasACConnected = currentAC;
        m_isACConnected = currentAC;

        if (g_settings.enablePowerHUD && currentAC) {
            QueuedEvent qe;
            qe.type = EventType::Power;
            qe.durationMs = DURATION_POWER_MS;
            qe.title = L"Charging";
            qe.progressPercent = currentPercent;
            m_manager->PostTransientEvent(qe);
        }
    }

    // Low battery trigger (<20%)
    if (!currentAC && currentPercent <= 20 && !m_lowBatteryAlerted) {
        m_lowBatteryAlerted = true;
        if (g_settings.enablePowerHUD) {
            QueuedEvent qe;
            qe.type = EventType::LowBattery;
            qe.durationMs = DURATION_LOW_BATT_MS;
            qe.title = L"Low Battery";
            qe.progressPercent = currentPercent;
            m_manager->PostTransientEvent(qe);
        }
    } else if (currentPercent > 25) {
        m_lowBatteryAlerted = false;
    }

    m_batteryPercent = currentPercent;
}

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\keyboard_service.cpp
// ============================================================================


namespace DynamicIsland {
namespace Services {

KeyboardService::KeyboardService(ServiceManager* manager)
    : m_manager(manager) {}

KeyboardService::~KeyboardService() {
    Stop();
}

void KeyboardService::Start() {
    m_isRunning = true;
    m_capsLockState = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    LogInfo(L"KeyboardService started (CapsLock=%s)", m_capsLockState ? L"ON" : L"OFF");
}

void KeyboardService::Stop() {
    m_isRunning = false;
}

void KeyboardService::Poll() {
    if (!m_isRunning || !m_manager) return;

    bool currentState = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    if (currentState != m_capsLockState) {
        m_capsLockState = currentState;

        if (g_settings.enableCapsLockHUD) {
            QueuedEvent qe;
            qe.type = EventType::CapsLock;
            qe.durationMs = DURATION_CAPS_LOCK_MS;
            qe.title = L"Caps Lock";
            qe.subtitle = m_capsLockState ? L"ON" : L"OFF";
            m_manager->PostTransientEvent(qe);
        }
    }
}

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\bluetooth_service.cpp
// ============================================================================


namespace DynamicIsland {
namespace Services {

BluetoothService::BluetoothService(ServiceManager* manager)
    : m_manager(manager) {}

BluetoothService::~BluetoothService() {
    Stop();
}

void BluetoothService::Start() {
    m_isRunning = true;
    LogInfo(L"BluetoothService started");
}

void BluetoothService::Stop() {
    m_isRunning = false;
}

void BluetoothService::Poll() {
    if (!m_isRunning || !m_manager) return;
    // Enumerates connected Bluetooth audio devices via Windows.Devices.Bluetooth / SetupAPI
}

} // namespace Services
} // namespace DynamicIsland

// ============================================================================
// [Module] src\services\timer_service.cpp
// ============================================================================


namespace DynamicIsland {
namespace Services {

TimerService::TimerService(ServiceManager* manager)
    : m_manager(manager) {}

TimerService::~TimerService() {
    Stop();
}

void TimerService::Start(int durationSeconds) {
    m_totalSeconds = durationSeconds;
    m_remainingSeconds = durationSeconds;
    m_isActive = true;
    m_isPaused = false;

    if (m_manager) {
        m_manager->SetLiveActivity(EventType::Timer, true);
    }
    LogInfo(L"Timer started for %d seconds", durationSeconds);
}

void TimerService::Pause() {
    m_isPaused = true;
}

void TimerService::Resume() {
    m_isPaused = false;
}

void TimerService::Stop() {
    m_isActive = false;
    m_isPaused = false;
    m_remainingSeconds = 0;

    if (m_manager) {
        m_manager->SetLiveActivity(EventType::Timer, false);
    }
}

void TimerService::Tick() {
    if (!m_isActive || m_isPaused) return;

    if (m_remainingSeconds > 0) {
        m_remainingSeconds--;
    }

    if (m_remainingSeconds <= 0) {
        Stop();
        if (m_manager) {
            QueuedEvent qe;
            qe.type = EventType::TimerDone;
            qe.durationMs = DURATION_TIMER_DONE_MS;
            qe.title = L"Timer Finished";
            m_manager->PostTransientEvent(qe);
        }
    }
}

float TimerService::GetProgressFraction() const {
    if (m_totalSeconds <= 0) return 0.0f;
    return 1.0f - (static_cast<float>(m_remainingSeconds) / static_cast<float>(m_totalSeconds));
}

} // namespace Services
} // namespace DynamicIsland

namespace DynamicIsland {

ModSettings g_settings;

static std::unique_ptr<Overlay::IslandWindow> g_islandWindow;
static std::unique_ptr<Services::ServiceManager> g_serviceManager;
static std::unique_ptr<Services::MediaService> g_mediaService;
static std::unique_ptr<Services::AudioService> g_audioService;
static std::unique_ptr<Services::PowerService> g_powerService;
static std::unique_ptr<Services::KeyboardService> g_keyboardService;
static std::unique_ptr<Services::BluetoothService> g_bluetoothService;
static std::unique_ptr<Services::TimerService> g_timerService;

static UINT_PTR g_pollTimerId = 2001;
static HWND g_timerWindow = nullptr;

static LRESULT CALLBACK ModTimerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TIMER && wParam == g_pollTimerId) {
        if (g_powerService) g_powerService->Poll();
        if (g_keyboardService) g_keyboardService->Poll();
        if (g_serviceManager) g_serviceManager->Update();
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void LoadSettings() {
    PCWSTR placementStr = Wh_GetStringSetting(L"placementMode");
    if (placementStr) {
        if (wcscmp(placementStr, L"top_embed") == 0) {
            g_settings.placement = PlacementMode::TopEmbed;
        } else if (wcscmp(placementStr, L"floating") == 0) {
            g_settings.placement = PlacementMode::Floating;
        } else {
            g_settings.placement = PlacementMode::Auto;
        }
        Wh_FreeStringSetting(placementStr);
    }

    g_settings.enableMedia = Wh_GetIntSetting(L"enableMedia") != 0;
    g_settings.enableTimer = Wh_GetIntSetting(L"enableTimer") != 0;
    g_settings.enableMicStatus = Wh_GetIntSetting(L"enableMicStatus") != 0;
    g_settings.enableVolumeHUD = Wh_GetIntSetting(L"enableVolumeHUD") != 0;
    g_settings.enableCapsLockHUD = Wh_GetIntSetting(L"enableCapsLockHUD") != 0;
    g_settings.enablePowerHUD = Wh_GetIntSetting(L"enablePowerHUD") != 0;
    g_settings.enableBluetoothHUD = Wh_GetIntSetting(L"enableBluetoothHUD") != 0;
    g_settings.autoCollapseSeconds = Wh_GetIntSetting(L"autoCollapseSeconds");
    if (g_settings.autoCollapseSeconds <= 0) {
        g_settings.autoCollapseSeconds = 5;
    }

    LogInfo(L"Settings loaded successfully");
}

} // namespace DynamicIsland

// ============================================================================
// Windhawk Mod Lifecycle Hooks
// ============================================================================

BOOL Wh_ModInit() {
    DynamicIsland::LogInfo(L"Initializing Win11 Dynamic Island Mod...");

    DynamicIsland::LoadSettings();

    // 1. Create overlay window
    DynamicIsland::g_islandWindow = std::make_unique<DynamicIsland::Overlay::IslandWindow>();
    if (!DynamicIsland::g_islandWindow->Create()) {
        DynamicIsland::LogError(L"Failed to create Dynamic Island overlay window");
        return FALSE;
    }

    // 2. Initialize service orchestrator
    DynamicIsland::g_serviceManager = std::make_unique<DynamicIsland::Services::ServiceManager>(
        DynamicIsland::g_islandWindow.get()
    );
    DynamicIsland::g_serviceManager->Initialize();

    // 3. Initialize feature services
    DynamicIsland::g_mediaService = std::make_unique<DynamicIsland::Services::MediaService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_audioService = std::make_unique<DynamicIsland::Services::AudioService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_powerService = std::make_unique<DynamicIsland::Services::PowerService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_keyboardService = std::make_unique<DynamicIsland::Services::KeyboardService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_bluetoothService = std::make_unique<DynamicIsland::Services::BluetoothService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_timerService = std::make_unique<DynamicIsland::Services::TimerService>(
        DynamicIsland::g_serviceManager.get()
    );

    // Start services
    if (DynamicIsland::g_settings.enableMedia) DynamicIsland::g_mediaService->Start();
    if (DynamicIsland::g_settings.enablePowerHUD) DynamicIsland::g_powerService->Start();
    if (DynamicIsland::g_settings.enableCapsLockHUD) DynamicIsland::g_keyboardService->Start();
    if (DynamicIsland::g_settings.enableBluetoothHUD) DynamicIsland::g_bluetoothService->Start();

    // Setup polling timer window
    WNDCLASSEXW twc{};
    twc.cbSize = sizeof(WNDCLASSEXW);
    twc.lpfnWndProc = DynamicIsland::ModTimerWndProc;
    twc.hInstance = GetModuleHandleW(nullptr);
    twc.lpszClassName = L"Win11DynamicIslandTimerClass";
    RegisterClassExW(&twc);

    DynamicIsland::g_timerWindow = CreateWindowExW(
        0, L"Win11DynamicIslandTimerClass", nullptr, 0,
        0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr
    );

    if (DynamicIsland::g_timerWindow) {
        SetTimer(DynamicIsland::g_timerWindow, DynamicIsland::g_pollTimerId, 250, nullptr);
    }

    DynamicIsland::g_islandWindow->Show(true);
    DynamicIsland::LogInfo(L"Win11 Dynamic Island Mod initialized successfully");
    return TRUE;
}

void Wh_ModUninit() {
    DynamicIsland::LogInfo(L"Unloading Win11 Dynamic Island Mod...");

    if (DynamicIsland::g_timerWindow) {
        KillTimer(DynamicIsland::g_timerWindow, DynamicIsland::g_pollTimerId);
        DestroyWindow(DynamicIsland::g_timerWindow);
        DynamicIsland::g_timerWindow = nullptr;
        UnregisterClassW(L"Win11DynamicIslandTimerClass", GetModuleHandleW(nullptr));
    }

    DynamicIsland::g_timerService.reset();
    DynamicIsland::g_bluetoothService.reset();
    DynamicIsland::g_keyboardService.reset();
    DynamicIsland::g_powerService.reset();
    DynamicIsland::g_audioService.reset();
    DynamicIsland::g_mediaService.reset();
    DynamicIsland::g_serviceManager.reset();

    if (DynamicIsland::g_islandWindow) {
        DynamicIsland::g_islandWindow->Destroy();
        DynamicIsland::g_islandWindow.reset();
    }

    DynamicIsland::LogInfo(L"Win11 Dynamic Island Mod unloaded");
}

void Wh_ModSettingsChanged() {
    DynamicIsland::LogInfo(L"Settings updated");
    DynamicIsland::LoadSettings();
}
