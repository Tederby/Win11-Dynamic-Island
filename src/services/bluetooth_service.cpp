#include "bluetooth_service.h"
#include "service_manager.h"
#include "../common/log.h"
#include "../common/defs.h"

namespace DynamicIsland {
namespace Services {

BluetoothService::BluetoothService(ServiceManager* manager)
    : m_manager(manager) {}

BluetoothService::~BluetoothService() {
    Stop();
}

void BluetoothService::Start() {
    m_isRunning = true;
    LogInfo(L"BluetoothService started");
}

void BluetoothService::Stop() {
    m_isRunning = false;
}

void BluetoothService::Poll() {
    if (!m_isRunning || !m_manager) return;
    // Enumerates connected Bluetooth audio devices via Windows.Devices.Bluetooth / SetupAPI
}

} // namespace Services
} // namespace DynamicIsland
