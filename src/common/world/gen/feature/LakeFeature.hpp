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
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include <memory>
#include <string>

// Forward declarations
namespace mc {
class IWorldWriter;
class IWorld;
} // namespace mc

namespace mc::world::gen::feature::lake {

/**
 * @brief 湖泊特征配置
 *
 * 对应 MC 1.21.11: LakeFeature.Configuration(fluid, barrier)。
 * fluid/barrier 均为 BlockStateProvider，运行时按其 kind 采样（simple/weighted/rule_based/...）。
 */
struct LakeFeatureConfig {
    /// 流体状态提供者
    std::unique_ptr<state::BlockStateProvider> fluidProvider;
    /// 边界状态提供者
    std::unique_ptr<state::BlockStateProvider> barrierProvider;

    LakeFeatureConfig() = default;
    ~LakeFeatureConfig() = default;

    LakeFeatureConfig(const LakeFeatureConfig& other);
    LakeFeatureConfig(LakeFeatureConfig&&) noexcept = default;
    LakeFeatureConfig& operator=(const LakeFeatureConfig& other);
    LakeFeatureConfig& operator=(LakeFeatureConfig&&) noexcept = default;

    /**
     * @brief 取本次放置使用的流体状态
     */
    [[nodiscard]] const BlockState* getFluidState(const IWorld& world, math::IRandom& rng, i32 x, i32 y, i32 z) const;

    /**
     * @brief 取本次放置使用的边界状态
     */
    [[nodiscard]] const BlockState* getBarrierState(const IWorld& world, math::IRandom& rng, i32 x, i32 y, i32 z) const;
};

/**
 * @brief 湖泊特征
 *
 * 16x8x16 布尔数组雕刻算法：4~7 个随机椭球叠加形成不规则湖盆，
 * 校验边界后填充流体/洞穴空气并放置边界方块，水湖表面按生物群系冻结。
 *
 * 对应 MC 1.21.11: LakeFeature.place()
 */
class LakeFeature {
public:
    explicit LakeFeature(LakeFeatureConfig config);

    /**
     * @brief 在 (x,y,z) 生成湖泊（y 已是放置点，内部再 below(4)）
     */
    bool place(WorldGenRegion& world, math::Random& rng, i32 x, i32 y, i32 z);

private:
    /// !state.is(BlockTags.FEATURES_CANNOT_REPLACE)
    [[nodiscard]] static bool canReplaceBlock(const BlockState& state);

    LakeFeatureConfig m_config;
};

} // namespace mc::world::gen::feature::lake

namespace mc {

/**
 * @brief 配置化湖泊特征
 *
 * 数据驱动下 placement 链（Count/HeightRange 等）由 PlacedFeature 持有并在 place() 前走完，
 * 本类只负责在已确定的 pos 处放置湖泊。stage()=Lakes。
 */
class ConfiguredLakeFeature : public ConfiguredFeatureBase {
public:
    ConfiguredLakeFeature(world::gen::feature::lake::LakeFeatureConfig config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::Lakes; }

private:
    mutable world::gen::feature::lake::LakeFeature m_feature;
    std::string m_name;
};

} // namespace mc
