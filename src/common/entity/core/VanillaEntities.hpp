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

#pragma once

#include "../entities/monster/MonsterEntity.hpp"
#include "../entities/monster/arthropod/CaveSpiderEntity.hpp"
#include "../entities/monster/arthropod/EndermiteEntity.hpp"
#include "../entities/monster/arthropod/SpiderEntity.hpp"
#include "../entities/monster/basic/CreeperEntity.hpp"
#include "../entities/monster/basic/GiantEntity.hpp"
#include "../entities/monster/basic/PhantomEntity.hpp"
#include "../entities/monster/basic/SlimeEntity.hpp"
#include "../entities/monster/end/EndermanEntity.hpp"
#include "../entities/monster/end/ShulkerEntity.hpp"
#include "../entities/monster/illager/EvokerEntity.hpp"
#include "../entities/monster/illager/IllagerEntities.hpp"
#include "../entities/monster/illager/IllusionerEntity.hpp"
#include "../entities/monster/illager/RavagerEntity.hpp"
#include "../entities/monster/illager/VexEntity.hpp"
#include "../entities/monster/illager/WitchEntity.hpp"
#include "../entities/monster/nether/BlazeEntity.hpp"
#include "../entities/monster/nether/NetherEntities.hpp"
#include "../entities/monster/ocean/ElderGuardianEntity.hpp"
#include "../entities/monster/ocean/GuardianEntity.hpp"
#include "../entities/monster/undead/DrownedEntity.hpp"
#include "../entities/monster/undead/HuskEntity.hpp"
#include "../entities/monster/undead/SkeletonEntity.hpp"
#include "../entities/monster/undead/StrayEntity.hpp"
#include "../entities/monster/undead/WitherSkeletonEntity.hpp"
#include "../entities/monster/undead/ZombieEntity.hpp"
#include "../entities/passive/ambient/AmbientEntity.hpp"
#include "../entities/passive/ambient/BatEntity.hpp"
#include "../entities/passive/basic/ChickenEntity.hpp"
#include "../entities/passive/basic/CowEntity.hpp"
#include "../entities/passive/basic/MooshroomEntity.hpp"
#include "../entities/passive/basic/PigEntity.hpp"
#include "../entities/passive/basic/RabbitEntity.hpp"
#include "../entities/passive/basic/SheepEntity.hpp"
#include "../entities/passive/fish/AbstractFishEntity.hpp"
#include "../entities/passive/fish/CodEntity.hpp"
#include "../entities/passive/fish/PufferfishEntity.hpp"
#include "../entities/passive/fish/SalmonEntity.hpp"
#include "../entities/passive/fish/TropicalFishEntity.hpp"
#include "../entities/passive/golem/GolemEntity.hpp"
#include "../entities/passive/golem/IronGolemEntity.hpp"
#include "../entities/passive/golem/SnowGolemEntity.hpp"
#include "../entities/passive/horse/AbstractHorseEntity.hpp"
#include "../entities/passive/horse/DonkeyEntity.hpp"
#include "../entities/passive/horse/HorseEntity.hpp"
#include "../entities/passive/horse/LlamaEntity.hpp"
#include "../entities/passive/horse/MuleEntity.hpp"
#include "../entities/passive/horse/SkeletonHorseEntity.hpp"
#include "../entities/passive/horse/ZombieHorseEntity.hpp"
#include "../entities/passive/special/BeeEntity.hpp"
#include "../entities/passive/special/FoxEntity.hpp"
#include "../entities/passive/special/PandaEntity.hpp"
#include "../entities/passive/special/PolarBearEntity.hpp"
#include "../entities/passive/special/StriderEntity.hpp"
#include "../entities/passive/special/TurtleEntity.hpp"
#include "../entities/passive/tamable/CatEntity.hpp"
#include "../entities/passive/tamable/OcelotEntity.hpp"
#include "../entities/passive/tamable/ParrotEntity.hpp"
#include "../entities/passive/tamable/WolfEntity.hpp"
#include "../entities/passive/water/DolphinEntity.hpp"
#include "../entities/passive/water/SquidEntity.hpp"
#include "../entities/passive/water/WaterMobEntity.hpp"
#include "EntityRegistry.hpp"
#include "EntitySpawnPlacementRegistry.hpp"
#include "EntityType.hpp"
// #include "../entities/boss/EnderDragonEntity.hpp"
// #include "../entities/boss/WitherEntity.hpp"
// #include "../entities/villager/VillagerEntity.hpp"
// #include "../entities/projectile/ProjectileEntity.hpp"
// #include "../entities/projectile/AbstractArrowEntity.hpp"
// #include "../entities/projectile/TridentEntity.hpp"
// #include "../entities/projectile/ProjectileItemEntity.hpp"
// #include "../entities/projectile/AbstractFireballEntity.hpp"
#include "../entities/projectile/OtherProjectiles.hpp"
#include "../entities/item/ItemEntity.hpp"
#include "../entities/misc/MiscEntities.hpp"
#include "../entities/orb/ExperienceOrbEntity.hpp"
#include "../entities/effect/EffectEntities.hpp"
#include "../entities/vehicle/BoatEntity.hpp"
#include "../entities/vehicle/MinecartEntity.hpp"
#include <mutex>
#include <spdlog/spdlog.h>

