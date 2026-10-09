#include "island_window.h"
#include "../services/service_manager.h"
#include "../common/log.h"
#include "../common/utils.h"
#include <dwmapi.h>
#include <windowsx.h>

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

    m_cachedTaskbar = Platform::QueryPrimaryTaskbar();
    m_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");

    if (!m_renderer->Initialize(m_hwnd)) {
        LogError(L"Failed to initialize D2D renderer for island window");
        return false;
    }

    // Timer for equalizer wave and marquee animation ticks (smooth 60 FPS: 16ms)
    SetTimer(m_hwnd, m_waveTimerId, 16, nullptr);

    // Periodic timer for taskbar geometry polling (every 500ms)
    SetTimer(m_hwnd, m_geometryCheckTimerId, 500, nullptr);

    if (g_settings.enableDebugHotkeys) {
        RegisterHotkeys();
    }

    LogInfo(L"Dynamic Island overlay window created successfully");
    return true;
}

void IslandWindow::Destroy() {
    UnregisterHotkeys();
    if (m_hwnd) {
        KillTimer(m_hwnd, m_autoCollapseTimerId);
        KillTimer(m_hwnd, m_animTimerId);
        KillTimer(m_hwnd, m_waveTimerId);
        KillTimer(m_hwnd, m_geometryCheckTimerId);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandleW(nullptr));
}

static constexpr int HOTKEY_ID_BASE = 5100;

void IslandWindow::RegisterHotkeys() {
    if (!m_hwnd || m_hotkeysRegistered) return;
    for (int i = 0; i <= 9; ++i) {
        UINT vk = (i == 9) ? '0' : static_cast<UINT>('1' + i);
        RegisterHotKey(m_hwnd, HOTKEY_ID_BASE + i, MOD_CONTROL | MOD_WIN, vk);
    }
    m_hotkeysRegistered = true;
    LogInfo(L"Global test hotkeys registered (Ctrl+Win+1..9, 0)");
}

void IslandWindow::UnregisterHotkeys() {
    if (!m_hwnd || !m_hotkeysRegistered) return;
    for (int i = 0; i <= 9; ++i) {
        UnregisterHotKey(m_hwnd, HOTKEY_ID_BASE + i);
    }
    m_hotkeysRegistered = false;
}

void IslandWindow::OnTaskbarOrDisplayChanged() {
    Platform::TaskbarInfo tb = Platform::QueryPrimaryTaskbar();
    m_cachedTaskbar = tb;
    UpdateDimensions();
    TriggerAnimationUpdate();
}

