#include "enemy/EnemyNativeDefeat.hpp"

#include "sync/WorldSync.hpp"
#include "sync/WorldSyncState.hpp"

#include "m_Do/m_Do_ext.h"
#include "JSystem/JParticle/JPAEmitter.h"

#include "d/actor/d_a_b_ds.h"
#include "d/actor/d_a_b_mgn.h"
#include "d/actor/d_a_b_ob.h"
#include "d/actor/d_a_b_tn.h"
#include "d/actor/d_a_b_zant.h"
#include "d/actor/d_a_e_hzelda.h"
#include "d/actor/d_a_b_gnd.h"
#include "d/actor/d_a_b_gm.h"
#include "d/actor/d_a_e_gm.h"
#include "d/actor/d_a_e_yr.h"
#include "d/actor/d_a_e_zs.h"
#include "d/actor/d_a_e_pm.h"
#include "d/actor/d_a_e_po.h"
#include "d/actor/d_a_e_hp.h"
#include "d/actor/d_a_e_mk.h"
#include "d/actor/d_a_e_pz.h"
#include "d/actor/d_a_e_rdb.h"
#include "d/d_com_inf_game.h"
#include "f_pc/f_pc_name.h"

#include <cstring>
#include "Z2AudioLib/Z2AudioMgr.h"

