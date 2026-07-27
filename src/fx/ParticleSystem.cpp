#include "fx/ParticleSystem.h"

#include <cmath>

namespace lumi
{
ParticleSystem::ParticleSystem() : rng (0x5354u /*'ST'*/)
{
}

void ParticleSystem::reset()
{
    for (auto& p : pool)
        p.alive = false;
    ambientAccum = 0.0f;
    previousBeatPhase = 0.0;
    sparkleCooldown = 0.0f;
}

int ParticleSystem::aliveCount() const
{
    int n = 0;
    for (const auto& p : pool)
        if (p.alive)
            ++n;
    return n;
}

Particle* ParticleSystem::findFreeSlot()
{
    for (auto& p : pool)
        if (! p.alive)
            return &p;
    return nullptr;   // pool exhausted: skip, never grow
}

void ParticleSystem::spawn (ParticleType type, Vec2 pos, Vec2 vel, float life,
                            float size, uint8_t variant)
{
    Particle* slot = findFreeSlot();
    if (slot == nullptr)
        return;

    slot->alive = true;
    slot->type = type;
    slot->pos = pos;
    slot->vel = vel;
    slot->life = slot->maxLife = life;
    slot->size = size;
    slot->rotation = rng.nextRange (0.0f, kTwoPi);
    slot->spin = rng.nextRange (-2.0f, 2.0f);
    slot->variant = variant;
}

void ParticleSystem::emitBurst (ParticleType type, int count, Vec2 origin)
{
    count = clamp (count, 0, 24);
    for (int i = 0; i < count; ++i)
    {
        const float ang = rng.nextRange (0.0f, kTwoPi);
        const float speed = rng.nextRange (0.15f, 0.5f);
        spawn (type, origin,
               { std::cos (ang) * speed, std::sin (ang) * speed - 0.2f },
               rng.nextRange (0.6f, 1.2f),
               rng.nextRange (0.02f, 0.05f),
               uint8_t (rng.nextInt (3)));
    }
}

void ParticleSystem::update (float dt, const AudioReactiveFrame& audio,
                             double beatPhase, float activityLevel,
                             const ParticleParams& params)
{
    dt = clamp (dt, 0.0f, 0.25f);

    // ------------------------------------------------------------- integrate
    for (auto& p : pool)
    {
        if (! p.alive)
            continue;
        p.life -= dt;
        if (p.life <= 0.0f)
        {
            p.alive = false;
            continue;
        }
        p.pos = p.pos + p.vel * dt;
        p.rotation += p.spin * dt;

        if (p.type == ParticleType::BeatRing)
            p.size += (params.reducedMotion ? 0.6f : 1.2f) * dt;   // expand
        else if (p.type == ParticleType::Star)
            p.vel.y += -0.02f * dt;                                // gentle rise
    }

    if (! params.enabled)
        return;

    float amount = clamp01 (params.amount) * clamp01 (0.25f + 0.75f * activityLevel);
    if (params.reducedMotion)
        amount *= 0.35f;
    if (amount <= 0.0f)
    {
        previousBeatPhase = beatPhase;
        return;
    }

    // ------------------------------------------------------- ambient drift
    ambientAccum += dt * amount * (0.8f + 3.0f * audio.rms);
    while (ambientAccum >= 1.0f)
    {
        ambientAccum -= 1.0f;
        const bool moon = rng.nextBool (0.06f);
        spawn (moon ? ParticleType::Moon : ParticleType::Star,
               { rng.nextRange (-0.95f, 0.95f), rng.nextRange (0.55f, 0.95f) },
               { rng.nextRange (-0.02f, 0.02f), rng.nextRange (-0.10f, -0.04f) },
               rng.nextRange (3.0f, 6.0f),
               rng.nextRange (0.015f, 0.04f),
               uint8_t (rng.nextInt (3)));
    }

    // -------------------------------------------------- high-band sparkles
    sparkleCooldown = std::max (0.0f, sparkleCooldown - dt);
    if (audio.highTransient > 0.35f && sparkleCooldown <= 0.0f)
    {
        const int n = int (std::ceil (audio.highTransient * 4.0f * amount));
        for (int i = 0; i < n; ++i)
            spawn (ParticleType::Sparkle,
                   { rng.nextRange (-0.35f, 0.35f), rng.nextRange (-0.55f, -0.15f) },
                   { rng.nextRange (-0.25f, 0.25f), rng.nextRange (-0.30f, 0.05f) },
                   rng.nextRange (0.35f, 0.7f),
                   rng.nextRange (0.012f, 0.028f),
                   uint8_t (rng.nextInt (3)));
        sparkleCooldown = 0.09f;
    }

    // ------------------------------------------------------------ beat ring
    const bool beatWrapped = beatPhase < previousBeatPhase;
    previousBeatPhase = beatPhase;
    if (beatWrapped && ! params.noFlash && audio.lowEnergy > 0.3f && activityLevel > 0.4f)
    {
        spawn (ParticleType::BeatRing,
               { 0.0f, 0.62f },                       // at the feet
               { 0.0f, 0.0f },
               params.reducedMotion ? 0.35f : 0.5f,
               0.12f,
               uint8_t (audio.lowTransient > 0.5f ? 1 : 0));
    }
}
} // namespace lumi
