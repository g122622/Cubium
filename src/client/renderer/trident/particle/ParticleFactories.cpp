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

// 粒子数据类型头文件
#include "data/DustParticleData.hpp"
#include "data/EntityEffectParticleData.hpp"
#include "data/ItemParticleData.hpp"
#include "data/TrailParticleData.hpp"
#include "data/VibrationParticleData.hpp"

// 粒子类型头文件
#include "particles/RainParticle.hpp"
#include "particles/SnowParticle.hpp"
#include "particles/ambient/BubbleParticle.hpp"
#include "particles/ambient/BubblePopParticle.hpp"
#include "particles/ambient/CloudParticle.hpp"
#include "particles/ambient/NetherSporeParticle.hpp"
#include "particles/ambient/SporeBlossomParticle.hpp"
#include "particles/ambient/SuspendedTownParticle.hpp"
#include "particles/ambient/UnderwaterParticle.hpp"
#include "particles/block/ComposterParticle.hpp"
#include "particles/block/DiggingParticle.hpp"
#include "particles/block/DustPillarParticle.hpp"
#include "particles/block/ItemParticle.hpp"
#include "particles/block/LeavesParticle.hpp"
#include "particles/block/ScrapeParticle.hpp"
#include "particles/block/WaxParticle.hpp"
#include "particles/effect/AshParticle.hpp"
#include "particles/effect/CampfireParticle.hpp"
#include "particles/effect/CopperFireFlameParticle.hpp"
#include "particles/effect/CritParticle.hpp"
#include "particles/effect/DamageIndicatorParticle.hpp"
#include "particles/effect/DragonBreathParticle.hpp"
#include "particles/effect/DustParticle.hpp"
#include "particles/effect/DustPlumeParticle.hpp"
#include "particles/effect/EggCrackParticle.hpp"
#include "particles/effect/ElderGuardianParticle.hpp"
#include "particles/effect/ElectricSparkParticle.hpp"
#include "particles/effect/EmitterParticle.hpp"
#include "particles/effect/ExplosionParticle.hpp"
#include "particles/effect/FireflyParticle.hpp"
#include "particles/effect/FireworkParticle.hpp"
#include "particles/effect/FlameParticle.hpp"
#include "particles/effect/FlashParticle.hpp"
#include "particles/effect/GlowParticle.hpp"
#include "particles/effect/GustEmitterParticle.hpp"
#include "particles/effect/GustParticle.hpp"
#include "particles/effect/InfestedParticle.hpp"
#include "particles/effect/ItemPickupParticle.hpp"
#include "particles/effect/LavaParticle.hpp"
#include "particles/effect/LightParticle.hpp"
#include "particles/effect/NoteParticle.hpp"
#include "particles/effect/OmenParticle.hpp"
#include "particles/effect/PoofParticle.hpp"
#include "particles/effect/PortalParticle.hpp"
#include "particles/effect/RedstoneParticle.hpp"
#include "particles/effect/SculkChargeParticle.hpp"
#include "particles/effect/SculkSoulParticle.hpp"
#include "particles/effect/ShriekParticle.hpp"
#include "particles/effect/SmallFlameParticle.hpp"
#include "particles/effect/SmokeParticle.hpp"
#include "particles/effect/SonicBoomParticle.hpp"
#include "particles/effect/SoulParticle.hpp"
#include "particles/effect/SpellParticle.hpp"
#include "particles/effect/TrialSpawnerParticle.hpp"
#include "particles/effect/WhiteSmokeParticle.hpp"
#include "particles/liquid/CherryLeavesDripParticle.hpp"
#include "particles/liquid/DripParticle.hpp"
#include "particles/liquid/DripWaterParticle.hpp"
#include "particles/liquid/DripstoneDripParticle.hpp"
#include "particles/liquid/FallingNectarParticle.hpp"
#include "particles/mob/HeartParticle.hpp"
#include "particles/mob/SpitParticle.hpp"
#include "particles/mob/SquidInkParticle.hpp"
#include "particles/mob/TotemParticle.hpp"
#include "particles/mob/VillagerParticle.hpp"
#include "particles/special/NautilusParticle.hpp"
#include "particles/special/TrailParticle.hpp"
#include "particles/special/VaultConnectionParticle.hpp"
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

    // BlockMarker: 方块标记粒子（静态显示方块纹理，用于结构方块等标记显示）
    registry.registerType(ParticleTypeId::BlockMarker,
        "minecraft:block_marker",
        BlockMarkerParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        80.0f,
        false,
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

    // CopperFireFlame: 铜火火焰粒子
    registry.registerType(ParticleTypeId::CopperFireFlame,
        "minecraft:copper_fire_flame",
        CopperFireFlameParticle::create,
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

    // DamageIndicator: 伤害指示器粒子（实体受伤时弹出，向上飘散）
    registry.registerType(ParticleTypeId::DamageIndicator,
        "minecraft:damage_indicator",
        DamageIndicatorParticle::create,
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
    registry.registerType(ParticleTypeId::Dust,
        "minecraft:dust",
        DustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    // DustColorTransition: 颜色过渡染色粒子
    registry.registerType(ParticleTypeId::DustColorTransition,
        "minecraft:dust_color_transition",
        DustColorTransitionParticle::create,
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
    registry.registerType(ParticleTypeId::ElderGuardian,
        "minecraft:elder_guardian",
        ElderGuardianParticle::create,
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
    registry.registerType(ParticleTypeId::Gust,
        "minecraft:gust",
        GustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // SmallGust: 小型风爆粒子
    registry.registerType(ParticleTypeId::SmallGust,
        "minecraft:small_gust",
        SmallGustParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // GustEmitterLarge: 大型风爆发射器粒子
    registry.registerType(ParticleTypeId::GustEmitterLarge,
        "minecraft:gust_emitter_large",
        GustEmitterLargeParticle::create,
        ParticleRenderType::NO_RENDER,
        8.0f,
        false,
        false);

    // GustEmitterSmall: 小型风爆发射器粒子
    registry.registerType(ParticleTypeId::GustEmitterSmall,
        "minecraft:gust_emitter_small",
        GustEmitterSmallParticle::create,
        ParticleRenderType::NO_RENDER,
        8.0f,
        false,
        false);

    // SonicBoom: 声波轰击粒子（监守者远程攻击）
    registry.registerType(ParticleTypeId::SonicBoom,
        "minecraft:sonic_boom",
        SonicBoomParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
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
    registry.registerType(ParticleTypeId::Firework,
        "minecraft:firework",
        FireworkParticle::create,
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
    registry.registerType(ParticleTypeId::Infested,
        "minecraft:infested",
        InfestedParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // CherryLeaves: 樱花树叶粒子
    registry.registerType(ParticleTypeId::CherryLeaves,
        "minecraft:cherry_leaves",
        CherryLeavesParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        40.0f,
        true,
        false);

    // PaleOakLeaves: 苍白橡树树叶粒子
    registry.registerType(ParticleTypeId::PaleOakLeaves,
        "minecraft:pale_oak_leaves",
        PaleOakLeavesParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        40.0f,
        true,
        false);

    // TintedLeaves: 着色树叶粒子（带颜色数据）
    registry.registerType(ParticleTypeId::TintedLeaves,
        "minecraft:tinted_leaves",
        TintedLeavesParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        40.0f,
        true,
        false);

    // SculkSoul: 幽匿灵魂粒子
    registry.registerType(ParticleTypeId::SculkSoul,
        "minecraft:sculk_soul",
        SculkSoulParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        12.0f,
        false,
        false);

    // SculkCharge: 幽匿充能粒子（带充能数据）
    registry.registerType(ParticleTypeId::SculkCharge,
        "minecraft:sculk_charge",
        SculkChargeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        10.0f,
        false,
        false);

    // SculkChargePop: 幽匿充能弹出粒子
    registry.registerType(ParticleTypeId::SculkChargePop,
        "minecraft:sculk_charge_pop",
        SculkChargePopParticle::create,
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

    // Flash: 闪光粒子
    registry.registerType(ParticleTypeId::Flash,
        "minecraft:flash",
        FlashParticle::create,
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
    registry.registerType(ParticleTypeId::Composter,
        "minecraft:composter",
        ComposterParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        5.0f,
        false,
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

    // Item: 物品粒子（物品破碎效果）
    registry.registerType(ParticleTypeId::Item,
        "minecraft:item",
        ItemParticle::create,
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

    // Trail: 轨迹粒子（带颜色+目标数据）
    registry.registerType(ParticleTypeId::Trail,
        "minecraft:trail",
        TrailParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        false,
        false);

    // 史莱姆物品粒子
    registry.registerType(ParticleTypeId::ItemSlime,
        "minecraft:item_slime",
        ItemParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // 蛛网物品粒子
    registry.registerType(ParticleTypeId::ItemCobweb,
        "minecraft:item_cobweb",
        ItemParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        20.0f,
        true,
        false);

    // 雪球物品粒子
    registry.registerType(ParticleTypeId::ItemSnowball,
        "minecraft:item_snowball",
        ItemParticle::create,
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

    // 菌丝悬浮粒子
    registry.registerType(ParticleTypeId::Mycelium,
        "minecraft:mycelium",
        SuspendedTownParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f,
        false,
        false);

    // Note: 音符粒子（音符盒）
    registry.registerType(ParticleTypeId::Note,
        "minecraft:note",
        NoteParticle::create,
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
    registry.registerType(ParticleTypeId::Spit,
        "minecraft:spit",
        SpitParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        6.0f,
        true,
        false);

    // SquidInk: 鱿鱼墨汁粒子
    registry.registerType(ParticleTypeId::SquidInk,
        "minecraft:squid_ink",
        SquidInkParticle::create,
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
    registry.registerType(ParticleTypeId::TotemOfUndying,
        "minecraft:totem_of_undying",
        TotemParticle::create,
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
    registry.registerType(ParticleTypeId::CurrentDown,
        "minecraft:current_down",
        CurrentDownParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        8.0f,
        false,
        false);

    // BubbleColumnUp: 气泡柱上升粒子
    registry.registerType(ParticleTypeId::BubbleColumnUp,
        "minecraft:bubble_column_up",
        BubbleColumnUpParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
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
    registry.registerType(ParticleTypeId::FallingNectar,
        "minecraft:falling_nectar",
        FallingNectarParticle::create,
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
    registry.registerType(ParticleTypeId::Ash,
        "minecraft:ash",
        AshParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    // 绯红孢子
    registry.registerType(ParticleTypeId::CrimsonSpore,
        "minecraft:crimson_spore",
        CrimsonSporeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        20.0f,
        false,
        false);

    // 诡异孢子
    registry.registerType(ParticleTypeId::WarpedSpore,
        "minecraft:warped_spore",
        WarpedSporeParticle::create,
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
    registry.registerType(ParticleTypeId::ReversePortal,
        "minecraft:reverse_portal",
        ReversePortalParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        45.0f,
        false,
        false);

    // WhiteAsh: 白色灰烬粒子
    registry.registerType(ParticleTypeId::WhiteAsh,
        "minecraft:white_ash",
        WhiteAshParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    // SmallFlame: 小型火焰粒子（蜡烛等）
    registry.registerType(ParticleTypeId::SmallFlame,
        "minecraft:small_flame",
        SmallFlameParticle::create,
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
    registry.registerType(ParticleTypeId::GlowSquidInk,
        "minecraft:glow_squid_ink",
        GlowSquidInkParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // Glow: 荧光地衣粒子
    registry.registerType(ParticleTypeId::Glow,
        "minecraft:glow",
        GlowParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        10.0f,
        false,
        false);

    // ========================================================================
    // 铜蚀/幽匿/试炼/不祥粒子 (99-114)
    // ========================================================================

    // WaxOn: 涂蜡粒子
    registry.registerType(ParticleTypeId::WaxOn,
        "minecraft:wax_on",
        WaxOnParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        25.0f,
        false,
        false);

    // WaxOff: 除蜡粒子
    registry.registerType(ParticleTypeId::WaxOff,
        "minecraft:wax_off",
        WaxOffParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        25.0f,
        false,
        false);

    // ElectricSpark: 电火花粒子（避雷针等）
    registry.registerType(ParticleTypeId::ElectricSpark,
        "minecraft:electric_spark",
        ElectricSparkParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        10.0f,
        false,
        false);

    // Scrape: 刮擦粒子
    registry.registerType(ParticleTypeId::Scrape,
        "minecraft:scrape",
        ScrapeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        25.0f,
        false,
        false);

    // Shriek: 幽匿尖啸体粒子（带延迟数据）
    registry.registerType(ParticleTypeId::Shriek,
        "minecraft:shriek",
        ShriekParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        12.0f,
        false,
        false);

    // EggCrack: 蛋破裂粒子
    registry.registerType(ParticleTypeId::EggCrack,
        "minecraft:egg_crack",
        EggCrackParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        8.0f,
        false,
        false);

    // DustPlume: 尘柱粒子（不需要方块状态）
    registry.registerType(ParticleTypeId::DustPlume,
        "minecraft:dust_plume",
        DustPlumeParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        false,
        false);

    // TrialSpawnerDetection: 试炼刷怪笼检测粒子
    registry.registerType(ParticleTypeId::TrialSpawnerDetection,
        "minecraft:trial_spawner_detection",
        TrialSpawnerDetectionParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    // TrialSpawnerDetectionOminous: 试炼刷怪笼检测粒子（不祥）
    registry.registerType(ParticleTypeId::TrialSpawnerDetectionOminous,
        "minecraft:trial_spawner_detection_ominous",
        TrialSpawnerDetectionOminousParticle::create,
        ParticleRenderType::PARTICLE_SHEET_LIT,
        20.0f,
        false,
        false);

    // 宝库连接粒子
    registry.registerType(ParticleTypeId::VaultConnection,
        "minecraft:vault_connection",
        VaultConnectionParticle::create,
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
    registry.registerType(ParticleTypeId::OminousSpawning,
        "minecraft:ominous_spawning",
        OminousSpawningParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // RaidOmen: 袭击预兆粒子
    registry.registerType(ParticleTypeId::RaidOmen,
        "minecraft:raid_omen",
        RaidOmenParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // TrialOmen: 试炼预兆粒子
    registry.registerType(ParticleTypeId::TrialOmen,
        "minecraft:trial_omen",
        TrialOmenParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        10.0f,
        false,
        false);

    // BlockCrumble: 方块碎裂粒子（带方块状态）
    registry.registerType(ParticleTypeId::BlockCrumble,
        "minecraft:block_crumble",
        BlockCrumbleParticle::create,
        ParticleRenderType::TERRAIN_SHEET,
        15.0f,
        true,
        false);

    // Firefly: 萤火虫粒子
    registry.registerType(ParticleTypeId::Firefly,
        "minecraft:firefly",
        FireflyParticle::create,
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

    // 光标粒子（显示结构方块位置）
    registry.registerType(ParticleTypeId::Light,
        "minecraft:light",
        LightParticle::create,
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

    // 物品拾取粒子
    registry.registerType(ParticleTypeId::ItemPickup,
        "minecraft:item_pickup",
        ItemPickupParticle::create,
        ParticleRenderType::PARTICLE_SHEET_OPAQUE,
        10.0f,
        false,
        false);

    // 滴落的樱花树叶
    registry.registerType(ParticleTypeId::DrippingCherryLeaves,
        "minecraft:dripping_cherry_leaves",
        DrippingCherryLeavesParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        40.0f,
        true,
        false);

    // 下落的樱花树叶
    registry.registerType(ParticleTypeId::FallingCherryLeaves,
        "minecraft:falling_cherry_leaves",
        FallingCherryLeavesParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        30.0f,
        true,
        false);

    // 落地的樱花树叶
    registry.registerType(ParticleTypeId::LandingCherryLeaves,
        "minecraft:landing_cherry_leaves",
        LandingCherryLeavesParticle::create,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        20.0f,
        false,
        false);

    // ========================================================================
    // 数据工厂注册
    //
    // 为需要额外数据的粒子类型注册 ParticleDataFactory，
    // 使得通过 createParticleWithData() 传入 ParticleData 时能正确创建粒子。
    // 当 data 为 nullptr 时，回退到普通工厂（使用默认值）。
    // ========================================================================

    // 振动信号粒子：从 VibrationParticleData 提取目标来源和到达时间
    // - 方块来源：使用固定目标位置
    // - 实体来源：持有实体 ID + yOffset，每 tick 通过 ClientWorld 动态解析
    registry.registerDataFactory(ParticleTypeId::Vibration,
        [](const glm::vec3& pos,
            const glm::vec3& velocity,
            mc::client::ClientWorld* world,
            const data::ParticleData* data) -> std::unique_ptr<Particle> {
            if (data) {
                auto* vibrationData = dynamic_cast<const data::VibrationParticleData*>(data);
                if (vibrationData) {
                    if (vibrationData->isEntitySource()) {
                        return VibrationSignalParticle::createWithEntityTarget(pos,
                            vibrationData->targetEntityId(),
                            vibrationData->yOffset(),
                            vibrationData->arrivalInTicks());
                    }
                    return VibrationSignalParticle::createWithTarget(
                        pos, vibrationData->targetPosition(), vibrationData->arrivalInTicks());
                }
            }
            // 回退到默认工厂（向上飞8格，8 tick）
            return VibrationSignalParticle::create(pos, velocity, world);
        });

    // 轨迹粒子：从 TrailParticleData 提取目标位置、颜色和持续时间
    registry.registerDataFactory(ParticleTypeId::Trail,
        [](const glm::vec3& pos,
            const glm::vec3& velocity,
            mc::client::ClientWorld* world,
            const data::ParticleData* data) -> std::unique_ptr<Particle> {
            if (data) {
                auto* trailData = dynamic_cast<const data::TrailParticleData*>(data);
                if (trailData) {
                    return TrailParticle::createWithTarget(
                        pos, trailData->targetPosition(), trailData->color(), trailData->durationInTicks());
                }
            }
            // 回退到默认工厂（velocity 作为目标偏移，白色，10 tick）
            return TrailParticle::create(pos, velocity, world);
        });

    // 宝库连接粒子：MC Java 中 vault_connection 是 SimpleParticleType，网络层通过 velocity 字段传递目标偏移，
    // 不需要专门的 ParticleData。但为支持编程方式创建（通过 addParticleWithData），
    // 注册使用 VibrationParticleData 的数据工厂（方块来源的 targetPosition + arrivalInTicks 模式相同）。
    // 宝库连接粒子始终飞向固定方块位置，实体来源变体在此回退到默认工厂。
    registry.registerDataFactory(ParticleTypeId::VaultConnection,
        [](const glm::vec3& pos,
            const glm::vec3& velocity,
            mc::client::ClientWorld* world,
            const data::ParticleData* data) -> std::unique_ptr<Particle> {
            if (data) {
                auto* vibrationData = dynamic_cast<const data::VibrationParticleData*>(data);
                if (vibrationData && vibrationData->isBlockSource()) {
                    return VaultConnectionParticle::createWithTarget(
                        pos, vibrationData->targetPosition(), vibrationData->arrivalInTicks());
                }
            }
            // 回退到默认工厂（velocity 作为目标偏移，30~39 tick 随机生命周期）
            return VaultConnectionParticle::create(pos, velocity, world);
        });

    // 灰尘粒子：从 DustParticleData 提取颜色和缩放数据
    // Dust 和 Redstone 共享相同的数据格式（i32 color(ARGB) + f32 scale）
    registry.registerDataFactory(ParticleTypeId::Dust,
        [](const glm::vec3& pos,
            const glm::vec3& velocity,
            mc::client::ClientWorld* world,
            const data::ParticleData* data) -> std::unique_ptr<Particle> {
            if (data) {
                auto* dustData = dynamic_cast<const data::DustParticleData*>(data);
                if (dustData) {
                    return DustParticle::createWithColor(pos, velocity, world, dustData->toRGBAVector());
                }
            }
            // 回退到默认工厂（红色灰尘粒子）
            return DustParticle::create(pos, velocity, world);
        });

    // 红石粒子：Redstone 是项目内部扩展类型，使用 DustParticleData（与 Dust 相同的 ARGB 颜色格式）
    registry.registerDataFactory(ParticleTypeId::Redstone,
        [](const glm::vec3& pos,
            const glm::vec3& velocity,
            mc::client::ClientWorld* world,
            const data::ParticleData* data) -> std::unique_ptr<Particle> {
            if (data) {
                auto* dustData = dynamic_cast<const data::DustParticleData*>(data);
                if (dustData) {
                    return DustParticle::createWithColor(pos, velocity, world, dustData->toRGBAVector());
                }
            }
            // 回退到默认工厂（红色红石粒子）
            return RedstoneParticle::create(pos, velocity, world);
        });

    // 颜色过渡灰尘粒子：从 DustColorTransitionParticleData 提取起始颜色、目标颜色和缩放数据
    registry.registerDataFactory(ParticleTypeId::DustColorTransition,
        [](const glm::vec3& pos,
            const glm::vec3& velocity,
            mc::client::ClientWorld* world,
            const data::ParticleData* data) -> std::unique_ptr<Particle> {
            if (data) {
                auto* transitionData = dynamic_cast<const data::DustColorTransitionParticleData*>(data);
                if (transitionData) {
                    return DustColorTransitionParticle::createWithColors(pos,
                        velocity,
                        world,
                        transitionData->fromColorToRGBAVector(),
                        transitionData->toColorToRGBAVector());
                }
            }
            // 回退到默认工厂（红到蓝颜色过渡）
            return DustColorTransitionParticle::create(pos, velocity, world);
        });

    // 实体效果粒子：从 EntityEffectParticleData 提取 ARGB 颜色数据
    // 对应 MC Java 的 ColorParticleOption(ENTITY_EFFECT)
    registry.registerDataFactory(ParticleTypeId::EntityEffect,
        [](const glm::vec3& pos,
            const glm::vec3& velocity,
            mc::client::ClientWorld* world,
            const data::ParticleData* data) -> std::unique_ptr<Particle> {
            if (data) {
                auto* effectData = dynamic_cast<const data::EntityEffectParticleData*>(data);
                if (effectData) {
                    return EntityEffectParticle::createWithColor(pos, velocity, world, effectData->toRGBAVector());
                }
            }
            // 回退到默认工厂（紫色药水效果粒子）
            return EntityEffectParticle::create(pos, velocity, world);
        });

    // 物品粒子：从 ItemParticleData 提取 ItemStack，调用 createWithItemStack 创建粒子。
    // Item、ItemSlime、ItemCobweb、ItemSnowball 共享相同的 ItemParticle 类与数据格式。
    // 对应 MC Java 1.21.11 的 ItemParticleProvider，其通过 ItemStack 解析物品纹理。
    auto itemDataFactory = [](const glm::vec3& pos,
                               const glm::vec3& velocity,
                               mc::client::ClientWorld* world,
                               const data::ParticleData* data) -> std::unique_ptr<Particle> {
        if (data) {
            auto* itemData = dynamic_cast<const data::ItemParticleData*>(data);
            if (itemData) {
                return particles::ItemParticle::createWithItemStack(pos, velocity, itemData->getItemStack());
            }
        }
        // 回退到默认工厂（占位纹理）
        MC_UNUSED(world);
        return particles::ItemParticle::create(pos, velocity, world);
    };

    registry.registerDataFactory(ParticleTypeId::Item, itemDataFactory);
    registry.registerDataFactory(ParticleTypeId::ItemSlime, itemDataFactory);
    registry.registerDataFactory(ParticleTypeId::ItemCobweb, itemDataFactory);
    registry.registerDataFactory(ParticleTypeId::ItemSnowball, itemDataFactory);
}

} // namespace mc::client::renderer::trident::particle
