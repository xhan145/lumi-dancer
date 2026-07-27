// LUMI//DANCER — the Constellation choreography model.
//
// Every dance move is a star. Related moves form constellations (Idle,
// Energy, Flow) and edges list which moves may follow which. The move
// library is the single source of truth: routine generation, the freestyle
// engine and the constellation UI all read it.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "analysis/AudioReactiveFrame.h"
#include "rig/CharacterPose.h"

namespace lumi
{
// Stable move identifiers — serialised in project state; never renumber.
enum class MoveId : uint64_t
{
    // Idle constellation
    IdleBounce   = 1,
    HeadNod      = 2,
    SideStep     = 3,
    HandWave     = 4,
    // Energy constellation
    Jump         = 10,
    Spin         = 11,
    DoubleStep   = 12,
    StarBurst    = 13,
    // Flow constellation
    Sway         = 20,
    OrbitArms    = 21,
    Float        = 22,
    TranceRise   = 23,
};

enum class ConstellationGroup : int { Idle = 0, Energy, Flow, Count };

struct DanceStar
{
    uint64_t id = 0;                       // MoveId value
    std::string name;
    ConstellationGroup group = ConstellationGroup::Idle;

    float energy     = 0.5f;
    float cuteness   = 0.5f;
    float speed      = 0.5f;
    float complexity = 0.5f;

    std::vector<uint64_t> compatibleNextMoves;

    bool favorite = false;
    bool locked   = false;

    // Layout position for the constellation view (unit square).
    float viewX = 0.5f;
    float viewY = 0.5f;
};

// The immutable factory move library.
const std::vector<DanceStar>& moveLibrary();

const DanceStar* findStar (uint64_t id);

// Evaluate one move's pose at a local beat position within the move.
// Deterministic; localBeat starts at 0 when the move begins.
CharacterPose evaluateMove (uint64_t id, double localBeat,
                            const AudioReactiveFrame& audio, float intensity);
} // namespace lumi
