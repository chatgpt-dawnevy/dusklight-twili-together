// Keep the enemy headers with conflicting unscoped actions in separate
// translation units (Shadow Keese and Mini Freezard both define ACT_ATTACK).
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_yk.h"

namespace twili::autotest {
void startShadowKeeseNativeDeath(fopAc_ac_c& actor) {
    auto& keese = static_cast<e_yk_class&>(actor);
    keese.health = 0;
    keese.mDeathFlag = 1;
    keese.mKnockbackSpeed = 70.0f;
    keese.mKnockbackAngle = keese.shape_angle.y;
}
}
