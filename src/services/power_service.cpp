#include "power_service.h"
#include "service_manager.h"
#include "../common/log.h"
#include "../common/defs.h"

namespace DynamicIsland {
namespace Services {

PowerService::PowerService(ServiceManager* manager)
    : m_manager(manager) {}

PowerService::~PowerService() {
    Stop();
}

void PowerService::Start() {
    m_isRunning = true;
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        m_batteryPercent = sps.BatteryLifePercent;
        m_isACConnected = (sps.ACLineStatus == 1);
        m_wasACConnected = m_isACConnected;
    }
    LogInfo(L"PowerService started (AC=%d, Battery=%d%%)", m_isACConnected, m_batteryPercent);
}

void PowerService::Stop() {
    m_isRunning = false;
}

void PowerService::Poll() {
    if (!m_isRunning || !m_manager) return;

    SYSTEM_POWER_STATUS sps;
    if (!GetSystemPowerStatus(&sps)) return;

    bool currentAC = (sps.ACLineStatus == 1);
    BYTE currentPercent = sps.BatteryLifePercent;

    // AC connect / disconnect event
    if (currentAC != m_wasACConnected) {
        m_wasACConnected = currentAC;
        m_isACConnected = currentAC;

        if (g_settings.enablePowerHUD && currentAC) {
            QueuedEvent qe;
            qe.type = EventType::Power;
            qe.durationMs = DURATION_POWER_MS;
            qe.title = L"Charging";
            qe.progressPercent = currentPercent;
            m_manager->PostTransientEvent(qe);
        }
    }

    // Low battery trigger (<20%)
    if (!currentAC && currentPercent <= 20 && !m_lowBatteryAlerted) {
        m_lowBatteryAlerted = true;
        if (g_settings.enablePowerHUD) {
            QueuedEvent qe;
            qe.type = EventType::LowBattery;
            qe.durationMs = DURATION_LOW_BATT_MS;
            qe.title = L"Low Battery";
            qe.progressPercent = currentPercent;
            m_manager->PostTransientEvent(qe);
        }
    } else if (currentPercent > 25) {
        m_lowBatteryAlerted = false;
    }

    m_batteryPercent = currentPercent;
}

} // namespace Services
} // namespace DynamicIsland
