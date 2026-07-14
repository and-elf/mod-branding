# #32 — Branded archetype armour sets (issue "add branded items and patterns")

**Status:** implemented (catalog + generated world SQL + tests) — client MPQ rebuild is the usual
manual step (#29) · **Deps:** #27 (output/bind model, `branding_recipe` mirror), #29 (craft-spell +
pattern surface, `tools/branding-craft`) · **Parallel-safe:** yes (data + generator) · **Size:** M
**Companion:** #27 / #29 (this extends the same single-source catalog)

## Context
The starter surface (#27/#29) shipped **three** Branded chest pieces (leather/plate/cloth). The
request: broaden that into full **armour sets**, one per armour class × role, "based on ilevel" using
the playerbots BiS table as the reference for slot coverage.

## Decisions (locked with owner, 2026-07-14)
- **Modest base + proc, NOT raid-BiS clones — #27 stands.** The BiS reuse is **coverage + provenance
  only**: each piece is *modelled slot-for-slot* on a real ilvl-226 item (playerbots BiS, WotLK
  Phase-2 / `auto_gear_score_limit = 245`), but carries **modest role stats**, not the source item's
  numbers. Power stays in the branding proc (§1 anti-obsolescence). This is why the issue's
  "3k AP from a level-1 2h" / druid-form / spell-power scaling worries **do not arise** — see below.
- **9 armour slots per set:** head, shoulders, chest, waist, legs, feet, wrists, hands, back. No
  jewelry / trinkets. The **cloak** is `subclass = Misc(0)` so it stays class-agnostic and any class
  can wear it.
- **One signature weapon per set** (added 2026-07-14) — the slot where the brand proc attaches (the
  crafted path brands `EQUIPMENT_SLOT_MAINHAND`; etch = weapons + trinkets). **All Blacksmithing-crafted**
  (owner decision), modest weapon damage (~100 DPS, below the ilvl-226 source) + the set's primary stat,
  `RequiredLevel 80`. Melee sets get a main-hand; the **hunter (mail-DPS) set gets a ranged crossbow**
  (`Branded Pathfinder Crossbow`, modelled on Skyforge Crossbow 45570) — a hunter's real weapon.
  Off-hands / relics / dual-wield off-hands deferred (one signature weapon first).
  - *Runtime (2026-07-14):* the crafted-brand adapter (`ItemBrandingMgr`) is now **multi-slot** — it
    brands/upgrades the item in any brandable slot (the 9 armour slots + main-hand + ranged), not just
    the main-hand. Commands take an optional slot: `.branding itembrand [slot]` and
    `.branding upgradeitem <levels> [slot]` (slot = `head|shoulders|chest|waist|legs|feet|wrists|hands|`
    `back|mainhand|ranged`; default main-hand). `LoadEquipped`/`OnPlayerEquip` cache all equipment
    slots, so per-slot state stays coherent without a relog. So the whole armour set **and** the hunter
    crossbow are brandable via the crafted path; off-hand/trinkets remain the Etch surface's domain.
    `.branding info` now lists the brand state of **every** brandable slot (with an `(etched)` marker)
    plus a help line for the itembrand/upgradeitem/etch commands and the slot tokens.
- **Eight sets — exactly the issue's matrix:** plate (tank, DPS), mail (heal, DPS), leather
  (heal, DPS), cloth (heal, DPS).
- **Endgame equip gate: `RequiredLevel = 80`.** This is the direct answer to the issue's scaling
  questions — the pieces cannot be equipped below 80, so there is no low-level twinking and no
  need to scale AP/SP/base stats down for shapeshift forms. No heirloom mechanics, no `mod-reforge`
  (which does not exist in this repo) are involved.
- **Professions by armour class:** plate → Blacksmithing (164); mail + leather → Leatherworking
  (165); cloth → Tailoring (197). Recipe pattern subclass matches. (A set's cloak, being Misc, is
  crafted under the set's own profession — game-legal; cosmetic-only quirk.)

## Scope
1. **Catalog** (`tools/branding-craft/src/branding_craft/catalog.py`) — `SetSpec` × `SlotSpec` tables
   + `_build_set_recipes()` generating 8 × 9 = **72** `Recipe`s, appended **after** the starter trio
   (so the trio keeps its ids/entries and byte-identical SQL). Modest per-slot stats from a slot-weight
   formula; role-appropriate `stat_type`s; `source_item_id` records the BiS provenance.
2. **Bands** (§16.4, widened to `190000–190999`): set outputs `190200–190499`, set patterns
   `190500–190799`; craft spells `1900013–1900084`, SLA `1900113–1900184`.
3. **Generated SQL** — `data/sql/db-world/2026_06_22_00_native_craft.sql` regenerated
   (`item_template` + `spell_dbc` + `skilllineability_dbc`); the **`branding_recipe` mirror** is seeded
   for all 83 recipes by a new module rev `data/sql/db-world/2026_07_14_00_branding_recipe.sql`
   (`branding-craft recipe-sql`), so `.branding craft <id>` resolves the set/weapon recipes in-game.
   Recipe ids: armour `100–178`, weapons `180–187` (starter trio stays `1–3`).
4. **Tests** — `tests/test_sets.py`; `validate` / lockstep / codestyle emit tests still green.

## Design context (owner, 2026-07-14)
- **Stat scaling reuses `mod-reforge`, not extra DB rows.** The Brand-Rank stat bonus (#05) is applied
  at runtime by driving [`mod-reforge`](https://github.com/and-elf/mod-reforge) via its injected
  `IReforgeConfig` — under a **grant policy** (net-additive), not reforge's default zero-sum
  redistribution. So each piece stays a single modest `item_template` row; no per-rank/per-brand
  variants. `mod-reforge` is not yet vendored here and the branding cores have no stat-application code
  yet — this is the planned seam, unbuilt.
- **A dedicated "Branding" profession was intended, then dropped.** May return later. Because the
  catalog centralises `skill_line` per recipe, re-homing all branded content onto a custom profession
  is a localised catalog change — but the profession itself is a client `SkillLine.dbc` deliverable
  (same MPQ lift as #29). Until then everything piggybacks real professions (BS / LW / Tailoring).

## Known follow-ups (non-blocking)
- **Display ids are placeholders.** Every armour slot shares its class chest model (`_SET_DISPLAYID`);
  **weapons** currently reuse an armour display (`_WEAPON_DISPLAYID`), which will render wrong in-hand —
  weapons need real weapon displayids most urgently. Retarget per piece to the recorded `source_item_id`
  once the world DB is reachable (README: displayid is a tunable; acceptance gates on behaviour, not icon).
- **Craft-spell band is ~75% full** (75/100). Do not widen past `1900099` without re-verifying client
  collision-freedom (the `1900100+` range is the SLA band).

## Acceptance
- `branding-craft validate` passes; `pytest` + `ruff` green; the 72 pieces are BoA, `RequiredLevel 80`,
  gated by their profession, with modest stats; each has a BoP pattern that teaches its craft spell;
  reagent counts stay in lockstep with the `branding_recipe` mirror.

## Touch points
`tools/branding-craft` (catalog, constants, validate, emit, tests),
`data/sql/db-world/2026_06_22_00_native_craft.sql` (generated), `docs/ARCHITECTURE.md` §16.4.
