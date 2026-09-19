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

#include "JavaAttributeIdMap.hpp"

#include "common/entity/attribute/Attributes.hpp"

#include <spdlog/spdlog.h>

#include <cstddef>
#include <iterator>
#include <string>

namespace mc::network::backend::java {

namespace {

/**
 * vanilla attribute 的 registry 项。
 *
 * `internalName` 为 `nullptr` 表示项目尚未实现该属性——它仍占一个 registry id（顺序由 vanilla
 * 的静态字段声明顺序固定，不可变），但不能被下发。
 */
struct VanillaAttributeEntry {
    u32 vanillaId;
    const char* vanillaPath;
    const char* internalName;
    bool clientSyncable;
};

/// vanilla 1.21.11 Attributes.java 静态字段声明顺序（wire id 权威源，0-based）。
/// clientSyncable 对应 vanilla 的 `RangedAttribute#setSyncable`，为假者不应下发给客户端。
constexpr VanillaAttributeEntry kVanillaAttributes[] = {
    {0, "armor", ::mc::entity::attribute::Attributes::ARMOR, true},
    {1, "armor_toughness", ::mc::entity::attribute::Attributes::ARMOR_TOUGHNESS, true},
    {2, "attack_damage", ::mc::entity::attribute::Attributes::ATTACK_DAMAGE, false},
    {3, "attack_knockback", ::mc::entity::attribute::Attributes::ATTACK_KNOCKBACK, false},
    {4, "attack_speed", ::mc::entity::attribute::Attributes::ATTACK_SPEED, true},
    {5, "block_break_speed", nullptr, true},
    {6, "block_interaction_range", ::mc::entity::attribute::Attributes::BLOCK_INTERACTION_RANGE, true},
    {7, "burning_time", ::mc::entity::attribute::Attributes::BURNING_TIME, true},
    {8, "camera_distance", nullptr, true},
    {9, "explosion_knockback_resistance",
        ::mc::entity::attribute::Attributes::EXPLOSION_KNOCKBACK_RESISTANCE,
        true},
    {10, "entity_interaction_range", ::mc::entity::attribute::Attributes::ENTITY_INTERACTION_RANGE, true},
    {11, "fall_damage_multiplier", ::mc::entity::attribute::Attributes::FALL_DAMAGE_MULTIPLIER, true},
    {12, "flying_speed", ::mc::entity::attribute::Attributes::FLYING_SPEED, true},
    {13, "follow_range", ::mc::entity::attribute::Attributes::FOLLOW_RANGE, false},
    // 项目的重力属性沿用 Forge 扩展名 forge.entity_gravity，语义与 vanilla 的 gravity 相同。
    {14, "gravity", ::mc::entity::attribute::Attributes::ENTITY_GRAVITY, true},
    {15, "jump_strength", ::mc::entity::attribute::Attributes::JUMP_STRENGTH, true},
    {16, "knockback_resistance", ::mc::entity::attribute::Attributes::KNOCKBACK_RESISTANCE, false},
    {17, "luck", ::mc::entity::attribute::Attributes::LUCK, true},
    {18, "max_absorption", ::mc::entity::attribute::Attributes::MAX_ABSORPTION, true},
    {19, "max_health", ::mc::entity::attribute::Attributes::MAX_HEALTH, true},
    {20, "mining_efficiency", nullptr, true},
    {21, "movement_efficiency", ::mc::entity::attribute::Attributes::MOVEMENT_EFFICIENCY, true},
    {22, "movement_speed", ::mc::entity::attribute::Attributes::MOVEMENT_SPEED, true},
    {23, "oxygen_bonus", ::mc::entity::attribute::Attributes::OXYGEN_BONUS, true},
    {24, "safe_fall_distance", ::mc::entity::attribute::Attributes::SAFE_FALL_DISTANCE, true},
    {25, "scale", nullptr, true},
    {26, "sneaking_speed", nullptr, true},
    {27, "spawn_reinforcements", ::mc::entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, false},
    {28, "step_height", nullptr, true},
    {29, "submerged_mining_speed", nullptr, true},
    {30, "sweeping_damage_ratio", nullptr, true},
    {31, "tempt_range", nullptr, false},
    {32, "water_movement_efficiency", nullptr, true},
    {33, "waypoint_transmit_range", nullptr, false},
    {34, "waypoint_receive_range", nullptr, false},
};

inline constexpr size_t kVanillaAttributeCount = std::size(kVanillaAttributes);

/// 查询未命中时的共享空串（避免返回悬垂引用）。
const std::string kEmptyName{};

} // namespace

// ============================================================================
// 单例
// ============================================================================

JavaAttributeIdMap& JavaAttributeIdMap::instance()
{
    static JavaAttributeIdMap s_instance;
    return s_instance;
}

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaAttributeIdMap::initialize()
{
    m_initialized = false;
    m_internalToJava.clear();
    m_javaToInternal.clear();
    m_javaSyncable.clear();

    for (size_t i = 0; i < kVanillaAttributeCount; ++i) {
        const auto& entry = kVanillaAttributes[i];
        m_javaSyncable[entry.vanillaId] = entry.clientSyncable;
        if (entry.internalName == nullptr) {
            continue;
        }
        // 同名冲突会静默覆盖，故显式拒绝——两张表任一出现重复都说明常量写错了。
        if (m_internalToJava.find(entry.internalName) != m_internalToJava.end()
            || m_javaToInternal.find(entry.vanillaId) != m_javaToInternal.end()) {
            return Error(ErrorCode::InvalidArgument,
                "JavaAttributeIdMap: duplicate mapping for '" + std::string(entry.internalName) + "'",
                "JavaAttributeIdMap::initialize");
        }
        m_internalToJava.emplace(entry.internalName, entry.vanillaId);
        m_javaToInternal.emplace(entry.vanillaId, entry.internalName);
    }

    m_initialized = true;
    spdlog::info("JavaAttributeIdMap: mapped {} attributes (vanilla total {})",
        m_internalToJava.size(),
        kVanillaAttributeCount);
    return Result<void>::ok();
}

std::optional<u32> JavaAttributeIdMap::toJavaRegistryId(const std::string& attributeName) const
{
    const auto it = m_internalToJava.find(attributeName);
    if (it == m_internalToJava.end()) {
        return std::nullopt;
    }
    return it->second;
}

const std::string& JavaAttributeIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    const auto it = m_javaToInternal.find(javaRegistryId);
    if (it == m_javaToInternal.end()) {
        return kEmptyName;
    }
    return it->second;
}

bool JavaAttributeIdMap::isImplemented(const std::string& attributeName) const
{
    return m_internalToJava.find(attributeName) != m_internalToJava.end();
}

bool JavaAttributeIdMap::isClientSyncable(u32 javaRegistryId) const
{
    const auto it = m_javaSyncable.find(javaRegistryId);
    if (it == m_javaSyncable.end()) {
        return false;
    }
    // 项目未实现该属性时无从取值，即使 vanilla 侧可同步也不能下发。
    if (m_javaToInternal.find(javaRegistryId) == m_javaToInternal.end()) {
        return false;
    }
    return it->second;
}

} // namespace mc::network::backend::java
