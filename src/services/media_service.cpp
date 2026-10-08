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
    m_state.isPlaying = true;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Pause() {
    m_state.isPlaying = false;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Next() {
    m_state.progress = 0.0f;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

void MediaService::Previous() {
    m_state.progress = 0.0f;
    if (m_manager) {
        m_manager->UpdateMediaState(m_state);
    }
}

} // namespace Services
} // namespace DynamicIsland
