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

#include "ParticleFactories.hpp"
#include "ParticleTypes.hpp"

// 粒子类型头文件
#include "particles/RainParticle.hpp"
#include "particles/SnowParticle.hpp"
#include "particles/ambient/BubbleParticle.hpp"
#include "particles/ambient/BubblePopParticle.hpp"
#include "particles/ambient/CloudParticle.hpp"
#include "particles/ambient/SporeBlossomParticle.hpp"
#include "particles/ambient/SuspendedTownParticle.hpp"
#include "particles/ambient/UnderwaterParticle.hpp"
#include "particles/block/DiggingParticle.hpp"
#include "particles/block/DustPillarParticle.hpp"
#include "particles/effect/CampfireParticle.hpp"
#include "particles/effect/CritParticle.hpp"
#include "particles/effect/DragonBreathParticle.hpp"
#include "particles/effect/EmitterParticle.hpp"
#include "particles/effect/ExplosionParticle.hpp"
#include "particles/effect/FlameParticle.hpp"
#include "particles/effect/LavaParticle.hpp"
#include "particles/effect/PoofParticle.hpp"
#include "particles/effect/PortalParticle.hpp"
#include "particles/effect/RedstoneParticle.hpp"
#include "particles/effect/SmokeParticle.hpp"
#include "particles/effect/SoulParticle.hpp"
#include "particles/effect/SpellParticle.hpp"
#include "particles/effect/WhiteSmokeParticle.hpp"
#include "particles/liquid/DripParticle.hpp"
#include "particles/liquid/DripWaterParticle.hpp"
#include "particles/liquid/DripstoneDripParticle.hpp"
#include "particles/mob/HeartParticle.hpp"
#include "particles/mob/VillagerParticle.hpp"
#include "particles/special/NautilusParticle.hpp"
#include "particles/special/VibrationSignalParticle.hpp"
#include "particles/weather/FishingParticle.hpp"
#include "particles/weather/SplashParticle.hpp"

// 导入粒子类型命名空间
using namespace mc::client::renderer::trident::particle::particles;

