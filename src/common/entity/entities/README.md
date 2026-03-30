# Entities 目录

本目录包含所有具体实体实现，按类别组织到子目录中。

## 目录结构

```
entities/
├── passive/          # 被动/中立生物
│   ├── basic/        # 普通动物 (Pig, Cow, Sheep, Chicken, Rabbit, Mooshroom)
│   ├── tamable/      # 可驯服动物 (Wolf, Cat, Ocelot, Parrot + TameableEntity基类)
│   ├── special/      # 特殊动物 (Fox, Panda, PolarBear, Turtle, Bee, Strider)
│   ├── horse/        # 马类 (AbstractHorseEntity基类)
│   ├── fish/         # 鱼类 (Cod, Salmon, Pufferfish, TropicalFish + AbstractFishEntity基类)
│   ├── water/        # 水生生物 (Squid, Dolphin + WaterMobEntity基类)
│   ├── ambient/      # 环境生物 (Bat + AmbientEntity基类)
│   └── golem/        # 傀儡 (IronGolem, SnowGolem + GolemEntity基类)
│
├── monster/          # 敌对生物
│   ├── MonsterEntity.hpp/cpp  # 敌对生物基类
│   ├── undead/       # 亡灵类 (Zombie, Skeleton, Husk, Stray, Drowned, WitherSkeleton, Phantom, ZombieVillager, ZombifiedPiglin)
│   ├── arthropod/    # 节肢类 (Spider, CaveSpider, Silverfish, Endermite)
│   ├── nether/       # 地狱生物 (Blaze, Ghast, MagmaCube, Piglin, PiglinBrute, Hoglin, Zoglin)
│   ├── end/          # 末地生物 (Enderman, Shulker)
│   ├── basic/        # 基础怪物 (Creeper, Slime, Phantom, Giant)
│   ├── ocean/        # 海洋怪物 (Guardian, ElderGuardian)
│   ├── illager/      # 灾厄村民 (Vindicator, Evoker, Illusioner, Pillager, Ravager, Vex, Witch + AbstractIllagerEntity基类)
│   └── piglin/       # 猪灵相关 (预留)
│
├── boss/             # Boss生物 (预留)
│   └── dragon/       # 末影龙相关 (预留)
│
├── villager/         # 村民/商人 (预留)
│   # 计划: AbstractVillagerEntity, VillagerEntity, VillagerData, WanderingTraderEntity
│
├── projectile/       # 投掷物 (预留)
│   # 计划: ProjectileEntity, ArrowEntity, SnowballEntity, FireballEntity等
│
├── vehicle/          # 交通工具 (预留)
│   └── minecart/     # 矿车类 (预留)
│
├── item/             # 物品相关实体
│   └── ItemEntity.hpp/cpp  # 掉落物品实体
│   # 计划: FallingBlockEntity, TNTEntity, ExperienceOrbEntity, AreaEffectCloudEntity
│
├── hanging/          # 悬挂实体 (预留)
│   # 计划: HangingEntity, ItemFrameEntity, PaintingEntity, LeashKnotEntity
│
├── effect/           # 效果实体 (预留)
│   # 计划: LightningBoltEntity, EndCrystalEntity
│
├── equipment/        # 装备实体 (预留)
│   # 计划: ArmorStandEntity
│
└── player/           # 玩家实体
    ├── Player.hpp/cpp         # 玩家实体类
    ├── PlayerManager.hpp/cpp  # 玩家管理器
    └── GameModeUtils.hpp/cpp  # 游戏模式工具函数
```

## 实现状态

### ✅ 已完成
| 类别 | 实体数量 | 说明 |
|------|----------|------|
| passive/basic | 6 | Pig, Cow, Sheep, Chicken, Rabbit, Mooshroom |
| passive/tamable | 5 | TameableEntity + Wolf, Cat, Ocelot, Parrot |
| passive/special | 6 | Fox, Panda, PolarBear, Turtle, Bee, Strider |
| passive/fish | 5 | AbstractFishEntity + Cod, Salmon, Pufferfish, TropicalFish |
| passive/water | 3 | WaterMobEntity + Squid, Dolphin |
| passive/ambient | 2 | AmbientEntity + Bat |
| passive/golem | 3 | GolemEntity + IronGolem, SnowGolem |
| passive/horse | 1 | AbstractHorseEntity (基类) |
| monster/undead | 9 | Zombie系列 + Skeleton系列 + Phantom |
| monster/arthropod | 4 | Spider, CaveSpider, Silverfish, Endermite |
| monster/nether | 7 | Blaze, Ghast, MagmaCube, Piglin系列, Hoglin系列 |
| monster/end | 2 | Enderman, Shulker |
| monster/basic | 3 | Creeper, Slime, Phantom |
| monster/ocean | 2 | Guardian, ElderGuardian |
| monster/illager | 8 | AbstractIllagerEntity + 7种灾厄村民 |
| player | 3 | Player, PlayerManager, GameModeUtils |
| item | 1 | ItemEntity |

