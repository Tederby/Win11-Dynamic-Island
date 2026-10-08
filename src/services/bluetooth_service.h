#pragma once

#include <windows.h>
#include <string>

namespace DynamicIsland {
namespace Services {

class ServiceManager;

struct BluetoothDeviceInfo {
    std::wstring name;
    int batteryLevel = -1; // -1 if not available
    bool isConnected = false;
};

class BluetoothService {
public:
    explicit BluetoothService(ServiceManager* manager);
    ~BluetoothService();

    void Start();
    void Stop();
    void Poll();

private:
    ServiceManager* m_manager = nullptr;
    bool m_isRunning = false;
};

} // namespace Services
} // namespace DynamicIsland
