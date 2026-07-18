#ifndef MOD_BRANDING_CORE_EFFECTS_ROLEPOLICY_H
#define MOD_BRANDING_CORE_EFFECTS_ROLEPOLICY_H

#include "branding/common/Brand.h"
#include <cstdint>

// Pure core (no AzerothCore includes). §14.11 talent-spec seam: a branded player's role is a FULLY-OPEN
// player choice per loadout -- NOT gated by class (#13, owner decision 2026-07-18). The default for an
// unset choice is produced by a swappable policy (flavor only). Class/form/presence arrive as plain
// numbers so the core stays engine-free; the adapter samples them from the live player.
namespace Branding
{
    // Bitmask of selectable RoleContribution values (bit = 1u << role). #13: role selection is fully
    // decoupled from class -- every class may express every selectable role (Damage|Tank|Healer), so
    // this is class-independent (the parameter is retained only for a stable seam signature). Kept as
    // the "all legal" constant the default policy clamps against (a no-op for the three real roles).
    uint8_t RoleCapabilityMask(uint8_t classId);
    bool RoleAllowed(uint8_t classId, RoleContribution role);

    // Live signals sampled by the adapter (kept numeric to keep core engine-free):
    //  - dominantTab:     Player::GetMostPointsTalentTree() (0/1/2)
    //  - shapeshiftForm:  Unit::GetShapeshiftForm() (UnitDefines: FORM_CAT=0x01, BEAR=0x05, DIREBEAR=0x08)
    //  - inFrostPresence: DK tank presence active (spell 48263)
    struct RoleSignals
    {
        uint8_t classId = 0;
        uint8_t dominantTab = 0;
        uint8_t shapeshiftForm = 0;
        bool inFrostPresence = false;
    };

    // Injection point (DI): decides the role when the player has made no explicit choice. Swappable by
    // config so prod and test realms can differ without touching the hot path or the validation rules.
    class IDefaultRolePolicy
    {
    public:
        virtual ~IDefaultRolePolicy() = default;
        virtual RoleContribution DefaultRole(RoleSignals const& signals, uint8_t capabilityMask) const = 0;

        // Whether DefaultRole reads any of the live RoleSignals (talents/form/presence). The adapter
        // skips the (relatively costly) talent-map sample when this is false -- so a policy that ignores
        // signals never pays for them on the per-hit path. Conservative default: true.
        virtual bool UsesSignals() const { return true; }
    };

    // Production default (flavor only): always Damage. Ignores talents -- players who care pick their
    // role explicitly, and any explicit pick is honoured (no class gate, #13).
    class ClassDefaultRolePolicy : public IDefaultRolePolicy
    {
    public:
        RoleContribution DefaultRole(RoleSignals const& signals, uint8_t capabilityMask) const override;
        bool UsesSignals() const override { return false; }
    };

    // Test-realm swap-in (flavor default only): infer from the dominant talent tab, disambiguating the
    // two tree-ambiguous classes by live signal -- Druid Feral by bear form (tank) vs cat (dps), DK by
    // Frost Presence (tank). Only decides the default for an unset choice; never a permission gate.
    class TalentInferredRolePolicy : public IDefaultRolePolicy
    {
    public:
        RoleContribution DefaultRole(RoleSignals const& signals, uint8_t capabilityMask) const override;
    };

    // Effective role for a loadout: honour any explicit (non-None) selectable choice at full magnitude
    // (no class gate, #13), and delegate an unset (None) choice to the default policy.
    RoleContribution ResolveSelectedRole(RoleContribution chosen, RoleSignals const& signals,
        IDefaultRolePolicy const& policy);
}

#endif // MOD_BRANDING_CORE_EFFECTS_ROLEPOLICY_H
