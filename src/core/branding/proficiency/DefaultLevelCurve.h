#ifndef MOD_BRANDING_CORE_PROFICIENCY_DEFAULTLEVELCURVE_H
#define MOD_BRANDING_CORE_PROFICIENCY_DEFAULTLEVELCURVE_H

#include <cstdint>

namespace Branding
{
    // Shipped default level curve (§7.4). Single source of truth for the production defaults: the
    // BrandingConfig adapter uses these as its sConfigMgr fallbacks and the curve tests pin the same
    // numbers, so retuning the ladder touches exactly one place.
    //
    // These are a *playtest-perceptible starting point* (issue #14): the first ranks accrue within a
    // single post-cap session so progression is visible, while the geometric ladder still steepens
    // toward the cap. Final production balance (the ~3-month time-to-Prestige target of §14.13.6) is
    // owned by the #14 xp-balance sim, which may re-derive these against the representative play
    // profile. The values are denominated in Proficiency units (the curve's own currency), not raw
    // player XP -- the post-cap adapter converts XP into units before it reaches the core (§14.13.3).
    namespace DefaultLevelCurve
    {
        inline constexpr double RankBaseXp = 1000.0;      // Proficiency units for rank 1 (§7.4)
        inline constexpr double RankGrowth = 1.01;         // +1%/rank geometric ladder (§14.13.6)
        inline constexpr uint8_t MaxLevel = 50;            // Prestige cap
    }
}

#endif // MOD_BRANDING_CORE_PROFICIENCY_DEFAULTLEVELCURVE_H
