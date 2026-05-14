#pragma once

#include "../../core/Types.hpp"

namespace mc {

/**
 * @brief 物品使用动作枚举
 *
 * 定义物品在使用时的动作类型，用于客户端播放正确的动画。
 * 参考: net.minecraft.item.UseAction
 */
enum class UseAction : u8 {
    None = 0,           ///< 无动作（默认）
    Eat = 1,            ///< 进食动作（食物）
    Drink = 2,          ///< 饮用动作（药水、牛奶等）
    Block = 3,          ///< 格挡动作（盾牌）
    Bow = 4,            ///< 拉弓动作（弓）
    Spear = 5,          ///< 投掷动作（三叉戟）
    Crossbow = 6,       ///< 装填动作（弩）
    Spyglass = 7,       ///< 望远镜动作
    TotemOfUndying = 8, ///< 不死图腾动作
    Trident = Spear     ///< 别名，与Spear相同
};

/**
 * @brief 将使用动作转换为字符串
 * @param action 使用动作
 * @return 字符串表示
 */
[[nodiscard]] inline constexpr const char* toString(UseAction action) noexcept
{
    switch (action) {
        case UseAction::None:
            return "none";
        case UseAction::Eat:
            return "eat";
        case UseAction::Drink:
            return "drink";
        case UseAction::Block:
            return "block";
        case UseAction::Bow:
            return "bow";
        case UseAction::Spear:
            return "spear";
        case UseAction::Crossbow:
            return "crossbow";
        case UseAction::Spyglass:
            return "spyglass";
        case UseAction::TotemOfUndying:
            return "totem_of_undying";
        default:
            return "unknown";
    }
}

} // namespace mc
