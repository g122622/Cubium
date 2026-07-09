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

#include <memory>
#include <vector>

#include "common/util/AxisAlignedBB.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"

namespace mc {

// 前向声明
class IWorld;

/**
 * @brief 黑曜石柱状态
 *
 * 存储单根黑曜石柱的生成状态。
 * 字段对齐 MC 1.21.11 SpikeFeature.EndSpike。
 */
struct EndSpike {
    i32 centerX;  ///< 中心X坐标（方块坐标）
    i32 centerZ;  ///< 中心Z坐标（方块坐标）
    i32 radius;   ///< 半径（2-5）
    i32 height;   ///< 高度（76-103）
    bool guarded; ///< 是否有铁栏杆笼子

    EndSpike(i32 x, i32 z, i32 r, i32 h, bool g)
        : centerX(x)
        , centerZ(z)
        , radius(r)
        , height(h)
        , guarded(g)
    {}

    /**
     * @brief 检查柱子中心是否在指定区块内
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @return 中心是否在该区块范围内
     */
    [[nodiscard]] bool isCenterWithinChunk(i32 chunkX, i32 chunkZ) const
    {
        return (centerX >> 4) == chunkX && (centerZ >> 4) == chunkZ;
    }

    /**
     * @brief 获取柱子顶部碰撞箱
     *
     * 用于在世界中查找位于柱顶的末影水晶实体（updateCrystalCount / resetSpikeCrystals）。
     * 碰撞箱为覆盖整个 Y 轴的柱形区域（半径 = spike.radius），与 MC 1.21.11
     * SpikeFeature.EndSpike.topBoundingBox 一致。
     *
     * @return 顶部碰撞箱（X/Z 覆盖柱子圆形外接方形，Y 覆盖整个世界高度）
     */
    [[nodiscard]] AxisAlignedBB getTopBoundingBox() const
    {
        return AxisAlignedBB(static_cast<f64>(centerX - radius),
            static_cast<f64>(world::MIN_BUILD_HEIGHT),
            static_cast<f64>(centerZ - radius),
            static_cast<f64>(centerX + radius),
            static_cast<f64>(world::MAX_BUILD_HEIGHT),
            static_cast<f64>(centerZ + radius));
    }
};

/**
 * @brief 黑曜石柱特征配置
 *
 * 定义黑曜石柱生成的参数配置。
 * 字段对齐 MC 1.21.11 SpikeConfiguration。
 */
struct EndSpikeFeatureConfig : public IFeatureConfig {
    /// 黑曜石柱列表（如果为空则自动生成）
    std::vector<EndSpike> spikes;

    /// 是否在生成后摧毁柱子（用于末影龙战斗重生阶段）
    bool destroying = false;

    /// 末影水晶光束目标（可选，nullopt 表示无光束）
    std::optional<BlockPos> crystalBeamTarget;

    /// 末影水晶是否无敌（重生阶段中柱顶水晶为 true）
    bool crystalInvulnerable = false;

    EndSpikeFeatureConfig() = default;

    /**
     * @brief 完整构造函数
     * @param spikeList 柱子列表
     * @param destroy 是否为摧毁模式
     * @param beamTarget 水晶光束目标（可选）
     * @param invulnerable 水晶是否无敌
     */
    explicit EndSpikeFeatureConfig(const std::vector<EndSpike>& spikeList,
        bool destroy = false,
        std::optional<BlockPos> beamTarget = std::nullopt,
        bool invulnerable = false)
        : spikes(spikeList)
        , destroying(destroy)
        , crystalBeamTarget(std::move(beamTarget))
        , crystalInvulnerable(invulnerable)
    {}

    /**
     * @brief 生成默认的黑曜石柱配置（10根柱子）
     * @param worldSeed 世界种子
     * @return 黑曜石柱列表
     */
    static std::vector<EndSpike> generateSpikes(u64 worldSeed);
};

/**
 * @brief 黑曜石柱特征
 *
 * 在末地生成黑曜石柱（末影龙战斗区域）。
 *
 * 特点：
 * - 10根黑曜石柱围绕末地中心（0,0）
 * - 高度范围：76-103
 * - 半径范围：2-5
 * - 部分柱子顶部有铁栏杆笼子保护
 * - 柱顶生成末影水晶（带光束目标/无敌标志）
 *
 * 提供两类放置接口：
 * - place(WorldGenRegion&, ...)：世界生成阶段使用，按区块划分放置
 * - placeSpike(IWorld&, ...)：运行时（如龙重生阶段）使用，立即放置单根柱子
 */
class EndSpikeFeature {
public:
    /**
     * @brief 放置黑曜石柱特征（世界生成阶段）
     *
     * 按 chunkX/chunkZ 划分，仅生成中心位于该区块的柱子。
     *
     * @param world 世界生成区域
     * @param random 随机数生成器
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param config 黑曜石柱配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& world, math::Random& random, i32 chunkX, i32 chunkZ, const EndSpikeFeatureConfig& config);

    /**
     * @brief 在运行时放置单根黑曜石柱（含末影水晶）
     *
     * 用于末影龙重生阶段：立即在世界中放置指定柱子，包括：
     * - 黑曜石柱体
     * - 顶部的铁栏杆笼子（如果 guarded）
     * - 顶部基岩底座
     * - 顶部末影水晶（带光束目标/无敌标志）
     * - 底部火焰
     *
     * 对齐 MC 1.21.11 SpikeFeature.placeSpike()。
     *
     * @param world 世界接口
     * @param random 随机数生成器
     * @param config 配置（使用 crystalBeamTarget / crystalInvulnerable）
     * @param spike 要放置的柱子
     */
    void placeSpike(IWorld& world, math::Random& random, const EndSpikeFeatureConfig& config, const EndSpike& spike);

private:
    /**
     * @brief 检查柱子是否可以放置在指定位置
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 生成单根黑曜石柱（世界生成阶段，不含末影水晶）
     */
    void _generateSpike(WorldGenRegion& world, math::Random& random, const EndSpike& spike);

    /**
     * @brief 生成铁栏杆笼子
     */
    void _generateCage(WorldGenRegion& world, const BlockPos& topPos, i32 radius);
};

/**
 * @brief 配置化黑曜石柱特征
 */
class ConfiguredEndSpikeFeature : public ConfiguredFeatureBase {
public:
    ConfiguredEndSpikeFeature(std::unique_ptr<EndSpikeFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::SurfaceStructures; }
    [[nodiscard]] const EndSpikeFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<EndSpikeFeatureConfig> m_config;
    std::string m_name;
    mutable EndSpikeFeature m_feature;
};

} // namespace mc
