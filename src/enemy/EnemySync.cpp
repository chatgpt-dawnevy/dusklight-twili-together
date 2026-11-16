// Shares supported enemy defeats, using the native ending for selected bosses.

#include "enemy/EnemySync.hpp"

#include "boss/BossFight.hpp"
#include "core/Log.hpp"
#include "core/SaveGate.hpp"
#include "core/Session.hpp"
#include "enemy/EnemyCount.hpp"
#include "sync/RemoteApplyGuard.hpp"

#include "m_Do/m_Do_ext.h"  // the enemy headers are not self-contained

#include "d/d_kankyo.h"  // LIGHT_INFLUENCE for d_a_e_fm.h

#include "d/actor/d_a_b_bq.h"
#include "d/actor/d_a_e_dt.h"
#include "d/actor/d_a_e_fm.h"
#include "d/actor/d_a_e_gob.h"
#include "d/actor/d_a_e_hz.h"
#include "d/actor/d_a_e_s1.h"
#include "d/actor/d_a_e_sb.h"
#include "d/actor/d_a_e_yg.h"
#include "d/actor/d_a_e_vt.h"
#include "d/actor/d_a_e_yk.h"
#include "d/actor/d_a_e_ymb.h"
#include "d/actor/d_a_e_sm2.h"
#include "d/actor/d_a_kago.h"
#include "d/actor/d_a_player.h"
#include "d/d_bg_w.h"
#include "d/d_camera.h"
#include "d/d_com_inf_game.h"
#include "d/d_event.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_name.h"

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace twili::enemy_sync {
namespace {

using Clock = std::chrono::steady_clock;

// e_rdy deletes 50 frames after its puff, the e_s1 field demo 57.
constexpr auto kPairWindow = std::chrono::seconds(6);
// Our copy's room may still be loading.
constexpr auto kPendingTtl = std::chrono::seconds(10);
// Waiting out a local event or a grab.
constexpr auto kMaxDefer = std::chrono::seconds(60);
// Echo and double-kill suppression.
constexpr auto kRecentTtl = std::chrono::seconds(8);
// A puff this recent means "dying".
constexpr auto kStillDying = std::chrono::seconds(2);
constexpr float kHomeTolerance = 1.0f;
constexpr size_t kMaxPending = 64;
constexpr size_t kMaxRecent = 256;

// Enemies that die without being deleted: synced as "defeated in place".
struct InPlaceDeath {
    s16 procName;
    bool (*isDead)(const fopAc_ac_c*);
    void (*kill)(fopAc_ac_c*);
    // A boss may have its own introductory scene or a player interaction to finish first.
    bool (*ready)(const fopAc_ac_c*);
    bool needsPuff;
};

bool hzIsDead(const fopAc_ac_c* ac) {
    return static_cast<const daE_HZ_c*>(ac)->field_0x6e8 != 0;
}

// executeDeathWait's created-dead branch: no switch write (the killer's comes by world sync).
void hzKill(fopAc_ac_c* ac) {
    auto* hz = static_cast<daE_HZ_c*>(ac);
    // Its execute stops running, so nothing would release a force lock later.
    if (dCamera_c* cam = dCam_getBody(); cam != nullptr && cam->GetForceLockOnActor() == ac) {
        cam->ForceLockOff(ac);
    }
    if (hz->mpBgW != nullptr) {
        dComIfG_Bgsp().Release(hz->mpBgW);
        hz->mpBgW = nullptr;
    }
    hz->setActionMode(11);  // ACTION_DEATH_WAIT
    hz->mMode = 1;
}

// ACT_DEATH in d_a_e_dt.cpp. Start the native sequence so it creates its chest and clear flag.
constexpr int kDekuToadDeath = 9;
constexpr int kDekuToadOpening = 10;

bool dekuToadIsDead(const fopAc_ac_c* ac) {
    return static_cast<const daE_DT_c*>(ac)->mAction == kDekuToadDeath;
}

void dekuToadKill(fopAc_ac_c* ac) {
    auto* toad = static_cast<daE_DT_c*>(ac);
    toad->health = 0;
    toad->mAction = kDekuToadDeath;
    toad->mMode = 0;
}

bool dekuToadReady(const fopAc_ac_c* ac) {
    const auto* toad = static_cast<const daE_DT_c*>(ac);
    return toad->mAction != kDekuToadOpening && !toad->mLinkPressed;
}

// d_a_b_bq.cpp: the body starts the native clear with ACTION_END and demo mode 50.
constexpr int kDiababaStay = 0;
constexpr int kDiababaDeath = 4;
constexpr int kDiababaDeathDemo = 50;

bool diababaIsDead(const fopAc_ac_c* ac) {
    return static_cast<const b_bq_class*>(ac)->mAction == kDiababaDeath;
}

bool diababaReady(const fopAc_ac_c* ac) {
    const auto* diababa = static_cast<const b_bq_class*>(ac);
    // The native clear dereferences its bomb-carrying monkey as soon as the event starts.
    return diababa->mAction != kDiababaStay && diababa->mDemoMode == 0 &&
           fopAcM_IsExecuting(diababa->mMonkeyBombID);
}

void diababaKill(fopAc_ac_c* ac) {
    auto* diababa = static_cast<b_bq_class*>(ac);
    diababa->mAction = kDiababaDeath;
    diababa->mMode = 0;
    diababa->mDemoMode = kDiababaDeathDemo;
}

// ACTION_START and ACTION_END in d_a_e_fm.cpp.
constexpr int kFyrusOpening = 11;
constexpr int kFyrusDeath = 12;

bool fyrusIsDead(const fopAc_ac_c* ac) {
    return reinterpret_cast<const e_fm_class*>(ac)->mAction == kFyrusDeath;
}

bool fyrusReady(const fopAc_ac_c* ac) {
    const auto* fyrus = reinterpret_cast<const e_fm_class*>(ac);
    return fyrus->mAction != kFyrusOpening && fyrus->mDemoCamMode == 0 &&
           fyrus->mChain[2].field_0x617c == 0 && fyrus->mChain[3].field_0x617c == 0;
}

void fyrusKill(fopAc_ac_c* ac) {
    auto* fyrus = reinterpret_cast<e_fm_class*>(ac);
    fyrus->health = 0;
    fyrus->mChain[2].field_0x617e = 0;
    fyrus->mChain[3].field_0x617e = 0;
    fyrus->mAction = kFyrusDeath;
    fyrus->mMode = 0;
}

// ACTION_DEATH and the first introductory action in d_a_e_ymb.cpp.
constexpr int kTwilitBloatDeath = 9;
constexpr int kTwilitBloatOpening = 10;

bool twilitBloatIsDead(const fopAc_ac_c* ac) {
    return static_cast<const daE_YMB_c*>(ac)->mAction == kTwilitBloatDeath;
}

bool twilitBloatReady(const fopAc_ac_c* ac) {
    return static_cast<const daE_YMB_c*>(ac)->mAction < kTwilitBloatOpening &&
           daPy_py_c::checkNowWolf();
}

void twilitBloatKill(fopAc_ac_c* ac) {
    auto* bloat = static_cast<daE_YMB_c*>(ac);
    if (bloat->mpBgW != nullptr && bloat->mpBgW->ChkUsed()) {
        dComIfG_Bgsp().Release(bloat->mpBgW);
    }
    {
        sync::RemoteApplyGuard guard;
        dComIfGs_offOneZoneSwitch(1, fopAcM_GetRoomNo(bloat));
    }
    bloat->health = 0;
    bloat->mAction = kTwilitBloatDeath;
    bloat->mMode = 0;
}

// Both the bomb and exposed-tongue routes converge on the death animation.
bool shellBladeIsDead(const fopAc_ac_c* ac) {
    const int action = static_cast<const daE_SB_c*>(ac)->field_0x5b0;
    return action == 2 || action == 3;
}

void shellBladeKill(fopAc_ac_c* ac) {
    auto& shell = *static_cast<daE_SB_c*>(ac);
    shell.field_0x5e6 = 0;
    sync::RemoteApplyGuard guard;
    shell.ChanceMotion();
}

bool deathSwordIsDead(const fopAc_ac_c* ac) {
    return static_cast<const daE_VA_c*>(ac)->mAction == daE_VA_c::ACTION_OPACI_DEATH_e;
}

bool deathSwordReady(const fopAc_ac_c* ac) {
    return static_cast<const daE_VA_c*>(ac)->mAction >= daE_VA_c::ACTION_CLEAR_WAIT_e;
}

void deathSwordKill(fopAc_ac_c* ac) {
    auto& sword = *static_cast<daE_VA_c*>(ac);
    sword.field_0x1381 = 1;
    sword.setActionMode(daE_VA_c::ACTION_OPACI_DEATH_e, 0);
}

// Dangoro becomes a friendly actor after his defeat scene. The lava demo
// controls the platform and writes the dungeon's progression switch.
bool dangoroIsDead(const fopAc_ac_c* ac) {
    const auto& goron = *static_cast<const e_gob_class*>(ac);
    return goron.mAction >= 10 || (goron.mDemoCamMode >= 10 && goron.mDemoCamMode <= 15);
}

bool dangoroReady(const fopAc_ac_c* ac) {
    const auto& goron = *static_cast<const e_gob_class*>(ac);
    return goron.mAction >= 2 && goron.mAction < 10 && goron.mDemoCamMode == 0;
}

void dangoroKill(fopAc_ac_c* ac) {
    auto& goron = *static_cast<e_gob_class*>(ac);
    goron.mAction = 10;
    goron.mMode = 0;
    goron.mDemoCamMode = 10;
    goron.field_0x6d6 = 2;
}

bool chuHasPlacedColor(const fopAc_ac_c* ac, const e_sm2_class& chu) {
    return chu.type == ((fopAcM_GetParam(ac) >> 4) & 0xF);
}

bool chuIsJelly(const fopAc_ac_c* ac) {
    const auto& chu = *reinterpret_cast<const e_sm2_class*>(ac);
    return chu.action == ACTION_FAIL && chuHasPlacedColor(ac, chu);
}

bool chuCanBecomeJelly(const fopAc_ac_c* ac) {
    const auto& chu = *reinterpret_cast<const e_sm2_class*>(ac);
    return chu.sizetype == 0 && !chu.is_roof && chuHasPlacedColor(ac, chu) &&
        (chu.action == ACTION_NORMAL_MOVE || chu.action == ACTION_ATTACK);
}

void chuBecomeJelly(fopAc_ac_c* ac) {
    // The smallest Chu enters its native jelly state after a hit.
    auto& chu = *reinterpret_cast<e_sm2_class*>(ac);
    chu.action = ACTION_FAIL;
    chu.mode = 0;
    chu.field_0x6b0 = 1.5f;
    chu.timers[0] = 0;
    chu.isPiece = TRUE;
    ac->speed.y = 20.0f;
    chu.field_0xfd4 = 1;
    fopAcM_OffStatus(ac, fopAcStts_CULL_e);
}

constexpr InPlaceDeath kInPlace[] = {
    {fpcNm_E_SM2_e, chuIsJelly, chuBecomeJelly, nullptr, false},
    {fpcNm_E_GOB_e, dangoroIsDead, dangoroKill, dangoroReady, false},
    {fpcNm_E_VT_e, deathSwordIsDead, deathSwordKill, deathSwordReady, false},
    {fpcNm_E_SB_e, shellBladeIsDead, shellBladeKill, nullptr, false},
    {fpcNm_B_BQ_e, diababaIsDead, diababaKill, diababaReady, false},
    {fpcNm_E_HZ_e, hzIsDead, hzKill, nullptr, true},
    {fpcNm_E_DT_e, dekuToadIsDead, dekuToadKill, dekuToadReady, false},
    {fpcNm_E_FM_e, fyrusIsDead, fyrusKill, fyrusReady, false},
    {fpcNm_E_YMB_e, twilitBloatIsDead, twilitBloatKill, twilitBloatReady, false},
};

const InPlaceDeath* findInPlace(s16 procName) {
    for (const InPlaceDeath& d : kInPlace) {
        if (d.procName == procName) {
            return &d;
        }
    }
    return nullptr;
}

uint64_t s_nextIncarnation = 1;

struct Record {
    uint64_t incarnation = 0;
    fopAc_ac_c* actor;  // valid until onActorDeleted
    SpawnKey key;
    const InPlaceDeath* inPlace = nullptr;
    detail::Location location;
    uint8_t fxSize = 10, fxType = 0;
    Clock::time_point disappearAt{}, deleteAt{};
    // Explicit mark: dies without a puff.
    bool defeated = false;
    // Sent, or dying by its own hand when a teammate's kill came.
    bool reported = false;
    bool remoteRemoved = false;
};

struct Out {
    Kill kill;
    detail::Location location;
};

struct Pending {
    Kill kill;
    detail::Location location;
    Clock::time_point at;
    bool deferred = false;
    uint64_t incarnation = 0;
};

struct Recent {
    SpawnKey key;
    detail::Location location;
    Clock::time_point at;
};

std::unordered_map<fpc_ProcID, Record> s_records;
std::vector<Out> s_outbox;
std::vector<Pending> s_pending;
std::vector<Recent> s_recent;
// Around our own fopAcM_delete of a remote kill, so the hook does not stamp it.
bool s_applyingRemote = false;
Stats s_stats{};

void logKill(const char* what, const Kill& k) {
    TwiliLog.info("[enemy] {} ({})", what, keyText(k.key));
}

// An explicit allowlist: the ENEMY group also holds bosses and mini-bosses.
bool isKillSyncable(s16 procName, u32 params) {
    switch (procName) {
    case fpcNm_E_OC_e:   // Bokoblin (its fall death has no puff: markDefeated)
    case fpcNm_E_MF_e:   // Dynalfos
    case fpcNm_E_DN_e:   // Lizalfos
    case fpcNm_E_KR_e:   // Kargorok
    case fpcNm_E_SF_e:   // Stalfos (only the final death puffs)
    case fpcNm_E_SB_e:   // Shell Blade (native death and switch)
    case fpcNm_E_ST_e:   // Skulltula
    case fpcNm_E_MM_e:   // Helmasaur
    case fpcNm_E_DD_e:   // Dodongo
    case fpcNm_E_KK_e:   // Chilfos
    case fpcNm_E_GI_e:   // Gibdo
    case fpcNm_E_SH_e:   // Stalhound
    case fpcNm_E_WW_e:   // White Wolfos
    case fpcNm_E_FS_e:   // Wooden Puppet
    case fpcNm_E_SM_e:   // Chu Worm
    case fpcNm_E_S1_e:   // Shadow Beast (the group goes through onShadowBeastGroupDown)
    case fpcNm_E_BA_e:   // Keese
    case fpcNm_E_DB_e:   // Deku Baba (sets its zone actor bit)
    case fpcNm_E_TT_e:   // Tektite
    case fpcNm_E_BU_e:   // Bubble
    case fpcNm_E_RB_e:   // Leever
    case fpcNm_E_CR_e:   // Bombskit
    case fpcNm_E_YK_e:   // Shadow Keese
    case fpcNm_E_DK_e:   // Bari
    case fpcNm_E_GE_e:   // Guay
    case fpcNm_E_SG_e:   // Skullfish
    case fpcNm_E_TK_e:   // Water Toadpoli
    case fpcNm_E_TK2_e:  // Fire Toadpoli
    case fpcNm_E_WS_e:   // Wall Skulltula
    case fpcNm_B_BQ_e:   // Diababa
    case fpcNm_E_GOB_e:  // Dangoro
    case fpcNm_E_VT_e:   // Death Sword
    case fpcNm_E_DT_e:   // Deku Toad
    case fpcNm_E_FM_e:   // Fyrus
    case fpcNm_E_YMB_e:  // Twilit Bloat
    case fpcNm_E_KG_e:   // Young Gohma
    case fpcNm_E_BS_e:   // Stalkin (no puff: markDefeated)
    case fpcNm_E_HZ_e:   // Tile Worm (defeated in place)
        return true;
    case fpcNm_E_SM2_e: {
        const unsigned variant = params & 0xF;
        const unsigned color = (params >> 4) & 0xF;
        const unsigned size = (params >> 8) & 0xFF;
        // Stable colors only, on independently placed, non-respawning small Chus.
        return (variant == 0 || variant == 1 || variant == 0xF) &&
            (color <= TYPE_PURPLE || color == TYPE_BLACK) &&
            (variant == 1 || size == 0 || size == 0xFF);
    }
    case fpcNm_E_FZ_e:   // Blizzeta owns and controls variant 3.
        return (params & 0xFF) != 3;
    case fpcNm_E_NZ_e:   // Ghoul Rat; variant 1 reappears at its spawn.
    case fpcNm_E_MS_e:   // Rat; variant 1 resets itself after a kill.
    case fpcNm_E_YG_e:   // Twilight Vermin; variant 1 resets itself after a kill.
        return (params & 0xFF) != 1;
    case fpcNm_E_YM_e:   // Shadow Insect, including the river sequence.
        return true;
    case fpcNm_E_RD_e: {
        // Bulblin, not King Bulblin (4, 5, 11, 12): scripted fights.
        const u8 arg0 = params & 0xFF;
        return arg0 != 4 && arg0 != 5 && arg0 != 11 && arg0 != 12;
    }
    case fpcNm_E_RDY_e:
        // Shadow Bulblin, not variant 12: the story Kargorok keeps a raw pointer to its rider.
        return ((params & 0xF000) >> 12) != 12;
    default:
        return false;
    }
}

// isActor/onActor assert a setID below ACTOR_MAX and a live zone for the room. An extra's setId is
// its original's.
bool zoneOk(const SpawnKey& k) {
    if (k.dup != 0 || k.setId >= dSv_zoneActor_c::ACTOR_MAX || k.roomNo < 0 || k.roomNo >= 64) {
        return false;
    }
    const int zoneNo = dComIfGp_roomControl_getZoneNo(k.roomNo);
    return zoneNo >= 0 && zoneNo < dSv_info_c::ZONE_MAX;
}

bool isUnset(Clock::time_point t) {
    return t == Clock::time_point{};
}

// Bounded: every remote kill scans it, and nothing limits how many a teammate sends.
void noteRecent(const SpawnKey& key, const detail::Location& location) {
    if (s_recent.size() >= kMaxRecent) {
        s_recent.erase(s_recent.begin());
    }
    s_recent.push_back({key, location, Clock::now()});
}

void report(Record& r, bool zoneActor) {
    r.reported = true;
    noteRecent(r.key, r.location);
    s_outbox.push_back({{r.key, r.fxSize, r.fxType, zoneActor}, r.location});
    TwiliLog.info("[enemy] {} defeated ({}{})", fopAcM_getProcNameString(r.actor), keyText(r.key),
        zoneActor ? ", zone actor bit" : "");
}

Record* live(fpc_ProcID id) {
    if (s_applyingRemote) {
        return nullptr;
    }
    const auto it = s_records.find(id);
    return it == s_records.end() ? nullptr : &it->second;
}

Record* live(const fopAc_ac_c* ac) {
    return ac == nullptr ? nullptr : live(fopAcM_GetID(ac));
}

enum class Apply { Obsolete, Done, NoMatch, Deferred };

// createDisappear's parameters, enemy id 0xFF (no item roll), queued in the enemy's layer.
void spawnPuff(fopAc_ac_c* ac, layer_class* layer, const Kill& kill) {
    if (layer == nullptr) {
        return;
    }
    const u32 prm = (0xFFu << 16) | (u32(kill.fxSize) << 8) | kill.fxType;
    layer_class* saved = fpcLy_CurrentLayer();
    fpcLy_SetCurrentLayer(layer);
    fopAcM_create(fpcNm_DISAPPEAR_e, prm, &ac->current.pos, fopAcM_GetRoomNo(ac),
        &ac->current.angle, nullptr, -1);
    fpcLy_SetCurrentLayer(saved);
}

// Why removing the enemy must wait (our cutscene, Link holds or bites it), or nullptr.
const char* deferReason(fopAc_ac_c* ac) {
    if (dComIfGp_event_runCheck()) {
        switch (dComIfGp_event_getMode()) {
        case dEvt_mode_TALK_e:
            return "a conversation is running";
        case dEvt_mode_DEMO_e:
            return "a cutscene is running";
        default:
            return "an event is running";
        }
    }
    daPy_py_c* player = daPy_getPlayerActorClass();
    if (player == nullptr) {
        return "no player";
    }
    if (player->getGrabActorID() == fopAcM_GetID(ac)) {
        return "Link is holding it";
    }
    if (player->checkWolfEnemyBiteAllOwn(ac)) {
        return "Link is biting it";
    }
    return nullptr;
}

void releaseKargorokTarget(fopAc_ac_c* actor, const SpawnKey& key) {
    if (key.procName != fpcNm_E_YM_e || (key.params & 0xFF) != 7) return;
    fopAc_ac_c* mount = nullptr;
    fopAcM_SearchByName(fpcNm_KAGO_e, &mount);
    if (mount == nullptr) return;
    auto& kargorok = *static_cast<daKago_c*>(mount);
    if (kargorok.getLockActor() == actor) kargorok.setLockActor(nullptr);
}

void applyDeathSwitch(fopAc_ac_c* actor, const SpawnKey& key) {
    unsigned bit = 0xFF;
    if (key.procName == fpcNm_E_YM_e || key.procName == fpcNm_E_YK_e) {
        bit = key.params >> 24;
    } else if (key.procName == fpcNm_E_GE_e || key.procName == fpcNm_E_WS_e) {
        bit = (key.params >> 8) & 0xFF;
    } else if (key.procName == fpcNm_E_YG_e || key.procName == fpcNm_E_MS_e) {
        bit = (key.params >> 16) & 0xFF;
    }
    if (bit != 0xFF) {
        sync::RemoteApplyGuard guard;
        dComIfGs_onSwitch(bit, fopAcM_GetRoomNo(actor));
    }
}

#if TWILI_ENABLE_AUTOTEST
bool s_holdRemoteDefeat = false;
#endif

Apply tryApply(Pending& p, const char*& why) {
    const auto now = Clock::now();
    for (auto& [id, r] : s_records) {
        if ((p.incarnation != 0 && p.incarnation != r.incarnation) ||
            r.reported || r.remoteRemoved || !p.location.matches(r.location) ||
            !sameKey(r.key, p.kill.key))
        {
            continue;
        }
        if (p.incarnation == 0) p.incarnation = r.incarnation;
        // Before the IsExecuting skip: an unconsumed kill would remove a respawned enemy.
        const bool executing = fopAcM_IsExecuting(id);
        if (r.defeated || (!isUnset(r.disappearAt) && now - r.disappearAt < kStillDying))
        {
            // Dying by its own hand here: let it finish with its own drop, and do not report it.
            r.reported = true;
            return Apply::Done;
        }
        if (!executing) {
            // Plain scene teardown or despawn is not a native defeat.
            if (!isUnset(r.deleteAt)) return Apply::Obsolete;
            continue;
        }
        fopAc_ac_c* ac = r.actor;
        if (r.inPlace != nullptr && r.inPlace->isDead(ac)) {
            return Apply::Done;
        }
#if TWILI_ENABLE_AUTOTEST
        if (s_holdRemoteDefeat) {
            why = "the autotest holds the defeat";
            return Apply::Deferred;
        }
#endif
        why = deferReason(ac);
        if (why != nullptr) {
            return Apply::Deferred;
        }
        if (r.inPlace != nullptr && r.inPlace->ready != nullptr &&
            (dComIfGp_isEnableNextStage() || !boss::fighting() ||
                !boss::inArena(r.location.stage.data(), fopAcM_GetRoomNo(ac)) ||
                fopAcM_GetRoomNo(ac) != dStage_roomControl_c::getStayNo() || !r.inPlace->ready(ac)))
        {
            why = "the local boss fight is not ready";
            return Apply::Deferred;
        }
        if (r.key.procName == fpcNm_E_SM2_e && !chuCanBecomeJelly(ac)) {
            return Apply::Obsolete;
        }
        if (r.inPlace != nullptr) {
            r.inPlace->kill(ac);
            r.remoteRemoved = true;
            if (r.inPlace->needsPuff) {
                spawnPuff(ac, ac->layer_tag.layer, p.kill);
            }
            ++s_stats.applied;
            TwiliLog.info("[enemy] {} ({}) defeated in place by a teammate",
                fopAcM_getProcNameString(ac), keyText(r.key));
            return Apply::Done;
        }
        // The actor stays readable until fopAc_Delete runs next frame.
        layer_class* layer = ac->layer_tag.layer;
        s_applyingRemote = true;
        const bool deleted = fopAcM_delete(ac) != 0;
        s_applyingRemote = false;
        if (!deleted) {
            why = "its delete was refused";
            return Apply::Deferred;
        }
        r.remoteRemoved = true;
        releaseKargorokTarget(ac, r.key);
        applyDeathSwitch(ac, r.key);
        spawnPuff(ac, layer, p.kill);
        ++s_stats.applied;
        TwiliLog.info("[enemy] removed {} ({}): defeated by a teammate",
            fopAcM_getProcNameString(ac), keyText(r.key));
        return Apply::Done;
    }
    return p.incarnation != 0 ? Apply::Obsolete : Apply::NoMatch;
}

}  // namespace

detail::Location detail::currentLocation() {
    Location location{};
    if (Session::active()) location.loadEpoch = Session::instance().enemyZoneEpoch();
    if (const char* stage = dComIfGp_getStartStageName(); stage != nullptr) {
        std::strncpy(location.stage.data(), stage, location.stage.size() - 1);
        location.layer = dComIfG_play_c::getLayerNo(0);
    }
    return location;
}

std::string keyText(const SpawnKey& k) {
    return fmt::format("proc {}, room {}, setId 0x{:04X}, home {:.1f} {:.1f} {:.1f}{}", k.procName,
        k.roomNo, k.setId, k.home[0], k.home[1], k.home[2],
        k.dup != 0 ? fmt::format(", extra {}", k.dup) : std::string{});
}

bool sameKey(const SpawnKey& a, const SpawnKey& b) {
    return a.procName == b.procName && a.roomNo == b.roomNo && a.params == b.params &&
           a.setId == b.setId && a.dup == b.dup &&
           std::fabs(a.home[0] - b.home[0]) <= kHomeTolerance &&
           std::fabs(a.home[1] - b.home[1]) <= kHomeTolerance &&
           std::fabs(a.home[2] - b.home[2]) <= kHomeTolerance;
}

bool appendKey(const fopAc_ac_c* actor, SpawnKey& out) {
    auto* ac = const_cast<fopAc_ac_c*>(actor);
    const fopAcM_prm_class* prm = fopAcM_GetAppend(ac);
    // Children of other actors spawn where the parent's timing puts them.
    if (prm == nullptr || prm->parent_id != fpcM_ERROR_PROCESS_ID_e) {
        return false;
    }
    const cXyz home = prm->base.position;
    out = {fopAcM_GetName(ac), prm->room_no, static_cast<uint32_t>(prm->base.parameters),
        static_cast<uint16_t>(prm->base.setID), {home.x, home.y, home.z}};
    return true;
}

const SpawnKey* trackedKey(fpc_ProcID id) {
    const auto it = s_records.find(id);
    return it == s_records.end() ? nullptr : &it->second.key;
}

bool isDying(fpc_ProcID id) {
    const auto it = s_records.find(id);
    if (it == s_records.end()) {
        return false;
    }
    const Record& r = it->second;
    return r.defeated || r.reported || r.remoteRemoved || !isUnset(r.deleteAt) ||
           (!isUnset(r.disappearAt) && Clock::now() - r.disappearAt < kStillDying) ||
           (r.inPlace != nullptr && r.inPlace->isDead(r.actor));
}

void onActorCreated(fopAc_ac_c* ac) {
    if (!isKillSyncable(fopAcM_GetName(ac), fopAcM_GetParam(ac))) {
        return;
    }
    const auto location = detail::currentLocation();
    Record r{};
    // An extra goes by its original's key, a spawner's child by its spawner's; others by the spawn
    // data, readable only now.
    if (location.stage[0] == '\0' ||
        (!enemy_count::extraKey(fopAcM_GetID(ac), r.key) && !appendKey(ac, r.key) &&
            !enemy_count::broodKey(ac, r.key)))
    {
        return;
    }
    r.incarnation = s_nextIncarnation++;
    r.actor = ac;
    r.inPlace = findInPlace(r.key.procName);
    r.location = location;
    s_records[fopAcM_GetID(ac)] = r;
}

void onDisappear(const fopAc_ac_c* ac, uint8_t size, uint8_t type) {
    if (Record* r = live(ac)) {
        r->disappearAt = Clock::now();
        r->fxSize = size;
        r->fxType = type;
    }
}

void onDeleteRequest(const fopAc_ac_c* ac) {
    if (Record* r = live(ac); r != nullptr && isUnset(r->deleteAt)) {
        r->deleteAt = Clock::now();
        // Twilight Vermin's only explicit deletion is in its native defeat routine.
        if (r->key.procName == fpcNm_E_YG_e &&
            reinterpret_cast<const e_yg_class*>(ac)->mDeathFlag != 0)
        {
            r->defeated = true;
        } else if (r->key.procName == fpcNm_E_YK_e) {
            // Its native deaths make particles directly, without a DISAPPEAR actor.
            const auto* keese = reinterpret_cast<const e_yk_class*>(ac);
            r->defeated = keese->mDeathFlag != 0 || keese->mAction == ACT_WOLFBITE;
        }
    }
}

void onDeleteRequest(fpc_ProcID id) {
    if (Record* r = live(id); r != nullptr && isUnset(r->deleteAt)) {
        r->deleteAt = Clock::now();
    }
}

void markDefeated(const fopAc_ac_c* ac) {
    if (Record* r = live(ac)) {
        r->defeated = true;
    }
}

void onActorDeleted(fopAc_ac_c* ac) {
    const auto it = s_records.find(fopAcM_GetID(ac));
    if (it == s_records.end()) {
        return;
    }
    Record r = it->second;
    s_records.erase(it);
    if (r.reported || r.remoteRemoved) {
        return;
    }
    const bool paired = r.key.procName != fpcNm_E_FZ_e && !isUnset(r.disappearAt) && !isUnset(r.deleteAt) &&
                        (r.disappearAt > r.deleteAt ? r.disappearAt - r.deleteAt :
                                                      r.deleteAt - r.disappearAt) <= kPairWindow;
    if (!r.defeated && !paired) {
        return;  // room unload, scene teardown, despawn
    }
    // A frame after the delete request: e_db sets its zone actor bit right after asking.
    report(r, zoneOk(r.key) && dComIfGs_isActor(r.key.setId, r.key.roomNo));
}

void onShadowBeastGroupDown(const e_s1_class* self) {
    // Each beast dies 30-40 frames from now; teammates lose the whole group together.
    for (auto& [id, r] : s_records) {
        if (r.key.procName != fpcNm_E_S1_e || r.reported || r.remoteRemoved) {
            continue;
        }
        if (static_cast<const e_s1_class*>(r.actor)->mGroupID != self->mGroupID) {
            continue;
        }
        r.defeated = true;
        // d_a_e_s1.cpp e_s1_fail
        r.fxSize = 12;
        r.fxType = 1;
        report(r, false);
    }
}

void flush() {
    if (s_outbox.empty()) {
        return;
    }
    const Session& session = Session::instance();
    if (session.joined() && session.roomState().syncNPCs && isSaveLoaded()) {
        // One packet per run of kills from the same stage and layer.
        Kill batch[kMaxKillsPerPacket];
        size_t i = 0;
        while (i < s_outbox.size()) {
            const Out& first = s_outbox[i];
            size_t n = 0;
            while (i < s_outbox.size() && n < kMaxKillsPerPacket &&
                   s_outbox[i].location.matches(first.location))
            {
                batch[n++] = s_outbox[i++].kill;
            }
            if (first.location.loadEpoch == session.enemyZoneEpoch() && detail::sendDefeated(first.location.stage.data(), first.location.layer, batch, n)) {
                s_stats.sent += static_cast<uint32_t>(n);
            }
        }
    }
    // Kills made offline or with the option off are never sent later.
    s_outbox.clear();
}

void detail::queueRemote(const Kill& kill) {
    ++s_stats.received;
    if (!isKillSyncable(kill.key.procName, kill.key.params)) {
        return;
    }
    const Location location = currentLocation();
    for (const Recent& recent : s_recent) {
        if (location.matches(recent.location) &&
            sameKey(recent.key, kill.key))
        {
            ++s_stats.echoes;
            return;
        }
    }
    noteRecent(kill.key, location);
    logKill("a teammate defeated an enemy", kill);
    // Mirrored without a live copy too: the bit keeps it from coming back with its room.
    if (kill.zoneActor && !boss::isBossProfile(kill.key.procName) && zoneOk(kill.key)) {
        dComIfGs_onActor(kill.key.setId, kill.key.roomNo);
    }
    if (s_pending.size() >= kMaxPending) {
        s_pending.erase(s_pending.begin());
    }
    s_pending.push_back({kill, location, Clock::now()});
}

void tick() {
    const auto now = Clock::now();
    std::erase_if(s_recent, [&](const auto& e) { return now - e.at > kRecentTtl; });
    // A native boss death is an action. The Tile Worm also needs its local death puff.
    for (auto& [id, r] : s_records) {
        if (r.inPlace != nullptr && !r.reported && !r.remoteRemoved &&
            (!r.inPlace->needsPuff || (!isUnset(r.disappearAt) && now - r.disappearAt <= kPairWindow)) &&
            fopAcM_IsExecuting(id) && r.inPlace->isDead(r.actor))
        {
            report(r, r.inPlace->needsPuff && zoneOk(r.key) &&
                dComIfGs_isActor(r.key.setId, r.key.roomNo));
        }
    }
    const Session& session = Session::instance();
    if (!session.isConnected() || !session.roomState().syncNPCs || !isSaveLoaded()) {
        s_pending.clear();
        return;
    }
    const detail::Location current = detail::currentLocation();
    for (auto it = s_pending.begin(); it != s_pending.end();) {
        if (!it->location.matches(current)) {
            ++s_stats.expired;
            it = s_pending.erase(it);
            continue;
        }
        if (now - it->at > (it->deferred ? kMaxDefer : kPendingTtl)) {
            ++s_stats.expired;
            logKill(it->deferred ? "a teammate's kill expired, never allowed to apply" :
                                   "a teammate's kill expired, no matching enemy here",
                it->kill);
            it = s_pending.erase(it);
            continue;
        }
        const char* why = nullptr;
        switch (tryApply(*it, why)) {
        case Apply::Obsolete:
            ++s_stats.expired;
            logKill("a teammate's deferred kill lost its original actor", it->kill);
            it = s_pending.erase(it);
            break;
        case Apply::Done:
            it = s_pending.erase(it);
            break;
        case Apply::Deferred:
            if (!it->deferred) {
                TwiliLog.info("[enemy] removing a teammate's kill waits: {}", why);
            }
            it->deferred = true;
            ++it;
            break;
        case Apply::NoMatch:
            ++it;
            break;
        }
    }
}

#if TWILI_ENABLE_AUTOTEST
void holdRemoteDefeatForTest(bool hold) {
    s_holdRemoteDefeat = hold;
}
#endif

void resetSession() {
    // Records stay: they mirror live actors, connected or not.
    s_outbox.clear();
    s_pending.clear();
    s_recent.clear();
}

void shutdown() {
    resetSession();
    s_records.clear();
}

const Stats& stats() {
    return s_stats;
}

}  // namespace twili::enemy_sync
