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

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/entities/boss/EnderDragonEntity.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/misc/OminousItemSpawnerEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/arthropod/CaveSpiderEntity.hpp"
#include "common/entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "common/entity/entities/monster/arthropod/SpiderEntity.hpp"
#include "common/entity/entities/monster/basic/CreeperEntity.hpp"
#include "common/entity/entities/monster/basic/GiantEntity.hpp"
#include "common/entity/entities/monster/basic/PhantomEntity.hpp"
#include "common/entity/entities/monster/basic/SlimeEntity.hpp"
#include "common/entity/entities/monster/breeze/BreezeEntity.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/entity/entities/monster/end/ShulkerEntity.hpp"
#include "common/entity/entities/monster/illager/EvokerEntity.hpp"
#include "common/entity/entities/monster/illager/IllagerEntities.hpp"
#include "common/entity/entities/monster/illager/IllusionerEntity.hpp"
#include "common/entity/entities/monster/illager/RavagerEntity.hpp"
#include "common/entity/entities/monster/illager/VexEntity.hpp"
#include "common/entity/entities/monster/illager/WitchEntity.hpp"
#include "common/entity/entities/monster/nether/BlazeEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/entities/monster/ocean/ElderGuardianEntity.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include "common/entity/entities/monster/undead/DrownedEntity.hpp"
#include "common/entity/entities/monster/undead/HuskEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/StrayEntity.hpp"
#include "common/entity/entities/monster/undead/WitherSkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieVillagerEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/passive/ambient/AmbientEntity.hpp"
#include "common/entity/entities/passive/ambient/BatEntity.hpp"
#include "common/entity/entities/passive/basic/ChickenEntity.hpp"
#include "common/entity/entities/passive/basic/CowEntity.hpp"
#include "common/entity/entities/passive/basic/MooshroomEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/entity/entities/passive/fish/AbstractFishEntity.hpp"
#include "common/entity/entities/passive/fish/CodEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/passive/fish/SalmonEntity.hpp"
#include "common/entity/entities/passive/fish/TropicalFishEntity.hpp"
#include "common/entity/entities/passive/golem/GolemEntity.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/passive/golem/SnowGolemEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/MuleEntity.hpp"
#include "common/entity/entities/passive/horse/SkeletonHorseEntity.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/passive/horse/ZombieHorseEntity.hpp"
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/entity/entities/passive/special/PandaEntity.hpp"
#include "common/entity/entities/passive/special/PolarBearEntity.hpp"
#include "common/entity/entities/passive/special/StriderEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"
#include "common/entity/entities/passive/tamable/OcelotEntity.hpp"
#include "common/entity/entities/passive/tamable/ParrotEntity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/entity/entities/passive/water/AxolotlEntity.hpp"
#include "common/entity/entities/passive/water/DolphinEntity.hpp"
#include "common/entity/entities/passive/water/SquidEntity.hpp"
#include "common/entity/entities/passive/water/WaterMobEntity.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/entity/entities/projectile/TridentEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/entities/vehicle/ChestBoatEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include <mutex>
#include <spdlog/spdlog.h>

