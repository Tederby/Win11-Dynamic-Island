#include "common/defs.h"
#include "common/log.h"
#include "common/utils.h"
#include "platform/taskbar.h"
#include "graphics/animation.h"
#include "graphics/d2d_renderer.h"
#include "graphics/icons.h"
#include "overlay/layout.h"
#include "overlay/island_window.h"
#include "services/service_manager.h"
#include "services/media_service.h"
#include "services/audio_service.h"
#include "services/power_service.h"
#include "services/keyboard_service.h"
#include "services/bluetooth_service.h"
#include "services/timer_service.h"

// Include module implementations for single compilation unit bundling
#include "common/utils.cpp"
#include "platform/taskbar.cpp"
#include "graphics/animation.cpp"
#include "graphics/d2d_renderer.cpp"
#include "overlay/layout.cpp"
#include "overlay/island_window.cpp"
#include "services/service_manager.cpp"
#include "services/media_service.cpp"
#include "services/audio_service.cpp"
#include "services/power_service.cpp"
#include "services/keyboard_service.cpp"
#include "services/bluetooth_service.cpp"
#include "services/timer_service.cpp"

namespace DynamicIsland {

ModSettings g_settings;

static std::unique_ptr<Overlay::IslandWindow> g_islandWindow;
static std::unique_ptr<Services::ServiceManager> g_serviceManager;
static std::unique_ptr<Services::MediaService> g_mediaService;
static std::unique_ptr<Services::AudioService> g_audioService;
static std::unique_ptr<Services::PowerService> g_powerService;
static std::unique_ptr<Services::KeyboardService> g_keyboardService;
static std::unique_ptr<Services::BluetoothService> g_bluetoothService;
static std::unique_ptr<Services::TimerService> g_timerService;

static UINT_PTR g_pollTimerId = 2001;
static HWND g_timerWindow = nullptr;

static LRESULT CALLBACK ModTimerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TIMER && wParam == g_pollTimerId) {
        if (g_powerService) g_powerService->Poll();
        if (g_keyboardService) g_keyboardService->Poll();
        if (g_serviceManager) g_serviceManager->Update();
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void LoadSettings() {
    PCWSTR placementStr = Wh_GetStringSetting(L"placementMode");
    if (placementStr) {
        if (wcscmp(placementStr, L"top_embed") == 0) {
            g_settings.placement = PlacementMode::TopEmbed;
        } else if (wcscmp(placementStr, L"floating") == 0) {
            g_settings.placement = PlacementMode::Floating;
        } else {
            g_settings.placement = PlacementMode::Auto;
        }
        Wh_FreeStringSetting(placementStr);
    }

    g_settings.enableMedia = Wh_GetIntSetting(L"enableMedia") != 0;
    g_settings.enableTimer = Wh_GetIntSetting(L"enableTimer") != 0;
    g_settings.enableMicStatus = Wh_GetIntSetting(L"enableMicStatus") != 0;
    g_settings.enableVolumeHUD = Wh_GetIntSetting(L"enableVolumeHUD") != 0;
    g_settings.enableCapsLockHUD = Wh_GetIntSetting(L"enableCapsLockHUD") != 0;
    g_settings.enablePowerHUD = Wh_GetIntSetting(L"enablePowerHUD") != 0;
    g_settings.enableBluetoothHUD = Wh_GetIntSetting(L"enableBluetoothHUD") != 0;
    g_settings.autoCollapseSeconds = Wh_GetIntSetting(L"autoCollapseSeconds");
    if (g_settings.autoCollapseSeconds <= 0) {
        g_settings.autoCollapseSeconds = 5;
    }

    LogInfo(L"Settings loaded successfully");
}

} // namespace DynamicIsland

// ============================================================================
// Windhawk Mod Lifecycle Hooks
// ============================================================================

