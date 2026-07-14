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

#include "JigsawStructure.hpp"
#include "Structure.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/jigsaw/PoolAliasBinding.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"

#include <nlohmann/json_fwd.hpp>

#include <optional>
#include <string>
#include <vector>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::structure {

/**
 * @brief 结构定义数据
 *
 * 从 JSON 解析的结构定义，包含生成结构所需的全部配置参数。
 * 对应 MC 1.21.11 的 Structure 结构。
 */
struct StructureDefinition {
    ResourceLocation id;                                           ///< 结构资源位置
    std::string type;                                              ///< 结构类型（如 "minecraft:jigsaw"）
    ResourceLocation biomes;                                       ///< 生物群系标签引用
    DecorationStage step = DecorationStage::SurfaceStructures;     ///< 生成阶段
    TerrainAdaptation terrainAdaptation = TerrainAdaptation::None; ///< 地形适配模式

    // Jigsaw 结构专用参数
    ResourceLocation startPool;                                        ///< 起始模板池
    i32 size = 7;                                                      ///< 递归深度
    std::unique_ptr<valueprovider::HeightProvider> startHeight;        ///< 起始高度提供者
    bool projectStartToHeightmap = false;                              ///< 是否投影到高度图
    std::string heightmapName;                                         ///< 高度图名称（如 "WORLD_SURFACE_WG"）
    std::optional<MaxDistance> maxDistanceFromCenter;                  ///< 距中心最大距离
    std::optional<ResourceLocation> startJigsawName;                   ///< 起始 Jigsaw 名称
    jigsaw::PoolAliasBindings poolAliases;                             ///< 池别名绑定
    DimensionPadding dimensionPadding;                                 ///< 维度填充
    LiquidSettings liquidSettings = LiquidSettings::ApplyWaterlogging; ///< 液体设置
    bool useExpansionHack = false;                                     ///< 是否使用扩展技巧

    StructureDefinition() = default;
    StructureDefinition(const StructureDefinition&) = delete;
    StructureDefinition& operator=(const StructureDefinition&) = delete;
    StructureDefinition(StructureDefinition&&) = default;
    StructureDefinition& operator=(StructureDefinition&&) = default;
};

/**
 * @brief 结构定义 JSON 加载器
 *
 * 从数据包加载结构定义 JSON 文件。
 *
 * JSON 格式 (MC 1.21.11 Jigsaw 类型):
 * {
 *   "type": "minecraft:jigsaw",
 *   "biomes": "#minecraft:has_structure/village_desert",
 *   "spawn_overrides": {},
 *   "step": "surface_structures",
 *   "terrain_adaptation": "beard_thin",
 *   "start_pool": "minecraft:village/desert/town_centers",
 *   "size": 6,
 *   "start_height": { "absolute": 0 },
 *   "project_start_to_heightmap": "WORLD_SURFACE_WG",
 *   "max_distance_from_center": 80,
 *   "use_expansion_hack": true,
 *   "start_jigsaw_name": "minecraft:bottom",
 *   "pool_aliases": [],
 *   "dimension_padding": { "bottom": 0, "top": 0 },
 *   "liquid_settings": "apply_waterlogging"
 * }
 *
 * 加载路径: data/<namespace>/worldgen/structure/<path>.json
 */

// TODO(数据驱动迁移未完成): StructureDefinitionLoader 是数据驱动结构定义加载半成品。它实现了从
// 数据包 JSON 解析 StructureDefinition 的完整逻辑, 但尚未接入 MinecraftServer 初始化链路——当前
// 结构注册仍走 StructureRegistry::initialize()(StructureManager.cpp) 硬编码。本加载器零生产消费者,
// 其 .cpp 虽在 CMakeLists 编译但无任何调用入口。待完成接入(替换硬编码注册)后即为活代码。

class StructureDefinitionLoader {
public:
    /**
     * @brief 从数据包列表加载所有结构定义
     *
     * @param dataPackList 数据包列表
     * @return 加载的结构定义数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有结构定义
     *
     * @param pack 资源包
     * @return 加载的结构定义数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 字符串加载单个结构定义
     *
     * @param json JSON 内容
     * @param location 结构定义资源位置
     * @return 是否成功
     */
    [[nodiscard]] static Result<void> loadFromJson(const std::string& json, const ResourceLocation& location);

    /**
     * @brief 获取指定 ID 的结构定义
     *
     * @param id 结构资源位置
     * @return 结构定义指针，未找到返回 nullptr
     */
    [[nodiscard]] static const StructureDefinition* getDefinition(const ResourceLocation& id);

    /**
     * @brief 获取所有已加载的结构定义
     */
    [[nodiscard]] static const std::vector<std::unique_ptr<StructureDefinition>>& getAllDefinitions();

    /**
     * @brief 清除所有已加载的结构定义
     */
    static void clear();

private:
    /**
     * @brief 解析地形适配模式字符串
     */
    static TerrainAdaptation _parseTerrainAdaptation(const std::string& str);

    /**
     * @brief 解析装饰阶段字符串
     */
    static DecorationStage _parseDecorationStage(const std::string& str);

    /**
     * @brief 解析液体设置字符串
     */
    static LiquidSettings _parseLiquidSettings(const std::string& str);

    /**
     * @brief 解析高度提供者 JSON
     */
    static std::unique_ptr<valueprovider::HeightProvider> _parseHeightProvider(const nlohmann::json& jsonObj);

    /// 已加载的结构定义存储
    static std::vector<std::unique_ptr<StructureDefinition>> s_definitions;
    static std::unordered_map<ResourceLocation, StructureDefinition*> s_byId;
};

} // namespace world::gen::structure
} // namespace mc
