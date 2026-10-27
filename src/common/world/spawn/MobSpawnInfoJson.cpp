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

#include "MobSpawnInfoJson.hpp"

#include "common/core/Result.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"

#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

namespace mc::world::spawn {

namespace {

using entity::EntityClassification;

/// 分类 → 生成列表的写入函数（与 {@link MobSpawnInfo::getSpawns} 对称）
void _addSpawn(MobSpawnInfo& info, EntityClassification category, const SpawnEntry& entry)
{
    switch (category) {
        case EntityClassification::Monster:
            info.addMonsterSpawn(entry);
            break;
        case EntityClassification::Creature:
            info.addCreatureSpawn(entry);
            break;
        case EntityClassification::Ambient:
            info.addAmbientSpawn(entry);
            break;
        case EntityClassification::Axolotls:
            info.addAxolotlSpawn(entry);
            break;
        case EntityClassification::UndergroundWaterCreature:
            info.addUndergroundWaterCreatureSpawn(entry);
            break;
        case EntityClassification::WaterCreature:
            info.addWaterCreatureSpawn(entry);
            break;
        case EntityClassification::WaterAmbient:
            info.addWaterAmbientSpawn(entry);
            break;
        case EntityClassification::Misc:
            info.addMiscSpawn(entry);
            break;
    }
}

/// 读一个非负浮点字段（缺省时返回 fallback）
Result<f64> _readNonNegative(const nlohmann::json& object, const char* key, f64 fallback)
{
    if (!object.contains(key)) {
        return fallback;
    }
    const auto& value = object[key];
    if (!value.is_number()) {
        return Error(ErrorCode::InvalidData, std::string("'") + key + "' must be a number");
    }
    const f64 number = value.get<f64>();
    if (number < 0.0) {
        return Error(ErrorCode::InvalidData, std::string("'") + key + "' must be non-negative");
    }
    return number;
}

} // namespace

std::optional<EntityClassification> spawnCategoryByName(std::string_view name)
{
    if (name == "monster") {
        return EntityClassification::Monster;
    }
    if (name == "creature") {
        return EntityClassification::Creature;
    }
    if (name == "ambient") {
        return EntityClassification::Ambient;
    }
    if (name == "axolotls") {
        return EntityClassification::Axolotls;
    }
    if (name == "underground_water_creature") {
        return EntityClassification::UndergroundWaterCreature;
    }
    if (name == "water_creature") {
        return EntityClassification::WaterCreature;
    }
    if (name == "water_ambient") {
        return EntityClassification::WaterAmbient;
    }
    if (name == "misc") {
        return EntityClassification::Misc;
    }
    return std::nullopt;
}

std::string_view spawnCategoryName(EntityClassification category)
{
    switch (category) {
        case EntityClassification::Monster:
            return "monster";
        case EntityClassification::Creature:
            return "creature";
        case EntityClassification::Ambient:
            return "ambient";
        case EntityClassification::Axolotls:
            return "axolotls";
        case EntityClassification::UndergroundWaterCreature:
            return "underground_water_creature";
        case EntityClassification::WaterCreature:
            return "water_creature";
        case EntityClassification::WaterAmbient:
            return "water_ambient";
        case EntityClassification::Misc:
            return "misc";
    }
    return "misc";
}

Result<SpawnEntry> spawnEntryFromJson(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "spawner entry must be an object");
    }
    if (!json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "spawner entry missing string 'type'");
    }
    const std::string type = json["type"].get<std::string>();
    if (entity::EntityRegistry::instance().getType(type) == nullptr) {
        return Error(ErrorCode::InvalidData, "spawner entry references unknown entity type '" + type + "'");
    }

    // 1.21.11 的字段名是 camelCase（minCount / maxCount），缺省 1 / 4。
    i32 weight = 1;
    i32 minCount = 1;
    i32 maxCount = 4;
    if (json.contains("weight")) {
        if (!json["weight"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "spawner entry 'weight' must be an integer");
        }
        weight = json["weight"].get<i32>();
    }
    if (json.contains("minCount")) {
        if (!json["minCount"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "spawner entry 'minCount' must be an integer");
        }
        minCount = json["minCount"].get<i32>();
    }
    if (json.contains("maxCount")) {
        if (!json["maxCount"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "spawner entry 'maxCount' must be an integer");
        }
        maxCount = json["maxCount"].get<i32>();
    }

    if (weight <= 0) {
        return Error(ErrorCode::InvalidData, "spawner entry '" + type + "' has non-positive weight");
    }
    if (minCount < 0) {
        return Error(ErrorCode::InvalidData, "spawner entry '" + type + "' has negative minCount");
    }
    if (minCount > maxCount) {
        return Error(ErrorCode::InvalidData, "spawner entry '" + type + "' has minCount > maxCount");
    }
    return SpawnEntry(type, weight, minCount, maxCount);
}