namespace twili::enemy_sync {
namespace {

bool stallordDefeated(const fopAc_ac_c* ac) {
    const auto& boss = *static_cast<const daB_DS_c*>(ac);
    return boss.arg0 == daB_DS_c::TYPE_BATTLE_2 && boss.mAction == daB_DS_c::ACT_B2_DEAD;
}

bool stallordReady(const fopAc_ac_c* ac) {
    const auto& boss = *static_cast<const daB_DS_c*>(ac);
    if (boss.arg0 == daB_DS_c::TYPE_BATTLE_2) {
        return boss.mAction != daB_DS_c::ACT_B2_OPENING_DEMO;
    }
    return !boss.mIsOpeningDemo && !boss.mIsDemo;
}

bool prepareStallord(fopAc_ac_c* ac) {
    auto& boss = *static_cast<daB_DS_c*>(ac);
    if (boss.arg0 == daB_DS_c::TYPE_BATTLE_2) {
        return boss.mAction != daB_DS_c::ACT_B2_OPENING_DEMO;
    }
    if (boss.mBossPhase == 0 && boss.mAction != daB_DS_c::ACT_OPENING_DEMO &&
        boss.mAction != daB_DS_c::ACT_DAMAGE)
    {
        // The final spine hit runs the native transition and creates the head actor.
        boss.mBackboneLevel = 2;
        boss.mHandAtLCyl.OffAtSetBit();
        boss.mHandAtRCyl.OffAtSetBit();
        boss.mHandAtLCyl.OffTgShield();
        boss.mHandAtRCyl.OffTgShield();
        if (boss.bitSw3 != 0xFF) {
            fopAcM_onSwitch(&boss, boss.bitSw3);
        }
        boss.setActionMode(daB_DS_c::ACT_DAMAGE, 0);
    }
    return false;
}

void defeatStallord(fopAc_ac_c* ac) {
    auto& boss = *static_cast<daB_DS_c*>(ac);
    boss.health = 0;
    boss.mPlayPatternAnm = false;
    boss.mSoundPos = boss.current.pos;
    if (auto* emitter = dComIfGp_particle_getEmitter(boss.mParticleKey1)) {
        emitter->stopDrawParticle();
    }
    Z2GetAudioMgr()->bgmStop(30, 0);
    Z2GetAudioMgr()->bgmStreamPrepare(0x2000048);
    Z2GetAudioMgr()->bgmStreamPlay();
    boss.mSound.startCreatureVoice(Z2SE_EN_DS_END_V, -1);
    boss.setBck(daB_DS_c::ANM_HEAD_DOWN_DAMAGE, 0, 3.0f, 1.0f);
    boss.setActionMode(daB_DS_c::ACT_B2_DEAD, 0);
}

bool beastGanonDefeated(const fopAc_ac_c* ac) {
    return static_cast<const daB_MGN_c*>(ac)->mActionMode == daB_MGN_c::ACTION_DEATH_e;
}

bool beastGanonReady(const fopAc_ac_c* ac) {
    const auto& boss = *static_cast<const daB_MGN_c*>(ac);
    return boss.mActionMode == daB_MGN_c::ACTION_DOWN_e ||
           boss.mActionMode == daB_MGN_c::ACTION_DOWN_DAMAGE_e;
}

void defeatBeastGanon(fopAc_ac_c* ac) {
    auto& boss = *static_cast<daB_MGN_c*>(ac);
    boss.health = -1;
    boss.setActionMode(daB_MGN_c::ACTION_DEATH_e, 0);
}

bool morpheelDefeated(const fopAc_ac_c* ac) {
    return reinterpret_cast<const b_ob_class*>(ac)->mAction == OB_ACTION_FISH_END;
}

bool morpheelReady(const fopAc_ac_c* ac) {
    const auto& boss = *reinterpret_cast<const b_ob_class*>(ac);
    return boss.mDemoAction == 0 && boss.mHangTimer == 0;
}

bool prepareMorpheel(fopAc_ac_c* ac) {
    auto& boss = *reinterpret_cast<b_ob_class*>(ac);
    if (boss.mAction == OB_ACTION_FISH_NORMAL) {
        return true;
    }
    if (boss.mAction == OB_ACTION_CORE_HAND_MOVE || boss.mAction == OB_ACTION_CORE_CHANCE) {
        // Let core_end allocate and animate the fish form before entering its ending.
        boss.field_0x4744 = 1;
        boss.mAction = OB_ACTION_CORE_END;
        boss.mMode = 0;
    }
    return false;
}

void defeatMorpheel(fopAc_ac_c* ac) {
    auto& boss = *reinterpret_cast<b_ob_class*>(ac);
    boss.mAction = OB_ACTION_FISH_END;
    boss.mMode = 0;
    boss.mDemoAction = 40;
}

bool darknutDefeated(const fopAc_ac_c* ac) {
    return static_cast<const daB_TN_c*>(ac)->mActionMode1 == daB_TN_c::ACT_ENDING;
}

bool prepareDarknut(fopAc_ac_c* ac) {
    auto& enemy = *static_cast<daB_TN_c*>(ac);
    if (enemy.mActionMode1 >= daB_TN_c::ACT_CHASEL) {
        return true;
    }
    if (enemy.mActionMode1 >= daB_TN_c::ACT_WAITH && enemy.mActionMode1 <= daB_TN_c::ACT_DAMAGEH) {
        enemy.mNextBreakPart = 12;
        enemy.setActionMode(daB_TN_c::ACT_CHANGEDEMO, daB_TN_c::ACTION2_0_e);
    }
    return false;
}

void defeatDarknut(fopAc_ac_c* ac) {
    auto& enemy = *static_cast<daB_TN_c*>(ac);
    enemy.setActionMode(daB_TN_c::ACT_ENDING, daB_TN_c::ACTION2_0_e);
}

bool zantDefeated(const fopAc_ac_c* ac) {
    return static_cast<const daB_ZANT_c*>(ac)->mAction == daB_ZANT_c::ACT_LAST_END_DEMO;
}

bool zantReady(const fopAc_ac_c* ac) {
    const auto& boss = *static_cast<const daB_ZANT_c*>(ac);
    return boss.mFightPhase == daB_ZANT_c::PHASE_LAST &&
           boss.mAction >= daB_ZANT_c::ACT_LAST_ATTACK &&
           boss.mAction < daB_ZANT_c::ACT_LAST_END_DEMO;
}

void defeatZant(fopAc_ac_c* ac) {
    auto& boss = *static_cast<daB_ZANT_c*>(ac);
    // Run the native final damage entry, which disables its attack spheres.
    boss.health = 0;
    boss.mTakenBigDmg = false;
    boss.setActionMode(daB_ZANT_c::ACT_LAST_DAMAGE, 0);
}

constexpr int kZeldaDamage = 5;

bool puppetZeldaDefeated(const fopAc_ac_c* ac) {
    const auto& zelda = *reinterpret_cast<const e_hzelda_class*>(ac);
    return zelda.mAction == kZeldaDamage && ac->health <= 0;
}

bool puppetZeldaReady(const fopAc_ac_c* ac) {
    const auto& zelda = *reinterpret_cast<const e_hzelda_class*>(ac);
    return zelda.mDemoMode == 0;
}

void defeatPuppetZelda(fopAc_ac_c* ac) {
    auto& zelda = *reinterpret_cast<e_hzelda_class*>(ac);
    ac->health = 0;
    zelda.field_0x6e4 = -1;
    zelda.mAction = kZeldaDamage;
    zelda.mMode = 10;
    zelda.mDemoMode = 1;
    Z2GetAudioMgr()->bgmStop(30, 0);
    zelda.mSound.startCreatureVoice(Z2SE_EN_HZE_V_DAMAGE_LAST, -1);
}

constexpr int kGanondorfDown = 21;
constexpr int kGanondorfEnding = 22;
constexpr int kGanondorfFinisherDemo = 60;

bool ganondorfDefeated(const fopAc_ac_c* ac) {
    return static_cast<const b_gnd_class*>(ac)->mActionMode == kGanondorfEnding;
}

bool ganondorfReady(const fopAc_ac_c* ac) {
    const auto& boss = *static_cast<const b_gnd_class*>(ac);
    return boss.mDemoCamMode == 0 && !boss.mDrawHorse && boss.mActionMode >= 10 &&
           fopAcM_IsExecuting(boss.mMantChildID);
}

bool prepareGanondorf(fopAc_ac_c* ac) {
    auto& boss = *static_cast<b_gnd_class*>(ac);
    if (boss.mActionMode == kGanondorfDown && boss.mMoveMode == 2) {
        return true;
    }
    if (boss.mActionMode != kGanondorfDown || boss.mMoveMode == 3) {
        boss.field_0x1e0c = 2; // The third knockdown sets up the final thrust.
        boss.field_0xc7c = 0;
        boss.mActionMode = kGanondorfDown;
        boss.mMoveMode = 0;
        boss.mDamageInvulnerabilityTimer = 10;
    }
    return false;
}

void defeatGanondorf(fopAc_ac_c* ac) {
    auto& boss = *static_cast<b_gnd_class*>(ac);
    boss.mDemoCamMode = kGanondorfFinisherDemo;
    boss.mActionMode = kGanondorfEnding;
    boss.mMoveMode = 0;
    boss.offDownFlg();
    Z2GetAudioMgr()->bgmStop(30, 0);
}

bool ookDefeated(const fopAc_ac_c* ac) {
    return reinterpret_cast<const e_mk_class*>(ac)->action == e_mk_class::ACT_E_DEMO;
}

bool ookReady(const fopAc_ac_c* ac) {
    const auto& boss = *reinterpret_cast<const e_mk_class*>(ac);
    return boss.demoMode == e_mk_class::DEMO_MODE_NONE &&
           boss.action < e_mk_class::ACT_S_DEMO;
}

void* clearOokPlants(void* process, void* data) {
    if (!fopAcM_IsActor(process)) {
        return nullptr;
    }
    auto* actor = static_cast<fopAc_ac_c*>(process);
    if (fopAcM_GetRoomNo(actor) != fopAcM_GetRoomNo(static_cast<fopAc_ac_c*>(data))) {
        return nullptr;
    }
    if (fopAcM_GetName(actor) == fpcNm_E_DB_e) {
        actor->health = 1000;
    } else if (fopAcM_GetName(actor) == fpcNm_E_DB_LEAF_e) {
        fopAcM_delete(actor);
    }
    return nullptr;
}

void defeatOok(fopAc_ac_c* ac) {
    auto& boss = *reinterpret_cast<e_mk_class*>(ac);
    fpcM_Search(clearOokPlants, ac);
    ac->health = 0;
    boss.action = e_mk_class::ACT_E_DEMO;
    boss.mode = 0;
    boss.invulnerabilityTimer = 20000;
    boss.sound.startCreatureVoice(Z2SE_EN_MK_V_DEAD, -1);
    Z2GetAudioMgr()->subBgmStop();
}

bool phantomZantDefeated(const fopAc_ac_c* ac) {
    return static_cast<const daE_PZ_c*>(ac)->mActionMode == daE_PZ_c::ACTION_DEAD_e;
}

bool phantomZantReady(const fopAc_ac_c* ac) {
    const auto& boss = *static_cast<const daE_PZ_c*>(ac);
    return boss.mActionMode == daE_PZ_c::ACTION_WAIT_e ||
           boss.mActionMode == daE_PZ_c::ACTION_ATTACK_e ||
           boss.mActionMode == daE_PZ_c::ACTION_DAMAGE_e;
}

void defeatPhantomZant(fopAc_ac_c* ac) {
    auto& boss = *static_cast<daE_PZ_c*>(ac);
    boss.health = 0;
    for (const auto id : boss.field_0x7dc) {
        if (id != 0) {
            if (auto* phantom = fopAcM_SearchByID(id)) {
                fopAcM_OnStatus(phantom, fopAcStts_UNK_0x4000_e);
            }
        }
    }
    Z2GetAudioMgr()->subBgmStop();
    boss.setActionMode(daE_PZ_c::ACTION_DEAD_e, boss.arg0 == 0 ? 0 : 10);
}

constexpr int kKingBulblinEnd = 7;

bool kingBulblinDefeated(const fopAc_ac_c* ac) {
    return reinterpret_cast<const e_rdb_class*>(ac)->mAction == kKingBulblinEnd;
}

bool kingBulblinReady(const fopAc_ac_c* ac) {
    const auto& boss = *reinterpret_cast<const e_rdb_class*>(ac);
    return boss.mDemoMode == 0 && boss.mAction > 0 && boss.mAction < kKingBulblinEnd;
}

void defeatKingBulblin(fopAc_ac_c* ac) {
    auto& boss = *reinterpret_cast<e_rdb_class*>(ac);
    ac->health = 0;
    boss.mAction = kKingBulblinEnd;
    boss.mMode = std::strcmp(dComIfGp_getStartStageName(), "D_MN09") == 0 ? 20 : 0;
    boss.field_0xfcd = 0;
    boss.field_0x6c0 = 1000;
    boss.mSound.startCreatureVoice(Z2SE_EN_RDB_V_ED_DEMO_DOWN, -1);
    Z2GetAudioMgr()->subBgmStop();
}

constexpr int kPoeExtracted = 6;

bool poeExtracted(const fopAc_ac_c* ac) {
    const auto& poe = *static_cast<const daE_HP_c*>(ac);
    return poe.mAction == kPoeExtracted && poe.mItemCheckHandled;
}

bool poeCanShareReward(const fopAc_ac_c*) {
    return sync::enabled();
}

void releasePoe(fopAc_ac_c* ac) {
    auto& poe = *static_cast<daE_HP_c*>(ac);
    poe.health = 0;
    poe.offDownFlg();
    poe.setBck(7, 0, 3.0f, 1.0f);
    poe.mSound1.startCreatureVoice(Z2SE_EN_HP_V_DEAD, -1);
    poe.field_0x71c = 0;
    // The extracting player owns the item check and sends the reward through item sync.
    poe.mItemCheckHandled = true;
    poe.setActionMode(kPoeExtracted, 1);
}

constexpr int kArmogohmaClear = 40;

void* findArmogohmaCore(void* process, void* data) {
    if (!fopAcM_IsActor(process)) {
        return nullptr;
    }
    auto* actor = static_cast<fopAc_ac_c*>(process);
    auto* parent = static_cast<fopAc_ac_c*>(data);
    return fopAcM_GetName(actor) == fpcNm_E_GM_e && fopAcM_GetParam(actor) == 3 &&
           fopAcM_GetRoomNo(actor) == fopAcM_GetRoomNo(parent) &&
           fopAcM_IsExecuting(fopAcM_GetID(actor)) ? process : nullptr;
}

bool armogohmaDefeated(const fopAc_ac_c* ac) {
    const auto& boss = *static_cast<const b_gm_class*>(ac);
    return boss.mIsDisappear && boss.mDemoMode >= kArmogohmaClear;
}

bool armogohmaReady(const fopAc_ac_c* ac) {
    const auto& boss = *static_cast<const b_gm_class*>(ac);
    return boss.mDemoMode == 0 && boss.mAction != 0;
}

void* clearArmogohmaBrood(void* process, void* data) {
    if (!fopAcM_IsActor(process)) {
        return nullptr;
    }
    auto* actor = static_cast<fopAc_ac_c*>(process);
    const auto* parent = static_cast<fopAc_ac_c*>(data);
    if (fopAcM_GetName(actor) == fpcNm_E_GM_e && fopAcM_GetParam(actor) == 0 &&
        fopAcM_GetRoomNo(actor) == fopAcM_GetRoomNo(parent))
    {
        static_cast<daE_GM_c*>(actor)->InstantKill();
    }
    return nullptr;
}

bool prepareArmogohma(fopAc_ac_c* ac) {
    auto& boss = *static_cast<b_gm_class*>(ac);
    if (boss.mIsDisappear) {
        return fpcM_Search(findArmogohmaCore, ac) != nullptr;
    }
    constexpr int kDrop = 11;
    if (boss.mAction == kDrop && (boss.mMode == 2 || boss.mMode == 3)) {
        boss.mHitCount = 3;
        boss.mMode = 20;
        boss.mDemoMode = 30;
        dComIfGs_onOneZoneSwitch(4, -1);
        fpcM_Search(clearArmogohmaBrood, ac);
        Z2GetAudioMgr()->bgmStop(30, 0);
    }
    return false;
}

void defeatArmogohma(fopAc_ac_c* ac) {
    if (auto* core = static_cast<daE_GM_c*>(fpcM_Search(findArmogohmaCore, ac))) {
        // The parent detects the core result and starts the native reward scene.
        core->field_0xa74 = 1;
        core->setAction(&daE_GM_c::core_damage);
    }
}

bool staltroopDefeated(const fopAc_ac_c* ac) {
    const auto& enemy = *static_cast<const daE_ZS_c*>(ac);
    return enemy.mAction == daE_ZS_c::ACT_DAMAGE && enemy.mMode >= 2 &&
           enemy.health == 0;
}

bool staltroopCanDie(const fopAc_ac_c* ac) {
    const auto& enemy = *static_cast<const daE_ZS_c*>(ac);
    return enemy.mAction == daE_ZS_c::ACT_WAIT || enemy.mAction == daE_ZS_c::ACT_DAMAGE;
}

void defeatStaltroop(fopAc_ac_c* ac) {
    auto& enemy = *static_cast<daE_ZS_c*>(ac);
    enemy.health = 0;
    enemy.field_0x560 = 0;
    enemy.mCyl.OffTgSetBit();
    enemy.mCyl.OffCoSetBit();
    enemy.setActionMode(daE_ZS_c::ACT_DAMAGE, 2);
}

constexpr int kPuppetMasterDamage = 4;
constexpr int kPuppetMasterDeath = 5;

bool puppetMasterDefeated(const fopAc_ac_c* ac) {
    return static_cast<const daE_PM_c*>(ac)->mAction == kPuppetMasterDeath;
}

bool puppetMasterCanDie(const fopAc_ac_c* ac) {
    const auto& master = *static_cast<const daE_PM_c*>(ac);
    return master.mStage == 4 && master.mDemoMode == 0 &&
           master.mAction >= 1 && master.mAction <= kPuppetMasterDamage;
}

void defeatPuppetMaster(fopAc_ac_c* ac) {
    auto& master = *static_cast<daE_PM_c*>(ac);
    master.mBossHealth = 0;
    master.mAction = kPuppetMasterDamage;
    master.mMode = 0;
}

bool arbiterPoeExtracted(const fopAc_ac_c* ac) {
    const auto& poe = *reinterpret_cast<const e_po_class*>(ac);
    // The reward check was completed before the continuation cutscene.
    return poe.mActionID == ACT_DEAD && poe.field_0x75B != 0;
}

bool arbiterPoeCanDie(const fopAc_ac_c* ac) {
    const auto& poe = *reinterpret_cast<const e_po_class*>(ac);
    return sync::enabled() && poe.mActionID != ACT_HOLL_DEMO && poe.mActionID != ACT_OPENING;
}

void defeatArbiterPoe(fopAc_ac_c* ac) {
    auto& poe = *reinterpret_cast<e_po_class*>(ac);
    // Pretend the local pull animation is ending. The source owns the soul check.
    poe.enemy.health = 0;
    poe.enemy.offDownFlg();
    poe.mActionID = ACT_DEAD;
    poe.mType = 2;
    poe.field_0x74A[2] = 2;
    poe.field_0x75B = 1;
    // The native continuation writes the soul switch. Record that the reward belongs
    // to the teammate even when their item packet arrives after that switch write.
    sync::detail::onRemoteSwitch(sync::detail::currentSaveTblNo(), poe.BitSW,
                                 fopAcM_GetRoomNo(ac), false);
}

constexpr int kTwilitKargorokDamage = 10;

bool twilitKargorokDefeated(const fopAc_ac_c* ac) {
    const auto& enemy = *reinterpret_cast<const e_yr_class*>(ac);
    return enemy.field_0x669 == kTwilitKargorokDamage && ac->health <= 0;
}

bool twilitKargorokCanDie(const fopAc_ac_c* ac) {
    const auto& enemy = *reinterpret_cast<const e_yr_class*>(ac);
    return enemy.field_0x669 == 3 || enemy.field_0x669 == 9 ||
           enemy.field_0x669 == kTwilitKargorokDamage;
}

void defeatTwilitKargorok(fopAc_ac_c* ac) {
    auto& enemy = *reinterpret_cast<e_yr_class*>(ac);
    ac->health = 0;
    enemy.field_0x669 = kTwilitKargorokDamage;
    enemy.field_0x67d = 0;
    enemy.field_0x6d2 = ac->shape_angle.y;
    enemy.field_0x6d8 = 0.0f;
    enemy.field_0x6dc = 0.0f;
    enemy.mSound.startCreatureVoice(Z2SE_EN_YR_V_DEATH, -1);
}

constexpr InPlaceDeath kDefeats[] = {
    {fpcNm_B_GM_e, armogohmaDefeated, defeatArmogohma, armogohmaReady, false, nullptr, prepareArmogohma},
    {fpcNm_E_ZS_e, staltroopDefeated, defeatStaltroop, nullptr, false, staltroopCanDie},
    {fpcNm_E_PM_e, puppetMasterDefeated, defeatPuppetMaster, nullptr, false, puppetMasterCanDie},
    {fpcNm_E_PO_e, arbiterPoeExtracted, defeatArbiterPoe, nullptr, false, arbiterPoeCanDie},
    {fpcNm_E_YR_e, twilitKargorokDefeated, defeatTwilitKargorok, nullptr, false, twilitKargorokCanDie},
    {fpcNm_B_GND_e, ganondorfDefeated, defeatGanondorf, ganondorfReady, false, nullptr, prepareGanondorf},
    {fpcNm_E_MK_e, ookDefeated, defeatOok, ookReady, false},
    {fpcNm_E_PZ_e, phantomZantDefeated, defeatPhantomZant, phantomZantReady, false},
    {fpcNm_E_RDB_e, kingBulblinDefeated, defeatKingBulblin, kingBulblinReady, false},
    {fpcNm_E_HP_e, poeExtracted, releasePoe, nullptr, false, poeCanShareReward},
    {fpcNm_B_DS_e, stallordDefeated, defeatStallord, stallordReady, false, nullptr, prepareStallord},
    {fpcNm_B_MGN_e, beastGanonDefeated, defeatBeastGanon, beastGanonReady, false},
    {fpcNm_B_OB_e, morpheelDefeated, defeatMorpheel, morpheelReady, false, nullptr, prepareMorpheel},
    {fpcNm_B_TN_e, darknutDefeated, defeatDarknut, nullptr, false, nullptr, prepareDarknut},
    {fpcNm_B_ZANT_e, zantDefeated, defeatZant, zantReady, false},
    {fpcNm_E_HZELDA_e, puppetZeldaDefeated, defeatPuppetZelda, puppetZeldaReady, false},
};

}  // namespace

const InPlaceDeath* findEncounterDefeat(s16 profile, u32 params) {
    if (profile == fpcNm_B_DS_e) {
        const auto type = params & 0xFF;
        if (type != daB_DS_c::TYPE_BATTLE_1 && type != daB_DS_c::TYPE_BATTLE_2 && type != 0xFF) {
            return nullptr;
        }
    }
    if (profile == fpcNm_E_PZ_e && (params & 0xFF) > 1) {
        return nullptr;
    }
    // The other Poe placement is the dungeon entrance cutscene controller.
    if (profile == fpcNm_E_PO_e && (params & 0xFF) > 3) {
        return nullptr;
    }
    // Skull Kid uses the native combat ending only in the final placement.
    if (profile == fpcNm_E_PM_e && (params & 0xFF) != 4 && (params & 0xFF) != 9) {
        return nullptr;
    }
    for (const auto& defeat : kDefeats) {
        if (defeat.procName == profile) {
            return &defeat;
        }
    }
    return nullptr;
}

}  // namespace twili::enemy_sync
