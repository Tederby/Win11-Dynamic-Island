#pragma once

#include <windows.h>

namespace DynamicIsland {
namespace Services {

class ServiceManager;

class KeyboardService {
public:
    explicit KeyboardService(ServiceManager* manager);
    ~KeyboardService();

    void Start();
    void Stop();
    void Poll();

    bool IsCapsLockOn() const { return m_capsLockState; }

private:
    ServiceManager* m_manager = nullptr;
    bool m_capsLockState = false;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland
