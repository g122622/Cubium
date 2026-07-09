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

#include "../gen/feature/DecorationStage.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <vector>

// 前向声明（必须在 mc::world::biome 命名空间之外，避免命名空间污染）
namespace mc {
class ConfiguredCarverBase;
class IChunkGenerator;
class WorldGenRegion;
namespace world::chunk {
class ChunkPrimer;
} // namespace world::chunk
} // namespace mc

namespace mc {
namespace world {
namespace biome {

// 跨命名空间类型简写
using ChunkPrimer = ::mc::world::chunk::ChunkPrimer;
using ConfiguredCarverBase = ::mc::ConfiguredCarverBase;
using IChunkGenerator = ::mc::IChunkGenerator;

/**
 * @brief 生物群系生成设置
 *
 * 存储生物群系特有的 placed_feature 与 carver 列表，按装饰阶段组织。
 *
 * 数据驱动：features 存储 placed_feature 的 ResourceLocation id（由 BiomeLoader
 * 从数据包 biome JSON 的 features 二维数组解析填充），运行时由 FeatureSorter
 * 拓扑排序后经 PlacedFeatureRegistry 解析为 const PlacedFeature*。
 */
class BiomeGenerationSettings {
public:
    BiomeGenerationSettings();
    ~BiomeGenerationSettings();

    BiomeGenerationSettings(BiomeGenerationSettings&&) noexcept;
    BiomeGenerationSettings& operator=(BiomeGenerationSettings&&) noexcept;
    BiomeGenerationSettings(const BiomeGenerationSettings&) = delete;
    BiomeGenerationSettings& operator=(const BiomeGenerationSettings&) = delete;

    /**
     * @brief 添加 placed_feature 到指定阶段
     * @param stage 装饰阶段
     * @param placedFeatureId placed_feature 的 ResourceLocation（对应 placed_feature JSON 文件名）
     */
    void addPlacedFeature(DecorationStage stage, ResourceLocation placedFeatureId);

    /**
     * @brief 添加花卉 placed_feature id
     *
     * 花卉特征同时添加到 VegetalDecoration 阶段的通用列表和独立的花卉列表。
     * 用于骨粉在草方块上放置花朵时，根据生物群系选择对应的花卉列表。
     *
     * TODO: BiomeLoader 当前未从 biome JSON 填充花卉列表（无对应字段），本方法暂无调用方，
     * 导致骨粉催花在所有生物群系都回退到蒲公英。待确定花卉列表的数据来源后接入。
     */
    void addFlowerFeature(ResourceLocation placedFeatureId);

    /**
     * @brief 获取指定阶段的 placed_feature id 列表
     */
    [[nodiscard]] const std::vector<ResourceLocation>& getFeatures(DecorationStage stage) const noexcept;

    /**
     * @brief 检查是否包含指定 placed_feature id
     *
     * 遍历所有装饰阶段的特征列表，查找是否存在匹配的 id。
     * 用于 BiomeFilterPlacement 的反向查询。
     */
    [[nodiscard]] bool hasPlacedFeature(const ResourceLocation& placedFeatureId) const noexcept;

    /**
     * @brief 获取此生物群系的花卉 placed_feature id 列表
     *
     * 用于骨粉在草方块上放置花朵时，根据生物群系选择对应的花卉列表。
     */
    [[nodiscard]] const std::vector<ResourceLocation>& getFlowerFeatureIds() const noexcept;

    /**
     * @brief 清除所有特征和雕刻器
     */
    void clear() noexcept;

    /**
     * @brief 添加配置化雕刻器（按 ResourceLocation 引用 ConfiguredCarverRegistry）
     * @param carverId configured_carver 的 ResourceLocation
     */
    void addCarver(ResourceLocation carverId);

    /**
     * @brief 获取雕刻器 id 列表
     */
    [[nodiscard]] const std::vector<ResourceLocation>& getCarvers() const noexcept;

private:
    // 按阶段存储 placed_feature 的 ResourceLocation 列表
    std::vector<std::vector<ResourceLocation>> m_featuresByStage;

    // 花卉 placed_feature id 列表（用于骨粉放置花朵时从生物群系获取花卉）
    std::vector<ResourceLocation> m_flowerFeatureIds;

    // 配置化雕刻器的 ResourceLocation 列表（引用 ConfiguredCarverRegistry）
    std::vector<ResourceLocation> m_carvers;
};

} // namespace biome
} // namespace world
} // namespace mc

// 旧命名空间兼容别名
namespace mc {
using BiomeGenerationSettings = ::mc::world::biome::BiomeGenerationSettings;
} // namespace mc
