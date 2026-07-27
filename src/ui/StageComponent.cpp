#include "ui/StageComponent.h"

#include "plugin/Parameters.h"
#include "ui/Theme.h"

using namespace lumi;

StageComponent::StageComponent (LumiDancerProcessor& processorIn)
    : processor (processorIn)
{
    setOpaque (false);
    setInterceptsMouseClicks (false, false);
    refreshFromSettings();
    lastTickSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
    updateFrameRate();
}

StageComponent::~StageComponent()
{
    stopTimer();
}

void StageComponent::refreshFromSettings()
{
    cachedSettings = processor.getSettings();
    updateFrameRate();

    // User background image: load once per path on the message thread; a
    // missing file falls back to the neutral gradient without errors.
    const juce::String path (cachedSettings.userImagePath);
    if (path != userBackgroundPath)
    {
        userBackgroundPath = path;
        userBackground = {};
        userBackgroundFailed = false;
        if (path.isNotEmpty())
        {
            const juce::File file (path);
            if (file.existsAsFile())
                userBackground = juce::ImageFileFormat::loadFrom (file);
            userBackgroundFailed = ! userBackground.isValid();
        }
    }
}

void StageComponent::updateFrameRate()
{
    int fps = 60;
    switch (FrameRateMode (cachedSettings.frameRateMode))
    {
        case FrameRateMode::Fps30:    fps = 30; break;
        case FrameRateMode::Fps60:    fps = 60; break;
        case FrameRateMode::Adaptive: fps = paintMsAverage > 9.0f ? 30 : 60; break;
        default: break;
    }
    if (cachedSettings.accessibility.reducedMotion)
        fps = std::min (fps, 30);
    if (fps != currentFps || ! isTimerRunning())
    {
        currentFps = fps;
        startTimerHz (fps);
    }
}

ChoreographerParams StageComponent::gatherParams (const LumiSettings& s) const
{
    const auto& apvts = processor.apvts;
    const auto raw = [&apvts] (const char* id)
    {
        return apvts.getRawParameterValue (id)->load();
    };

    ChoreographerParams p;
    p.style = DanceStyle (clamp (int (raw (params::danceStyle)), 0,
                                 int (DanceStyle::Count) - 1));
    p.intensity = raw (params::intensity);
    p.reactionAmount = raw (params::reactionAmount);
    p.lowSens = raw (params::lowSens);
    p.midSens = raw (params::midSens);
    p.highSens = raw (params::highSens);
    p.transientSens = raw (params::transientSens);
    p.smoothing = raw (params::smoothing);
    p.beatLock = raw (params::beatLock) > 0.5f;
    p.mood = ExpressionTheme (clamp (int (raw (params::mood)), 0,
                                     int (ExpressionTheme::Count) - 1));
    p.seed = s.seed;
    p.reducedMotion = s.accessibility.reducedMotion || raw (params::reducedMotion) > 0.5f;
    p.idleEnabled = s.idleEnabled;
    p.sleepDelaySeconds = s.sleepDelaySeconds;
    p.idleMotionAmount = s.idleMotionAmount;
    p.useRoutine = s.useRoutine;
    p.playbackMode = PlaybackMode (clamp (s.playbackMode, 0, int (PlaybackMode::Count) - 1));
    return p;
}

