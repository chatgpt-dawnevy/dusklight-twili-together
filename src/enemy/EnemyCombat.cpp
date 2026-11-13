#include "enemy/EnemyCombat.hpp"

#include "core/SaveGate.hpp"
#include "core/Session.hpp"
#include "enemy/EnemySync.hpp"
#include "sync/WorldSync.hpp"

#include "m_Do/m_Do_ext.h"
#include "d/d_kankyo.h"
#include "d/actor/d_a_e_fm.h"
#include "d/actor/d_a_e_dt.h"
#include "d/actor/d_a_e_oc.h"
#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace twili::enemy_combat {
namespace {
using Clock = std::chrono::steady_clock;
using Json = nlohmann::json;
using enemy_sync::SpawnKey;
using enemy_sync::detail::Location;

constexpr int kVersion = 1;
constexpr size_t kBatchSize = 16;
constexpr size_t kMaxSnapshots = 128;
constexpr auto kInterval = std::chrono::milliseconds(100);
constexpr int kFyrusDown = 9;
constexpr int kToadBombDamage = 8;
constexpr int kToadDeath = 9;
constexpr auto kTimeout = std::chrono::milliseconds(750);
constexpr auto kToadStunTimeout = std::chrono::seconds(8);

struct Snapshot {
    SpawnKey key{};
    cXyz position{};
    s16 yaw = 0;
    int action = 0;
    int phase = 0;
    int animation = 0;
    int playMode = 0;
    float frame = 0;
    float rate = 0;
    bool attacking = false;
    Clock::time_point at{};
    int knockdowns = 0;
    bool bombStun = false;
    bool applied = false;
};

std::unordered_map<fpc_ProcID, Snapshot> s_local;
std::vector<Snapshot> s_remote;
std::unordered_set<fpc_ProcID> s_replicas;
Location s_location;
int s_room = -1;
uint32_t s_authority = 0;
Clock::time_point s_lastSend{};

// daE_OC_c actions with self-contained combat behavior. Talk, scripted encounters
// and deaths stay on their native paths.
bool supportedAction(int action) {
    switch (action) {
    case 0:  // wait
    case 1:  // walk
    case 3:  // find
    case 4:  // attack
    case 5:  // damage
    case 6:  // big damage
    case 7:  // watch
    case 8:  // sound watch
    case 15: // find stay
    case 16: // move out
        return true;
    default:
        return false;
    }
}

uint32_t authority(const Session& session, int room) {
    if (!session.joined() || !session.roomState().syncNPCs ||
        !session.roomState().syncEnemyCombat || !session.memberMaySync(session.selfClientId()) || !isSaveLoaded() ||
        dComIfGp_isEnableNextStage() || room < 0)
    {
        return 0;
    }
    uint32_t first = session.selfClientId();
    bool hasPeer = false;
    for (const auto& [id, client] : session.clients()) {
        if (client.roomNo == room && session.clientIsInCurrentLayer(client) &&
            client.protocolVersion == Session::kProtocolVersion && session.isTeammate(client) &&
            session.memberMaySync(id))
        {
            first = std::min(first, id);
            hasPeer = true;
        }
    }
    return hasPeer ? first : 0;
}

bool eligible(const daE_OC_c& actor) {
    if (s_authority == 0 || dComIfGp_isEnableNextStage() || dComIfGp_event_runCheck()) return false;
    const auto* player = daPy_getPlayerActorClass();
    return s_location.matches(enemy_sync::detail::currentLocation()) &&
           s_room == dStage_roomControl_c::getStayNo() && fopAcM_GetRoomNo(&actor) == s_room &&
           actor.health > 0 && supportedAction(actor.mActionMode) &&
           !enemy_sync::isDying(fopAcM_GetID(&actor)) &&
           player != nullptr && player->getGrabActorID() != fopAcM_GetID(&actor) &&
           !player->checkWolfEnemyBiteAllOwn(&actor);
}

bool number(const Json& j, const char* field, float lo, float hi, float& out) {
    const auto it = j.find(field);
    if (it == j.end() || !it->is_number()) return false;
    const double value = it->get<double>();
    if (!std::isfinite(value) || value < lo || value > hi) return false;
    out = static_cast<float>(value);
    return true;
}

bool parse(const Json& j, Snapshot& s) {
    using enemy_sync::detail::intField;
    int64_t yaw, action, phase, animation, playMode;
    if (!enemy_sync::detail::parseKey(j, s.key) || s.key.roomNo != s_room) return false;
    if (s.key.procName == fpcNm_E_FM_e) {
        int64_t knockdowns;
        if (!intField(j, "knockdowns", 1, 127, knockdowns)) return false;
        s.knockdowns = static_cast<int>(knockdowns);
        return true;
    }
    if (s.key.procName == fpcNm_E_DT_e) {
        const auto stun = j.find("bombStun");
        if (stun == j.end() || !stun->is_boolean()) return false;
        s.bombStun = stun->get<bool>();
        return true;
    }
    if (s.key.procName != fpcNm_E_OC_e || !intField(j, "yaw", -32768, 32767, yaw) ||
        !intField(j, "action", 0, 16, action) || !supportedAction(static_cast<int>(action)) ||
        !intField(j, "phase", 0, 64, phase) || !intField(j, "animation", 3, 30, animation) ||
        !intField(j, "playMode", 0, 4, playMode) ||
        !number(j, "x", -1e6f, 1e6f, s.position.x) || !number(j, "y", -1e6f, 1e6f, s.position.y) ||
        !number(j, "z", -1e6f, 1e6f, s.position.z) || !number(j, "frame", 0, 1000, s.frame) ||
        !number(j, "rate", -4, 4, s.rate))
    {
        return false;
    }
    const auto attack = j.find("attacking");
    if (attack == j.end() || !attack->is_boolean()) return false;
    s.yaw = static_cast<s16>(yaw);
    s.action = static_cast<int>(action);
    s.phase = static_cast<int>(phase);
    s.animation = static_cast<int>(animation);
    s.playMode = static_cast<int>(playMode);
    s.attacking = s.action == 4 && (s.animation == 5 || s.animation == 6) && attack->get<bool>();
    return true;
}

void sendBatch(Session& session, Json& states) {
    if (states.empty()) return;
    Json packet{{"type", "ENEMY_COMBAT"}, {"v", kVersion}, {"quiet", true},
                {"stageName", s_location.stage.data()}, {"layerNo", s_location.layer},
                {"roomNo", s_room}, {"states", std::move(states)}};
    sync::stampPacket(packet, false);
    session.send(packet, net::Delivery::Droppable);
    states = Json::array();
}
}  // namespace

