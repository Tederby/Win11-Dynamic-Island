#include "media_service.h"
#include "service_manager.h"
#include "../common/log.h"

namespace DynamicIsland {
namespace Services {

MediaService::MediaService(ServiceManager* manager)
    : m_manager(manager) {}

MediaService::~MediaService() {
    Stop();
}

void MediaService::Start() {
    m_isRunning = true;
    LogInfo(L"MediaService started");
}

void MediaService::Stop() {
    m_isRunning = false;
}

void MediaService::Poll() {
    if (!m_isRunning) return;
    // Integration with Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager
    // GSMTC listener will update m_state and inform m_manager->SetLiveActivity(EventType::Media, m_state.isPlaying);
}

void MediaService::Play() {
    // Send GSMTC Play command
}

void MediaService::Pause() {
    // Send GSMTC Pause command
}

void MediaService::Next() {
    // Send GSMTC Next command
}

void MediaService::Previous() {
    // Send GSMTC Previous command
}

} // namespace Services
} // namespace DynamicIsland
