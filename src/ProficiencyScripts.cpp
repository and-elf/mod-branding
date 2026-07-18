#include "ProficiencyMgr.h"
#include "mod_branding_loader.h"
#include "Player.h"
#include "ScriptMgr.h"

// Loads/refreshes branding config on startup and on `.reload config`.
class BrandingWorldScript : public WorldScript
{
public:
    BrandingWorldScript() : WorldScript("BrandingWorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sProficiencyMgr->LoadConfig();
    }
};

// Player lifecycle only. Proficiency is *earned* through the single intended source path -- post-cap
// XP redirected into the active brand (§14.13.3, PostCapXpScripts.cpp). There is deliberately no
// per-kill or other ad-hoc earn here: kills accrue Proficiency only via the XP they already grant,
// once that XP is redirected at max player level. Additional activity sources (invasions, raids,
// gathering) plug into the same post-cap redirect / contribution tracker rather than a parallel hook.
class BrandingProficiencyPlayerScript : public PlayerScript
{
public:
    BrandingProficiencyPlayerScript() : PlayerScript("BrandingProficiencyPlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        sProficiencyMgr->LoadPlayer(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        sProficiencyMgr->SavePlayer(player);
        sProficiencyMgr->UnloadPlayer(player->GetGUID());
    }
};

void AddBrandingProficiencyScripts()
{
    new BrandingWorldScript();
    new BrandingProficiencyPlayerScript();
}
