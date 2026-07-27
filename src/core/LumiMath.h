// LUMI//DANCER — small maths toolbox shared by analysis, rig and dance code.
// Header-only, JUCE-free, deterministic.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace lumi
{
inline constexpr float kPi     = 3.14159265358979323846f;
inline constexpr float kTwoPi  = 6.28318530717958647692f;
inline constexpr float kHalfPi = 1.57079632679489661923f;

template <typename T>
inline T clamp (T v, T lo, T hi) { return std::min (std::max (v, lo), hi); }

inline float clamp01 (float v) { return clamp (v, 0.0f, 1.0f); }

inline float lerp (float a, float b, float t) { return a + (b - a) * t; }

inline double lerpd (double a, double b, double t) { return a + (b - a) * t; }

// Hermite smoothstep, clamped to [0,1].
inline float smoothstep (float t)
{
    t = clamp01 (t);
    return t * t * (3.0f - 2.0f * t);
}

// Cubic ease in/out, clamped to [0,1].
inline float easeInOutCubic (float t)
{
    t = clamp01 (t);
    if (t < 0.5f)
        return 4.0f * t * t * t;
    const float u = -2.0f * t + 2.0f;
    return 1.0f - u * u * u * 0.5f;
}

inline float easeOutCubic (float t)
{
    t = clamp01 (t);
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}

inline float easeOutBack (float t)
{
    t = clamp01 (t);
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

// Wrap an angle into (-pi, pi].
inline float wrapAngle (float a)
{
    a = std::fmod (a + kPi, kTwoPi);
    if (a < 0.0f)
        a += kTwoPi;
    return a - kPi;
}

// Interpolate between two angles along the shortest arc.
inline float lerpAngle (float a, float b, float t)
{
    return a + wrapAngle (b - a) * t;
}

// Fractional part in [0,1) that behaves for negative inputs.
inline double fract (double v)
{
    return v - std::floor (v);
}

inline float dbToGain (float db)      { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float gain)    { return 20.0f * std::log10 (std::max (gain, 1.0e-12f)); }

// Replace NaN/inf with a fallback so bad audio can never poison animation.
inline float sanitize (float v, float fallback = 0.0f)
{
    return std::isfinite (v) ? v : fallback;
}

// One-pole smoothing coefficient for a given time constant.
inline float onePoleCoeff (float timeSeconds, float updateRateHz)
{
    if (timeSeconds <= 0.0f || updateRateHz <= 0.0f)
        return 0.0f; // no smoothing: output follows input immediately
    return std::exp (-1.0f / (timeSeconds * updateRateHz));
}

// Critically damped spring integrator (stable for any dt >= 0).
// omega is the natural angular frequency: higher = snappier.
struct Spring
{
    float value    = 0.0f;
    float velocity = 0.0f;

    void snapTo (float target)
    {
        value = target;
        velocity = 0.0f;
    }

    float update (float target, float dt, float omega)
    {
        // Closed-form critically damped spring:
        //   x(t) = (x0 + (v0 + w*x0) t) e^{-w t}
        //   v(t) = (v0 - (v0 + w*x0) w t) e^{-w t}
        dt = std::max (dt, 0.0f);
        const float x    = value - target;
        const float damp = std::exp (-omega * dt);
        const float k    = velocity + omega * x;
        value    = target + (x + k * dt) * damp;
        velocity = (velocity - k * omega * dt) * damp;
        if (! std::isfinite (value))    snapTo (target);
        if (! std::isfinite (velocity)) velocity = 0.0f;
        return value;
    }
};

// 2D point used by the rig (kept trivially copyable and JUCE-free).
struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    Vec2 operator+ (const Vec2& o) const { return { x + o.x, y + o.y }; }
    Vec2 operator- (const Vec2& o) const { return { x - o.x, y - o.y }; }
    Vec2 operator* (float s)       const { return { x * s, y * s }; }
};

inline Vec2 lerp (const Vec2& a, const Vec2& b, float t)
{
    return { lerp (a.x, b.x, t), lerp (a.y, b.y, t) };
}
} // namespace lumi
