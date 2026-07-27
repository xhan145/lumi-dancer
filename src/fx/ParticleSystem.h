// LUMI//DANCER — audio-reactive particles.
//
// A fixed-capacity pool updated on the message thread. Positions live in
// stage coordinates: x in [-1,1], y in [-1,1] with (0,0) at Lumi's chest.
// The system only simulates; the UI layer draws. Never allocates after
// construction.
#pragma once

#include <array>
#include <cstdint>

#include "analysis/AudioReactiveFrame.h"
#include "core/LumiMath.h"
#include "core/SeededRng.h"

namespace lumi
{
enum class ParticleType : uint8_t
{
    Star = 0,      // ambient drifting star
    Sparkle,       // high-transient glitter near the hair
    Heart,         // kawaii burst
    BeatRing,      // expanding ring at the feet on the beat
    Trail,         // lavender motion trail (breakcore)
    Moon,          // rare crescent drift
};

struct Particle
{
    bool alive = false;
    ParticleType type = ParticleType::Star;
    Vec2 pos {};
    Vec2 vel {};
    float life = 0.0f;      // seconds remaining
    float maxLife = 1.0f;
    float size = 0.05f;     // stage units
    float rotation = 0.0f;
    float spin = 0.0f;
    uint8_t variant = 0;    // colour/shape variant for the renderer
};

struct ParticleParams
{
    float amount = 0.6f;          // 0..1 user "Particle Amount"
    bool  enabled = true;
    bool  reducedMotion = false;
    bool  noFlash = false;        // suppress bright beat rings/flashes
};

class ParticleSystem
{
public:
    static constexpr int kCapacity = 256;

    ParticleSystem();

    void reset();

    void update (float dt, const AudioReactiveFrame& audio, double beatPhase,
                 float activityLevel, const ParticleParams& params);

    // Style hooks (e.g. Kawaii Pop hearts, Breakcore trails).
    void emitBurst (ParticleType type, int count, Vec2 origin);

    const std::array<Particle, kCapacity>& particles() const { return pool; }
    int aliveCount() const;

private:
    Particle* findFreeSlot();
    void spawn (ParticleType type, Vec2 pos, Vec2 vel, float life, float size,
                uint8_t variant);

    std::array<Particle, kCapacity> pool;
    SeededRng rng;
    float ambientAccum = 0.0f;
    double previousBeatPhase = 0.0;
    float sparkleCooldown = 0.0f;
};
} // namespace lumi
