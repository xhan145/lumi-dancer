// LUMI//DANCER — automatable parameters.
//
// Parameter IDs are a public contract: they are saved in Ableton projects
// and referenced by automation lanes. NEVER rename a released ID — add new
// ones and migrate instead.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace lumi::params
{
// Stable IDs (v1).
inline constexpr const char* danceStyle      = "danceStyle";
inline constexpr const char* intensity       = "intensity";
inline constexpr const char* reactionAmount  = "reactionAmount";
inline constexpr const char* lowSens         = "lowSens";
inline constexpr const char* midSens         = "midSens";
inline constexpr const char* highSens        = "highSens";
inline constexpr const char* transientSens   = "transientSens";
inline constexpr const char* smoothing       = "smoothing";
inline constexpr const char* particleAmount  = "particleAmount";
inline constexpr const char* mood            = "mood";
inline constexpr const char* routinePosition = "routinePosition";
inline constexpr const char* visualScale     = "visualScale";
inline constexpr const char* mirror          = "mirror";
inline constexpr const char* visualOpacity   = "visualOpacity";
inline constexpr const char* beatLock        = "beatLock";
inline constexpr const char* reducedMotion   = "reducedMotion";
inline constexpr const char* bypass          = "bypass";

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace lumi::params