### ❌ 未实现
| 类别 | 缺失实体 |
|------|----------|
| boss/ | WitherEntity, EnderDragonEntity, EnderDragonPartEntity |
| villager/ | AbstractVillagerEntity, VillagerEntity, WanderingTraderEntity |
| projectile/ | 全部投掷物（Arrow, Snowball, Fireball等约18种） |
| vehicle/ | BoatEntity, AbstractMinecartEntity + 8种矿车 |
| hanging/ | HangingEntity, ItemFrameEntity, PaintingEntity, LeashKnotEntity |
| effect/ | LightningBoltEntity, EndCrystalEntity |
| equipment/ | ArmorStandEntity |
| passive/horse/ | HorseEntity, DonkeyEntity, MuleEntity, SkeletonHorseEntity, ZombieHorseEntity, LlamaEntity |
| item/ | FallingBlockEntity, TNTEntity, ExperienceOrbEntity, AreaEffectCloudEntity |

## 继承层次

```
Entity (core/Entity.hpp)
├── LivingEntity (core/LivingEntity.hpp)
│   ├── MobEntity (core/MobEntity.hpp)
│   │   ├── CreatureEntity (core/CreatureEntity.hpp)
│   │   │   ├── AgeableEntity (core/AgeableEntity.hpp)
│   │   │   │   └── AnimalEntity (passive/basic/AnimalEntity.hpp)
│   │   │   │       ├── basic/ (Pig, Cow, Sheep, Chicken, Rabbit, Mooshroom)
│   │   │   │       ├── tamable/TameableEntity (+ IAngerable)
│   │   │   │       │   ├── WolfEntity
│   │   │   │       │   ├── CatEntity
│   │   │   │       │   ├── OcelotEntity
│   │   │   │       │   └── ParrotEntity
│   │   │   │       ├── special/ (Fox, Panda, PolarBear, Turtle, Bee, Strider)
│   │   │   │       └── horse/AbstractHorseEntity (+ IRideable, IJumpingMount)
│   │   │   └── WaterMobEntity (passive/water/WaterMobEntity.hpp)
│   │   │       ├── water/ (Squid, Dolphin)
│   │   │       └── fish/AbstractFishEntity
│   │   │           └── fish/ (Cod, Salmon, Pufferfish, TropicalFish)
│   │   ├── AmbientEntity (passive/ambient/AmbientEntity.hpp)
│   │   │   └── ambient/BatEntity
│   │   ├── GolemEntity (passive/golem/GolemEntity.hpp + IAngerable)
│   │   │   ├── IronGolemEntity
│   │   │   └── SnowGolemEntity
│   │   └── MonsterEntity (monster/MonsterEntity.hpp)
│   │       ├── undead/ (Zombie系列, Skeleton系列, Phantom)
│   │       ├── arthropod/ (Spider, CaveSpider, Silverfish, Endermite)
│   │       ├── nether/ (Blaze, Ghast, MagmaCube, Piglin系列, Hoglin系列)
│   │       ├── end/ (Enderman, Shulker)
│   │       ├── basic/ (Creeper, Slime, Phantom)
│   │       ├── ocean/ (Guardian, ElderGuardian)
│   │       └── illager/ (Vindicator, Evoker, Illusioner, Pillager, Ravager, Vex, Witch)
│   └── Player (player/Player.hpp)
└── ItemEntity (item/ItemEntity.hpp)
```

## 设计原则

1. **继承清晰**：严格遵循MC Java的继承层次
2. **接口分离**：用接口表达行为能力（IRideable, IShearable等）
3. **目录组织**：按实体类别划分子目录
4. **基类优先**：每个类别先实现基类，再实现具体实体

## 添加新实体

```cpp
// 1. 在适当子目录创建实体类
// 例如: entities/passive/basic/FoxEntity.hpp
#pragma once
#include "AnimalEntity.hpp"

namespace mc {
namespace entity {

class FoxEntity : public AnimalEntity {
public:
    // 构造函数
    FoxEntity(EntityType type, EntityId id);

    // 实体类型工厂
    static std::unique_ptr<Entity> create(IWorld* world);

protected:
    // 注册属性
    void registerAttributes() override;

    // 注册AI目标
    void registerGoals() override;

    // 繁殖
    bool isBreedingItem(const ItemStack& stack) const override;
    std::unique_ptr<AgeableEntity> createChild(AgeableEntity* partner) override;
};

} // namespace entity
} // namespace mc

// 2. 在 VanillaEntities.hpp/cpp 中注册
EntityRegistry::instance().registerType(
    "minecraft:fox",
    EntityType::Builder(&FoxEntity::create, EntityClassification::Creature)
        .size(0.6f, 0.7f)
        .trackingRange(10)
        .build()
);
```

## 相关文档

- [实体系统总览](../README.md)
- [核心实体层](../core/README.md)
- [实体接口](../interfaces/README.md)
- [AI系统](../ai/README.md)
