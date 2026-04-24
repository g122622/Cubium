#include "ParticleRegistry.hpp"
#include "Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle {

ParticleRegistry& ParticleRegistry::instance() {
    static ParticleRegistry instance;
    return instance;
}

ParticleRegistry::ParticleRegistry() {
    registerBuiltinTypes();
}

void ParticleRegistry::registerType(
    ParticleTypeId id,
    const String& name,
    ParticleFactory factory,
    ParticleRenderType defaultRenderType,
    f64 defaultLifetime,
    bool hasPhysics,
    bool ignoreDistance) {

    MC_ASSERT_MSG(isValidParticleType(id), "Invalid particle type ID");
    // factory 可以为 nullptr，用于仅注册元数据的情况
    MC_ASSERT_MSG(!name.empty(), "Particle name cannot be empty");

    ParticleTypeInfo info;
    info.id = id;
    info.name = name;
    info.factory = std::move(factory);
    info.defaultRenderType = defaultRenderType;
    info.defaultLifetime = defaultLifetime;
    info.hasPhysics = hasPhysics;
    info.ignoreDistance = ignoreDistance;

    m_types[id] = std::move(info);
    m_nameToId[name] = id;
}

void ParticleRegistry::registerSimpleType(
    ParticleTypeId id,
    const String& name,
    ParticleFactory factory,
    ParticleRenderType defaultRenderType) {

    registerType(id, name, std::move(factory), defaultRenderType, 1.0f, true, false);
}

std::unique_ptr<Particle> ParticleRegistry::createParticle(
    ParticleTypeId id,
    const glm::vec3& pos,
    const glm::vec3& velocity,
    ClientWorld* world) const {

    auto it = m_types.find(id);
    if (it == m_types.end() || !it->second.factory) {
        return nullptr;
    }

    return it->second.factory(pos, velocity, world);
}

std::unique_ptr<Particle> ParticleRegistry::createParticle(
    const String& name,
    const glm::vec3& pos,
    const glm::vec3& velocity,
    ClientWorld* world) const {

    auto id = getTypeId(name);
    if (!id.has_value()) {
        return nullptr;
    }

    return createParticle(id.value(), pos, velocity, world);
}

Optional<ParticleTypeId> ParticleRegistry::getTypeId(const String& name) const {
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end()) {
        return it->second;
    }
    return std::nullopt;
}

Optional<ParticleTypeId> ParticleRegistry::getTypeId(const ResourceLocation& location) const {
    return getTypeId(location.toString());
}

const String& ParticleRegistry::getTypeName(ParticleTypeId id) const {
    auto it = m_types.find(id);
    if (it != m_types.end()) {
        return it->second.name;
    }
    return m_invalidTypeName;
}