void StageComponent::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const double dt = clamp (now - lastTickSeconds, 0.0, 0.25);
    lastTickSeconds = now;

    if (frozen)
        return;

    const HostTimingSnapshot timing = processor.timingBus.read();
    const AudioReactiveFrame frame = processor.frameBus.read();
    clock.update (timing, dt);

    const LumiSettings s = cachedSettings;
    ChoreographerParams params = gatherParams (s);

    // Routine data lives in settings; hand it to the choreographer when the
    // node list changed (cheap size/pointer compare via serialised text).
    if (params.useRoutine && choreographer.routine().empty() && ! s.routineData.empty())
    {
        Routine routine;
        if (deserializeRoutine (s.routineData, routine))
            choreographer.setRoutine (std::move (routine));
    }

    currentPose = choreographer.update (float (dt), clock, frame, params);

    ParticleParams pp;
    pp.amount = processor.apvts.getRawParameterValue (params::particleAmount)->load();
    pp.enabled = s.effectsEnabled;
    pp.reducedMotion = params.reducedMotion;
    pp.noFlash = s.accessibility.disableFlashes;
    particles.update (float (dt), choreographer.reactiveFrame(), clock.beatPhase(),
                      choreographer.activityLevel(), pp);

    if (FrameRateMode (s.frameRateMode) == FrameRateMode::Adaptive)
        updateFrameRate();

    repaint();
}

void StageComponent::drawBackground (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    using namespace theme;
    const Background bg = Background (cachedSettings.background);

    if (overlayMode && ! cachedSettings.overlay.showBackground)
        return;   // fully transparent overlay

    switch (bg)
    {
        case Background::Transparent:
            return;

        case Background::SolidDark:
            g.fillAll (shadow());
            return;

        case Background::LavenderGradient:
        {
            g.setGradientFill (juce::ColourGradient (deepPlum(), bounds.getCentreX(), bounds.getY(),
                                                     shadow(), bounds.getCentreX(), bounds.getBottom(),
                                                     false));
            g.fillAll();
            g.setGradientFill (juce::ColourGradient (
                lavender().withAlpha (0.16f), bounds.getCentreX(), bounds.getBottom() * 0.4f,
                lavender().withAlpha (0.0f), bounds.getCentreX(), bounds.getBottom(), true));
            g.fillAll();
            return;
        }

        case Background::GoldConstellation:
        case Background::NightSky:
        {
            g.setGradientFill (juce::ColourGradient (
                bg == Background::GoldConstellation ? deepPlum() : shadow().darker (0.3f),
                bounds.getCentreX(), bounds.getY(),
                shadow(), bounds.getCentreX(), bounds.getBottom(), false));
            g.fillAll();

            // Deterministic starfield + constellation lines.
            SeededRng stars (bg == Background::GoldConstellation ? 11u : 12u);
            juce::Point<float> previous;
            for (int i = 0; i < 46; ++i)
            {
                const juce::Point<float> pos (bounds.getX() + stars.nextFloat01() * bounds.getWidth(),
                                              bounds.getY() + stars.nextFloat01() * bounds.getHeight() * 0.85f);
                const float r = 0.8f + stars.nextFloat01() * 1.8f;
                const auto starColour = bg == Background::GoldConstellation
                                            ? softGold() : cream();
                g.setColour (starColour.withAlpha (0.25f + stars.nextFloat01() * 0.5f));
                g.fillEllipse (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f);
                if (bg == Background::GoldConstellation && i % 5 == 1 && i > 1)
                {
                    g.setColour (gold().withAlpha (0.12f));
                    g.drawLine ({ previous, pos }, 1.0f);
                }
                previous = pos;
            }
            return;
        }

        case Background::SoftStudio:
        {
            g.setGradientFill (juce::ColourGradient (panel().brighter (0.08f),
                                                     bounds.getCentreX(), bounds.getY(),
                                                     panelDeep(), bounds.getCentreX(),
                                                     bounds.getBottom(), false));
            g.fillAll();
            // Stage floor.
            g.setColour (shadow().withAlpha (0.5f));
            g.fillRect (bounds.removeFromBottom (bounds.getHeight() * 0.10f));
            return;
        }

        case Background::NeutralStudio:
            g.fillAll (juce::Colour (0xff1a1a1a));   // Ableton-friendly neutral
            return;

        case Background::UserImage:
        {
            if (userBackground.isValid())
            {
                g.drawImage (userBackground, bounds,
                             juce::RectanglePlacement::fillDestination);
            }
            else
            {
                // Safe fallback: neutral gradient + a gentle plain-language hint.
                g.setGradientFill (juce::ColourGradient (deepPlum(), 0, 0, shadow(),
                                                         0, bounds.getBottom(), false));
                g.fillAll();
                if (! overlayMode && userBackgroundFailed)
                {
                    g.setColour (cream().withAlpha (0.5f));
                    g.setFont (withDefaultMetrics (juce::FontOptions (12.0f)));
                    g.drawText ("Background image not found - using the default stage",
                                bounds.reduced (8.0f).removeFromBottom (18.0f),
                                juce::Justification::centred);
                }
            }
            return;
        }

        default:
            g.fillAll (panelDeep());
            return;
    }
}

