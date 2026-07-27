#include "plugin/PluginProcessor.h"

#include "constellation/RoutineEngine.h"
#include "plugin/Parameters.h"
#include "state/Presets.h"
#include "ui/OverlayWindow.h"
#include "ui/PluginEditor.h"

std::atomic<int> LumiDancerProcessor::instanceCounter { 0 };

LumiDancerProcessor::LumiDancerProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LUMI", lumi::params::createParameterLayout()),
      instanceNumber (++instanceCounter)
{
}

LumiDancerProcessor::~LumiDancerProcessor()
{
    // If this instance owns the detached overlay, tear it down synchronously —
    // the overlay reads this processor's buses and must never outlive it.
    if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
        if (mm->isThisTheMessageThread())
            OverlayController::instance().notifyProcessorDying (*this);
}

void LumiDancerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    analysis.prepare (sampleRate, samplesPerBlock);
}

void LumiDancerProcessor::releaseResources()
{
}

bool LumiDancerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Mono or stereo, input matching output. The audio is untouched either way.
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out)
        return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void LumiDancerProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midi);

    // ------------------------------------------------------------ host timing
    lumi::HostTimingSnapshot timing;
    timing.sampleRate = getSampleRate();
    if (auto* hostPlayHead = getPlayHead())
    {
        if (const auto position = hostPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
            {
                timing.hasBpm = true;
                timing.bpm = *bpm;
            }
            if (const auto ppq = position->getPpqPosition())
            {
                timing.hasPpq = true;
                timing.ppq = *ppq;
            }
            timing.isPlaying = position->getIsPlaying();
            timing.isRecording = position->getIsRecording();
            timing.isLooping = position->getIsLooping();
            if (const auto loop = position->getLoopPoints())
            {
                timing.loopStartPpq = loop->ppqStart;
                timing.loopEndPpq = loop->ppqEnd;
            }
            if (const auto sig = position->getTimeSignature())
            {
                timing.timeSigNumerator = sig->numerator;
                timing.timeSigDenominator = sig->denominator;
            }
            if (const auto samples = position->getTimeInSamples())
                timing.samplePos = *samples;
        }
    }
    timingBus.publish (timing);

    // --------------------------------------------------------- analysis tap
    // The buffer itself is NEVER written: LUMI//DANCER is a bit-transparent
    // pass-through. (An unwritten in-place buffer IS the pass-through.)
    const bool bypassed = apvts.getRawParameterValue (lumi::params::bypass)->load() > 0.5f;
    if (! bypassed)
    {
        frameBus.publish (analysis.process (buffer.getArrayOfReadPointers(),
                                            buffer.getNumChannels(),
                                            buffer.getNumSamples()));
    }
    else
    {
        // Bypass = Lumi rests: publish a silent frame, touch nothing else.
        lumi::AudioReactiveFrame resting;
        resting.silence = true;
        frameBus.publish (resting);
    }
}

juce::AudioProcessorParameter* LumiDancerProcessor::getBypassParameter() const
{
    return apvts.getParameter (lumi::params::bypass);
}

// --------------------------------------------------------------------- state
lumi::LumiSettings LumiDancerProcessor::getSettings() const
{
    const juce::ScopedLock lock (settingsLock);
    return settings;
}

void LumiDancerProcessor::setSettings (const lumi::LumiSettings& newSettings, bool notify)
{
    {
        const juce::ScopedLock lock (settingsLock);
        settings = newSettings;
        lumi::clampSettings (settings);
    }
    if (notify)
        settingsBroadcaster.sendChangeMessage();
}

void LumiDancerProcessor::pushSettingsToParameters (const lumi::LumiSettings& s)
{
    using namespace lumi::params;
    const auto setFloat = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setFloat (danceStyle, float (s.danceStyle));
    setFloat (intensity, s.intensity);
    setFloat (reactionAmount, s.reactionAmount);
    setFloat (lowSens, s.lowSens);
    setFloat (midSens, s.midSens);
    setFloat (highSens, s.highSens);
    setFloat (transientSens, s.transientSens);
    setFloat (smoothing, s.smoothing);
    setFloat (particleAmount, s.particleAmount);
    setFloat (mood, float (s.mood));
    setFloat (visualScale, s.visualScale);
    setFloat (mirror, s.mirror ? 1.0f : 0.0f);
    setFloat (visualOpacity, s.visualOpacity);
    setFloat (beatLock, s.beatLock ? 1.0f : 0.0f);
    setFloat (reducedMotion, s.accessibility.reducedMotion ? 1.0f : 0.0f);
}

void LumiDancerProcessor::applyPreset (int factoryPresetIndex)
{
    const auto& presets = lumi::factoryPresets();
    if (factoryPresetIndex < 0 || factoryPresetIndex >= int (presets.size()))
        return;

    lumi::LumiSettings presetSettings = presets[size_t (factoryPresetIndex)].settings;

    // Presets never steal the user's overlay geometry or editor size.
    const lumi::LumiSettings current = getSettings();
    presetSettings.overlay.x = current.overlay.x;
    presetSettings.overlay.y = current.overlay.y;
    presetSettings.overlay.w = current.overlay.w;
    presetSettings.overlay.h = current.overlay.h;
    presetSettings.editorWidth = current.editorWidth;
    presetSettings.editorHeight = current.editorHeight;

    setSettings (presetSettings, false);
    pushSettingsToParameters (presetSettings);
    settingsBroadcaster.sendChangeMessage();
}

void LumiDancerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("LumiDancerState");
    root.setAttribute ("schema", lumi::kSettingsSchemaVersion);
    root.setAttribute ("version", LUMI_VERSION_STRING);
    root.setAttribute ("settings", juce::String (lumi::serializeSettings (getSettings())));

    if (auto apvtsXml = apvts.copyState().createXml())
        root.addChildElement (new juce::XmlElement (*apvtsXml));

    copyXmlToBinary (root, destData);
}

void LumiDancerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("LumiDancerState"))
    {
        // Corrupt or foreign state: keep safe defaults, never crash.
        setSettings (lumi::LumiSettings {});
        return;
    }

    lumi::LumiSettings restored;
    if (! lumi::deserializeSettings (xml->getStringAttribute ("settings").toStdString(),
                                     restored))
        restored = lumi::LumiSettings {};
    setSettings (restored, false);

    if (auto* apvtsXml = xml->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*apvtsXml));

    settingsBroadcaster.sendChangeMessage();
}

// ------------------------------------------------------------------- editor
juce::AudioProcessorEditor* LumiDancerProcessor::createEditor()
{
    return new LumiDancerEditor (*this);
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LumiDancerProcessor();
}
