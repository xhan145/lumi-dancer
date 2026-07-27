// LUMI//DANCER — constellation choreography and routines.
//
// The ChoreographyEngine walks the move-star graph deterministically from a
// seed, honouring complexity limits, repeat avoidance, surprise and the
// energy/cuteness targets. Routines are timed sequences of move stars played
// back against the host beat (never wall-clock time).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "constellation/DanceStar.h"
#include "core/SeededRng.h"

namespace lumi
{
struct RoutineNode
{
    uint64_t nodeId = 0;
    uint64_t danceStarId = 0;

    double startBeat = 0.0;
    double durationBeats = 4.0;

    float intensity = 1.0f;
    bool locked = false;
};

enum class PlaybackMode : int { Loop = 0, OneShot, PingPong, Shuffle, Count };

const char* playbackModeName (PlaybackMode m);

struct ChoreoParams
{
    float complexity      = 0.75f;  // moves above this complexity are excluded
    float repeatAvoidance = 0.7f;   // penalty weight for recently used moves
    float surprise        = 0.3f;   // chance to jump outside the compatibility list
    float energy          = 0.5f;   // preferred move energy
    float cuteness        = 0.5f;   // preferred move cuteness
};

class ChoreographyEngine
{
public:
    // Deterministic: identical (currentId, params, history, rng state) always
    // yields the same move.
    static uint64_t chooseNext (uint64_t currentId,
                                const ChoreoParams& params,
                                const std::vector<uint64_t>& recentHistory,
                                SeededRng& rng);
};

struct Routine
{
    std::vector<RoutineNode> nodes;

    double lengthBeats() const;
    bool empty() const { return nodes.empty(); }

    // Node index containing the (already mapped) beat, or -1.
    int nodeIndexAt (double mappedBeat) const;
};

// Generate a routine covering `bars` bars. Deterministic per seed.
Routine generateRoutine (int bars, int beatsPerBar, uint64_t seed,
                         const ChoreoParams& params);

// Evaluate the routine at an absolute beat position under a playback mode.
// `finished` (optional) reports OneShot completion.
CharacterPose evaluateRoutine (const Routine& routine, double beat,
                               PlaybackMode mode, uint64_t shuffleSeed,
                               const AudioReactiveFrame& audio, float intensity,
                               bool* finished = nullptr);

// Which star is active at this beat under this mode (for UI highlighting).
uint64_t routineActiveStar (const Routine& routine, double beat,
                            PlaybackMode mode, uint64_t shuffleSeed);

// --------------------------------------------------------- serialisation
// Compact line format used inside the plugin state blob.
std::string serializeRoutine (const Routine& routine);
bool deserializeRoutine (const std::string& text, Routine& out);
} // namespace lumi
