#include "ProcEngineMgr.h"
#include "EffectMgr.h"
#include "LoadoutMgr.h"
#include "ProficiencyMgr.h"
#include "branding/effects/EffectModel.h"
#include "branding/proc/ProcSpellMap.h"
#include "Configuration/Config.h"
#include "GameTime.h"
#include "Player.h"

namespace Branding
{
    namespace
    {
        uint64_t NowMs()
        {
            return static_cast<uint64_t>(GameTime::GetGameTimeMS().count());
        }
    }

    ProcEngineMgr* ProcEngineMgr::instance()
    {
        static ProcEngineMgr mgr;
        return &mgr;
    }

    void ProcEngineMgr::LoadConfig()
    {
        // The proc engine rides the §7.9 effect gate (ResolveActiveProfile), so it is inert unless
        // Branding.Effect.Enable is on too; this switch lets the melee proc slice be toggled alone.
        _enabled = sConfigMgr->GetOption<bool>("Branding.Proc.Enable", false);
        _meleePpm = sConfigMgr->GetOption<double>("Branding.Proc.MeleePpm", 2.0);
        _meleeBaseValue = sConfigMgr->GetOption<int32>("Branding.Proc.MeleeBaseValue", 250);
    }

    ProcResult ProcEngineMgr::ResolveMeleeProc(Player* attacker)
    {
        ProcResult none;
        if (!_enabled || !attacker)
            return none;

        // Same knowledge/expressibility gate as the §7.9 EffectMgr: fills the role-derived profile
        // (the window cadence) and returns false unless the CURRENT account can express the brand.
        EffectProfile profile;
        uint8_t level = 0;
        if (!sEffectMgr->ResolveActiveProfile(attacker, profile, level))
            return none;

        ObjectGuid const guid = attacker->GetGUID();
        uint32 const account = attacker->GetSession()->GetAccountId();
        BrandId const brand = sLoadoutMgr->GetLoadout(guid).activeBrand;

        ProcOpportunity opp;
        opp.ppm = _meleePpm;
        opp.weaponSpeedS = static_cast<double>(attacker->GetAttackTime(BASE_ATTACK)) / 1000.0;
        opp.effectStrength = sProficiencyMgr->EffectStrength(guid, account, brand);   // anti-P2W gated
        opp.windowActive = IsWindowActive(profile, NowMs());

        // #11 school->spell map: the shell to cast + its value model. A SpellDefault shell (absorb/aura)
        // carries its own magnitude, so it gets no scaled base override; a ScaledBase shell (the wired
        // damage shells) is fed the config base value, scaled by effect strength inside ResolveProc.
        MeleeProcSpell const shell = MeleeProcEntry(brand);
        opp.spellId = shell.spellId;
        opp.baseValue = shell.value == ProcValueModel::SpellDefault ? 0 : _meleeBaseValue;

        return ResolveProc(opp, _rng);
    }
}
