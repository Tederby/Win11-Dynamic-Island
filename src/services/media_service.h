#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include "../common/defs.h"

#if defined(__has_include)
#if __has_include(<winrt/Windows.Foundation.h>) && __has_include(<winrt/Windows.Media.Control.h>) && __has_include(<winrt/Windows.Storage.Streams.h>)
#define DYNAMIC_ISLAND_HAS_WINRT_GSMTC 1
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
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
    bool HasRealSession() const { return m_hasRealSession; }

    void Play();
    void Pause();
    void Next();
    void Previous();

    bool CopyThumbnail(std::vector<uint8_t>& outPixels, uint32_t& outW, uint32_t& outH, uint64_t& outVersion) const;
    void StoreDecodedThumbnail(std::vector<uint8_t> pixels, uint32_t width, uint32_t height, const std::wstring& trackId);
    void FinishThumbnailFetch();

private:
    ServiceManager* m_manager = nullptr;
    MediaState m_state;
    bool m_isRunning = false;
    bool m_hasRealSession = false;

    mutable std::mutex m_thumbMutex;
    std::vector<uint8_t> m_thumbPixels;
    uint32_t m_thumbWidth = 0;
    uint32_t m_thumbHeight = 0;
    uint64_t m_thumbVersion = 0;
    std::wstring m_lastFetchedTrackId;
    std::atomic<bool> m_isFetchingThumbnail{false};

#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager m_sessionManager{nullptr};
    winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession m_currentSession{nullptr};

    void TriggerAsyncThumbnailFetch(winrt::Windows::Storage::Streams::IRandomAccessStreamReference thumbRef, const std::wstring& trackId);
#endif
};

} // namespace Services
} // namespace DynamicIsland
