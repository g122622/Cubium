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

#include "FlatLayerInfo.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockState.hpp"
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

/**
 * @brief 填充层条目
 *
 * 描述平坦世界中需要由特性系统放置的非运动阻挡层。
 * 在 FlatLevelGeneratorSettings::updateLayers() 中，非运动阻挡方块
 * （如水、空气等不阻挡运动的方块）被替换为 nullptr，由 placeFeatures()
 * 在 TOP_LAYER_MODIFICATION 阶段补充放置。这样设计是为了让湖泊等特性
 * 有机会先在那些位置生成，避免冲突。
 */
struct FillLayerEntry {
    i32 height;                   ///< 层高度（相对于世界最小高度的 Y 偏移量）
    const BlockState* blockState; ///< 要放置的方块状态
};

/**
 * @brief 平坦世界生成设置
 *
 * 定义平坦世界的层配置、生物群系和特性标志。
 *
 * MC 默认配置：
 * - 生物群系: Plains
 * - 层: 1x Bedrock + 2x Dirt + 1x Grass Block
 * - decoration: false
 * - addLakes: false
 */
class FlatLevelGeneratorSettings {
public:
    FlatLevelGeneratorSettings() = default;

    /**
     * @brief 构造平坦世界生成设置
     * @param biomeId 生物群系 ID
     * @param decoration 是否添加生物群系装饰特性
     * @param addLakes 是否添加湖泊
     */
    FlatLevelGeneratorSettings(BiomeId biomeId, bool decoration = false, bool addLakes = false)
        : m_biomeId(biomeId)
        , m_decoration(decoration)
        , m_addLakes(addLakes)
    {}

    // === 层配置 ===

    /** 获取层信息列表（可修改） */
    std::vector<FlatLayerInfo>& layersInfo() { return m_layersInfo; }

    /** 获取层信息列表（只读） */
    [[nodiscard]] const std::vector<FlatLayerInfo>& layersInfo() const { return m_layersInfo; }

    /**
     * @brief 获取展开后的层列表（每个 Y 级别一个 BlockState）
     *
     * 从底部（minY）开始，逐层展开为每个 Y 级别一个 BlockState。
     * 不阻挡运动的方块（如水）替换为 nullptr，由特性系统放置。
     *
     * 必须在设置完 layersInfo 后调用 updateLayers() 来生成此列表。
     */
    [[nodiscard]] const std::vector<const BlockState*>& layers() const { return m_layers; }

    /**
     * @brief 获取需要由特性系统填充的层条目
     *
     * 在 updateLayers() 中，非运动阻挡方块被替换为 nullptr，
     * 同时将原始方块状态和高度记录到填充层列表中。
     * 这些层将在 placeFeatures() 的 TOP_LAYER_MODIFICATION 阶段补充放置。
     */
    [[nodiscard]] const std::vector<FillLayerEntry>& fillLayerEntries() const { return m_fillLayerEntries; }

    // === 生物群系 ===

    /** 获取生物群系 ID */
    [[nodiscard]] BiomeId biomeId() const { return m_biomeId; }

    /** 设置生物群系 ID */
    void setBiomeId(BiomeId id) { m_biomeId = id; }

    // === 特性标志 ===

    /** 是否添加生物群系装饰特性（矿石、树木等） */
    [[nodiscard]] bool hasDecoration() const { return m_decoration; }
    void setDecoration(bool decoration) { m_decoration = decoration; }

    /** 是否添加湖泊 */
    [[nodiscard]] bool hasLakes() const { return m_addLakes; }
    void setLakes(bool addLakes) { m_addLakes = addLakes; }

    // === 结构生成覆盖 ===

    /**
     * @brief 获取结构生成覆盖列表
     *
     * 指定平坦世界中允许生成的结构集（白名单）。
     * 如果列表为空，表示使用所有可用的结构集（受生物群系过滤）。
     * MC 1.21.11: 对应 FlatLevelGeneratorSettings.structureOverrides 字段。
     *
     * MC 默认超平坦世界启用: minecraft:villages, minecraft:strongholds
     * MC 预设 "Tunneler's Dream": minecraft:mineshafts, minecraft:strongholds
     */
    [[nodiscard]] const std::vector<ResourceLocation>& structureOverrides() const { return m_structureOverrides; }

