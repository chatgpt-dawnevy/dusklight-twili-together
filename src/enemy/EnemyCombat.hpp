#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

class daB_DR_c;
class daE_OC_c;
class daE_DT_c;
class e_fm_class;
class fopAc_ac_c;

namespace twili::enemy_combat {
void tick();
void reset();
void onActorDeleted(const fopAc_ac_c* actor);
bool handlePacket(const std::string& type, const nlohmann::json& packet);
bool executeReplica(daE_OC_c& actor);
void capture(daE_OC_c& actor);
void updateArgorok(daB_DR_c& actor);
void updateDekuToad(daE_DT_c& actor);
void updateFyrus(e_fm_class& actor);
}
