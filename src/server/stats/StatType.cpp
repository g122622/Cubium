#include "server/stats/StatType.hpp"

namespace mc {
namespace server {
namespace stats {

std::optional<StatType> parseStatType(std::string_view prefix) noexcept {
    if (prefix == "mined") {
        return StatType::Mined;
    } else if (prefix == "crafted") {
        return StatType::Crafted;
    } else if (prefix == "used") {
        return StatType::Used;
    } else if (prefix == "broken") {
        return StatType::Broken;
    } else if (prefix == "picked_up") {
        return StatType::PickedUp;
    } else if (prefix == "dropped") {
        return StatType::Dropped;
    } else if (prefix == "killed") {
        return StatType::Killed;
    } else if (prefix == "killed_by") {
        return StatType::KilledBy;
    } else if (prefix == "custom") {
        return StatType::Custom;
    }
    return std::nullopt;
}

ResourceLocation buildStatLocation(StatType type, const ResourceLocation& id) {
    std::string prefix(getStatTypePrefix(type));
    std::string fullId = "minecraft." + prefix + ":" + id.toString();
    return ResourceLocation(fullId);
}

} // namespace stats
} // namespace server
} // namespace mc
