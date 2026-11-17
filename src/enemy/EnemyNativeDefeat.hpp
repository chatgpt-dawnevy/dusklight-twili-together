#pragma once

#include "dolphin/types.h"

class fopAc_ac_c;

namespace twili::enemy_sync {

struct InPlaceDeath {
    s16 procName;
    bool (*isDead)(const fopAc_ac_c*);
    void (*kill)(fopAc_ac_c*);
    // Arena encounters also require the local boss guard and matching room.
    bool (*ready)(const fopAc_ac_c*);
    bool needsPuff;
    bool (*canStart)(const fopAc_ac_c*) = nullptr;
    // May start a prerequisite native transition. Return true when the ending can start.
    bool (*prepare)(fopAc_ac_c*) = nullptr;
};

const InPlaceDeath* findEncounterDefeat(s16 profile, u32 params);

}  // namespace twili::enemy_sync
