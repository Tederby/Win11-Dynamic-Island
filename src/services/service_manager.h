#pragma once

#include <windows.h>
#include <deque>
#include <memory>
#include "../common/defs.h"
#include "../overlay/island_window.h"

namespace DynamicIsland {
namespace Services {

class MediaService;
class TimerService;

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

    void RegisterMediaService(MediaService* ms) { m_mediaService = ms; }
    void RegisterTimerService(TimerService* ts) { m_timerService = ts; }

    // Event triggering
    void PostTransientEvent(const QueuedEvent& event);
    void SetLiveActivity(EventType type, bool active);

    void Update();

    void UpdateMediaState(const MediaState& state);
    void UpdateTimerState(const TimerState& state);
    bool GetMediaThumbnail(std::vector<uint8_t>& pixels, uint32_t& width, uint32_t& height, uint64_t& version);

    // Interactive button actions from expanded Island
    void OnMediaPlayPause();
    void OnMediaPrev();
    void OnMediaNext();
    void OnTimerTogglePause();
    void OnTimerStop();

    // Testing / demo cycle
    void CycleDemoScenario();
    void TriggerTestScenario(int index);
    void FireTransient(EventType type);
    void RefreshVisibility();
    EventType ResolveCurrentLiveActivity() const;

private:
    Overlay::IslandWindow* m_window = nullptr;
    MediaService* m_mediaService = nullptr;
    TimerService* m_timerService = nullptr;
    std::deque<QueuedEvent> m_transientQueue;

    bool m_hasActiveTransient = false;
    QueuedEvent m_currentTransient;
    DWORD m_transientExpiryTick = 0;

    // Persistent live activities
    bool m_mediaActive = false;
    bool m_timerActive = false;
    bool m_micActive = false;
    int m_demoIndex = 0;
    std::wstring m_lastTrackTitle;

    void PumpNextEvent();
};

} // namespace Services
} // namespace DynamicIsland
