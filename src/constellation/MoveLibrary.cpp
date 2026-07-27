// LUMI//DANCER — the factory move library and per-move pose evaluation.
#include "constellation/DanceStar.h"

#include <cmath>

#include "core/LumiMath.h"
#include "dance/PoseHelpers.h"

namespace lumi
{
using namespace posehelpers;

namespace
{
uint64_t raw (MoveId m) { return uint64_t (m); }

std::vector<DanceStar> buildLibrary()
{
    std::vector<DanceStar> stars;
    const auto add = [&stars] (MoveId id, const char* name, ConstellationGroup group,
                               float energy, float cuteness, float speed, float complexity,
                               std::vector<uint64_t> next, float vx, float vy)
    {
        DanceStar s;
        s.id = raw (id);
        s.name = name;
        s.group = group;
        s.energy = energy;
        s.cuteness = cuteness;
        s.speed = speed;
        s.complexity = complexity;
        s.compatibleNextMoves = std::move (next);
        s.viewX = vx;
        s.viewY = vy;
        stars.push_back (std::move (s));
    };

    // Idle constellation — calm anchors; every idle move can reach the others
    // plus one gateway into each neighbouring constellation.
    add (MoveId::IdleBounce, "Bounce",    ConstellationGroup::Idle,   0.30f, 0.60f, 0.40f, 0.15f,
         { raw (MoveId::HeadNod), raw (MoveId::SideStep), raw (MoveId::HandWave),
           raw (MoveId::DoubleStep), raw (MoveId::Sway) },                       0.18f, 0.25f);
    add (MoveId::HeadNod,   "Head Nod",   ConstellationGroup::Idle,   0.25f, 0.50f, 0.45f, 0.10f,
         { raw (MoveId::IdleBounce), raw (MoveId::SideStep), raw (MoveId::Sway) }, 0.10f, 0.42f);
    add (MoveId::SideStep,  "Side Step",  ConstellationGroup::Idle,   0.35f, 0.65f, 0.50f, 0.25f,
         { raw (MoveId::IdleBounce), raw (MoveId::HandWave), raw (MoveId::DoubleStep),
           raw (MoveId::Jump) },                                                  0.26f, 0.40f);
    add (MoveId::HandWave,  "Hand Wave",  ConstellationGroup::Idle,   0.30f, 0.85f, 0.45f, 0.20f,
         { raw (MoveId::IdleBounce), raw (MoveId::SideStep), raw (MoveId::StarBurst) }, 0.18f, 0.55f);

    // Energy constellation — high-motion moves.
    add (MoveId::Jump,       "Jump",        ConstellationGroup::Energy, 0.85f, 0.55f, 0.75f, 0.45f,
         { raw (MoveId::Spin), raw (MoveId::DoubleStep), raw (MoveId::StarBurst),
           raw (MoveId::IdleBounce) },                                            0.62f, 0.20f);
    add (MoveId::Spin,       "Spin",        ConstellationGroup::Energy, 0.80f, 0.60f, 0.80f, 0.70f,
         { raw (MoveId::Jump), raw (MoveId::StarBurst), raw (MoveId::OrbitArms) }, 0.74f, 0.32f);
    add (MoveId::DoubleStep, "Double Step", ConstellationGroup::Energy, 0.75f, 0.55f, 0.90f, 0.55f,
         { raw (MoveId::Jump), raw (MoveId::Spin), raw (MoveId::SideStep) },       0.58f, 0.38f);
    add (MoveId::StarBurst,  "Star Burst",  ConstellationGroup::Energy, 0.95f, 0.80f, 0.70f, 0.65f,
         { raw (MoveId::Jump), raw (MoveId::HandWave), raw (MoveId::TranceRise) }, 0.70f, 0.14f);

    // Flow constellation — smooth, floating moves.
    add (MoveId::Sway,       "Sway",        ConstellationGroup::Flow,   0.25f, 0.55f, 0.30f, 0.15f,
         { raw (MoveId::OrbitArms), raw (MoveId::Float), raw (MoveId::HeadNod) },  0.38f, 0.72f);
    add (MoveId::OrbitArms,  "Orbit Arms",  ConstellationGroup::Flow,   0.45f, 0.65f, 0.50f, 0.50f,
         { raw (MoveId::Sway), raw (MoveId::Float), raw (MoveId::TranceRise),
           raw (MoveId::Spin) },                                                  0.52f, 0.66f);
    add (MoveId::Float,      "Float",       ConstellationGroup::Flow,   0.35f, 0.70f, 0.35f, 0.40f,
         { raw (MoveId::Sway), raw (MoveId::OrbitArms), raw (MoveId::TranceRise) }, 0.46f, 0.82f);
    add (MoveId::TranceRise, "Trance Rise", ConstellationGroup::Flow,   0.60f, 0.60f, 0.45f, 0.60f,
         { raw (MoveId::OrbitArms), raw (MoveId::Float), raw (MoveId::StarBurst) }, 0.60f, 0.76f);

    return stars;
}
} // namespace

const std::vector<DanceStar>& moveLibrary()
{
    static const std::vector<DanceStar> library = buildLibrary();
    return library;
}

const DanceStar* findStar (uint64_t id)
{
    for (const auto& star : moveLibrary())
        if (star.id == id)
            return &star;
    return nullptr;
}

CharacterPose evaluateMove (uint64_t id, double localBeat,
                            const AudioReactiveFrame& audio, float intensity)
{
    const float m = clamp (intensity, 0.0f, 2.0f) * (0.5f + 0.5f * clamp01 (audio.rms * 2.5f));
    const float kickAcc = clamp01 (audio.lowTransient);
    CharacterPose p = neutralPose();

    switch (MoveId (id))
    {
        case MoveId::IdleBounce:
            addBounce (p, fract (localBeat), (0.05f + 0.04f * kickAcc) * m);
            addArmSwing (p, localBeat, 0.2f * m, 2.0f);
            break;

        case MoveId::HeadNod:
            addHeadNod (p, localBeat, 0.5f * m, 1.0f);
            addBounce (p, fract (localBeat), 0.02f * m);
            break;

        case MoveId::SideStep:
            addSideStep (p, localBeat, 0.10f * m, 2.0f);
            addBounce (p, fract (localBeat), 0.025f * m);
            break;

        case MoveId::HandWave:
            p.rightUpperArm.rotationRadians += -1.8f * m;
            p.rightLowerArm.rotationRadians += 0.5f * m * std::sin (float (localBeat) * kTwoPi * 2.0f);
            addBounce (p, fract (localBeat), 0.02f * m);
            p.mouthSmileAmount = 0.8f;
            break;

        case MoveId::Jump:
        {
            // Anticipate in the first half beat, leap on the beat.
            const float phase = float (fract (localBeat));
            const float air = std::max (0.0f, std::sin (phase * kPi));
            p.root.position.y += (-0.12f * air - 0.03f * (1.0f - air)) * m;
            p.leftUpperLeg.rotationRadians += 0.5f * m * air;
            p.rightUpperLeg.rotationRadians -= 0.5f * m * air;
            addHandsUp (p, 0.5f * m * air);
            break;
        }

        case MoveId::Spin:
        {
            // Reads as a spin through fast body-facing wobble + skirt flare.
            const float ang = float (fract (localBeat / 2.0)) * kTwoPi;
            p.torso.rotationRadians += 0.25f * m * std::sin (ang * 2.0f);
            p.root.position.x += 0.06f * m * std::cos (ang);
            p.head.rotationRadians += 0.2f * m * std::sin (ang * 2.0f + 0.5f);
            p.leftUpperArm.rotationRadians += 1.2f * m;
            p.rightUpperArm.rotationRadians -= 1.2f * m;
            p.hairBounceAmount += 0.4f * m;
            break;
        }

        case MoveId::DoubleStep:
            addSideStep (p, localBeat * 2.0, 0.08f * m, 2.0f);
            addBounce (p, fract (localBeat * 2.0), 0.04f * m);
            addArmSwing (p, localBeat * 2.0, 0.25f * m, 1.0f);
            break;

        case MoveId::StarBurst:
        {
            // Explosive open pose on the beat, sparkly.
            const float burst = 1.0f - smoothstep (float (fract (localBeat)));
            addHandsUp (p, (0.4f + 0.6f * burst) * m);
            p.root.position.y += -0.05f * m * burst;
            p.starEyeAmount = clamp01 (0.5f + 0.5f * burst);
            p.mouthOpenAmount = 0.4f * burst;
            p.torso.scale += 0.04f * burst;
            break;
        }

        case MoveId::Sway:
            p.root.position.x += 0.05f * m * std::sin (float (fract (localBeat / 4.0)) * kTwoPi);
            p.head.rotationRadians += 0.1f * m * std::sin (float (fract (localBeat / 4.0)) * kTwoPi);
            p.torso.rotationRadians += 0.06f * m * std::sin (float (fract (localBeat / 4.0)) * kTwoPi + 0.4f);
            break;

        case MoveId::OrbitArms:
        {
            const float ang = float (fract (localBeat / 2.0)) * kTwoPi;
            p.leftUpperArm.rotationRadians = ang;
            p.rightUpperArm.rotationRadians = wrapAngle (ang + kPi);
            p.root.position.y += -0.02f * m;
            break;
        }

        case MoveId::Float:
            p.root.position.y += (-0.04f - 0.02f * std::sin (float (fract (localBeat / 2.0)) * kTwoPi)) * m;
            p.leftUpperArm.rotationRadians += 0.6f * m;
            p.rightUpperArm.rotationRadians -= 0.6f * m;
            p.leftLowerLeg.rotationRadians += 0.3f * m;
            p.eyeOpenAmount = 0.85f;
            break;

        case MoveId::TranceRise:
        {
            const float rise = easeInOutCubic (float (fract (localBeat / 4.0)));
            p.root.position.y += -0.06f * m * rise;
            p.leftUpperArm.rotationRadians += (1.2f + 0.6f * rise) * m;
            p.rightUpperArm.rotationRadians -= (1.2f + 0.6f * rise) * m;
            p.starEyeAmount = 0.4f * rise;
            break;
        }

        default:
            addBounce (p, fract (localBeat), 0.04f * m);
            break;
    }

    sanitizePose (p);
    return p;
}
} // namespace lumi
