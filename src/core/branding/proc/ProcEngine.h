#ifndef MOD_BRANDING_CORE_PROC_PROCENGINE_H
#define MOD_BRANDING_CORE_PROC_PROCENGINE_H

#include "branding/common/Brand.h"
#include "branding/common/Rng.h"
#include <cstdint>

// Pure core (§7.9.1, GitHub #10). No AzerothCore includes anywhere under core/.
//
// The headline branding mechanic: a brand does NOT add flat +-% damage -- on a combat opportunity
// (this slice: a melee swing) it ROLLS against its procs-per-minute and, when it fires, casts a reused
// spell shell scaled by the character's effect strength. This is the pure DECISION layer -- whether a
// proc fires this opportunity and what payload the adapter must cast. Deterministic: the roll is
// injected (tests seed it; production draws from IRng), so there is no hidden std::rand / <random>.
namespace Branding
{
    // Per-swing proc probability for a PPM proc at a given weapon speed, in [0, 1]. Built on the §14.3
    // ExpectedProcs PPM normalization (density-independent) taken over exactly one swing window, so a
    // fast weapon gets many small chances and a slow weapon few large ones -- equal expected procs per
    // real second. Clamped: a high PPM on a slow weapon can exceed 1 expected proc/swing.
    double ProcChancePerSwing(double ppm, double weaponSpeedS);

    // What the adapter casts when a proc fires: a reused 3.3.5a spell shell + a scaled base value fed to
    // SPELLVALUE_BASE_POINT0 (mirrors the healer overheal->shield cast in EffectScripts.cpp). A
    // basePoints of 0 means "cast with the spell's own effect values".
    struct ProcPayload
    {
        uint32_t spellId = 0;
        int32_t  basePoints = 0;
    };

    // Everything the decision needs as a plain POD (no AC types). The adapter fills it from the live
    // state it can read: PPM (mastery/effect cadence), the attacker's weapon speed, the character's
    // [0, 1] effect strength (§7.3, anti-P2W gated to 0 when the brand is not expressible), the window
    // gate (§7.9 "no passive uptime"), and the resolved payload spell + its unscaled base value.
    struct ProcOpportunity
    {
        double   ppm = 0.0;
        double   weaponSpeedS = 1.0;
        double   effectStrength = 0.0;   // [0, 1]; scales baseValue and gates the proc (0 => never)
        bool     windowActive = true;    // §7.9 window state; false => no proc this opportunity
        uint32_t spellId = 0;            // resolved payload shell (0 => no proc); see MeleeProcSpellId
        int32_t  baseValue = 0;          // unscaled base points; scaled by effectStrength in the payload
    };

    struct ProcResult
    {
        bool        fired = false;
        ProcPayload payload{};
    };

    // Decide + resolve for one opportunity. `roll` is a uniform value in [0, 1) the CALLER supplies. A
    // proc fires iff the window is active, the payload is castable (spellId != 0) and the brand is
    // expressible (effectStrength > 0), and roll < ProcChancePerSwing(ppm, weaponSpeedS). The payload
    // base points are baseValue scaled by effectStrength (clamped to [0, 1]). Deterministic.
    ProcResult ResolveProc(ProcOpportunity const& opp, double roll);

    // Convenience overload: draws the roll from an injected IRng (FakeRng in tests, ServerRng in prod)
    // and delegates to the roll overload. Kept in core so the RNG stays an injected seam (§8).
    ProcResult ResolveProc(ProcOpportunity const& opp, IRng& rng);

    // §7.9 proc payload SOURCE (melee slice). Convenience seam over the full brand/school->spell map
    // (#11, branding/proc/ProcSpellMap.h): returns just the reused spell shell the engine casts when a
    // proc fires. Classic schools reuse their §14.4 Offensive lattice shell; the exotic schools are
    // wired where a cast-at-victim shell exists. 0 => no proc for this school. Pure data. Callers that
    // also need the value model use MeleeProcEntry.
    uint32_t MeleeProcSpellId(BrandId brand);
}

#endif // MOD_BRANDING_CORE_PROC_PROCENGINE_H
