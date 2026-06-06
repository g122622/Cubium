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

#include "CaveFeatureConfigs.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::feature::cave {

// ============================================================================
// SimpleBlockFeature — 简单方块放置特征
// ============================================================================

/**
 * @brief 简单方块放置特征
 *
 * 在指定位置放置单个方块，检查canSurvive条件。
 * 用于孢子花、苔藓植被等单方块放置。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.SimpleBlockFeature
 */
class SimpleBlockFeature {
public:
    /**
     * @brief 在指定位置放置方块
     * @param region 世界生成区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 简单方块配置
     * @return 是否成功放置
     */
    static bool place(
        WorldGenRegion& region, math::Random& random, const BlockPos& pos, const SimpleBlockConfig& config);
};

// ============================================================================
// RandomBooleanSelectorFeature — 随机布尔选择特征
// ============================================================================

/**
 * @brief 随机布尔选择特征
 *
 * 50%概率选择两个特征中的一个放置。
 * 用于LUSH_CAVES_CLAY（选择干黏土或水黏土池）。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.RandomBooleanSelectorFeature
 */
class RandomBooleanSelectorFeature {
public:
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const RandomBooleanFeatureConfig& config);
};

// ============================================================================
// SimpleRandomSelectorFeature — 随机选择特征
// ============================================================================

/**
 * @brief 随机选择特征
 *
 * 从特征列表中均匀随机选择一个放置。
 * 用于垂滴叶（选择小型垂滴叶或大型垂滴叶方向）。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.SimpleRandomSelectorFeature
 */
class SimpleRandomSelectorFeature {
public:
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const SimpleRandomFeatureConfig& config);
};

// ============================================================================
// BlockColumnFeature — 方块柱特征
// ============================================================================

/**
 * @brief 方块柱特征
 *
 * 沿指定方向放置多层方块柱。
 * 用于洞穴藤蔓和大型垂滴叶茎的生成。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.BlockColumnFeature
 */
class BlockColumnFeature {
public:
    /**
     * @brief 在指定位置放置方块柱
     * @param region 世界生成区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 方块柱配置
     * @return 是否成功放置
     */
    static bool place(
        WorldGenRegion& region, math::Random& random, const BlockPos& pos, const BlockColumnConfig& config);
};

// ============================================================================
// VegetationPatchFeature — 植被贴片特征
// ============================================================================

/**
 * @brief 植被贴片特征
 *
 * 在洞穴地面或天花板生成植被贴片。
 * 先放置地面方块（苔藓/黏土），然后在上面放置植被。
 * 用于LUSH_CAVES_VEGETATION、LUSH_CAVES_CEILING_VEGETATION、CLAY_WITH_DRIPLEAVES。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.VegetationPatchFeature
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

// ============================================================================
// WaterloggedVegetationPatchFeature — 含水植被贴片特征
// ============================================================================

/**
 * @brief 含水植被贴片特征
 *
 * 继承VegetationPatchFeature，额外将非暴露位置的地面方块替换为水，
 * 并对含水方块设置WATERLOGGED属性。
 * 用于CLAY_POOL_WITH_DRIPLEAVES。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.WaterloggedVegetationPatchFeature
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

// ============================================================================
// RootSystemFeature — 根系特征
// ============================================================================

/**
 * @brief 根系特征
 *
 * 生成杜鹃树及其根系：先向上寻找有效位置放置树木，
 * 然后填充缠根泥土柱，最后在下方放置垂根。
 * 用于ROOTED_AZALEA_TREE。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.RootSystemFeature
 */
class RootSystemFeature {
public:
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const RootSystemConfig& config);

private:
    /**
     * @brief 检查树木是否有足够的垂直空间
     */
    static bool spaceForTree(WorldGenRegion& region, const BlockPos& pos, i32 requiredSpace, i32 allowedWater);

    /**
     * @brief 放置缠根泥土柱
     */
    static void placeRootedDirtColumn(WorldGenRegion& region,
        math::Random& random,
        const BlockPos& origin,
        i32 targetY,
        const RootSystemConfig& config);

    /**
     * @brief 放置垂根
     */
    static void placeHangingRoots(
        WorldGenRegion& region, math::Random& random, const BlockPos& rootCenter, const RootSystemConfig& config);
};

// ============================================================================
// 配置化洞穴特征
// ============================================================================

/**
 * @brief 配置化简单方块特征
 */
class ConfiguredSimpleBlockFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSimpleBlockFeature(std::unique_ptr<SimpleBlockConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<SimpleBlockConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

/**
 * @brief 配置化植被贴片特征
 */
class ConfiguredVegetationPatchFeature : public ConfiguredFeatureBase {
public:
    ConfiguredVegetationPatchFeature(std::unique_ptr<VegetationPatchConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<VegetationPatchConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

/**
 * @brief 配置化含水植被贴片特征
 */
class ConfiguredWaterloggedPatchFeature : public ConfiguredFeatureBase {
public:
    ConfiguredWaterloggedPatchFeature(std::unique_ptr<VegetationPatchConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<VegetationPatchConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

/**
 * @brief 配置化方块柱特征
 */
class ConfiguredBlockColumnFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBlockColumnFeature(std::unique_ptr<BlockColumnConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<BlockColumnConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

/**
 * @brief 配置化根系特征
 */
class ConfiguredRootSystemFeature : public ConfiguredFeatureBase {
public:
    ConfiguredRootSystemFeature(std::unique_ptr<RootSystemConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<RootSystemConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

/**
 * @brief 配置化随机布尔选择特征
 */
class ConfiguredRandomBooleanSelectorFeature : public ConfiguredFeatureBase {
public:
    ConfiguredRandomBooleanSelectorFeature(std::unique_ptr<RandomBooleanFeatureConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<RandomBooleanFeatureConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

/**
 * @brief 配置化随机选择特征
 */
class ConfiguredSimpleRandomSelectorFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSimpleRandomSelectorFeature(std::unique_ptr<SimpleRandomFeatureConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<SimpleRandomFeatureConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

} // namespace mc::world::gen::feature::cave
