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

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

#include <memory>
#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace cave {

/**
 * @brief 幽匿斑块配置（MC SculkPatchConfiguration）
 *
 * chargeCount 个电荷源，每个 amountPerCharge 电荷；每轮 spreadAttempts 次
 * updateCursors；spreadRounds 轮纯扩散 + growthRounds 轮生长。extraRareGrowths
 * 为额外尖叫体生成数量的 IntProvider；catalystChance 为把中心替换为催化体的概率。
 *
 * 字段范围（MC codec）：charge_count[1,32] / amount_per_charge[1,500] /
 * spread_attempts[1,64] / growth_rounds[0,8] / spread_rounds[0,8] /
 * catalyst_chance[0.0,1.0]。
 */
struct SculkPatchConfig : public IFeatureConfig {
    i32 chargeCount = 0;
    i32 amountPerCharge = 0;
    i32 spreadAttempts = 0;
    i32 growthRounds = 0;
    i32 spreadRounds = 0;
    std::unique_ptr<valueprovider::IntProvider> extraRareGrowths;
    f32 catalystChance = 0.0f;
};

/**
 * @brief 幽匿斑块特征（MC SculkPatchFeature）
 *
 * 算法：
 * 1. canSpreadFrom(origin)：origin 是 SculkBehaviour，或为空气/非水源水且六邻至少一个
 *    碰撞完整方块。否则放弃。
 * 2. 创建 worldgen 扩散器，循环 spreadRounds+growthRounds 轮：每轮注入 chargeCount×
 *    amountPerCharge 电荷，spreadAttempts 次 updateCursors（前 spreadRounds 轮
 *    shouldUpdateBlocks=true 触发方块转化，其后=false 仅生长），末尾 clear。
 * 3. catalystChance 概率把 origin 替换为 SCULK_CATALYST（前提：下方碰撞完整方块）。
 * 4. extraRareGrowths.sample() 次：origin 的 5×5（xz 各 -2..2）随机点，若为空气且
 *    下方 UP 面 sturdy，放置 CAN_SUMMON=true 的 SCULK_SHRIEKER。
 *
 * 装饰阶段为 UndergroundDecoration。
 */
class SculkPatchFeature {
public:
    bool place(IWorld& world,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const SculkPatchConfig& config);

private:
    /// MC SculkPatchFeature.canSpreadFrom。
    [[nodiscard]] static bool canSpreadFrom(IWorld& world, const BlockPos& pos);
};

/**
 * @brief 配置化幽匿斑块特征
 */
class ConfiguredSculkPatchFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSculkPatchFeature(std::unique_ptr<SculkPatchConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<SculkPatchConfig> m_config;
    std::string m_name;
    mutable SculkPatchFeature m_feature;
};

} // namespace cave
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
