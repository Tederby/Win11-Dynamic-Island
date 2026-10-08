#pragma once

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <wrl/client.h>

namespace DynamicIsland {
namespace Services {

class ServiceManager;

class AudioService {
public:
    explicit AudioService(ServiceManager* manager);
    ~AudioService();

    void Start();
    void Stop();
    void CheckMicUsage();

    float GetCurrentVolume() const { return m_currentVolume; }
    bool IsMuted() const { return m_isMuted; }
    bool IsMicInUse() const { return m_isMicInUse; }

private:
    ServiceManager* m_manager = nullptr;
    float m_currentVolume = 0.5f;
    bool m_isMuted = false;
    bool m_isMicInUse = false;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland
