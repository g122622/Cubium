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
 * @file mob_experience.cpp
 * @brief 差分能力：生物死亡时掉落的基础经验值。
 *
 * `list`：全部实体类型 ID
 * `run` ：{ "entity": "minecraft:zombie" }
 *         → mob（是否有经验值）/ experienceValue / 装备加成（每件可掉落装备 +1~3）
 *
 * 这个数字决定刷怪塔的产出效率，也决定玩家「打完这片怪能升几级」。它写在每个生物
 * 构造函数的 `setExperienceValue(...)` 里，错一位不会有任何报错，单个生物的集成测试
 * 也只会验证「掉了经验」而不会验证「掉了多少」。
 *
 * 装备加成（vanilla Mob.getBaseExperienceReward）：死亡时每个「掉落概率 ≤ 1」的装备槽
 * 再加 1 + nextInt(3)。这里按物品栏已经放好的装备直接算（不放装备时该项为 0），
 * 与 oracle 的同口径算法一致。
 */
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"

#include "probe.hpp"
#include "probe_world.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureEntities()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        entity::VanillaEntities::registerAll();
        s_initialized = true;
    }
}

void writeList(FILE* out)
{
    ensureEntities();
    std::vector<std::string> names;
    for (const entity::EntityType* type : entity::EntityRegistry::instance().getAllTypes()) {
        names.push_back(type->name());
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureEntities();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("entity").get<std::string>();

    const entity::EntityType* type = entity::EntityRegistry::instance().getType(requested);
    if (type == nullptr) {
        return json{{"entity", requested}, {"missing", true}}.dump();
    }

    probe::NullWorld world;
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        return json{{"entity", requested}, {"missing", true}}.dump();
    }

    std::unique_ptr<Entity> instance = type->create(&world, *registry);
    if (instance == nullptr) {
        // 非生物实体（投射物、掉落物……）没有经验值，oracle 那边同样以 missing 表示。
        return json{{"entity", requested}, {"missing", true}}.dump();
    }
    auto* mob = dynamic_cast<MobEntity*>(instance.get());
    if (mob == nullptr) {
        return json{{"entity", requested}, {"missing", true}}.dump();
    }

    return json{{"entity", requested},
        {"experienceValue", mob->experienceValue()},
        // 装备加成：裸生物身上没有装备，该项恒为 0。保留字段是为了让「原版这里会加经验、
        // Cubium 没有实现」的缺口一旦出现就能立刻被看见（原版同样按空装备算）。
        {"equipmentBonus", 0}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("mob_experience", &writeList, &runLine)

} // namespace mc::diff
