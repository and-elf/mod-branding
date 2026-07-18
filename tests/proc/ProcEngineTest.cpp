#include "branding/proc/ProcEngine.h"
#include "fakes/FakeRng.h"
#include <gtest/gtest.h>

using namespace Branding;
using Branding::Test::FakeRng;

namespace
{
    // A fully-castable, sure-fire opportunity: window open, brand expressible, a wired payload, and a
    // per-swing chance of exactly 1.0 (ppm 60 at a 1s weapon => 60*1/60 = 1). Tests tweak one field.
    ProcOpportunity SureFire()
    {
        ProcOpportunity opp;
        opp.ppm = 60.0;
        opp.weaponSpeedS = 1.0;
        opp.effectStrength = 1.0;
        opp.windowActive = true;
        opp.spellId = 2120;
        opp.baseValue = 1000;
        return opp;
    }
}

// ---- ProcChancePerSwing: the §14.3 PPM normalization, one swing wide -------------------------------

TEST(ProcChancePerSwing, ZeroPpmNeverProcs)
{
    EXPECT_DOUBLE_EQ(0.0, ProcChancePerSwing(0.0, 2.5));
}

TEST(ProcChancePerSwing, NonPositiveWeaponSpeedNeverProcs)
{
    EXPECT_DOUBLE_EQ(0.0, ProcChancePerSwing(6.0, 0.0));
    EXPECT_DOUBLE_EQ(0.0, ProcChancePerSwing(6.0, -1.0));
}

TEST(ProcChancePerSwing, ScalesWithPpmAndWeaponSpeed)
{
    // 6 ppm on a 3.6s weapon => 6 * 3.6 / 60 = 0.36 chance per swing.
    EXPECT_NEAR(0.36, ProcChancePerSwing(6.0, 3.6), 1e-9);
    // 60 ppm on a 1s weapon => exactly 1.0.
    EXPECT_NEAR(1.0, ProcChancePerSwing(60.0, 1.0), 1e-9);
}

TEST(ProcChancePerSwing, ClampsAboveOne)
{
    // A slow weapon at high ppm exceeds one expected proc/swing; a swing can fire at most once.
    EXPECT_DOUBLE_EQ(1.0, ProcChancePerSwing(100.0, 2.0));
}

// ---- ResolveProc(roll): the deterministic decision ------------------------------------------------

TEST(ResolveProc, FiresWhenRollBelowChance)
{
    ProcResult const r = ResolveProc(SureFire(), 0.0);
    EXPECT_TRUE(r.fired);
    EXPECT_EQ(2120u, r.payload.spellId);
}

TEST(ResolveProc, DoesNotFireWhenRollAtOrAboveChance)
{
    ProcOpportunity opp = SureFire();
    opp.ppm = 6.0;
    opp.weaponSpeedS = 1.0;   // chance = 0.1
    EXPECT_FALSE(ResolveProc(opp, 0.1).fired);     // roll == chance -> miss (half-open interval)
    EXPECT_FALSE(ResolveProc(opp, 0.5).fired);
    EXPECT_TRUE(ResolveProc(opp, 0.05).fired);     // roll below chance -> fire
}

TEST(ResolveProc, InactiveWindowNeverFires)
{
    ProcOpportunity opp = SureFire();
    opp.windowActive = false;
    EXPECT_FALSE(ResolveProc(opp, 0.0).fired);
}

TEST(ResolveProc, NoPayloadSpellNeverFires)
{
    ProcOpportunity opp = SureFire();
    opp.spellId = 0;
    EXPECT_FALSE(ResolveProc(opp, 0.0).fired);
}

TEST(ResolveProc, NonExpressibleBrandNeverFires)
{
    // effectStrength 0 == anti-P2W CanExpressBrand gate resolved to 0 -> inert.
    ProcOpportunity opp = SureFire();
    opp.effectStrength = 0.0;
    EXPECT_FALSE(ResolveProc(opp, 0.0).fired);
}

TEST(ResolveProc, PayloadBaseScalesWithEffectStrength)
{
    ProcOpportunity opp = SureFire();
    opp.baseValue = 1000;

    opp.effectStrength = 1.0;
    EXPECT_EQ(1000, ResolveProc(opp, 0.0).payload.basePoints);

    opp.effectStrength = 0.5;
    EXPECT_EQ(500, ResolveProc(opp, 0.0).payload.basePoints);

    opp.effectStrength = 0.25;
    EXPECT_EQ(250, ResolveProc(opp, 0.0).payload.basePoints);
}

TEST(ResolveProc, EffectStrengthClampedAtOne)
{
    ProcOpportunity opp = SureFire();
    opp.baseValue = 1000;
    opp.effectStrength = 4.0;   // out of range; must not amplify the payload beyond baseValue
    EXPECT_EQ(1000, ResolveProc(opp, 0.0).payload.basePoints);
}

TEST(ResolveProc, IsDeterministic)
{
    ProcOpportunity const opp = SureFire();
    ProcResult const a = ResolveProc(opp, 0.42);
    ProcResult const b = ResolveProc(opp, 0.42);
    EXPECT_EQ(a.fired, b.fired);
    EXPECT_EQ(a.payload.spellId, b.payload.spellId);
    EXPECT_EQ(a.payload.basePoints, b.payload.basePoints);
}

// ---- ResolveProc(IRng): the injected-RNG convenience overload -------------------------------------

TEST(ResolveProcRng, SureFireFiresForAnySeed)
{
    FakeRng rng(999);
    EXPECT_TRUE(ResolveProc(SureFire(), rng).fired);   // chance 1.0 => any roll in [0,1) is below it
}

TEST(ResolveProcRng, IsReproducibleForAGivenSeed)
{
    ProcOpportunity opp = SureFire();
    opp.ppm = 6.0;   // chance 0.1 -- outcome depends on the drawn roll

    FakeRng a(12345);
    FakeRng b(12345);
    EXPECT_EQ(ResolveProc(opp, a).fired, ResolveProc(opp, b).fired);
}

// ---- MeleeProcSpellId: the #11 school->spell map seam (full map in ProcSpellMapTest) ----------------

TEST(MeleeProcSpellId, ClassicSchoolsAreWiredToConcreteSpells)
{
    // #11 replaced the Fire-only stub with the full map: the classic schools reuse their §14.4
    // Offensive lattice shell (docs/issues/30).
    EXPECT_EQ(42833u, MeleeProcSpellId(BrandId::Fire));      // Fireball
    EXPECT_EQ(122u,   MeleeProcSpellId(BrandId::Frost));     // Frost Nova
    EXPECT_EQ(845u,   MeleeProcSpellId(BrandId::Physical));  // Cleave
}

TEST(MeleeProcSpellId, DeferredExoticSchoolsReturnZero)
{
    // Exotic schools without a cast-at-victim offensive shell stay unwired (0 => no proc) until the
    // §7.10 exotic tree projection lands; #11 leaves the structure extensible.
    EXPECT_EQ(0u, MeleeProcSpellId(BrandId::Void));
    EXPECT_EQ(0u, MeleeProcSpellId(BrandId::Stone));
    EXPECT_EQ(0u, MeleeProcSpellId(BrandId::Chrono));
}
