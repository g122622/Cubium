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
 * @file entity_attributes.cpp
 * @brief 差分能力：生物实体的默认属性基础值。
 *
 * `list`：全部生物实体类型 ID
 * `run` ：{ "entity": "minecraft:pig" }
 *         → max_health / movement_speed / follow_range / attack_damage 等基础值（位模式）
 *
 * 这些值决定了生物的满血值、移动快慢、会不会注意到玩家：心数算错、速度差 0.02
 * 在游戏里都看得出来，但单元测试覆盖不到上百种生物，只能与原版逐一比对。
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * 被比对的属性：Cubium 属性名 → 原版 1.21 的键名（去掉了 "generic." 前缀）。
 *
 * 只比 harness 参考表登记过的键，且只收「双方都有明确原版默认值」的项：原版的属性集里
 * 还有 SPAWN_REINFORCEMENTS_CHANCE、ATTACK_KNOCKBACK、JUMP_STRENGTH 的逐生物差异等
 * Cubium 未建模/未逐生物对齐的属性，多导只会制造「实现缺口 vs 值不一致」的噪声。
 */
struct TrackedAttribute {
    const char* cubiumName;
    const char* vanillaKey;
};

constexpr TrackedAttribute TRACKED_ATTRIBUTES[] = {
    {entity::attribute::Attributes::MAX_HEALTH, "max_health"},
    {entity::attribute::Attributes::MOVEMENT_SPEED, "movement_speed"},
    {entity::attribute::Attributes::FOLLOW_RANGE, "follow_range"},
    {entity::attribute::Attributes::ATTACK_DAMAGE, "attack_damage"},
    {entity::attribute::Attributes::ARMOR, "armor"},
    {entity::attribute::Attributes::KNOCKBACK_RESISTANCE, "knockback_resistance"},
    {entity::attribute::Attributes::ARMOR_TOUGHNESS, "armor_toughness"},
};

void ensureEntities()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        entity::VanillaEntities::registerAll();
        s_initialized = true;
    }
}

/**
 * @brief 属性值的位模式，先把值收敛到 f32 精度
 *
 * 原版的属性基础值是 float（如 0.23f），Cubium 内部用 double 存储，直接比较会得到
 * 0.23f 与 0.22999999999999998 这种纯粹的精度差。两侧都按原版口径（float）比较。
 */
std::string f64Bits(f64 value)
{
    const f32 narrowed = static_cast<f32>(value);
    f64 widened = static_cast<f64>(narrowed);
    std::uint64_t bits = 0;
    std::memcpy(&bits, &widened, sizeof(bits));
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(bits));
    return buffer;
}

std::vector<std::string> livingEntityNames()
{
    std::vector<std::string> names;
    for (const entity::EntityType* type : entity::EntityRegistry::instance().getAllTypes()) {
        names.push_back(type->name());
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

void writeList(FILE* out)
{
    ensureEntities();
    for (const auto& name : livingEntityNames()) {
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

    // 属性挂在实体实例上（构造时 registerAttributes() 填充），所以必须真正创建一个。
    ecs::EntityRegistry ecsRegistry("diff-entity-attributes");
    std::unique_ptr<Entity> instance = type->create(nullptr, ecsRegistry);
    auto* living = dynamic_cast<LivingEntity*>(instance.get());
    if (living == nullptr) {
        return json{{"entity", requested}, {"notLiving", true}}.dump();
    }

    json attributes = json::object();
    for (const auto& tracked : TRACKED_ATTRIBUTES) {
        const entity::attribute::AttributeInstance* attribute =
            living->attributes().getInstance(tracked.cubiumName);
        if (attribute == nullptr) {
            continue;
        }
        // 1.21 起原版属性 ID 去掉了 "generic." 前缀（minecraft:max_health），Cubium 内部
        // 沿用的是 1.20 的 "generic.max_health"；比较前统一成原版命名。
        attributes[std::string("minecraft:") + tracked.vanillaKey] = f64Bits(attribute->baseValue());
    }

    return json{{"entity", requested}, {"attributes", std::move(attributes)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("entity_attributes", &writeList, &runLine)

} // namespace mc::diff