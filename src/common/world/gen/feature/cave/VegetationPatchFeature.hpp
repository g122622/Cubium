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

#include "CaveSurface.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::feature::cave {

/**
 * @brief 植被贴片配置
 *
 * 定义洞穴地面/天花板植被贴片的生成参数。
 * 用于苔藓贴片和黏土贴片的生成。
 */
struct VegetationPatchConfig {
    /// 可被替换的方块标签名称
    std::string replaceableTag;

    /// 地面方块状态提供者
    const BlockState* groundState = nullptr;

    /// 植被特征ID（ConfiguredFeatureRegistry 中的 ResourceLocation）
    ResourceLocation vegetationFeatureId;

    /// 贴片表面方向
    CaveSurface surface = CaveSurface::Floor;

    /// 地面深度（层数）
    std::unique_ptr<valueprovider::IntProvider> depth;

    /// 底部额外方块概率
    f32 extraBottomBlockChance = 0.0f;

    /// 垂直搜索范围
    i32 verticalRange = 5;

    /// 植被放置概率
    f32 vegetationChance = 0.8f;

    /// XZ半径范围
    std::unique_ptr<valueprovider::IntProvider> xzRadius;

    /// 边缘额外列概率
    f32 extraEdgeColumnChance = 0.3f;

    VegetationPatchConfig() = default;

    /**
     * @brief 构造地面贴片配置
     */
    static VegetationPatchConfig floorPatch(const std::string& replaceableTag,
        const BlockState* groundState,
        ResourceLocation vegetationFeatureId,
        std::unique_ptr<valueprovider::IntProvider> depth,
        f32 extraBottomBlockChance,
        i32 verticalRange,
        f32 vegetationChance,
        std::unique_ptr<valueprovider::IntProvider> xzRadius,
        f32 extraEdgeColumnChance);

    /**
     * @brief 构造天花板贴片配置
     */
    static VegetationPatchConfig ceilingPatch(const std::string& replaceableTag,
        const BlockState* groundState,
        ResourceLocation vegetationFeatureId,
        std::unique_ptr<valueprovider::IntProvider> depth,
        f32 extraBottomBlockChance,
        i32 verticalRange,
        f32 vegetationChance,
        std::unique_ptr<valueprovider::IntProvider> xzRadius,
        f32 extraEdgeColumnChance);
};

/**
 * @brief 植被贴片特征
 *
 * 在洞穴地面或天花板生成植被贴片。
 * 先放置地面方块（苔藓/黏土），然后在上面放置植被。
 * 用于LUSH_CAVES_VEGETATION、LUSH_CAVES_CEILING_VEGETATION、CLAY_WITH_DRIPLEAVES。
 */
class VegetationPatchFeature {
public:
    /**
     * @brief 在指定位置放置植被贴片
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param generator 区块生成器
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 植被贴片配置
     * @return 是否成功放置
     */
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const VegetationPatchConfig& config);

    /**
     * @brief 放置地面贴片
     * @return 成功放置的地面位置集合
     */
    static std::vector<BlockPos> placeGroundPatch(
        WorldGenRegion& region, math::Random& random, const BlockPos& pos, const VegetationPatchConfig& config);

private:
    /**
     * @brief 在地面上放置一个方块列
     * @return 是否放置了至少一个方块
     */
    static bool placeGround(WorldGenRegion& region,
        math::Random& random,
        const BlockPos& pos,
        const VegetationPatchConfig& config,
        Direction surfaceDir,
        i32 depth);

    /**
     * @brief 在地面贴片上分布植被
     */
    static void distributeVegetation(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const std::vector<BlockPos>& groundPositions,
        const VegetationPatchConfig& config);
};

/**
 * @brief 含水植被贴片特征
 *
 * 继承VegetationPatchFeature，额外将非暴露位置的地面方块替换为水，
 * 并对含水方块设置WATERLOGGED属性。
 * 用于CLAY_POOL_WITH_DRIPLEAVES。
 */
class WaterloggedVegetationPatchFeature {
public:
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const VegetationPatchConfig& config);

private:
    /**
     * @brief 检查位置是否暴露在非实心面
     */
    static bool isExposed(WorldGenRegion& region, const BlockPos& pos);
};

/**
 * @brief 配置化植被贴片特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，本类只负责在已确定的 pos 处放置。
 */
class ConfiguredVegetationPatchFeature : public ConfiguredFeatureBase {
public:
    ConfiguredVegetationPatchFeature(std::unique_ptr<VegetationPatchConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<VegetationPatchConfig> m_config;
    std::string m_name;
};

/**
 * @brief 配置化含水植被贴片特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，本类只负责在已确定的 pos 处放置。
 */
class ConfiguredWaterloggedPatchFeature : public ConfiguredFeatureBase {
public:
    ConfiguredWaterloggedPatchFeature(std::unique_ptr<VegetationPatchConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<VegetationPatchConfig> m_config;
    std::string m_name;
};

} // namespace mc::world::gen::feature::cave
