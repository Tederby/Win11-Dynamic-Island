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
    RefreshVisibility();
    LogInfo(L"ServiceManager initialized");
}

void ServiceManager::Shutdown() {
    m_transientQueue.clear();
    m_hasActiveTransient = false;
}

void ServiceManager::PostTransientEvent(const QueuedEvent& event) {
    // Event Preemption: If currently displaying an event of the same type,
    // immediately supersede it without waiting for queue timeout (e.g. Caps Lock toggle, Volume adjustments)
    if (m_hasActiveTransient && m_currentTransient.type == event.type) {
        m_currentTransient = event;
        m_transientExpiryTick = GetTickCount() + event.durationMs;

        // Purge any pending duplicate events of the same category in the queue
        auto it = m_transientQueue.begin();
        while (it != m_transientQueue.end()) {
            if (it->type == event.type) {
                it = m_transientQueue.erase(it);
            } else {
                ++it;
            }
        }

        if (m_window) {
            TransientState ts;
            ts.type = event.type;
            ts.title = event.title;
            ts.subtitle = event.subtitle;
            ts.value = event.progressPercent;
            ts.progressFraction = static_cast<float>(event.progressPercent) / 100.0f;

            m_window->UpdateTransientState(ts);
            m_window->SetCurrentEvent(event.type);
            if (m_window->GetState() != IslandState::Expanded) {
                m_window->SetState(IslandState::Compact);
            }
        }
        return;
    }

    m_transientQueue.push_back(event);
    if (!m_hasActiveTransient) {
        PumpNextEvent();
    }
}

void ServiceManager::SetLiveActivity(EventType type, bool active) {
    bool stateChanged = false;
    switch (type) {
        case EventType::Media:     if (m_mediaActive != active) { m_mediaActive = active; stateChanged = true; } break;
        case EventType::Timer:     if (m_timerActive != active) { m_timerActive = active; stateChanged = true; } break;
        case EventType::MicStatus: if (m_micActive != active)   { m_micActive = active;   stateChanged = true; } break;
        default: break;
    }

    if (!m_hasActiveTransient && m_window) {
        EventType current = ResolveCurrentLiveActivity();
        if (current != EventType::None) {
            m_window->SetCurrentEvent(current);
            if (m_window->GetState() != IslandState::Expanded) {
                m_window->SetState(IslandState::Compact);
            }
        } else {
            if (g_settings.idleVisibilityMode == IdleVisibilityMode::AlwaysVisible) {
                m_window->SetCurrentEvent(EventType::None);
                if (m_window->GetState() != IslandState::Expanded) {
                    m_window->SetState(IslandState::Compact);
                }
            } else {
                if (m_window->GetState() != IslandState::Expanded) {
                    m_window->SetState(IslandState::Hidden);
                }
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
                if (m_window->GetState() != IslandState::Expanded) {
                    m_window->SetState(IslandState::Compact);
                }
            } else {
                if (g_settings.idleVisibilityMode == IdleVisibilityMode::AlwaysVisible) {
                    m_window->SetCurrentEvent(EventType::None);
                    if (m_window->GetState() != IslandState::Expanded) {
                        m_window->SetState(IslandState::Compact);
                    }
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
        if (m_window->GetState() != IslandState::Expanded) {
            m_window->SetState(IslandState::Compact);
        }
    }
}

bool ServiceManager::GetMediaThumbnail(std::vector<uint8_t>& pixels, uint32_t& width, uint32_t& height, uint64_t& version) {
    if (m_mediaService) {
        return m_mediaService->CopyThumbnail(pixels, width, height, version);
    }
    return false;
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
    if (m_mediaService && m_mediaService->HasRealSession()) {
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
    if (m_mediaService && m_mediaService->HasRealSession()) {
        m_mediaService->Previous();
    }
    if (m_window) {
        m_window->GetMediaState().progress = 0.0f;
        m_window->Render();
    }
}

void ServiceManager::OnMediaNext() {
    if (m_mediaService && m_mediaService->HasRealSession()) {
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
        case 0: {
            m_timerActive = false;
            m_micActive = false;
            MediaState demoMedia;
            demoMedia.title = L"Senbonzakura";
            demoMedia.artist = L"Kurousa-P feat. Hatsune Miku";
            demoMedia.isPlaying = true;
            demoMedia.progress = 0.3f;
            if (m_window) {
                m_window->UpdateMediaState(demoMedia);
            }
            SetLiveActivity(EventType::Media, true);
            break;
        }
        case 1:
            m_mediaActive = false;
            m_micActive = false;
            if (m_timerService && !m_timerService->IsActive()) {
                m_timerService->Start(30);
            }
            SetLiveActivity(EventType::Timer, true);
            break;
        case 2: {
            m_mediaActive = false;
            m_timerActive = false;
            MicState demoMic;
            demoMic.title = L"Mikrofon lagi dipakai";
            demoMic.appName = L"Discord";
            demoMic.isActive = true;
            if (m_window) {
                m_window->UpdateMicState(demoMic);
            }
            SetLiveActivity(EventType::MicStatus, true);
            break;
        }
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
            if (m_window->GetState() != IslandState::Expanded) {
                m_window->SetState(IslandState::Compact);
            }
        } else {
            if (g_settings.idleVisibilityMode == IdleVisibilityMode::AlwaysVisible) {
                m_window->SetCurrentEvent(EventType::None);
                if (m_window->GetState() != IslandState::Expanded) {
                    m_window->SetState(IslandState::Compact);
                }
            } else {
                if (m_window->GetState() != IslandState::Expanded) {
                    m_window->SetState(IslandState::Hidden);
                }
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
