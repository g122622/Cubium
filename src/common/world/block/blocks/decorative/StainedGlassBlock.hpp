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

#include "../../Block.hpp"
#include "../../IBeaconBeamColorProvider.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>

namespace mc {
namespace block {

/**
 * @brief 染色玻璃方块
 *
 * 透明玻璃方块，可以为信标光束提供颜色。
 * 实现 IBeaconBeamColorProvider 接口以支持信标光束颜色修改。
 */
class StainedGlassBlock : public Block, public IBeaconBeamColorProvider {
public:
    /**
     * @brief 构造染色玻璃方块
     *
     * @param properties 方块属性
     * @param color 染料颜色
     */
    StainedGlassBlock(BlockProperties properties, DyeColor color);

    /**
     * @brief 获取信标光束颜色
     *
     * 实现 IBeaconBeamColorProvider 接口。
     * 返回此染色玻璃对应的染料颜色。
     *
     * @return 染料颜色
     */
    [[nodiscard]] DyeColor getBeaconColor() const override { return m_color; }

    /**
     * @brief 获取信标光束颜色倍数
     *
     * 重写 Block::getBeaconColorMultiplier。
     * 返回与此染色玻璃颜色对应的 RGB 值。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 方块位置（可选）
     * @param beaconPos 信标位置（可选）
     * @return RGB 颜色数组指针
     */
    [[nodiscard]] const std::array<f32, 3>* getBeaconColorMultiplier(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const BlockPos* beaconPos = nullptr) const override;

private:
    DyeColor m_color;
    std::array<f32, 3> m_colorComponents;
};

} // namespace block
} // namespace mc
