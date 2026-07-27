// Constellation system tests: library integrity, deterministic choreography,
// routine generation/serialisation, playback modes.
#include "TestFramework.h"

#include <set>

#include "constellation/RoutineEngine.h"

using namespace lumi;

namespace
{
AudioReactiveFrame someAudio()
{
    AudioReactiveFrame a;
    a.rms = 0.3f;
    a.lowEnergy = 0.6f;
    a.midEnergy = 0.4f;
    a.silence = false;
    return a;
}
} // namespace

LD_TEST (constellation_library_is_valid)
{
    const auto& lib = moveLibrary();
    LD_EQ (lib.size(), size_t (12));

    std::set<uint64_t> ids;
    int idleCount = 0, energyCount = 0, flowCount = 0;
    bool edgesValid = true, hasEdges = true, attributesValid = true;

    for (const auto& star : lib)
    {
        ids.insert (star.id);
        if (star.group == ConstellationGroup::Idle)   ++idleCount;
        if (star.group == ConstellationGroup::Energy) ++energyCount;
        if (star.group == ConstellationGroup::Flow)   ++flowCount;

        if (star.compatibleNextMoves.empty())
            hasEdges = false;
        for (uint64_t next : star.compatibleNextMoves)
            if (findStar (next) == nullptr || next == star.id)
                edgesValid = false;

        if (star.energy < 0.0f || star.energy > 1.0f
            || star.complexity < 0.0f || star.complexity > 1.0f)
            attributesValid = false;
    }

    LD_EQ (ids.size(), size_t (12));   // unique ids
    LD_EQ (idleCount, 4);
    LD_EQ (energyCount, 4);
    LD_EQ (flowCount, 4);
    LD_CHECK (hasEdges);
    LD_CHECK (edgesValid);
    LD_CHECK (attributesValid);
}

LD_TEST (constellation_moves_produce_distinct_poses)
{
    const AudioReactiveFrame audio = someAudio();
    const auto& lib = moveLibrary();

    for (size_t a = 0; a < lib.size(); ++a)
        for (size_t b = a + 1; b < lib.size(); ++b)
        {
            float diff = 0.0f;
            for (int i = 0; i < 32; ++i)
            {
                const double beat = double (i) / 8.0;
                diff += poseDistance (evaluateMove (lib[a].id, beat, audio, 1.0f),
                                      evaluateMove (lib[b].id, beat, audio, 1.0f));
            }
            _ctx.report (diff > 0.1f, (lib[a].name + " vs " + lib[b].name).c_str(),
                         __FILE__, __LINE__);
        }
}

LD_TEST (choreography_choose_next_is_deterministic)
{
    ChoreoParams params;
    std::vector<uint64_t> history;

    SeededRng rngA (77), rngB (77);
    bool identical = true;
    uint64_t currentA = uint64_t (MoveId::IdleBounce);
    uint64_t currentB = currentA;
    for (int i = 0; i < 64; ++i)
    {
        currentA = ChoreographyEngine::chooseNext (currentA, params, history, rngA);
        currentB = ChoreographyEngine::chooseNext (currentB, params, history, rngB);
        if (currentA != currentB)
            identical = false;
    }
    LD_CHECK (identical);
}

LD_TEST (choreography_respects_complexity_gate)
{
    ChoreoParams params;
    params.complexity = 0.3f;   // only the simplest moves allowed
    params.surprise = 1.0f;     // force whole-library candidate sets often
    std::vector<uint64_t> history;
    SeededRng rng (5);

    uint64_t current = uint64_t (MoveId::IdleBounce);
    bool allSimple = true;
    for (int i = 0; i < 128; ++i)
    {
        current = ChoreographyEngine::chooseNext (current, params, history, rng);
        const DanceStar* star = findStar (current);
        if (star != nullptr && star->complexity > 0.31f)
            allSimple = false;
    }
    LD_CHECK (allSimple);
}

