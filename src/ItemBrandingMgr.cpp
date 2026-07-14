#include "ItemBrandingMgr.h"
#include "CatalystMgr.h"
#include "LoadoutMgr.h"
#include "ProficiencyMgr.h"
#include "branding/catalyst/CatalystStacking.h"
#include "branding/effects/ItemBrand.h"
#include "branding/proficiency/Knowledge.h"
#include "Configuration/Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Player.h"

namespace
{
    // Etch-eligible equipment slots (proc-surface: weapons + trinkets, #31 first cut).
    constexpr uint8 EtchSlots[] = { EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND,
        EQUIPMENT_SLOT_RANGED, EQUIPMENT_SLOT_TRINKET1, EQUIPMENT_SLOT_TRINKET2 };

    // Slots a *crafted* Branded item (#1) occupies: the 9 armour slots + main-hand + ranged (the
    // hunter's weapon). The crafted brand/upgrade path accepts these; off-hand/trinkets stay Etch-only.
    constexpr uint8 BrandableSlots[] = {
        EQUIPMENT_SLOT_HEAD, EQUIPMENT_SLOT_SHOULDERS, EQUIPMENT_SLOT_CHEST, EQUIPMENT_SLOT_WAIST,
        EQUIPMENT_SLOT_LEGS, EQUIPMENT_SLOT_FEET, EQUIPMENT_SLOT_WRISTS, EQUIPMENT_SLOT_HANDS,
        EQUIPMENT_SLOT_BACK, EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_RANGED };
}

namespace Branding
{
    ItemBrandingMgr* ItemBrandingMgr::instance()
    {
        static ItemBrandingMgr mgr;
        return &mgr;
    }

    void ItemBrandingMgr::LoadConfig()
    {
        _enabled = sConfigMgr->GetOption<bool>("Branding.Item.Enable", false);
        _etchEnabled = sConfigMgr->GetOption<bool>("Branding.Etch.Enable", false);
        _essenceItemId = sConfigMgr->GetOption<uint32>("Branding.Etch.EssenceItemId", 190002);
        _essenceCost = sConfigMgr->GetOption<uint32>("Branding.Etch.EssenceCost", 500);
        _config.Load();
    }

