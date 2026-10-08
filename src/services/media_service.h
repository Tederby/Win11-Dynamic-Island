#pragma once

#include <windows.h>
#include <string>

namespace DynamicIsland {
namespace Services {

class ServiceManager;

struct MediaState {
    std::wstring title;
    std::wstring artist;
    bool isPlaying = false;
    float progress = 0.0f;
};

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
};

} // namespace Services
} // namespace DynamicIsland
