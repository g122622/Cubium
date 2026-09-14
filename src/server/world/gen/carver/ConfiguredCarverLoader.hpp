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

#include <cstddef>
#include <memory>
#include <string>

namespace mc {

class ConfiguredCarverBase;

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::carver {

/**
 * @brief 配置化雕刻器 JSON 加载器
 *
 * 从数据包加载 configured_carver JSON 文件，注册到 ConfiguredCarverRegistry。
 *
 * JSON 格式 (MC 1.21.11):
 * {
 *   "type": "minecraft:cave",
 *   "config": {
 *     "probability": 0.15,
 *     "y": { "type": "minecraft:uniform", ... },
 *     "yScale": { ... } 或裸浮点,
 *     "lava_level": { "above_bottom": 8 },
 *     "replaceable": "#minecraft:overworld_carver_replaceables",
 *     "horizontal_radius_multiplier": { ... },   // cave 系
 *     "vertical_radius_multiplier": { ... },     // cave 系
 *     "floor_level": { ... } 或裸浮点,            // cave 系
 *     "vertical_rotation": { ... },              // canyon 系
 *     "shape": { "distance_factor":..., "thickness":..., "width_smoothness":...,
 *               "horizontal_radius_factor":..., "vertical_radius_default_factor":...,
 *               "vertical_radius_center_factor":... }  // canyon 系
 *   }
 * }
 *
 * 加载路径: data/<namespace>/worldgen/configured_carver/<path>.json
 * 支持的 type: cave / canyon / nether_cave（cave_extra_underground 复用 cave type）。
 * 严格报错：type 未支持、config 字段缺失或解析失败时返回 Error 中断加载。
 */
class ConfiguredCarverLoader {
public:
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& repo);
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 对象加载单个配置化雕刻器
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param id 雕刻器的 ResourceLocation（对应 JSON 文件名）
     * @return 构造的 ConfiguredCarverBase，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<ConfiguredCarverBase>> loadFromJson(
        const nlohmann::json& jsonObj, const ResourceLocation& id);
};

} // namespace world::gen::carver
} // namespace mc
