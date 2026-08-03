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

#include "ConfiguredFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"

namespace mc {

// 前向声明
class WorldGenRegion;

namespace world::gen::feature {

/**
 * @brief 藤蔓特征（vines），NoneFeatureConfiguration。
 *
 * 忠实复刻 MC 1.21.11 VinesFeature：
 * - origin 非空 → return false；
 * - 遍历除 DOWN 外的全部方向，若
 *   VineBlock.isAcceptableNeighbour(world, origin.relative(dir), dir) 为真
 *   （= 该邻居方块对 dir 的反向面提供 Center 支撑），则在 origin 放置
 *   VINE 默认状态并把 dir 对应的面属性置 true，return true；
 * - 全部方向都不满足 → return false。
 *
 * 装饰阶段 VegetalDecoration。
 */
class ConfiguredVinesFeature : public ConfiguredFeatureBase {
public:
    ConfiguredVinesFeature() = default;

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return "vines"; }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
};

} // namespace world::gen::feature
} // namespace mc
