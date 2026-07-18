#include "BrandingConfig.h"
#include "branding/proficiency/DefaultLevelCurve.h"
#include "Configuration/Config.h"

namespace Branding
{
    void BrandingConfig::Load()
    {
        _enabled = sConfigMgr->GetOption<bool>("Branding.Enable", false);

        _matchBonus = sConfigMgr->GetOption<float>("Branding.Xp.MatchBonus", 1.25f);
        _drSoftCap = sConfigMgr->GetOption<uint32_t>("Branding.Xp.DrSoftCap", 50000);
        _drFloor = sConfigMgr->GetOption<float>("Branding.Xp.DrFloor", 0.1f);
        _drSlope = sConfigMgr->GetOption<float>("Branding.Xp.DrSlope", 0.00001f);
        _drWindowSeconds = sConfigMgr->GetOption<uint32_t>("Branding.Xp.DrWindowSeconds", 3600);

        _rankBaseXp = sConfigMgr->GetOption<float>("Branding.Level.RankBaseXp", static_cast<float>(DefaultLevelCurve::RankBaseXp));
        _rankGrowth = sConfigMgr->GetOption<float>("Branding.Level.RankGrowth", static_cast<float>(DefaultLevelCurve::RankGrowth));
        _maxLevel = static_cast<uint8_t>(sConfigMgr->GetOption<uint32_t>("Branding.Level.Max", DefaultLevelCurve::MaxLevel));
    }
}
