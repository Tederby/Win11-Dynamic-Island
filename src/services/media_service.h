#pragma once

#include <windows.h>
#include <string>
#include "../common/defs.h"

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
};

} // namespace Services
} // namespace DynamicIsland