void StageComponent::drawParticles (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    using namespace theme;
    const float unit = juce::jmin (bounds.getWidth(), bounds.getHeight());

    for (const auto& p : particles.particles())
    {
        if (! p.alive)
            continue;

        const float fade = clamp01 (p.life / juce::jmax (0.01f, p.maxLife));
        const juce::Point<float> pos (bounds.getCentreX() + p.pos.x * bounds.getWidth() * 0.5f,
                                      bounds.getCentreY() + p.pos.y * bounds.getHeight() * 0.5f);
        const float size = p.size * unit;

        switch (p.type)
        {
            case ParticleType::Star:
            case ParticleType::Sparkle:
            {
                const auto colour = (p.variant == 0 ? softGold()
                                     : p.variant == 1 ? lavender() : cream())
                                        .withAlpha (0.75f * fade);
                juce::Path star;
                star.addStar (pos, p.type == ParticleType::Sparkle ? 4 : 5,
                              size * 0.42f, size, p.rotation);
                g.setColour (colour);
                g.fillPath (star);
                break;
            }
            case ParticleType::Heart:
            {
                juce::Path heart;
                const float s = size;
                heart.startNewSubPath (pos.x, pos.y + s * 0.6f);
                heart.cubicTo (pos.x - s * 1.1f, pos.y - s * 0.2f,
                               pos.x - s * 0.5f, pos.y - s * 0.9f, pos.x, pos.y - s * 0.3f);
                heart.cubicTo (pos.x + s * 0.5f, pos.y - s * 0.9f,
                               pos.x + s * 1.1f, pos.y - s * 0.2f, pos.x, pos.y + s * 0.6f);
                g.setColour (rose().withAlpha (0.8f * fade));
                g.fillPath (heart);
                break;
            }
            case ParticleType::BeatRing:
            {
                g.setColour (lavender().withAlpha (0.4f * fade));
                const float r = size * 3.0f;
                g.drawEllipse (pos.x - r, pos.y - r * 0.35f, r * 2.0f, r * 0.7f,
                               2.0f + fade * 2.0f);
                break;
            }
            case ParticleType::Trail:
            {
                g.setColour (lavender().withAlpha (0.3f * fade));
                g.fillEllipse (pos.x - size, pos.y - size, size * 2.0f, size * 2.0f);
                break;
            }
            case ParticleType::Moon:
            {
                juce::Path moon;
                moon.addCentredArc (pos.x, pos.y, size, size, p.rotation, 0.6f,
                                    kPi + 2.2f, true);
                moon.addCentredArc (pos.x + size * 0.4f, pos.y, size * 0.75f, size * 0.75f,
                                    p.rotation, kPi + 2.0f, 0.8f, false);
                moon.closeSubPath();
                g.setColour (softGold().withAlpha (0.6f * fade));
                g.fillPath (moon);
                break;
            }
            default:
                break;
        }
    }
}

