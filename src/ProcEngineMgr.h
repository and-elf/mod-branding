#ifndef MOD_BRANDING_SRC_PROCENGINEMGR_H
#define MOD_BRANDING_SRC_PROCENGINEMGR_H

#include "ServerRng.h"
#include "branding/proc/ProcEngine.h"
#include <cstdint>

class Player;

namespace Branding
{
    // Adapter for the §7.9.1 proc-CASTING engine (GitHub #10), melee slice. On a melee-damage-dealt
    // hook it builds the pure ProcOpportunity from the live state it can read -- config PPM/base value,
    // the attacker's weapon speed, ProficiencyMgr's anti-P2W effect strength, the §7.9 window phase,
    // and the brand's wired payload spell -- then asks the pure ResolveProc whether the brand procs.
    // The decision (whether/what) lives in the pure core; this is a thin gate + injected-RNG shim.
    //
    // Gated by the SAME knowledge/expressibility check the §7.9 EffectMgr uses (ResolveActiveProfile):
    // inert unless the CURRENT account can express the active brand. No raw Player* is retained --
    // everything resolves from the ObjectGuid at call time (project long-lived-reference rule).
    class ProcEngineMgr
    {
    public:
        static ProcEngineMgr* instance();

        void LoadConfig();
        bool Enabled() const { return _enabled; }

        // Resolve whether `attacker`'s active brand procs on this melee opportunity, and the payload to
        // cast. Returns {fired = false} when disabled, `attacker` is not a proc-expressible branded
        // player, the brand has no wired payload (#11), or the roll misses. The caller casts the
        // payload at the target on a fire (see ProcEngineScripts.cpp).
        ProcResult ResolveMeleeProc(Player* attacker);

    private:
        ProcEngineMgr() = default;

        bool     _enabled = false;
        double   _meleePpm = 0.0;
        int32_t  _meleeBaseValue = 0;
        ServerRng _rng;
    };
}

#define sProcEngineMgr Branding::ProcEngineMgr::instance()

#endif // MOD_BRANDING_SRC_PROCENGINEMGR_H
