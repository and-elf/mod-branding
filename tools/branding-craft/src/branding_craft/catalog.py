"""The one place Branded recipes are defined (#29).

Each :class:`Recipe` ties together its native craft spell, the profession it lives in, its reagent
counts, the Branded output it produces, and the BoP pattern item that teaches it. ``branding_recipe``
(the server mirror, #27) and the client ``Spell.dbc`` reagents are both generated from the *same*
reagent counts here, so the two can never disagree -- the drift guard #29 calls for.

Resources and Branded outputs are the §27 server content; they live here too so the whole closed loop
(resources -> craft spell -> output) is described and validated in a single catalog.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from . import constants as C


@dataclass(frozen=True)
class ResourceItem:
    """A crafting input (Material or Fragment), config-mapped per §16.3."""

    entry: int
    name: str
    bonding: int
    item_class: int
    subclass: int
    quality: int
    displayid: int
    stackable: int
    config_key: str  # the Branding.Economy.*ItemId key this entry is wired to


@dataclass(frozen=True)
class BrandedItem:
    """A Branded output: heroic-cloned model, modest base stats (§27), BoA.

    The stat / armour / required_level fields default to the shipped starter-trio values so the three
    legacy chest recipes emit byte-for-byte identical SQL; the archetype sets (#1) set them per slot.
    """

    entry: int
    name: str
    item_class: int
    subclass: int
    inventory_type: int
    quality: int
    displayid: int
    item_level: int
    required_skill: int       # profession gate holds even before the native window (#27 scope 2)
    required_skill_rank: int
    required_level: int = 0        # endgame equip gate for the sets (#1); 0 keeps the starter trio open
    stat1_type: int = 7            # ITEM_MOD_STAMINA
    stat1_value: int = 12
    stat2_type: int = 5            # ITEM_MOD_INTELLECT
    stat2_value: int = 8
    armor: int = 250
    # Weapon fields (0 for armour/resource entries). A branded weapon (#1) carries modest weapon
    # damage + delay; like the armour base, power comes from the brand proc, not the weapon DPS.
    dmg_min: int = 0
    dmg_max: int = 0
    dmg_type: int = 0
    delay: int = 0
    source_item_id: int = 0        # provenance: the real ilvl-226 BiS item this piece is modelled on
    #                                (playerbots BiS, Phase-2/gs=245). Used to retarget displayid once
    #                                the world DB is reachable; behaviour never depends on it (#29).


@dataclass(frozen=True)
class Recipe:
    """A native profession craft: reagents -> Branded output, taught by a BoP pattern item."""

    id: int                   # branding_recipe.id (mirror) -- also the .branding upgrade handle
    name: str                 # player-facing "Branded <item>"
    skill_line: int           # SkillLine.dbc id (profession the recipe appears in)
    spell_id: int             # Spell.dbc / spell_dbc craft spell id
    skill_line_ability_id: int
    pattern_entry: int        # item_template pattern (BoP) that teaches spell_id
    pattern_subclass: int     # recipe subclass matching the profession
    pattern_quality: int
    pattern_displayid: int
    output: BrandedItem
    material_count: int       # reagent: Material item (BoE)
    fragment_count: int       # reagent: Fragment item (BoA)
    char_xp: int
    req_skill_value: int      # MinSkillLineRank: skill needed to learn/craft
    trivial_high: int         # TrivialSkillLineRankHigh: skill at which the recipe greys out
    school: int = C.BRAND_GENERIC  # BrandId of the Fragment this recipe consumes; BRAND_GENERIC keeps
    #                                the generic Fragment. A schooled recipe consumes the per-school
    #                                Fragment at SCHOOL_FRAGMENT_BASE + school (the "Fire-Branded
    #                                Fragment" loop): mine fire in a fire event, forge fire gear.


def _school_fragment(school: int) -> ResourceItem:
    """The per-school Fragment for BrandId ``school`` (BoA, like the generic Fragment)."""

    name = C.BRAND_SCHOOL_NAMES[school]
    return ResourceItem(
        entry=C.SCHOOL_FRAGMENT_BASE + school,
        name=f"{name}-Branded Fragment",
        bonding=C.BIND_TO_ACCOUNT,  # BoA -- raid/invasion-sourced, account-wide (§16.3)
        item_class=C.ITEM_CLASS_TRADE_GOODS,
        subclass=C.ITEM_SUBCLASS_TRADE_GOODS_CLOTH,
        quality=C.QUALITY_RARE,
        displayid=41111,
        stackable=1000,
        config_key="Branding.Economy.SchoolFragmentBaseItemId",
    )


# One per-school Fragment for every BrandId, in enum order (so its entry == base + index).
SCHOOL_FRAGMENTS = [_school_fragment(i) for i in range(C.BRAND_SCHOOL_COUNT)]


@dataclass(frozen=True)
class Catalog:
    material: ResourceItem
    fragment: ResourceItem
    recipes: list[Recipe] = field(default_factory=list)
    school_fragments: list[ResourceItem] = field(default_factory=lambda: list(SCHOOL_FRAGMENTS))

    @property
    def outputs(self) -> list[BrandedItem]:
        return [r.output for r in self.recipes]

    def fragment_for(self, recipe: Recipe) -> ResourceItem:
        """The Fragment a recipe consumes: its per-school Fragment, or the generic one."""

        if recipe.school < C.BRAND_SCHOOL_COUNT:
            return self.school_fragments[recipe.school]
        return self.fragment

    def reagents(self, recipe: Recipe) -> list[tuple[int, int]]:
        """(item_entry, count) reagent pairs for a recipe, in (Material, Fragment) order.

        This is the canonical reagent definition consumed by BOTH the spell_dbc/Spell.dbc emitter
        and the branding_recipe mirror -- the single point that keeps them in lockstep. The Fragment
        entry is the recipe's per-school Fragment (or the generic Fragment for an unschooled recipe).
        """

        pairs: list[tuple[int, int]] = []
        if recipe.material_count:
            pairs.append((self.material.entry, recipe.material_count))
        if recipe.fragment_count:
            pairs.append((self.fragment_for(recipe).entry, recipe.fragment_count))
        return pairs


# --- The shipped starter set ------------------------------------------------------------------
# Three recipes, one per armour profession, scaling input cost / skill gate with tier (§8.3).
# displayid values clone heroic-dungeon / trade-good art; they are tunables -- retarget freely (the
# acceptance criteria gate on consume/produce/skill behaviour, not on which icon shows).

MATERIAL = ResourceItem(
    entry=190000,
    name="Branding Material",
    bonding=C.BIND_WHEN_EQUIPPED,  # BoE -- the surviving player market (§16.3)
    item_class=C.ITEM_CLASS_TRADE_GOODS,
    subclass=C.ITEM_SUBCLASS_TRADE_GOODS_CLOTH,
    quality=C.QUALITY_COMMON,
    displayid=6884,
    stackable=1000,
    config_key="Branding.Economy.MaterialItemId",
)

FRAGMENT = ResourceItem(
    entry=190001,
    name="Branding Fragment",
    bonding=C.BIND_TO_ACCOUNT,  # BoA -- raid/invasion-sourced, account-wide (§16.3)
    item_class=C.ITEM_CLASS_TRADE_GOODS,
    subclass=C.ITEM_SUBCLASS_TRADE_GOODS_CLOTH,
    quality=C.QUALITY_RARE,
    displayid=41111,
    stackable=1000,
    config_key="Branding.Economy.FragmentItemId",
)


def _branded(entry, name, subclass, ilvl, skill, rank, displayid, quality):
    return BrandedItem(
        entry=entry,
        name=name,
        item_class=C.ITEM_CLASS_ARMOR,
        subclass=subclass,
        inventory_type=C.INVTYPE_CHEST,
        quality=quality,
        displayid=displayid,
        item_level=ilvl,
        required_skill=skill,
        required_skill_rank=rank,
    )


STARTER_RECIPES = [
        Recipe(
            id=1,
            name="Branded Leather Chestguard",
            skill_line=C.SKILL_LEATHERWORKING,
            spell_id=1900010,
            skill_line_ability_id=1900110,
            pattern_entry=190050,
            pattern_subclass=C.ITEM_SUBCLASS_RECIPE_LEATHERWORKING,
            pattern_quality=C.QUALITY_UNCOMMON,
            pattern_displayid=1387,
            output=_branded(
                190010, "Branded Leather Chestguard", C.ITEM_SUBCLASS_ARMOR_LEATHER,
                ilvl=187, skill=C.SKILL_LEATHERWORKING, rank=350, displayid=48723,
                quality=C.QUALITY_UNCOMMON,
            ),
            material_count=5,
            fragment_count=0,
            char_xp=100,
            req_skill_value=350,
            trivial_high=375,
            school=C.BRAND_NATURE,
        ),
        Recipe(
            id=2,
            name="Branded Plate Chestpiece",
            skill_line=C.SKILL_BLACKSMITHING,
            spell_id=1900011,
            skill_line_ability_id=1900111,
            pattern_entry=190051,
            pattern_subclass=C.ITEM_SUBCLASS_RECIPE_BLACKSMITHING,
            pattern_quality=C.QUALITY_RARE,
            pattern_displayid=1387,
            output=_branded(
                190011, "Branded Plate Chestpiece", C.ITEM_SUBCLASS_ARMOR_PLATE,
                ilvl=200, skill=C.SKILL_BLACKSMITHING, rank=400, displayid=48729,
                quality=C.QUALITY_RARE,
            ),
            material_count=10,
            fragment_count=5,
            char_xp=250,
            req_skill_value=400,
            trivial_high=425,
            school=C.BRAND_FIRE,
        ),
        Recipe(
            id=3,
            name="Branded Cloth Robe",
            skill_line=C.SKILL_TAILORING,
            spell_id=1900012,
            skill_line_ability_id=1900112,
            pattern_entry=190052,
            pattern_subclass=C.ITEM_SUBCLASS_RECIPE_TAILORING,
            pattern_quality=C.QUALITY_RARE,
            pattern_displayid=1387,
            output=_branded(
                190012, "Branded Cloth Robe", C.ITEM_SUBCLASS_ARMOR_CLOTH,
                ilvl=213, skill=C.SKILL_TAILORING, rank=450, displayid=48733,
                quality=C.QUALITY_RARE,
            ),
            material_count=20,
            fragment_count=10,
            char_xp=600,
            req_skill_value=450,
            trivial_high=450,
            school=C.BRAND_ARCANE,
        ),
]


# --- Archetype armour sets (#1) ---------------------------------------------------------------
# Eight sets -- one per (armour class x role) the issue calls for -- each a full 9-slot armour set,
# modelled slot-for-slot on real ilvl-226 gear (playerbots BiS, WotLK Phase-2 / gs=245). Per the
# locked #27 decision this reuse is *coverage + provenance only*: the pieces carry modest role stats
# and RequiredLevel 80, never the source item's raid-BiS numbers, so the "3k AP at level 1" and
# druid-form-scaling worries in the issue never arise. Everything is generated from SET_SPECS below
# so all 72 recipes stay in lockstep and inside the reserved bands.

@dataclass(frozen=True)
class SlotSpec:
    """One armour slot shared by every set: its noun, INVTYPE, and armour/stat weight."""

    noun: str
    inventory_type: int
    weight: float             # slot budget multiplier (chest/legs/head = 1.0, wrists/cloak lightest)
    subclass_override: int | None = None  # cloaks are Misc(0), not the set's armour class


# EquipmentSlots (BiS table) -> our slot list. Cloak is Misc so it stays account-agnostic/equippable
# by any class; every other slot inherits the set's armour subclass.
SET_SLOTS = [
    SlotSpec("Helm",       C.INVTYPE_HEAD,      1.00),
    SlotSpec("Pauldrons",  C.INVTYPE_SHOULDERS, 0.75),
    SlotSpec("Chestguard", C.INVTYPE_CHEST,     1.00),
    SlotSpec("Girdle",     C.INVTYPE_WAIST,     0.75),
    SlotSpec("Legguards",  C.INVTYPE_LEGS,      1.00),
    SlotSpec("Boots",      C.INVTYPE_FEET,      0.75),
    SlotSpec("Bracers",    C.INVTYPE_WRISTS,    0.5625),
    SlotSpec("Gloves",     C.INVTYPE_HANDS,     0.75),
    SlotSpec("Cloak",      C.INVTYPE_CLOAK,     0.5625, subclass_override=C.ITEM_SUBCLASS_ARMOR_MISC),
]

# EquipmentSlots ids (playerbots BiS `slot` column) for each SET_SLOTS entry, in order -- the key we
# look the source item up by. head=0, shoulders=2, chest=4, waist=5, legs=6, feet=7, wrists=8,
# hands=9, back=14.
_BIS_SLOT_IDS = [0, 2, 4, 5, 6, 7, 8, 9, 14]

# Armour-class armour multiplier (relative to cloth) for the modest base armour value.
_ARMOR_MULT = {
    C.ITEM_SUBCLASS_ARMOR_CLOTH: 1.0,
    C.ITEM_SUBCLASS_ARMOR_LEATHER: 1.6,
    C.ITEM_SUBCLASS_ARMOR_MAIL: 2.4,
    C.ITEM_SUBCLASS_ARMOR_PLATE: 4.0,
}


@dataclass(frozen=True)
class SetSpec:
    """One archetype set: armour class + role + the modest role stats its pieces carry."""

    key: str                  # 'plate-tank' -- stable id / band ordering
    theme: str                # player-facing set name token ("Branded <theme> <slot noun>")
    subclass: int             # armour class (plate/mail/leather/cloth)
    skill_line: int           # crafting profession (plate->BS, mail+leather->LW, cloth->Tailoring)
    pattern_subclass: int     # recipe subclass matching the profession
    stat1_type: int           # primary modest stat
    stat2_type: int           # secondary modest stat
    item_level: int
    sources: list[int]        # real ilvl-226 BiS item id per SET_SLOTS entry (0 = none at this tier)


# Real ilvl-226 source item ids (playerbots BiS, gs=245) per set, in SET_SLOTS order
# (Helm, Pauldrons, Chest, Girdle, Legs, Boots, Bracers, Gloves, Cloak). 0 = the spec had no armour
# piece in that slot at this tier (e.g. Fury bracers) -- coverage still gets a modest branded piece.
SET_SPECS = [
    SetSpec("plate-tank", "Bulwark", C.ITEM_SUBCLASS_ARMOR_PLATE, C.SKILL_BLACKSMITHING,
            C.ITEM_SUBCLASS_RECIPE_BLACKSMITHING, C.ITEM_MOD_STAMINA, C.ITEM_MOD_DODGE_RATING, 226,
            [46166, 46167, 46162, 45139, 46169, 45542, 45111, 45487, 45496]),
    SetSpec("plate-dps", "Warmonger", C.ITEM_SUBCLASS_ARMOR_PLATE, C.SKILL_BLACKSMITHING,
            C.ITEM_SUBCLASS_RECIPE_BLACKSMITHING, C.ITEM_MOD_STRENGTH, C.ITEM_MOD_CRIT_RATING, 226,
            [46151, 46037, 46146, 46041, 45536, 45599, 0, 46148, 46032]),
    SetSpec("mail-heal", "Tidecaller", C.ITEM_SUBCLASS_ARMOR_MAIL, C.SKILL_LEATHERWORKING,
            C.ITEM_SUBCLASS_RECIPE_LEATHERWORKING, C.ITEM_MOD_INTELLECT, C.ITEM_MOD_SPIRIT, 226,
            [46201, 46204, 45867, 45151, 46202, 45537, 45460, 46199, 44005]),
    SetSpec("mail-dps", "Pathfinder", C.ITEM_SUBCLASS_ARMOR_MAIL, C.SKILL_LEATHERWORKING,
            C.ITEM_SUBCLASS_RECIPE_LEATHERWORKING, C.ITEM_MOD_AGILITY, C.ITEM_MOD_ATTACK_POWER, 226,
            [46143, 46145, 46141, 45467, 46144, 45244, 45454, 45444, 46032]),
    SetSpec("leather-heal", "Wildkeeper", C.ITEM_SUBCLASS_ARMOR_LEATHER, C.SKILL_LEATHERWORKING,
            C.ITEM_SUBCLASS_RECIPE_LEATHERWORKING, C.ITEM_MOD_INTELLECT, C.ITEM_MOD_SPIRIT, 226,
            [46184, 46187, 45519, 45616, 46185, 45135, 45446, 46183, 45618]),
    SetSpec("leather-dps", "Nightblade", C.ITEM_SUBCLASS_ARMOR_LEATHER, C.SKILL_LEATHERWORKING,
            C.ITEM_SUBCLASS_RECIPE_LEATHERWORKING, C.ITEM_MOD_AGILITY, C.ITEM_MOD_CRIT_RATING, 226,
            [46125, 46127, 45473, 46095, 45536, 45564, 45611, 46124, 46032]),
    SetSpec("cloth-heal", "Lightbearer", C.ITEM_SUBCLASS_ARMOR_CLOTH, C.SKILL_TAILORING,
            C.ITEM_SUBCLASS_RECIPE_TAILORING, C.ITEM_MOD_INTELLECT, C.ITEM_MOD_SPIRIT, 226,
            [45497, 46068, 45240, 45619, 46195, 45135, 45460, 46188, 45618]),
    SetSpec("cloth-dps", "Spellfire", C.ITEM_SUBCLASS_ARMOR_CLOTH, C.SKILL_TAILORING,
            C.ITEM_SUBCLASS_RECIPE_TAILORING, C.ITEM_MOD_INTELLECT, C.ITEM_MOD_SPELL_POWER, 226,
            [46129, 46134, 46130, 45619, 46133, 45135, 45446, 45665, 45242]),
]

# Verified-present display ids by armour class (ItemDisplayInfo.dbc, checked 2026-06-22). Placeholder
# per class -- every slot of a set currently shares its class chest model. Retarget per slot to the
# recorded source_item_id's real displayid once the world DB is reachable (README: displayid is a
# tunable; acceptance gates on craft/equip behaviour, not the icon).
_SET_DISPLAYID = {
    C.ITEM_SUBCLASS_ARMOR_PLATE: 48729,
    C.ITEM_SUBCLASS_ARMOR_MAIL: 48723,
    C.ITEM_SUBCLASS_ARMOR_LEATHER: 48723,
    C.ITEM_SUBCLASS_ARMOR_CLOTH: 48733,
}

# Band bases -- outputs/patterns grouped 10-per-set for readable entries (band is wide); spell/SLA ids
# packed contiguously because the client-verified craft-spell band (1900000-1900099) is tight.
_SET_OUTPUT_BASE = 190200
_SET_PATTERN_BASE = 190500
_SET_RECIPE_ID_BASE = 100
_SET_SPELL_BASE = 1900013         # starter trio holds 1900010-1900012
_SET_SLA_BASE = 1900113           # starter trio holds 1900110-1900112
_SET_SKILL_RANK = 440             # endgame recipe: learnable at 440, greys out at the 450 cap


def _build_set_recipes() -> list[Recipe]:
    recipes: list[Recipe] = []
    idx = 0  # global running index -> contiguous spell / SLA ids
    for s, spec in enumerate(SET_SPECS):
        for k, slot in enumerate(SET_SLOTS):
            subclass = slot.subclass_override if slot.subclass_override is not None else spec.subclass
            weight = slot.weight
            name = f"Branded {spec.theme} {slot.noun}"
            output = BrandedItem(
                entry=_SET_OUTPUT_BASE + s * 10 + k,
                name=name,
                item_class=C.ITEM_CLASS_ARMOR,
                subclass=subclass,
                inventory_type=slot.inventory_type,
                quality=C.QUALITY_RARE,
                displayid=_SET_DISPLAYID[spec.subclass],
                item_level=spec.item_level,
                required_skill=spec.skill_line,
                required_skill_rank=_SET_SKILL_RANK,
                required_level=C.SET_REQUIRED_LEVEL,
                stat1_type=spec.stat1_type,
                stat1_value=round(15 * weight),
                stat2_type=spec.stat2_type,
                stat2_value=round(10 * weight),
                armor=round(60 * _ARMOR_MULT[spec.subclass] * weight),
                source_item_id=spec.sources[k],
            )
            recipes.append(Recipe(
                id=_SET_RECIPE_ID_BASE + s * 10 + k,
                name=name,
                skill_line=spec.skill_line,
                spell_id=_SET_SPELL_BASE + idx,
                skill_line_ability_id=_SET_SLA_BASE + idx,
                pattern_entry=_SET_PATTERN_BASE + s * 10 + k,
                pattern_subclass=spec.pattern_subclass,
                pattern_quality=C.QUALITY_RARE,
                pattern_displayid=1387,
                output=output,
                material_count=round(8 * weight) + 4,
                fragment_count=max(1, round(4 * weight)),
                char_xp=round(200 * weight),
                req_skill_value=_SET_SKILL_RANK,
                trivial_high=450,
                school=C.BRAND_GENERIC,
            ))
            idx += 1
    return recipes


SET_RECIPES = _build_set_recipes()


# --- Archetype main-hand weapons (#1, weapons) ------------------------------------------------
# One signature main-hand per set -- the slot where the brand proc actually attaches (the crafted
# path brands EQUIPMENT_SLOT_MAINHAND; the etch surface is weapons + trinkets). All Blacksmithing
# (owner decision), modest weapon damage + one modest stat (the set's primary), RequiredLevel 80.
# The weapon's own DPS is deliberately below the ilvl-226 source -- power is the brand proc (§27).

@dataclass(frozen=True)
class WeaponSpec:
    """A set's signature weapon: type + damage profile, keyed to a SET_SPECS entry by index.

    Usually a main-hand; the hunter (mail-DPS) set instead carries a ranged crossbow, since that is a
    hunter's real weapon. ``two_handed`` selects the heavier damage/delay profile (also used by ranged).
    """

    noun: str                 # "Greatsword", "Warmace", "Crossbow", ...
    weapon_subclass: int
    inventory_type: int
    two_handed: bool
    source_item_id: int       # real ilvl-226 weapon from BiS (0 = the spec had none at gs=245)


# Aligned 1:1 with SET_SPECS (same order): plate-tank, plate-dps, mail-heal, mail-dps,
# leather-heal, leather-dps, cloth-heal, cloth-dps.
WEAPON_BY_SET = [
    WeaponSpec("Warmace",    C.ITEM_SUBCLASS_WEAPON_MACE1H,  C.INVTYPE_WEAPONMAINHAND, False, 0),
    WeaponSpec("Greatsword", C.ITEM_SUBCLASS_WEAPON_SWORD2H, C.INVTYPE_2HWEAPON,       True,  0),
    WeaponSpec("Hammer",     C.ITEM_SUBCLASS_WEAPON_MACE1H,  C.INVTYPE_WEAPONMAINHAND, False, 46017),
    # Hunter (mail-DPS): the signature weapon is ranged, not melee -- a crossbow in the ranged slot.
    WeaponSpec("Crossbow",   C.ITEM_SUBCLASS_WEAPON_CROSSBOW, C.INVTYPE_RANGEDRIGHT,   True,  45570),
    WeaponSpec("Mace",       C.ITEM_SUBCLASS_WEAPON_MACE1H,  C.INVTYPE_WEAPONMAINHAND, False, 0),
    WeaponSpec("Dagger",     C.ITEM_SUBCLASS_WEAPON_DAGGER,  C.INVTYPE_WEAPONMAINHAND, False, 0),
    WeaponSpec("Scepter",    C.ITEM_SUBCLASS_WEAPON_MACE1H,  C.INVTYPE_WEAPONMAINHAND, False, 46017),
    WeaponSpec("Blade",      C.ITEM_SUBCLASS_WEAPON_SWORD1H, C.INVTYPE_WEAPONMAINHAND, False, 45620),
]

_WEAPON_OUTPUT_BASE = 190280      # after the armour sets (190200-190278), still in SET_OUTPUT_BAND
_WEAPON_PATTERN_BASE = 190580     # still in SET_PATTERN_BAND
_WEAPON_RECIPE_ID_BASE = 180
_WEAPON_SPELL_BASE = 1900085      # after the armour set spells (1900013-1900084)
_WEAPON_SLA_BASE = 1900185
_WEAPON_DISPLAYID = 48729         # PLACEHOLDER (armour model) -- weapons need real weapon displayids;
#                                   retarget per source_item_id once the world DB is reachable.


def _build_weapon_recipes() -> list[Recipe]:
    recipes: list[Recipe] = []
    for s, (spec, weapon) in enumerate(zip(SET_SPECS, WEAPON_BY_SET, strict=True)):
        delay = 3600 if weapon.two_handed else 2600
        dmg_min, dmg_max = (288, 432) if weapon.two_handed else (208, 312)  # modest ~100 DPS
        name = f"Branded {spec.theme} {weapon.noun}"
        output = BrandedItem(
            entry=_WEAPON_OUTPUT_BASE + s,
            name=name,
            item_class=C.ITEM_CLASS_WEAPON,
            subclass=weapon.weapon_subclass,
            inventory_type=weapon.inventory_type,
            quality=C.QUALITY_RARE,
            displayid=_WEAPON_DISPLAYID,
            item_level=spec.item_level,
            required_skill=C.SKILL_BLACKSMITHING,
            required_skill_rank=_SET_SKILL_RANK,
            required_level=C.SET_REQUIRED_LEVEL,
            stat1_type=spec.stat1_type,
            stat1_value=15,
            stat2_type=0,
            stat2_value=0,
            armor=0,
            dmg_min=dmg_min,
            dmg_max=dmg_max,
            dmg_type=0,
            delay=delay,
            source_item_id=weapon.source_item_id,
        )
        recipes.append(Recipe(
            id=_WEAPON_RECIPE_ID_BASE + s,
            name=name,
            skill_line=C.SKILL_BLACKSMITHING,
            spell_id=_WEAPON_SPELL_BASE + s,
            skill_line_ability_id=_WEAPON_SLA_BASE + s,
            pattern_entry=_WEAPON_PATTERN_BASE + s,
            pattern_subclass=C.ITEM_SUBCLASS_RECIPE_BLACKSMITHING_WEAPON,
            pattern_quality=C.QUALITY_RARE,
            pattern_displayid=1387,
            output=output,
            material_count=12,
            fragment_count=4,
            char_xp=200,
            req_skill_value=_SET_SKILL_RANK,
            trivial_high=450,
            school=C.BRAND_GENERIC,
        ))
    return recipes


WEAPON_RECIPES = _build_weapon_recipes()


CATALOG = Catalog(
    material=MATERIAL,
    fragment=FRAGMENT,
    recipes=STARTER_RECIPES + SET_RECIPES + WEAPON_RECIPES,
)
