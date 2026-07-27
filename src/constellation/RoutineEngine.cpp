#include "constellation/RoutineEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

#include "core/LumiMath.h"

namespace lumi
{
const char* playbackModeName (PlaybackMode m)
{
    switch (m)
    {
        case PlaybackMode::Loop:     return "Loop";
        case PlaybackMode::OneShot:  return "One Shot";
        case PlaybackMode::PingPong: return "Ping Pong";
        case PlaybackMode::Shuffle:  return "Shuffle";
        default:                     return "Unknown";
    }
}

// ------------------------------------------------------ ChoreographyEngine
uint64_t ChoreographyEngine::chooseNext (uint64_t currentId,
                                         const ChoreoParams& params,
                                         const std::vector<uint64_t>& recentHistory,
                                         SeededRng& rng)
{
    const auto& library = moveLibrary();
    const DanceStar* current = findStar (currentId);

    // Candidate set: the current star's compatibility list, or (surprise roll /
    // no current star) the whole library.
    std::vector<const DanceStar*> candidates;
    candidates.reserve (library.size());

    const bool surpriseJump = rng.nextBool (clamp01 (params.surprise) * 0.5f);
    if (current != nullptr && ! surpriseJump)
    {
        for (uint64_t nextId : current->compatibleNextMoves)
            if (const DanceStar* star = findStar (nextId))
                candidates.push_back (star);
    }
    if (candidates.empty())
    {
        for (const auto& star : library)
            if (star.id != currentId)
                candidates.push_back (&star);
    }

    // Complexity gate (keep at least the simplest candidate).
    std::vector<const DanceStar*> allowed;
    for (const DanceStar* star : candidates)
        if (star->complexity <= params.complexity + 1.0e-3f && ! star->locked)
            allowed.push_back (star);
    if (allowed.empty())
    {
        const DanceStar* simplest = *std::min_element (
            candidates.begin(), candidates.end(),
            [] (const DanceStar* a, const DanceStar* b) { return a->complexity < b->complexity; });
        allowed.push_back (simplest);
    }

    // Weighted pick: closeness to the energy/cuteness targets, recency penalty,
    // favourite bonus.
    float totalWeight = 0.0f;
    std::vector<float> weights;
    weights.reserve (allowed.size());
    for (const DanceStar* star : allowed)
    {
        float w = 1.0f;
        w *= 1.0f - 0.7f * std::fabs (star->energy - params.energy);
        w *= 1.0f - 0.5f * std::fabs (star->cuteness - params.cuteness);
        if (star->favorite)
            w *= 1.5f;

        // Recency penalty, strongest for the most recent entries.
        for (size_t i = 0; i < recentHistory.size(); ++i)
        {
            if (recentHistory[i] == star->id)
            {
                const float recency = 1.0f - float (i) / float (std::max<size_t> (1, recentHistory.size()));
                w *= 1.0f - clamp01 (params.repeatAvoidance) * 0.85f * recency;
            }
        }
        w = std::max (w, 0.02f);
        weights.push_back (w);
        totalWeight += w;
    }

    float roll = rng.nextFloat01() * totalWeight;
    for (size_t i = 0; i < allowed.size(); ++i)
    {
        roll -= weights[i];
        if (roll <= 0.0f)
            return allowed[i]->id;
    }
    return allowed.back()->id;
}

// ------------------------------------------------------------------ Routine
double Routine::lengthBeats() const
{
    double length = 0.0;
    for (const auto& node : nodes)
        length = std::max (length, node.startBeat + node.durationBeats);
    return length;
}

int Routine::nodeIndexAt (double mappedBeat) const
{
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const auto& n = nodes[i];
        if (mappedBeat >= n.startBeat && mappedBeat < n.startBeat + n.durationBeats)
            return int (i);
    }
    return nodes.empty() ? -1 : int (nodes.size()) - 1;
}

Routine generateRoutine (int bars, int beatsPerBar, uint64_t seed,
                         const ChoreoParams& params)
{
    bars = std::clamp (bars, 1, 64);
    beatsPerBar = std::clamp (beatsPerBar, 1, 12);
    const double totalBeats = double (bars) * beatsPerBar;

    Routine routine;
    SeededRng rng (seed);
    std::vector<uint64_t> history;

    uint64_t currentMove = uint64_t (MoveId::IdleBounce);
    double cursor = 0.0;
    uint64_t nextNodeId = 1;

    while (cursor < totalBeats - 1.0e-9)
    {
        // Segment length: energetic/fast moves get shorter segments. Snap the
        // final segment to the routine end.
        const DanceStar* star = findStar (currentMove);
        const float speed = star != nullptr ? star->speed : 0.5f;
        double duration = speed > 0.65f ? 2.0 : 4.0;
        if (rng.nextBool (0.25f))
            duration *= 0.5;
        duration = std::min (duration, totalBeats - cursor);

        RoutineNode node;
        node.nodeId = nextNodeId++;
        node.danceStarId = currentMove;
        node.startBeat = cursor;
        node.durationBeats = duration;
        node.intensity = 0.85f + 0.3f * rng.nextFloat01();
        routine.nodes.push_back (node);

        history.insert (history.begin(), currentMove);
        if (history.size() > 4)
            history.pop_back();

        cursor += duration;
        currentMove = ChoreographyEngine::chooseNext (currentMove, params, history, rng);
    }

    return routine;
}

