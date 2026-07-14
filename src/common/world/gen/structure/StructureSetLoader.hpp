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

#include "StructureSet.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <string>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::structure {

/**
 * @brief 结构集合 JSON 加载器
 *
 * 从数据包加载 StructureSet JSON 文件。
 *
 * JSON 格式 (MC 1.21.11):
 * {
 *   "structures": [
 *     { "structure": "minecraft:village_plains", "weight": 1 },
 *     ...
 *   ],
 *   "placement": {
 *     "type": "minecraft:random_spread",
 *     "salt": 10387312,
 *     "spacing": 34,
 *     "separation": 8,
 *     "spread_type": "linear",
 *     "frequency": 1.0,
 *     "frequency_reduction_method": "default",
 *     "exclusion_zone": { "other_set": "minecraft:villages", "chunk_count": 10 },
 *     "locate_offset": [9, 0, 9]
 *   }
 * }
 *
 * 同心环类型:
 * {
 *   "structures": [{ "structure": "minecraft:stronghold", "weight": 1 }],
 *   "placement": {
 *     "type": "minecraft:concentric_rings",
 *     "distance": 32,
 *     "spread": 3,
 *     "count": 128,
 *     "preferred_biomes": "#minecraft:stronghold_biased_to",
 *     "salt": 0
 *   }
 * }
 *
 * 加载路径: data/<namespace>/worldgen/structure_set/<path>.json
 */

// TODO(数据驱动迁移未完成): StructureSetLoader 是数据驱动结构集合加载半成品。它实现了从数据包 JSON
// 解析 StructureSet 的完整逻辑(含 random_spread / concentric_rings 放置), 但尚未接入 MinecraftServer
// 初始化链路——当前结构集合注册仍走硬编码。本加载器零生产消费者, 其 .cpp 虽在 CMakeLists 编译但无
// 任何调用入口(StructureSetLoader.cpp 内部调用 StructureSetRegistry 不构成活引用, 因加载器本身无人调)。
// 待完成接入后即为活代码。

class StructureSetLoader {
public:
    /**
     * @brief 从数据包列表加载所有结构集合
     *
     * @param dataPackList 数据包列表
     * @return 加载的结构集合数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有结构集合
     *
     * @param pack 资源包
     * @return 加载的结构集合数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 字符串加载单个结构集合
     *
     * @param json JSON 内容
     * @param location 结构集合资源位置
     * @return 加载的结构集合，或错误信息
     */
    [[nodiscard]] static Result<std::unique_ptr<StructureSet>> loadFromJson(
        const std::string& json, const ResourceLocation& location);

    /**
     * @brief 从 JSON 对象加载单个结构集合
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param location 结构集合资源位置
     * @return 加载的结构集合，或错误信息
     */
    [[nodiscard]] static Result<std::unique_ptr<StructureSet>> loadFromJson(
        const nlohmann::json& jsonObj, const ResourceLocation& location);

private:
    /**
     * @brief 解析放置配置
     *
     * @param placementObj 放置配置 JSON 对象
     * @return 放置策略实例，或 nullptr
     */
    static std::unique_ptr<placement::StructurePlacement> _parsePlacement(const nlohmann::json& placementObj);

    /**
     * @brief 解析随机分布放置配置
     */
    static std::unique_ptr<placement::StructurePlacement> _parseRandomSpreadPlacement(
        const nlohmann::json& placementObj);

    /**
     * @brief 解析同心环放置配置
     */
    static std::unique_ptr<placement::StructurePlacement> _parseConcentricRingsPlacement(
        const nlohmann::json& placementObj);

    /**
     * @brief 解析频率缩减方法字符串
     */
    static placement::FrequencyReductionMethod _parseFrequencyReductionMethod(const std::string& method);

    /**
     * @brief 解析分布类型字符串
     */
    static placement::RandomSpreadType _parseSpreadType(const std::string& spreadType);

    /**
     * @brief 解析排斥区配置
     */
    static std::optional<placement::ExclusionZone> _parseExclusionZone(const nlohmann::json& placementObj);
};

} // namespace world::gen::structure
} // namespace mc
