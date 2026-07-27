// LUMI//DANCER — persisted plugin state (everything section 26 of the spec
// requires, nothing it forbids: no meter history, no raw audio, no images).
//
// Serialised as a versioned key=value text blob embedded in the host chunk.
// The parser is tolerant: unknown keys are ignored (forward compatibility),
// malformed values fall back to defaults, and a missing/garbage header
// yields pristine defaults with ok=false (corrupt-state fallback).
#pragma once

#include <cstdint>
#include <string>

#include "core/Palette.h"

namespace lumi
{
inline constexpr int kSettingsSchemaVersion = 1;

struct OverlaySettings
{
    bool  enabled      = false;
    bool  alwaysOnTop  = true;
    bool  clickThrough = false;
    bool  locked       = false;
    bool  showBackground = false;
    float opacity = 1.0f;
    float scale   = 1.0f;
    bool  mirror  = false;
    int   x = -1, y = -1;           // -1 = centre on first open
    int   w = 420, h = 520;
};

struct AccessibilitySettings
{
    bool  reducedMotion  = false;
    bool  disableFlashes = false;
    bool  highContrast   = false;
    float uiScale = 1.0f;           // 0.75 .. 1.5
};

enum class FrameRateMode : int { Fps30 = 0, Fps60, Adaptive, Count };
enum class CameraMode : int { FullBody = 0, WaistUp, CloseUp, Auto, Stage, Count };
enum class UiMode : int { Full = 0, Compact, Count };

struct LumiSettings
{
    int schemaVersion = kSettingsSchemaVersion;

    // Dance
    int      danceStyle = 0;                  // DanceStyle enum value
    uint64_t seed = 1;
    bool     seedLock = false;
    int      mood = 0;                        // ExpressionTheme

    // Constellation / routine
    bool  useRoutine = false;
    int   routineBars = 4;
    int   playbackMode = 0;                   // PlaybackMode
    float choreoComplexity = 0.75f;
    float choreoRepeatAvoidance = 0.7f;
    float choreoSurprise = 0.3f;
    float choreoEnergy = 0.5f;
    float choreoCuteness = 0.5f;
    std::string routineData;                  // serialised nodes (';' separated)

    // Reaction
    float intensity = 1.0f;
    float reactionAmount = 1.0f;
    float lowSens = 1.0f, midSens = 1.0f, highSens = 1.0f;
    float transientSens = 1.0f;
    float smoothing = 0.35f;
    bool  beatLock = false;

    // Appearance
    int      hairPalette = 0;                 // HairPalette
    int      outfit = 0;                      // Outfit
    int      goldAccent = 0;                  // GoldAccent
    uint32_t accessories = accStarClip | accOrbitBelt | accCompanionStar;
    int      background = 2;                  // Background::LavenderGradient
    std::string userImagePath;                // external path, never embedded

    // Effects
    float particleAmount = 0.6f;
    bool  effectsEnabled = true;

    // Idle
    bool  idleEnabled = true;
    float sleepDelaySeconds = 180.0f;
    float idleMotionAmount = 0.7f;

    // View
    float visualScale = 1.0f;
    bool  mirror = false;
    float visualOpacity = 1.0f;
    int   frameRateMode = int (FrameRateMode::Fps60);
    int   cameraMode = int (CameraMode::FullBody);
    int   uiMode = int (UiMode::Full);
    int   editorWidth = 900, editorHeight = 650;

    OverlaySettings overlay;
    AccessibilitySettings accessibility;
};

// Serialise to the versioned text blob.
std::string serializeSettings (const LumiSettings& settings);

// Parse; returns false (and resets `out` to defaults) when the blob is
// missing, corrupt, or from an incompatible future major schema.
bool deserializeSettings (const std::string& text, LumiSettings& out);

// Clamp every field into its legal range (used after parsing and by tests).
void clampSettings (LumiSettings& settings);
} // namespace lumi