void IslandWindow::CheckTaskbarGeometry() {
    Platform::TaskbarInfo tb = Platform::QueryPrimaryTaskbar();
    bool changed = (tb.position != m_cachedTaskbar.position) ||
                   (tb.isAutoHide != m_cachedTaskbar.isAutoHide) ||
                   (tb.taskbarRect.left != m_cachedTaskbar.taskbarRect.left) ||
                   (tb.taskbarRect.top != m_cachedTaskbar.taskbarRect.top) ||
                   (tb.taskbarRect.right != m_cachedTaskbar.taskbarRect.right) ||
                   (tb.taskbarRect.bottom != m_cachedTaskbar.taskbarRect.bottom) ||
                   (tb.workAreaRect.left != m_cachedTaskbar.workAreaRect.left) ||
                   (tb.workAreaRect.top != m_cachedTaskbar.workAreaRect.top) ||
                   (tb.workAreaRect.right != m_cachedTaskbar.workAreaRect.right) ||
                   (tb.workAreaRect.bottom != m_cachedTaskbar.workAreaRect.bottom);

    if (changed) {
        LogInfo(L"Detected real-time taskbar geometry change, adapting island position");
        m_cachedTaskbar = tb;
        UpdateDimensions();
    }
    if (m_hwnd && m_state != IslandState::Hidden) {
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void IslandWindow::Show(bool show) {
    if (m_hwnd) {
        if (show) {
            SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
        } else {
            ShowWindow(m_hwnd, SW_HIDE);
        }
    }
}

void IslandWindow::SetState(IslandState state) {
    if (m_state == state) return;
    IslandState oldState = m_state;
    m_state = state;

    if (m_state != IslandState::Hidden && oldState == IslandState::Hidden) {
        Show(true);
    }

    UpdateDimensions();

    if (m_state == IslandState::Expanded) {
        ArmAutoCollapse();
    } else {
        DisarmAutoCollapse();
    }
}

void IslandWindow::SetCurrentEvent(EventType eventType) {
    if (m_currentEvent != eventType) {
        m_prevEvent = m_currentEvent;
        m_prevTransientState = m_transientState;
        m_prevMediaState = m_mediaState;
        m_prevTimerState = m_timerState;
        m_currentEvent = eventType;

        m_crossfadeAlpha.SnapTo(0.0f);
        m_crossfadeAlpha.SetTarget(1.0f, 220);

        Platform::TaskbarInfo tb = Platform::QueryPrimaryTaskbar();
        IslandMetrics oldM = LayoutEngine::CalculateMetrics(m_prevEvent, m_state, tb);
        IslandMetrics newM = LayoutEngine::CalculateMetrics(m_currentEvent, m_state, tb);
        if (std::abs(oldM.width - newM.width) < 24.0f) {
            m_animScale.SnapTo(1.035f);
            m_animScale.SetTarget(1.0f, 260);
        }
    }
    UpdateDimensions();
}

void IslandWindow::ArmAutoCollapse() {
    if (!m_hwnd) return;
    DisarmAutoCollapse();
    UINT delayMs = (g_settings.autoCollapseDelayMs > 0) ? static_cast<UINT>(g_settings.autoCollapseDelayMs) : 1500;
    SetTimer(m_hwnd, m_autoCollapseTimerId, delayMs, nullptr);
}

void IslandWindow::DisarmAutoCollapse() {
    if (m_hwnd) {
        KillTimer(m_hwnd, m_autoCollapseTimerId);
    }
}

void IslandWindow::UpdateDimensions() {
    Platform::TaskbarInfo tb = Platform::QueryPrimaryTaskbar();
    m_cachedTaskbar = tb;
    IslandMetrics target = LayoutEngine::CalculateMetrics(m_currentEvent, m_state, tb);

    // Calculate maximum envelope bounds needed for transitions
    float animW = m_animWidth.GetValue();
    float animH = m_animHeight.GetValue();
    float maxW = (target.width > animW) ? target.width : animW;
    float maxH = (target.height > animH) ? target.height : animH;

    float minCanvasW = Utils::ScaleDpiF(360.0f, tb.dpi);
    float minCanvasH = Utils::ScaleDpiF(160.0f, tb.dpi);
    if (maxW < minCanvasW) maxW = minCanvasW;
    if (maxH < minCanvasH) maxH = minCanvasH;

    int screenWidth = tb.monitorRect.right - tb.monitorRect.left;
    int centerX = tb.monitorRect.left + screenWidth / 2;
    if (tb.position == TaskbarPosition::Left || tb.position == TaskbarPosition::Right) {
        centerX = (tb.workAreaRect.left + tb.workAreaRect.right) / 2;
    }

    int winW = static_cast<int>(maxW + 8.0f);
    int winH = static_cast<int>(maxH + 8.0f);
    int winX = centerX - (winW / 2);
    int winY = 0;

    if (tb.position == TaskbarPosition::Top) {
        int topAnchor = (tb.taskbarRect.top >= tb.monitorRect.top) ? tb.taskbarRect.top : tb.monitorRect.top;
        winY = topAnchor;
    } else {
        int bottomAnchor = tb.workAreaRect.bottom;
        if (tb.position == TaskbarPosition::Bottom && !tb.isAutoHide) {
            bottomAnchor = tb.taskbarRect.top - 10;
        } else {
            bottomAnchor = tb.monitorRect.bottom - 14;
        }
        winY = bottomAnchor - winH;
    }

    if (m_hwnd && (m_windowX != winX || m_windowY != winY || m_windowW != winW || m_windowH != winH)) {
        m_windowX = winX;
        m_windowY = winY;
        m_windowW = winW;
        m_windowH = winH;
        SetWindowPos(m_hwnd, HWND_TOPMOST, winX, winY, winW, winH, SWP_NOACTIVATE);
        m_renderer->Resize(static_cast<UINT>(winW), static_cast<UINT>(winH));
    }

    if (m_animWidth.GetValue() <= 0.0f && target.width > 0.0f) {
        // Void Entry Animation: smoothly spring open from 0.35 scale and 0 opacity
        m_animWidth.SnapTo(target.width * 0.35f);
        m_animHeight.SnapTo(target.height * 0.35f);
        m_animRadius.SnapTo(target.cornerRadius * 0.35f);
        m_animOpacity.SnapTo(0.0f);

        m_animWidth.SetTarget(target.width, 450);
        m_animHeight.SetTarget(target.height, 450);
        m_animRadius.SetTarget(target.cornerRadius, 450);
        m_animOpacity.SetTarget(1.0f, 250);

        if (m_hwnd) {
            SetTimer(m_hwnd, m_animTimerId, 8, nullptr);
        }
        TriggerAnimationUpdate();
    } else {
        m_animWidth.SetTarget(target.width, 450);
        m_animHeight.SetTarget(target.height, 450);
        m_animRadius.SetTarget(target.cornerRadius, 450);
        m_animOpacity.SetTarget(m_state == IslandState::Hidden ? 0.0f : 1.0f, 200);

        if (m_hwnd) {
            SetTimer(m_hwnd, m_animTimerId, 8, nullptr); // ~120Hz-240Hz high refresh rate tick
        }
    }
}

void IslandWindow::TriggerAnimationUpdate() {
    double now = Graphics::GetHighPrecisionTimeMs();
    m_animWidth.Update(now);
    m_animHeight.Update(now);
    m_animRadius.Update(now);
    m_animOpacity.Update(now);
    m_crossfadeAlpha.Update(now);
    m_animScale.Update(now);

    float curW = m_animWidth.GetValue();
    float curH = m_animHeight.GetValue();
    float curR = m_animRadius.GetValue();

    if (curW > 0.0f && curH > 0.0f && m_hwnd) {
        Platform::TaskbarInfo tb = m_cachedTaskbar;
        float pillX = (static_cast<float>(m_windowW) - curW) / 2.0f;
        float pillY = 0.0f;

        if (tb.position == TaskbarPosition::Top) {
            int tbHeight = tb.taskbarRect.bottom - tb.taskbarRect.top;
            if (tbHeight <= 0) tbHeight = 34;

            if (m_state == IslandState::Expanded) {
                pillY = 3.0f;
            } else {
                pillY = (static_cast<float>(tbHeight) - curH) / 2.0f;
                if (pillY < 0.0f) pillY = 0.0f;
            }
        } else {
            pillY = static_cast<float>(m_windowH) - curH;
        }

        m_currentPillX = pillX;
        m_currentPillY = pillY;
        m_currentPillW = curW;
        m_currentPillH = curH;
        m_currentPillR = curR;
    }

    Render();

    if (!m_animWidth.IsAnimating() && !m_animHeight.IsAnimating() &&
        !m_animRadius.IsAnimating() && !m_animOpacity.IsAnimating() &&
        !m_crossfadeAlpha.IsAnimating() && !m_animScale.IsAnimating()) {
        KillTimer(m_hwnd, m_animTimerId);
        if (m_state == IslandState::Hidden) {
            ShowWindow(m_hwnd, SW_HIDE);
        }
    }
}

bool IslandWindow::IsPointInSquircle(float px, float py) const {
    if (m_animOpacity.GetValue() <= 0.05f) return false;

    float left = m_currentPillX;
    float top = m_currentPillY;
    float right = m_currentPillX + m_currentPillW;
    float bottom = m_currentPillY + m_currentPillH;
    float r = m_currentPillR;

    if (px < left || px > right || py < top || py > bottom) {
        return false;
    }

    if (r <= 0.0f) return true;

    // Inside central rectangle horizontally or vertically
    if ((px >= left + r && px <= right - r) || (py >= top + r && py <= bottom - r)) {
        return true;
    }

    // Determine nearest corner arc center
    float cx = (px < left + r) ? (left + r) : (right - r);
    float cy = (py < top + r) ? (top + r) : (bottom - r);

    float dx = px - cx;
    float dy = py - cy;
    return (dx * dx + dy * dy) <= (r * r);
}

void IslandWindow::OnClick(int clientX, int clientY) {
    float localX = static_cast<float>(clientX) - m_currentPillX;
    float localY = static_cast<float>(clientY) - m_currentPillY;
    float w = m_animWidth.GetValue();
    float h = m_animHeight.GetValue();

    // Verify click is within pill bounds
    if (localX < 0.0f || localX > w || localY < 0.0f || localY > h) {
        return;
    }

    if (m_state == IslandState::Compact) {
        if (m_currentEvent == EventType::Media ||
            m_currentEvent == EventType::Timer ||
            m_currentEvent == EventType::MicStatus) {
            SetState(IslandState::Expanded);
            m_isHovered = true;
            TRACKMOUSEEVENT tme{sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hwnd, 0};
            TrackMouseEvent(&tme);
            ArmAutoCollapse();
        }
        return;
    }

    if (m_state == IslandState::Expanded) {
        UINT dpi = m_cachedTaskbar.dpi;
        if (m_currentEvent == EventType::Media) {
            float midX = w / 2.0f;
            float btnY1 = Utils::ScaleDpiF(98.0f, dpi);
            float btnY2 = Utils::ScaleDpiF(136.0f, dpi);

            if (localY >= btnY1 && localY <= btnY2) {
                // Prev button (midX - 48 to midX - 12)
                float prevX1 = midX - Utils::ScaleDpiF(48.0f, dpi);
                float prevX2 = midX - Utils::ScaleDpiF(12.0f, dpi);
                if (localX >= prevX1 && localX <= prevX2) {
                    if (m_serviceManager) m_serviceManager->OnMediaPrev();
                    ArmAutoCollapse();
                    return;
                }

                // Play / Pause button (midX - 16 to midX + 16)
                float playX1 = midX - Utils::ScaleDpiF(16.0f, dpi);
                float playX2 = midX + Utils::ScaleDpiF(16.0f, dpi);
                if (localX >= playX1 && localX <= playX2) {
                    if (m_serviceManager) m_serviceManager->OnMediaPlayPause();
                    ArmAutoCollapse();
                    return;
                }

                // Next button (midX + 12 to midX + 48)
                float nextX1 = midX + Utils::ScaleDpiF(12.0f, dpi);
                float nextX2 = midX + Utils::ScaleDpiF(48.0f, dpi);
                if (localX >= nextX1 && localX <= nextX2) {
                    if (m_serviceManager) m_serviceManager->OnMediaNext();
                    ArmAutoCollapse();
                    return;
                }
            }
        } else if (m_currentEvent == EventType::Timer) {
            float btnW = Utils::ScaleDpiF(90.0f, dpi);
            float btnGap = Utils::ScaleDpiF(16.0f, dpi);
            float b1Left = (w - btnW * 2.0f - btnGap) / 2.0f;
            float btnY1 = Utils::ScaleDpiF(70.0f, dpi);
            float btnY2 = Utils::ScaleDpiF(108.0f, dpi);

            if (localY >= btnY1 && localY <= btnY2) {
                if (localX >= b1Left && localX <= b1Left + btnW) {
                    if (m_serviceManager) m_serviceManager->OnTimerTogglePause();
                    ArmAutoCollapse();
                    return;
                }
                if (localX >= b1Left + btnW + btnGap && localX <= b1Left + btnW * 2.0f + btnGap) {
                    if (m_serviceManager) m_serviceManager->OnTimerStop();
                    SetState(IslandState::Compact);
                    return;
                }
            }
        }

        // Clicking on content body (album art, title, timer digits, empty card background)
        // keeps the island open and resets the inactivity collapse timer
        ArmAutoCollapse();
    }
}

void IslandWindow::RenderCompactContent(EventType eventType, float w, float h, float alpha) {
    if (alpha <= 0.005f || !m_renderer) return;

    const TransientState& trState = (eventType == m_prevEvent) ? m_prevTransientState : m_transientState;
    const MediaState& medState = (eventType == m_prevEvent) ? m_prevMediaState : m_mediaState;
    const TimerState& timState = (eventType == m_prevEvent) ? m_prevTimerState : m_timerState;

    switch (eventType) {
        case EventType::Media: {
            // 18x18 album art with 1:1 cover-crop or gradient fallback
            D2D1_RECT_F artRect = D2D1::RectF(11.0f, (h - 18.0f) / 2.0f, 29.0f, (h + 18.0f) / 2.0f);
            m_renderer->DrawAlbumArt(artRect, 5.0f);

            // Track title marquee text
            std::wstring track = medState.title;
            if (!medState.artist.empty()) {
                track += L" \x2022 " + medState.artist;
            }
            m_renderer->DrawMarqueeText(
                track,
                D2D1::RectF(35.0f, (h - 16.0f) / 2.0f, w - 32.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                medState.isPlaying ? m_marqueeOffset : 0.0f,
                false
            );

            // Animated mint green 3-bar equalizer wave
            m_renderer->DrawEqualizerWaves(
                D2D1::Point2F(w - 24.0f, (h - 12.0f) / 2.0f),
                12.0f,
                medState.isPlaying ? m_waveProgress : 0.0f
            );
            break;
        }

        case EventType::Timer: {
            float fraction = timState.totalSeconds > 0
                ? (1.0f - static_cast<float>(timState.remainingSeconds) / static_cast<float>(timState.totalSeconds))
                : 0.0f;

            m_renderer->DrawProgressRing(
                D2D1::Point2F(19.0f, h / 2.0f),
                6.0f,
                fraction,
                D2D1::ColorF(1.0f, 0.624f, 0.039f, alpha),
                2.0f
            );

            m_renderer->DrawTextString(
                Utils::FormatDuration(timState.remainingSeconds),
                D2D1::RectF(32.0f, (h - 16.0f) / 2.0f, w - 54.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                false
            );

            m_renderer->DrawTextString(
                timState.label,
                D2D1::RectF(w - 52.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(0.66f, 0.67f, 0.71f, alpha),
                12.0f,
                false
            );
            break;
        }

        case EventType::MicStatus: {
            m_renderer->DrawStatusDot(
                D2D1::Point2F(19.0f, h / 2.0f),
                4.0f,
                D2D1::ColorF(1.0f, 0.624f, 0.039f, alpha)
            );
            m_renderer->DrawTextString(
                L"Mikrofon aktif",
                D2D1::RectF(32.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                false
            );
            break;
        }

        case EventType::Volume: {
            D2D1_RECT_F volRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
            m_renderer->DrawIconImage(Graphics::IconType::Volume, volRect, D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha));

            float barLeft = 34.0f;
            float barRight = w - 42.0f;
            float frac = trState.progressFraction > 0.0f ? trState.progressFraction : 0.68f;
            m_renderer->DrawProgressBar(
                D2D1::RectF(barLeft, (h - 4.0f) / 2.0f, barRight, (h + 4.0f) / 2.0f),
                frac,
                D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.20f * alpha),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha)
            );

            int val = trState.value > 0 ? trState.value : 68;
            m_renderer->DrawTextString(
                std::to_wstring(val),
                D2D1::RectF(w - 36.0f, (h - 16.0f) / 2.0f, w - 8.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                true
            );
            break;
        }

        case EventType::CapsLock: {
            D2D1_RECT_F capsRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
            m_renderer->DrawIconImage(Graphics::IconType::CapsLock, capsRect, D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha));

            m_renderer->DrawTextString(
                L"Caps Lock",
                D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 45.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                false
            );

            std::wstring status = trState.subtitle.empty() ? L"ON" : trState.subtitle;
            m_renderer->DrawTextString(
                status,
                D2D1::RectF(w - 42.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                true
            );
            break;
        }

        case EventType::Power: {
            D2D1_RECT_F boltRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
            m_renderer->DrawIconImage(Graphics::IconType::Bolt, boltRect, D2D1::ColorF(0.188f, 0.820f, 0.345f, alpha));

            m_renderer->DrawTextString(
                L"Mengisi daya",
                D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 50.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                false
            );

            int pVal = trState.value > 0 ? trState.value : 62;
            m_renderer->DrawTextString(
                std::to_wstring(pVal) + L"%",
                D2D1::RectF(w - 46.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                true
            );
            break;
        }

        case EventType::Bluetooth: {
            D2D1_RECT_F btRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
            m_renderer->DrawIconImage(Graphics::IconType::Bluetooth, btRect, D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha));

            std::wstring dev = trState.title.empty() ? L"WH-1000XM5" : trState.title;
            m_renderer->DrawTextString(
                dev,
                D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 50.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                false
            );

            int bVal = trState.value > 0 ? trState.value : 80;
            m_renderer->DrawTextString(
                std::to_wstring(bVal) + L"%",
                D2D1::RectF(w - 46.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                true
            );
            break;
        }

        case EventType::LowBattery: {
            D2D1_RECT_F batRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
            m_renderer->DrawIconImage(Graphics::IconType::Battery, batRect, D2D1::ColorF(1.0f, 0.271f, 0.227f, alpha));

            m_renderer->DrawTextString(
                L"Baterai lemah",
                D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 50.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                false
            );

            int lbVal = trState.value > 0 ? trState.value : 15;
            m_renderer->DrawTextString(
                std::to_wstring(lbVal) + L"%",
                D2D1::RectF(w - 46.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 0.271f, 0.227f, alpha),
                12.0f,
                true
            );
            break;
        }

        case EventType::TimerDone: {
            D2D1_RECT_F okRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
            m_renderer->DrawIconImage(Graphics::IconType::Checkmark, okRect, D2D1::ColorF(0.188f, 0.820f, 0.345f, alpha));

            m_renderer->DrawTextString(
                L"Timer selesai",
                D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                false
            );
            break;
        }

        default: {
            // Minimalist idle digital clock (HH:mm) with mathematical dead-centering
            SYSTEMTIME st;
            GetLocalTime(&st);
            wchar_t timeBuf[16];
            swprintf_s(timeBuf, L"%02d:%02d", st.wHour, st.wMinute);

            float textW = 34.0f;
            if (m_renderer->GetDWriteFactory()) {
                Microsoft::WRL::ComPtr<IDWriteTextFormat> fmt;
                if (SUCCEEDED(m_renderer->GetDWriteFactory()->CreateTextFormat(
                    L"Segoe UI Variable", nullptr,
                    DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                    12.0f, L"en-US", fmt.GetAddressOf()))) {
                    Microsoft::WRL::ComPtr<IDWriteTextLayout> tLayout;
                    if (SUCCEEDED(m_renderer->GetDWriteFactory()->CreateTextLayout(
                        timeBuf, static_cast<UINT32>(wcslen(timeBuf)), fmt.Get(), 1000.0f, h, tLayout.GetAddressOf()))) {
                        DWRITE_TEXT_METRICS tm;
                        tLayout->GetMetrics(&tm);
                        textW = tm.width;
                    }
                }
            }

            // Total element group: Dot (dia: 5px) + Gap (6px) + Text (textW)
            float groupW = 5.0f + 6.0f + textW;
            float startX = (w - groupW) / 2.0f;
            if (startX < 6.0f) startX = 6.0f;

            double now = Graphics::GetHighPrecisionTimeMs();
            float pulse = 0.70f + 0.30f * static_cast<float>(std::sin(now * 0.003));

            m_renderer->DrawStatusDot(
                D2D1::Point2F(startX + 2.5f, h / 2.0f),
                2.5f,
                D2D1::ColorF(0.49f, 0.88f, 0.76f, pulse * alpha)
            );

            m_renderer->DrawTextString(
                timeBuf,
                D2D1::RectF(startX + 11.0f, (h - 16.0f) / 2.0f, startX + 11.0f + textW + 2.0f, (h + 16.0f) / 2.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha),
                12.0f,
                true
            );
            break;
        }
    }
}

void IslandWindow::Render() {
    if (!m_renderer) return;

    // Check for updated media thumbnail
    if (m_serviceManager) {
        std::vector<uint8_t> thumbPixels;
        uint32_t thumbW = 0, thumbH = 0;
        if (m_serviceManager->GetMediaThumbnail(thumbPixels, thumbW, thumbH, m_currentThumbVersion)) {
            if (!thumbPixels.empty() && thumbW > 0 && thumbH > 0) {
                m_renderer->SetAlbumArtBitmap(thumbPixels.data(), thumbW, thumbH);
            } else {
                m_renderer->ClearAlbumArtBitmap();
            }
        }
    }

    m_renderer->BeginDraw();
    m_renderer->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

    float w = m_animWidth.GetValue();
    float h = m_animHeight.GetValue();
    float r = m_animRadius.GetValue();
    float o = m_animOpacity.GetValue();

    if (w > 0.0f && h > 0.0f && o > 0.01f) {
        ID2D1RenderTarget* rt = m_renderer->GetRenderTarget();
        if (rt) {
            float scale = m_animScale.GetValue();
            if (std::abs(scale - 1.0f) > 0.001f) {
                D2D1_POINT_2F center = D2D1::Point2F(m_currentPillX + w / 2.0f, m_currentPillY + h / 2.0f);
                rt->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale, center) * D2D1::Matrix3x2F::Translation(m_currentPillX, m_currentPillY));
            } else {
                rt->SetTransform(D2D1::Matrix3x2F::Translation(m_currentPillX, m_currentPillY));
            }
        }

        D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(D2D1::RectF(0.0f, 0.0f, w, h), r, r);

        // Pure pitch-black background (#000000) with zero border/outline stroke
        D2D1_COLOR_F bgColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f * o);
        D2D1_COLOR_F borderColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);

        m_renderer->DrawRoundedPill(pill, bgColor, borderColor, 0.0f);

        if (m_state == IslandState::Compact) {
            if (m_crossfadeAlpha.IsAnimating() && m_prevEvent != EventType::None && m_prevEvent != m_currentEvent) {
                float aPrev = (1.0f - m_crossfadeAlpha.GetValue()) * o;
                float aCur = m_crossfadeAlpha.GetValue() * o;
                RenderCompactContent(m_prevEvent, w, h, aPrev);
                RenderCompactContent(m_currentEvent, w, h, aCur);
            } else {
                RenderCompactContent(m_currentEvent, w, h, o);
            }
        } else if (m_state == IslandState::Expanded) {
            switch (m_currentEvent) {
                case EventType::Media: {
                    // 52x52 album art
                    D2D1_RECT_F artRect = D2D1::RectF(16.0f, 16.0f, 68.0f, 68.0f);
                    m_renderer->DrawAlbumArt(artRect, 11.0f);

                    // Track title & artist (with smooth horizontal marquee)
                    m_renderer->DrawMarqueeText(
                        m_mediaState.title,
                        D2D1::RectF(80.0f, 22.0f, w - 16.0f, 42.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        m_mediaState.isPlaying ? m_marqueeOffset : 0.0f,
                        true
                    );
                    m_renderer->DrawMarqueeText(
                        m_mediaState.artist,
                        D2D1::RectF(80.0f, 44.0f, w - 16.0f, 64.0f),
                        D2D1::ColorF(0.66f, 0.67f, 0.71f, 1.0f),
                        m_mediaState.isPlaying ? m_marqueeOffset * 0.7f : 0.0f,
                        false
                    );

                    // Progress bar
                    m_renderer->DrawProgressBar(
                        D2D1::RectF(16.0f, 82.0f, w - 16.0f, 86.0f),
                        m_mediaState.progress,
                        D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f),
                        D2D1::ColorF(D2D1::ColorF::White)
                    );

                    // Playback controls (Prev, Play/Pause, Next)
                    float midX = w / 2.0f;
                    m_renderer->DrawCircularButton(
                        D2D1::RectF(midX - 45.0f, 102.0f, midX - 15.0f, 132.0f),
                        Graphics::IconType::Previous,
                        false
                    );
                    m_renderer->DrawCircularButton(
                        D2D1::RectF(midX - 15.0f, 102.0f, midX + 15.0f, 132.0f),
                        m_mediaState.isPlaying ? Graphics::IconType::Pause : Graphics::IconType::Play,
                        false
                    );
                    m_renderer->DrawCircularButton(
                        D2D1::RectF(midX + 15.0f, 102.0f, midX + 45.0f, 132.0f),
                        Graphics::IconType::Next,
                        false
                    );
                    break;
                }

                case EventType::Timer: {
                    // Big 32px timer
                    m_renderer->DrawBigText(
                        Utils::FormatDuration(m_timerState.remainingSeconds),
                        D2D1::RectF(20.0f, 16.0f, 120.0f, 60.0f),
                        D2D1::ColorF(D2D1::ColorF::White)
                    );

                    // Session title & subtitle
                    m_renderer->DrawTextString(
                        m_timerState.label,
                        D2D1::RectF(126.0f, 18.0f, w - 16.0f, 38.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        14.0f,
                        true
                    );
                    m_renderer->DrawTextString(
                        m_timerState.session,
                        D2D1::RectF(126.0f, 38.0f, w - 16.0f, 58.0f),
                        D2D1::ColorF(0.66f, 0.67f, 0.71f, 1.0f),
                        12.0f,
                        false
                    );

                    // Pill buttons (Jeda/Lanjut, Stop)
                    float btnW = 90.0f;
                    float btnH = 28.0f;
                    float b1Left = (w - btnW * 2.0f - 16.0f) / 2.0f;

                    std::wstring pauseText = m_timerState.isPaused ? L"Lanjut" : L"Jeda";
                    m_renderer->DrawButtonPill(
                        D2D1::RectF(b1Left, 74.0f, b1Left + btnW, 74.0f + btnH),
                        pauseText,
                        false
                    );
                    m_renderer->DrawButtonPill(
                        D2D1::RectF(b1Left + btnW + 16.0f, 74.0f, b1Left + btnW * 2.0f + 16.0f, 74.0f + btnH),
                        L"Stop",
                        false
                    );
                    break;
                }

                case EventType::MicStatus: {
                    m_renderer->DrawStatusDot(
                        D2D1::Point2F(28.0f, 39.0f),
                        5.0f,
                        D2D1::ColorF(1.0f, 0.624f, 0.039f, 1.0f)
                    );
                    m_renderer->DrawTextString(
                        m_micState.title,
                        D2D1::RectF(46.0f, 20.0f, w - 16.0f, 40.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        14.0f,
                        true
                    );
                    m_renderer->DrawTextString(
                        m_micState.appName,
                        D2D1::RectF(46.0f, 42.0f, w - 16.0f, 62.0f),
                        D2D1::ColorF(0.66f, 0.67f, 0.71f, 1.0f),
                        12.0f,
                        false
                    );
                    break;
                }

                default:
                    break;
            }
        }
        if (rt) {
            rt->SetTransform(D2D1::Matrix3x2F::Identity());
        }
    }

    m_renderer->EndDraw();
    m_renderer->PresentLayeredWindow(m_hwnd, m_windowX, m_windowY);
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

    if (self->m_taskbarCreatedMsg != 0 && msg == self->m_taskbarCreatedMsg) {
        LogInfo(L"TaskbarCreated window message received, adapting island window");
        self->OnTaskbarOrDisplayChanged();
        return 0;
    }

    switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            if (self->IsPointInSquircle(static_cast<float>(pt.x), static_cast<float>(pt.y))) {
                return HTCLIENT;
            }
            return HTTRANSPARENT;
        }

        case WM_HOTKEY: {
            int hotkeyIdx = static_cast<int>(wParam) - HOTKEY_ID_BASE;
            if (self->m_serviceManager && hotkeyIdx >= 0 && hotkeyIdx <= 9) {
                self->m_serviceManager->TriggerTestScenario(hotkeyIdx);
            }
            return 0;
        }

        case WM_SETTINGCHANGE:
        case WM_DISPLAYCHANGE:
        case WM_DPICHANGED:
            self->OnTaskbarOrDisplayChanged();
            return 0;

        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;

        case WM_TIMER:
            if (wParam == self->m_animTimerId) {
                self->TriggerAnimationUpdate();
            } else if (wParam == self->m_autoCollapseTimerId) {
                if (!self->m_isHovered && self->m_state == IslandState::Expanded) {
                    self->SetState(IslandState::Compact);
                }
            } else if (wParam == self->m_waveTimerId) {
                double now = Graphics::GetHighPrecisionTimeMs();
                if (self->m_lastMarqueeTimeMs > 0.0) {
                    double dt = (now - self->m_lastMarqueeTimeMs) / 1000.0;
                    if (dt > 0.1) dt = 0.1;
                    self->m_marqueeOffset += static_cast<float>(dt * 16.0); // smooth, readable 16 px/sec
                    self->m_waveProgress += static_cast<float>(dt * 1.25);
                    if (self->m_waveProgress > 1.0f) self->m_waveProgress -= 1.0f;
                }
                self->m_lastMarqueeTimeMs = now;

                if (self->m_state != IslandState::Hidden) {
                    self->Render();
                }
            } else if (wParam == self->m_geometryCheckTimerId) {
                self->CheckTaskbarGeometry();
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

        case WM_LBUTTONDOWN:
            self->m_isMouseDown = true;
            self->m_mouseDownX = GET_X_LPARAM(lParam);
            self->m_mouseDownY = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            return 0;

        case WM_LBUTTONUP:
            if (self->m_isMouseDown) {
                self->m_isMouseDown = false;
                ReleaseCapture();
                int upX = GET_X_LPARAM(lParam);
                int upY = GET_Y_LPARAM(lParam);
                if (std::abs(upX - self->m_mouseDownX) < 10 && std::abs(upY - self->m_mouseDownY) < 10) {
                    self->OnClick(upX, upY);
                }
            }
            return 0;

        case WM_CAPTURECHANGED:
            self->m_isMouseDown = false;
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
