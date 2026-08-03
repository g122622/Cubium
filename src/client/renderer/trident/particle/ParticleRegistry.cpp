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

#include "ParticleRegistry.hpp"
#include "Particle.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "particles/special/NautilusParticle.hpp"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::renderer::trident::particle {

ParticleRegistry& ParticleRegistry::instance()
{
    static ParticleRegistry instance;
    return instance;
}

ParticleRegistry::ParticleRegistry()
{
    _registerBuiltinTypes();
}

void ParticleRegistry::registerType(ParticleTypeId id,
    const std::string& name,
    ParticleFactory factory,
    ParticleRenderType defaultRenderType,
    f64 defaultLifetime,
    bool hasPhysics,
    bool ignoreDistance,
    bool overrideLimiter)
{

    MC_ASSERT_RELEASE_MSG(isValidParticleType(id), "Invalid particle type ID");
    // factory 可以为 nullptr，用于仅注册元数据的情况
    MC_ASSERT_RELEASE_MSG(!name.empty(), "Particle name cannot be empty");

    ParticleTypeInfo info;
    info.id = id;
    info.name = name;
    info.factory = std::move(factory);
    info.defaultRenderType = defaultRenderType;
    info.defaultLifetime = defaultLifetime;
    info.hasPhysics = hasPhysics;
    info.ignoreDistance = ignoreDistance;
    info.overrideLimiter = overrideLimiter;

    m_types[id] = std::move(info);
    m_nameToId[name] = id;
}

void ParticleRegistry::registerSimpleType(ParticleTypeId id,
    const std::string& name,
    ParticleFactory factory,
    ParticleRenderType defaultRenderType,
    bool overrideLimiter)
{

    registerType(id, name, std::move(factory), defaultRenderType, 1.0, true, false, overrideLimiter);
}

std::unique_ptr<Particle> ParticleRegistry::createParticle(
    ParticleTypeId id, const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world) const
{

    auto it = m_types.find(id);
    if (it == m_types.end() || !it->second.factory) {
        return nullptr;
    }

    return it->second.factory(pos, velocity, world);
}

std::unique_ptr<Particle> ParticleRegistry::createParticle(
    const std::string& name, const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world) const
{

    auto id = getTypeId(name);
    if (!id.has_value()) {
        return nullptr;
    }

    return createParticle(id.value(), pos, velocity, world);
}

void ParticleRegistry::registerDataFactory(ParticleTypeId id, ParticleDataFactory dataFactory)
{
    MC_ASSERT_RELEASE_MSG(isValidParticleType(id), "Invalid particle type ID");

    auto it = m_types.find(id);
    if (it == m_types.end()) {
        // 类型尚未注册，无法设置数据工厂
        return;
    }

    it->second.dataFactory = std::move(dataFactory);
}

std::unique_ptr<Particle> ParticleRegistry::createParticleWithData(ParticleTypeId id,
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world,
    const data::ParticleData* data) const
{
    auto it = m_types.find(id);
    if (it == m_types.end()) {
        return nullptr;
    }

    // 如果有数据工厂且提供了数据，使用数据工厂
    if (data && it->second.dataFactory) {
        return it->second.dataFactory(pos, velocity, world, data);
    }

    // 回退到普通工厂
    if (!it->second.factory) {
        return nullptr;
    }

    return it->second.factory(pos, velocity, world);
}