    /** 获取结构生成覆盖列表（可修改） */
    std::vector<ResourceLocation>& structureOverrides() { return m_structureOverrides; }

    /** 设置结构生成覆盖列表 */
    void setStructureOverrides(std::vector<ResourceLocation> overrides) { m_structureOverrides = std::move(overrides); }

    /**
     * @brief 检查是否启用了结构生成
     *
     * 当 structureOverrides 为空时返回 false（不生成任何结构），
     * 因为空的覆盖列表在 MC 中表示"使用所有结构集"，但在项目中
     * 我们选择与 hasDecoration/hasLakes 保持一致的行为——
     * 需要显式指定才生成结构。
     * 如果需要生成所有结构，可以使用 setStructureOverridesAll()。
     *
     * @note 实际上，MC 原版的 structureOverrides 为 Optional<HolderSet<StructureSet>>，
     *       Optional.empty 表示使用所有结构集，Optional.present 表示仅使用指定集合。
     *       这里简化为：空列表 = 不生成结构，非空列表 = 仅生成指定结构集。
     */
    [[nodiscard]] bool hasStructureGeneration() const { return !m_structureOverrides.empty(); }

    // === 虚空检测 ===

    /** 是否为虚空世界（所有层都是空气） */
    [[nodiscard]] bool isVoidGen() const { return m_voidGen; }

    // === 更新操作 ===

    /**
     * @brief 从 layersInfo 重新计算展开层列表
     *
     * 必须在修改 layersInfo 后调用此方法。
     * 展开层列表从底部开始，每层按高度展开为多个条目。
     * 不阻挡运动的方块替换为 nullptr。
     */
    void updateLayers();

    /**
     * @brief 创建默认平坦世界设置
     *
     * 默认配置：
     * - 生物群系: Plains
     * - 层: 1x Bedrock + 2x Dirt + 1x Grass Block
     * - decoration: false
     * - addLakes: false
     */
    static FlatLevelGeneratorSettings createDefault();

    /**
     * @brief 从 flat_level_generator_preset JSON 解析平坦世界设置
     *
     * JSON 顶层为 { "display": ..., "settings": { biome/layers/features/lakes/structure_overrides } }。
     * 仅解析 settings 子对象：biome(RL→BiomeId 经 BiomeLoader::biomeIdByName)、
     * layers（每层 {block:RL, height:int}，block 经 BlockRegistry::get 取默认 BlockState）、
     * features/lakes(bool)、structure_overrides(string|array，兼容单字符串/数组/空数组三态）。
     *
     * @param root 顶层 JSON 对象
     * @param id 预设资源位置（用于错误日志）
     * @return 平坦世界设置，或错误
     */
    [[nodiscard]] static Result<FlatLevelGeneratorSettings> fromJson(
        const nlohmann::json& root, const ResourceLocation& id);

private:
    std::vector<FlatLayerInfo> m_layersInfo; ///< 层定义（方块 + 高度）
    std::vector<const BlockState*>
        m_layers; ///< 展开后的层列表（每个 Y 级别一个 BlockState，nullptr 表示由特性系统放置）
    std::vector<FillLayerEntry> m_fillLayerEntries;     ///< 非运动阻挡层的填充信息（高度 + 方块状态）
    BiomeId m_biomeId = Biomes::Plains;                 ///< 生物群系
    bool m_decoration = false;                          ///< 是否添加装饰特性
    bool m_addLakes = false;                            ///< 是否添加湖泊
    bool m_voidGen = true;                              ///< 是否为虚空世界（初始为 true，updateLayers 时更新）
    std::vector<ResourceLocation> m_structureOverrides; ///< 结构生成覆盖列表（白名单，空=不生成结构）
};

} // namespace mc
