#include "layout.h"
#include "../common/utils.h"

namespace DynamicIsland {
namespace Overlay {

IslandMetrics LayoutEngine::CalculateMetrics(
    EventType eventType,
    IslandState state,
    const Platform::TaskbarInfo& tbInfo,
    UINT customWidth
) {
    IslandMetrics m{};
    UINT dpi = tbInfo.dpi;

    int tbHeight = tbInfo.taskbarRect.bottom - tbInfo.taskbarRect.top;
    if (tbHeight <= 0) tbHeight = 34;

    int compactH = tbHeight - 6;
    if (compactH < 26) compactH = 26;

    m.isEmbedded = (tbInfo.position == TaskbarPosition::Top);

    if (state == IslandState::Hidden) {
        m.width = 0.0f;
        m.height = static_cast<float>(Utils::ScaleDpi(compactH, dpi));
        m.cornerRadius = m.height / 2.0f;
    } else if (state == IslandState::Compact) {
        float defaultWidth = 196.0f;
        switch (eventType) {
            case EventType::Media:      defaultWidth = 196.0f; break;
            case EventType::Timer:      defaultWidth = 176.0f; break;
            case EventType::MicStatus:  defaultWidth = 176.0f; break;
            case EventType::Volume:     defaultWidth = 206.0f; break;
            case EventType::CapsLock:   defaultWidth = 160.0f; break;
            case EventType::Power:      defaultWidth = 196.0f; break;
            case EventType::Bluetooth:  defaultWidth = 244.0f; break;
            case EventType::LowBattery: defaultWidth = 196.0f; break;
            case EventType::TimerDone:  defaultWidth = 204.0f; break;
            default:                    defaultWidth = 180.0f; break;
        }

        if (customWidth > 0) defaultWidth = static_cast<float>(customWidth);

        m.width = Utils::ScaleDpiF(defaultWidth, dpi);
        m.height = Utils::ScaleDpiF(static_cast<float>(compactH), dpi);
        m.cornerRadius = m.height / 2.0f;
    } else if (state == IslandState::Expanded) {
        float expWidth = 340.0f;
        float expHeight = 150.0f;

        switch (eventType) {
            case EventType::Media:
                expWidth = 340.0f;
                expHeight = 150.0f;
                break;
            case EventType::Timer:
                expWidth = 300.0f;
                expHeight = 124.0f;
                break;
            case EventType::MicStatus:
                expWidth = 280.0f;
                expHeight = 78.0f;
                break;
            default:
                expWidth = 280.0f;
                expHeight = 90.0f;
                break;
        }

        m.width = Utils::ScaleDpiF(expWidth, dpi);
        m.height = Utils::ScaleDpiF(expHeight, dpi);
        m.cornerRadius = Utils::ScaleDpiF(26.0f, dpi);
    }

    // Horizontal centering
    int screenWidth = tbInfo.monitorRect.right - tbInfo.monitorRect.left;
    int centerX = tbInfo.monitorRect.left + screenWidth / 2;

    if (tbInfo.position == TaskbarPosition::Left || tbInfo.position == TaskbarPosition::Right) {
        centerX = (tbInfo.workAreaRect.left + tbInfo.workAreaRect.right) / 2;
    }

    m.posX = static_cast<float>(centerX) - (m.width / 2.0f);

    // Vertical placement
    if (m.isEmbedded) {
        int topAnchor = (tbInfo.taskbarRect.top >= tbInfo.monitorRect.top)
            ? tbInfo.taskbarRect.top
            : tbInfo.monitorRect.top;

        if (state == IslandState::Expanded) {
            m.posY = static_cast<float>(topAnchor + 3);
        } else {
            m.posY = static_cast<float>(topAnchor + (tbHeight - compactH) / 2);
        }
    } else {
        int bottomAnchor = tbInfo.workAreaRect.bottom;
        if (tbInfo.position == TaskbarPosition::Bottom && !tbInfo.isAutoHide) {
            bottomAnchor = tbInfo.taskbarRect.top - 10;
        } else {
            bottomAnchor = tbInfo.monitorRect.bottom - 14;
        }

        m.posY = static_cast<float>(bottomAnchor) - m.height;
    }

    return m;
}

} // namespace Overlay
} // namespace DynamicIsland
