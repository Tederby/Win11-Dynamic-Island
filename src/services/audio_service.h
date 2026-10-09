#pragma once

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <wrl/client.h>

namespace DynamicIsland {
namespace Services {

using Microsoft::WRL::ComPtr;

class ServiceManager;

class AudioService {
public:
    explicit AudioService(ServiceManager* manager);
    ~AudioService();

    void Start();
    void Stop();
    void CheckMicUsage();
    void UpdateLoopback();

    float GetCurrentVolume() const { return m_currentVolume; }
    bool IsMuted() const { return m_isMuted; }
    bool IsMicInUse() const { return m_isMicInUse; }
    float GetCurrentAmplitude() const { return m_smoothedRms; }

private:
    [[maybe_unused]] ServiceManager* m_manager = nullptr;
    float m_currentVolume = 0.5f;
    bool m_isMuted = false;
    bool m_isMicInUse = false;
    bool m_isRunning = false;

    // WASAPI Loopback capture state
    ComPtr<IMMDeviceEnumerator> m_deviceEnumerator;
    ComPtr<IAudioClient> m_audioClient;
    ComPtr<IAudioCaptureClient> m_captureClient;
    WAVEFORMATEX* m_pwfx = nullptr;
    bool m_isLoopbackActive = false;
    float m_currentRms = 0.0f;
    float m_smoothedRms = 0.0f;
};

} // namespace Services
} // namespace DynamicIsland
