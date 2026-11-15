#include "sync/SharedStock.hpp"
#include "sync/WorldSync.hpp"
#include "sync/WorldSyncState.hpp"
#include "core/Session.hpp"
#include "core/SaveGate.hpp"
#include "game/GameIdentity.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace twili::shared_stock {
namespace {
using Json = nlohmann::json;
using Counts = std::map<std::string, int>;
using Clock = std::chrono::steady_clock;
struct Change { uint64_t sequence; Counts delta; };
Counts baseline, authoritative, opening;
std::deque<Change> pending;
std::string epoch, scope;
uint64_t nextSequence = 1, revision = 0;
uint32_t generation = 0;
bool initialized = false, baselined = false;
Clock::time_point lastSend{};
const char* bombKey(int item) {
    switch (item) {
    case dItemNo_NORMAL_BOMB_e: return "bomb:normal";
    case dItemNo_WATER_BOMB_e: return "bomb:water";
    case dItemNo_POKE_BOMB_e: return "bomb:insect";
    default: return nullptr;
    }
}
int count(const Counts& values, const std::string& key) {
    const auto it = values.find(key);
    return it == values.end() ? 0 : it->second;
}
bool usable() {
    return enabled() && isSaveLoaded() && Session::instance().currentSaveTblNo() >= 0 &&
           !dComIfGp_isEnableNextStage();
}
Counts read() {
    Counts values{{"rupees", dComIfGs_getRupee()}, {"arrows", dComIfGs_getArrowNum()},
                  {"seeds", dComIfGs_getPachinkoNum()}, {"oil", dComIfGs_getOil()}};
    for (int bag = 0; bag < 3; ++bag) {
        const char* key = bombKey(dComIfGs_getItem(SLOT_15 + bag, false));
        if (key) values[key] += dComIfGs_getBombNum(bag);
    }
    for (int bottle = 0; bottle < 4; ++bottle) {
        const int item = dComIfGs_getItem(SLOT_11 + bottle, false);
        if (item != dItemNo_NONE_e) values["bottle:" + std::to_string(bottle) + ":" + std::to_string(item)] =
            std::max(1, int(dComIfGs_getBottleNum(bottle)));
    }
    auto* info = dComIfGs_getSaveInfo();
    for (int table = 0; table < dSv_save_c::STAGE_MAX; ++table) {
        const auto& bit = table == Session::instance().currentSaveTblNo() ?
            info->getMemory().getBit() : info->getSavedata().getSave(table).getBit();
        values["key:" + std::to_string(table)] = bit.mKeyNum;
    }
    return values;
}
void write(const Counts& values) {
    dComIfGs_setRupee(std::clamp(count(values, "rupees"), 0, int(dComIfGs_getRupeeMax())));
    dComIfGs_setArrowNum(std::clamp(count(values, "arrows"), 0, int(dComIfGs_getArrowMax())));
    dComIfGs_setPachinkoNum(std::clamp(count(values, "seeds"), 0, int(dComIfGs_getPachinkoMax())));
    dComIfGs_setOil(std::clamp(count(values, "oil"), 0, int(dComIfGs_getMaxOil())));
    Counts remaining = values;
    // Existing matching bags first; never create a bag the player does not own.
    for (int bag = 0; bag < 3; ++bag) {
        const int item = dComIfGs_getItem(SLOT_15 + bag, false);
        if (const char* key = bombKey(item)) {
            const int amount = std::clamp(count(remaining, key), 0, int(dComIfGs_getBombMax(item)));
            dComIfGs_setBombNum(bag, amount);
            remaining[key] -= amount;
        }
    }
    for (int bag = 0; bag < 3; ++bag) {
        const int old = dComIfGs_getItem(SLOT_15 + bag, false);
        if (old == dItemNo_NONE_e || dComIfGs_getBombNum(bag) != 0) continue;
        for (const int item : {dItemNo_NORMAL_BOMB_e, dItemNo_WATER_BOMB_e, dItemNo_POKE_BOMB_e}) {
            const char* key = bombKey(item);
            const int amount = std::clamp(count(remaining, key), 0, int(dComIfGs_getBombMax(item)));
            if (!amount) continue;
            dComIfGs_setItem(SLOT_15 + bag, item);
            dComIfGs_setBombNum(bag, amount);
            remaining[key] -= amount;
            break;
        }
    }
    for (int bottle = 0; bottle < 4; ++bottle) {
        const std::string prefix = "bottle:" + std::to_string(bottle) + ":";
        for (int item = 0; item < 255; ++item) {
            const int amount = count(values, prefix + std::to_string(item));
            if (amount <= 0) continue;
            dComIfGs_setItem(SLOT_11 + bottle, item);
            dComIfGs_setBottleNum(bottle, std::clamp(amount, 0, 255));
            break;
        }
    }
    dComIfGs_setLineUpItem();
    auto* info = dComIfGs_getSaveInfo();
    const int liveBefore = info->getMemory().getBit().mKeyNum;
    const auto bitsBefore = info->getMemory().getBit().mDungeonItem;
    for (int table = 0; table < dSv_save_c::STAGE_MAX; ++table) {
        auto& bit = table == Session::instance().currentSaveTblNo() ?
            info->getMemory().getBit() : info->getSavedata().getSave(table).getBit();
        bit.mKeyNum = std::clamp(count(values, "key:" + std::to_string(table)), 0, 255);
    }
    sync::detail::shiftDungeonItemBaseline(liveBefore, bitsBefore);
}
void apply() {
    Counts values = authoritative;
    for (const auto& change : pending) for (const auto& [key, delta] : change.delta) values[key] += delta;
    write(values);
    baseline = read();
    baselined = true;
}
}
bool enabled() { return sync::enabled() && Session::instance().roomState().sharedStock; }
bool ready() { return usable() && initialized; }
void reset() {
    baseline.clear(); authoritative.clear(); opening.clear(); pending.clear(); epoch.clear(); scope.clear();
    initialized = baselined = false; nextSequence = 1; revision = 0; generation = 0; lastSend = {};
}
void capture() {
    if (!usable() || !baselined) return;
    Counts now = read(), delta;
    for (const auto& [key, value] : baseline) if (!now.contains(key)) now[key] = 0;
    for (const auto& [key, value] : now) {
        const int difference = value - count(baseline, key);
        if (difference) delta[key] = difference;
    }
    baseline = std::move(now);
    if (!delta.empty()) pending.push_back({nextSequence++, std::move(delta)});
}
void rebase() { if (usable() && baselined) baseline = read(); }
void tick() {
    if (!enabled()) { reset(); return; }
    if (!usable()) return;
    const std::string currentScope = Session::instance().selfTeamId() + "|" + game_identity::current().key;
    if (scope != currentScope || generation != saveGeneration()) {
        reset(); scope = currentScope; generation = saveGeneration();
    }
    if (!sync::ownStateSettled()) return;
    if (!baselined) { baseline = read(); opening = baseline; baselined = true; }
    capture();
    const auto now = Clock::now();
    if (now - lastSend < std::chrono::milliseconds(100)) return;
    lastSend = now;
    if (!initialized) {
        Session::instance().send(Json{{"type", "INVENTORY_OPEN"}, {"counts", opening}});
    } else if (!pending.empty()) {
        const auto& change = pending.front();
        Session::instance().send(Json{{"type", "INVENTORY_DELTA"}, {"epoch", epoch},
            {"sequence", change.sequence}, {"delta", change.delta}});
    }
}
void receive(const Json& packet) {
    if (!usable() || !baselined || packet.value("teamId", std::string{}) != Session::instance().selfTeamId() ||
        packet.value("gameKey", std::string{}) != game_identity::current().key ||
        packet.value("layout", std::string{}) != sync::localLayout()) return;
    const auto items = packet.find("counts");
    if (items == packet.end() || !items->is_object() || items->size() > 128 ||
        !packet.contains("acknowledged") || !packet["acknowledged"].is_number_unsigned() ||
        !packet.contains("revision") || !packet["revision"].is_number_unsigned()) return;
    Counts values;
    for (const auto& [key, value] : items->items()) {
        if (!value.is_number_integer()) return;
        const auto number = value.get<int64_t>();
        if (number < 0 || number > 65535) return;
        values[key] = static_cast<int>(number);
    }
    capture();
    const std::string incomingEpoch = packet.value("epoch", std::string{});
    if (incomingEpoch.empty()) return;
    const uint64_t acknowledged = packet["acknowledged"].get<uint64_t>();
    const uint64_t incomingRevision = packet["revision"].get<uint64_t>();
    if (initialized && incomingEpoch == epoch && incomingRevision < revision) return;
    if (!initialized || incomingEpoch != epoch) {
        // Sequence numbers belong to the server's ledger lifetime. Reconnects
        // adopt its acknowledgement rather than replaying stale local saves.
        nextSequence = acknowledged + 1;
        for (auto& change : pending) change.sequence = nextSequence++;
    } else {
        while (!pending.empty() && pending.front().sequence <= acknowledged) pending.pop_front();
    }
    epoch = incomingEpoch; revision = incomingRevision; initialized = true;
    authoritative = std::move(values);
    apply();
}
}
