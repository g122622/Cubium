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