    uint32_t ItemBrandingMgr::ItemGuidAtSlot(Player* player, uint8_t equipSlot) const
    {
        if (!player)
            return 0;

        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlot);
        return item ? item->GetGUID().GetCounter() : 0;
    }

    bool ItemBrandingMgr::EtchEligibleSlot(uint8_t equipSlot)
    {
        for (uint8 slot : EtchSlots)
            if (slot == equipSlot)
                return true;

        return false;
    }

    bool ItemBrandingMgr::IsBrandableSlot(uint8_t equipSlot)
    {
        for (uint8 slot : BrandableSlots)
            if (slot == equipSlot)
                return true;

        return false;
    }

    bool ItemBrandingMgr::ParseEquipSlot(std::string_view token, uint8_t& equipSlot)
    {
        // A superset of the brandable slots (+ off-hand) so the command layer can distinguish an
        // unknown token from a known-but-not-brandable one. String-literal keys have static storage,
        // so the string_view keys stay valid.
        static std::unordered_map<std::string_view, uint8_t> const slots = {
            { "head", EQUIPMENT_SLOT_HEAD },
            { "shoulders", EQUIPMENT_SLOT_SHOULDERS }, { "shoulder", EQUIPMENT_SLOT_SHOULDERS },
            { "chest", EQUIPMENT_SLOT_CHEST },
            { "waist", EQUIPMENT_SLOT_WAIST }, { "belt", EQUIPMENT_SLOT_WAIST },
            { "legs", EQUIPMENT_SLOT_LEGS },
            { "feet", EQUIPMENT_SLOT_FEET }, { "boots", EQUIPMENT_SLOT_FEET },
            { "wrists", EQUIPMENT_SLOT_WRISTS }, { "wrist", EQUIPMENT_SLOT_WRISTS },
            { "hands", EQUIPMENT_SLOT_HANDS }, { "gloves", EQUIPMENT_SLOT_HANDS },
            { "back", EQUIPMENT_SLOT_BACK }, { "cloak", EQUIPMENT_SLOT_BACK },
            { "mainhand", EQUIPMENT_SLOT_MAINHAND }, { "mh", EQUIPMENT_SLOT_MAINHAND },
            { "offhand", EQUIPMENT_SLOT_OFFHAND }, { "oh", EQUIPMENT_SLOT_OFFHAND },
            { "ranged", EQUIPMENT_SLOT_RANGED }, { "rg", EQUIPMENT_SLOT_RANGED },
        };

        auto it = slots.find(token);
        if (it == slots.end())
            return false;

        equipSlot = it->second;
        return true;
    }

    void ItemBrandingMgr::Save(uint32_t itemGuid, ItemBrandState const& state)
    {
        CharacterDatabase.Execute(
            "REPLACE INTO `item_branding` (`item_guid`, `brand`, `step`, `level_in_step`, `etched`) "
            "VALUES ({}, {}, {}, {}, {})",
            itemGuid, static_cast<uint32>(state.brand), static_cast<uint32>(state.step),
            static_cast<uint32>(state.levelInStep), static_cast<uint32>(state.etched ? 1 : 0));
    }

    void ItemBrandingMgr::CacheBrandState(uint32_t itemGuid)
    {
        if (itemGuid == 0 || _items.find(itemGuid) != _items.end())
            return;

        QueryResult result = CharacterDatabase.Query(
            "SELECT `brand`, `step`, `level_in_step`, `etched` FROM `item_branding` WHERE `item_guid` = {}",
            itemGuid);
        if (!result)
            return;

        Field* fields = result->Fetch();
        ItemBrandState state;
        state.brand = static_cast<BrandId>(fields[0].Get<uint8>());
        state.step = fields[1].Get<uint8>();
        state.levelInStep = fields[2].Get<uint8>();
        state.etched = fields[3].Get<uint8>() != 0;
        _items[itemGuid] = state;
    }

    void ItemBrandingMgr::LoadEquipped(Player* player)
    {
        if (!_enabled || !player)
            return;

        // Cache the brand state of EVERY equipped item (armour, weapons, trinkets), so both the crafted
        // per-slot lookups (#1) and the Etch multi-slot aggregate (#31) see them without a relog. A
        // slot with no brand row is a cheap no-op in CacheBrandState.
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            CacheBrandState(ItemGuidAtSlot(player, slot));
    }

    void ItemBrandingMgr::CacheItem(Item* item)
    {
        if (!_enabled || !item)
            return;

        // Mid-session equip refresh: an item branded earlier and equipped now lands in the cache so the
        // multi-slot aggregate and the active-school gate stay correct without a relog (#31).
        CacheBrandState(item->GetGUID().GetCounter());
    }

    bool ItemBrandingMgr::BrandEquipped(Player* player, BrandId brand, uint8_t equipSlot)
    {
        if (!IsBrandableSlot(equipSlot))
            return false;

        uint32_t const itemGuid = ItemGuidAtSlot(player, equipSlot);
        if (itemGuid == 0)
            return false;

        ItemBrandState state;
        state.brand = brand;
        _items[itemGuid] = state;
        Save(itemGuid, state);
        return true;
    }

    uint8_t ItemBrandingMgr::UpgradeEquipped(Player* player, uint32_t resources, uint8_t equipSlot)
    {
        uint32_t const itemGuid = ItemGuidAtSlot(player, equipSlot);
        if (itemGuid == 0)
            return 0;

        // Ensure the item's persisted brand state is in the cache before we take a reference: without
        // this, operator[] on an uncached-but-branded item would insert a fresh rank-0 state and the
        // Save() below would wipe its real rank. (LoadEquipped/CacheItem usually cover this; belt-and-braces.)
        CacheBrandState(itemGuid);

        ItemBrandState& state = _items[itemGuid];
        ItemUpgradeResult const result = ApplyItemUpgrade(state, resources, _config);
        Save(itemGuid, state);
        return result.levelsGained;
    }

    double ItemBrandingMgr::EquippedIntensity(Player* player, uint8_t equipSlot) const
    {
        uint32_t const itemGuid = ItemGuidAtSlot(player, equipSlot);
        auto it = _items.find(itemGuid);
        if (itemGuid == 0 || it == _items.end())
            return 1.0;

        bool canExpress = player &&
            CanExpressBrand(it->second.brand, sProficiencyMgr->AccountKnowledge(player->GetSession()->GetAccountId()));

        // Etch (#31, decision 2): an Etched item's proc expresses ONLY while its brand is the player's
        // active school -- a stronger gate than account Knowledge alone. Switch active school and it goes
        // dormant. (Crafted Branded items keep the §05 Knowledge-only gate, unchanged.)
        if (canExpress && it->second.etched)
            canExpress = sLoadoutMgr->GetLoadout(player->GetGUID()).activeBrand == it->second.brand;

        return ResolvedItemEffectIntensity(it->second, canExpress, _config);
    }

    EtchResult ItemBrandingMgr::EtchSlot(Player* player, uint8_t equipSlot)
    {
        if (!EtchEnabled())
            return EtchResult::Disabled;

        if (!EtchEligibleSlot(equipSlot))
            return EtchResult::NoWeapon;

        uint32_t const itemGuid = ItemGuidAtSlot(player, equipSlot);
        if (itemGuid == 0)
            return EtchResult::NoWeapon;

        // One Etch per item, permanent: refuse if this item already carries any Brand (etched or crafted).
        // Authoritative DB check, not just the cache -- the cache only holds items loaded at login, so an
        // item branded earlier and equipped mid-session would otherwise be re-etched (double-charging the
        // premium Essence). A rare command, so a blocking lookup is fine.
        if (_items.find(itemGuid) != _items.end())
            return EtchResult::AlreadyBranded;
        if (CharacterDatabase.Query("SELECT 1 FROM `item_branding` WHERE `item_guid` = {}", itemGuid))
            return EtchResult::AlreadyBranded;

        BrandId const brand = sLoadoutMgr->GetLoadout(player->GetGUID()).activeBrand;

        // Gated behind enrollment (#31 decision 6): the active school must be account-unlocked, which is
        // also what lets the Etched proc express (decision 2 / anti-P2W §1).
        if (!CanExpressBrand(brand, sProficiencyMgr->AccountKnowledge(player->GetSession()->GetAccountId())))
            return EtchResult::NotEnrolled;

        if (player->GetItemCount(_essenceItemId, false) < _essenceCost)
            return EtchResult::InsufficientEssence;

        // Commit only past every gate: consume the premium BoP Essence, write the rank-locked Etched
        // state, and soulbind the item (BoE -> BoP, #31 decision 3 / §16.3).
        player->DestroyItemCount(_essenceItemId, _essenceCost, true);

        ItemBrandState state;
        state.brand = brand;
        state.etched = true;
        _items[itemGuid] = state;
        Save(itemGuid, state);

        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlot))
        {
            item->SetBinding(true);
            item->SetState(ITEM_CHANGED, player);
        }

        return EtchResult::Success;
    }

    double ItemBrandingMgr::AggregateEtchedIntensity(Player* player) const
    {
        if (!player)
            return 1.0;

        BrandId const active = sLoadoutMgr->GetLoadout(player->GetGUID()).activeBrand;

        // Etched procs express only while their brand is the active school AND the account can express it
        // (#31 decision 2). If the active school is inexpressible, every Etched item is dormant.
        if (!CanExpressBrand(active, sProficiencyMgr->AccountKnowledge(player->GetSession()->GetAccountId())))
            return 1.0;

        // Count equipped Etched items of the active school across the eligible slots; they all share the
        // active (school, tree) catalyst bucket, so the self-stack DR collapses them to one saturating
        // multiplier -- more items, a little more, never past the cap (decisions 1 & 5).
        uint8_t count = 0;
        for (uint8 slot : EtchSlots)
        {
            auto it = _items.find(ItemGuidAtSlot(player, slot));
            if (it != _items.end() && it->second.etched && it->second.brand == active)
                ++count;
        }

        return CatalystSelfStackMultiplier(count, sCatalystMgr->Config());
    }

    bool ItemBrandingMgr::EquippedState(Player* player, ItemBrandState& out, uint8_t equipSlot) const
    {
        auto it = _items.find(ItemGuidAtSlot(player, equipSlot));
        if (it == _items.end())
            return false;

        out = it->second;
        return true;
    }
}
