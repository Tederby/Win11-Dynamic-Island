#include "service_manager.h"
#include "media_service.h"
#include "timer_service.h"
#include "../common/log.h"

namespace DynamicIsland {
namespace Services {

ServiceManager::ServiceManager(Overlay::IslandWindow* window)
    : m_window(window) {
    if (m_window) {
        m_window->SetServiceManager(this);
    }
}

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
            if (g_settings.idleVisibilityMode == IdleVisibilityMode::AlwaysVisible) {
                m_window->SetCurrentEvent(EventType::None);
                m_window->SetState(IslandState::Compact);
            } else {
                m_window->SetState(IslandState::Hidden);
            }
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
                if (g_settings.idleVisibilityMode == IdleVisibilityMode::AlwaysVisible) {
                    m_window->SetCurrentEvent(EventType::None);
                    m_window->SetState(IslandState::Compact);
                } else {
                    m_window->SetState(IslandState::Hidden);
                }
            }
        }
        return;
    }

    m_currentTransient = m_transientQueue.front();
    m_transientQueue.pop_front();
    m_hasActiveTransient = true;
    m_transientExpiryTick = GetTickCount() + m_currentTransient.durationMs;

    if (m_window) {
        TransientState ts;
        ts.type = m_currentTransient.type;
        ts.title = m_currentTransient.title;
        ts.subtitle = m_currentTransient.subtitle;
        ts.value = m_currentTransient.progressPercent;
        ts.progressFraction = static_cast<float>(m_currentTransient.progressPercent) / 100.0f;

        m_window->UpdateTransientState(ts);
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

void ServiceManager::UpdateMediaState(const MediaState& state) {
    bool trackChanged = (m_lastTrackTitle != state.title);
    m_lastTrackTitle = state.title;

    if (m_window) {
        m_window->UpdateMediaState(state);
    }

    if (g_settings.mediaVisibilityPolicy == MediaVisibilityPolicy::TrackChangeOnly) {
        if (trackChanged && state.isPlaying && !state.title.empty()) {
            QueuedEvent qe;
            qe.type = EventType::Media;
            qe.durationMs = 3500;
            qe.title = state.title;
            qe.subtitle = state.artist;
            PostTransientEvent(qe);
        }
    } else {
        SetLiveActivity(EventType::Media, state.isPlaying);
    }
}

void ServiceManager::UpdateTimerState(const TimerState& state) {
    if (m_window) {
        m_window->UpdateTimerState(state);
    }
}

void ServiceManager::OnMediaPlayPause() {
    if (m_mediaService) {
        if (m_mediaService->GetState().isPlaying) {
            m_mediaService->Pause();
        } else {
            m_mediaService->Play();
        }
    }
    if (m_window) {
        m_window->GetMediaState().isPlaying = !m_window->GetMediaState().isPlaying;
        m_window->Render();
    }
}

void ServiceManager::OnMediaPrev() {
    if (m_mediaService) {
        m_mediaService->Previous();
    }
    if (m_window) {
        m_window->GetMediaState().progress = 0.0f;
        m_window->Render();
    }
}

void ServiceManager::OnMediaNext() {
    if (m_mediaService) {
        m_mediaService->Next();
    }
    if (m_window) {
        m_window->GetMediaState().progress = 0.0f;
        m_window->Render();
    }
}

void ServiceManager::OnTimerTogglePause() {
    if (m_timerService) {
        if (m_timerService->IsPaused()) {
            m_timerService->Resume();
        } else {
            m_timerService->Pause();
        }
    }
    if (m_window) {
        m_window->GetTimerState().isPaused = !m_window->GetTimerState().isPaused;
        m_window->Render();
    }
}

void ServiceManager::OnTimerStop() {
    if (m_timerService) {
        m_timerService->Stop();
    }
    SetLiveActivity(EventType::Timer, false);
}

void ServiceManager::FireTransient(EventType type) {
    QueuedEvent qe;
    qe.type = type;

    switch (type) {
        case EventType::Volume:
            qe.durationMs = DURATION_VOLUME_MS;
            qe.title = L"Volume";
            qe.progressPercent = 68;
            break;
        case EventType::CapsLock:
            qe.durationMs = DURATION_CAPS_LOCK_MS;
            qe.title = L"Caps Lock";
            qe.subtitle = L"ON";
            break;
        case EventType::Power:
            qe.durationMs = DURATION_POWER_MS;
            qe.title = L"Mengisi daya";
            qe.progressPercent = 62;
            break;
        case EventType::Bluetooth:
            qe.durationMs = DURATION_BLUETOOTH_MS;
            qe.title = L"WH-1000XM5";
            qe.progressPercent = 80;
            break;
        case EventType::LowBattery:
            qe.durationMs = DURATION_LOW_BATT_MS;
            qe.title = L"Baterai lemah";
            qe.progressPercent = 15;
            break;
        case EventType::TimerDone:
            qe.durationMs = DURATION_TIMER_DONE_MS;
            qe.title = L"Timer selesai";
            break;
        default:
            return;
    }

    PostTransientEvent(qe);
}

void ServiceManager::TriggerTestScenario(int index) {
    if (index >= 0 && index < 9) {
        m_demoIndex = index;
    } else {
        m_demoIndex = (m_demoIndex + 1) % 9;
    }

    m_transientQueue.clear();
    m_hasActiveTransient = false;

    switch (m_demoIndex) {
        case 0:
            m_timerActive = false;
            m_micActive = false;
            SetLiveActivity(EventType::Media, true);
            break;
        case 1:
            m_mediaActive = false;
            m_micActive = false;
            if (m_timerService && !m_timerService->IsActive()) {
                m_timerService->Start(30);
            }
            SetLiveActivity(EventType::Timer, true);
            break;
        case 2:
            m_mediaActive = false;
            m_timerActive = false;
            SetLiveActivity(EventType::MicStatus, true);
            break;
        case 3:
            FireTransient(EventType::Volume);
            break;
        case 4:
            FireTransient(EventType::CapsLock);
            break;
        case 5:
            FireTransient(EventType::Power);
            break;
        case 6:
            FireTransient(EventType::Bluetooth);
            break;
        case 7:
            FireTransient(EventType::LowBattery);
            break;
        case 8:
            FireTransient(EventType::TimerDone);
            break;
    }
}

void ServiceManager::CycleDemoScenario() {
    TriggerTestScenario(9);
}

void ServiceManager::RefreshVisibility() {
    if (!m_hasActiveTransient && m_window) {
        EventType current = ResolveCurrentLiveActivity();
        if (current != EventType::None) {
            m_window->SetCurrentEvent(current);
            m_window->SetState(IslandState::Compact);
        } else {
            if (g_settings.idleVisibilityMode == IdleVisibilityMode::AlwaysVisible) {
                m_window->SetCurrentEvent(EventType::None);
                m_window->SetState(IslandState::Compact);
            } else {
                m_window->SetState(IslandState::Hidden);
            }
        }
    }
}

EventType ServiceManager::ResolveCurrentLiveActivity() const {
    // Priority order: mic > timer > media
    if (m_micActive)   return EventType::MicStatus;
    if (m_timerActive) return EventType::Timer;
    if (m_mediaActive) return EventType::Media;
    return EventType::None;
}

} // namespace Services
} // namespace DynamicIsland
