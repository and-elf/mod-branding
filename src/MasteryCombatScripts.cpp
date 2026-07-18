#include "MasteryCombatMgr.h"
#include "mod_branding_loader.h"
#include "Player.h"
#include "ScriptMgr.h"

using namespace Branding;

// §14.12 (issue #27): refreshes the combat-mastery config on startup / `.reload config`. The config is
// owned by MasteryLoadoutMgr (the production MasteryConfig implements all three injected interfaces);
// this just rides its load + enable switch so the combat layer and the loadout layer stay in lockstep.
class BrandingMasteryCombatWorldScript : public WorldScript
{
public:
    BrandingMasteryCombatWorldScript() : WorldScript("BrandingMasteryCombatWorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sMasteryCombatMgr->LoadConfig();
    }
};

// §14.12 application: a branded player's OUTGOING damage is multiplied by the aggregate of their
// currently-active mastery cells (windowed Off/Def during their phase + sustained Support always on).
//
// §0/#12: like the §7.9 EffectMgr multiplier, this aggregate flat ±% is a DEPRECATED placeholder -- a
// flat ±% is never a primary brand lever (§0); the proc-casting engine (§7.9.1, #10) is the real
// expression. It is retired as the default: gated behind Branding.Effect.LegacyDamageMultiplier (off by
// default, the same single flag EffectMgr uses), a reversible escape hatch until #10 covers all
// triggers/brands, then to be deleted outright. The per-cell proc-cadence buff/debuff auras (PPM via
// ExpectedProcs + EventMap/TaskScheduler) and the personal-spike tank-survivability auras are the
// data-layer expansion; the testable decision (WHICH cells, HOW strong, WHEN active) lives in the pure
// MasteryPlan and is covered. Composes multiplicatively with §2.1 zone scaling (unaffected by the flag).
class BrandingMasteryCombatUnitScript : public UnitScript
{
public:
    BrandingMasteryCombatUnitScript() : UnitScript("BrandingMasteryCombatUnitScript") { }

    void ModifyMeleeDamage(Unit* /*target*/, Unit* attacker, uint32& damage) override
    {
        ApplyMastery(attacker, damage);
    }

    void ModifyPeriodicDamageAurasTick(Unit* /*target*/, Unit* attacker, uint32& damage, SpellInfo const* /*spellInfo*/) override
    {
        ApplyMastery(attacker, damage);
    }

    void ModifySpellDamageTaken(Unit* /*target*/, Unit* attacker, int32& damage, SpellInfo const* /*spellInfo*/) override
    {
        // §0/#12: the flat ±% multiplier is a DEPRECATED placeholder, off by default (the proc engine
        // §7.9.1 is the real lever). No-op unless the legacy escape hatch is explicitly enabled.
        if (!sMasteryCombatMgr->Enabled() || !sMasteryCombatMgr->LegacyDamageMultiplierEnabled() || !attacker)
            return;

        if (Player* player = attacker->ToPlayer())
            damage = static_cast<int32>(damage * sMasteryCombatMgr->OutgoingMultiplierFor(player));
    }

private:
    static void ApplyMastery(Unit* attacker, uint32& damage)
    {
        // §0/#12: deprecated flat ±% multiplier, off by default (see ModifySpellDamageTaken).
        if (!sMasteryCombatMgr->Enabled() || !sMasteryCombatMgr->LegacyDamageMultiplierEnabled() || !attacker)
            return;

        if (Player* player = attacker->ToPlayer())
            damage = static_cast<uint32>(damage * sMasteryCombatMgr->OutgoingMultiplierFor(player));
    }
};

void AddBrandingMasteryCombatScripts()
{
    new BrandingMasteryCombatWorldScript();
    new BrandingMasteryCombatUnitScript();
}
