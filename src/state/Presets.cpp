#include "state/Presets.h"

namespace lumi
{
namespace
{
LumiSettings base()
{
    return LumiSettings {};
}

std::vector<FactoryPreset> buildPresets()
{
    std::vector<FactoryPreset> presets;
    const auto add = [&presets] (const char* name, const char* category,
                                 LumiSettings s)
    {
        clampSettings (s);
        presets.push_back ({ name, category, std::move (s) });
    };

    // ------------------------------------------------------------- General
    {
        LumiSettings s = base();
        add ("Lumi Default", "General", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 0;                       // Bounce
        s.intensity = 0.7f;
        s.smoothing = 0.55f;
        s.particleAmount = 0.35f;
        s.mood = int (ExpressionTheme::Soft);
        add ("Soft Bounce", "General", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 1;                       // Kawaii Pop
        s.mood = int (ExpressionTheme::Cheerful);
        s.intensity = 1.2f;
        s.particleAmount = 0.8f;
        s.hairPalette = int (HairPalette::LightPurple);
        s.outfit = int (Outfit::DreamDress);
        add ("Kawaii Pop", "General", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 2;                       // Orbit
        s.goldAccent = int (GoldAccent::SoftGold);
        s.background = int (Background::GoldConstellation);
        s.accessories |= accOrbitBelt | accCompanionStar | accCrescentPin;
        s.particleAmount = 0.7f;
        add ("Golden Orbit", "General", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 3;                       // Groove
        s.background = int (Background::LavenderGradient);
        s.midSens = 1.4f;
        s.mood = int (ExpressionTheme::Confident);
        add ("Lavender Groove", "General", s);
    }
    {
        LumiSettings s = base();
        s.visualScale = 0.65f;
        s.danceStyle = 0;
        s.intensity = 0.9f;
        s.particleAmount = 0.3f;
        s.uiMode = int (UiMode::Compact);
        add ("Tiny Dancer", "General", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 1;
        s.background = int (Background::SoftStudio);
        s.cameraMode = int (CameraMode::Stage);
        s.particleAmount = 0.9f;
        s.mood = int (ExpressionTheme::Confident);
        add ("Stage Star", "General", s);
    }

    // -------------------------------------------------------- Music Styles
    {
        LumiSettings s = base();
        s.danceStyle = 5;                       // Chill
        s.intensity = 0.6f;
        s.smoothing = 0.8f;
        s.transientSens = 0.5f;
        s.background = int (Background::NightSky);
        s.mood = int (ExpressionTheme::Sleepy);
        add ("Ambient Sway", "Music Styles", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 8;                       // Trance
        s.highSens = 1.4f;
        s.background = int (Background::GoldConstellation);
        s.particleAmount = 0.85f;
        s.mood = int (ExpressionTheme::Cheerful);
        add ("Trance Angel", "Music Styles", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 7;                       // Drum & Bass
        s.transientSens = 1.5f;
        s.midSens = 1.3f;
        s.intensity = 1.2f;
        add ("Drum and Bass Steps", "Music Styles", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 6;                       // Breakcore
        s.transientSens = 1.6f;
        s.intensity = 1.3f;
        s.particleAmount = 0.75f;
        s.mood = int (ExpressionTheme::Hyper);
        s.outfit = int (Outfit::CosmicStreetwear);
        add ("Breakcore Sprite", "Music Styles", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 7;
        s.lowSens = 1.5f;
        s.smoothing = 0.2f;
        s.intensity = 1.1f;
        s.outfit = int (Outfit::ProducerOutfit);
        add ("Neurofunk Bounce", "Music Styles", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 4;                       // Hyper
        s.intensity = 1.5f;
        s.highSens = 1.5f;
        s.particleAmount = 1.0f;
        s.mood = int (ExpressionTheme::Hyper);
        s.hairPalette = int (HairPalette::CreamTips);
        add ("Hyperpop Sparkle", "Music Styles", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 5;
        s.intensity = 0.75f;
        s.smoothing = 0.65f;
        s.background = int (Background::SoftStudio);
        s.mood = int (ExpressionTheme::Soft);
        add ("Chill Lo-Fi", "Music Styles", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 0;
        s.lowSens = 1.6f;
        s.intensity = 1.3f;
        s.smoothing = 0.25f;
        s.hairPalette = int (HairPalette::DeepPlum);
        s.mood = int (ExpressionTheme::Confident);
        add ("Metal Head Nod", "Music Styles", s);
    }
    {
        LumiSettings s = base();
        s.danceStyle = 3;
        s.lowSens = 1.3f;
        s.smoothing = 0.45f;
        s.intensity = 0.95f;
        s.outfit = int (Outfit::CosmicStreetwear);
        add ("Rap Groove", "Music Styles", s);
    }

    // -------------------------------------------------------------- Visual
    {
        LumiSettings s = base();
        s.background = int (Background::Transparent);
        s.overlay.enabled = true;
        s.overlay.showBackground = false;
        s.particleAmount = 0.4f;
        add ("Transparent Overlay", "Visual", s);
    }
    {
        LumiSettings s = base();
        s.background = int (Background::GoldConstellation);
        s.goldAccent = int (GoldAccent::PaleChampagne);
        s.particleAmount = 0.7f;
        s.accessories |= accCrescentPin | accCompanionStar;
        add ("Gold Constellation", "Visual", s);
    }
    {
        LumiSettings s = base();
        s.background = int (Background::LavenderGradient);
        s.hairPalette = int (HairPalette::Lavender);
        s.particleAmount = 0.55f;
        s.mood = int (ExpressionTheme::Soft);
        add ("Lavender Dream", "Visual", s);
    }
    {
        LumiSettings s = base();
        s.background = int (Background::SolidDark);
        s.particleAmount = 0.1f;
        s.effectsEnabled = true;
        s.cameraMode = int (CameraMode::WaistUp);
        add ("Minimal Stage", "Visual", s);
    }
    {
        LumiSettings s = base();
        s.particleAmount = 1.0f;
        s.background = int (Background::NightSky);
        s.accessories = accStarClip | accOrbitBelt | accCompanionStar | accCrescentPin;
        add ("Maximum Sparkles", "Visual", s);
    }
    {
        LumiSettings s = base();
        s.accessibility.reducedMotion = true;
        s.accessibility.disableFlashes = true;
        s.particleAmount = 0.15f;
        s.smoothing = 0.75f;
        s.intensity = 0.8f;
        add ("Reduced Motion", "Visual", s);
    }

    return presets;
}
} // namespace

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = buildPresets();
    return presets;
}

int findFactoryPreset (const std::string& name)
{
    const auto& presets = factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        if (presets[i].name == name)
            return int (i);
    return -1;
}
} // namespace lumi
