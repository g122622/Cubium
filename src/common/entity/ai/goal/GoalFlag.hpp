#pragma once

#include "../../../core/EnumSet.hpp"
#include "../../../core/Types.hpp"

namespace mc::entity::ai {

/**
 * @brief AI目标互斥标志
 *
 * 用于控制多个AI目标之间的互斥关系。
 * 如果两个目标共享相同的标志，则不能同时运行。
 *
 * 参考 MC 1.16.5 Goal.Flag
 */
enum class GoalFlag : u8 {
    Move,   // 移动
    Look,   // 视线
    Jump,   // 跳跃
    Target, // 目标选择
    Count   // 标志数量
};

/**
 * @brief 获取所有目标标志的集合
 * @return 包含所有标志的集合
 */
inline EnumSet<GoalFlag> allGoalFlags()
{
    return EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump, GoalFlag::Target};
}

} // namespace mc::entity::ai
