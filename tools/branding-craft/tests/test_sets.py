"""Archetype armour sets (#1) -- 8 (armour class x role) sets, 9 armour slots each.

The pieces reuse real ilvl-226 BiS gear for *coverage and provenance only*; per the locked #27
decision they carry modest role stats and an endgame equip gate, never raid-BiS numbers.
"""

import re

from branding_craft import constants as C
from branding_craft.catalog import (
    CATALOG,
    SET_RECIPES,
    SET_SLOTS,
    SET_SPECS,
    STARTER_RECIPES,
    WEAPON_RECIPES,
)
from branding_craft.emit_sql import emit_world_sql
from branding_craft.validate import validate


def test_eight_sets_of_nine_slots_are_generated():
    assert len(SET_SPECS) == 8
    assert len(SET_SLOTS) == 9
    assert len(SET_RECIPES) == 8 * 9
    # Appended after the starter trio, so the index-based legacy tests keep pointing at the trio.
    assert CATALOG.recipes[:len(STARTER_RECIPES)] == STARTER_RECIPES
    assert len(CATALOG.recipes) == len(STARTER_RECIPES) + 8 * 9 + len(WEAPON_RECIPES)


def test_the_issues_role_matrix_is_covered_exactly():
    keys = {s.key for s in SET_SPECS}
    assert keys == {
        "plate-tank", "plate-dps", "mail-heal", "mail-dps",
        "leather-heal", "leather-dps", "cloth-heal", "cloth-dps",
    }


def test_catalog_stays_valid_and_ids_unique_after_expansion():
    assert validate() == []
    outputs = [r.output.entry for r in CATALOG.recipes]
    patterns = [r.pattern_entry for r in CATALOG.recipes]
    spells = [r.spell_id for r in CATALOG.recipes]
    slas = [r.skill_line_ability_id for r in CATALOG.recipes]
    ids = [r.id for r in CATALOG.recipes]
    for pool in (outputs, patterns, spells, slas, ids):
        assert len(pool) == len(set(pool))
    # The whole set surface never collides with the starter trio's ids.
    for r in SET_RECIPES:
        assert r.spell_id not in (1900010, 1900011, 1900012)


def test_set_entries_and_spells_sit_in_their_reserved_bands():
    for r in SET_RECIPES:
        assert r.output.entry in C.SET_OUTPUT_BAND
        assert r.pattern_entry in C.SET_PATTERN_BAND
        # Craft spells must stay inside the client-verified band (1900000-1900099); SLA in 1900100+.
        assert r.spell_id in C.CRAFT_SPELL_BAND
        assert r.skill_line_ability_id in C.SKILL_LINE_ABILITY_BAND


def test_professions_map_by_armour_class():
    want = {
        C.ITEM_SUBCLASS_ARMOR_PLATE: C.SKILL_BLACKSMITHING,
        C.ITEM_SUBCLASS_ARMOR_MAIL: C.SKILL_LEATHERWORKING,
        C.ITEM_SUBCLASS_ARMOR_LEATHER: C.SKILL_LEATHERWORKING,
        C.ITEM_SUBCLASS_ARMOR_CLOTH: C.SKILL_TAILORING,
    }
    for spec in SET_SPECS:
        assert spec.skill_line == want[spec.subclass]


def test_mail_armour_class_is_present():
    assert any(s.subclass == C.ITEM_SUBCLASS_ARMOR_MAIL for s in SET_SPECS)


def test_pieces_carry_the_endgame_gate_and_modest_stats():
    for r in SET_RECIPES:
        out = r.output
        assert out.required_level == C.SET_REQUIRED_LEVEL == 80  # no low-level twinking (#1)
        assert out.required_skill == r.skill_line                # profession equip gate (#27)
        # "Modest": a chest never exceeds a small primary stat -- nowhere near ilvl-226 BiS numbers.
        assert 0 < out.stat1_value <= 20
        assert 0 < out.stat2_value <= 20


