#pragma once

#include <windows.h>
#include <cmath>

namespace DynamicIsland {
namespace Graphics {

// Evaluates cubic-bezier(x1, y1, x2, y2) for parameter t in [0, 1]
// Standard spring curve uses cubic-bezier(0.34, 1.3, 0.5, 1.0)
float EvaluateCubicBezier(float x1, float y1, float x2, float y2, float t);

// High-precision QPC timestamp in milliseconds
double GetHighPrecisionTimeMs();

// Spring animation property animator
class AnimatedValue {
public:
    AnimatedValue(float initialValue = 0.0f)
        : m_current(initialValue), m_start(initialValue), m_target(initialValue),
          m_startTime(0.0), m_durationMs(450.0), m_isAnimating(false) {}

    void SetTarget(float target, double durationMs = 450.0);
    void SnapTo(float value);
    void Update(double currentTimeMs);

    float GetValue() const { return m_current; }
    bool IsAnimating() const { return m_isAnimating; }

private:
    float m_current;
    float m_start;
    float m_target;
    double m_startTime;
    double m_durationMs;
    bool m_isAnimating;
};

} // namespace Graphics
} // namespace DynamicIsland
