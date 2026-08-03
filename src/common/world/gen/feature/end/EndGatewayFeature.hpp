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
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

/**
 * @brief 末地折跃门配置
 */
struct EndGatewayFeatureConfig : public IFeatureConfig {
    /// 是否为退出折跃门（在玩家进入时生成）
    bool isExit = false;

    /// 传送到外岛的精确位置（如果为空则使用默认位置）
    std::optional<BlockPos> exactPosition;

    EndGatewayFeatureConfig() noexcept = default;

    explicit EndGatewayFeatureConfig(bool exit, const std::optional<BlockPos>& pos = {})
        : isExit(exit)
        , exactPosition(pos)
    {}
};

/**
 * @brief 末地折跃门特征
 *
 * 在末地生成末地折跃门，用于在主岛和外岛之间传送。
 *
 * 特点：
 * - 末影龙死亡后生成（最多20个）
 * - 由基岩、末地折跃门方块组成
 * - 传送到1024格外的外岛
 * - 折跃门方块有紫色光柱效果
 */
class EndGatewayFeature {
public:
    /**
     * @brief 放置末地折跃门
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 折跃门配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const EndGatewayFeatureConfig& config);

    /**
     * @brief 计算折跃门的传送目标
     * @param currentPos 当前折跃门位置
     * @param seed 世界种子
     * @return 传送目标位置
     */
    static BlockPos calculateTeleportTarget(const BlockPos& currentPos, u64 seed);

private:
    /**
     * @brief 检查折跃门是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 生成折跃门结构
     */
    void _generateGateway(WorldGenRegion& world, math::Random& random, const BlockPos& pos);
};

/**
 * @brief 配置化末地折跃门特征
 */
class ConfiguredEndGatewayFeature : public ConfiguredFeatureBase {
public:
    ConfiguredEndGatewayFeature(std::unique_ptr<EndGatewayFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::SurfaceStructures; }
    [[nodiscard]] const EndGatewayFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<EndGatewayFeatureConfig> m_config;
    std::string m_name;
    mutable EndGatewayFeature m_feature;
};

} // namespace mc
