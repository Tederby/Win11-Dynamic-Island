#include "media_service.h"
#include "service_manager.h"
#include "../common/log.h"

namespace DynamicIsland {
namespace Services {

MediaService::MediaService(ServiceManager* manager)
    : m_manager(manager) {
    m_state.title = L"Senbonzakura";
    m_state.artist = L"Kurousa-P feat. Hatsune Miku";
    m_state.isPlaying = true;
    m_state.progress = 0.3f;
}

MediaService::~MediaService() {
    Stop();
}

void MediaService::Start() {
    m_isRunning = true;
    if (m_manager) {
        m_manager->SetLiveActivity(EventType::Media, true);
        m_manager->UpdateMediaState(m_state);
    }
    LogInfo(L"MediaService started");
}

void MediaService::Stop() {
    m_isRunning = false;
    if (m_manager) {
        m_manager->SetLiveActivity(EventType::Media, false);
    }
}

void MediaService::Poll() {
    if (!m_isRunning) return;

#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    try {
        if (!m_sessionManager) {
            m_sessionManager = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        }
        if (m_sessionManager) {
            winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession session = nullptr;
            auto sessions = m_sessionManager.GetSessions();
            uint32_t sessionCount = sessions ? sessions.Size() : 0;
            for (uint32_t i = 0; i < sessionCount; ++i) {
                auto s = sessions.GetAt(i);
                if (s) {
                    auto pb = s.GetPlaybackInfo();
                    if (pb && pb.PlaybackStatus() == winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
                        session = s;
                        break;
                    }
                }
            }
            if (!session) {
                session = m_sessionManager.GetCurrentSession();
            }

            m_currentSession = session;
            if (session) {
                auto props = session.TryGetMediaPropertiesAsync().get();
                auto info = session.GetPlaybackInfo();
                auto timeline = session.GetTimelineProperties();

                std::wstring title = props.Title().c_str();
                std::wstring artist = props.Artist().c_str();
                bool isPlaying = (info && info.PlaybackStatus() == winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);

                float prog = 0.0f;
                if (timeline) {
                    auto total = timeline.EndTime().count();
                    auto pos = timeline.Position().count();
                    if (total > 0) {
                        prog = static_cast<float>(pos) / static_cast<float>(total);
                        if (prog > 1.0f) prog = 1.0f;
                    }
                }

                if (!title.empty()) {
                    m_state.title = title;
                    m_state.artist = artist;
                    m_state.isPlaying = isPlaying;
                    if (prog > 0.0f) m_state.progress = prog;
                }

                if (m_manager) {
                    m_manager->UpdateMediaState(m_state);
                }
                return;
            }
        }
    } catch (...) {}
#endif

    // Fallback playback progression when no active WinRT session
    if (m_state.isPlaying) {
        m_state.progress += 0.0025f;
        if (m_state.progress >= 1.0f) {
            m_state.progress = 0.0f;
        }
        if (m_manager) {
            m_manager->UpdateMediaState(m_state);
        }
    }
}

void MediaService::Play() {
#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    if (m_currentSession) {
        try { m_currentSession.TryPlayAsync(); } catch (...) {}
    }
#endif
    m_state.isPlaying = true;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Pause() {
#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    if (m_currentSession) {
        try { m_currentSession.TryPauseAsync(); } catch (...) {}
    }
#endif
    m_state.isPlaying = false;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Next() {
#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    if (m_currentSession) {
        try { m_currentSession.TrySkipNextAsync(); } catch (...) {}
    }
#endif
    m_state.progress = 0.0f;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Previous() {
#if defined(DYNAMIC_ISLAND_HAS_WINRT_GSMTC)
    if (m_currentSession) {
        try { m_currentSession.TrySkipPreviousAsync(); } catch (...) {}
    }
#endif
    m_state.progress = 0.0f;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

} // namespace Services
} // namespace DynamicIsland
