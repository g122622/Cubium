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
#include "common/item/component/FireworkExplosion.hpp"

#include <vector>

namespace mc {
namespace item {
namespace component {

/**
 * @brief 烟花火箭数据（1.21.11 net.minecraft.world.item.component.Fireworks）
 *
 * 承载烟花火箭物品的飞行时长与爆炸列表。飞行时长决定火箭总存活时间
 * （lifeTime = (1 + flight_duration) * 10 + rand），爆炸列表决定爆炸伤害与粒子。
 *
 * 字段默认值对齐 vanilla CODEC：flight_duration=0、explosions 为空。
 */
struct Fireworks {
    /// vanilla Fireworks.MAX_EXPLOSIONS：爆炸列表上限
    static constexpr i32 MAX_EXPLOSIONS = 256;

    i32 flightDuration = 0;
    std::vector<FireworkExplosion> explosions;

    [[nodiscard]] bool operator==(const Fireworks& other) const noexcept
    {
        return flightDuration == other.flightDuration && explosions == other.explosions;
    }

    [[nodiscard]] bool operator!=(const Fireworks& other) const noexcept
    {
        return !(*this == other);
    }
};

} // namespace component
} // namespace item
} // namespace mc
