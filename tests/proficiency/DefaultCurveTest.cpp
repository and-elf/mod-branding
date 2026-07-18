#include "branding/proficiency/BrandXp.h"
#include "branding/proficiency/DefaultLevelCurve.h"
#include "branding/proficiency/Proficiency.h"
#include "fakes/FakeClock.h"
#include "fakes/FakeConfig.h"
#include <gtest/gtest.h>

using namespace Branding;
using namespace Branding::Test;

// Guards the *shipped* production level curve (issue #14): the DefaultLevelCurve constants that
// BrandingConfig snapshots into sConfigMgr defaults. These tests pin the single source of truth so a
// retune of the ladder must consciously move a value here, and assert the design intent -- post-cap
// progression is perceptible within a single play session, not imperceptibly slow.
namespace
{
    // A representative *active* post-cap session in Proficiency units. §14.13.6 pins one representative
    // active day at ~1.20M player-XP; the post-cap adapter (§14.13.3) converts that stream into
    // Proficiency units before it reaches the core (ConversionRatio default 0.01 -> ~12,000 units).
    constexpr uint64_t kRepresentativeSessionUnits = 12000;

    // The shipped production curve, mirrored from the DefaultLevelCurve single source of truth.
    FakeConfig ShippedCurveCfg()
    {
        FakeConfig cfg;
        cfg.rankBaseXp = DefaultLevelCurve::RankBaseXp;
        cfg.rankGrowth = DefaultLevelCurve::RankGrowth;
        cfg.maxLevel = DefaultLevelCurve::MaxLevel;
        return cfg;
    }

    uint64_t RankCost(uint8_t rank, IBrandingConfig const& cfg)
    {
        return XpForLevel(rank, cfg) - XpForLevel(static_cast<uint8_t>(rank - 1), cfg);
    }
}

// The heart of #14: a single representative session must yield several brand levels, not zero. With
// the old default (RankBaseXp = 1,670,800) this session buys 0 levels -- imperceptible.
TEST(DefaultCurve, RepresentativeSessionIsPerceptible)
{
    FakeConfig const cfg = ShippedCurveCfg();
    EXPECT_GE(LevelForXp(kRepresentativeSessionUnits, cfg), 5)
        << "a representative post-cap session must move the brand level perceptibly";
}

// Reaching the very first rank must cost less than a single session -- the player sees level 1 quickly.
TEST(DefaultCurve, FirstRankIsReachableWithinASession)
{
    FakeConfig const cfg = ShippedCurveCfg();
    EXPECT_LE(XpForLevel(1, cfg), kRepresentativeSessionUnits);
}

// Shape guard: the ladder stays geometric (steepens toward the cap), so the endgame remains a grind
// even though the opening is quick -- the "easy to start, hard to finish" curve of §7.4 / §14.13.6.
TEST(DefaultCurve, LadderSteepensTowardCap)
{
    FakeConfig const cfg = ShippedCurveCfg();
    EXPECT_GT(RankCost(cfg.maxLevel, cfg), RankCost(1, cfg));
}

// End-to-end pure path (§14.13.3): units routed through ApplyActivity accrue, level up perceptibly,
// and persist in the mutated state -- the same core call the post-cap XP adapter drives.
TEST(DefaultCurve, PostCapUnitsAccruePersistAndLevelPerceptibly)
{
    FakeConfig const cfg = ShippedCurveCfg();
    FakeClock clock;
    KnowledgeState const known{ static_cast<uint32_t>(1u << static_cast<int>(BrandId::Fire)) };

    XpActivity activity;
    activity.source = ActivitySource::Invasion;
    activity.activeBrand = BrandId::Fire;
    activity.contentBrand = BrandId::Fire;
    activity.role = RoleContribution::None;
    activity.baseUnits = static_cast<uint32_t>(kRepresentativeSessionUnits);

    ProficiencyState state;
    XpResult const first = ApplyActivity(state, activity, known, cfg, clock);

    EXPECT_GT(state.totalXp, 0u);                       // accrued
    EXPECT_EQ(state.totalXp, first.xpGained);           // persisted into state
    EXPECT_GE(first.levelAfter, 5);                     // perceptible after one session
    EXPECT_GE(first.levelAfter, first.levelBefore);     // monotonic

    // A second session keeps climbing (accrual persists across calls).
    XpResult const second = ApplyActivity(state, activity, known, cfg, clock);
    EXPECT_GT(state.totalXp, first.xpGained);
    EXPECT_GE(second.levelAfter, first.levelAfter);
}
