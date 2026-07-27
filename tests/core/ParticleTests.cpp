// Particle system tests: bounded pool, spawn rules, decay, reduced modes.
#include "TestFramework.h"

#include "fx/ParticleSystem.h"

using namespace lumi;

namespace
{
AudioReactiveFrame brightAudio()
{
    AudioReactiveFrame a;
    a.rms = 0.5f;
    a.lowEnergy = 0.8f;
    a.highEnergy = 0.7f;
    a.highTransient = 0.9f;
    a.silence = false;
    return a;
}
} // namespace

LD_TEST (particles_pool_never_exceeds_capacity)
{
    ParticleSystem system;
    ParticleParams params;
    params.amount = 1.0f;

    // Hammer it with maximal input for 30 simulated seconds.
    for (int i = 0; i < 1800; ++i)
    {
        system.update (1.0f / 60.0f, brightAudio(), (i % 30) / 30.0, 1.0f, params);
        system.emitBurst (ParticleType::Heart, 24, { 0.0f, 0.0f });
        LD_LE (system.aliveCount(), ParticleSystem::kCapacity);
    }
    LD_LE (system.aliveCount(), ParticleSystem::kCapacity);
}

LD_TEST (particles_spawn_on_high_transients)
{
    ParticleSystem system;
    ParticleParams params;
    params.amount = 0.8f;

    AudioReactiveFrame calm;
    calm.silence = false;
    for (int i = 0; i < 30; ++i)
        system.update (1.0f / 60.0f, calm, 0.5, 1.0f, params);
    const int before = system.aliveCount();

    system.update (1.0f / 60.0f, brightAudio(), 0.5, 1.0f, params);
    LD_GT (system.aliveCount(), before);
}

LD_TEST (particles_beat_ring_on_beat_wrap)
{
    ParticleSystem system;
    ParticleParams params;
    params.amount = 0.8f;

    AudioReactiveFrame kicky;
    kicky.lowEnergy = 0.8f;
    kicky.silence = false;

    system.update (1.0f / 60.0f, kicky, 0.9, 1.0f, params);
    system.update (1.0f / 60.0f, kicky, 0.05, 1.0f, params);   // wrapped

    bool sawRing = false;
    for (const auto& p : system.particles())
        if (p.alive && p.type == ParticleType::BeatRing)
            sawRing = true;
    LD_CHECK (sawRing);

    // noFlash suppresses rings entirely.
    ParticleSystem quiet;
    ParticleParams noFlash = params;
    noFlash.noFlash = true;
    quiet.update (1.0f / 60.0f, kicky, 0.9, 1.0f, noFlash);
    quiet.update (1.0f / 60.0f, kicky, 0.05, 1.0f, noFlash);
    bool ringInNoFlash = false;
    for (const auto& p : quiet.particles())
        if (p.alive && p.type == ParticleType::BeatRing)
            ringInNoFlash = true;
    LD_CHECK (! ringInNoFlash);
}

LD_TEST (particles_disabled_or_zero_amount_spawn_nothing)
{
    {
        ParticleSystem system;
        ParticleParams params;
        params.enabled = false;
        for (int i = 0; i < 120; ++i)
            system.update (1.0f / 60.0f, brightAudio(), (i % 30) / 30.0, 1.0f, params);
        LD_EQ (system.aliveCount(), 0);
    }
    {
        ParticleSystem system;
        ParticleParams params;
        params.amount = 0.0f;
        for (int i = 0; i < 120; ++i)
            system.update (1.0f / 60.0f, brightAudio(), (i % 30) / 30.0, 1.0f, params);
        LD_EQ (system.aliveCount(), 0);
    }
}

LD_TEST (particles_reduced_motion_spawns_fewer)
{
    ParticleSystem normal, reduced;
    ParticleParams normalParams;
    normalParams.amount = 0.8f;
    ParticleParams reducedParams = normalParams;
    reducedParams.reducedMotion = true;

    for (int i = 0; i < 600; ++i)
    {
        normal.update (1.0f / 60.0f, brightAudio(), (i % 30) / 30.0, 1.0f, normalParams);
        reduced.update (1.0f / 60.0f, brightAudio(), (i % 30) / 30.0, 1.0f, reducedParams);
    }
    LD_LT (reduced.aliveCount(), normal.aliveCount());
}

LD_TEST (particles_decay_and_pool_recycles)
{
    ParticleSystem system;
    ParticleParams params;
    system.emitBurst (ParticleType::Sparkle, 20, { 0.0f, 0.0f });
    const int spawned = system.aliveCount();
    LD_GT (spawned, 0);

    AudioReactiveFrame silence;
    silence.silence = true;
    for (int i = 0; i < 300; ++i)   // 5 s ≫ every sparkle lifetime
        system.update (1.0f / 60.0f, silence, 0.5, 0.0f, params);
    LD_EQ (system.aliveCount(), 0);
}
