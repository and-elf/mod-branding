#include "branding/proc/ProcSpellMap.h"
#include "branding/mastery/MasteryContent.h"
#include "branding/mastery/MasteryTrees.h"

namespace Branding
{
    namespace
    {
        // A classic school's on-swing melee proc reuses its authored §14.4 OFFENSIVE lattice shell
        // (docs/issues/30) -- one source of truth, fed the config base value scaled by effect strength.
        MeleeProcSpell ClassicOffensive(BrandId school)
        {
            return MeleeProcSpell{ LatticeSpellId(school, MasteryTree::Offensive, 0), ProcValueModel::ScaledBase };
        }
    }

    MeleeProcSpell MeleeProcEntry(BrandId brand)
    {
        switch (brand)
        {
            // ---- Classic schools: the §14.4 Offensive lattice shell (docs/issues/30). ---------------
            //   Fire -> Fireball 42833 · Frost -> Frost Nova 122 · Nature -> Poison Cloud 57061 ·
            //   Shadow -> Shadow Bolt Volley 55850 · Arcane -> Arcane Explosion 42921 ·
            //   Holy -> Holy Nova 15237 · Physical -> Cleave 845.
            case BrandId::Fire:
            case BrandId::Frost:
            case BrandId::Nature:
            case BrandId::Shadow:
            case BrandId::Arcane:
            case BrandId::Holy:
            case BrandId::Physical:
                return ClassicOffensive(brand);

            // ---- Exotic schools (§7.10): PROVISIONAL, doc-16 DPS-role shells that cast at the victim.
            // These are the exotic schools whose docs/issues/16 DPS-role pick is a clean cast-at-target
            // offensive spell; the tree-axis projection (doc 16 "Remaining") will finalize them.
            case BrandId::Wind:      return MeleeProcSpell{ 25504u, ProcValueModel::ScaledBase };  // Windfury Attack (PROVISIONAL)
            case BrandId::Lightning: return MeleeProcSpell{ 421u,   ProcValueModel::ScaledBase };  // Chain Lightning (PROVISIONAL)
            case BrandId::Blood:     return MeleeProcSpell{ 47855u, ProcValueModel::ScaledBase };  // Drain Soul (PROVISIONAL)
            case BrandId::Spirit:    return MeleeProcSpell{ 1120u,  ProcValueModel::ScaledBase };  // Drain Soul, spirit rank (PROVISIONAL)

            // ---- Exotic schools without a clean cast-at-victim offensive shell: unwired (extensible).
            //   Void  -> doc-16 DPS = Cloak of Shadows (self-aura)   -- not a target payload
            //   Stone -> doc-16 DPS = Earthbind Totem (summon)       -- not a target payload
            //   Venom -> doc-16 DPS = Pestilence (needs applied DoTs) -- deferred to the tree projection
            //   Chrono-> doc-16 DPS = Mirror Image (self summon)     -- not a target payload
            case BrandId::Void:
            case BrandId::Stone:
            case BrandId::Venom:
            case BrandId::Chrono:
            default:
                return MeleeProcSpell{};   // spellId 0, ProcValueModel::None => no proc this school
        }
    }
}