def test_cloaks_are_class_agnostic_misc():
    # Every set's cloak (back slot) must be Misc so any class can wear it, not the set's armour class.
    cloaks = [r for r in SET_RECIPES if r.output.inventory_type == C.INVTYPE_CLOAK]
    assert len(cloaks) == 8
    for r in cloaks:
        assert r.output.subclass == C.ITEM_SUBCLASS_ARMOR_MISC


def test_one_branded_main_hand_weapon_per_set_all_blacksmithing():
    assert len(WEAPON_RECIPES) == 8
    for r in WEAPON_RECIPES:
        w = r.output
        assert w.item_class == C.ITEM_CLASS_WEAPON
        assert r.skill_line == C.SKILL_BLACKSMITHING          # owner decision: all weapons under BS
        assert w.required_skill == C.SKILL_BLACKSMITHING      # validator: output skill == recipe skill
        assert w.required_level == C.SET_REQUIRED_LEVEL == 80  # no low-level druid-form AP (#1)
        assert w.delay > 0 and w.dmg_max >= w.dmg_min > 0      # a real weapon carries damage + delay
        # main-hand for melee sets; the hunter (mail-DPS) set carries a ranged weapon instead.
        assert w.inventory_type in (
            C.INVTYPE_WEAPONMAINHAND, C.INVTYPE_2HWEAPON, C.INVTYPE_RANGEDRIGHT,
        )


def test_hunter_set_gets_a_ranged_weapon():
    # mail-DPS is the hunter archetype -- its signature weapon must be ranged, not melee (#1).
    pathfinder = next(r for r in WEAPON_RECIPES if r.output.name.startswith("Branded Pathfinder"))
    assert pathfinder.output.inventory_type == C.INVTYPE_RANGEDRIGHT
    assert pathfinder.output.subclass == C.ITEM_SUBCLASS_WEAPON_CROSSBOW


def test_weapon_entries_and_spells_stay_in_band_and_unique():
    for r in WEAPON_RECIPES:
        assert r.output.entry in C.SET_OUTPUT_BAND
        assert r.pattern_entry in C.SET_PATTERN_BAND
        assert r.spell_id in C.CRAFT_SPELL_BAND                # must stay client-verified <= 1900099
        assert r.skill_line_ability_id in C.SKILL_LINE_ABILITY_BAND
    # Whole catalog: trio + 72 armour + 8 weapons, ids globally unique.
    assert len(CATALOG.recipes) == len(STARTER_RECIPES) + 72 + 8
    spells = [r.spell_id for r in CATALOG.recipes]
    assert len(spells) == len(set(spells))
    assert max(spells) <= 1900099                             # band ceiling (client-verified range)


def test_world_sql_emits_weapon_damage():
    block = emit_world_sql().split("INSERT INTO `item_template`")[1].split(";")[0]
    greatsword = next(r for r in WEAPON_RECIPES if r.output.name == "Branded Warmonger Greatsword")
    row = re.search(rf"\(\s*{greatsword.output.entry},[^)]*\)", block).group(0)
    cells = [c.strip().strip("'") for c in row.strip("()").split(",")]
    # class=2 (weapon), and the four damage cells (dmg_min1,dmg_max1,dmg_type1,delay) after armor(18).
    assert cells[1] == str(C.ITEM_CLASS_WEAPON)
    assert int(cells[19]) > 0 and int(cells[20]) >= int(cells[19])  # dmg_min1 / dmg_max1
    assert int(cells[22]) == 3600                                    # 2H delay


def test_world_sql_emits_every_set_piece_with_required_level_80():
    block = emit_world_sql().split("INSERT INTO `item_template`")[1].split(";")[0]
    for r in SET_RECIPES:
        assert f"'{r.output.name}'" in block
    # RequiredLevel is the 11th column; spot-check a plate-tank chest carries the 80 gate + stamina.
    chest = next(r for r in SET_RECIPES if r.output.name == "Branded Bulwark Chestguard")
    row = re.search(rf"\(\s*{chest.output.entry},[^)]*\)", block).group(0)
    cells = [c.strip().strip("'") for c in row.strip("()").split(",")]
    assert cells[10] == "80"                               # RequiredLevel
    assert cells[14] == str(C.ITEM_MOD_STAMINA)            # stat_type1 == Stamina for a tank
