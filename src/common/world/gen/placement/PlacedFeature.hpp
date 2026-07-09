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

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include <memory>

namespace mc {

// 前向声明
class WorldGenRegion;
namespace world::chunk {
class ChunkPrimer;
}
using world::chunk::ChunkPrimer;
class IChunkGenerator;

/**
 * @brief 配置化放置特征（PlacedFeature）
 *
 * 组合一个 ConfiguredFeatureBase 与一条 ConfiguredPlacement 修饰链。
 *
 * place() 先走 placement 链得到候选位置列表，再对每个位置调用
 * ConfiguredFeatureBase::place()。
 *
 * 生物群系的 BiomeGenerationSettings 存储 PlacedFeature 的 ResourceLocation id
 * （而非 u32 featureId），由 PlacedFeatureRegistry 解析 id → const PlacedFeature*。
 */
class PlacedFeature {
public:
    /**
     * @brief 构造放置特征
     * @param feature 配置化特征（不拥有所有权，由 ConfiguredFeatureRegistry 持有）
     * @param placement 放置链（拥有所有权）
     * @param id 放置特征的 ResourceLocation（对应 placed_feature JSON 文件名）
     */
    PlacedFeature(
        const ConfiguredFeatureBase* feature, std::unique_ptr<ConfiguredPlacement> placement, ResourceLocation id);

    /**
     * @brief 在指定区块原点放置特征
     *
     * 先走 placement 链 getPositions 得到候选位置，
     * 再对每个位置调用配置化特征的 place()。
     *
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param generator 区块生成器
     * @param random 随机数生成器
     * @param origin 区块原点（区块世界坐标 ×16 起点）
     * @return 是否任意一次放置成功
     */
    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& origin) const;

    /**
     * @brief 获取放置特征的 ResourceLocation id
     */
    [[nodiscard]] const ResourceLocation& id() const noexcept { return m_id; }

    /**
     * @brief 获取配置化特征
     */
    [[nodiscard]] const ConfiguredFeatureBase* feature() const noexcept { return m_feature; }

    /**
     * @brief 获取装饰阶段（委托给配置化特征）
     */
    [[nodiscard]] DecorationStage stage() const noexcept { return m_feature->stage(); }

private:
    const ConfiguredFeatureBase* m_feature;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    ResourceLocation m_id;
};

} // namespace mc
