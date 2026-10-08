#include "audio_service.h"
#include "service_manager.h"
#include "../common/log.h"

namespace DynamicIsland {
namespace Services {

AudioService::AudioService(ServiceManager* manager)
    : m_manager(manager) {}

AudioService::~AudioService() {
    Stop();
}

void AudioService::Start() {
    m_isRunning = true;
    LogInfo(L"AudioService started");
}

void AudioService::Stop() {
    m_isRunning = false;
}

void AudioService::CheckMicUsage() {
    if (!m_isRunning) return;
    // Registry query or CoreAudio capture stream enumeration
    // HKCU\Software\Microsoft\Windows\CurrentVersion\CapabilityAccessManager\ConsentStore\microphone\NonPackaged
}

} // namespace Services
} // namespace DynamicIsland