namespace mc {
namespace entity {

/**
 * @brief 原版实体初始化器
 *
 * 注册所有原版实体类型到实体注册表。
 * 必须在服务器启动时或客户端初始化时调用。
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
                .size(0.4f, 0.3f)
                .trackingRange(8)
                .updateInterval(1) // 蜜蜂更新频繁
                .canSummon(true)
                .build());

        // 炽足兽
        registry.registerType(EntityTypes::STRIDER,
            EntityType::Builder(&StriderEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.8f)
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

        // 美西螈
        registry.registerType(EntityTypes::AXOLOTL,
            EntityType::Builder(&AxolotlEntity::create, EntityClassification::WaterCreature)
                .size(0.75f, 0.42f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== 鱼类 ==========
        // 鳕鱼
        registry.registerType(EntityTypes::COD,
            EntityType::Builder(&CodEntity::create, EntityClassification::WaterCreature)
                .size(0.5f, 0.3f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 鲑鱼
        registry.registerType(EntityTypes::SALMON,
            EntityType::Builder(&SalmonEntity::create, EntityClassification::WaterCreature)
                .size(0.7f, 0.4f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 河豚
        registry.registerType(EntityTypes::PUFFERFISH,
            EntityType::Builder(&PufferfishEntity::create, EntityClassification::WaterCreature)
                .size(0.7f, 0.7f)
                .trackingRange(8)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 热带鱼
        registry.registerType(EntityTypes::TROPICAL_FISH,
            EntityType::Builder(&TropicalFishEntity::create, EntityClassification::WaterCreature)
                .size(0.5f, 0.4f)
                .trackingRange(8)
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

        // 商队羊驼
        registry.registerType(EntityTypes::TRADER_LLAMA,
            EntityType::Builder(&TraderLlamaEntity::create, EntityClassification::Creature)
                .size(0.9f, 1.87f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // ========== Boss ==========
        // TODO: 末影龙实现不完整，暂未注册
        // registry.registerType(EntityTypes::ENDER_DRAGON,
        //     EntityType::Builder(&EnderDragonEntity::create, EntityClassification::Monster)
        //         .size(16.0f, 8.0f)
        //         .trackingRange(128)
        //         .updateInterval(1)
        //         .immuneToFire()
        //         .build());

        // 凋灵
        registry.registerType(EntityTypes::WITHER,
            EntityType::Builder(&WitherEntity::create, EntityClassification::Monster)
                .size(0.9f, 3.5f)
                .trackingRange(10)
                .updateInterval(3)
                .immuneToFire()
                .canSummon(true)
                .build());

        // ========== 村民 ==========
        // 村民
        registry.registerType(EntityTypes::VILLAGER,
            EntityType::Builder(&VillagerEntity::create, EntityClassification::Creature)
                .size(0.6f, 1.95f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

        // 流浪商人
        registry.registerType(EntityTypes::WANDERING_TRADER,
            EntityType::Builder(&WanderingTraderEntity::create, EntityClassification::Creature)
                .size(0.6f, 1.95f)
                .trackingRange(10)
                .updateInterval(3)
                .canSummon(true)
                .build());

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

        // 僵尸村民
        registry.registerType(EntityTypes::ZOMBIE_VILLAGER,
            EntityType::Builder(&ZombieVillagerEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
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

        // 僵尸猪灵
        registry.registerType(EntityTypes::ZOMBIFIED_PIGLIN,
            EntityType::Builder(&ZombifiedPiglinEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.95f)
                .trackingRange(8)
                .updateInterval(3)
                .immuneToFire()
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

        // 旋风人
        registry.registerType(EntityTypes::BREEZE,
            EntityType::Builder(&BreezeEntity::create, EntityClassification::Monster)
                .size(0.6f, 1.77f)
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

        // ========== 投掷物 ==========
        // 箭
        registry.registerType(EntityTypes::ARROW,
            EntityType::Builder(&ArrowEntity::create, EntityClassification::Misc)
                .size(0.5f, 0.5f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 光灵箭
        registry.registerType(EntityTypes::SPECTRAL_ARROW,
            EntityType::Builder(&SpectralArrowEntity::create, EntityClassification::Misc)
                .size(0.5f, 0.5f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 三叉戟
        registry.registerType(EntityTypes::TRIDENT,
            EntityType::Builder(&TridentEntity::create, EntityClassification::Misc)
                .size(0.5f, 0.5f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 雪球
        registry.registerType(EntityTypes::SNOWBALL,
            EntityType::Builder(&SnowballEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(1)
                .canSummon(true)
                .build());

        // 鸡蛋
        registry.registerType(EntityTypes::EGG,
            EntityType::Builder(&EggEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(1)
                .canSummon(true)
                .build());

        // 末影珍珠
        registry.registerType(EntityTypes::ENDER_PEARL,
            EntityType::Builder(&EnderPearlEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(1)
                .canSummon(true)
                .build());

        // 喷溅药水
        registry.registerType(EntityTypes::POTION,
            EntityType::Builder(&PotionEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(1)
                .canSummon(true)
                .build());

        // 附魔之瓶
        registry.registerType(EntityTypes::EXPERIENCE_BOTTLE,
            EntityType::Builder(&ExperienceBottleEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(1)
                .canSummon(true)
                .build());

        // 火球
        registry.registerType(EntityTypes::FIREBALL,
            EntityType::Builder(&FireballEntity::create, EntityClassification::Misc)
                .size(1.0f, 1.0f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 小火球
        registry.registerType(EntityTypes::SMALL_FIREBALL,
            EntityType::Builder(&SmallFireballEntity::create, EntityClassification::Misc)
                .size(0.3125f, 0.3125f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 龙火球
        registry.registerType(EntityTypes::DRAGON_FIREBALL,
            EntityType::Builder(&DragonFireballEntity::create, EntityClassification::Misc)
                .size(1.0f, 1.0f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 凋灵之首
        registry.registerType(EntityTypes::WITHER_SKULL,
            EntityType::Builder(&WitherSkullEntity::create, EntityClassification::Misc)
                .size(0.3125f, 0.3125f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 羊驼唾液
        registry.registerType(EntityTypes::LLAMA_SPIT,
            EntityType::Builder(&LlamaSpitEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(1)
                .build());

        // 钓鱼浮标
        registry.registerType(EntityTypes::FISHING_BOBBER,
            EntityType::Builder(&FishingBobberEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(4)
                .updateInterval(1)
                .build());

        // 末影之眼
        registry.registerType(EntityTypes::EYE_OF_ENDER,
            EntityType::Builder(&EyeOfEnderEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 烟花火箭
        registry.registerType(EntityTypes::FIREWORK_ROCKET,
            EntityType::Builder(&FireworkRocketEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 风弹
        registry.registerType(EntityTypes::WIND_CHARGE,
            EntityType::Builder(&WindChargeEntity::create, EntityClassification::Misc)
                .size(0.3125f, 0.3125f)
                .trackingRange(4)
                .updateInterval(1)
                .canSummon(true)
                .build());

        // ========== 交通工具 ==========
        // 船
        registry.registerType(EntityTypes::BOAT,
            EntityType::Builder(&BoatEntity::create, EntityClassification::Misc)
                .size(1.375f, 0.5625f)
                .trackingRange(10)
                .updateInterval(3)
                .build());

        // 箱子船
        registry.registerType(EntityTypes::CHEST_BOAT,
            EntityType::Builder(&ChestBoatEntity::create, EntityClassification::Misc)
                .size(1.375f, 0.5625f)
                .trackingRange(10)
                .updateInterval(3)
                .build());

        // 矿车
        registry.registerType(EntityTypes::MINECART,
            EntityType::Builder(&RideableMinecartEntity::create, EntityClassification::Misc)
                .size(0.98f, 0.7f)
                .trackingRange(8)
                .updateInterval(3)
                .build());

        // 箱子矿车
        registry.registerType(EntityTypes::CHEST_MINECART,
            EntityType::Builder(&ChestMinecartEntity::create, EntityClassification::Misc)
                .size(0.98f, 0.7f)
                .trackingRange(8)
                .updateInterval(3)
                .build());

        // 熔炉矿车
        registry.registerType(EntityTypes::FURNACE_MINECART,
            EntityType::Builder(&FurnaceMinecartEntity::create, EntityClassification::Misc)
                .size(0.98f, 0.7f)
                .trackingRange(8)
                .updateInterval(3)
                .build());

        // 漏斗矿车
        registry.registerType(EntityTypes::HOPPER_MINECART,
            EntityType::Builder(&HopperMinecartEntity::create, EntityClassification::Misc)
                .size(0.98f, 0.7f)
                .trackingRange(8)
                .updateInterval(3)
                .build());

        // TNT矿车
        registry.registerType(EntityTypes::TNT_MINECART,
            EntityType::Builder(&TNTMinecartEntity::create, EntityClassification::Misc)
                .size(0.98f, 0.7f)
                .trackingRange(8)
                .updateInterval(3)
                .build());

        // 刷怪笼矿车
        registry.registerType(EntityTypes::SPAWNER_MINECART,
            EntityType::Builder(&SpawnerMinecartEntity::create, EntityClassification::Misc)
                .size(0.98f, 0.7f)
                .trackingRange(8)
                .updateInterval(3)
                .build());

        // ========== 其他实体 ==========
        // 下落方块
        registry.registerType(EntityTypes::FALLING_BLOCK,
            EntityType::Builder(&FallingBlockEntity::create, EntityClassification::Misc)
                .size(0.98f, 0.98f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // 末地水晶
        registry.registerType(EntityTypes::END_CRYSTAL,
            EntityType::Builder(&EnderCrystalEntity::create, EntityClassification::Misc)
                .size(2.0f, 2.0f)
                .trackingRange(16)
                .updateInterval(1)
                .build());

        // 闪电
        registry.registerType(EntityTypes::LIGHTNING_BOLT,
            EntityType::Builder(&LightningBoltEntity::create, EntityClassification::Misc)
                .size(0.0f, 0.0f)
                .trackingRange(16)
                .updateInterval(1)
                .build());

        // 盔甲架
        registry.registerType(EntityTypes::ARMOR_STAND,
            EntityType::Builder(&ArmorStandEntity::create, EntityClassification::Misc)
                .size(0.5f, 1.975f)
                .trackingRange(10)
                .updateInterval(3)
                .build());

        // 不祥物品生成器
        registry.registerType(EntityTypes::OMINOUS_ITEM_SPAWNER,
            EntityType::Builder(&OminousItemSpawnerEntity::create, EntityClassification::Misc)
                .size(0.25f, 0.25f)
                .trackingRange(8)
                .updateInterval(1)
                .build());

        // ========== 悬挂实体 ==========
        // 画
        registry.registerType(EntityTypes::PAINTING,
            EntityType::Builder(&PaintingEntity::create, EntityClassification::Misc)
                .size(0.5f, 0.5f)
                .trackingRange(10)
                .updateInterval(20)
                .build());

        // 物品展示框
        registry.registerType(EntityTypes::ITEM_FRAME,
            EntityType::Builder(&ItemFrameEntity::create, EntityClassification::Misc)
                .size(0.5f, 0.5f)
                .trackingRange(10)
                .updateInterval(20)
                .build());

        // 拴绳结
        registry.registerType(EntityTypes::LEASH_KNOT,
            EntityType::Builder(&LeashKnotEntity::create, EntityClassification::Misc)
                .size(0.5f, 0.5f)
                .trackingRange(10)
                .updateInterval(20)
                .build());

        // ========== 初始化生成放置规则 ==========
        // 这必须在实体注册完成后调用
        world::spawn::EntitySpawnPlacementRegistry::initializeDefaults();

        // 初始化实体类型 ID 缓存
        EntityTypeIdNumber::initialize();

        spdlog::info("Registered {} entity types", registry.size());
    }
};

} // namespace entity
} // namespace mc
