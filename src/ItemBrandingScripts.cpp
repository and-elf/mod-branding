#include "ItemBrandingMgr.h"
#include "mod_branding_loader.h"
#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"

using namespace Branding;

// Loads/refreshes item-branding config on startup and on `.reload config`.
class BrandingItemWorldScript : public WorldScript
{
public:
    BrandingItemWorldScript() : WorldScript("BrandingItemWorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sItemBrandingMgr->LoadConfig();
    }
};

// Loads equipped items' brand state on login, and refreshes the cache when any item is equipped
// mid-session (so the crafted per-slot lookups (#1) and the Etch aggregate / gates stay correct
// without a relog, #31).
class BrandingItemPlayerScript : public PlayerScript
{
public:
    BrandingItemPlayerScript() : PlayerScript("BrandingItemPlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        sItemBrandingMgr->LoadEquipped(player);
    }

    void OnPlayerEquip(Player* /*player*/, Item* it, uint8 bag, uint8 slot, bool /*update*/) override
    {
        // Any equipped slot may now carry a Brand (crafted armour/weapon, #1, or an Etched item, #31).
        if (bag == INVENTORY_SLOT_BAG_0 && slot < EQUIPMENT_SLOT_END)
            sItemBrandingMgr->CacheItem(it);
    }
};

void AddBrandingItemScripts()
{
    new BrandingItemWorldScript();
    new BrandingItemPlayerScript();
}
