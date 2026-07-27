// Expression engine + idle behaviour tests.
#include "TestFramework.h"

#include "dance/ExpressionSystem.h"
#include "dance/IdleBehavior.h"

using namespace lumi;

namespace
{
AudioReactiveFrame loudAudio()
{
    AudioReactiveFrame a;
    a.rms = 0.5f;
    a.lowEnergy = 0.8f;
    a.midEnergy = 0.6f;
    a.highEnergy = 0.5f;
    a.transientProbability = 0.4f;
    a.silence = false;
    return a;
}

AudioReactiveFrame quietAudio()
{
    AudioReactiveFrame a;
    a.silence = true;
    return a;
}
} // namespace

LD_TEST (expressions_blink_happens_and_recovers)
{
    ExpressionEngine engine (5);
    engine.reset (5);

    bool sawClosed = false, sawOpenAfterClosed = false;
    for (int i = 0; i < 600; ++i)   // 10 s at 60 fps
    {
        const ExpressionState s = engine.update (1.0f / 60.0f, loudAudio(),
                                                 ExpressionTheme::Soft, 0.0f);
        if (s.eyeOpenAmount < 0.2f)
            sawClosed = true;
        else if (sawClosed && s.eyeOpenAmount > 0.8f)
            sawOpenAfterClosed = true;
    }
    LD_CHECK (sawClosed);
    LD_CHECK (sawOpenAfterClosed);
}

LD_TEST (expressions_energy_raises_excitement)
{
    ExpressionEngine engine (5);
    engine.reset (5);
    for (int i = 0; i < 400; ++i)
        engine.update (1.0f / 60.0f, loudAudio(), ExpressionTheme::Soft, 0.0f);

    const Expression e = engine.current();
    LD_CHECK (e == Expression::Excited || e == Expression::Happy
              || e == Expression::Surprised || e == Expression::Hyper);

    // Sustained energy also lights the star eyes.
    const ExpressionState s = engine.update (1.0f / 60.0f, loudAudio(),
                                             ExpressionTheme::Soft, 0.0f);
    LD_GT (s.starEyeAmount, 0.2f);
}

LD_TEST (expressions_long_silence_turns_sleepy)
{
    ExpressionEngine engine (5);
    engine.reset (5);
    for (int i = 0; i < 400; ++i)
        engine.update (1.0f / 60.0f, quietAudio(), ExpressionTheme::Soft, 60.0f);
    LD_CHECK (engine.current() == Expression::Sleepy);
}

LD_TEST (expressions_mood_biases_selection)
{
    ExpressionEngine cheerful (5), sleepy (5);
    cheerful.reset (5);
    sleepy.reset (5);

    AudioReactiveFrame medium;
    medium.rms = 0.1f;
    medium.silence = false;

    for (int i = 0; i < 400; ++i)
    {
        cheerful.update (1.0f / 60.0f, medium, ExpressionTheme::Cheerful, 0.0f);
        sleepy.update (1.0f / 60.0f, medium, ExpressionTheme::Sleepy, 0.0f);
    }
    LD_CHECK (cheerful.current() != Expression::Sleepy);
    LD_CHECK (sleepy.current() == Expression::Sleepy);
}

LD_TEST (expressions_deterministic_with_seed)
{
    ExpressionEngine a (7), b (7);
    a.reset (7);
    b.reset (7);
    float diff = 0.0f;
    for (int i = 0; i < 600; ++i)
    {
        const ExpressionState sa = a.update (1.0f / 60.0f, loudAudio(), ExpressionTheme::Soft, 0.0f);
        const ExpressionState sb = b.update (1.0f / 60.0f, loudAudio(), ExpressionTheme::Soft, 0.0f);
        diff += std::fabs (sa.eyeOpenAmount - sb.eyeOpenAmount)
              + std::fabs (sa.starEyeAmount - sb.starEyeAmount);
    }
    LD_NEAR (diff, 0.0f, 1e-6);
}

LD_TEST (idle_breathing_is_subtle_and_finite)
{
    IdleBehavior idle (3);
    idle.reset (3);
    for (int i = 0; i < 120; ++i)
    {
        const CharacterPose p = idle.update (1.0f / 60.0f, float (i) / 60.0f, 0.7f, 180.0f);
        LD_CHECK (std::isfinite (p.root.position.y));
        if (i == 60)
            LD_LT (std::fabs (p.root.position.y), 0.05f);
    }
    LD_CHECK (idle.currentState() == IdleState::Breathing
              || idle.currentState() == IdleState::LookAround
              || idle.currentState() == IdleState::Wave
              || idle.currentState() == IdleState::Stretch
              || idle.currentState() == IdleState::CheckStar);
}

LD_TEST (idle_gestures_eventually_occur)
{
    IdleBehavior idle (3);
    idle.reset (3);
    bool sawGesture = false;
    for (int i = 0; i < 60 * 40; ++i)   // 40 s
    {
        idle.update (1.0f / 60.0f, float (i) / 60.0f, 0.7f, 500.0f);
        const IdleState s = idle.currentState();
        if (s == IdleState::LookAround || s == IdleState::Wave
            || s == IdleState::Stretch || s == IdleState::CheckStar)
            sawGesture = true;
    }
    LD_CHECK (sawGesture);
}

LD_TEST (idle_sits_then_sleeps)
{
    IdleBehavior idle (3);
    idle.reset (3);

    // 70 s idle → sitting.
    for (int i = 0; i < 60 * 5; ++i)
        idle.update (1.0f / 60.0f, 70.0f, 0.7f, 120.0f);
    LD_CHECK (idle.currentState() == IdleState::Sitting);

    // Past the sleep delay → sleeping with closed eyes.
    CharacterPose p;
    for (int i = 0; i < 60 * 5; ++i)
        p = idle.update (1.0f / 60.0f, 130.0f, 0.7f, 120.0f);
    LD_CHECK (idle.currentState() == IdleState::Sleeping);
    LD_LT (p.eyeOpenAmount, 0.15f);

    // Waking up: idle time resets to a small value → back to breathing.
    idle.update (1.0f / 60.0f, 0.5f, 0.7f, 120.0f);
    LD_CHECK (idle.currentState() == IdleState::Breathing);
}