BOOL Wh_ModInit() {
    DynamicIsland::LogInfo(L"Initializing Win11 Dynamic Island Mod...");

    DynamicIsland::LoadSettings();

    // 1. Create overlay window
    DynamicIsland::g_islandWindow = std::make_unique<DynamicIsland::Overlay::IslandWindow>();
    if (!DynamicIsland::g_islandWindow->Create()) {
        DynamicIsland::LogError(L"Failed to create Dynamic Island overlay window");
        return FALSE;
    }

    // 2. Initialize service orchestrator
    DynamicIsland::g_serviceManager = std::make_unique<DynamicIsland::Services::ServiceManager>(
        DynamicIsland::g_islandWindow.get()
    );
    DynamicIsland::g_serviceManager->Initialize();

    // 3. Initialize feature services
    DynamicIsland::g_mediaService = std::make_unique<DynamicIsland::Services::MediaService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_audioService = std::make_unique<DynamicIsland::Services::AudioService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_powerService = std::make_unique<DynamicIsland::Services::PowerService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_keyboardService = std::make_unique<DynamicIsland::Services::KeyboardService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_bluetoothService = std::make_unique<DynamicIsland::Services::BluetoothService>(
        DynamicIsland::g_serviceManager.get()
    );
    DynamicIsland::g_timerService = std::make_unique<DynamicIsland::Services::TimerService>(
        DynamicIsland::g_serviceManager.get()
    );

    // Start services
    if (DynamicIsland::g_settings.enableMedia) DynamicIsland::g_mediaService->Start();
    if (DynamicIsland::g_settings.enablePowerHUD) DynamicIsland::g_powerService->Start();
    if (DynamicIsland::g_settings.enableCapsLockHUD) DynamicIsland::g_keyboardService->Start();
    if (DynamicIsland::g_settings.enableBluetoothHUD) DynamicIsland::g_bluetoothService->Start();

    // Setup polling timer window
    WNDCLASSEXW twc{};
    twc.cbSize = sizeof(WNDCLASSEXW);
    twc.lpfnWndProc = DynamicIsland::ModTimerWndProc;
    twc.hInstance = GetModuleHandleW(nullptr);
    twc.lpszClassName = L"Win11DynamicIslandTimerClass";
    RegisterClassExW(&twc);

    DynamicIsland::g_timerWindow = CreateWindowExW(
        0, L"Win11DynamicIslandTimerClass", nullptr, 0,
        0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr
    );

    if (DynamicIsland::g_timerWindow) {
        SetTimer(DynamicIsland::g_timerWindow, DynamicIsland::g_pollTimerId, 250, nullptr);
    }

    DynamicIsland::g_islandWindow->Show(true);
    DynamicIsland::LogInfo(L"Win11 Dynamic Island Mod initialized successfully");
    return TRUE;
}

void Wh_ModUninit() {
    DynamicIsland::LogInfo(L"Unloading Win11 Dynamic Island Mod...");

    if (DynamicIsland::g_timerWindow) {
        KillTimer(DynamicIsland::g_timerWindow, DynamicIsland::g_pollTimerId);
        DestroyWindow(DynamicIsland::g_timerWindow);
        DynamicIsland::g_timerWindow = nullptr;
        UnregisterClassW(L"Win11DynamicIslandTimerClass", GetModuleHandleW(nullptr));
    }

    DynamicIsland::g_timerService.reset();
    DynamicIsland::g_bluetoothService.reset();
    DynamicIsland::g_keyboardService.reset();
    DynamicIsland::g_powerService.reset();
    DynamicIsland::g_audioService.reset();
    DynamicIsland::g_mediaService.reset();
    DynamicIsland::g_serviceManager.reset();

    if (DynamicIsland::g_islandWindow) {
        DynamicIsland::g_islandWindow->Destroy();
        DynamicIsland::g_islandWindow.reset();
    }

    DynamicIsland::LogInfo(L"Win11 Dynamic Island Mod unloaded");
}

void Wh_ModSettingsChanged() {
    DynamicIsland::LogInfo(L"Settings updated");
    DynamicIsland::LoadSettings();
}
