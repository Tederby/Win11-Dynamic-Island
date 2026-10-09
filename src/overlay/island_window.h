#pragma once

#include <windows.h>
#include <memory>
#include "../common/defs.h"
#include "../graphics/d2d_renderer.h"
#include "../graphics/animation.h"
#include "layout.h"

namespace DynamicIsland {

namespace Services {
class ServiceManager;
}

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
    void SetServiceManager(Services::ServiceManager* sm) { m_serviceManager = sm; }

    void UpdateMediaState(const MediaState& state) { m_mediaState = state; }
    void UpdateTimerState(const TimerState& state) { m_timerState = state; }
    void UpdateMicState(const MicState& state) { m_micState = state; }
    void UpdateTransientState(const TransientState& state) { m_transientState = state; }

    MediaState& GetMediaState() { return m_mediaState; }
    TimerState& GetTimerState() { return m_timerState; }
    MicState& GetMicState() { return m_micState; }
    TransientState& GetTransientState() { return m_transientState; }

    void TriggerAnimationUpdate();
    void Render();

    void UpdateDimensions();
    void OnTaskbarOrDisplayChanged();
    void RegisterHotkeys();
    void UnregisterHotkeys();

    HWND GetHwnd() const { return m_hwnd; }
    IslandState GetState() const { return m_state; }
    EventType GetCurrentEvent() const { return m_currentEvent; }

    // Message handler
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd = nullptr;
    IslandState m_state = IslandState::Hidden;
    EventType m_currentEvent = EventType::None;
    Services::ServiceManager* m_serviceManager = nullptr;

    MediaState m_mediaState;
    TimerState m_timerState;
    MicState m_micState;
    TransientState m_transientState;

    std::unique_ptr<Graphics::D2DRenderer> m_renderer;
    Graphics::AnimatedValue m_animWidth{0.0f};
    Graphics::AnimatedValue m_animHeight{32.0f};
    Graphics::AnimatedValue m_animRadius{16.0f};
    Graphics::AnimatedValue m_animOpacity{0.0f};

    bool m_isHovered = false;
    float m_waveProgress = 0.0f;
    UINT_PTR m_autoCollapseTimerId = 1001;
    UINT_PTR m_animTimerId = 1002;
    UINT_PTR m_waveTimerId = 1003;
    UINT_PTR m_geometryCheckTimerId = 1004;

    Platform::TaskbarInfo m_cachedTaskbar;
    UINT m_taskbarCreatedMsg = 0;
    bool m_hotkeysRegistered = false;

    int m_windowX = 0;
    int m_windowY = 0;
    int m_windowW = 0;
    int m_windowH = 0;
    float m_currentPillX = 0.0f;
    float m_currentPillY = 0.0f;
    float m_currentPillW = 0.0f;
    float m_currentPillH = 0.0f;
    float m_currentPillR = 0.0f;
    bool m_isMouseDown = false;
    int m_mouseDownX = 0;
    int m_mouseDownY = 0;

    void ArmAutoCollapse();
    void DisarmAutoCollapse();
    void OnClick(int clientX, int clientY);
    void OnRightClick();
    void CheckTaskbarGeometry();
    bool IsPointInSquircle(float clientX, float clientY) const;
};

} // namespace Overlay
} // namespace DynamicIsland
