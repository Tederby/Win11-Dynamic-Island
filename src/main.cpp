#include "common/defs.h"
#include "common/log.h"
#include "common/utils.h"
#include "platform/taskbar.h"
#include "graphics/animation.h"
#include "graphics/icons.h"
#include "graphics/d2d_renderer.h"
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
#include "graphics/icons.cpp"
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
static DWORD g_lastTimerTick = 0;

static HANDLE g_uiThread = nullptr;
static DWORD g_uiThreadId = 0;
static HANDLE g_uiReadyEvent = nullptr;

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

    PCWSTR idleVisStr = Wh_GetStringSetting(L"idleVisibilityMode");
    if (idleVisStr) {
        if (wcscmp(idleVisStr, L"events_only") == 0) {
            g_settings.idleVisibilityMode = IdleVisibilityMode::EventsOnly;
        } else {
            g_settings.idleVisibilityMode = IdleVisibilityMode::AlwaysVisible;
        }
        Wh_FreeStringSetting(idleVisStr);
    }

    PCWSTR mediaVisStr = Wh_GetStringSetting(L"mediaVisibilityPolicy");
    if (mediaVisStr) {
        if (wcscmp(mediaVisStr, L"track_change_only") == 0) {
            g_settings.mediaVisibilityPolicy = MediaVisibilityPolicy::TrackChangeOnly;
        } else {
            g_settings.mediaVisibilityPolicy = MediaVisibilityPolicy::AlwaysVisible;
        }
        Wh_FreeStringSetting(mediaVisStr);
    }

    g_settings.enableMedia = Wh_GetIntSetting(L"enableMedia") != 0;
    g_settings.enableTimer = Wh_GetIntSetting(L"enableTimer") != 0;
    g_settings.enableMicStatus = Wh_GetIntSetting(L"enableMicStatus") != 0;
    g_settings.enableVolumeHUD = Wh_GetIntSetting(L"enableVolumeHUD") != 0;
    g_settings.enableCapsLockHUD = Wh_GetIntSetting(L"enableCapsLockHUD") != 0;
    g_settings.enablePowerHUD = Wh_GetIntSetting(L"enablePowerHUD") != 0;
    g_settings.enableBluetoothHUD = Wh_GetIntSetting(L"enableBluetoothHUD") != 0;
    g_settings.enableDebugHotkeys = Wh_GetIntSetting(L"enableDebugHotkeys") != 0;
    g_settings.autoCollapseDelayMs = Wh_GetIntSetting(L"autoCollapseDelayMs");
    if (g_settings.autoCollapseDelayMs <= 0) {
        int legacySec = Wh_GetIntSetting(L"autoCollapseSeconds");
        if (legacySec > 0) {
            g_settings.autoCollapseDelayMs = legacySec * 1000;
        } else {
            g_settings.autoCollapseDelayMs = 1500;
        }
    }

    LogInfo(L"Settings loaded successfully");
}

static DWORD WINAPI IslandUIThreadProc(LPVOID) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // 1. Create overlay window on this dedicated UI thread
    g_islandWindow = std::make_unique<Overlay::IslandWindow>();
    if (!g_islandWindow->Create()) {
        LogError(L"Failed to create Dynamic Island overlay window");
        if (g_uiReadyEvent) SetEvent(g_uiReadyEvent);
        CoUninitialize();
        return 0;
    }

    // 2. Initialize service orchestrator
    g_serviceManager = std::make_unique<Services::ServiceManager>(g_islandWindow.get());
    g_serviceManager->Initialize();

    // 3. Initialize feature services
    g_mediaService = std::make_unique<Services::MediaService>(g_serviceManager.get());
    g_audioService = std::make_unique<Services::AudioService>(g_serviceManager.get());
    g_powerService = std::make_unique<Services::PowerService>(g_serviceManager.get());
    g_keyboardService = std::make_unique<Services::KeyboardService>(g_serviceManager.get());
    g_bluetoothService = std::make_unique<Services::BluetoothService>(g_serviceManager.get());
    g_timerService = std::make_unique<Services::TimerService>(g_serviceManager.get());

    g_serviceManager->RegisterMediaService(g_mediaService.get());
    g_serviceManager->RegisterTimerService(g_timerService.get());

    // Start background monitors
    if (g_settings.enableMedia) g_mediaService->Start();
    if (g_settings.enableMicStatus || g_settings.enableVolumeHUD) g_audioService->Start();
    if (g_settings.enablePowerHUD) g_powerService->Start();
    if (g_settings.enableCapsLockHUD) g_keyboardService->Start();
    if (g_settings.enableBluetoothHUD) g_bluetoothService->Start();

    // Set polling timer on the island window
    SetTimer(g_islandWindow->GetHwnd(), g_pollTimerId, 200, nullptr);

    if (g_settings.idleVisibilityMode == IdleVisibilityMode::AlwaysVisible ||
        g_serviceManager->ResolveCurrentLiveActivity() != EventType::None) {
        g_islandWindow->Show(true);
    } else {
        g_islandWindow->Show(false);
    }

    // Signal initialization ready
    if (g_uiReadyEvent) SetEvent(g_uiReadyEvent);

    // Standard Win32 Message Pump for the UI thread
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_TIMER && msg.wParam == g_pollTimerId) {
            if (g_powerService) g_powerService->Poll();
            if (g_keyboardService) g_keyboardService->Poll();
            if (g_mediaService) g_mediaService->Poll();
            if (g_audioService) {
                g_audioService->CheckMicUsage();
                if (g_mediaService && g_mediaService->GetState().isPlaying) {
                    g_audioService->UpdateLoopback();
                }
            }

            DWORD now = GetTickCount();
            if (now - g_lastTimerTick >= 1000) {
                g_lastTimerTick = now;
                if (g_timerService) g_timerService->Tick();
            }

            if (g_serviceManager) g_serviceManager->Update();
            continue;
        }

        if (msg.message == WM_USER + 101) {
            if (g_serviceManager) g_serviceManager->RefreshVisibility();
            if (g_islandWindow) {
                if (g_settings.enableDebugHotkeys) {
                    g_islandWindow->RegisterHotkeys();
                } else {
                    g_islandWindow->UnregisterHotkeys();
                }
                g_islandWindow->UpdateDimensions();
            }
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Teardown
    if (g_islandWindow && g_islandWindow->GetHwnd()) {
        KillTimer(g_islandWindow->GetHwnd(), g_pollTimerId);
    }

    g_timerService.reset();
    g_bluetoothService.reset();
    g_keyboardService.reset();
    g_powerService.reset();
    g_audioService.reset();
    g_mediaService.reset();
    g_serviceManager.reset();

    if (g_islandWindow) {
        g_islandWindow->Destroy();
        g_islandWindow.reset();
    }

    CoUninitialize();
    return 0;
}

} // namespace DynamicIsland

