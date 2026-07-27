#include "plugin/Parameters.h"

#include "dance/DanceAnimation.h"

namespace lumi::params
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray styleNames;
    for (int i = 0; i < int (DanceStyle::Count); ++i)
        styleNames.add (danceStyleName (DanceStyle (i)));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { danceStyle, 1 }, "Dance Style", styleNames, 0));

    const auto range01 = NormalisableRange<float> (0.0f, 1.0f, 0.0f);
    const auto range02 = NormalisableRange<float> (0.0f, 2.0f, 0.0f);

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { intensity, 1 }, "Animation Intensity", range02, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { reactionAmount, 1 }, "Reaction Amount", range02, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { lowSens, 1 }, "Low Sensitivity", range02, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { midSens, 1 }, "Mid Sensitivity", range02, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { highSens, 1 }, "High Sensitivity", range02, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { transientSens, 1 }, "Transient Sensitivity", range02, 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { smoothing, 1 }, "Motion Smoothing", range01, 0.35f));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { particleAmount, 1 }, "Particle Amount", range01, 0.6f));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { mood, 1 }, "Expression Mood",
        StringArray { "Soft", "Cheerful", "Confident", "Sleepy", "Hyper" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { routinePosition, 1 }, "Routine Position", range01, 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { visualScale, 1 }, "Scale",
        NormalisableRange<float> (0.4f, 2.0f, 0.0f), 1.0f));
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { mirror, 1 }, "Mirror", false));
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { visualOpacity, 1 }, "Visual Opacity",
        NormalisableRange<float> (0.1f, 1.0f, 0.0f), 1.0f));
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { beatLock, 1 }, "Beat Lock", false));
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { reducedMotion, 1 }, "Reduced Motion", false));
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { bypass, 1 }, "Bypass", false));

    return layout;
}
} // namespace lumi::params
