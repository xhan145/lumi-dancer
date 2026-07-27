// Dance style tests: every style is constructible, deterministic, in-bounds,
// audio-reactive, and choreographically distinct from every other style.
#include "TestFramework.h"

#include <vector>

#include "dance/DanceAnimation.h"

using namespace lumi;

namespace
{
AudioReactiveFrame typicalAudio()
{
    AudioReactiveFrame a;
    a.rms = 0.3f;
    a.peak = 0.6f;
    a.lowEnergy = 0.7f;
    a.midEnergy = 0.5f;
    a.highEnergy = 0.4f;
    a.lowTransient = 0.3f;
    a.midTransient = 0.2f;
    a.highTransient = 0.2f;
    a.transientProbability = 0.3f;
    a.spectralCentroid = 0.4f;
    a.silence = false;
    return a;
}

// Sample a style over two bars at 16 samples per beat.
std::vector<CharacterPose> samplePoses (DanceAnimation& anim, float intensity = 1.0f)
{
    std::vector<CharacterPose> poses;
    const AudioReactiveFrame audio = typicalAudio();
    for (int i = 0; i < 128; ++i)
        poses.push_back (anim.evaluate (double (i) / 16.0, 120.0, audio, intensity));
    return poses;
}

// Mean pose-to-pose distance between two sampled sequences.
float sequenceDistance (const std::vector<CharacterPose>& a,
                        const std::vector<CharacterPose>& b)
{
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
        sum += poseDistance (a[i], b[i]);
    return sum / float (a.size());
}

// Total movement (frame-to-frame change) within one sequence.
float sequenceMovement (const std::vector<CharacterPose>& poses)
{
    float sum = 0.0f;
    for (size_t i = 1; i < poses.size(); ++i)
        sum += poseDistance (poses[i - 1], poses[i]);
    return sum;
}
} // namespace

LD_TEST (styles_all_constructible_with_names)
{
    for (int i = 0; i < int (DanceStyle::Count); ++i)
    {
        auto anim = createDanceStyle (DanceStyle (i), 7);
        LD_CHECK (anim != nullptr);
        LD_CHECK (anim->name() != nullptr && anim->name()[0] != '\0');
        LD_CHECK (danceStyleName (DanceStyle (i))[0] != '\0');
    }
}

LD_TEST (styles_deterministic_for_identical_inputs)
{
    for (int i = 0; i < int (DanceStyle::Count); ++i)
    {
        auto animA = createDanceStyle (DanceStyle (i), 42);
        auto animB = createDanceStyle (DanceStyle (i), 42);
        const auto seqA = samplePoses (*animA);
        const auto seqB = samplePoses (*animB);
        LD_NEAR (sequenceDistance (seqA, seqB), 0.0f, 1e-6);
    }
}

LD_TEST (styles_pairwise_distinct_choreography)
{
    // Every pair of styles must differ meaningfully over two bars —
    // renamed-only styles would fail this immediately.
    std::vector<std::vector<CharacterPose>> sequences;
    for (int i = 0; i < int (DanceStyle::Count); ++i)
    {
        auto anim = createDanceStyle (DanceStyle (i), 7);
        sequences.push_back (samplePoses (*anim));
    }

    for (size_t a = 0; a < sequences.size(); ++a)
        for (size_t b = a + 1; b < sequences.size(); ++b)
        {
            const float d = sequenceDistance (sequences[a], sequences[b]);
            _ctx.report (d > 0.05f, (std::string ("styles distinct: ")
                          + danceStyleName (DanceStyle (int (a))) + " vs "
                          + danceStyleName (DanceStyle (int (b)))
                          + " d=" + std::to_string (d)).c_str(),
                         __FILE__, __LINE__);
        }
}

LD_TEST (styles_stay_inside_bounds)
{
    for (int i = 0; i < int (DanceStyle::Count); ++i)
    {
        auto anim = createDanceStyle (DanceStyle (i), 7);
        bool inBounds = true;
        AudioReactiveFrame loud = typicalAudio();
        loud.rms = 1.0f;
        loud.lowTransient = loud.midTransient = loud.highTransient = 1.0f;
        loud.transientProbability = 1.0f;

        for (int step = 0; step < 256; ++step)
        {
            const CharacterPose p = anim->evaluate (double (step) / 16.0, 174.0, loud, 2.0f);
            if (std::fabs (p.root.position.x) > kMaxRootOffset + 1e-5f
                || std::fabs (p.root.position.y) > kMaxRootOffset + 1e-5f
                || ! std::isfinite (p.head.rotationRadians)
                || p.eyeOpenAmount < 0.0f || p.eyeOpenAmount > 1.0f)
                inBounds = false;
        }
        _ctx.report (inBounds, (std::string ("bounds: ") + danceStyleName (DanceStyle (i))).c_str(),
                     __FILE__, __LINE__);
    }
}

