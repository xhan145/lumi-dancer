// LUMI//DANCER — the product palette and the customisation colour sets.
// Colours live in the JUCE-free core so customisation and serialisation can
// be unit-tested without a GUI; the UI layer converts Rgba to juce::Colour.
#pragma once

#include <array>
#include <cstdint>

namespace lumi
{
struct Rgba
{
    uint8_t r = 0, g = 0, b = 0, a = 255;

    constexpr bool operator== (const Rgba& o) const
    {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
};

constexpr Rgba rgb (uint32_t hex) // 0xRRGGBB
{
    return { uint8_t ((hex >> 16) & 0xff), uint8_t ((hex >> 8) & 0xff), uint8_t (hex & 0xff), 255 };
}

namespace palette
{
    // Product identity palette (docs/architecture.md documents usage rules).
    inline constexpr Rgba lavender   = rgb (0xB9A3FF);
    inline constexpr Rgba softPurple = rgb (0x8C6FE8);
    inline constexpr Rgba deepPlum   = rgb (0x302344);
    inline constexpr Rgba gold       = rgb (0xE4B84C);
    inline constexpr Rgba softGold   = rgb (0xF5D98D);
    inline constexpr Rgba cream      = rgb (0xFFF7E6);
    inline constexpr Rgba roseAccent = rgb (0xE9A6C7);
    inline constexpr Rgba shadow     = rgb (0x17121F);

    // Derived UI tones.
    inline constexpr Rgba plumPanel  = rgb (0x241A33);
    inline constexpr Rgba plumDeep   = rgb (0x1B1426);
    inline constexpr Rgba outline    = rgb (0x2A1E3D);
} // namespace palette

// ------------------------------------------------------- customisation sets
enum class HairPalette : int   { Lavender = 0, LightPurple, DeepPlum, CreamTips, Count };
enum class Outfit : int        { StarHoodie = 0, OrbitJacket, DreamDress, ProducerOutfit, CosmicStreetwear, Count };
enum class GoldAccent : int    { SoftGold = 0, RoseGold, PaleChampagne, Count };
enum class Background : int    { Transparent = 0, SolidDark, LavenderGradient, GoldConstellation,
                                 NightSky, SoftStudio, NeutralStudio, UserImage, Count };
enum class ExpressionTheme : int { Soft = 0, Cheerful, Confident, Sleepy, Hyper, Count };

// Accessory toggles, stored as a bitmask in the settings.
enum AccessoryBits : uint32_t
{
    accStarClip      = 1u << 0,
    accHeadphones    = 1u << 1,
    accCrescentPin   = 1u << 2,
    accOrbitBelt     = 1u << 3,
    accCompanionStar = 1u << 4,
};

struct HairColours { Rgba base, shade, highlight; };

inline constexpr std::array<HairColours, size_t (HairPalette::Count)> hairColours {{
    { rgb (0xB9A3FF), rgb (0x8C6FE8), rgb (0xE6DCFF) },  // Lavender
    { rgb (0xCDBBFF), rgb (0xA48FF0), rgb (0xF0E9FF) },  // Light purple
    { rgb (0x4A3766), rgb (0x302344), rgb (0x8C6FE8) },  // Deep plum
    { rgb (0xFFF7E6), rgb (0xE8D9C9), rgb (0xB9A3FF) },  // Cream with lavender tips
}};

struct AccentColours { Rgba main, soft; };

inline constexpr std::array<AccentColours, size_t (GoldAccent::Count)> accentColours {{
    { rgb (0xE4B84C), rgb (0xF5D98D) },  // Soft gold
    { rgb (0xE0A080), rgb (0xF2C9B0) },  // Rose gold
    { rgb (0xEED9A0), rgb (0xFBF0CE) },  // Pale champagne
}};

struct OutfitColours { Rgba jacket, jacketShade, top, skirt, boots; };

inline constexpr std::array<OutfitColours, size_t (Outfit::Count)> outfitColours {{
    { rgb (0xB9A3FF), rgb (0x8C6FE8), rgb (0xFFF7E6), rgb (0x8C6FE8), rgb (0xB9A3FF) },  // Star Hoodie
    { rgb (0x302344), rgb (0x241A33), rgb (0xFFF7E6), rgb (0x302344), rgb (0x302344) },  // Orbit Jacket
    { rgb (0xE9A6C7), rgb (0xD98BB2), rgb (0xFFF7E6), rgb (0xE9A6C7), rgb (0xFFF7E6) },  // Dream Dress
    { rgb (0x17121F), rgb (0x100C16), rgb (0xB9A3FF), rgb (0x17121F), rgb (0x17121F) },  // Producer Outfit
    { rgb (0x8C6FE8), rgb (0x6b52c4), rgb (0x302344), rgb (0xB9A3FF), rgb (0xFFF7E6) },  // Cosmic Streetwear
}};
} // namespace lumi
