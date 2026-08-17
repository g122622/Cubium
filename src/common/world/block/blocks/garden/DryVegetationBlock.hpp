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
 */

#pragma once

#include "../agricultural/BushBlock.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 干草类植物方块基类
 *
 * 干草（short_dry_grass / tall_dry_grass）可放置在沙子、陶瓦、泥土类方块及耕地上，
 * 比普通植物（仅 #dirt + farmland）更宽松，以支持在沙漠/恶地生物群系生成。
 * 重写 canSustain 查询 #dry_vegetation_may_place_on 标签（= SAND + TERRACOTTA + DIRT + FARMLAND）。
 *
 * MC ID: minecraft:short_dry_grass / minecraft:tall_dry_grass
 */
class DryVegetationBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DryVegetationBlock(const BlockProperties& properties);

    ~DryVegetationBlock() noexcept override = default;

protected:
    /**
     * @brief 检查下方是否可支撑
     *
     * 下方方块须属于 #dry_vegetation_may_place_on 标签（沙/陶瓦/泥土/耕地）。
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;
};

} // namespace blocks
} // namespace mc
