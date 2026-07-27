// Maths toolbox + deterministic RNG tests.
#include "TestFramework.h"

#include "core/LumiMath.h"
#include "core/SeededRng.h"

using namespace lumi;

LD_TEST (math_clamp_lerp_smoothstep)
{
    LD_EQ (clamp (5, 0, 3), 3);
    LD_EQ (clamp (-1.0f, 0.0f, 3.0f), 0.0f);
    LD_NEAR (lerp (0.0f, 10.0f, 0.25f), 2.5f, 1e-6);
    LD_NEAR (smoothstep (0.0f), 0.0f, 1e-6);
    LD_NEAR (smoothstep (1.0f), 1.0f, 1e-6);
    LD_NEAR (smoothstep (0.5f), 0.5f, 1e-6);
    LD_CHECK (smoothstep (0.25f) < 0.25f);   // ease-in below midpoint
    LD_NEAR (easeInOutCubic (0.0f), 0.0f, 1e-6);
    LD_NEAR (easeInOutCubic (1.0f), 1.0f, 1e-6);
    LD_NEAR (easeOutCubic (1.0f), 1.0f, 1e-6);
}

LD_TEST (math_angle_wrapping)
{
    LD_NEAR (wrapAngle (0.0f), 0.0f, 1e-6);
    LD_NEAR (wrapAngle (kTwoPi), 0.0f, 1e-5);
    LD_NEAR (wrapAngle (kPi + 0.1f), -kPi + 0.1f, 1e-5);

    // Shortest-arc interpolation: from +170° to -170° must pass through 180°,
    // not swing back through 0°.
    const float a = 170.0f * kPi / 180.0f;
    const float b = -170.0f * kPi / 180.0f;
    const float mid = lerpAngle (a, b, 0.5f);
    LD_CHECK (std::fabs (mid) > 175.0f * kPi / 180.0f);
}

LD_TEST (math_fract_negative_safe)
{
    LD_NEAR (fract (3.25), 0.25, 1e-9);
    LD_NEAR (fract (-0.25), 0.75, 1e-9);
    LD_NEAR (fract (-4.0), 0.0, 1e-9);
}

LD_TEST (math_sanitize_rejects_nan_inf)
{
    LD_EQ (sanitize (std::numeric_limits<float>::quiet_NaN(), 7.0f), 7.0f);
    LD_EQ (sanitize (std::numeric_limits<float>::infinity(), 7.0f), 7.0f);
    LD_EQ (sanitize (1.5f, 7.0f), 1.5f);
}

LD_TEST (math_spring_converges_without_overshoot_blowup)
{
    Spring s;
    s.snapTo (0.0f);
    float maxVal = 0.0f;
    for (int i = 0; i < 600; ++i)   // 10 s at 60 fps
    {
        s.update (1.0f, 1.0f / 60.0f, 12.0f);
        maxVal = std::max (maxVal, s.value);
    }
    LD_NEAR (s.value, 1.0f, 1e-2);
    LD_LT (maxVal, 1.15f);           // critically damped: minimal overshoot

    // Huge dt must not explode.
    s.update (0.0f, 10.0f, 12.0f);
    LD_CHECK (std::isfinite (s.value));
}

LD_TEST (rng_deterministic_and_uniform)
{
    SeededRng a (1234), b (1234), c (999);

    bool identical = true;
    bool different = false;
    for (int i = 0; i < 64; ++i)
    {
        const uint64_t va = a.nextU64();
        if (va != b.nextU64()) identical = false;
        if (va != c.nextU64()) different = true;
    }
    LD_CHECK (identical);
    LD_CHECK (different);

    SeededRng r (42);
    float mn = 1.0f, mx = 0.0f;
    double sum = 0.0;
    constexpr int n = 10000;
    for (int i = 0; i < n; ++i)
    {
        const float v = r.nextFloat01();
        mn = std::min (mn, v);
        mx = std::max (mx, v);
        sum += v;
    }
    LD_GE (mn, 0.0f);
    LD_LT (mx, 1.0f);
    LD_NEAR (sum / n, 0.5, 0.02);

    // nextInt stays in range and is not constant.
    SeededRng ri (7);
    bool inRange = true, varied = false;
    int first = ri.nextInt (10);
    for (int i = 0; i < 200; ++i)
    {
        const int v = ri.nextInt (10);
        if (v < 0 || v >= 10) inRange = false;
        if (v != first) varied = true;
    }
    LD_CHECK (inRange);
    LD_CHECK (varied);
    LD_EQ (ri.nextInt (0), 0);
}
