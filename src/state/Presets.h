// LUMI//DANCER — factory presets. Every preset carries meaningful values
// (style, mood, reaction shaping, look, effects, accessibility) — never just
// a renamed default.
#pragma once

#include <string>
#include <vector>

#include "state/Settings.h"

namespace lumi
{
struct FactoryPreset
{
    std::string name;
    std::string category;    // "General", "Music Styles", "Visual"
    LumiSettings settings;
};

const std::vector<FactoryPreset>& factoryPresets();

// Index into factoryPresets() by name; -1 when absent.
int findFactoryPreset (const std::string& name);
} // namespace lumi
