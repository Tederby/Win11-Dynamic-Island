#include "keyboard_service.h"
#include "service_manager.h"
#include "../common/log.h"
#include "../common/defs.h"

namespace DynamicIsland {
namespace Services {

KeyboardService::KeyboardService(ServiceManager* manager)
    : m_manager(manager) {}

KeyboardService::~KeyboardService() {
    Stop();
}

void KeyboardService::Start() {
    m_isRunning = true;
    m_capsLockState = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    LogInfo(L"KeyboardService started (CapsLock=%s)", m_capsLockState ? L"ON" : L"OFF");
}

void KeyboardService::Stop() {
    m_isRunning = false;
}

void KeyboardService::Poll() {
    if (!m_isRunning || !m_manager) return;

    bool currentState = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    if (currentState != m_capsLockState) {
        m_capsLockState = currentState;

        if (g_settings.enableCapsLockHUD) {
            QueuedEvent qe;
            qe.type = EventType::CapsLock;
            qe.durationMs = DURATION_CAPS_LOCK_MS;
            qe.title = L"Caps Lock";
            qe.subtitle = m_capsLockState ? L"ON" : L"OFF";
            m_manager->PostTransientEvent(qe);
        }
    }
}

} // namespace Services
} // namespace DynamicIsland
