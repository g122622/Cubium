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
#include "common/world/gen/feature/parser/BlockStateProviderParser.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class WorldGenRegion;

namespace world::gen::feature {

/**
 * @brief disk 配置
 *
 * 对应 MC 1.21.11 DiskConfiguration{stateProvider, target, radius, halfHeight}。
 * stateProvider 为 RuleBasedBlockStateProvider（fallback + rules）；target 为 BlockPredicate。
 */
struct DiskConfig {
    std::unique_ptr<parser::BlockStateProviderHandle> stateProvider;
    std::unique_ptr<predicate::BlockPredicate> target;
    std::unique_ptr<valueprovider::IntProvider> radius;
    i32 halfHeight = 0;

    DiskConfig() = default;
};

/**
 * @brief 磁盘特征（disk）
 *
 * 忠实复刻 MC 1.21.11 DiskFeature：
 * - r = radius.sample(rng)；遍历 origin 周围 [-r,0,-r]..[r,0,r] 的 XZ 圆盘；
 * - 对 dx²+dz² <= r² 的列，从 y=origin.y+halfHeight 向下到 origin.y-halfHeight-1，
 *   每格若 target.test(world, pos) 为真则替换为 stateProvider.getState(world, rng, pos)；
 * - markAboveForPostProcessing 省略（项目无对应 API，不影响生成正确性）。
 *
 * 装饰阶段 UndergroundDecoration。
 */
class ConfiguredDiskFeature : public ConfiguredFeatureBase {
public:
    explicit ConfiguredDiskFeature(std::unique_ptr<DiskConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<DiskConfig> m_config;
    std::string m_name;
};

} // namespace world::gen::feature
} // namespace mc
