#include "ToolType.hpp"

namespace mc {
namespace item {
namespace tool {

const char* toString(ToolType type) {
    switch (type) {
        case ToolType::None:
            return "none";
        case ToolType::Pickaxe:
            return "pickaxe";
        case ToolType::Axe:
            return "axe";
        case ToolType::Shovel:
            return "shovel";
        case ToolType::Hoe:
            return "hoe";
        case ToolType::Sword:
            return "sword";
        case ToolType::Shears:
            return "shears";
        default:
            return "unknown";
    }
}

} // namespace tool
} // namespace item
} // namespace mc