// ----------------------------------------------------------- beat mapping
namespace
{
// Map an absolute beat into routine-local time under the playback mode.
// Returns the mapped beat and (for Shuffle) the node reorder permutation.
double mapBeat (const Routine& routine, double beat, PlaybackMode mode,
                bool* finished)
{
    const double length = routine.lengthBeats();
    if (finished != nullptr)
        *finished = false;
    if (length <= 0.0)
        return 0.0;

    beat = std::max (0.0, beat);
    switch (mode)
    {
        case PlaybackMode::OneShot:
            if (beat >= length)
            {
                if (finished != nullptr)
                    *finished = true;
                return length - 1.0e-6;
            }
            return beat;

        case PlaybackMode::PingPong:
        {
            const double cycle = std::fmod (beat, length * 2.0);
            return cycle < length ? cycle : length * 2.0 - cycle - 1.0e-9;
        }

        case PlaybackMode::Shuffle:
        case PlaybackMode::Loop:
        default:
            return std::fmod (beat, length);
    }
}

// Shuffle support: deterministic node visit order per repeat cycle.
int shuffledNodeIndex (const Routine& routine, double beat, uint64_t shuffleSeed,
                       double* localBeatOut)
{
    const double length = routine.lengthBeats();
    const uint64_t cycle = uint64_t (std::floor (std::max (0.0, beat) / length));
    const double inCycle = std::fmod (std::max (0.0, beat), length);

    // Fisher-Yates permutation of node indices, deterministic per cycle.
    std::vector<int> order (routine.nodes.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = int (i);
    SeededRng rng (shuffleSeed * 0x9e3779b97f4a7c15ull + cycle + 1);
    for (int i = int (order.size()) - 1; i > 0; --i)
        std::swap (order[size_t (i)], order[size_t (rng.nextInt (i + 1))]);

    double cursor = 0.0;
    for (int idx : order)
    {
        const double dur = routine.nodes[size_t (idx)].durationBeats;
        if (inCycle < cursor + dur)
        {
            if (localBeatOut != nullptr)
                *localBeatOut = inCycle - cursor;
            return idx;
        }
        cursor += dur;
    }
    if (localBeatOut != nullptr)
        *localBeatOut = 0.0;
    return order.empty() ? -1 : order.back();
}
} // namespace

CharacterPose evaluateRoutine (const Routine& routine, double beat,
                               PlaybackMode mode, uint64_t shuffleSeed,
                               const AudioReactiveFrame& audio, float intensity,
                               bool* finished)
{
    if (routine.empty())
    {
        CharacterPose p;
        sanitizePose (p);
        if (finished != nullptr)
            *finished = false;
        return p;
    }

    int index = -1;
    double localBeat = 0.0;

    if (mode == PlaybackMode::Shuffle)
    {
        index = shuffledNodeIndex (routine, beat, shuffleSeed, &localBeat);
        if (finished != nullptr)
            *finished = false;
    }
    else
    {
        const double mapped = mapBeat (routine, beat, mode, finished);
        index = routine.nodeIndexAt (mapped);
        if (index >= 0)
            localBeat = mapped - routine.nodes[size_t (index)].startBeat;
    }

    if (index < 0)
    {
        CharacterPose p;
        sanitizePose (p);
        return p;
    }

    const RoutineNode& node = routine.nodes[size_t (index)];
    return evaluateMove (node.danceStarId, localBeat, audio,
                         intensity * node.intensity);
}

uint64_t routineActiveStar (const Routine& routine, double beat,
                            PlaybackMode mode, uint64_t shuffleSeed)
{
    if (routine.empty())
        return 0;

    if (mode == PlaybackMode::Shuffle)
    {
        const int index = shuffledNodeIndex (routine, beat, shuffleSeed, nullptr);
        return index >= 0 ? routine.nodes[size_t (index)].danceStarId : 0;
    }

    const double mapped = mapBeat (routine, beat, mode, nullptr);
    const int index = routine.nodeIndexAt (mapped);
    return index >= 0 ? routine.nodes[size_t (index)].danceStarId : 0;
}

// ----------------------------------------------------------- serialisation
std::string serializeRoutine (const Routine& routine)
{
    std::string out;
    char line[160];
    for (const auto& n : routine.nodes)
    {
        std::snprintf (line, sizeof (line), "%llu,%llu,%.6f,%.6f,%.4f,%d\n",
                       (unsigned long long) n.nodeId,
                       (unsigned long long) n.danceStarId,
                       n.startBeat, n.durationBeats, double (n.intensity),
                       n.locked ? 1 : 0);
        out += line;
    }
    return out;
}

bool deserializeRoutine (const std::string& text, Routine& out)
{
    Routine parsed;
    std::istringstream stream (text);
    std::string line;
    while (std::getline (stream, line))
    {
        if (line.empty())
            continue;
        unsigned long long nodeId = 0, starId = 0;
        double start = 0.0, dur = 0.0, inten = 1.0;
        int locked = 0;
        if (std::sscanf (line.c_str(), "%llu,%llu,%lf,%lf,%lf,%d",
                         &nodeId, &starId, &start, &dur, &inten, &locked) != 6)
            return false;
        if (! std::isfinite (start) || ! std::isfinite (dur) || dur <= 0.0 || dur > 256.0
            || start < 0.0 || start > 4096.0)
            return false;
        if (findStar (starId) == nullptr)
            return false;   // unknown move (corrupt or future state)

        RoutineNode node;
        node.nodeId = nodeId;
        node.danceStarId = starId;
        node.startBeat = start;
        node.durationBeats = dur;
        node.intensity = float (clamp (inten, 0.0, 2.0));
        node.locked = locked != 0;
        parsed.nodes.push_back (node);
    }

    if (parsed.nodes.empty())
        return false;
    out = std::move (parsed);
    return true;
}
} // namespace lumi
