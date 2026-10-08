#pragma once

#include <windows.h>
#include <deque>
#include <memory>
#include "../common/defs.h"
#include "../overlay/island_window.h"

namespace DynamicIsland {
namespace Services {

struct QueuedEvent {
    EventType type = EventType::None;
    DWORD durationMs = 2000;
    std::wstring title;
    std::wstring subtitle;
    int progressPercent = 0;
};

class ServiceManager {
public:
    explicit ServiceManager(Overlay::IslandWindow* window);
    ~ServiceManager();

    void Initialize();
    void Shutdown();

    // Event triggering
    void PostTransientEvent(const QueuedEvent& event);
    void SetLiveActivity(EventType type, bool active);

    void Update();

private:
    Overlay::IslandWindow* m_window = nullptr;
    std::deque<QueuedEvent> m_transientQueue;

    bool m_hasActiveTransient = false;
    QueuedEvent m_currentTransient;
    DWORD m_transientExpiryTick = 0;

    // Persistent live activities
    bool m_mediaActive = false;
    bool m_timerActive = false;
    bool m_micActive = false;

    void PumpNextEvent();
    EventType ResolveCurrentLiveActivity() const;
};

} // namespace Services
} // namespace DynamicIsland
