#include "island_window.h"
#include "../services/service_manager.h"
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

    // Timer for equalizer wave and marquee animation ticks
    SetTimer(m_hwnd, m_waveTimerId, 40, nullptr);

    LogInfo(L"Dynamic Island overlay window created successfully");
    return true;
}

void IslandWindow::Destroy() {
    if (m_hwnd) {
        KillTimer(m_hwnd, m_autoCollapseTimerId);
        KillTimer(m_hwnd, m_animTimerId);
        KillTimer(m_hwnd, m_waveTimerId);
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

    if (m_animWidth.GetValue() <= 0.0f && target.width > 0.0f) {
        m_animWidth.SnapTo(target.width);
        m_animHeight.SnapTo(target.height);
        m_animRadius.SnapTo(target.cornerRadius);
        m_animOpacity.SnapTo(1.0f);
        TriggerAnimationUpdate();
    } else {
        m_animWidth.SetTarget(target.width, 450);
        m_animHeight.SetTarget(target.height, 450);
        m_animRadius.SetTarget(target.cornerRadius, 450);
        m_animOpacity.SetTarget(m_state == IslandState::Hidden ? 0.0f : 1.0f, 200);

        if (m_hwnd) {
            SetTimer(m_hwnd, m_animTimerId, 16, nullptr); // ~60fps spring animation tick
        }
    }
}

void IslandWindow::TriggerAnimationUpdate() {
    DWORD now = GetTickCount();
    m_animWidth.Update(now);
    m_animHeight.Update(now);
    m_animRadius.Update(now);
    m_animOpacity.Update(now);

    float curW = m_animWidth.GetValue();
    float curH = m_animHeight.GetValue();

    if (curW > 0.0f && curH > 0.0f && m_hwnd) {
        Platform::TaskbarInfo tb = Platform::QueryPrimaryTaskbar();
        int screenWidth = tb.monitorRect.right - tb.monitorRect.left;
        int centerX = tb.monitorRect.left + screenWidth / 2;
        if (tb.position == TaskbarPosition::Left || tb.position == TaskbarPosition::Right) {
            centerX = (tb.workAreaRect.left + tb.workAreaRect.right) / 2;
        }

        float curX = static_cast<float>(centerX) - (curW / 2.0f);
        float curY = 0.0f;

        if (tb.position == TaskbarPosition::Top) {
            int topAnchor = (tb.taskbarRect.top >= tb.monitorRect.top) ? tb.taskbarRect.top : tb.monitorRect.top;
            int tbHeight = tb.taskbarRect.bottom - tb.taskbarRect.top;
            if (tbHeight <= 0) tbHeight = 34;

            if (m_state == IslandState::Expanded) {
                curY = static_cast<float>(topAnchor + 3);
            } else {
                curY = static_cast<float>(topAnchor + (tbHeight - static_cast<int>(curH)) / 2);
            }
        } else {
            // Anchor at bottom, expand upwards
            int bottomAnchor = tb.workAreaRect.bottom;
            if (tb.position == TaskbarPosition::Bottom && !tb.isAutoHide) {
                bottomAnchor = tb.taskbarRect.top - 10;
            } else {
                bottomAnchor = tb.monitorRect.bottom - 14;
            }
            curY = static_cast<float>(bottomAnchor) - curH;
        }

        SetWindowPos(
            m_hwnd,
            HWND_TOPMOST,
            static_cast<int>(curX),
            static_cast<int>(curY),
            static_cast<int>(curW > 1.0f ? curW : 1.0f),
            static_cast<int>(curH > 1.0f ? curH : 1.0f),
            SWP_NOACTIVATE | SWP_NOZORDER
        );
        m_renderer->Resize(static_cast<UINT>(curW), static_cast<UINT>(curH));
    }

    Render();

    if (!m_animWidth.IsAnimating() && !m_animHeight.IsAnimating() &&
        !m_animRadius.IsAnimating() && !m_animOpacity.IsAnimating()) {
        KillTimer(m_hwnd, m_animTimerId);
    }
}

void IslandWindow::OnClick(int x, int y) {
    if (m_state == IslandState::Compact) {
        if (m_currentEvent == EventType::Media ||
            m_currentEvent == EventType::Timer ||
            m_currentEvent == EventType::MicStatus) {
            SetState(IslandState::Expanded);
        }
        return;
    }

    if (m_state == IslandState::Expanded) {
        float w = m_animWidth.GetValue();

        if (m_currentEvent == EventType::Media) {
            float midX = w / 2.0f;
            if (y >= 95 && y <= 135) {
                if (x >= midX - 50.0f && x <= midX - 15.0f) {
                    if (m_serviceManager) m_serviceManager->OnMediaPrev();
                    ArmAutoCollapse();
                    return;
                }
                if (x >= midX - 15.0f && x <= midX + 15.0f) {
                    if (m_serviceManager) m_serviceManager->OnMediaPlayPause();
                    ArmAutoCollapse();
                    return;
                }
                if (x >= midX + 15.0f && x <= midX + 50.0f) {
                    if (m_serviceManager) m_serviceManager->OnMediaNext();
                    ArmAutoCollapse();
                    return;
                }
            }
        } else if (m_currentEvent == EventType::Timer) {
            float btnW = 90.0f;
            float b1Left = (w - btnW * 2.0f - 16.0f) / 2.0f;
            if (y >= 70 && y <= 108) {
                if (x >= b1Left && x <= b1Left + btnW) {
                    if (m_serviceManager) m_serviceManager->OnTimerTogglePause();
                    ArmAutoCollapse();
                    return;
                }
                if (x >= b1Left + btnW + 16.0f && x <= b1Left + btnW * 2.0f + 16.0f) {
                    if (m_serviceManager) m_serviceManager->OnTimerStop();
                    SetState(IslandState::Compact);
                    return;
                }
            }
        }

        // Clicking outside control buttons collapses back to compact
        SetState(IslandState::Compact);
    }
}

void IslandWindow::OnRightClick() {
    if (m_serviceManager) {
        m_serviceManager->CycleDemoScenario();
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
        Platform::TaskbarInfo tb = Platform::QueryPrimaryTaskbar();
        bool isTop = (tb.position == TaskbarPosition::Top);

        D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(D2D1::RectF(0.0f, 0.0f, w, h), r, r);

        // Solid dark background and subtle border for clear visibility on top/bottom taskbars
        D2D1_COLOR_F bgColor = D2D1::ColorF(0.043f, 0.043f, 0.051f, 1.0f); // #0b0b0d
        D2D1_COLOR_F borderColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.16f);

        m_renderer->DrawRoundedPill(pill, bgColor, borderColor, 1.0f);

        if (m_state == IslandState::Compact) {
            switch (m_currentEvent) {
                case EventType::Media: {
                    // 18x18 gradient album art
                    D2D1_RECT_F artRect = D2D1::RectF(11.0f, (h - 18.0f) / 2.0f, 29.0f, (h + 18.0f) / 2.0f);
                    m_renderer->DrawAlbumArt(artRect, 5.0f);

                    // Track title marquee text
                    std::wstring track = m_mediaState.title + L" - " + m_mediaState.artist;
                    m_renderer->DrawTextString(
                        track,
                        D2D1::RectF(35.0f, (h - 16.0f) / 2.0f, w - 32.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        false
                    );

                    // Animated mint green 3-bar equalizer wave
                    m_renderer->DrawEqualizerWaves(
                        D2D1::Point2F(w - 24.0f, (h - 12.0f) / 2.0f),
                        12.0f,
                        m_mediaState.isPlaying ? m_waveProgress : 0.0f
                    );
                    break;
                }

                case EventType::Timer: {
                    float fraction = m_timerState.totalSeconds > 0
                        ? (1.0f - static_cast<float>(m_timerState.remainingSeconds) / static_cast<float>(m_timerState.totalSeconds))
                        : 0.0f;

                    // Progress ring arc
                    m_renderer->DrawProgressRing(
                        D2D1::Point2F(19.0f, h / 2.0f),
                        6.0f,
                        fraction,
                        D2D1::ColorF(1.0f, 0.624f, 0.039f, 1.0f), // Orange #ff9f0a
                        2.0f
                    );

                    // MM:SS countdown
                    m_renderer->DrawTextString(
                        Utils::FormatDuration(m_timerState.remainingSeconds),
                        D2D1::RectF(32.0f, (h - 16.0f) / 2.0f, w - 54.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        false
                    );

                    // Muted "Fokus" label
                    m_renderer->DrawTextString(
                        m_timerState.label,
                        D2D1::RectF(w - 52.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(0.66f, 0.67f, 0.71f, 1.0f),
                        12.0f,
                        false
                    );
                    break;
                }

                case EventType::MicStatus: {
                    m_renderer->DrawStatusDot(
                        D2D1::Point2F(19.0f, h / 2.0f),
                        4.0f,
                        D2D1::ColorF(1.0f, 0.624f, 0.039f, 1.0f) // #ff9f0a
                    );
                    m_renderer->DrawTextString(
                        L"Mikrofon aktif",
                        D2D1::RectF(32.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        false
                    );
                    break;
                }

                case EventType::Volume: {
                    D2D1_RECT_F volRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
                    m_renderer->DrawIconImage(Graphics::IconType::Volume, volRect, D2D1::ColorF(D2D1::ColorF::White));

                    float barLeft = 34.0f;
                    float barRight = w - 42.0f;
                    float frac = m_transientState.progressFraction > 0.0f ? m_transientState.progressFraction : 0.68f;
                    m_renderer->DrawProgressBar(
                        D2D1::RectF(barLeft, (h - 4.0f) / 2.0f, barRight, (h + 4.0f) / 2.0f),
                        frac,
                        D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.20f),
                        D2D1::ColorF(D2D1::ColorF::White)
                    );

                    int val = m_transientState.value > 0 ? m_transientState.value : 68;
                    m_renderer->DrawTextString(
                        std::to_wstring(val),
                        D2D1::RectF(w - 36.0f, (h - 16.0f) / 2.0f, w - 8.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        true
                    );
                    break;
                }

                case EventType::CapsLock: {
                    D2D1_RECT_F capsRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
                    m_renderer->DrawIconImage(Graphics::IconType::CapsLock, capsRect, D2D1::ColorF(D2D1::ColorF::White));

                    m_renderer->DrawTextString(
                        L"Caps Lock",
                        D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 45.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        false
                    );

                    std::wstring status = m_transientState.subtitle.empty() ? L"ON" : m_transientState.subtitle;
                    m_renderer->DrawTextString(
                        status,
                        D2D1::RectF(w - 42.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        true
                    );
                    break;
                }

                case EventType::Power: {
                    D2D1_RECT_F boltRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
                    m_renderer->DrawIconImage(Graphics::IconType::Bolt, boltRect, D2D1::ColorF(0.188f, 0.820f, 0.345f, 1.0f));

                    m_renderer->DrawTextString(
                        L"Mengisi daya",
                        D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 50.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        false
                    );

                    int pVal = m_transientState.value > 0 ? m_transientState.value : 62;
                    m_renderer->DrawTextString(
                        std::to_wstring(pVal) + L"%",
                        D2D1::RectF(w - 46.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        true
                    );
                    break;
                }

                case EventType::Bluetooth: {
                    D2D1_RECT_F btRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
                    m_renderer->DrawIconImage(Graphics::IconType::Bluetooth, btRect, D2D1::ColorF(D2D1::ColorF::White));

                    std::wstring dev = m_transientState.title.empty() ? L"WH-1000XM5" : m_transientState.title;
                    m_renderer->DrawTextString(
                        dev,
                        D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 50.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        false
                    );

                    int bVal = m_transientState.value > 0 ? m_transientState.value : 80;
                    m_renderer->DrawTextString(
                        std::to_wstring(bVal) + L"%",
                        D2D1::RectF(w - 46.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        true
                    );
                    break;
                }

                case EventType::LowBattery: {
                    D2D1_RECT_F batRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
                    m_renderer->DrawIconImage(Graphics::IconType::Battery, batRect, D2D1::ColorF(1.0f, 0.271f, 0.227f, 1.0f));

                    m_renderer->DrawTextString(
                        L"Baterai lemah",
                        D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 50.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        false
                    );

                    int lbVal = m_transientState.value > 0 ? m_transientState.value : 15;
                    m_renderer->DrawTextString(
                        std::to_wstring(lbVal) + L"%",
                        D2D1::RectF(w - 46.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(1.0f, 0.271f, 0.227f, 1.0f),
                        12.0f,
                        true
                    );
                    break;
                }

                case EventType::TimerDone: {
                    D2D1_RECT_F okRect = D2D1::RectF(12.0f, (h - 15.0f) / 2.0f, 27.0f, (h + 15.0f) / 2.0f);
                    m_renderer->DrawIconImage(Graphics::IconType::Checkmark, okRect, D2D1::ColorF(0.188f, 0.820f, 0.345f, 1.0f));

                    m_renderer->DrawTextString(
                        L"Timer selesai",
                        D2D1::RectF(34.0f, (h - 16.0f) / 2.0f, w - 10.0f, (h + 16.0f) / 2.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        12.0f,
                        false
                    );
                    break;
                }

                default:
                    break;
            }
        } else if (m_state == IslandState::Expanded) {
            switch (m_currentEvent) {
                case EventType::Media: {
                    // 52x52 album art
                    D2D1_RECT_F artRect = D2D1::RectF(16.0f, 16.0f, 68.0f, 68.0f);
                    m_renderer->DrawAlbumArt(artRect, 11.0f);

                    // Track title & artist
                    m_renderer->DrawTextString(
                        m_mediaState.title,
                        D2D1::RectF(80.0f, 22.0f, w - 16.0f, 42.0f),
                        D2D1::ColorF(D2D1::ColorF::White),
                        14.0f,
                        true
                    );
                    m_renderer->DrawTextString(
                        m_mediaState.artist,
                        D2D1::RectF(80.0f, 44.0f, w - 16.0f, 64.0f),
                        D2D1::ColorF(0.66f, 0.67f, 0.71f, 1.0f),
                        12.0f,
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
            } else if (wParam == self->m_waveTimerId) {
                self->m_waveProgress += 0.05f;
                if (self->m_waveProgress > 1.0f) self->m_waveProgress -= 1.0f;
                if (self->m_state != IslandState::Hidden) {
                    self->Render();
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

        case WM_RBUTTONUP:
            self->OnRightClick();
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
