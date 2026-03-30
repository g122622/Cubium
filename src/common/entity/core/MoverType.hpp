#pragma once

#include "../core/Types.hpp"

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::u8;

/**
 * @brief 实体移动类型枚举
 *
 * 标识实体移动的来源，用于区分不同类型的移动事件。
 * 例如：活塞推动、玩家推动、自身移动等。
 *
 * 参考 MC 1.16.5 MoverType
 */
enum class MoverType : u8 {
    Self = 0,       // 自身移动（AI、行走等）
    Player = 1,     // 玩家推动
    Piston = 2,     // 活塞推动
    ShulkerBox = 3, // 潜影盒推动
    Shulker = 4     // 潜影贝推动
};

/**
 * @brief 获取移动类型名称（用于调试）
 * @param type 移动类型
 * @return 名称字符串
 */
inline const char* getMoverTypeName(MoverType type) {
    switch (type) {
        case MoverType::Self: return "self";
        case MoverType::Player: return "player";
        case MoverType::Piston: return "piston";
        case MoverType::ShulkerBox: return "shulker_box";
        case MoverType::Shulker: return "shulker";
    }
    return "unknown";
}

} // namespace mc::entity
