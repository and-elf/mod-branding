#ifndef MOD_BRANDING_CORE_PROC_PROCSPELLMAP_H
#define MOD_BRANDING_CORE_PROC_PROCSPELLMAP_H

#include "branding/common/Brand.h"
#include <cstdint>

// Pure core (§7.9.1, GitHub #11). No AzerothCore includes anywhere under core/.
//
// The §7.9.1 proc engine casts a reused 3.3.5a spell shell when a melee proc fires. This is the SOURCE
// of that payload: the brand/school -> { spell shell, value model } map that replaces the Fire-only
// stub. It stays pure so it is unit-testable, and the engine's MeleeProcSpellId + the melee adapter
// (ProcEngineMgr) consume it.
namespace Branding
{
    // How the engine derives the payload's base points for a school's reused spell shell. Carried
    // alongside the spell id so a shell that supplies its own magnitude (an absorb/aura) is never fed a
    // raw damage override.
    enum class ProcValueModel : uint8_t
    {
        None = 0,     // no melee proc wired for this school (spellId 0 => the ResolveProc gate is inert)
        ScaledBase,   // feed the config base value into the shell, scaled by the [0, 1] effect strength
        SpellDefault  // cast the shell with its OWN effect values (payload base points 0); extensible
    };

    // One school's melee-proc payload source: the reused spell shell + its value model. A default (0 /
    // None) is a school the melee slice does not proc for.
    struct MeleeProcSpell
    {
        uint32_t       spellId = 0;
        ProcValueModel value = ProcValueModel::None;
    };

    // §7.9.1 melee-proc SPELL MAP (GitHub #11). Maps a brand/school to the reused spell shell the proc
    // engine casts at the victim when a melee proc fires. Pure data (no AC types).
    //
    //  - CLASSIC schools (Fire..Physical): fully wired. The on-swing proc reuses the school's authored
    //    §14.4 OFFENSIVE lattice shell (docs/issues/30-classic-school-spell-map.md) via
    //    LatticeSpellId(school, Offensive, 0) -- a SINGLE source of truth, so the proc payload and the
    //    mastery lattice cannot drift. All are direct-damage shells -> ProcValueModel::ScaledBase.
    //  - EXOTIC schools (Wind..Spirit, §7.10): structured separately and extensibly. Those whose
    //    docs/issues/16-exotic-school-spell-map.md DPS-role shell is a clean cast-at-the-victim
    //    offensive spell are wired PROVISIONALLY (Wind/Lightning/Blood/Spirit). The rest stay unwired
    //    (spellId 0) because a self-buff / summon / disease-spread shell is not a valid cast-at-victim
    //    melee payload; they light up when the §7.10 exotic tree projection lands (doc 16 "Remaining").
    MeleeProcSpell MeleeProcEntry(BrandId brand);
}

#endif // MOD_BRANDING_CORE_PROC_PROCSPELLMAP_H