LD_TEST (choreography_repeat_avoidance_reduces_repeats)
{
    ChoreoParams avoid;
    avoid.repeatAvoidance = 1.0f;
    ChoreoParams allow;
    allow.repeatAvoidance = 0.0f;

    int repeatsAvoid = 0, repeatsAllow = 0;
    for (int trial = 0; trial < 8; ++trial)
    {
        SeededRng rngA (uint64_t (trial) * 7 + 1), rngB (uint64_t (trial) * 7 + 1);
        uint64_t a = uint64_t (MoveId::IdleBounce), b = a;
        std::vector<uint64_t> histA, histB;
        for (int i = 0; i < 64; ++i)
        {
            const uint64_t nextA = ChoreographyEngine::chooseNext (a, avoid, histA, rngA);
            const uint64_t nextB = ChoreographyEngine::chooseNext (b, allow, histB, rngB);
            if (nextA == a) ++repeatsAvoid;
            if (nextB == b) ++repeatsAllow;
            histA.insert (histA.begin(), nextA);
            if (histA.size() > 4) histA.pop_back();
            histB.insert (histB.begin(), nextB);
            if (histB.size() > 4) histB.pop_back();
            a = nextA;
            b = nextB;
        }
    }
    LD_LE (repeatsAvoid, repeatsAllow);
}

LD_TEST (routine_generation_covers_bars_and_is_deterministic)
{
    ChoreoParams params;
    const Routine r1 = generateRoutine (4, 4, 1234, params);
    const Routine r2 = generateRoutine (4, 4, 1234, params);
    const Routine r3 = generateRoutine (4, 4, 999, params);

    LD_CHECK (! r1.empty());
    LD_NEAR (r1.lengthBeats(), 16.0, 1e-9);

    // Nodes tile the timeline with no gaps.
    double cursor = 0.0;
    bool contiguous = true;
    for (const auto& node : r1.nodes)
    {
        if (std::fabs (node.startBeat - cursor) > 1e-9)
            contiguous = false;
        cursor += node.durationBeats;
    }
    LD_CHECK (contiguous);

    // Locked seed reproduces exactly.
    LD_EQ (r1.nodes.size(), r2.nodes.size());
    bool identical = r1.nodes.size() == r2.nodes.size();
    for (size_t i = 0; identical && i < r1.nodes.size(); ++i)
        identical = r1.nodes[i].danceStarId == r2.nodes[i].danceStarId
                 && r1.nodes[i].startBeat == r2.nodes[i].startBeat;
    LD_CHECK (identical);

    // A different seed differs somewhere.
    bool differs = r1.nodes.size() != r3.nodes.size();
    for (size_t i = 0; ! differs && i < r1.nodes.size() && i < r3.nodes.size(); ++i)
        differs = r1.nodes[i].danceStarId != r3.nodes[i].danceStarId;
    LD_CHECK (differs);

    // Consecutive nodes follow compatibility (or a surprise jump — verify at
    // least half follow the graph edges).
    int compatible = 0;
    for (size_t i = 1; i < r1.nodes.size(); ++i)
    {
        const DanceStar* prev = findStar (r1.nodes[i - 1].danceStarId);
        if (prev == nullptr)
            continue;
        for (uint64_t next : prev->compatibleNextMoves)
            if (next == r1.nodes[i].danceStarId)
            {
                ++compatible;
                break;
            }
    }
    LD_GT (compatible * 2, int (r1.nodes.size()) - 1);
}

LD_TEST (routine_serialization_round_trip)
{
    ChoreoParams params;
    const Routine original = generateRoutine (8, 4, 555, params);
    const std::string text = serializeRoutine (original);

    Routine restored;
    LD_CHECK (deserializeRoutine (text, restored));
    LD_EQ (restored.nodes.size(), original.nodes.size());

    bool identical = restored.nodes.size() == original.nodes.size();
    for (size_t i = 0; identical && i < original.nodes.size(); ++i)
    {
        const auto& a = original.nodes[i];
        const auto& b = restored.nodes[i];
        identical = a.nodeId == b.nodeId && a.danceStarId == b.danceStarId
                 && std::fabs (a.startBeat - b.startBeat) < 1e-6
                 && std::fabs (a.durationBeats - b.durationBeats) < 1e-6
                 && std::fabs (a.intensity - b.intensity) < 1e-3
                 && a.locked == b.locked;
    }
    LD_CHECK (identical);
}

