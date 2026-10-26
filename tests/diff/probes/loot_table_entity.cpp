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

/**
 * @file loot_table_entity.cpp
 * @brief 差分能力：生物类战利品表在「击杀」上下文下的抽取结果。
 *
 * `list`：数据包里全部战利品表 ID
 * `run` ：{ "table": "minecraft:entities/zombie", "entity": "minecraft:zombie",
 *           "attacker": "minecraft:player", "looting": 3, "onFire": false }
 *         → 4 次掷骰的 `[{id, count}, ...]`
 *
 * 生物掉落表的条件集中在 `entity_properties`（着火、装备、主手附魔）、
 * `killed_by_player` 与 `enchanted_count_increase`（掠夺）。两侧都构造同类型的
 * `this_entity` 与 `attacking_entity`，只差夺掠等级与着火状态。
 */
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootCallbackInvoker.hpp"

#include "loot_probe_common.hpp"
#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace lootprobe;

void writeList(FILE* out)
{
    for (const auto& id : environment().tableIds) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

/**
 * @brief 按名字构造一个实体；找不到类型时返回空。
 *
 * 实体的位置固定为 (0.5, 64, 0.5)：两侧坐标一致，`origin` 与位置检查才不会引入噪声。
 */
std::unique_ptr<Entity> createNamed(const json& input, const char* key)
{
    if (!input.contains(key)) {
        return nullptr;
    }
    return lootworld::createEntity(*environment().world, input.at(key).get<std::string>());
}

std::string runLine(std::string_view inputLine)
{
    auto& env = environment();
    const json input = json::parse(inputLine);
    const std::string id = input.at("table").get<std::string>();

    const loot::LootTable* table = env.tables.getTable(id);
    if (table == nullptr) {
        return json{{"table", id}, {"missing", true}}.dump();
    }

    // 没指定类型就用一个固定实体（猪）：entity 参数集的掉落表只关心「有没有实体」。
    std::unique_ptr<Entity> thisEntity = input.contains("entity")
        ? createNamed(input, "entity")
        : lootworld::createEntity(*env.world, "minecraft:pig");
    if (thisEntity == nullptr) {
        // 实体类型没注册（或注册表为空）：与「表不存在」区分开，方便定位是数据包问题
        // 还是实体类型缺失。
        const std::string name = input.contains("entity") ? input.at("entity").get<std::string>() : "minecraft:pig";
        std::fprintf(stderr, "loot_table_entity: unknown entity type '%s' for table '%s'\n", name.c_str(), id.c_str());
        return json{{"table", id}, {"missing", true}}.dump();
    }

    std::unique_ptr<Entity> attacker = createNamed(input, "attacker");
    const auto origin = parseOrigin(input);
    const i32 looting = input.value("looting", 0);
    const bool onFire = input.value("onFire", false);

    // 掠夺等级必须真实地"写在攻击者的主手武器上"：原版 `enchanted_count_increase`
    // 与 `random_chance_with_enchanted_bonus` 都从 ATTACKING_ENTITY 的装备读附魔，
    // 没有独立的「掠夺等级」上下文键。oracle 侧同样这么做（LootTableEntityOracle#equipLooting），
    // 两侧形态必须一致。
    if (attacker != nullptr) {
        loot::equipLooting(*attacker, looting);
    }

    if (onFire) {
        thisEntity->setRemainingFireTicks(100);
    }

    // 原版 ENTITY 参数集把 damage_source 列为必需键；这里用固定的魔法伤害，
    // 与两侧共享同一份「非特定击杀来源」的判定条件。
    EnvironmentalDamage damageSource = DamageSources::magic();

    // 原版 `LootParams.Builder#create(paramSet)` 会校验必需参数，缺了就抛
    // `IllegalArgumentException`；oracle 捕获后按「该样本无产出」处理，但**样本的随机源
    // 已经创建并推进过，后续样本的随机序列不变**。Cubium 的 `build()` 只告警，所以这里
    // 先按同一规则判一次，参数集不满足时直接产出空样本（不掷骰，随机源也不额外前进）。
    std::vector<std::string> providedParams = {
        loot::LootParams::THIS_ENTITY.getId(),
        loot::LootParams::DAMAGE_SOURCE.getId(),
        loot::LootParams::ORIGIN.getId(),
    };
    if (attacker != nullptr) {
        providedParams.push_back(loot::LootParams::KILLER_ENTITY.getId());
        if (attacker->entityType() != nullptr && attacker->entityType()->name() == "minecraft:player") {
            providedParams.push_back(loot::LootParams::KILLER_PLAYER.getId());
        }
    }
    if (!table->getParameterSet().validate(providedParams)) {
        json emptySamples = json::array();
        for (int sample = 0; sample < ROLL_SAMPLES; ++sample) {
            emptySamples.push_back(json::array());
        }
        return json{{"table", id}, {"samples", std::move(emptySamples)}}.dump();
    }

    json samples = rollSamples(*table, *env.world, table->getParameterSet(), [&](loot::LootContextBuilder& builder) {
        builder.withParameter(loot::LootParams::THIS_ENTITY, thisEntity.get());
        builder.withParameter(loot::LootParams::DAMAGE_SOURCE, static_cast<DamageSource*>(&damageSource));
        const Vector3 ownedOrigin = origin;
        builder.withOwnedValue(loot::LootParams::ORIGIN, ownedOrigin);
        if (attacker != nullptr) {
            builder.withParameter(loot::LootParams::KILLER_ENTITY, attacker.get());
        }
        if (attacker != nullptr && attacker->entityType() != nullptr &&
            attacker->entityType()->name() == "minecraft:player") {
            builder.withParameter(loot::LootParams::KILLER_PLAYER, static_cast<Player*>(attacker.get()));
        }
    });
    return json{{"table", id}, {"samples", std::move(samples)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE_WITH_SHUTDOWN("loot_table_entity", &writeList, &runLine, &lootprobe::releaseEnvironment)

} // namespace mc::diff