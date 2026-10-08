#include "animation.h"
#include "../common/utils.h"

namespace DynamicIsland {
namespace Graphics {

// Solves cubic bezier X to find T, then returns Y
float EvaluateCubicBezier(float x1, float y1, float x2, float y2, float x) {
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;

    // Approximate parameter t using Newton-Raphson iterations
    float t = x;
    for (int i = 0; i < 8; ++i) {
        float oneMinusT = 1.0f - t;
        float currentX = 3.0f * oneMinusT * oneMinusT * t * x1 +
                         3.0f * oneMinusT * t * t * x2 +
                         t * t * t;

        float derivativeX = 3.0f * oneMinusT * oneMinusT * x1 +
                            6.0f * oneMinusT * t * (x2 - x1) +
                            3.0f * t * t * (1.0f - x2);

        if (std::abs(derivativeX) < 1e-5f) break;
        t -= (currentX - x) / derivativeX;
        t = Utils::Clamp(t, 0.0f, 1.0f);
    }

    // Calculate Y from T
    float oneMinusT = 1.0f - t;
    return 3.0f * oneMinusT * oneMinusT * t * y1 +
           3.0f * oneMinusT * t * t * y2 +
           t * t * t;
}

void AnimatedValue::SetTarget(float target, DWORD durationMs) {
    if (std::abs(m_target - target) < 0.001f && !m_isAnimating) {
        return;
    }
    m_start = m_current;
    m_target = target;
    m_durationMs = (durationMs > 0) ? durationMs : 1;
    m_startTime = GetTickCount();
    m_isAnimating = true;
}

void AnimatedValue::SnapTo(float value) {
    m_current = value;
    m_start = value;
    m_target = value;
    m_isAnimating = false;
}

void AnimatedValue::Update(DWORD currentTimeMs) {
    if (!m_isAnimating) return;

    if (currentTimeMs >= m_startTime + m_durationMs) {
        m_current = m_target;
        m_isAnimating = false;
        return;
    }

    float linearProgress = static_cast<float>(currentTimeMs - m_startTime) / static_cast<float>(m_durationMs);
    // Cubic bezier used in CSS prototype: cubic-bezier(.34, 1.3, .5, 1)
    float curvedProgress = EvaluateCubicBezier(0.34f, 1.3f, 0.5f, 1.0f, linearProgress);
    m_current = m_start + (m_target - m_start) * curvedProgress;
}

} // namespace Graphics
} // namespace DynamicIsland