namespace mc::client::renderer::trident::particle {

void registerBuiltinParticleFactories()
{
    auto& registry = ParticleRegistry::instance();

    // ========================================================================
    // 方块类粒子 (0-2)
    // ========================================================================

    registry.registerType(ParticleTypeId::AngryVillager,
        "minecraft:angry_villager",
        AngryVillagerParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Block,
        "minecraft:block",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // BlockMarker: 方块标记粒子（需要方块状态，用于结构方块等标记显示）
    // TODO: 实现 BlockMarkerParticle，暂时复用 DiggingParticle
    registry.registerType(ParticleTypeId::BlockMarker,
        "minecraft:block_marker",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // ========================================================================
    // 环境类粒子 (3-9)
    // ========================================================================

    registry.registerType(ParticleTypeId::Bubble,
        "minecraft:bubble",
        BubbleParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Cloud,
        "minecraft:cloud",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        false,
        false);

    // CopperFireFlame: 铜火火焰粒子（类似灵魂火焰）
    // TODO: 实现 CopperFireFlameParticle，暂时复用 SoulFireFlameParticle
    registry.registerType(ParticleTypeId::CopperFireFlame,
        "minecraft:copper_fire_flame",
        SoulFireFlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        30.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Crit,
        "minecraft:crit",
        CritParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        6.0f,
        false,
        false);

    // DamageIndicator: 伤害指示器粒子（实体受伤时弹出）
    // TODO: 实现 DamageIndicatorParticle，暂时复用 CritParticle
    registry.registerType(ParticleTypeId::DamageIndicator,
        "minecraft:damage_indicator",
        CritParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        6.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::DragonBreath,
        "minecraft:dragon_breath",
        DragonBreathParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f,
        false,
        false);

    // ========================================================================
    // 液体滴落类粒子 (9-13)
    // ========================================================================

    registry.registerType(ParticleTypeId::DrippingLava,
        "minecraft:dripping_lava",
        DripParticle::createDrippingLava,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        40.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::FallingLava,
        "minecraft:falling_lava",
        DripParticle::createFallingLava,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        64.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::LandingLava,
        "minecraft:landing_lava",
        DripParticle::createLandingLava,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        16.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::DrippingWater,
        "minecraft:dripping_water",
        DripWaterParticle::createDripping,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        40.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::FallingWater,
        "minecraft:falling_water",
        DripWaterParticle::createFalling,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        64.0f,
        true,
        false);

    // ========================================================================
    // 染色粒子 (14-15)
    // ========================================================================

    // Dust: 染色粒子（红石粉尘等，需要颜色数据）
    // TODO: 实现 DustParticle，暂时复用 RedstoneParticle
    registry.registerType(ParticleTypeId::Dust,
        "minecraft:dust",
        RedstoneParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    // DustColorTransition: 颜色过渡染色粒子
    // TODO: 实现 DustColorTransitionParticle，暂时复用 RedstoneParticle
    registry.registerType(ParticleTypeId::DustColorTransition,
        "minecraft:dust_color_transition",
        RedstoneParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    // ========================================================================
    // 效果类粒子 (16-28)
    // ========================================================================

    registry.registerType(ParticleTypeId::Spell,
        "minecraft:spell",
        SpellParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f,
        false,
        false);

    // ElderGuardian: 守卫者外观粒子
    // TODO: 实现 ElderGuardianParticle
    registry.registerType(ParticleTypeId::ElderGuardian,
        "minecraft:elder_guardian",
        EntityEffectParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::EnchantedHit,
        "minecraft:enchanted_hit",
        EnchantedHitParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        6.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Enchant,
        "minecraft:enchant",
        EnchantParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        30.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::EndRod,
        "minecraft:end_rod",
        EndRodParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        60.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::EntityEffect,
        "minecraft:entity_effect",
        EntityEffectParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::HugeExplosion,
        "minecraft:explosion_emitter",
        HugeExplosionEmitterParticle::create,
        ParticleRenderType::NO_RENDER,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Explosion,
        "minecraft:explosion",
        ExplosionParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        6.0f,
        false,
        false);

    // Gust: 风爆粒子
    // TODO: 实现 GustParticle，暂时复用 CloudParticle
    registry.registerType(ParticleTypeId::Gust,
        "minecraft:gust",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // SmallGust: 小型风爆粒子
    // TODO: 实现 SmallGustParticle，暂时复用 CloudParticle
    registry.registerType(ParticleTypeId::SmallGust,
        "minecraft:small_gust",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // GustEmitterLarge: 大型风爆发射器粒子
    // TODO: 实现 GustEmitterLargeParticle
    registry.registerType(ParticleTypeId::GustEmitterLarge,
        "minecraft:gust_emitter_large",
        HugeExplosionEmitterParticle::create,
        ParticleRenderType::NO_RENDER,
        8.0f,
        false,
        false);

    // GustEmitterSmall: 小型风爆发射器粒子
    // TODO: 实现 GustEmitterSmallParticle
    registry.registerType(ParticleTypeId::GustEmitterSmall,
        "minecraft:gust_emitter_small",
        HugeExplosionEmitterParticle::create,
        ParticleRenderType::NO_RENDER,
        8.0f,
        false,
        false);

    // SonicBoom: 声波轰击粒子（监守者远程攻击）
    // TODO: 实现 SonicBoomParticle，暂时复用 DragonBreathParticle
    registry.registerType(ParticleTypeId::SonicBoom,
        "minecraft:sonic_boom",
        DragonBreathParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        false,
        false);

    // ========================================================================
    // 方块/物品/烟花粒子 (29-31)
    // ========================================================================

    registry.registerType(ParticleTypeId::FallingDust,
        "minecraft:falling_dust",
        FallingDustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        29.0f,
        true,
        false);

    // Firework: 烟花粒子
    // TODO: 实现 FireworkParticle
    registry.registerType(ParticleTypeId::Firework,
        "minecraft:firework",
        ExplosionParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        6.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Fishing,
        "minecraft:fishing",
        FishingParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    // ========================================================================
    // 火焰/效果粒子 (32-52)
    // ========================================================================

    registry.registerType(ParticleTypeId::Flame,
        "minecraft:flame",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        30.0f,
        false,
        false);

    // Infested: 虫蚀方块粒子（蠹虫出现时）
    // TODO: 实现 InfestedParticle，暂时复用 CloudParticle
    registry.registerType(ParticleTypeId::Infested,
        "minecraft:infested",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // CherryLeaves: 樱花树叶粒子
    // TODO: 实现 CherryLeavesParticle，暂时复用 CloudParticle
    registry.registerType(ParticleTypeId::CherryLeaves,
        "minecraft:cherry_leaves",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        true,
        false);

    // PaleOakLeaves: 苍白橡树树叶粒子
    // TODO: 实现 PaleOakLeavesParticle，暂时复用 CloudParticle
    registry.registerType(ParticleTypeId::PaleOakLeaves,
        "minecraft:pale_oak_leaves",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        true,
        false);

    // TintedLeaves: 着色树叶粒子（带颜色数据）
    // TODO: 实现 TintedLeavesParticle，暂时复用 FallingDustParticle
    registry.registerType(ParticleTypeId::TintedLeaves,
        "minecraft:tinted_leaves",
        FallingDustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f,
        true,
        false);

    // SculkSoul: 幽匿灵魂粒子
    // TODO: 实现 SculkSoulParticle，暂时复用 SoulParticle
    registry.registerType(ParticleTypeId::SculkSoul,
        "minecraft:sculk_soul",
        SoulParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        12.0f,
        false,
        false);

    // SculkCharge: 幽匿充能粒子（带充能数据）
    // TODO: 实现 SculkChargeParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::SculkCharge,
        "minecraft:sculk_charge",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        10.0f,
        false,
        false);

    // SculkChargePop: 幽匿充能弹出粒子
    // TODO: 实现 SculkChargePopParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::SculkChargePop,
        "minecraft:sculk_charge_pop",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        5.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::SoulFireFlame,
        "minecraft:soul_fire_flame",
        SoulFireFlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        30.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Soul,
        "minecraft:soul",
        SoulParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        12.0f,
        false,
        false);

    // Flash: 闪光粒子（带颜色数据）
    // TODO: 实现 FlashParticle，暂时复用 ExplosionParticle
    registry.registerType(ParticleTypeId::Flash,
        "minecraft:flash",
        ExplosionParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        4.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::HappyVillager,
        "minecraft:happy_villager",
        HappyVillagerParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // Composter: 堆肥桶粒子
    // TODO: 实现 ComposterParticle，暂时复用 FallingDustParticle
    registry.registerType(ParticleTypeId::Composter,
        "minecraft:composter",
        FallingDustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::Heart,
        "minecraft:heart",
        HeartParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        16.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::InstantSpell,
        "minecraft:instant_spell",
        InstantSpellParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f,
        false,
        false);

    // Item: 物品粒子（需要物品数据）
    // TODO: 实现 ItemParticle，暂时复用 DiggingParticle
    registry.registerType(ParticleTypeId::Item,
        "minecraft:item",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // 振动信号粒子（从源位置飞向目标位置）
    registry.registerType(ParticleTypeId::Vibration,
        "minecraft:vibration",
        VibrationSignalParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        60.0f,
        false,
        true); // 忽略距离限制，确保可见

    // Trail: 轨迹粒子（带颜色/目标数据）
    // TODO: 实现 TrailParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::Trail,
        "minecraft:trail",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        10.0f,
        false,
        false);

    // ItemSlime: 史莱姆粒子（需要物品数据）
    // TODO: 实现 ItemSlimeParticle，暂时复用 DiggingParticle
    registry.registerType(ParticleTypeId::ItemSlime,
        "minecraft:item_slime",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // ItemCobweb: 蛛网物品粒子（需要物品数据）
    // TODO: 实现 ItemCobwebParticle，暂时复用 DiggingParticle
    registry.registerType(ParticleTypeId::ItemCobweb,
        "minecraft:item_cobweb",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // ItemSnowball: 雪球粒子（需要物品数据）
    // TODO: 实现 ItemSnowballParticle，暂时复用 DiggingParticle
    registry.registerType(ParticleTypeId::ItemSnowball,
        "minecraft:item_snowball",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // ========================================================================
    // 烟雾/天气/生物粒子 (53-69)
    // ========================================================================

    registry.registerType(ParticleTypeId::LargeSmoke,
        "minecraft:large_smoke",
        SmokeParticle::create, // LargeSmoke 使用 Smoke 粒子，但大小不同
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Lava,
        "minecraft:lava",
        LavaParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        16.0f,
        false,
        false);

    // 菌丝悬浮粒子（MC 原版 SuspendedTownParticle.Provider）
    registry.registerType(ParticleTypeId::Mycelium,
        "minecraft:mycelium",
        SuspendedTownParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f,
        false,
        false);

    // Note: 音符粒子（音符盒）
    // TODO: 实现 NoteParticle
    registry.registerType(ParticleTypeId::Note,
        "minecraft:note",
        EnchantParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        30.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Poof,
        "minecraft:poof",
        PoofParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        18.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Portal,
        "minecraft:portal",
        PortalParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        45.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Rain,
        "minecraft:rain",
        RainParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::Smoke,
        "minecraft:smoke",
        SmokeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::WhiteSmoke,
        "minecraft:white_smoke",
        WhiteSmokeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Sneeze,
        "minecraft:sneeze",
        SneezeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // Spit: 羊驼吐沫粒子
    // TODO: 实现 SpitParticle，暂时复用 CloudParticle
    registry.registerType(ParticleTypeId::Spit,
        "minecraft:spit",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        6.0f,
        false,
        false);

    // SquidInk: 鱿鱼墨汁粒子
    // TODO: 实现 SquidInkParticle，暂时复用 CloudParticle
    registry.registerType(ParticleTypeId::SquidInk,
        "minecraft:squid_ink",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::SweepAttack,
        "minecraft:sweep_attack",
        SweepAttackParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        4.0f,
        false,
        false);

    // TotemOfUndying: 不死图腾粒子
    // TODO: 实现 TotemOfUndyingParticle，暂时复用 EntityEffectParticle
    registry.registerType(ParticleTypeId::TotemOfUndying,
        "minecraft:totem_of_undying",
        EntityEffectParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Underwater,
        "minecraft:underwater",
        UnderwaterParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        30.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Splash,
        "minecraft:splash",
        SplashParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::Witch,
        "minecraft:witch",
        WitchParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f,
        false,
        false);

    // ========================================================================
    // 水下/营地/蜂蜜粒子 (70-79)
    // ========================================================================

    registry.registerType(ParticleTypeId::BubblePop,
        "minecraft:bubble_pop",
        BubblePopParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        4.0f,
        true,
        false);

    // CurrentDown: 向下的水流粒子
    // TODO: 实现 CurrentDownParticle，暂时复用 BubbleParticle
    registry.registerType(ParticleTypeId::CurrentDown,
        "minecraft:current_down",
        BubbleParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    // BubbleColumnUp: 气泡柱上升粒子
    // TODO: 实现 BubbleColumnUpParticle，暂时复用 BubbleParticle
    registry.registerType(ParticleTypeId::BubbleColumnUp,
        "minecraft:bubble_column_up",
        BubbleParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Nautilus,
        "minecraft:nautilus",
        NautilusParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        60.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Dolphin,
        "minecraft:dolphin",
        DolphinParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        false,
        false);

    // 营火烟雾粒子
    registry.registerType(ParticleTypeId::CampfireCozy,
        "minecraft:campfire_cozy_smoke",
        CampfireParticle::createCozy,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        80.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::CampfireSignal,
        "minecraft:campfire_signal_smoke",
        CampfireParticle::createSignal,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        280.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::DrippingHoney,
        "minecraft:dripping_honey",
        DripParticle::createDrippingHoney,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        40.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::FallingHoney,
        "minecraft:falling_honey",
        DripParticle::createFallingHoney,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        64.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::LandingHoney,
        "minecraft:landing_honey",
        DripParticle::createLandingHoney,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        16.0f,
        true,
        false);

    // ========================================================================
    // 花蜜/孢子/下界粒子 (80-98)
    // ========================================================================

    // FallingNectar: 下落的花蜜粒子（蜜蜂相关）
    // TODO: 实现 FallingNectarParticle，暂时复用 DripWaterParticle::createDripping
    registry.registerType(ParticleTypeId::FallingNectar,
        "minecraft:falling_nectar",
        DripWaterParticle::createDripping,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        40.0f,
        true,
        false);

    // 孢子花掉落粒子
    registry.registerType(ParticleTypeId::FallingSporeBlossom,
        "minecraft:falling_spore_blossom",
        FallingSporeBlossomParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        60.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::SporeBlossomAir,
        "minecraft:spore_blossom_air",
        SporeBlossomAirParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        100.0f,
        false,
        false);

    // Ash: 灰烬粒子
    // TODO: 实现 AshParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::Ash,
        "minecraft:ash",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    // CrimsonSpore: 绯红孢子
    // TODO: 实现 CrimsonSporeParticle，暂时复用 SuspendedTownParticle
    registry.registerType(ParticleTypeId::CrimsonSpore,
        "minecraft:crimson_spore",
        SuspendedTownParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f,
        false,
        false);

    // WarpedSpore: 诡异孢子
    // TODO: 实现 WarpedSporeParticle，暂时复用 SuspendedTownParticle
    registry.registerType(ParticleTypeId::WarpedSpore,
        "minecraft:warped_spore",
        SuspendedTownParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::DrippingObsidianTear,
        "minecraft:dripping_obsidian_tear",
        DripParticle::createDrippingObsidianTear,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        40.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::FallingObsidianTear,
        "minecraft:falling_obsidian_tear",
        DripParticle::createFallingObsidianTear,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        64.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::LandingObsidianTear,
        "minecraft:landing_obsidian_tear",
        DripParticle::createLandingObsidianTear,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        28.0f,
        true,
        false);

    // ReversePortal: 反向传送门粒子
    // TODO: 实现 ReversePortalParticle，暂时复用 PortalParticle
    registry.registerType(ParticleTypeId::ReversePortal,
        "minecraft:reverse_portal",
        PortalParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        45.0f,
        false,
        false);

    // WhiteAsh: 白色灰烬粒子
    // TODO: 实现 WhiteAshParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::WhiteAsh,
        "minecraft:white_ash",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    // SmallFlame: 小型火焰粒子（蜡烛等）
    // TODO: 实现 SmallFlameParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::SmallFlame,
        "minecraft:small_flame",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    registry.registerType(ParticleTypeId::Snowflake,
        "minecraft:snowflake",
        SnowParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        60.0f,
        true,
        false);

    // 滴水石专用粒子（落地时播放滴水音效）
    registry.registerType(ParticleTypeId::DrippingDripstoneLava,
        "minecraft:dripping_dripstone_lava",
        DripstoneLavaDripParticle::createDripping,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        40.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::FallingDripstoneLava,
        "minecraft:falling_dripstone_lava",
        DripstoneLavaDripParticle::createFalling,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        64.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::DrippingDripstoneWater,
        "minecraft:dripping_dripstone_water",
        DripstoneWaterDripParticle::createDripping,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        40.0f,
        true,
        false);

    registry.registerType(ParticleTypeId::FallingDripstoneWater,
        "minecraft:falling_dripstone_water",
        DripstoneWaterDripParticle::createFalling,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        64.0f,
        true,
        false);

    // GlowSquidInk: 荧光墨囊粒子
    // TODO: 实现 GlowSquidInkParticle，暂时复用 CloudParticle
    registry.registerType(ParticleTypeId::GlowSquidInk,
        "minecraft:glow_squid_ink",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // Glow: 荧光地衣粒子
    // TODO: 实现 GlowParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::Glow,
        "minecraft:glow",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        10.0f,
        false,
        false);

    // ========================================================================
    // 铜蚀/幽匿/试炼/不祥粒子 (99-114)
    // ========================================================================

    // WaxOn: 蜡烛涂抹粒子（上蜡）
    // TODO: 实现 WaxOnParticle，暂时复用 FallingDustParticle
    registry.registerType(ParticleTypeId::WaxOn,
        "minecraft:wax_on",
        FallingDustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        true,
        false);

    // WaxOff: 蜡烛涂抹粒子（除蜡）
    // TODO: 实现 WaxOffParticle，暂时复用 FallingDustParticle
    registry.registerType(ParticleTypeId::WaxOff,
        "minecraft:wax_off",
        FallingDustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        true,
        false);

    // ElectricSpark: 电火花粒子（避雷针等）
    // TODO: 实现 ElectricSparkParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::ElectricSpark,
        "minecraft:electric_spark",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        10.0f,
        false,
        false);

    // Scrape: 刮擦粒子（铜氧化去除）
    // TODO: 实现 ScrapeParticle，暂时复用 FallingDustParticle
    registry.registerType(ParticleTypeId::Scrape,
        "minecraft:scrape",
        FallingDustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        true,
        false);

    // Shriek: 幽匿尖啸体粒子（带延迟数据）
    // TODO: 实现 ShriekParticle，暂时复用 SoulParticle
    registry.registerType(ParticleTypeId::Shriek,
        "minecraft:shriek",
        SoulParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        12.0f,
        false,
        false);

    // EggCrack: 蛋破裂粒子
    // TODO: 实现 EggCrackParticle，暂时复用 PoofParticle
    registry.registerType(ParticleTypeId::EggCrack,
        "minecraft:egg_crack",
        PoofParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    // DustPlume: 尘柱粒子
    // TODO: 实现 DustPlumeParticle（与 DustPillar 不同，DustPlume 不需要方块状态）
    registry.registerType(ParticleTypeId::DustPlume,
        "minecraft:dust_plume",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        false,
        false);

    // TrialSpawnerDetection: 试炼刷怪笼检测粒子
    // TODO: 实现 TrialSpawnerDetectionParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::TrialSpawnerDetection,
        "minecraft:trial_spawner_detection",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    // TrialSpawnerDetectionOminous: 试炼刷怪笼检测粒子（不祥）
    // TODO: 实现 TrialSpawnerDetectionOminousParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::TrialSpawnerDetectionOminous,
        "minecraft:trial_spawner_detection_ominous",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    // VaultConnection: 宝库连接粒子
    // TODO: 实现 VaultConnectionParticle，暂时复用 VibrationSignalParticle
    registry.registerType(ParticleTypeId::VaultConnection,
        "minecraft:vault_connection",
        VibrationSignalParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        60.0f,
        false,
        true);

    // 尘柱粒子（带方块状态，重锤砸地攻击产生）
    registry.registerType(ParticleTypeId::DustPillar,
        "minecraft:dust_pillar",
        DustPillarParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        30.0f,
        true,
        false);

    // OminousSpawning: 不祥生成粒子
    // TODO: 实现 OminousSpawningParticle，暂时复用 EntityEffectParticle
    registry.registerType(ParticleTypeId::OminousSpawning,
        "minecraft:ominous_spawning",
        EntityEffectParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // RaidOmen: 袭击预兆粒子
    // TODO: 实现 RaidOmenParticle，暂时复用 EntityEffectParticle
    registry.registerType(ParticleTypeId::RaidOmen,
        "minecraft:raid_omen",
        EntityEffectParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // TrialOmen: 试炼预兆粒子
    // TODO: 实现 TrialOmenParticle，暂时复用 EntityEffectParticle
    registry.registerType(ParticleTypeId::TrialOmen,
        "minecraft:trial_omen",
        EntityEffectParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // BlockCrumble: 方块碎裂粒子（带方块状态）
    // TODO: 实现 BlockCrumbleParticle，暂时复用 DiggingParticle
    registry.registerType(ParticleTypeId::BlockCrumble,
        "minecraft:block_crumble",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // Firefly: 萤火虫粒子
    // TODO: 实现 FireflyParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::Firefly,
        "minecraft:firefly",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        30.0f,
        false,
        false);

    // ========================================================================
    // 项目内部扩展粒子（115+，不在 MC 协议中）
    // ========================================================================

    // 方块破坏粒子（项目内部，MC 中 Block 兼用此功能）
    registry.registerType(ParticleTypeId::Breaking,
        "minecraft:breaking",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // 屏障粒子（显示屏障方块，项目内部）
    registry.registerType(ParticleTypeId::Barrier,
        "minecraft:barrier",
        BarrierParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        80.0f,
        false,
        false);

    // 光标粒子（显示结构方块位置，项目内部）
    // TODO: 实现 LightParticle，暂时复用 FlameParticle
    registry.registerType(ParticleTypeId::Light,
        "minecraft:light",
        FlameParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        30.0f,
        false,
        false);

    // 红石粉尘粒子（项目内部兼容，MC 中由 Dust + 颜色数据实现）
    registry.registerType(ParticleTypeId::Redstone,
        "minecraft:redstone",
        RedstoneParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    // 大型爆炸粒子（项目内部，MC 中为 HugeExplosion / explosion_emitter）
    registry.registerType(ParticleTypeId::LargeExplosion,
        "minecraft:large_explosion",
        LargeExplosionParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        6.0f,
        false,
        false);

    // 物品拾取粒子（项目内部）
    // TODO: 实现 ItemPickupParticle，暂时复用 DiggingParticle
    registry.registerType(ParticleTypeId::ItemPickup,
        "minecraft:item_pickup",
        DiggingParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        10.0f,
        true,
        false);

    // 滴落的樱花树叶（项目内部，MC 中仅有 CherryLeaves）
    // TODO: 实现 DrippingCherryLeavesParticle
    registry.registerType(ParticleTypeId::DrippingCherryLeaves,
        "minecraft:dripping_cherry_leaves",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        true,
        false);

    // 下落的樱花树叶（项目内部，MC 中仅有 CherryLeaves）
    // TODO: 实现 FallingCherryLeavesParticle
    registry.registerType(ParticleTypeId::FallingCherryLeaves,
        "minecraft:falling_cherry_leaves",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        true,
        false);

    // 落地的樱花树叶（项目内部，MC 中仅有 CherryLeaves）
    // TODO: 实现 LandingCherryLeavesParticle
    registry.registerType(ParticleTypeId::LandingCherryLeaves,
        "minecraft:landing_cherry_leaves",
        CloudParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        true,
        false);
}

} // namespace mc::client::renderer::trident::particle