void reset() {
    s_local.clear();
    s_remote.clear();
    s_authority = 0;
    s_room = -1;
    s_location = {};
    s_lastSend = {};
}

void onActorDeleted(const fopAc_ac_c* actor) {
    s_local.erase(fopAcM_GetID(actor));
    s_replicas.erase(fopAcM_GetID(actor));
}

void tick() {
    auto& session = Session::instance();
    const auto location = enemy_sync::detail::currentLocation();
    const int room = dStage_roomControl_c::getStayNo();
    const uint32_t owner = authority(session, room);
    if (owner != s_authority || room != s_room || !location.matches(s_location)) {
        s_local.clear();
        s_remote.clear();
        s_lastSend = {};
        s_authority = owner;
        s_room = room;
        s_location = location;
    }
    const auto now = Clock::now();
    std::erase_if(s_remote, [&](const Snapshot& s) { return now - s.at >= (s.bombStun ? kToadStunTimeout : kTimeout); });
    if (owner == 0 || owner != session.selfClientId() || now - s_lastSend < kInterval ||
        !enemy_sync::detail::canSendHere(location.stage.data(), location.layer))
    {
        return;
    }
    s_lastSend = now;
    Json states = Json::array();
    for (const auto& [id, state] : s_local) {
        if (now - state.at >= kTimeout || enemy_sync::isDying(id)) continue;
        Json item = enemy_sync::detail::keyJson(state.key);
        if (state.key.procName == fpcNm_E_FM_e) {
            item["knockdowns"] = state.knockdowns;
        } else if (state.key.procName == fpcNm_E_DT_e) {
            item["bombStun"] = state.bombStun;
        } else {
            item.update(Json{{"x", state.position.x}, {"y", state.position.y}, {"z", state.position.z},
                         {"yaw", state.yaw}, {"action", state.action}, {"phase", state.phase},
                         {"animation", state.animation}, {"playMode", state.playMode}, {"frame", state.frame},
                         {"rate", state.rate}, {"attacking", state.attacking}});
        }
        states.push_back(std::move(item));
        if (states.size() == kBatchSize) sendBatch(session, states);
    }
    sendBatch(session, states);
    s_local.clear();
}

