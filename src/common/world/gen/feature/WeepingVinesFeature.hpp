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
 * @brief 泣泪藤特征（weeping_vines），NoneFeatureConfiguration。
 *
 * 忠实复刻 MC 1.21.11 WeepingVinesFeature：
 * - origin 非空 → return false；
 * - origin.above() 非 NETHERRACK 且非 NETHER_WART_BLOCK → return false；
 * - placeRoofNetherWart：origin 处放 NETHER_WART_BLOCK，200 次随机偏移尝试，
 *   空格且水平/上下邻居恰 1 个 NETHERRACK|NETHER_WART_BLOCK → 放 NETHER_WART_BLOCK；
 * - placeRoofWeepingVines：100 次随机偏移尝试，空格且 above 为
 *   NETHERRACK|NETHER_WART_BLOCK → 向下生长泣泪藤柱（高度 1..8，1/6 翻倍，1/5 归 1），
 *   柱身 WEEPING_VINES_PLANT，柱顶 WEEPING_VINES（AGE 17..25）。
 *
 * 装饰阶段 VegetalDecoration。
 */
class ConfiguredWeepingVinesFeature : public ConfiguredFeatureBase {
public:
    ConfiguredWeepingVinesFeature() = default;

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return "weeping_vines"; }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
};

} // namespace world::gen::feature
} // namespace mc
