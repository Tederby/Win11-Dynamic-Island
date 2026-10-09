#include "audio_service.h"
#include "service_manager.h"
#include "../common/log.h"
#include <vector>
#include <cstdint>
#include <string>

#include <cmath>

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
    if (m_isLoopbackActive) {
        if (m_audioClient) {
            m_audioClient->Stop();
        }
        m_captureClient.Reset();
        m_audioClient.Reset();
        m_deviceEnumerator.Reset();
        if (m_pwfx) {
            CoTaskMemFree(m_pwfx);
            m_pwfx = nullptr;
        }
        m_isLoopbackActive = false;
    }
}

void AudioService::UpdateLoopback() {
    if (!m_isRunning) return;

    if (!m_isLoopbackActive) {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&m_deviceEnumerator));
        if (FAILED(hr)) return;

        ComPtr<IMMDevice> defaultDevice;
        hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
        if (FAILED(hr)) return;

        hr = defaultDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(m_audioClient.GetAddressOf()));
        if (FAILED(hr)) return;

        hr = m_audioClient->GetMixFormat(&m_pwfx);
        if (FAILED(hr) || !m_pwfx) return;

        hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 1000000, 0, m_pwfx, nullptr);
        if (FAILED(hr)) return;

        hr = m_audioClient->GetService(IID_PPV_ARGS(&m_captureClient));
        if (FAILED(hr)) return;

        hr = m_audioClient->Start();
        if (FAILED(hr)) return;

        m_isLoopbackActive = true;
    }

    if (m_isLoopbackActive && m_captureClient) {
        UINT32 packetLength = 0;
        float maxRms = 0.0f;
        while (SUCCEEDED(m_captureClient->GetNextPacketSize(&packetLength)) && packetLength > 0) {
            BYTE* pData = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;
            if (SUCCEEDED(m_captureClient->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr))) {
                if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && pData && numFrames > 0 && m_pwfx) {
                    float sum = 0.0f;
                    UINT totalSamples = numFrames * m_pwfx->nChannels;
                    if (m_pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                        (m_pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && m_pwfx->wBitsPerSample == 32)) {
                        const float* samples = reinterpret_cast<const float*>(pData);
                        for (UINT i = 0; i < totalSamples; ++i) {
                            float s = samples[i];
                            sum += s * s;
                        }
                        float mean = sum / static_cast<float>(totalSamples);
                        float rms = std::sqrt(mean) * 4.2f;
                        if (rms > maxRms) maxRms = rms;
                    }
                }
                m_captureClient->ReleaseBuffer(numFrames);
            }
        }

        if (maxRms > 1.0f) maxRms = 1.0f;
        m_currentRms = maxRms;

        // Attack (0.75) and Release (0.82) envelope filter for punchy responsive bounce
        if (m_currentRms > m_smoothedRms) {
            m_smoothedRms = m_smoothedRms + 0.75f * (m_currentRms - m_smoothedRms);
        } else {
            m_smoothedRms = m_smoothedRms * 0.82f;
        }
    }
}

void AudioService::CheckMicUsage() {
    if (!m_isRunning || !g_settings.enableMicStatus) return;

    static const wchar_t* REG_PATHS[] = {
        L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone\\NonPackaged",
        L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone"
    };

    bool currentlyInUse = false;
    std::wstring activeAppName = L"";

    for (const auto* regPath : REG_PATHS) {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD subKeyCount = 0;
            DWORD maxSubKeyLen = 0;
            if (RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, &subKeyCount, &maxSubKeyLen, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                std::vector<wchar_t> keyNameBuf(maxSubKeyLen + 2);
                for (DWORD i = 0; i < subKeyCount; ++i) {
                    DWORD keyNameLen = static_cast<DWORD>(keyNameBuf.size());
                    if (RegEnumKeyExW(hKey, i, keyNameBuf.data(), &keyNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                        HKEY hSubKey = nullptr;
                        if (RegOpenKeyExW(hKey, keyNameBuf.data(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                            uint64_t stopTime = 0;
                            DWORD dataSize = sizeof(stopTime);
                            DWORD type = 0;
                            if (RegQueryValueExW(hSubKey, L"LastUsedTimeStop", nullptr, &type, reinterpret_cast<LPBYTE>(&stopTime), &dataSize) == ERROR_SUCCESS) {
                                if (stopTime == 0) {
                                    currentlyInUse = true;
                                    std::wstring nameStr(keyNameBuf.data(), keyNameLen);
                                    size_t lastSep = nameStr.find_last_of(L"#/\\");
                                    if (lastSep != std::wstring::npos && lastSep + 1 < nameStr.length()) {
                                        activeAppName = nameStr.substr(lastSep + 1);
                                    } else {
                                        activeAppName = nameStr;
                                    }
                                    size_t exePos = activeAppName.find(L".exe");
                                    if (exePos != std::wstring::npos) {
                                        activeAppName = activeAppName.substr(0, exePos);
                                    }
                                    size_t dotPos = activeAppName.find(L'.');
                                    size_t underPos = activeAppName.rfind(L'_');
                                    if (dotPos != std::wstring::npos && underPos != std::wstring::npos && underPos > dotPos) {
                                        activeAppName = activeAppName.substr(dotPos + 1, underPos - dotPos - 1);
                                    }
                                }
                            }
                            RegCloseKey(hSubKey);
                            if (currentlyInUse) break;
                        }
                    }
                }
            }
            RegCloseKey(hKey);
            if (currentlyInUse) break;
        }
    }

    if (currentlyInUse && !m_isMicInUse) {
        m_isMicInUse = true;
        if (m_manager) {
            QueuedEvent qe;
            qe.type = EventType::MicStatus;
            qe.durationMs = 2800; // 2.8s transient HUD
            qe.title = L"Mikrofon aktif";
            qe.subtitle = activeAppName.empty() ? L"Aplikasi sistem" : activeAppName;
            m_manager->PostTransientEvent(qe);
        }
    } else if (!currentlyInUse && m_isMicInUse) {
        m_isMicInUse = false;
    }
}

} // namespace Services
} // namespace DynamicIsland
