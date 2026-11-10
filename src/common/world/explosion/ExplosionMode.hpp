/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc {
namespace world {
namespace explosion {

/**
 * @brief 爆炸模式枚举
 *
 * 定义爆炸对方块的影响方式。
 */
enum class ExplosionMode : u8 {
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
    Destroy,

    /**
     * @brief 不破坏方块，只把爆炸"通知"给方块
     *
     * 对应 `Explosion.BlockInteraction.TRIGGER_BLOCK`。
     *
     * 与 `None` 的差别只在一点：`None` 完全跳过方块交互，而本模式仍会对每个受影响方块
     * 调用 `Block::onBlockExploded`，让重写过该钩子的方块自行反应——典型用途是引爆 TNT、
     * 触发苦力怕等"被爆炸激活"的行为。默认实现什么也不做，因此普通方块原地保留、不掉落。
     *
     * 风爆附魔（`minecraft:wind_burst`）用的是这个模式：数据包写 `block_interaction: "trigger"`。
     */
    TriggerBlock
};

} // namespace explosion
} // namespace world
} // namespace mc
