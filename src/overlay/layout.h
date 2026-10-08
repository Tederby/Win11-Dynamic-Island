#pragma once

#include <windows.h>
#include "../common/defs.h"
#include "../platform/taskbar.h"

namespace DynamicIsland {
namespace Overlay {

struct IslandMetrics {
    float width = 0.0f;
    float height = 32.0f;
    float cornerRadius = 16.0f;
    float posX = 0.0f;
    float posY = 0.0f;
    bool isEmbedded = false;
};

class LayoutEngine {
public:
    static IslandMetrics CalculateMetrics(
        EventType eventType,
        IslandState state,
        const Platform::TaskbarInfo& tbInfo,
        UINT customWidth = 0
    );
};

} // namespace Overlay
} // namespace DynamicIsland