LD_TEST (styles_react_to_audio_level)
{
    // Louder audio must produce more movement (RMS → amplitude mapping).
    for (DanceStyle style : { DanceStyle::Bounce, DanceStyle::Groove, DanceStyle::Hyper })
    {
        auto anim = createDanceStyle (style, 7);

        AudioReactiveFrame quiet;
        quiet.rms = 0.02f;
        quiet.silence = false;

        AudioReactiveFrame loud = typicalAudio();
        loud.rms = 0.8f;

        float quietMove = 0.0f, loudMove = 0.0f;
        CharacterPose prevQ = anim->evaluate (0.0, 120.0, quiet, 1.0f);
        CharacterPose prevL = anim->evaluate (0.0, 120.0, loud, 1.0f);
        for (int i = 1; i < 64; ++i)
        {
            const double beat = double (i) / 16.0;
            const CharacterPose q = anim->evaluate (beat, 120.0, quiet, 1.0f);
            const CharacterPose l = anim->evaluate (beat, 120.0, loud, 1.0f);
            quietMove += poseDistance (prevQ, q);
            loudMove += poseDistance (prevL, l);
            prevQ = q;
            prevL = l;
        }
        _ctx.report (loudMove > quietMove * 1.2f,
                     (std::string ("audio reactivity: ") + danceStyleName (style)).c_str(),
                     __FILE__, __LINE__);
    }
}

LD_TEST (styles_zero_intensity_is_nearly_still)
{
    auto anim = createDanceStyle (DanceStyle::Hyper, 7);
    const AudioReactiveFrame audio = typicalAudio();
    float movement = 0.0f;
    CharacterPose prev = anim->evaluate (0.0, 120.0, audio, 0.0f);
    for (int i = 1; i < 64; ++i)
    {
        const CharacterPose p = anim->evaluate (double (i) / 16.0, 120.0, audio, 0.0f);
        movement += poseDistance (prev, p);
        prev = p;
    }
    LD_LT (movement, 1.0f);
}

LD_TEST (style_signatures_bounce_vertical_hyper_double_time)
{
    // Bounce: strong vertical periodicity once per beat.
    auto bounce = createDanceStyle (DanceStyle::Bounce, 7);
    const AudioReactiveFrame audio = typicalAudio();
    const float atBeat  = bounce->evaluate (0.0, 120.0, audio, 1.0f).root.position.y;
    const float atMid   = bounce->evaluate (0.5, 120.0, audio, 1.0f).root.position.y;
    LD_LT (atMid, atBeat - 0.01f);   // dips between beats

    // Chill moves less than Hyper.
    auto chill = createDanceStyle (DanceStyle::Chill, 7);
    auto hyper = createDanceStyle (DanceStyle::Hyper, 7);
    const float chillMove = sequenceMovement (samplePoses (*chill));
    const float hyperMove = sequenceMovement (samplePoses (*hyper));
    LD_GT (hyperMove, chillMove * 1.5f);

    // Breakcore has the most abrupt frame-to-frame changes.
    auto breakcore = createDanceStyle (DanceStyle::Breakcore, 7);
    const auto seq = samplePoses (*breakcore);
    float maxJump = 0.0f;
    for (size_t i = 1; i < seq.size(); ++i)
        maxJump = std::max (maxJump, poseDistance (seq[i - 1], seq[i]));
    LD_GT (maxJump, 0.2f);
}

LD_TEST (style_freestyle_varies_and_repeats_with_seed)
{
    auto freeA = createDanceStyle (DanceStyle::Freestyle, 99);
    auto freeB = createDanceStyle (DanceStyle::Freestyle, 99);
    auto freeC = createDanceStyle (DanceStyle::Freestyle, 100);

    const AudioReactiveFrame audio = typicalAudio();

    // Same seed = identical; different seed = different somewhere over 8 segments.
    float sameDiff = 0.0f, otherDiff = 0.0f;
    for (int i = 0; i < 512; ++i)
    {
        const double beat = double (i) / 16.0;   // 32 beats = 8 segments
        const CharacterPose pa = freeA->evaluate (beat, 120.0, audio, 1.0f);
        const CharacterPose pb = freeB->evaluate (beat, 120.0, audio, 1.0f);
        const CharacterPose pc = freeC->evaluate (beat, 120.0, audio, 1.0f);
        sameDiff += poseDistance (pa, pb);
        otherDiff += poseDistance (pa, pc);
    }
    LD_NEAR (sameDiff, 0.0f, 1e-5);
    LD_GT (otherDiff, 0.5f);
}