namespace DynamicIsland {

static void InitHighPrecisionTimer() {
    HMODULE hWinmm = GetModuleHandleW(L"winmm.dll");
    if (!hWinmm) hWinmm = LoadLibraryW(L"winmm.dll");
    if (hWinmm) {
        using PFN_timeBeginPeriod = UINT(WINAPI*)(UINT);
        auto pfn = reinterpret_cast<PFN_timeBeginPeriod>(GetProcAddress(hWinmm, "timeBeginPeriod"));
        if (pfn) pfn(1);
    }
}

static void CleanupHighPrecisionTimer() {
    HMODULE hWinmm = GetModuleHandleW(L"winmm.dll");
    if (hWinmm) {
        using PFN_timeEndPeriod = UINT(WINAPI*)(UINT);
        auto pfn = reinterpret_cast<PFN_timeEndPeriod>(GetProcAddress(hWinmm, "timeEndPeriod"));
        if (pfn) pfn(1);
    }
}

} // namespace DynamicIsland

// ============================================================================
// Windhawk Mod Lifecycle Hooks
// ============================================================================

BOOL Wh_ModInit() {
    DynamicIsland::LogInfo(L"Initializing Win11 Dynamic Island Mod...");

    DynamicIsland::InitHighPrecisionTimer();
    DynamicIsland::LoadSettings();

    DynamicIsland::g_uiReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    DynamicIsland::g_uiThread = CreateThread(
        nullptr,
        0,
        DynamicIsland::IslandUIThreadProc,
        nullptr,
        0,
        &DynamicIsland::g_uiThreadId
    );

    if (!DynamicIsland::g_uiThread) {
        DynamicIsland::LogError(L"Failed to create Dynamic Island UI thread");
        if (DynamicIsland::g_uiReadyEvent) {
            CloseHandle(DynamicIsland::g_uiReadyEvent);
            DynamicIsland::g_uiReadyEvent = nullptr;
        }
        return FALSE;
    }

    // Wait until UI window is created and ready
    WaitForSingleObject(DynamicIsland::g_uiReadyEvent, 3000);
    CloseHandle(DynamicIsland::g_uiReadyEvent);
    DynamicIsland::g_uiReadyEvent = nullptr;

    DynamicIsland::LogInfo(L"Win11 Dynamic Island Mod initialized successfully");
    return TRUE;
}

void Wh_ModUninit() {
    DynamicIsland::LogInfo(L"Unloading Win11 Dynamic Island Mod...");

    if (DynamicIsland::g_uiThread) {
        PostThreadMessageW(DynamicIsland::g_uiThreadId, WM_QUIT, 0, 0);
        WaitForSingleObject(DynamicIsland::g_uiThread, 2000);
        CloseHandle(DynamicIsland::g_uiThread);
        DynamicIsland::g_uiThread = nullptr;
    }

    DynamicIsland::CleanupHighPrecisionTimer();
    DynamicIsland::LogInfo(L"Win11 Dynamic Island Mod unloaded");
}

void Wh_ModSettingsChanged() {
    DynamicIsland::LogInfo(L"Settings updated");
    DynamicIsland::LoadSettings();
    if (DynamicIsland::g_uiThreadId != 0) {
        PostThreadMessageW(DynamicIsland::g_uiThreadId, WM_USER + 101, 0, 0);
    }
}
