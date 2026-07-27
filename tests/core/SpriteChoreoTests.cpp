// Painted-sprite choreography tests: valid frames, determinism, per-style
// sequence distinctness, idle mapping, accent behaviour.
#include "TestFramework.h"

#include <vector>

#include "dance/SpriteChoreo.h"

using namespace lumi;

namespace
{
AudioReactiveFrame midAudio()
{
    AudioReactiveFrame a;
    a.rms = 0.3f;
    a.lowEnergy = 0.5f;
    a.midEnergy = 0.4f;
    a.silence = false;
    return a;
}

std::vector<int> sampleFrames (DanceStyle style, uint64_t seed, int quarterBeats = 64)
{
    std::vector<int> frames;
    const AudioReactiveFrame audio = midAudio();
    for (int i = 0; i < quarterBeats; ++i)
        frames.push_back (int (spriteFrameForDance (style, double (i) * 0.25, audio, seed)));
    return frames;
}
} // namespace

LD_TEST (sprites_frames_always_valid)
{
    const AudioReactiveFrame audio = midAudio();
    for (int s = 0; s < int (DanceStyle::Count); ++s)
        for (int i = 0; i < 128; ++i)
        {
            const int f = int (spriteFrameForDance (DanceStyle (s), double (i) * 0.31, audio, 7));
            LD_GE (f, 0);
            LD_LT (f, int (SpriteFrame::Count));
        }
    for (int idle = 0; idle <= int (IdleState::Sleeping); ++idle)
    {
        const int f = int (spriteFrameForIdle (IdleState (idle)));
        LD_GE (f, 0);
        LD_LT (f, int (SpriteFrame::Count));
    }
}

LD_TEST (sprites_deterministic_with_seed)
{
    for (const DanceStyle style : { DanceStyle::Breakcore, DanceStyle::Freestyle,
                                    DanceStyle::Bounce })
    {
        const auto a = sampleFrames (style, 42);
        const auto b = sampleFrames (style, 42);
        LD_CHECK (a == b);
    }

    // Different seeds vary the stochastic styles.
    LD_CHECK (sampleFrames (DanceStyle::Breakcore, 1) != sampleFrames (DanceStyle::Breakcore, 2));
    LD_CHECK (sampleFrames (DanceStyle::Freestyle, 1) != sampleFrames (DanceStyle::Freestyle, 2));
}

LD_TEST (sprites_styles_have_distinct_sequences)
{
    std::vector<std::vector<int>> sequences;
    for (int s = 0; s < int (DanceStyle::Count); ++s)
        sequences.push_back (sampleFrames (DanceStyle (s), 7));

    for (size_t a = 0; a < sequences.size(); ++a)
        for (size_t b = a + 1; b < sequences.size(); ++b)
        {
            int differing = 0;
            for (size_t i = 0; i < sequences[a].size(); ++i)
                if (sequences[a][i] != sequences[b][i])
                    ++differing;
            _ctx.report (differing >= int (sequences[a].size() / 5),
                         (std::string ("sprite sequences differ: ")
                          + danceStyleName (DanceStyle (int (a))) + " vs "
                          + danceStyleName (DanceStyle (int (b)))).c_str(),
                         __FILE__, __LINE__);
        }
}

LD_TEST (sprites_cool_frame_on_the_drop)
{
    AudioReactiveFrame drop = midAudio();
    drop.lowEnergy = 0.95f;
    drop.rms = 0.7f;
    LD_CHECK (spriteFrameForDance (DanceStyle::Bounce, 3.0, drop, 7) == SpriteFrame::Cool);
    LD_CHECK (spriteFrameForDance (DanceStyle::Trance, 9.5, drop, 7) == SpriteFrame::Cool);
}

LD_TEST (sprites_idle_mapping)
{
    LD_CHECK (spriteFrameForIdle (IdleState::Sitting) == SpriteFrame::Sitting);
    LD_CHECK (spriteFrameForIdle (IdleState::Sleeping) == SpriteFrame::Sitting);
    LD_CHECK (spriteFrameForIdle (IdleState::Breathing) == SpriteFrame::Shy);
    LD_CHECK (spriteFrameForIdle (IdleState::Wave) == SpriteFrame::Cheer);
}

LD_TEST (sprites_state_rides_the_pose)
{
    CharacterPose pose;
    pose.root.position = { 0.1f, -0.2f };
    pose.torso.rotationRadians = 0.3f;
    pose.torso.scale = 1.1f;

    const SpriteState dancing = evaluateSpriteState (DanceStyle::Bounce, 4.0, midAudio(),
                                                     7, pose, 1.0f, IdleState::Breathing);
    LD_NEAR (dancing.offsetX, 0.1f, 1e-6);
    LD_NEAR (dancing.offsetY, -0.2f, 1e-6);
    LD_GT (dancing.rotation, 0.0f);
    LD_LE (std::fabs (dancing.rotation), 0.35f);
    LD_NEAR (dancing.squash, 1.1f, 1e-3);

    // Idle activity uses the idle frame set.
    const SpriteState idle = evaluateSpriteState (DanceStyle::Bounce, 4.0, midAudio(),
                                                  7, pose, 0.0f, IdleState::Sitting);
    LD_CHECK (idle.frame == SpriteFrame::Sitting);

    // Mirroring alternates over beats while dancing (variety), never on the
    // asymmetric frames.
    bool sawMirrored = false, sawUnmirrored = false;
    for (int beat = 0; beat < 16; ++beat)
    {
        const SpriteState s = evaluateSpriteState (DanceStyle::Trance, double (beat),
                                                   midAudio(), 7, pose, 1.0f,
                                                   IdleState::Breathing);
        (s.mirrored ? sawMirrored : sawUnmirrored) = true;
    }
    LD_CHECK (sawMirrored);
    LD_CHECK (sawUnmirrored);
}
