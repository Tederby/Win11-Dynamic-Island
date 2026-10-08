#include "island_window.h"
#include "../common/log.h"
#include "../common/utils.h"

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