LD_TEST (routine_deserialize_rejects_corrupt_input)
{
    Routine out;
    LD_CHECK (! deserializeRoutine ("", out));
    LD_CHECK (! deserializeRoutine ("garbage\nnot,numbers", out));
    LD_CHECK (! deserializeRoutine ("1,99999,0.0,4.0,1.0,0\n", out));   // unknown star
    LD_CHECK (! deserializeRoutine ("1,1,0.0,-4.0,1.0,0\n", out));      // negative duration
    LD_CHECK (! deserializeRoutine ("1,1,nan,4.0,1.0,0\n", out));       // non-finite
}

LD_TEST (routine_playback_modes)
{
    ChoreoParams params;
    const Routine routine = generateRoutine (2, 4, 42, params);   // 8 beats
    const AudioReactiveFrame audio = someAudio();

    // Loop: same position every cycle.
    const CharacterPose loopA = evaluateRoutine (routine, 1.0, PlaybackMode::Loop, 1, audio, 1.0f);
    const CharacterPose loopB = evaluateRoutine (routine, 9.0, PlaybackMode::Loop, 1, audio, 1.0f);
    LD_NEAR (poseDistance (loopA, loopB), 0.0f, 1e-6);

    // OneShot: reports finished past the end and holds a stable pose.
    bool finished = false;
    evaluateRoutine (routine, 3.0, PlaybackMode::OneShot, 1, audio, 1.0f, &finished);
    LD_CHECK (! finished);
    evaluateRoutine (routine, 30.0, PlaybackMode::OneShot, 1, audio, 1.0f, &finished);
    LD_CHECK (finished);

    // PingPong: position x mirrors to 2L-x in the second half of the cycle.
    // (6.3 sits mid-node — node boundaries fall on integer beats.)
    const CharacterPose ppA = evaluateRoutine (routine, 6.3, PlaybackMode::PingPong, 1, audio, 1.0f);
    const CharacterPose ppB = evaluateRoutine (routine, 9.7, PlaybackMode::PingPong, 1, audio, 1.0f);
    LD_NEAR (poseDistance (ppA, ppB), 0.0f, 1e-4);

    // Shuffle: deterministic for the same seed, and all nodes get visited
    // within one cycle.
    std::set<uint64_t> visitedA, visitedB;
    for (int i = 0; i < 64; ++i)
    {
        const double beat = double (i) / 8.0;
        visitedA.insert (routineActiveStar (routine, beat, PlaybackMode::Shuffle, 9));
        visitedB.insert (routineActiveStar (routine, beat, PlaybackMode::Shuffle, 9));
    }
    LD_CHECK (visitedA == visitedB);

    std::set<uint64_t> expected;
    for (const auto& node : routine.nodes)
        expected.insert (node.danceStarId);
    LD_CHECK (visitedA == expected);

    // Every mode yields finite poses at arbitrary positions.
    for (int mode = 0; mode < int (PlaybackMode::Count); ++mode)
    {
        const CharacterPose p = evaluateRoutine (routine, 123.456, PlaybackMode (mode), 3, audio, 1.0f);
        LD_CHECK (std::isfinite (p.root.position.x));
    }
}

LD_TEST (routine_empty_is_safe)
{
    Routine empty;
    const CharacterPose p = evaluateRoutine (empty, 5.0, PlaybackMode::Loop, 1, someAudio(), 1.0f);
    LD_CHECK (std::isfinite (p.root.position.x));
    LD_EQ (routineActiveStar (empty, 5.0, PlaybackMode::Loop, 1), uint64_t (0));
}
