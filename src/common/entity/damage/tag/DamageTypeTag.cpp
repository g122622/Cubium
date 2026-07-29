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

#include "DamageTypeTag.hpp"

#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// DamageSource::is() 实现（在此处实现以避免循环依赖）
// ============================================================================

bool DamageSource::is(const DamageTypeTag& tag) const
{
    return tag.contains(*this);
}

// ============================================================================
// DamageTypeTag 实现
// ============================================================================

DamageTypeTag::DamageTypeTag(ResourceLocation id) noexcept
    : m_id(std::move(id))
{}

void DamageTypeTag::add(DamageType type)
{
    m_damageTypes.insert(type);
}

void DamageTypeTag::addAll(const std::vector<DamageType>& types)
{
    for (const auto& type : types) {
        m_damageTypes.insert(type);
    }
}

bool DamageTypeTag::addByResourceLocation(const ResourceLocation& typeId)
{
    auto type = DamageTypeNames::fromResourceLocation(typeId);
    if (type.has_value()) {
        m_damageTypes.insert(*type);
        return true;
    }
    return false;
}

bool DamageTypeTag::contains(DamageType type) const noexcept
{
    return m_damageTypes.find(type) != m_damageTypes.end();
}

bool DamageTypeTag::contains(const DamageSource& source) const
{
    return contains(source.type());
}

bool DamageTypeTag::containsByResourceLocation(const ResourceLocation& typeId) const
{
    auto type = DamageTypeNames::fromResourceLocation(typeId);
    if (!type.has_value()) {
        return false;
    }
    return contains(*type);
}

void DamageTypeTag::clear()
{
    m_damageTypes.clear();
}

// ============================================================================
// DamageTypeNames 实现
// ============================================================================

namespace DamageTypeNames {

namespace {
struct DamageTypeEntry {
    DamageType type;
    std::string_view name;
};

// 与 MC 1.21.11 DamageTypes.java 注册表保持一致的完整映射表
// 参考: net.minecraft.world.damagesource.DamageTypes.bootstrap()
constexpr DamageTypeEntry kEntries[] = {
    {DamageType::InFire, "minecraft:in_fire"},
    {DamageType::Campfire, "minecraft:campfire"},
    {DamageType::LightningBolt, "minecraft:lightning_bolt"},
    {DamageType::OnFire, "minecraft:on_fire"},
    {DamageType::Lava, "minecraft:lava"},
    {DamageType::HotFloor, "minecraft:hot_floor"},
    {DamageType::InWall, "minecraft:in_wall"},
    {DamageType::Cramming, "minecraft:cramming"},
    {DamageType::Drown, "minecraft:drown"},
    {DamageType::Starve, "minecraft:starve"},
    {DamageType::Cactus, "minecraft:cactus"},
    {DamageType::Fall, "minecraft:fall"},
    {DamageType::EnderPearl, "minecraft:ender_pearl"},
    {DamageType::FlyIntoWall, "minecraft:fly_into_wall"},
    {DamageType::OutOfWorld, "minecraft:out_of_world"},
    {DamageType::Generic, "minecraft:generic"},
    {DamageType::Magic, "minecraft:magic"},
    {DamageType::Wither, "minecraft:wither"},
    {DamageType::DragonBreath, "minecraft:dragon_breath"},
    {DamageType::Dryout, "minecraft:dry_out"},
    {DamageType::SweetBerryBush, "minecraft:sweet_berry_bush"},
    {DamageType::Freeze, "minecraft:freeze"},
    {DamageType::Stalagmite, "minecraft:stalagmite"},
    {DamageType::FallingBlock, "minecraft:falling_block"},
    {DamageType::FallingAnvil, "minecraft:falling_anvil"},
    {DamageType::FallingStalactite, "minecraft:falling_stalactite"},
    {DamageType::Sting, "minecraft:sting"},
    {DamageType::MobAttack, "minecraft:mob_attack"},
    {DamageType::MobAttackNoAggro, "minecraft:mob_attack_no_aggro"},
    {DamageType::PlayerAttack, "minecraft:player_attack"},
    {DamageType::Spear, "minecraft:spear"},
    {DamageType::Arrow, "minecraft:arrow"},
    {DamageType::Trident, "minecraft:trident"},
    {DamageType::MobProjectile, "minecraft:mob_projectile"},
    {DamageType::Spit, "minecraft:spit"},
    {DamageType::WindBurst, "minecraft:wind_charge"},
    {DamageType::Fireworks, "minecraft:fireworks"},
    {DamageType::UnattributedFireball, "minecraft:unattributed_fireball"},
    {DamageType::Fireball, "minecraft:fireball"},
    {DamageType::WitherSkull, "minecraft:wither_skull"},
    {DamageType::Thrown, "minecraft:thrown"},
    {DamageType::IndirectMagic, "minecraft:indirect_magic"},
    {DamageType::Thorns, "minecraft:thorns"},
    {DamageType::Explosion, "minecraft:explosion"},
    {DamageType::ExplosionPlayer, "minecraft:player_explosion"},
    {DamageType::SonicBoom, "minecraft:sonic_boom"},
    {DamageType::BadRespawnPoint, "minecraft:bad_respawn_point"},
    {DamageType::OutsideBorder, "minecraft:outside_border"},
    {DamageType::GenericKill, "minecraft:generic_kill"},
    {DamageType::MaceSmash, "minecraft:mace_smash"},
};
} // namespace

ResourceLocation getResourceLocation(DamageType type)
{
    for (const auto& entry : kEntries) {
        if (entry.type == type) {
            return ResourceLocation(entry.name);
        }
    }
    spdlog::warn("DamageTypeNames: unknown DamageType enum value: {}", static_cast<int>(type));
    return ResourceLocation();
}

std::optional<DamageType> fromResourceLocation(const ResourceLocation& location)
{
    std::string fullName = location.toString();
    for (const auto& entry : kEntries) {
        if (entry.name == fullName) {
            return entry.type;
        }
    }
    return std::nullopt;
}

std::optional<DamageType> fromString(const std::string& name)
{
    // 若用户传入不带命名空间的名字，自动补 minecraft: 前缀
    std::string fullName;
    if (name.find(':') == std::string::npos) {
        fullName = "minecraft:" + name;
    } else {
        fullName = name;
    }
    for (const auto& entry : kEntries) {
        if (entry.name == fullName) {
            return entry.type;
        }
    }
    return std::nullopt;
}

} // namespace DamageTypeNames

} // namespace mc
