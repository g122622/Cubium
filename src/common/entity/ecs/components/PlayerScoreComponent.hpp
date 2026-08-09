#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 玩家分数组件
 *
 * 承载 Player::m_score。对齐基岩版 PlayerScoreComponent
 * （mc/entity/components/PlayerScoreComponent.h）。
 *
 * 仅 Player attach，普通实体不持有（避免污染所有实体内存）。
 *
 * 字段语义：
 * - m_score：玩家分数（击杀/挖矿等积累），同步真相源（DATA_PLAYER_SCORE_PARAM 退为镜像）。
 *   setScore/increaseScore 同时写组件 + DataParameter，NBT 反序列化走 setter。
 */
struct PlayerScoreComponent {
    i32 m_score{0};
};

} // namespace mc::ecs
