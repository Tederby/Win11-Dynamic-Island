#pragma once

#include <windows.h>

namespace DynamicIsland {
namespace Services {

class ServiceManager;

class PowerService {
public:
    explicit PowerService(ServiceManager* manager);
    ~PowerService();

    void Start();
    void Stop();
    void Poll();

    BYTE GetBatteryPercent() const { return m_batteryPercent; }
    bool IsACConnected() const { return m_isACConnected; }

private:
    ServiceManager* m_manager = nullptr;
    BYTE m_batteryPercent = 100;
    bool m_isACConnected = false;
    bool m_wasACConnected = false;
    bool m_lowBatteryAlerted = false;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland
