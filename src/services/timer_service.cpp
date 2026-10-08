#include "timer_service.h"
#include "service_manager.h"
#include "../common/log.h"
#include "../common/defs.h"

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
    SyncState();
    LogInfo(L"Timer started for %d seconds", durationSeconds);
}

void TimerService::Pause() {
    m_isPaused = true;
    SyncState();
}

void TimerService::Resume() {
    m_isPaused = false;
    SyncState();
}

void TimerService::Stop() {
    m_isActive = false;
    m_isPaused = false;
    m_remainingSeconds = 0;

    if (m_manager) {
        m_manager->SetLiveActivity(EventType::Timer, false);
    }
    SyncState();
}

void TimerService::Tick() {
    if (!m_isActive || m_isPaused) return;

    if (m_remainingSeconds > 0) {
        m_remainingSeconds--;
    }

    SyncState();

    if (m_remainingSeconds <= 0) {
        Stop();
        if (m_manager) {
            QueuedEvent qe;
            qe.type = EventType::TimerDone;
            qe.durationMs = DURATION_TIMER_DONE_MS;
            qe.title = L"Timer selesai";
            m_manager->PostTransientEvent(qe);
        }
    }
}

float TimerService::GetProgressFraction() const {
    if (m_totalSeconds <= 0) return 0.0f;
    return 1.0f - (static_cast<float>(m_remainingSeconds) / static_cast<float>(m_totalSeconds));
}

void TimerService::SyncState() {
    if (!m_manager) return;
    TimerState ts;
    ts.remainingSeconds = m_remainingSeconds;
    ts.totalSeconds = m_totalSeconds;
    ts.isPaused = m_isPaused;
    ts.label = L"Fokus";
    ts.session = L"Sesi 1 dari 4";
    m_manager->UpdateTimerState(ts);
}

} // namespace Services
} // namespace DynamicIsland
