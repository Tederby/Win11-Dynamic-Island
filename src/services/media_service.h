#pragma once

#include <windows.h>
#include <string>
#include "../common/defs.h"

#if defined(__has_include)
#if __has_include(<winrt/Windows.Foundation.h>) && __has_include(<winrt/Windows.Media.Control.h>)
#define DYNAMIC_ISLAND_HAS_WINRT_GSMTC 1
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#endif
#endif

namespace DynamicIsland {
namespace Services {

class ServiceManager;

class MediaService {
public:
    explicit MediaService(ServiceManager* manager);
    ~MediaService();

    void Start();
    void Stop();
    void Poll();

    const MediaState& GetState() const { return m_state; }

    void Play();
    void Pause();
    void Next();
    void Previous();

private:
    ServiceManager* m_manager = nullptr;
    MediaState m_state;
    bool m_isRunning = false;

#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager m_sessionManager{nullptr};
    winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession m_currentSession{nullptr};
#endif
};

} // namespace Services
} // namespace DynamicIsland
