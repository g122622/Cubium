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
#include <memory>

namespace mc {

// 前向声明
class WorldGenRegion;

namespace world::gen::feature {

/**
 * @brief 缠绕藤配置（TwistingVinesConfig）。
 *
 * 对应 MC 1.21.11 TwistingVinesConfig{spreadWidth, spreadHeight, maxHeight}。
 */
struct TwistingVinesFeatureConfig {
    i32 spreadWidth = 0;
    i32 spreadHeight = 0;
    i32 maxHeight = 0;
};

/**
 * @brief 缠绕藤特征（twisting_vines），TwistingVinesConfig。
 *
 * 忠实复刻 MC 1.21.11 TwistingVinesFeature：
 * - isInvalidPlacement(origin)：origin 非空 或 origin.below() 非
 *   NETHERRACK|WARPED_NYLIUM|WARPED_WART_BLOCK → return false；
 * - spreadWidth^2 次随机偏移尝试，findFirstAirBlockAboveGround 且非
 *   isInvalidPlacement → 向上生长缠绕藤柱（高度 1..maxHeight，1/6 翻倍，1/5 归 1），
 *   柱身 TWISTING_VINES_PLANT，柱顶 TWISTING_VINES（AGE 17..25）。
 *
 * 装饰阶段 VegetalDecoration。
 */
class ConfiguredTwistingVinesFeature : public ConfiguredFeatureBase {
public:
    explicit ConfiguredTwistingVinesFeature(std::unique_ptr<TwistingVinesFeatureConfig> config);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return "twisting_vines"; }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<TwistingVinesFeatureConfig> m_config;
};

} // namespace world::gen::feature
} // namespace mc