bool handlePacket(const std::string& type, const Json& packet) {
    if (type != "ENEMY_COMBAT") return false;
    const auto& session = Session::instance();
    if (s_authority == 0 || s_authority == session.selfClientId() ||
        authority(session, dStage_roomControl_c::getStayNo()) != s_authority ||
        packet.value("clientId", 0u) != s_authority || packet.value("roomNo", -1) != s_room ||
        !s_location.matches(enemy_sync::detail::currentLocation()) ||
        !enemy_sync::detail::acceptFromTeammate(packet, kVersion))
    {
        return true;
    }
    const auto states = packet.find("states");
    if (states == packet.end() || !states->is_array() || states->size() > kBatchSize) return true;
    for (const auto& item : *states) {
        Snapshot state;
        if (!parse(item, state)) continue;
        state.at = Clock::now();
        const auto existing = std::find_if(s_remote.begin(), s_remote.end(),
            [&](const Snapshot& s) { return enemy_sync::sameKey(s.key, state.key); });
        if (existing != s_remote.end()) {
            if (!state.bombStun && existing->bombStun && !existing->applied) continue;
            state.applied = state.bombStun && existing->applied;
            *existing = state;
        } else if (s_remote.size() < kMaxSnapshots) {
            s_remote.push_back(state);
        }
    }
    return true;
}

void capture(daE_OC_c& actor) {
    if (s_authority != Session::instance().selfClientId() || !eligible(actor)) return;
    const auto* key = enemy_sync::trackedKey(fopAcM_GetID(&actor));
    if (key == nullptr || s_local.size() >= kMaxSnapshots) return;
    // E_ocb resources 3 through 30 are the animations used by setBck.
    for (int animation = 3; animation <= 30; ++animation) {
        if (!actor.checkBck(animation)) continue;
        const bool attack = actor.mSphs_at[0].ChkAtSet() || actor.mSphs_at[1].ChkAtSet();
        s_local[fopAcM_GetID(&actor)] = {*key, actor.current.pos, actor.shape_angle.y,
            actor.mActionMode, actor.mOcState, animation, actor.mpMorf->getPlayMode(),
            actor.mpMorf->getFrame(), actor.mpMorf->getPlaySpeed(), attack, Clock::now()};
        return;
    }
}

void updateDekuToad(daE_DT_c& actor) {
    const auto id = fopAcM_GetID(&actor);
    const auto* key = enemy_sync::trackedKey(id);
    if (key == nullptr || s_authority == 0 ||
        !s_location.matches(enemy_sync::detail::currentLocation()) ||
        key->roomNo != s_room || dStage_roomControl_c::getStayNo() != s_room ||
        dComIfGp_isEnableNextStage() || enemy_sync::isDying(id) ||
        actor.mAction < 0 || actor.mAction >= kToadDeath)
    {
        return;
    }
    if (s_authority == Session::instance().selfClientId()) {
        Snapshot snapshot;
        snapshot.key = *key;
        snapshot.bombStun = actor.mAction == kToadBombDamage;
        snapshot.at = Clock::now();
        s_local[id] = snapshot;
        return;
    }
    if (actor.mLinkPressed || dComIfGp_event_runCheck()) return;
    const auto snapshot = std::find_if(s_remote.begin(), s_remote.end(),
        [&](const Snapshot& state) { return enemy_sync::sameKey(state.key, *key); });
    if (snapshot == s_remote.end() || !snapshot->bombStun || snapshot->applied ||
        Clock::now() - snapshot->at >= kToadStunTimeout)
    {
        return;
    }
    snapshot->applied = true;
    if (actor.mAction != kToadBombDamage) {
        actor.setActionMode(kToadBombDamage, 0);
    }
}

