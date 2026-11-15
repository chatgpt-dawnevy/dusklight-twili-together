#pragma once
#include <nlohmann/json_fwd.hpp>
namespace twili::shared_stock {
bool enabled();
bool ready();
void tick();
void reset();
void capture();
void rebase();
void receive(const nlohmann::json& packet);
}