namespace mc {
namespace entity {

/**
 * @brief 原版实体初始化器
 *
 * 注册所有原版实体类型到实体注册表。
 * 必须在服务器启动时或客户端初始化时调用。
 *
 * 参考 MC 1.16.5 EntityType 注册
 */
class VanillaEntities {
public:
    /**
     * @brief 注册所有原版实体类型
     *
     * 包括动物、怪物和其他实体类型。
     * 此方法线程安全，可以安全地多次调用，后续调用将被忽略。
     */
    static void registerAll()
    {
        std::call_once(s_onceFlag, []() { doRegisterAll(); });
    }

    /**
     * @brief 获取实体类型的本地化名称
     * @param typeId 实体类型ID
     * @return 本地化名称键（如 entity.minecraft.pig）
     */
    static std::string getLocalizedNameKey(EntityTypeId typeId)
    {
        const auto* type = EntityRegistry::instance().getType(typeId);
        if (!type) {
            return "entity.minecraft.unknown";
        }

        // 将 minecraft:pig 转换为 entity.minecraft.pig
        const std::string& name = type->name();
        if (name.find(':') != std::string::npos) {
            return "entity." + name;
        }
        return "entity.minecraft." + name;
    }

private:
    static inline std::once_flag s_onceFlag;

    static void doRegisterAll()
    {
        auto& registry = EntityRegistry::instance();

        // ========== 动物 ==========
        // 猪
        registry.registerType(EntityTypes::PIG,
            EntityType::Builder(&PigEntity::create, EntityClassification::Creature)
                .size(0.9f, 0.9f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 牛
        registry.registerType(EntityTypes::COW,
            EntityType::Builder(&CowEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.4f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 羊
        registry.registerType(EntityTypes::SHEEP,
            EntityType::Builder(&SheepEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.3f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 鸡
        registry.registerType(EntityTypes::CHICKEN,
            EntityType::Builder(&ChickenEntity::create, EntityClassification::Creature)
                .size(0.4f, 0.7f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 兔子
        registry.registerType(EntityTypes::RABBIT,
            EntityType::Builder(&RabbitEntity::create, EntityClassification::Creature)
                .size(0.4f, 0.5f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 哞菇
        registry.registerType(EntityTypes::MOOSHROOM,
            EntityType::Builder(&MooshroomEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.4f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 可驯服动物 ==========
        // 狼
        registry.registerType(EntityTypes::WOLF,
            EntityType::Builder(&WolfEntity::create, EntityClassification::Creature)
                .size(0.6f, 0.85f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 猫
        registry.registerType(EntityTypes::CAT,
            EntityType::Builder(&CatEntity::create, EntityClassification::Creature)
                .size(0.6f, 0.7f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 特殊动物 ==========
        // 狐狸
        registry.registerType(EntityTypes::FOX,
            EntityType::Builder(&FoxEntity::create, EntityClassification::Creature)
                .size(0.6f, 0.7f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 熊猫
        registry.registerType(EntityTypes::PANDA,
            EntityType::Builder(&PandaEntity::create, EntityClassification::Creature)
                .size(1.3f, 1.25f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 北极熊
        registry.registerType(EntityTypes::POLAR_BEAR,
            EntityType::Builder(&PolarBearEntity::create, EntityClassification::Creature)
                .size(1.4f, 1.4f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 海龟
        registry.registerType(EntityTypes::TURTLE,
            EntityType::Builder(&TurtleEntity::create, EntityClassification::Creature)
                .size(1.2f, 0.4f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 蜜蜂
        registry.registerType(EntityTypes::BEE,
            EntityType::Builder(&BeeEntity::create, EntityClassification::Creature)
                .size(0.4f, 0.3f) // MC 1.16.5: 0.4 x 0.3
                .trackingRange(8)
                .updateInterval(1) // 蜜蜂更新频繁
                .canSummon(true)
                .build());

        // 炽足兽
        registry.registerType(EntityTypes::STRIDER,
            EntityType::Builder(&StriderEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.8f) // MC 1.16.5: 0.9 x 1.8
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 豹猫
        registry.registerType(EntityTypes::OCELOT,
            EntityType::Builder(&OcelotEntity::create, EntityClassification::Creature)
                .size(0.6f, 0.7f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 鹦鹉
        registry.registerType(EntityTypes::PARROT,
            EntityType::Builder(&ParrotEntity::create, EntityClassification::Creature)
                .size(0.5f, 0.9f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 水生生物 ==========
        // 鱿鱼
        registry.registerType(EntityTypes::SQUID,
            EntityType::Builder(&SquidEntity::create, EntityClassification::WaterCreature)
                .size(0.8f, 0.8f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 海豚
        registry.registerType(EntityTypes::DOLPHIN,
            EntityType::Builder(&DolphinEntity::create, EntityClassification::WaterCreature)
                .size(0.9f, 0.6f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 鱼类 ==========
        // 鳕鱼
        registry.registerType(EntityTypes::COD,
            EntityType::Builder(&CodEntity::create, EntityClassification::WaterCreature)
                .size(0.5f, 0.3f)
                .trackingRange(8) // MC 1.16.5: 鱼类追踪范围为 8
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 鲑鱼
        registry.registerType(EntityTypes::SALMON,
            EntityType::Builder(&SalmonEntity::create, EntityClassification::WaterCreature)
                .size(0.7f, 0.4f)
                .trackingRange(8) // MC 1.16.5: 鱼类追踪范围为 8
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 河豚
        registry.registerType(EntityTypes::PUFFERFISH,
            EntityType::Builder(&PufferfishEntity::create, EntityClassification::WaterCreature)
                .size(0.7f, 0.7f)
                .trackingRange(8) // MC 1.16.5: 鱼类追踪范围为 8
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 热带鱼
        registry.registerType(EntityTypes::TROPICAL_FISH,
            EntityType::Builder(&TropicalFishEntity::create, EntityClassification::WaterCreature)
                .size(0.5f, 0.4f)
                .trackingRange(8) // MC 1.16.5: 鱼类追踪范围为 8
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 环境生物 ==========
        // 蝙蝠
        registry.registerType(EntityTypes::BAT,
            EntityType::Builder(&BatEntity::create, EntityClassification::Ambient)
                .size(0.5f, 0.9f)
                .trackingRange(5)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 傀儡 ==========
        // 铁傀儡
        registry.registerType(EntityTypes::IRON_GOLEM,
            EntityType::Builder(&IronGolemEntity::create, EntityClassification::Misc)
                .size(1.4f, 2.7f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 雪傀儡
        registry.registerType(EntityTypes::SNOW_GOLEM,
            EntityType::Builder(&SnowGolemEntity::create, EntityClassification::Misc)
                .size(0.7f, 1.9f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 怪物 ==========
        // 僵尸
        registry.registerType(EntityTypes::ZOMBIE,
            EntityType::Builder(&ZombieEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 骷髅
        registry.registerType(EntityTypes::SKELETON,
            EntityType::Builder(&SkeletonEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.99f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 苦力怕
        registry.registerType(EntityTypes::CREEPER,
            EntityType::Builder(&CreeperEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.7f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 蜘蛛
        registry.registerType(EntityTypes::SPIDER,
            EntityType::Builder(&SpiderEntity::create, EntityClassification::Monster)
                .size(1.4f, 0.9f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 末影人
        registry.registerType(EntityTypes::ENDERMAN,
            EntityType::Builder(&EndermanEntity::create, EntityClassification::Monster)
                .size(0.6f, 2.9f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 烈焰人
        registry.registerType(EntityTypes::BLAZE,
            EntityType::Builder(&BlazeEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.8f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 女巫
        registry.registerType(EntityTypes::WITCH,
            EntityType::Builder(&WitchEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 史莱姆
        registry.registerType(EntityTypes::SLIME,
            EntityType::Builder(&SlimeEntity::create, EntityClassification::Monster)
                .size(0.6f, 0.6f) // 尺寸会动态变化
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 海洋怪物 ==========
        // 守卫者
        registry.registerType(EntityTypes::GUARDIAN,
            EntityType::Builder(&GuardianEntity::create, EntityClassification::Monster)
                .size(0.85f, 0.85f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 远古守卫者
        registry.registerType(EntityTypes::ELDER_GUARDIAN,
            EntityType::Builder(&ElderGuardianEntity::create, EntityClassification::Monster)
                .size(1.9975f, 1.9975f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 亡灵变种 ==========
        // 尸壳
        registry.registerType(EntityTypes::HUSK,
            EntityType::Builder(&HuskEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 溺尸
        registry.registerType(EntityTypes::DROWNED,
            EntityType::Builder(&DrownedEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 流浪者
        registry.registerType(EntityTypes::STRAY,
            EntityType::Builder(&StrayEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.99f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 凋灵骷髅
        registry.registerType(EntityTypes::WITHER_SKELETON,
            EntityType::Builder(&WitherSkeletonEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.99f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 节肢动物变种 ==========
        // 洞穴蜘蛛
        registry.registerType(EntityTypes::CAVE_SPIDER,
            EntityType::Builder(&CaveSpiderEntity::create, EntityClassification::Monster)
                .size(0.7f, 0.5f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 马类 ==========
        // 马
        registry.registerType(EntityTypes::HORSE,
            EntityType::Builder(&HorseEntity::create, EntityClassification::Creature)
                .size(1.3964844f, 1.6f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 驴
        registry.registerType(EntityTypes::DONKEY,
            EntityType::Builder(&DonkeyEntity::create, EntityClassification::Creature)
                .size(1.3964844f, 1.5f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 骡
        registry.registerType(EntityTypes::MULE,
            EntityType::Builder(&MuleEntity::create, EntityClassification::Creature)
                .size(1.3964844f, 1.6f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 骷髅马
        registry.registerType(EntityTypes::SKELETON_HORSE,
            EntityType::Builder(&SkeletonHorseEntity::create, EntityClassification::Creature)
                .size(1.3964844f, 1.6f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 僵尸马
        registry.registerType(EntityTypes::ZOMBIE_HORSE,
            EntityType::Builder(&ZombieHorseEntity::create, EntityClassification::Creature)
                .size(1.3964844f, 1.6f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 羊驼
        registry.registerType(EntityTypes::LLAMA,
            EntityType::Builder(&LlamaEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.87f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== Boss ==========
        // 末影龙 - TODO: 需要完成实现
        // registry.registerType(
        //     EntityTypes::ENDER_DRAGON,
        //     EntityType::Builder(&EnderDragonEntity::create, EntityClassification::Monster)
        //         .size(16.0f, 8.0f)
        //         .trackingRange(128)
        //         .updateInterval(1)
        //         .immuneToFire()
        //         .build()
        // );

        // 凋灵 - TODO: 需要完成实现
        // registry.registerType(
        //     EntityTypes::WITHER,
        //     EntityType::Builder(&WitherEntity::create, EntityClassification::Monster)
        //         .size(0.9f, 3.5f)
        //         .trackingRange(10)
        //         .updateInterval(3)
        //         .immuneToFire()
        //         .canSummon(true)
        //         .build()
        // );

        // ========== 村民 ==========
        // 村民 - TODO: 需要完成实现
        // registry.registerType(
        //     EntityTypes::VILLAGER,
        //     EntityType::Builder(&VillagerEntity::create, EntityClassification::Creature)
        //         .size(0.6f, 1.95f)
        //         .trackingRange(10)
        //         .updateInterval(3)
        //         .canSummon(true)
        //         .build()
        // );

        // 流浪商人 - TODO: 需要完成实现
        // registry.registerType(
        //     EntityTypes::WANDERING_TRADER,
        //     EntityType::Builder(&WanderingTraderEntity::create, EntityClassification::Creature)
        //         .size(0.6f, 1.95f)
        //         .trackingRange(10)
        //         .updateInterval(3)
        //         .canSummon(true)
        //         .build()
        // );

        // ========== 更多怪物 ==========
        // 巨人
        registry.registerType(EntityTypes::GIANT,
            EntityType::Builder(&GiantEntity::create, EntityClassification::Monster)
                .size(3.6f, 12.0f)
                .trackingRange(16)
                .updateInterval(3)
                .build() // 不可召唤
        );

        // 幻翼
        registry.registerType(EntityTypes::PHANTOM,
            EntityType::Builder(&PhantomEntity::create, EntityClassification::Monster)
                .size(0.9f, 0.5f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 末影螨
        registry.registerType(EntityTypes::ENDERMITE,
            EntityType::Builder(&EndermiteEntity::create, EntityClassification::Monster)
                .size(0.4f, 0.3f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 蠹虫
        registry.registerType(EntityTypes::SILVERFISH,
            EntityType::Builder(&SilverfishEntity::create, EntityClassification::Monster)
                .size(0.4f, 0.3f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 潜影贝
        registry.registerType(EntityTypes::SHULKER,
            EntityType::Builder(&ShulkerEntity::create, EntityClassification::Monster)
                .size(1.0f, 1.0f)
                .trackingRange(10)
                .updateInterval(3)
                .immuneToFire()
                .canSummon(true)
                .build());

        // ========== 地狱生物 ==========
        // 恶魂
        registry.registerType(EntityTypes::GHAST,
            EntityType::Builder(&GhastEntity::create, EntityClassification::Monster)
                .size(4.0f, 4.0f)
                .trackingRange(10)
                .updateInterval(3)
                .immuneToFire()
                .canSummon(true)
                .build());

        // 岩浆怪
        registry.registerType(EntityTypes::MAGMA_CUBE,
            EntityType::Builder(&MagmaCubeEntity::create, EntityClassification::Monster)
                .size(0.6f, 0.6f) // 尺寸会动态变化
                .trackingRange(10)
                .updateInterval(3)
                .immuneToFire()
                .canSummon(true)
                .build());

        // 猪灵
        registry.registerType(EntityTypes::PIGLIN,
            EntityType::Builder(&PiglinEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 猪灵蛮兵
        registry.registerType(EntityTypes::PIGLIN_BRUTE,
            EntityType::Builder(&PiglinBruteEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 疣猪兽
        registry.registerType(EntityTypes::HOGLIN,
            EntityType::Builder(&HoglinEntity::create, EntityClassification::Creature)
                .size(1.3964844f, 1.4f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 僵尸疣猪兽
        registry.registerType(EntityTypes::ZOGLIN,
            EntityType::Builder(&ZoglinEntity::create, EntityClassification::Monster)
                .size(1.3964844f, 1.4f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 灾厄村民 ==========
        // 卫道士
        registry.registerType(EntityTypes::VINDICATOR,
            EntityType::Builder(&VindicatorEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 唤魔者
        registry.registerType(EntityTypes::EVOKER,
            EntityType::Builder(&EvokerEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 幻术师
        registry.registerType(EntityTypes::ILLUSIONER,
            EntityType::Builder(&IllusionerEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 掠夺者
        registry.registerType(EntityTypes::PILLAGER,
            EntityType::Builder(&PillagerEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 劫掠兽
        registry.registerType(EntityTypes::RAVAGER,
            EntityType::Builder(&RavagerEntity::create, EntityClassification::Monster)
                .size(1.95f, 2.2f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 恼鬼
        registry.registerType(EntityTypes::VEX,
            EntityType::Builder(&VexEntity::create, EntityClassification::Monster)
                .size(0.4f, 0.8f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 唤魔者尖牙
        registry.registerType(EntityTypes::EVOKER_FANGS,
            EntityType::Builder(&entity::EvokerFangsEntity::create, EntityClassification::Misc)
                .size(0.5f, 0.8f)
                .trackingRange(8)
                .updateInterval(1) // 尖牙需要频繁更新（攻击时序）
                .canSummon(true)
                .build());

        // 潜影贝子弹
        registry.registerType(EntityTypes::SHULKER_BULLET,
            EntityType::Builder(&entity::ShulkerBulletEntity::create, EntityClassification::Misc)
                .size(0.3125f, 0.3125f)
                .trackingRange(8)
                .updateInterval(1) // 子弹需要频繁更新
                .canSummon(true)
                .build());

        // ========== 物品 ==========
        registry.registerType(EntityTypes::ITEM,
            EntityType::Builder(&ItemEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(20)
                .canSummon(true)
                .build());

        // ========== 经验球 ==========
        registry.registerType(EntityTypes::EXPERIENCE_ORB,
            EntityType::Builder(&ExperienceOrbEntity::create, EntityClassification::Misc)
                .size(0.5f, 0.5f)
                .trackingRange(6)
                .updateInterval(1) // 经验球需要频繁更新
                .canSummon(true)
                .build());

        // ========== TNT ==========
        registry.registerType(EntityTypes::TNT,
            EntityType::Builder(&entity::TNTEntity::create, EntityClassification::Misc)
                .size(0.98f, 0.98f)
                .trackingRange(10)
                .updateInterval(1) // TNT 需要频繁更新（引信倒计时）
                .canSummon(true)
                .build());

        // ========== 区域效果云 ==========
        registry.registerType(EntityTypes::AREA_EFFECT_CLOUD,
            EntityType::Builder(&entity::AreaEffectCloudEntity::create, EntityClassification::Misc)
                .size(6.0f, 0.5f) // 初始半径3.0，宽度=半径*2
                .trackingRange(10)
                .updateInterval(5) // 每5tick更新一次
                .canSummon(true)
                .build());

        // ========== 玩家 ==========
        // 玩家实体类型由 Player 类自行管理

        // ========== 初始化生成放置规则 ==========
        // 这必须在实体注册完成后调用
        world::spawn::EntitySpawnPlacementRegistry::initializeDefaults();

        spdlog::info("Registered {} entity types", registry.size());
    }
};

} // namespace entity
} // namespace mc
