#include "ProcEngineMgr.h"
#include "mod_branding_loader.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"

using namespace Branding;

// §7.9.1 (GitHub #10): loads/refreshes the proc-engine config on startup and on `.reload config`.
class BrandingProcEngineWorldScript : public WorldScript
{
public:
    BrandingProcEngineWorldScript() : WorldScript("BrandingProcEngineWorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sProcEngineMgr->LoadConfig();
    }
};

// §7.9.1 proc-CASTING engine, MELEE slice. When a branded Player deals melee damage, the brand rolls
// against its PPM (gated by the §7.9 window + the anti-P2W expressibility check) and, on a fire, CASTS
// the resolved payload at the target -- reusing the overheal->shield cast pattern (EffectScripts.cpp).
//
// This is ADDITIVE: it casts a spell, it does NOT touch the melee damage number, so it composes with
// the existing §7.9/§14.12 multiplier scripts without disturbing them (retiring those is #12). The
// decision (whether/what) lives in the pure ProcEngine; this script is a dumb applier.
class BrandingProcEngineUnitScript : public UnitScript
{
public:
    BrandingProcEngineUnitScript() : UnitScript("BrandingProcEngineUnitScript") { }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& /*damage*/) override
    {
        if (!sProcEngineMgr->Enabled() || !attacker || !target)
            return;

        Player* player = attacker->ToPlayer();
        if (!player)
            return;

        ProcResult const proc = sProcEngineMgr->ResolveMeleeProc(player);
        if (!proc.fired)
            return;

        // §7.9 "the adapter applies the chosen proc to the actual weapon/spell": a triggered custom
        // cast with the resolved base-point override, aimed at the struck target.
        player->CastCustomSpell(proc.payload.spellId, SPELLVALUE_BASE_POINT0,
            proc.payload.basePoints, target, TRIGGERED_FULL_MASK);
    }
};

void AddBrandingProcEngineScripts()
{
    new BrandingProcEngineWorldScript();
    new BrandingProcEngineUnitScript();
}