void updateFyrus(e_fm_class& actor) {
    const auto id = fopAcM_GetID(&actor);
    const auto* key = enemy_sync::trackedKey(id);
    if (key == nullptr || s_authority == 0 ||
        !s_location.matches(enemy_sync::detail::currentLocation()) ||
        key->roomNo != s_room || dStage_roomControl_c::getStayNo() != s_room ||
        dComIfGp_isEnableNextStage() || enemy_sync::isDying(id))
    {
        return;
    }
    if (s_authority == Session::instance().selfClientId()) {
        if (actor.mAction == kFyrusDown && actor.mDownCnt > 0) {
            Snapshot snapshot;
            snapshot.key = *key;
            snapshot.knockdowns = actor.mDownCnt;
            snapshot.at = Clock::now();
            s_local[id] = snapshot;
        }
        return;
    }

    // Do not interrupt an intro, camera sequence, or a player holding either chain.
    if (actor.mAction < 0 || actor.mAction > kFyrusDown || actor.mDemoCamMode != 0 ||
        dComIfGp_event_runCheck() || actor.mChain[2].field_0x617c || actor.mChain[3].field_0x617c)
    {
        return;
    }
    const auto snapshot = std::find_if(s_remote.begin(), s_remote.end(),
        [&](const Snapshot& state) { return enemy_sync::sameKey(state.key, *key); });
    if (snapshot == s_remote.end() || snapshot->knockdowns <= actor.mDownCnt ||
        Clock::now() - snapshot->at >= kTimeout)
    {
        return;
    }

    actor.mDownCnt = static_cast<s8>(snapshot->knockdowns);
    if (actor.mAction != kFyrusDown) {
        // The native down action initializes the animation and vulnerable period.
        actor.health = 50;
        actor.mAction = kFyrusDown;
        actor.mMode = 0;
        // A remotely started knockdown does not own the local camera.
        actor.field_0x1b078 = 1;
    }
}

bool executeReplica(daE_OC_c& actor) {
    const auto id = fopAcM_GetID(&actor);
    const auto* key = enemy_sync::trackedKey(id);
    const auto resumeLocal = [&] {
        if (s_replicas.erase(id) != 0 && actor.health > 0 && supportedAction(actor.mActionMode)) {
            // Start native AI from a valid entry point after the last snapshot.
            actor.setActionMode(0, 0);  // E_OC_ACTION_WAIT
            actor.speedF = 0.0f;
            for (auto& sphere : actor.mSphs_at) sphere.OffAtSetBit();
        }
        return false;
    };
    if (key == nullptr || s_authority == Session::instance().selfClientId() || !eligible(actor)) {
        return resumeLocal();
    }
    const auto it = std::find_if(s_remote.begin(), s_remote.end(),
        [&](const Snapshot& s) { return enemy_sync::sameKey(s.key, *key); });
    if (it == s_remote.end() || Clock::now() - it->at >= kTimeout) return resumeLocal();
    const Snapshot& state = *it;

    // Process contact with the local player through the native damage routine.
    // A fatal hit still enters the normal death path and publishes ENEMY_DEFEATED.
    if (actor.field_0x6cc > 0) --actor.field_0x6cc;
    actor.damage_check();
    if (actor.health <= 0 || !supportedAction(actor.mActionMode)) {
        s_replicas.erase(id);
        return false;
    }
    s_replicas.insert(id);

    actor.old.pos = actor.current.pos;
    actor.current.pos += (state.position - actor.current.pos) * 0.5f;
    actor.shape_angle.y = state.yaw;
    actor.current.angle.y = state.yaw;
    actor.mActionMode = state.action;
    actor.mOcState = state.phase;
    if (!actor.checkBck(state.animation) || actor.mpMorf->getPlayMode() != state.playMode) {
        actor.setBck(state.animation, static_cast<u8>(state.playMode), 0.0f, state.rate);
    }
    const float age = std::chrono::duration<float>(Clock::now() - state.at).count();
    const float frame = std::clamp(state.frame + std::min(age, 0.1f) * 30.0f * state.rate,
                                  0.0f, actor.mpMorf->getEndFrame());
    actor.mpMorf->setPlaySpeed(state.rate);
    actor.mpMorf->setFrameF(frame);
    for (auto& sphere : actor.mSphs_at) {
        if (state.attacking && age <= 0.2f && frame >= 14.0f && frame <= 22.0f) {
            sphere.OnAtSetBit();
        } else {
            sphere.OffAtSetBit();
        }
    }
    actor.mtx_set();
    actor.cc_set();
    return true;
}
}  // namespace twili::enemy_combat
