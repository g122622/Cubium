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

#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <string>

namespace mc {

class ConfiguredPlacement;
class PlacedFeature;

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::placement {

/**
 * @brief 放置特征 JSON 加载器
 *
 * 从数据包加载 placed_feature JSON 文件，注册到 PlacedFeatureRegistry。
 *
 * JSON 格式 (MC 1.21.11):
 * {
 *   "feature": "minecraft:monster_room",
 *   "placement": [
 *     { "type": "minecraft:count", "count": 10 },
 *     { "type": "minecraft:in_square" },
 *     { "type": "minecraft:height_range",
 *       "height": { "type": "minecraft:uniform",
 *                   "min_inclusive": { "absolute": 0 },
 *                   "max_inclusive": { "below_top": 0 } } },
 *     { "type": "minecraft:biome" }
 *   ]
 * }
 *
 * 加载路径: data/<namespace>/worldgen/placed_feature/<path>.json
 * 严格报错：feature 引用未注册的 configured_feature、或 placement type 未注册时返回 Error 中断。
 */
class PlacedFeatureLoader {
public:
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& repo);
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 对象加载单个放置特征
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param id 放置特征的 ResourceLocation（对应 JSON 文件名）
     * @return 构造的 PlacedFeature，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<PlacedFeature>> loadFromJson(
        const nlohmann::json& jsonObj, const ResourceLocation& id);

    /**
     * @brief 解析 placement 数组为 ConfiguredPlacement 链
     *
     * 供内联 PlacedFeature（random_patch/flower 等 config 中的 feature.placement）
     * 复用：内联 feature 无独立 id，BiomeFilterConfig 回填用调用方提供的 placedFeatureId。
     *
     * @param placementArr placement 数组
     * @param placedFeatureId 当前 placed_feature 的 id（回填给 BiomeFilterConfig，
     *                        用于运行时反查生物群系是否包含此特征）
     * @return 链头 ConfiguredPlacement，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<ConfiguredPlacement>> parsePlacementChain(
        const nlohmann::json& placementArr, const ResourceLocation& placedFeatureId);
};

} // namespace world::gen::placement
} // namespace mc
