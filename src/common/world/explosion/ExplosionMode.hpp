#pragma once

#include <cstdint>

namespace mc {
namespace world {
namespace explosion {

/**
 * @brief 爆炸模式枚举
 *
 * 定义爆炸对方块的影响方式。
 * 对应 Minecraft 1.16.5 的 Explosion.Mode 枚举。
 */
enum class ExplosionMode : std::uint8_t {
    /**
     * @brief 仅造成伤害和击退，不破坏方块
     *
     * 用于 mobGriefing=false 时的苦力怕爆炸，
     * 或不希望破坏地形的特殊爆炸效果。
     */
    None,

    /**
     * @brief 破坏方块但不掉落物品
     *
     * TNT 的默认爆炸模式。
     * 方块被移除但不生成掉落物。
     */
    Break,

    /**
     * @brief 破坏方块并掉落物品
     *
     * 苦力怕的默认爆炸模式（mobGriefing=true 时）。
     * 方块被移除并生成掉落物实体。
     */
    Destroy
};

} // namespace explosion
} // namespace world
} // namespace mc
