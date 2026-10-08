#pragma once

#include <windows.h>
#include <memory>
#include "../common/defs.h"
#include "../graphics/d2d_renderer.h"
#include "../graphics/animation.h"
#include "layout.h"

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
