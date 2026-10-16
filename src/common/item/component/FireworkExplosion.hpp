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

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {
namespace item {
namespace component {

/**
 * @brief 烟花爆炸形状（1.21.11 FireworkExplosion.Shape）
 *
 * 数值 id 严格对齐 vanilla 枚举声明顺序：wire 上为 VarInt id，NBT 上为字符串名。
 */
enum class FireworkShape : u8 {
    SmallBall = 0,
    LargeBall = 1,
    Star = 2,
    Creeper = 3,
    Burst = 4,
};

/// 形状的 NBT 名（vanilla Shape.getSerializedName）
[[nodiscard]] std::string_view fireworkShapeName(FireworkShape shape) noexcept;

/// 由 NBT 名解析形状；未知名返回 nullopt
[[nodiscard]] std::optional<FireworkShape> fireworkShapeFromName(std::string_view name) noexcept;

/// 由 wire 数值 id 解析形状；越界回落 SmallBall（对齐 vanilla ByIdMap.OutOfBoundsStrategy.ZERO）
[[nodiscard]] FireworkShape fireworkShapeById(i32 id) noexcept;

/**
 * @brief 单次烟花爆炸（1.21.11 net.minecraft.world.item.component.FireworkExplosion）
 *
 * 描述烟花之星携带的一次爆炸：形状、主色列表、渐变色列表、是否拖尾（钻石）、
 * 是否闪烁（荧石粉）。颜色为染料烟花的 RGB 整数（DyeColor.getFireworkColor()）。
 *
 * 字段默认值对齐 vanilla CODEC：shape=SMALL_BALL、colors/fade_colors 为空、
 * has_trail/has_twinkle=false。
 */
struct FireworkExplosion {
    FireworkShape shape = FireworkShape::SmallBall;
    std::vector<i32> colors;     ///< 主色（RGB 整数列表）
    std::vector<i32> fadeColors; ///< 渐变色（RGB 整数列表）
    bool hasTrail = false;       ///< 是否拖尾（钻石）
    bool hasTwinkle = false;     ///< 是否闪烁（荧石粉）

    /**
     * @brief 默认爆炸（vanilla FireworkExplosion.DEFAULT）
     *
     * 用于 FireworkStarFadeRecipe 在物品未携带组件时的兜底初始值。
     */
    [[nodiscard]] static FireworkExplosion defaultExplosion() noexcept { return FireworkExplosion{}; }

    /**
     * @brief 仅替换渐变色列表（vanilla FireworkExplosion.withFadeColors）
     *
     * 其余字段原样保留。对应配方侧 `itemstack.update(FIREWORK_EXPLOSION, DEFAULT, list, withFadeColors)`。
     */
    [[nodiscard]] FireworkExplosion withFadeColors(std::vector<i32> fade) const
    {
        FireworkExplosion result = *this;
        result.fadeColors = std::move(fade);
        return result;
    }

    [[nodiscard]] bool operator==(const FireworkExplosion& other) const noexcept
    {
        return shape == other.shape && colors == other.colors && fadeColors == other.fadeColors &&
            hasTrail == other.hasTrail && hasTwinkle == other.hasTwinkle;
    }

    [[nodiscard]] bool operator!=(const FireworkExplosion& other) const noexcept { return !(*this == other); }
};

} // namespace component
} // namespace item
} // namespace mc
