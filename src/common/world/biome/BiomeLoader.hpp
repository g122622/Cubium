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
#include "common/world/biome/BiomeIds.hpp"

#include <nlohmann/json_fwd.hpp>

#include <cstddef>
#include <optional>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::biome {

/**
 * @brief 生物群系 JSON 加载器（数据驱动世界生成的最后一环）
 *
 * 从数据包加载 biome JSON 文件，将 JSON 覆盖的字段叠加到已由 BiomeFactory 构造的
 * Biome 对象上。BiomeFactory 提供深度/比例/地表方块等非 JSON 字段，BiomeLoader
 * 仅覆盖 climate/effects/spawners/spawn_costs/generationSettings 这些 JSON 字段。
 *
 * 混合策略：
 * - BiomeRegistry 必须先 initialize()（由 BiomeFactory 构造所有默认 Biome）
 * - BiomeLoader 通过 BiomeRegistry::getMutable(id) 取得可变 Biome&
 * - 调用 Biome 的 setter / 可变 generationSettings() 叠加 JSON 字段
 *
 * JSON 格式 (MC 1.21.11) 示例（plains.json）：
 * {
 *   "temperature": 0.8,
 *   "downfall": 0.4,
 *   "has_precipitation": true,
 *   "temperature_modifier": "none",       // 可选，"frozen" 仅冰洋等
 *   "effects": { "sky_color": "#78a7ff", "water_color": "#3f76e4", ... },
 *   "carvers": ["minecraft:cave", ...] 或 "minecraft:nether_cave" 或 { "air": [...], "liquid": [...] },
 *   "features": [ [], [...], ..., [...] ], // 10 或 11 个数组，每数组是 placed_feature id 列表
 *   "spawners": { "monster": [...], "creature": [...], ... },
 *   "spawn_costs": { "minecraft:zombie": { "charge": 0.7, "energy_budget": 0.12 }, ... }
 * }
 *
 * 加载路径: data/<namespace>/worldgen/biome/<path>.json
 * ResourceLocation = <namespace>:<path>（去 .json），通过内置 65 项映射表解析为 BiomeId。
 *
 * 错误策略：
 * - 数据包 biome 名在映射表中找不到对应 BiomeId → warn + skip（不中断）
 * - placed_feature / carver id 在对应 Registry 中未注册 → warn + skip（不中断，世界仍可生成）
 * - JSON 格式错误 → warn + skip 该文件（不中断）
 * - BiomeId 对应的 Biome 未在 BiomeRegistry 注册 → warn + skip
 *
 * 必须在 ConfiguredFeatureLoader / PlacedFeatureLoader / ConfiguredCarverLoader 之后调用。
 */
class BiomeLoader {
public:
    /**
     * @brief 从数据包仓库加载所有生物群系 JSON 并叠加到 BiomeRegistry
     * @return 成功叠加的生物群系数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& repo);

    /**
     * @brief 从单个资源包加载所有生物群系 JSON 并叠加到 BiomeRegistry
     * @return 成功叠加的生物群系数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 对象加载单个生物群系并叠加到 BiomeRegistry
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param id 生物群系的 ResourceLocation（对应 JSON 文件名，如 minecraft:plains）
     * @return void 成功，或错误（仅 JSON 严重畸形时返回错误）
     */
    [[nodiscard]] static Result<void> loadFromJson(const nlohmann::json& jsonObj, const ResourceLocation& id);

    /**
     * @brief 按 biome 名（ResourceLocation 的 path，如 "plains"/"the_void"）查 BiomeId
     *
     * 复用本 Loader 内置的 65 项 biome 名→BiomeId 映射表，供其它数据驱动加载器
     * （如 FlatLevelGeneratorPresetLoader 解析 flat preset 的 biome 字段）共用，
     * 避免重复维护映射表。
     *
     * @param name biome 资源位置（仅 path 参与查表，namespace 忽略）
     * @return BiomeId，或 std::nullopt（映射表中无此名）
     */
    [[nodiscard]] static std::optional<BiomeId> biomeIdByName(const ResourceLocation& name);
};

} // namespace world::biome
} // namespace mc
