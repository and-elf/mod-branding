#include "branding/proc/ProcEngine.h"
#include "branding/proc/ProcSpellMap.h"
#include "branding/mastery/MasteryTrees.h"
#include <algorithm>

namespace Branding
{
    double ProcChancePerSwing(double ppm, double weaponSpeedS)
    {
        if (ppm <= 0.0 || weaponSpeedS <= 0.0)
        {
            return 0.0;
        }

        // One swing's worth of the density-independent PPM model (§14.3 ExpectedProcs); over exactly
        // one swing window the expected count IS this swing's proc probability. Clamp -- a high PPM on
        // a slow weapon can exceed 1 expected proc/swing, but a single swing fires at most once.
        uint32_t const oneSwingMs = static_cast<uint32_t>(weaponSpeedS * 1000.0);
        return std::clamp(ExpectedProcs(ppm, oneSwingMs, weaponSpeedS), 0.0, 1.0);
    }

    ProcResult ResolveProc(ProcOpportunity const& opp, double roll)
    {
        ProcResult result;

        // §7.9 gates: outside its window, nothing wired to cast, or a non-expressible brand (anti-P2W
        // EffectStrength resolved to 0) -> the brand is inert this opportunity.
        if (!opp.windowActive || opp.spellId == 0 || opp.effectStrength <= 0.0)
        {
            return result;
        }

        double const chance = ProcChancePerSwing(opp.ppm, opp.weaponSpeedS);
        if (chance <= 0.0 || roll >= chance)
        {
            return result;
        }

        double const strength = std::clamp(opp.effectStrength, 0.0, 1.0);
        result.fired = true;
        result.payload.spellId = opp.spellId;
        result.payload.basePoints = static_cast<int32_t>(static_cast<double>(opp.baseValue) * strength);
        return result;
    }

    ProcResult ResolveProc(ProcOpportunity const& opp, IRng& rng)
    {
        // Draw a uniform roll in [0, 1) at a fixed resolution; deterministic under a seeded FakeRng.
        constexpr uint32_t kResolution = 1000000u;
        double const roll = static_cast<double>(rng.Next(kResolution)) / static_cast<double>(kResolution);
        return ResolveProc(opp, roll);
    }

    uint32_t MeleeProcSpellId(BrandId brand)
    {
        // #11: delegates to the full school->spell map (ProcSpellMap). Convenience seam that yields just
        // the shell id; callers needing the value model call MeleeProcEntry directly. Unwired schools
        // return 0 => no proc fires (the ResolveProc spellId gate).
        return MeleeProcEntry(brand).spellId;
    }
}
