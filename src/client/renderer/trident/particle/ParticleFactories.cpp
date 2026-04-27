#include "ParticleFactories.hpp"
#include "ParticleTypes.hpp"

// 粒子类型头文件
#include "particles/effect/FlameParticle.hpp"
#include "particles/effect/SmokeParticle.hpp"
#include "particles/effect/LavaParticle.hpp"
#include "particles/effect/PortalParticle.hpp"
#include "particles/effect/CritParticle.hpp"
#include "particles/effect/ExplosionParticle.hpp"
#include "particles/effect/EmitterParticle.hpp"
#include "particles/effect/PoofParticle.hpp"
#include "particles/effect/SpellParticle.hpp"
#include "particles/effect/DragonBreathParticle.hpp"
#include "particles/effect/SoulParticle.hpp"
#include "particles/effect/RedstoneParticle.hpp"
#include "particles/effect/CampfireParticle.hpp"
#include "particles/ambient/BubbleParticle.hpp"
#include "particles/ambient/UnderwaterParticle.hpp"
#include "particles/ambient/CloudParticle.hpp"
#include "particles/RainParticle.hpp"
#include "particles/weather/SplashParticle.hpp"
#include "particles/mob/HeartParticle.hpp"
#include "particles/mob/VillagerParticle.hpp"
#include "particles/block/DiggingParticle.hpp"
#include "particles/liquid/DripParticle.hpp"
#include "particles/liquid/DripWaterParticle.hpp"
#include "particles/SnowParticle.hpp"

// 导入粒子类型命名空间
using namespace mc::client::renderer::trident::particle::particles;

