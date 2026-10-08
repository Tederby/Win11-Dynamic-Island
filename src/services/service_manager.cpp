#include "service_manager.h"
#include "../common/log.h"

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
