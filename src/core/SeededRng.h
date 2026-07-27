// LUMI//DANCER — deterministic seeded random generator (SplitMix64 stream).
// std::* distributions are deliberately not used: their output differs
// between standard-library implementations, which would break the locked-seed
// choreography guarantee across platforms.
#pragma once

#include <cstdint>

namespace lumi
{
class SeededRng
{
public:
    explicit SeededRng (uint64_t seed = 0x4c554d49u /*'LUMI'*/) : state (seed) {}

    void reseed (uint64_t seed) { state = seed; }

    uint64_t nextU64()
    {
        state += 0x9e3779b97f4a7c15ull;
        uint64_t z = state;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        return z ^ (z >> 31);
    }

    // Uniform in [0, 1).
    float nextFloat01()
    {
        return float (nextU64() >> 40) * (1.0f / 16777216.0f);
    }

    // Uniform integer in [0, upperExclusive). Returns 0 for upperExclusive <= 0.
    int nextInt (int upperExclusive)
    {
        if (upperExclusive <= 0)
            return 0;
        return int (nextU64() % uint64_t (upperExclusive));
    }

    // Uniform in [lo, hi).
    float nextRange (float lo, float hi)
    {
        return lo + (hi - lo) * nextFloat01();
    }

    bool nextBool (float probabilityTrue = 0.5f)
    {
        return nextFloat01() < probabilityTrue;
    }

private:
    uint64_t state;
};
} // namespace lumi
