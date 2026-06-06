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

#include "../../../core/Constants.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../jigsaw/JigsawPattern.hpp"
#include "../jigsaw/PoolAliasBinding.hpp"
#include "../valueprovider/HeightProvider.hpp"
#include "Structure.hpp"
#include "StructureBoundingBox.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 液体设置
 *
 * 控制结构生成时如何处理含水方块。
 */
enum class LiquidSettings : u8 {
    ApplyWaterlogging, ///< 允许含水方块
    IgnoreWaterlogging ///< 忽略含水方块（试炼密室使用此模式）
};

/**
 * @brief 维度填充
 *
 * 控制结构距世界边界的最小距离。
 */
struct DimensionPadding {
    i32 top = 0;    ///< 顶部填充距离
    i32 bottom = 0; ///< 底部填充距离

    constexpr DimensionPadding() = default;
    constexpr DimensionPadding(i32 t, i32 b)
        : top(t)
        , bottom(b)
    {}
};

/**
 * @brief 最大距离约束
 *
 * 限制结构片段距中心的最大距离。
 */
struct MaxDistance {
    i32 maxDistance = 116; ///< 最大距离（格）

    constexpr MaxDistance() = default;
    constexpr MaxDistance(i32 dist)
        : maxDistance(dist)
    {}
};

/**
 * @brief Jigsaw 结构配置
 *
 * 配置 Jigsaw 结构的生成参数，包括起始模板池、深度、高度、地形适配等。
 * 支持 MC 1.21 的全部 Jigsaw 配置选项，包括池别名绑定、维度填充等。
 */
struct JigsawConfig {
    ResourceLocation startPool;                                         ///< 起始模板池
    i32 size = 7;                                                       ///< 结构尺寸（递归深度）
    std::optional<ResourceLocation> startJigsawName;                    ///< 起始 Jigsaw 名称（可选）
    std::unique_ptr<valueprovider::HeightProvider> startHeight;         ///< 起始高度提供者
    bool projectStartToHeightmap = false;                               ///< 是否将起始点投影到高度图
    std::optional<MaxDistance> maxDistanceFromCenter;                   ///< 距中心最大距离约束
    jigsaw::PoolAliasBindings poolAliases;                              ///< 池别名绑定集合
    DimensionPadding dimensionPadding;                                  ///< 维度填充
    LiquidSettings liquidSettings = LiquidSettings::IgnoreWaterlogging; ///< 液体设置

    JigsawConfig() = default;

    JigsawConfig(const JigsawConfig&) = delete;
    JigsawConfig& operator=(const JigsawConfig&) = delete;
    JigsawConfig(JigsawConfig&&) = default;
    JigsawConfig& operator=(JigsawConfig&&) = default;

    /**
     * @brief 构造 Jigsaw 配置（简单版本，兼容旧代码）
     * @param pool 起始模板池
     * @param s 结构尺寸
     */
    JigsawConfig(const ResourceLocation& pool, i32 s)
        : startPool(pool)
        , size(s)
    {}

    /**
     * @brief 构造 Jigsaw 配置（完整版本，支持所有选项）
     * @param pool 起始模板池
     * @param s 结构尺寸
     * @param height 起始高度提供者
     * @param aliases 池别名绑定
     * @param maxDist 距中心最大距离
     * @param padding 维度填充
     * @param liquid 液体设置
     */
    JigsawConfig(const ResourceLocation& pool,
        i32 s,
        std::unique_ptr<valueprovider::HeightProvider> height,
        jigsaw::PoolAliasBindings aliases = {},
        std::optional<MaxDistance> maxDist = std::nullopt,
        DimensionPadding padding = {},
        LiquidSettings liquid = LiquidSettings::IgnoreWaterlogging)
        : startPool(pool)
        , size(s)
        , startHeight(std::move(height))
        , maxDistanceFromCenter(maxDist)
        , poolAliases(std::move(aliases))
        , dimensionPadding(padding)
        , liquidSettings(liquid)
    {}
};

/**
 * @brief Jigsaw 结构
 *
 * 使用 Jigsaw 模板池生成的结构，如村庄、掠夺者前哨站等。
 * 支持地形适配、池别名绑定和动态组装。
 */
class JigsawStructure : public Structure {
public:
    /**
     * @brief 构造 Jigsaw 结构（简单版本，兼容旧代码）
     * @param type 结构类型
     * @param config Jigsaw 配置
     * @param startY 起始 Y 坐标，0 表示自动检测
     * @param nearTerrain 是否需要贴近地形生成
     * @param adjustForTerrain 是否根据地形调整高度
     * @param terrainAdaptation 地形适配模式
     */
    explicit JigsawStructure(StructureType type,
        JigsawConfig config,
        i32 startY = 0,
        bool nearTerrain = false,
        bool adjustForTerrain = false,
        TerrainAdaptation terrainAdaptation = TerrainAdaptation::None);

    [[nodiscard]] const std::string& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    /**
     * @brief 获取地形适配模式
     */
    [[nodiscard]] TerrainAdaptation terrainAdaptation() const noexcept override { return m_terrainAdaptation; }

    /**
     * @brief 检查是否可以在指定位置生成结构
     */
    bool canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成 Jigsaw 结构
     */
    std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

protected:
    JigsawConfig m_config;                 ///< Jigsaw 配置
    i32 m_startY;                          ///< 起始 Y 坐标（简单模式）
    bool m_nearTerrain;                    ///< 是否贴近地形
    bool m_adjustForTerrain;               ///< 是否根据地形调整
    TerrainAdaptation m_terrainAdaptation; ///< 地形适配模式

    static constexpr StructureSeparationSettings m_settings{8, 4, 12345};
    static const std::string m_name;
    static const std::vector<BiomeId> m_validBiomes;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
