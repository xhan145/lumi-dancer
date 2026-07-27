#include "dance/SpriteChoreo.h"

#include <cmath>

#include "core/LumiMath.h"
#include "dance/PoseHelpers.h"

namespace lumi
{
const char* spriteFrameName (SpriteFrame f)
{
    switch (f)
    {
        case SpriteFrame::Stand:     return "Stand";
        case SpriteFrame::Cheer:     return "Cheer";
        case SpriteFrame::Clasp:     return "Clasp";
        case SpriteFrame::Excited:   return "Excited";
        case SpriteFrame::StepUp:    return "Step Up";
        case SpriteFrame::Shy:       return "Shy";
        case SpriteFrame::Wink:      return "Wink";
        case SpriteFrame::Heart:     return "Heart";
        case SpriteFrame::Paws:      return "Paws";
        case SpriteFrame::Kick:      return "Kick";
        case SpriteFrame::PawsSmall: return "Paws Small";
        case SpriteFrame::WinkPose:  return "Wink Pose";
        case SpriteFrame::Lean:      return "Lean";
        case SpriteFrame::JumpKick:  return "Jump Kick";
        case SpriteFrame::Cool:      return "Cool";
        case SpriteFrame::Sitting:   return "Sitting";
        default:                     return "Unknown";
    }
}

namespace
{
// Per-style beat sequences. Grid unit is one beat unless noted; a table of
// N frames repeats every N grid steps. Sequences are the painted analogue
// of each style's vector choreography signature.
struct StyleSequence
{
    const SpriteFrame* frames;
    int length;
    double gridBeats;        // beats per frame step (0.25 = 16ths, 2 = half-time)
};

constexpr SpriteFrame seqBounce[]    = { SpriteFrame::Stand, SpriteFrame::Clasp,
                                         SpriteFrame::Stand, SpriteFrame::Clasp };
constexpr SpriteFrame seqKawaii[]    = { SpriteFrame::Cheer, SpriteFrame::Heart,
                                         SpriteFrame::Wink, SpriteFrame::Paws };
constexpr SpriteFrame seqOrbit[]     = { SpriteFrame::Clasp, SpriteFrame::StepUp,
                                         SpriteFrame::Heart, SpriteFrame::StepUp };
constexpr SpriteFrame seqGroove[]    = { SpriteFrame::Lean, SpriteFrame::Shy,
                                         SpriteFrame::Lean, SpriteFrame::WinkPose };
constexpr SpriteFrame seqHyper[]     = { SpriteFrame::Cheer, SpriteFrame::Excited,
                                         SpriteFrame::Kick, SpriteFrame::StepUp,
                                         SpriteFrame::JumpKick, SpriteFrame::Excited,
                                         SpriteFrame::Cheer, SpriteFrame::Kick };
constexpr SpriteFrame seqChill[]     = { SpriteFrame::Shy, SpriteFrame::Stand };
constexpr SpriteFrame seqDnb[]       = { SpriteFrame::StepUp, SpriteFrame::Kick,
                                         SpriteFrame::StepUp, SpriteFrame::JumpKick };
constexpr SpriteFrame seqTrance[]    = { SpriteFrame::Clasp, SpriteFrame::StepUp,
                                         SpriteFrame::Cheer, SpriteFrame::Heart };

// Breakcore/Freestyle pick pseudo-randomly from a pool per grid step.
constexpr SpriteFrame poolBreakcore[] = { SpriteFrame::Kick, SpriteFrame::Excited,
                                          SpriteFrame::JumpKick, SpriteFrame::Cool,
                                          SpriteFrame::StepUp, SpriteFrame::Cheer };
constexpr SpriteFrame poolFreestyle[] = { SpriteFrame::Cheer, SpriteFrame::Heart,
                                          SpriteFrame::Clasp, SpriteFrame::Lean,
                                          SpriteFrame::Wink, SpriteFrame::StepUp,
                                          SpriteFrame::Paws, SpriteFrame::WinkPose };

StyleSequence sequenceFor (DanceStyle style)
{
    switch (style)
    {
        case DanceStyle::KawaiiPop:   return { seqKawaii, 4, 1.0 };
        case DanceStyle::Orbit:       return { seqOrbit, 4, 1.0 };
        case DanceStyle::Groove:      return { seqGroove, 4, 1.0 };
        case DanceStyle::Hyper:       return { seqHyper, 8, 0.5 };
        case DanceStyle::Chill:       return { seqChill, 2, 2.0 };
        case DanceStyle::DrumAndBass: return { seqDnb, 4, 0.5 };
        case DanceStyle::Trance:      return { seqTrance, 4, 2.0 };
        case DanceStyle::Bounce:
        default:                      return { seqBounce, 4, 1.0 };
    }
}
} // namespace

SpriteFrame spriteFrameForDance (DanceStyle style, double beat,
                                 const AudioReactiveFrame& audio, uint64_t seed)
{
    beat = std::max (0.0, beat);

    // Sunglasses moment: sustained high energy = the drop. Held for the bar.
    if (audio.lowEnergy > 0.85f && audio.rms > 0.5f)
        return SpriteFrame::Cool;

    if (style == DanceStyle::Breakcore)
    {
        const uint64_t step = uint64_t (beat * 4.0);   // 16th-note glitch grid
        const int n = int (std::size (poolBreakcore));
        return poolBreakcore[int (posehelpers::hash01 (step * 2654435761u + seed) * float (n)) % n];
    }

    if (style == DanceStyle::Freestyle)
    {
        const uint64_t step = uint64_t (beat);         // one pick per beat
        const int n = int (std::size (poolFreestyle));
        int pick = int (posehelpers::hash01 (step * 0x9e3779b97f4a7c15ull + seed) * float (n)) % n;
        // Repeat avoidance: nudge when the previous beat hashed identically.
        if (step > 0)
        {
            const int prev = int (posehelpers::hash01 ((step - 1) * 0x9e3779b97f4a7c15ull + seed)
                                  * float (n)) % n;
            if (pick == prev)
                pick = (pick + 1) % n;
        }
        return poolFreestyle[pick];
    }

    const StyleSequence seq = sequenceFor (style);
    const uint64_t step = uint64_t (beat / seq.gridBeats);
    SpriteFrame frame = seq.frames[step % uint64_t (seq.length)];

    // Snare accent: a strong mid transient flashes a wink on the off-frame.
    if (audio.midTransient > 0.75f && frame == SpriteFrame::Stand)
        frame = SpriteFrame::Wink;

    return frame;
}

SpriteFrame spriteFrameForIdle (IdleState idleState)
{
    switch (idleState)
    {
        case IdleState::Sitting:
        case IdleState::Sleeping:   return SpriteFrame::Sitting;
        case IdleState::Wave:       return SpriteFrame::Cheer;
        case IdleState::CheckStar:  return SpriteFrame::Paws;
        case IdleState::Stretch:    return SpriteFrame::Excited;
        case IdleState::LookAround:
        case IdleState::Breathing:
        default:                    return SpriteFrame::Shy;
    }
}

SpriteState evaluateSpriteState (DanceStyle style, double beat,
                                 const AudioReactiveFrame& audio, uint64_t seed,
                                 const CharacterPose& pose, float activityLevel,
                                 IdleState idleState)
{
    SpriteState state;

    state.frame = activityLevel > 0.5f ? spriteFrameForDance (style, beat, audio, seed)
                                       : spriteFrameForIdle (idleState);

    // Motion rides on the rig pose, so smoothing/reduced-motion/idle blending
    // all apply automatically. Painted frames flip every two beats for
    // variety (except the asymmetric-by-design sunglasses/sitting frames).
    state.offsetX = pose.root.position.x;
    state.offsetY = pose.root.position.y;
    state.rotation = clamp (pose.torso.rotationRadians * 0.6f
                                + pose.head.rotationRadians * 0.2f,
                            -0.35f, 0.35f);
    state.squash = clamp (pose.torso.scale, 0.85f, 1.15f);

    if (state.frame != SpriteFrame::Cool && state.frame != SpriteFrame::Sitting)
        state.mirrored = (uint64_t (std::max (0.0, beat) * 0.5) & 1u) != 0
                         && activityLevel > 0.5f;

    return state;
}
} // namespace lumi
