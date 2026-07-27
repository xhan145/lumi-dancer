#include "state/Settings.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "core/LumiMath.h"

namespace lumi
{
namespace
{
constexpr const char* kHeader = "LUMI//DANCER-STATE";

std::string sanitizeValue (const std::string& v)
{
    // Values are single-line; strip anything that would break the format.
    std::string out;
    out.reserve (v.size());
    for (char c : v)
        if (c != '\n' && c != '\r')
            out += c;
    return out;
}

int toInt (const std::string& v, int fallback)
{
    char* end = nullptr;
    const long parsed = std::strtol (v.c_str(), &end, 10);
    if (end == v.c_str())
        return fallback;
    return int (parsed);
}

uint64_t toU64 (const std::string& v, uint64_t fallback)
{
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull (v.c_str(), &end, 10);
    if (end == v.c_str())
        return fallback;
    return uint64_t (parsed);
}

float toFloat (const std::string& v, float fallback)
{
    char* end = nullptr;
    const double parsed = std::strtod (v.c_str(), &end);
    if (end == v.c_str() || ! std::isfinite (parsed))
        return fallback;
    return float (parsed);
}

bool toBool (const std::string& v, bool fallback)
{
    if (v == "1" || v == "true")  return true;
    if (v == "0" || v == "false") return false;
    return fallback;
}

void appendKv (std::string& out, const char* key, const std::string& value)
{
    out += key;
    out += '=';
    out += sanitizeValue (value);
    out += '\n';
}

void appendKv (std::string& out, const char* key, int v)      { appendKv (out, key, std::to_string (v)); }
void appendKv (std::string& out, const char* key, uint64_t v) { appendKv (out, key, std::to_string (v)); }
void appendKv (std::string& out, const char* key, bool v)     { appendKv (out, key, std::string (v ? "1" : "0")); }
void appendKv (std::string& out, const char* key, float v)
{
    char buf[48];
    std::snprintf (buf, sizeof (buf), "%.6g", double (v));
    appendKv (out, key, std::string (buf));
}
} // namespace

void clampSettings (LumiSettings& s)
{
    s.danceStyle = clamp (s.danceStyle, 0, 9);
    s.mood = clamp (s.mood, 0, int (ExpressionTheme::Count) - 1);
    s.routineBars = clamp (s.routineBars, 1, 64);
    s.playbackMode = clamp (s.playbackMode, 0, 3);
    s.choreoComplexity = clamp01 (sanitize (s.choreoComplexity, 0.75f));
    s.choreoRepeatAvoidance = clamp01 (sanitize (s.choreoRepeatAvoidance, 0.7f));
    s.choreoSurprise = clamp01 (sanitize (s.choreoSurprise, 0.3f));
    s.choreoEnergy = clamp01 (sanitize (s.choreoEnergy, 0.5f));
    s.choreoCuteness = clamp01 (sanitize (s.choreoCuteness, 0.5f));

    s.intensity = clamp (sanitize (s.intensity, 1.0f), 0.0f, 2.0f);
    s.reactionAmount = clamp (sanitize (s.reactionAmount, 1.0f), 0.0f, 2.0f);
    s.lowSens = clamp (sanitize (s.lowSens, 1.0f), 0.0f, 2.0f);
    s.midSens = clamp (sanitize (s.midSens, 1.0f), 0.0f, 2.0f);
    s.highSens = clamp (sanitize (s.highSens, 1.0f), 0.0f, 2.0f);
    s.transientSens = clamp (sanitize (s.transientSens, 1.0f), 0.0f, 2.0f);
    s.smoothing = clamp01 (sanitize (s.smoothing, 0.35f));

    s.hairPalette = clamp (s.hairPalette, 0, int (HairPalette::Count) - 1);
    s.outfit = clamp (s.outfit, 0, int (Outfit::Count) - 1);
    s.goldAccent = clamp (s.goldAccent, 0, int (GoldAccent::Count) - 1);
    s.background = clamp (s.background, 0, int (Background::Count) - 1);

    s.particleAmount = clamp01 (sanitize (s.particleAmount, 0.6f));
    s.sleepDelaySeconds = clamp (sanitize (s.sleepDelaySeconds, 180.0f), 10.0f, 3600.0f);
    s.idleMotionAmount = clamp01 (sanitize (s.idleMotionAmount, 0.7f));

    s.visualScale = clamp (sanitize (s.visualScale, 1.0f), 0.4f, 2.0f);
    s.visualOpacity = clamp (sanitize (s.visualOpacity, 1.0f), 0.1f, 1.0f);
    s.frameRateMode = clamp (s.frameRateMode, 0, int (FrameRateMode::Count) - 1);
    s.cameraMode = clamp (s.cameraMode, 0, int (CameraMode::Count) - 1);
    s.uiMode = clamp (s.uiMode, 0, int (UiMode::Count) - 1);
    s.editorWidth = clamp (s.editorWidth, 380, 3840);
    s.editorHeight = clamp (s.editorHeight, 300, 2400);

    s.overlay.opacity = clamp (sanitize (s.overlay.opacity, 1.0f), 0.15f, 1.0f);
    s.overlay.scale = clamp (sanitize (s.overlay.scale, 1.0f), 0.3f, 3.0f);
    s.overlay.w = clamp (s.overlay.w, 160, 2400);
    s.overlay.h = clamp (s.overlay.h, 160, 2400);
    s.overlay.x = clamp (s.overlay.x, -1, 16384);
    s.overlay.y = clamp (s.overlay.y, -1, 16384);

    s.accessibility.uiScale = clamp (sanitize (s.accessibility.uiScale, 1.0f), 0.75f, 1.5f);
}

std::string serializeSettings (const LumiSettings& s)
{
    std::string out;
    out.reserve (1200);
    out += kHeader;
    out += '\n';
    appendKv (out, "schema", s.schemaVersion);

    appendKv (out, "dance.style", s.danceStyle);
    appendKv (out, "dance.seed", s.seed);
    appendKv (out, "dance.seedLock", s.seedLock);
    appendKv (out, "dance.mood", s.mood);

    appendKv (out, "routine.enabled", s.useRoutine);
    appendKv (out, "routine.bars", s.routineBars);
    appendKv (out, "routine.playback", s.playbackMode);
    appendKv (out, "routine.complexity", s.choreoComplexity);
    appendKv (out, "routine.repeatAvoid", s.choreoRepeatAvoidance);
    appendKv (out, "routine.surprise", s.choreoSurprise);
    appendKv (out, "routine.energy", s.choreoEnergy);
    appendKv (out, "routine.cuteness", s.choreoCuteness);
    {
        // Routine nodes are newline-separated internally; embed on one line.
        std::string flat = s.routineData;
        std::replace (flat.begin(), flat.end(), '\n', ';');
        appendKv (out, "routine.nodes", flat);
    }

    appendKv (out, "react.intensity", s.intensity);
    appendKv (out, "react.amount", s.reactionAmount);
    appendKv (out, "react.low", s.lowSens);
    appendKv (out, "react.mid", s.midSens);
    appendKv (out, "react.high", s.highSens);
    appendKv (out, "react.transient", s.transientSens);
    appendKv (out, "react.smoothing", s.smoothing);
    appendKv (out, "react.beatLock", s.beatLock);

    appendKv (out, "look.hair", s.hairPalette);
    appendKv (out, "look.outfit", s.outfit);
    appendKv (out, "look.accent", s.goldAccent);
    appendKv (out, "look.accessories", uint64_t (s.accessories));
    appendKv (out, "look.background", s.background);
    appendKv (out, "look.userImage", s.userImagePath);

    appendKv (out, "fx.particles", s.particleAmount);
    appendKv (out, "fx.enabled", s.effectsEnabled);

    appendKv (out, "idle.enabled", s.idleEnabled);
    appendKv (out, "idle.sleepDelay", s.sleepDelaySeconds);
    appendKv (out, "idle.motion", s.idleMotionAmount);

    appendKv (out, "view.scale", s.visualScale);
    appendKv (out, "view.mirror", s.mirror);
    appendKv (out, "view.opacity", s.visualOpacity);
    appendKv (out, "view.frameRate", s.frameRateMode);
    appendKv (out, "view.camera", s.cameraMode);
    appendKv (out, "view.uiMode", s.uiMode);
    appendKv (out, "view.editorW", s.editorWidth);
    appendKv (out, "view.editorH", s.editorHeight);

    appendKv (out, "overlay.enabled", s.overlay.enabled);
    appendKv (out, "overlay.onTop", s.overlay.alwaysOnTop);
    appendKv (out, "overlay.clickThrough", s.overlay.clickThrough);
    appendKv (out, "overlay.locked", s.overlay.locked);
    appendKv (out, "overlay.background", s.overlay.showBackground);
    appendKv (out, "overlay.opacity", s.overlay.opacity);
    appendKv (out, "overlay.scale", s.overlay.scale);
    appendKv (out, "overlay.mirror", s.overlay.mirror);
    appendKv (out, "overlay.x", s.overlay.x);
    appendKv (out, "overlay.y", s.overlay.y);
    appendKv (out, "overlay.w", s.overlay.w);
    appendKv (out, "overlay.h", s.overlay.h);

    appendKv (out, "a11y.reducedMotion", s.accessibility.reducedMotion);
    appendKv (out, "a11y.noFlash", s.accessibility.disableFlashes);
    appendKv (out, "a11y.highContrast", s.accessibility.highContrast);
    appendKv (out, "a11y.uiScale", s.accessibility.uiScale);

    return out;
}

bool deserializeSettings (const std::string& text, LumiSettings& out)
{
    out = LumiSettings {};

    std::istringstream stream (text);
    std::string line;
    if (! std::getline (stream, line))
        return false;
    // Tolerate trailing CR from CRLF round-trips.
    while (! line.empty() && (line.back() == '\r' || line.back() == ' '))
        line.pop_back();
    if (line != kHeader)
        return false;

    bool sawSchema = false;
    while (std::getline (stream, line))
    {
        while (! line.empty() && line.back() == '\r')
            line.pop_back();
        const size_t eq = line.find ('=');
        if (eq == std::string::npos || eq == 0)
            continue;   // tolerate junk lines
        const std::string key = line.substr (0, eq);
        const std::string value = line.substr (eq + 1);

        if (key == "schema")
        {
            out.schemaVersion = toInt (value, -1);
            sawSchema = true;
            if (out.schemaVersion < 1 || out.schemaVersion > kSettingsSchemaVersion)
            {
                // Unknown future schema: fall back to safe defaults.
                out = LumiSettings {};
                return false;
            }
        }
        else if (key == "dance.style")        out.danceStyle = toInt (value, out.danceStyle);
        else if (key == "dance.seed")         out.seed = toU64 (value, out.seed);
        else if (key == "dance.seedLock")     out.seedLock = toBool (value, out.seedLock);
        else if (key == "dance.mood")         out.mood = toInt (value, out.mood);
        else if (key == "routine.enabled")    out.useRoutine = toBool (value, out.useRoutine);
        else if (key == "routine.bars")       out.routineBars = toInt (value, out.routineBars);
        else if (key == "routine.playback")   out.playbackMode = toInt (value, out.playbackMode);
        else if (key == "routine.complexity") out.choreoComplexity = toFloat (value, out.choreoComplexity);
        else if (key == "routine.repeatAvoid") out.choreoRepeatAvoidance = toFloat (value, out.choreoRepeatAvoidance);
        else if (key == "routine.surprise")   out.choreoSurprise = toFloat (value, out.choreoSurprise);
        else if (key == "routine.energy")     out.choreoEnergy = toFloat (value, out.choreoEnergy);
        else if (key == "routine.cuteness")   out.choreoCuteness = toFloat (value, out.choreoCuteness);
        else if (key == "routine.nodes")
        {
            std::string nodes = value;
            std::replace (nodes.begin(), nodes.end(), ';', '\n');
            out.routineData = nodes;
        }
        else if (key == "react.intensity")    out.intensity = toFloat (value, out.intensity);
        else if (key == "react.amount")       out.reactionAmount = toFloat (value, out.reactionAmount);
        else if (key == "react.low")          out.lowSens = toFloat (value, out.lowSens);
        else if (key == "react.mid")          out.midSens = toFloat (value, out.midSens);
        else if (key == "react.high")         out.highSens = toFloat (value, out.highSens);
        else if (key == "react.transient")    out.transientSens = toFloat (value, out.transientSens);
        else if (key == "react.smoothing")    out.smoothing = toFloat (value, out.smoothing);
        else if (key == "react.beatLock")     out.beatLock = toBool (value, out.beatLock);
        else if (key == "look.hair")          out.hairPalette = toInt (value, out.hairPalette);
        else if (key == "look.outfit")        out.outfit = toInt (value, out.outfit);
        else if (key == "look.accent")        out.goldAccent = toInt (value, out.goldAccent);
        else if (key == "look.accessories")   out.accessories = uint32_t (toU64 (value, out.accessories));
        else if (key == "look.background")    out.background = toInt (value, out.background);
        else if (key == "look.userImage")     out.userImagePath = value;
        else if (key == "fx.particles")       out.particleAmount = toFloat (value, out.particleAmount);
        else if (key == "fx.enabled")         out.effectsEnabled = toBool (value, out.effectsEnabled);
        else if (key == "idle.enabled")       out.idleEnabled = toBool (value, out.idleEnabled);
        else if (key == "idle.sleepDelay")    out.sleepDelaySeconds = toFloat (value, out.sleepDelaySeconds);
        else if (key == "idle.motion")        out.idleMotionAmount = toFloat (value, out.idleMotionAmount);
        else if (key == "view.scale")         out.visualScale = toFloat (value, out.visualScale);
        else if (key == "view.mirror")        out.mirror = toBool (value, out.mirror);
        else if (key == "view.opacity")       out.visualOpacity = toFloat (value, out.visualOpacity);
        else if (key == "view.frameRate")     out.frameRateMode = toInt (value, out.frameRateMode);
        else if (key == "view.camera")        out.cameraMode = toInt (value, out.cameraMode);
        else if (key == "view.uiMode")        out.uiMode = toInt (value, out.uiMode);
        else if (key == "view.editorW")       out.editorWidth = toInt (value, out.editorWidth);
        else if (key == "view.editorH")       out.editorHeight = toInt (value, out.editorHeight);
        else if (key == "overlay.enabled")    out.overlay.enabled = toBool (value, out.overlay.enabled);
        else if (key == "overlay.onTop")      out.overlay.alwaysOnTop = toBool (value, out.overlay.alwaysOnTop);
        else if (key == "overlay.clickThrough") out.overlay.clickThrough = toBool (value, out.overlay.clickThrough);
        else if (key == "overlay.locked")     out.overlay.locked = toBool (value, out.overlay.locked);
        else if (key == "overlay.background") out.overlay.showBackground = toBool (value, out.overlay.showBackground);
        else if (key == "overlay.opacity")    out.overlay.opacity = toFloat (value, out.overlay.opacity);
        else if (key == "overlay.scale")      out.overlay.scale = toFloat (value, out.overlay.scale);
        else if (key == "overlay.mirror")     out.overlay.mirror = toBool (value, out.overlay.mirror);
        else if (key == "overlay.x")          out.overlay.x = toInt (value, out.overlay.x);
        else if (key == "overlay.y")          out.overlay.y = toInt (value, out.overlay.y);
        else if (key == "overlay.w")          out.overlay.w = toInt (value, out.overlay.w);
        else if (key == "overlay.h")          out.overlay.h = toInt (value, out.overlay.h);
        else if (key == "a11y.reducedMotion") out.accessibility.reducedMotion = toBool (value, out.accessibility.reducedMotion);
        else if (key == "a11y.noFlash")       out.accessibility.disableFlashes = toBool (value, out.accessibility.disableFlashes);
        else if (key == "a11y.highContrast")  out.accessibility.highContrast = toBool (value, out.accessibility.highContrast);
        else if (key == "a11y.uiScale")       out.accessibility.uiScale = toFloat (value, out.accessibility.uiScale);
        // Unknown keys: ignored on purpose (newer minor versions).
    }

    if (! sawSchema)
    {
        out = LumiSettings {};
        return false;
    }

    out.schemaVersion = kSettingsSchemaVersion;   // migrated forward
    clampSettings (out);
    return true;
}
} // namespace lumi