namespace mc::client::renderer::trident::particle {

void registerBuiltinParticleFactories() {
    auto& registry = ParticleRegistry::instance();

    // 环境类粒子
    registry.registerType(
        ParticleTypeId::AmbientEntityEffect,
        "minecraft:ambient_entity_effect",
        AmbientEntityEffectParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        12.0f, false, false);

    registry.registerType(
        ParticleTypeId::Bubble,
        "minecraft:bubble",
        BubbleParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f, false, false);

    registry.registerType(
        ParticleTypeId::Underwater,
        "minecraft:underwater",
        UnderwaterParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        30.0f, false, false);

    registry.registerType(
        ParticleTypeId::Barrier,
        "minecraft:barrier",
        BarrierParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        80.0f, false, false);

    registry.registerType(
        ParticleTypeId::SoulFireFlame,
        "minecraft:soul_fire_flame",
        SoulFireFlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        30.0f, false, false);

    registry.registerType(
        ParticleTypeId::Soul,
        "minecraft:soul",
        SoulParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        12.0f, false, false);

    // 效果类粒子
    registry.registerType(
        ParticleTypeId::Flame,
        "minecraft:flame",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        30.0f, false, false);

    registry.registerType(
        ParticleTypeId::Smoke,
        "minecraft:smoke",
        SmokeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f, false, false);

    registry.registerType(
        ParticleTypeId::LargeSmoke,
        "minecraft:large_smoke",
        SmokeParticle::create,  // LargeSmoke 使用 Smoke 粒子，但大小不同
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f, false, false);

    registry.registerType(
        ParticleTypeId::Lava,
        "minecraft:lava",
        LavaParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        16.0f, false, false);

    registry.registerType(
        ParticleTypeId::Portal,
        "minecraft:portal",
        PortalParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        45.0f, false, false);

    registry.registerType(
        ParticleTypeId::Explosion,
        "minecraft:explosion",
        ExplosionParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        6.0f, false, false);

    registry.registerType(
        ParticleTypeId::Poof,
        "minecraft:poof",
        PoofParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        18.0f, false, false);

    registry.registerType(
        ParticleTypeId::Crit,
        "minecraft:crit",
        CritParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        6.0f, false, false);

    registry.registerType(
        ParticleTypeId::Spell,
        "minecraft:spell",
        SpellParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f, false, false);

    registry.registerType(
        ParticleTypeId::InstantSpell,
        "minecraft:instant_spell",
        InstantSpellParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f, false, false);

    registry.registerType(
        ParticleTypeId::EntityEffect,
        "minecraft:entity_effect",
        EntityEffectParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f, false, false);

    registry.registerType(
        ParticleTypeId::Redstone,
        "minecraft:redstone",
        RedstoneParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f, false, false);

    registry.registerType(
        ParticleTypeId::Enchant,
        "minecraft:enchant",
        EnchantParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        30.0f, false, false);

    registry.registerType(
        ParticleTypeId::SweepAttack,
        "minecraft:sweep_attack",
        SweepAttackParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        4.0f, false, false);

    registry.registerType(
        ParticleTypeId::DragonBreath,
        "minecraft:dragon_breath",
        DragonBreathParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f, false, false);

    registry.registerType(
        ParticleTypeId::EndRod,
        "minecraft:end_rod",
        EndRodParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        60.0f, false, false);

    // 营火烟雾粒子
    registry.registerType(
        ParticleTypeId::CampfireCozy,
        "minecraft:campfire_cozy_smoke",
        CampfireParticle::createCozy,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        80.0f, false, false);

    registry.registerType(
        ParticleTypeId::CampfireSignal,
        "minecraft:campfire_signal_smoke",
        CampfireParticle::createSignal,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        280.0f, false, false);

    // 天气类粒子
    registry.registerType(
        ParticleTypeId::Rain,
        "minecraft:rain",
        RainParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f, true, false);

    registry.registerType(
        ParticleTypeId::Snowflake,
        "minecraft:snowflake",
        SnowParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        60.0f, true, false);

    registry.registerType(
        ParticleTypeId::Splash,
        "minecraft:splash",
        SplashParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f, true, false);

    // 生物相关粒子
    registry.registerType(
        ParticleTypeId::Heart,
        "minecraft:heart",
        HeartParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        16.0f, false, false);

    registry.registerType(
        ParticleTypeId::AngryVillager,
        "minecraft:angry_villager",
        AngryVillagerParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f, false, false);

    registry.registerType(
        ParticleTypeId::HappyVillager,
        "minecraft:happy_villager",
        HappyVillagerParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f, false, false);

    registry.registerType(
        ParticleTypeId::Sneeze,
        "minecraft:sneeze",
        SneezeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f, false, false);

    registry.registerType(
        ParticleTypeId::Dolphin,
        "minecraft:dolphin",
        DolphinParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f, false, false);

    // 液体滴落类粒子
    registry.registerType(
        ParticleTypeId::DrippingWater,
        "minecraft:dripping_water",
        DripWaterParticle::createDripping,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        40.0f, true, false);

    registry.registerType(
        ParticleTypeId::FallingWater,
        "minecraft:falling_water",
        DripWaterParticle::createFalling,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        64.0f, true, false);

    registry.registerType(
        ParticleTypeId::DrippingLava,
        "minecraft:dripping_lava",
        DripParticle::createDrippingLava,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        40.0f, true, false);

    registry.registerType(
        ParticleTypeId::FallingLava,
        "minecraft:falling_lava",
        DripParticle::createFallingLava,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        64.0f, true, false);

    registry.registerType(
        ParticleTypeId::LandingLava,
        "minecraft:landing_lava",
        DripParticle::createLandingLava,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        16.0f, true, false);

    registry.registerType(
        ParticleTypeId::DrippingHoney,
        "minecraft:dripping_honey",
        DripParticle::createDrippingHoney,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        40.0f, true, false);

    registry.registerType(
        ParticleTypeId::FallingHoney,
        "minecraft:falling_honey",
        DripParticle::createFallingHoney,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        64.0f, true, false);

    registry.registerType(
        ParticleTypeId::LandingHoney,
        "minecraft:landing_honey",
        DripParticle::createLandingHoney,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        16.0f, true, false);

    // 方块粒子
    registry.registerType(
        ParticleTypeId::Block,
        "minecraft:block",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f, true, false);

    registry.registerType(
        ParticleTypeId::Breaking,
        "minecraft:breaking",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f, true, false);

    registry.registerType(
        ParticleTypeId::FallingDust,
        "minecraft:falling_dust",
        FallingDustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        29.0f, true, false);

    // 云朵粒子
    registry.registerType(
        ParticleTypeId::Cloud,
        "minecraft:cloud",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f, false, false);

    // 发射器粒子（不渲染，只发射其他粒子）
    registry.registerType(
        ParticleTypeId::HugeExplosion,
        "minecraft:explosion_emitter",
        HugeExplosionEmitterParticle::create,
        ParticleRenderType::NO_RENDER,
        8.0f, false, false);
}

} // namespace mc::client::renderer::trident::particle
