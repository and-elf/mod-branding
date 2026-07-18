# #03 — Effect application layer (§7.9)

**Status:** re-opened / superseded by #10 (+#11, #12) · **Deps:** #02 (active-brand/loadout) ·
**Parallel-safe:** after #02 · **Size:** L

> **Why re-opened (GitHub #15).** This was previously marked done, but the acceptance below was
> satisfied only by a **placeholder flat ±% damage multiplier** — never the spec'd mechanic. §7.9
> requires *proc-casting* effects (procs that cast spells, bounded exposure windows, no passive
> uptime); a flat multiplier is exactly the always-on passive §1/§7.9 forbid. The core proc-casting
> engine is now built under epic **#10** (melee vertical slice in flight); wiring the per-school
> school→spell maps is **#11**; retiring the placeholder multiplier is **#12**. This issue stays open
> until the proc engine — not the multiplier — satisfies the acceptance.

## Context
The effect model is fully specified + tested in `core/effects/` (`PersonalMultiplier`,
`RaidMultiplier`, `WindowUptimeFraction`, `EffectProfile`, `IsPrestige`) — those cores are done.
What is **not** done is application: nothing casts the spec'd proc/window effect. The interim
placeholder flat multiplier is not the mechanic and is being retired (#12).
This is the genuinely adapter-heavy slice and the prerequisite for catalyst (#04) and item
branding (#05).

## Scope
- Define the per-(brand, role) `EffectProfile` table source (config/data): kind (PersonalSpike /
  RaidWindow / MechanicTransform), window/cooldown.
- Window scheduling: drive exposure/burst windows via `EventMap`/`TaskScheduler` (no ad-hoc tick
  counters). Respect `WindowUptimeFraction` < 1.0 (no passive uptime).
- Apply during a window:
  - **PersonalSpike** (tank-flavoured): `PersonalMultiplier` → survivability/outgoing via the same
    `UnitScript::Modify*Damage` plumbing used by scaling (compose: scaling first, branding on top —
    §2.1 ordering).
  - **RaidWindow**: `RaidMultiplier` (bounded, catalyst-DR'd — coordinate with #04).
  - **MechanicTransform** (healer): structural change (overheal→shield etc.) — start with one
    transform behind an `AuraScript`/heal hook; others follow.
- Resolve effect strength via `ProficiencyMgr` (level) + active brand (#02) + anti-P2W gate.

## Acceptance
- Standard DoD. New core only if new decisions arise (most logic exists). Adapter integration-tested
  where possible.
- **Proc-casting required — not a flat multiplier.** A branded player's effect must fire as an actual
  proc that **casts a spell** (via the #10 engine, resolving school→spell through the #11 maps), open a
  **bounded exposure/burst window** with `WindowUptimeFraction < 1.0` (no passive uptime), and compose
  on top of scaling per §2.1 ordering. A flat ±% damage multiplier does **not** satisfy this.
- The placeholder flat-multiplier applier is **removed** (#12) — its presence is a fail condition, not
  a partial pass.
- Manual verify: a branded player shows a bounded, windowed, **proc-cast** effect (not always-on and
  not a passive multiplier), inspectable via `.branding info`; the effect is off outside its window.

## Touch points
`src/Effect*.*` (new), `UnitScript` hooks, possibly `AuraScript`/heal hooks. Compose with
`ScalingMgr` ordering (§2.1). Large — consider splitting PersonalSpike / RaidWindow / Transform into
sub-PRs.