std::optional<ParticleTypeId> ParticleRegistry::getTypeId(const std::string& name) const
{
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ParticleTypeId> ParticleRegistry::getTypeId(const ResourceLocation& location) const
{
    return getTypeId(location.toString());
}

const std::string& ParticleRegistry::getTypeName(ParticleTypeId id) const
{
    auto it = m_types.find(id);
    if (it != m_types.end()) {
        return it->second.name;
    }
    return m_invalidTypeName;
}

const ParticleTypeInfo* ParticleRegistry::getTypeInfo(ParticleTypeId id) const
{
    auto it = m_types.find(id);
    if (it != m_types.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ParticleRegistry::isRegistered(ParticleTypeId id) const
{
    return m_types.find(id) != m_types.end();
}

bool ParticleRegistry::isRegistered(const std::string& name) const
{
    return m_nameToId.find(name) != m_nameToId.end();
}

std::vector<ParticleTypeId> ParticleRegistry::getAllTypeIds() const
{
    std::vector<ParticleTypeId> ids;
    ids.reserve(m_types.size());
    for (const auto& pair : m_types) {
        ids.push_back(pair.first);
    }
    return ids;
}

void ParticleRegistry::_registerBuiltinTypes()
{
    // 注意：这里只注册类型元数据，不注册工厂函数
    // 工厂函数在具体粒子类型实现后注册
    // 枚举值与 MC Java 1.21.11 协议 ID 一致（0~114）
    // 115~123 为项目内部扩展粒子，不参与网络通信

    // 方块类粒子 (0-2)
    registerSimpleType(ParticleTypeId::AngryVillager,
        "minecraft:angry_villager",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Block, "minecraft:block", ParticleFactory{}, ParticleRenderType::TERRAIN_SHEET);
    // BlockMarker: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::BlockMarker,
        "minecraft:block_marker",
        ParticleFactory{},
        ParticleRenderType::TERRAIN_SHEET,
        true);

    // 环境类粒子 (3-8)
    registerSimpleType(
        ParticleTypeId::Bubble, "minecraft:bubble", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Cloud, "minecraft:cloud", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::CopperFireFlame,
        "minecraft:copper_fire_flame",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(
        ParticleTypeId::Crit, "minecraft:crit", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    // DamageIndicator: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::DamageIndicator,
        "minecraft:damage_indicator",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        true);
    registerSimpleType(ParticleTypeId::DragonBreath,
        "minecraft:dragon_breath",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 液体滴落类粒子 (9-13)
    registerSimpleType(ParticleTypeId::DrippingLava,
        "minecraft:dripping_lava",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::FallingLava,
        "minecraft:falling_lava",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::LandingLava,
        "minecraft:landing_lava",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::DrippingWater,
        "minecraft:dripping_water",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingWater,
        "minecraft:falling_water",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 染色粒子 (14-15)
    registerSimpleType(
        ParticleTypeId::Dust, "minecraft:dust", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::DustColorTransition,
        "minecraft:dust_color_transition",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);

    // 效果类粒子 (16-28)
    registerSimpleType(
        ParticleTypeId::Spell, "minecraft:spell", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    // ElderGuardian: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::ElderGuardian,
        "minecraft:elder_guardian",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(ParticleTypeId::EnchantedHit,
        "minecraft:enchanted_hit",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Enchant,
        "minecraft:enchant",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::EndRod, "minecraft:end_rod", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::EntityEffect,
        "minecraft:entity_effect",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    // HugeExplosion: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::HugeExplosion,
        "minecraft:explosion_emitter",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT,
        true);
    // Explosion: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::Explosion,
        "minecraft:explosion",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // Gust: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::Gust,
        "minecraft:gust",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(ParticleTypeId::SmallGust,
        "minecraft:small_gust",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    // GustEmitterLarge: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::GustEmitterLarge,
        "minecraft:gust_emitter_large",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // GustEmitterSmall: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::GustEmitterSmall,
        "minecraft:gust_emitter_small",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // SonicBoom: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::SonicBoom,
        "minecraft:sonic_boom",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);

    // 方块/物品/烟花粒子 (29-31)
    registerSimpleType(
        ParticleTypeId::FallingDust, "minecraft:falling_dust", ParticleFactory{}, ParticleRenderType::TERRAIN_SHEET);
    registerSimpleType(ParticleTypeId::Firework,
        "minecraft:firework",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Fishing,
        "minecraft:fishing",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 火焰/效果粒子 (32-52)
    registerSimpleType(
        ParticleTypeId::Flame, "minecraft:flame", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Infested,
        "minecraft:infested",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::CherryLeaves,
        "minecraft:cherry_leaves",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::PaleOakLeaves,
        "minecraft:pale_oak_leaves",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::TintedLeaves,
        "minecraft:tinted_leaves",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::SculkSoul, "minecraft:sculk_soul", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    // SculkCharge: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::SculkCharge,
        "minecraft:sculk_charge",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // SculkChargePop: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::SculkChargePop,
        "minecraft:sculk_charge_pop",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(ParticleTypeId::SoulFireFlame,
        "minecraft:soul_fire_flame",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(
        ParticleTypeId::Soul, "minecraft:soul", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Flash, "minecraft:flash", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::HappyVillager,
        "minecraft:happy_villager",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Composter,
        "minecraft:composter",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Heart, "minecraft:heart", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::InstantSpell,
        "minecraft:instant_spell",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Item, "minecraft:item", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    // Vibration: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::Vibration,
        "minecraft:vibration",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(
        ParticleTypeId::Trail, "minecraft:trail", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    registerSimpleType(ParticleTypeId::ItemSlime,
        "minecraft:item_slime",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::ItemCobweb,
        "minecraft:item_cobweb",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::ItemSnowball,
        "minecraft:item_snowball",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 烟雾/天气/生物粒子 (53-69)
    registerSimpleType(ParticleTypeId::LargeSmoke,
        "minecraft:large_smoke",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Lava, "minecraft:lava", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(
        ParticleTypeId::Mycelium, "minecraft:mycelium", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    registerSimpleType(
        ParticleTypeId::Note, "minecraft:note", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    // Poof: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::Poof,
        "minecraft:poof",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(
        ParticleTypeId::Portal, "minecraft:portal", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Rain, "minecraft:rain", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Smoke, "minecraft:smoke", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::WhiteSmoke,
        "minecraft:white_smoke",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    registerSimpleType(
        ParticleTypeId::Sneeze, "minecraft:sneeze", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    // Spit: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::Spit,
        "minecraft:spit",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // SquidInk: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::SquidInk,
        "minecraft:squid_ink",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // SweepAttack: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::SweepAttack,
        "minecraft:sweep_attack",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT,
        true);
    registerSimpleType(ParticleTypeId::TotemOfUndying,
        "minecraft:totem_of_undying",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Underwater,
        "minecraft:underwater",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Splash, "minecraft:splash", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Witch, "minecraft:witch", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 水下/营地/蜂蜜粒子 (70-79)
    registerSimpleType(ParticleTypeId::BubblePop,
        "minecraft:bubble_pop",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    registerSimpleType(ParticleTypeId::CurrentDown,
        "minecraft:current_down",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::BubbleColumnUp,
        "minecraft:bubble_column_up",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Nautilus,
        "minecraft:nautilus",
        particles::NautilusParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Dolphin,
        "minecraft:dolphin",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    // CampfireCozy: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::CampfireCozy,
        "minecraft:campfire_cozy_smoke",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // CampfireSignal: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::CampfireSignal,
        "minecraft:campfire_signal_smoke",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(ParticleTypeId::DrippingHoney,
        "minecraft:dripping_honey",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingHoney,
        "minecraft:falling_honey",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::LandingHoney,
        "minecraft:landing_honey",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 花蜜/孢子/下界粒子 (80-98)
    registerSimpleType(ParticleTypeId::FallingNectar,
        "minecraft:falling_nectar",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingSporeBlossom,
        "minecraft:falling_spore_blossom",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::Ash, "minecraft:ash", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::CrimsonSpore,
        "minecraft:crimson_spore",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::WarpedSpore,
        "minecraft:warped_spore",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::SporeBlossomAir,
        "minecraft:spore_blossom_air",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::DrippingObsidianTear,
        "minecraft:dripping_obsidian_tear",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingObsidianTear,
        "minecraft:falling_obsidian_tear",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::LandingObsidianTear,
        "minecraft:landing_obsidian_tear",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::ReversePortal,
        "minecraft:reverse_portal",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::WhiteAsh,
        "minecraft:white_ash",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::SmallFlame, "minecraft:small_flame", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Snowflake,
        "minecraft:snowflake",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::DrippingDripstoneLava,
        "minecraft:dripping_dripstone_lava",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::FallingDripstoneLava,
        "minecraft:falling_dripstone_lava",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::DrippingDripstoneWater,
        "minecraft:dripping_dripstone_water",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingDripstoneWater,
        "minecraft:falling_dripstone_water",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    // GlowSquidInk: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::GlowSquidInk,
        "minecraft:glow_squid_ink",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT,
        true);
    // Glow: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(
        ParticleTypeId::Glow, "minecraft:glow", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT, true);

    // 铜蚀/幽匿/试炼/不祥粒子 (99-114)
    // WaxOn: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::WaxOn,
        "minecraft:wax_on",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // WaxOff: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::WaxOff,
        "minecraft:wax_off",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // ElectricSpark: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::ElectricSpark,
        "minecraft:electric_spark",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT,
        true);
    // Scrape: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::Scrape,
        "minecraft:scrape",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(
        ParticleTypeId::Shriek, "minecraft:shriek", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::EggCrack,
        "minecraft:egg_crack",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::DustPlume,
        "minecraft:dust_plume",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    // TrialSpawnerDetection: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::TrialSpawnerDetection,
        "minecraft:trial_spawner_detection",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // TrialSpawnerDetectionOminous: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::TrialSpawnerDetectionOminous,
        "minecraft:trial_spawner_detection_ominous",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    // VaultConnection: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::VaultConnection,
        "minecraft:vault_connection",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(
        ParticleTypeId::DustPillar, "minecraft:dust_pillar", ParticleFactory{}, ParticleRenderType::TERRAIN_SHEET);
    // OminousSpawning: overrideLimiter=true — 重要粒子，始终显示
    registerSimpleType(ParticleTypeId::OminousSpawning,
        "minecraft:ominous_spawning",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        true);
    registerSimpleType(ParticleTypeId::RaidOmen,
        "minecraft:raid_omen",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::TrialOmen,
        "minecraft:trial_omen",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(
        ParticleTypeId::BlockCrumble, "minecraft:block_crumble", ParticleFactory{}, ParticleRenderType::TERRAIN_SHEET);
    registerSimpleType(
        ParticleTypeId::Firefly, "minecraft:firefly", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);

    // 项目内部扩展粒子（不在 MC 协议中）
    registerSimpleType(
        ParticleTypeId::Breaking, "minecraft:breaking", ParticleFactory{}, ParticleRenderType::TERRAIN_SHEET);
    registerSimpleType(
        ParticleTypeId::Barrier, "minecraft:barrier", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    registerSimpleType(
        ParticleTypeId::Light, "minecraft:light", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(
        ParticleTypeId::Redstone, "minecraft:redstone", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::LargeExplosion,
        "minecraft:large_explosion",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::ItemPickup,
        "minecraft:item_pickup",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::DrippingCherryLeaves,
        "minecraft:dripping_cherry_leaves",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingCherryLeaves,
        "minecraft:falling_cherry_leaves",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::LandingCherryLeaves,
        "minecraft:landing_cherry_leaves",
        ParticleFactory{},
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

} // namespace mc::client::renderer::trident::particle
