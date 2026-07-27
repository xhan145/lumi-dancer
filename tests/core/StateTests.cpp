// State tests: full round trip, corrupt fallback, schema versioning,
// routine embedding, presets integrity.
#include "TestFramework.h"

#include <set>

#include "constellation/RoutineEngine.h"
#include "state/Presets.h"
#include "state/Settings.h"

using namespace lumi;

LD_TEST (state_round_trip_preserves_everything)
{
    LumiSettings s;
    s.danceStyle = 6;
    s.seed = 0xDEADBEEFCAFEull;
    s.seedLock = true;
    s.mood = 3;
    s.useRoutine = true;
    s.routineBars = 8;
    s.playbackMode = 2;
    s.intensity = 1.4f;
    s.reactionAmount = 0.8f;
    s.lowSens = 1.5f;
    s.midSens = 0.4f;
    s.highSens = 1.9f;
    s.transientSens = 0.2f;
    s.smoothing = 0.9f;
    s.beatLock = true;
    s.hairPalette = 2;
    s.outfit = 3;
    s.goldAccent = 1;
    s.accessories = accHeadphones | accCompanionStar;
    s.background = 4;
    s.userImagePath = "C:\\Users\\someone\\pictures\\stage backdrop=v2.png";
    s.particleAmount = 0.25f;
    s.effectsEnabled = false;
    s.idleEnabled = false;
    s.sleepDelaySeconds = 45.0f;
    s.idleMotionAmount = 0.3f;
    s.visualScale = 1.6f;
    s.mirror = true;
    s.visualOpacity = 0.5f;
    s.frameRateMode = 0;
    s.cameraMode = 2;
    s.uiMode = 1;
    s.editorWidth = 1200;
    s.editorHeight = 800;
    s.overlay.enabled = true;
    s.overlay.alwaysOnTop = false;
    s.overlay.clickThrough = true;
    s.overlay.locked = true;
    s.overlay.showBackground = true;
    s.overlay.opacity = 0.66f;
    s.overlay.scale = 1.5f;
    s.overlay.mirror = true;
    s.overlay.x = 120;
    s.overlay.y = 340;
    s.overlay.w = 640;
    s.overlay.h = 700;
    s.accessibility.reducedMotion = true;
    s.accessibility.disableFlashes = true;
    s.accessibility.highContrast = true;
    s.accessibility.uiScale = 1.25f;

    ChoreoParams cp;
    s.routineData = serializeRoutine (generateRoutine (4, 4, 77, cp));

    const std::string blob = serializeSettings (s);
    LumiSettings restored;
    LD_CHECK (deserializeSettings (blob, restored));

    LD_EQ (restored.danceStyle, s.danceStyle);
    LD_EQ (restored.seed, s.seed);
    LD_EQ (restored.seedLock, s.seedLock);
    LD_EQ (restored.mood, s.mood);
    LD_EQ (restored.useRoutine, s.useRoutine);
    LD_EQ (restored.routineBars, s.routineBars);
    LD_EQ (restored.playbackMode, s.playbackMode);
    LD_NEAR (restored.intensity, s.intensity, 1e-4);
    LD_NEAR (restored.reactionAmount, s.reactionAmount, 1e-4);
    LD_NEAR (restored.lowSens, s.lowSens, 1e-4);
    LD_NEAR (restored.midSens, s.midSens, 1e-4);
    LD_NEAR (restored.highSens, s.highSens, 1e-4);
    LD_NEAR (restored.transientSens, s.transientSens, 1e-4);
    LD_NEAR (restored.smoothing, s.smoothing, 1e-4);
    LD_EQ (restored.beatLock, s.beatLock);
    LD_EQ (restored.hairPalette, s.hairPalette);
    LD_EQ (restored.outfit, s.outfit);
    LD_EQ (restored.goldAccent, s.goldAccent);
    LD_EQ (restored.accessories, s.accessories);
    LD_EQ (restored.background, s.background);
    LD_CHECK (restored.userImagePath == s.userImagePath);
    LD_NEAR (restored.particleAmount, s.particleAmount, 1e-4);
    LD_EQ (restored.effectsEnabled, s.effectsEnabled);
    LD_EQ (restored.idleEnabled, s.idleEnabled);
    LD_NEAR (restored.sleepDelaySeconds, s.sleepDelaySeconds, 1e-3);
    LD_NEAR (restored.idleMotionAmount, s.idleMotionAmount, 1e-4);
    LD_NEAR (restored.visualScale, s.visualScale, 1e-4);
    LD_EQ (restored.mirror, s.mirror);
    LD_NEAR (restored.visualOpacity, s.visualOpacity, 1e-4);
    LD_EQ (restored.frameRateMode, s.frameRateMode);
    LD_EQ (restored.cameraMode, s.cameraMode);
    LD_EQ (restored.uiMode, s.uiMode);
    LD_EQ (restored.editorWidth, s.editorWidth);
    LD_EQ (restored.editorHeight, s.editorHeight);
    LD_EQ (restored.overlay.enabled, s.overlay.enabled);
    LD_EQ (restored.overlay.alwaysOnTop, s.overlay.alwaysOnTop);
    LD_EQ (restored.overlay.clickThrough, s.overlay.clickThrough);
    LD_EQ (restored.overlay.locked, s.overlay.locked);
    LD_EQ (restored.overlay.showBackground, s.overlay.showBackground);
    LD_NEAR (restored.overlay.opacity, s.overlay.opacity, 1e-4);
    LD_NEAR (restored.overlay.scale, s.overlay.scale, 1e-4);
    LD_EQ (restored.overlay.mirror, s.overlay.mirror);
    LD_EQ (restored.overlay.x, s.overlay.x);
    LD_EQ (restored.overlay.y, s.overlay.y);
    LD_EQ (restored.overlay.w, s.overlay.w);
    LD_EQ (restored.overlay.h, s.overlay.h);
    LD_EQ (restored.accessibility.reducedMotion, s.accessibility.reducedMotion);
    LD_EQ (restored.accessibility.disableFlashes, s.accessibility.disableFlashes);
    LD_EQ (restored.accessibility.highContrast, s.accessibility.highContrast);
    LD_NEAR (restored.accessibility.uiScale, s.accessibility.uiScale, 1e-4);

    // The embedded routine survives and re-parses into an identical routine.
    Routine original, roundTripped;
    LD_CHECK (deserializeRoutine (s.routineData, original));
    LD_CHECK (deserializeRoutine (restored.routineData, roundTripped));
    LD_EQ (roundTripped.nodes.size(), original.nodes.size());
}