Result<MobSpawnInfo> mobSpawnInfoFromJson(const nlohmann::json& spawners,
    const std::optional<nlohmann::json>& creatureSpawnProbability,
    f32 fallbackProbability)
{
    // 每区块上限是 MobCategory 的静态属性，不是群系 JSON 字段；由分类表统一提供，
    // 避免这里再抄一份常量。
    MobSpawnInfo info;
    info.setMaxMonsterInstances(entity::getMaxCount(EntityClassification::Monster));
    info.setMaxCreatureInstances(entity::getMaxCount(EntityClassification::Creature));
    info.setMaxAmbientInstances(entity::getMaxCount(EntityClassification::Ambient));
    info.setMaxAxolotlInstances(entity::getMaxCount(EntityClassification::Axolotls));
    info.setMaxUndergroundWaterCreatureInstances(entity::getMaxCount(EntityClassification::UndergroundWaterCreature));
    info.setMaxWaterCreatureInstances(entity::getMaxCount(EntityClassification::WaterCreature));
    info.setMaxWaterAmbientInstances(entity::getMaxCount(EntityClassification::WaterAmbient));

    f32 probability = fallbackProbability;
    if (creatureSpawnProbability.has_value()) {
        const auto& value = *creatureSpawnProbability;
        if (!value.is_number()) {
            return Error(ErrorCode::InvalidData, "'creature_spawn_probability' must be a number");
        }
        const f64 number = value.get<f64>();
        if (number < 0.0 || number > 1.0) {
            return Error(ErrorCode::InvalidData, "'creature_spawn_probability' must be within [0, 1]");
        }
        probability = static_cast<f32>(number);
    }
    info.setCreatureSpawnProbability(probability);

    if (spawners.is_null()) {
        return info;
    }
    if (!spawners.is_object()) {
        return Error(ErrorCode::InvalidData, "'spawners' must be an object");
    }
    for (const auto& [categoryName, entries] : spawners.items()) {
        const auto category = spawnCategoryByName(categoryName);
        if (!category.has_value()) {
            return Error(ErrorCode::InvalidData, "unknown spawner category '" + categoryName + "'");
        }
        if (!entries.is_array()) {
            return Error(ErrorCode::InvalidData, "spawner category '" + categoryName + "' must be an array");
        }
        for (const auto& entryJson : entries) {
            auto entry = spawnEntryFromJson(entryJson);
            if (entry.failed()) {
                return entry.error();
            }
            _addSpawn(info, *category, entry.value());
        }
    }
    return info;
}

Result<void> applySpawnCosts(MobSpawnInfo& info, const nlohmann::json& spawnCosts)
{
    if (spawnCosts.is_null()) {
        return {};
    }
    if (!spawnCosts.is_object()) {
        return Error(ErrorCode::InvalidData, "'spawn_costs' must be an object");
    }
    for (const auto& [entityId, value] : spawnCosts.items()) {
        if (!value.is_object()) {
            return Error(ErrorCode::InvalidData, "spawn cost '" + entityId + "' must be an object");
        }
        auto charge = _readNonNegative(value, "charge", 0.0);
        if (charge.failed()) {
            return charge.error();
        }
        auto budget = _readNonNegative(value, "energy_budget", 1.0);
        if (budget.failed()) {
            return budget.error();
        }
        info.setSpawnCost(entityId, SpawnCosts(budget.value(), charge.value()));
    }
    return {};
}

} // namespace mc::world::spawn
