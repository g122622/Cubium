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

/**
 * @file MobSpawnInfoJson.hpp
 * @brief 生物群系 JSON 里与「生成」相关的字段的 Codec（对齐 vanilla `MobSpawnSettings`）
 *
 * 数据包字段：
 * - `spawners`：{ "<category>": [{"type", "weight", "minCount", "maxCount"}, ...] }
 * - `spawn_costs`：{ "<entity id>": {"charge", "energy_budget"} }
 * - `creature_spawn_probability`：顶层浮点数（缺省沿用 `BiomeFactory` 的设定）
 *
 * 解码放在 common 层：服务端的 `BiomeLoader` 与差分探针读的是同一份实现。
 */

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"

#include <optional>
#include <string_view>

#include <nlohmann/json_fwd.hpp>

namespace mc::world::spawn {

/**
 * @brief 生成分类名（`monster` / `creature` / …）→ 实体分类
 *
 * @param name 数据包 `spawners` 对象的键（不带命名空间）
 * @return 实体分类；未知名字返回 nullopt
 */
[[nodiscard]] std::optional<entity::EntityClassification> spawnCategoryByName(std::string_view name);

/**
 * @brief 实体分类 → 数据包 `spawners` 对象的键
 */
[[nodiscard]] std::string_view spawnCategoryName(entity::EntityClassification category);

/**
 * @brief 解析单个生成条目
 *
 * 校验与原版 `MobSpawnSettings.SpawnerData` 一致：权重必须为正、最小数量非负且不大于
 * 最大数量、实体类型必须已注册（未注册的条目原版直接解码失败，这里同样返回错误）。
 */
[[nodiscard]] Result<SpawnEntry> spawnEntryFromJson(const nlohmann::json& json);

/**
 * @brief 把数据包的 `spawners` 对象解析成完整的 {@link MobSpawnInfo}
 *
 * 每区块上限与是否适合玩家生成由 {@link MobSpawnInfo} 的默认常量决定；
 * `creatureSpawnProbability` 是数据包顶层字段，缺省时沿用 `fallbackProbability`
 * （`BiomeFactory` 为少数群系设了非默认值，如雪原的 0.07）。
 *
 * @param spawners `spawners` 对象（缺省时传 null，结果为空生成表）
 * @param creatureSpawnProbability `creature_spawn_probability` 字段（缺省时传 nullopt）
 * @param fallbackProbability 顶层字段缺省时使用的概率
 */
[[nodiscard]] Result<MobSpawnInfo> mobSpawnInfoFromJson(const nlohmann::json& spawners,
    const std::optional<nlohmann::json>& creatureSpawnProbability,
    f32 fallbackProbability);

/**
 * @brief 把 `spawn_costs` 对象并入生成信息
 *
 * 每条 `{"charge": >=0 浮点, "energy_budget": >=0 浮点}`。与原版 `MobSpawnSettings.CODEC`
 * 一致：`charge` 缺省为 0、`energy_budget` 缺省为 1，两者都必须非负，否则整体失败。
 */
[[nodiscard]] Result<void> applySpawnCosts(MobSpawnInfo& info, const nlohmann::json& spawnCosts);

} // namespace mc::world::spawn