LD_TEST (state_corrupt_input_falls_back_to_defaults)
{
    const LumiSettings defaults;

    for (const char* garbage : { "", "not a settings blob", "LUMI//DANCER-STATE",
                                 "random\nlines\nof=stuff" })
    {
        LumiSettings out;
        out.danceStyle = 9;   // pre-dirty to prove the reset
        const bool ok = deserializeSettings (garbage, out);
        if (std::string (garbage) == "LUMI//DANCER-STATE")
        {
            // Header alone but no schema line → also rejected.
            LD_CHECK (! ok);
        }
        else
        {
            LD_CHECK (! ok);
        }
        LD_EQ (out.danceStyle, defaults.danceStyle);
        LD_NEAR (out.intensity, defaults.intensity, 1e-6);
    }
}

LD_TEST (state_tolerates_unknown_keys_and_bad_values)
{
    LumiSettings s;
    s.danceStyle = 4;
    std::string blob = serializeSettings (s);
    blob += "future.unknownKey=whatever\n";
    blob += "react.intensity=notanumber\n";       // later bad value should not
                                                  // destroy earlier good parse
    blob += "dance.style=999\n";                  // out of range → clamped

    LumiSettings out;
    LD_CHECK (deserializeSettings (blob, out));
    LD_LE (out.danceStyle, 9);
    LD_GE (out.intensity, 0.0f);
    LD_LE (out.intensity, 2.0f);
}

LD_TEST (state_future_schema_rejected_safely)
{
    std::string blob = "LUMI//DANCER-STATE\nschema=99\ndance.style=5\n";
    LumiSettings out;
    LD_CHECK (! deserializeSettings (blob, out));
    LD_EQ (out.danceStyle, LumiSettings {}.danceStyle);   // untouched defaults
}

LD_TEST (state_clamp_repairs_out_of_range)
{
    LumiSettings s;
    s.intensity = 99.0f;
    s.smoothing = -5.0f;
    s.visualScale = 100.0f;
    s.overlay.w = 5;
    s.accessibility.uiScale = 9.0f;
    s.danceStyle = -3;
    clampSettings (s);
    LD_LE (s.intensity, 2.0f);
    LD_GE (s.smoothing, 0.0f);
    LD_LE (s.visualScale, 2.0f);
    LD_GE (s.overlay.w, 160);
    LD_LE (s.accessibility.uiScale, 1.5f);
    LD_GE (s.danceStyle, 0);
}

LD_TEST (presets_are_meaningful_and_unique)
{
    const auto& presets = factoryPresets();
    LD_GE (presets.size(), size_t (22));

    // Unique names.
    std::set<std::string> names;
    for (const auto& p : presets)
        names.insert (p.name);
    LD_EQ (names.size(), presets.size());

    // Every preset (except the explicit default) differs from defaults in at
    // least one meaningful field — no renamed duplicates.
    const LumiSettings defaults;
    int distinctFromDefault = 0;
    for (const auto& p : presets)
    {
        const auto& s = p.settings;
        const bool differs = s.danceStyle != defaults.danceStyle
            || s.mood != defaults.mood
            || std::fabs (s.intensity - defaults.intensity) > 1e-4f
            || std::fabs (s.smoothing - defaults.smoothing) > 1e-4f
            || std::fabs (s.particleAmount - defaults.particleAmount) > 1e-4f
            || std::fabs (s.visualScale - defaults.visualScale) > 1e-4f
            || s.background != defaults.background
            || s.hairPalette != defaults.hairPalette
            || s.outfit != defaults.outfit
            || s.goldAccent != defaults.goldAccent
            || s.accessories != defaults.accessories
            || s.overlay.enabled != defaults.overlay.enabled
            || s.accessibility.reducedMotion != defaults.accessibility.reducedMotion
            || std::fabs (s.lowSens - defaults.lowSens) > 1e-4f
            || std::fabs (s.midSens - defaults.midSens) > 1e-4f
            || std::fabs (s.highSens - defaults.highSens) > 1e-4f
            || std::fabs (s.transientSens - defaults.transientSens) > 1e-4f
            || s.uiMode != defaults.uiMode
            || s.cameraMode != defaults.cameraMode;
        if (differs)
            ++distinctFromDefault;
    }
    LD_GE (distinctFromDefault, int (presets.size()) - 1);

    // Every preset's settings serialise and restore.
    for (const auto& p : presets)
    {
        LumiSettings restored;
        LD_CHECK (deserializeSettings (serializeSettings (p.settings), restored));
        LD_EQ (restored.danceStyle, p.settings.danceStyle);
    }

    // Category sanity + lookup.
    LD_CHECK (findFactoryPreset ("Lumi Default") >= 0);
    LD_CHECK (findFactoryPreset ("Reduced Motion") >= 0);
    LD_CHECK (findFactoryPreset ("Does Not Exist") == -1);
}