void StageComponent::drawStatusHints (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    using namespace theme;
    if (overlayMode)
        return;

    // FREE MODE badge when the host provides no timeline.
    if (clock.status() == SyncStatus::FreeMode)
    {
        const auto badge = bounds.reduced (10.0f).removeFromTop (20.0f).removeFromRight (86.0f);
        g.setColour (panel().withAlpha (0.85f));
        g.fillRoundedRectangle (badge, 10.0f);
        g.setColour (softGold());
        g.setFont (withDefaultMetrics (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText ("FREE MODE", badge, juce::Justification::centred);
    }

    // Beat pips along the bottom while synced and playing.
    if (clock.status() == SyncStatus::HostSync && clock.isPlaying()
        && ! cachedSettings.accessibility.disableFlashes)
    {
        const int beatsPerBar = clock.beatsPerBar();
        const int currentBeat = int (clock.barPhase() * beatsPerBar);
        const float pipSize = 6.0f;
        const float totalW = float (beatsPerBar) * (pipSize + 6.0f);
        float x = bounds.getCentreX() - totalW * 0.5f;
        const float y = bounds.getBottom() - 14.0f;
        for (int i = 0; i < beatsPerBar; ++i)
        {
            g.setColour (i == currentBeat ? gold() : lavender().withAlpha (0.35f));
            g.fillEllipse (x, y, pipSize, pipSize);
            x += pipSize + 6.0f;
        }
    }
}

void StageComponent::paint (juce::Graphics& g)
{
    const double paintStart = juce::Time::getMillisecondCounterHiRes();
    auto bounds = getLocalBounds().toFloat();

    drawBackground (g, bounds);

    // Camera framing: zoom about Lumi's upper body for closer modes.
    float zoom = 1.0f;
    float focusY = 0.0f;   // fraction of height to shift upward focus
    switch (CameraMode (cachedSettings.cameraMode))
    {
        case CameraMode::WaistUp: zoom = 1.55f; focusY = 0.18f; break;
        case CameraMode::CloseUp: zoom = 2.30f; focusY = 0.30f; break;
        case CameraMode::Auto:
            zoom = 1.0f + 0.45f * (1.0f - choreographer.activityLevel());
            focusY = 0.12f * (1.0f - choreographer.activityLevel());
            break;
        case CameraMode::Stage:   zoom = 0.9f; break;
        case CameraMode::FullBody:
        default: break;
    }

    const float opacity = clamp (processor.apvts.getRawParameterValue (
                                     params::visualOpacity)->load(), 0.1f, 1.0f);
    const float userScale = clamp (processor.apvts.getRawParameterValue (
                                       params::visualScale)->load(), 0.4f, 2.0f);
    const bool mirrored = processor.apvts.getRawParameterValue (params::mirror)->load() > 0.5f;

    {
        juce::Graphics::ScopedSaveState save (g);
        if (opacity < 0.999f)
            g.beginTransparencyLayer (opacity);

        if (zoom != 1.0f)
            g.addTransform (juce::AffineTransform::scale (
                zoom, zoom, bounds.getCentreX(),
                bounds.getCentreY() + bounds.getHeight() * focusY));

        drawParticles (g, bounds);

        RenderLook look;
        look.hair = HairPalette (cachedSettings.hairPalette);
        look.outfit = Outfit (cachedSettings.outfit);
        look.accent = GoldAccent (cachedSettings.goldAccent);
        look.accessories = cachedSettings.accessories;
        look.mirror = mirrored;
        look.highContrast = cachedSettings.accessibility.highContrast;
        look.timeSeconds = lastTickSeconds;

        auto lumiBounds = bounds.reduced (bounds.getWidth() * 0.5f * (1.0f - 0.62f * userScale),
                                          bounds.getHeight() * 0.5f * (1.0f - 0.94f * userScale));
        LumiRenderer::draw (g, currentPose, look, lumiBounds);

        if (opacity < 0.999f)
            g.endTransparencyLayer();
    }

    drawStatusHints (g, bounds);

    // Track paint cost for the adaptive frame-rate mode.
    const float paintMs = float (juce::Time::getMillisecondCounterHiRes() - paintStart);
    paintMsAverage += 0.1f * (paintMs - paintMsAverage);
}