const ParticleTypeInfo* ParticleRegistry::getTypeInfo(ParticleTypeId id) const {
    auto it = m_types.find(id);
    if (it != m_types.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ParticleRegistry::isRegistered(ParticleTypeId id) const {
    return m_types.find(id) != m_types.end();
}

bool ParticleRegistry::isRegistered(const String& name) const {
    return m_nameToId.find(name) != m_nameToId.end();
}

std::vector<ParticleTypeId> ParticleRegistry::getAllTypeIds() const {
    std::vector<ParticleTypeId> ids;
    ids.reserve(m_types.size());
    for (const auto& pair : m_types) {
        ids.push_back(pair.first);
    }
    return ids;
}

void ParticleRegistry::registerBuiltinTypes() {
    // 注意：这里只注册类型元数据，不注册工厂函数
    // 工厂函数在具体粒子类型实现后注册

    // 环境类粒子
    registerSimpleType(ParticleTypeId::AmbientEntityEffect, "minecraft:ambient_entity_effect", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Bubble, "minecraft:bubble", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::BubblePop, "minecraft:bubble_pop", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::BubbleColumnUp, "minecraft:bubble_column_up", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::CurrentDown, "minecraft:current_down", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Underwater, "minecraft:underwater", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Barrier, "minecraft:barrier", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    registerSimpleType(ParticleTypeId::Light, "minecraft:light", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::SoulFireFlame, "minecraft:soul_fire_flame", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Soul, "minecraft:soul", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 方块/物品类粒子
    registerSimpleType(ParticleTypeId::Block, "minecraft:block", ParticleFactory{}, ParticleRenderType::TERRAIN_SHEET);
    registerSimpleType(ParticleTypeId::Breaking, "minecraft:breaking", ParticleFactory{}, ParticleRenderType::TERRAIN_SHEET);
    registerSimpleType(ParticleTypeId::FallingDust, "minecraft:falling_dust", ParticleFactory{}, ParticleRenderType::TERRAIN_SHEET);
    registerSimpleType(ParticleTypeId::Item, "minecraft:item", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::ItemSlime, "minecraft:item_slime", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::ItemSnowball, "minecraft:item_snowball", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 效果类粒子
    registerSimpleType(ParticleTypeId::Flame, "minecraft:flame", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Smoke, "minecraft:smoke", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::LargeSmoke, "minecraft:large_smoke", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Lava, "minecraft:lava", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Portal, "minecraft:portal", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::ReversePortal, "minecraft:reverse_portal", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Explosion, "minecraft:explosion", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Poof, "minecraft:poof", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Crit, "minecraft:crit", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::EnchantedHit, "minecraft:enchanted_hit", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Spell, "minecraft:spell", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::InstantSpell, "minecraft:instant_spell", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::EntityEffect, "minecraft:entity_effect", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Redstone, "minecraft:redstone", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Enchant, "minecraft:enchant", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::SweepAttack, "minecraft:sweep_attack", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Spit, "minecraft:spit", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::SquidInk, "minecraft:squid_ink", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::DragonBreath, "minecraft:dragon_breath", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::EndRod, "minecraft:end_rod", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);

    // 液体滴落类粒子
    registerSimpleType(ParticleTypeId::DrippingWater, "minecraft:dripping_water", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingWater, "minecraft:falling_water", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::DrippingLava, "minecraft:dripping_lava", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::FallingLava, "minecraft:falling_lava", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::LandingLava, "minecraft:landing_lava", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::DrippingHoney, "minecraft:dripping_honey", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingHoney, "minecraft:falling_honey", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::LandingHoney, "minecraft:landing_honey", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::DrippingObsidianTear, "minecraft:dripping_obsidian_tear", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::FallingObsidianTear, "minecraft:falling_obsidian_tear", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 天气类粒子
    registerSimpleType(ParticleTypeId::Rain, "minecraft:rain", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Snowflake, "minecraft:snowflake", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Splash, "minecraft:splash", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 生物相关粒子
    registerSimpleType(ParticleTypeId::Heart, "minecraft:heart", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::AngryVillager, "minecraft:angry_villager", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::HappyVillager, "minecraft:happy_villager", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Sneeze, "minecraft:sneeze", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Dolphin, "minecraft:dolphin", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 特殊粒子
    registerSimpleType(ParticleTypeId::TotemOfUndying, "minecraft:totem_of_undying", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Flash, "minecraft:flash", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::ElderGuardian, "minecraft:elder_guardian", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Nautilus, "minecraft:nautilus", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Firework, "minecraft:firework", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 下界更新粒子
    registerSimpleType(ParticleTypeId::Ash, "minecraft:ash", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::WhiteAsh, "minecraft:white_ash", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::CrimsonSpore, "minecraft:crimson_spore", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::WarpedSpore, "minecraft:warped_spore", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::LandingObsidianTear, "minecraft:landing_obsidian_tear", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::Dust, "minecraft:dust", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::DustColorTransition, "minecraft:dust_color_transition", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Vibration, "minecraft:vibration", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    registerSimpleType(ParticleTypeId::GlowSquidInk, "minecraft:glow_squid_ink", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
    registerSimpleType(ParticleTypeId::Glow, "minecraft:glow", ParticleFactory{}, ParticleRenderType::PARTICLE_SHEET_LIT);
}

} // namespace mc::client::renderer::trident::particle
