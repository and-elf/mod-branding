#include "branding/proc/ProcSpellMap.h"
#include "branding/proc/ProcEngine.h"
#include "branding/mastery/MasteryContent.h"
#include "branding/mastery/MasteryTrees.h"
#include <gtest/gtest.h>

using namespace Branding;

// ---- The #11 melee-proc school->spell map ----------------------------------------------------------
//
// The classic schools reuse their authored §14.4 OFFENSIVE lattice shell (docs/issues/30) as the
// on-swing melee proc: one source of truth (LatticeSpellId), so the proc payload and the mastery
// lattice can never drift. The exotic schools (§7.10) are structured separately and wired only where a
// clean cast-at-the-victim offensive shell exists in docs/issues/16.

TEST(MeleeProcMap, ClassicSchoolsWiredToOffensiveLatticeShell)
{
    struct Case { BrandId brand; uint32_t spellId; };
    Case const cases[] = {
        { BrandId::Fire,     42833u },  // Fireball
        { BrandId::Frost,    122u   },  // Frost Nova
        { BrandId::Nature,   57061u },  // Poison Cloud
        { BrandId::Shadow,   55850u },  // Shadow Bolt Volley
        { BrandId::Arcane,   42921u },  // Arcane Explosion
        { BrandId::Holy,     15237u },  // Holy Nova
        { BrandId::Physical, 845u   },  // Cleave
    };

    for (auto const& c : cases)
    {
        MeleeProcSpell const entry = MeleeProcEntry(c.brand);
        EXPECT_EQ(c.spellId, entry.spellId) << "school ordinal " << static_cast<int>(c.brand);
        EXPECT_EQ(ProcValueModel::ScaledBase, entry.value) << "school ordinal " << static_cast<int>(c.brand);
        // Lockstep with the mastery lattice's Offensive cell (single source of truth).
        EXPECT_EQ(LatticeSpellId(c.brand, MasteryTree::Offensive, 0), entry.spellId);
    }
}

TEST(MeleeProcMap, ExoticWiredSchoolsAreProvisionalDamageShells)
{
    // PROVISIONAL: the doc-16 DPS-role shell for each of these is a clean cast-at-target offensive spell.
    struct Case { BrandId brand; uint32_t spellId; };
    Case const cases[] = {
        { BrandId::Wind,      25504u },  // Windfury Attack
        { BrandId::Lightning, 421u   },  // Chain Lightning
        { BrandId::Blood,     47855u },  // Drain Soul
        { BrandId::Spirit,    1120u  },  // Drain Soul (spirit rank)
    };

    for (auto const& c : cases)
    {
        MeleeProcSpell const entry = MeleeProcEntry(c.brand);
        EXPECT_EQ(c.spellId, entry.spellId) << "school ordinal " << static_cast<int>(c.brand);
        EXPECT_EQ(ProcValueModel::ScaledBase, entry.value) << "school ordinal " << static_cast<int>(c.brand);
    }
}

TEST(MeleeProcMap, DeferredExoticSchoolsReturnNoPayload)
{
    // No clean cast-at-victim offensive shell exists yet (self-aura / summon / disease-spread); the
    // structure stays extensible -- these light up when the §7.10 exotic tree projection lands.
    for (BrandId const brand : { BrandId::Void, BrandId::Stone, BrandId::Venom, BrandId::Chrono })
    {
        MeleeProcSpell const entry = MeleeProcEntry(brand);
        EXPECT_EQ(0u, entry.spellId) << "school ordinal " << static_cast<int>(brand);
        EXPECT_EQ(ProcValueModel::None, entry.value) << "school ordinal " << static_cast<int>(brand);
    }
}

TEST(MeleeProcMap, WiredEntriesNameASpellAndUnwiredNeverDo)
{
    // Invariant: a wired shell (value != None) always names a concrete spell, and an unwired entry
    // never does -- so ResolveProc's spellId gate and the value model can never disagree.
    for (uint8_t i = 0; i < static_cast<uint8_t>(BrandId::COUNT); ++i)
    {
        MeleeProcSpell const entry = MeleeProcEntry(static_cast<BrandId>(i));
        if (entry.value == ProcValueModel::None)
            EXPECT_EQ(0u, entry.spellId) << "school ordinal " << static_cast<int>(i);
        else
            EXPECT_NE(0u, entry.spellId) << "school ordinal " << static_cast<int>(i);
    }
}

TEST(MeleeProcMap, MeleeProcSpellIdMirrorsTheMapEntry)
{
    // The ProcEngine convenience seam yields exactly the map entry's spell id for every school.
    for (uint8_t i = 0; i < static_cast<uint8_t>(BrandId::COUNT); ++i)
    {
        BrandId const brand = static_cast<BrandId>(i);
        EXPECT_EQ(MeleeProcEntry(brand).spellId, MeleeProcSpellId(brand))
            << "school ordinal " << static_cast<int>(i);
    }
}
