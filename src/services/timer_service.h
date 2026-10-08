#pragma once

#include <windows.h>
#include <string>

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

    void SyncState();
};

} // namespace Services
} // namespace DynamicIsland
