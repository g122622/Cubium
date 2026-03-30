# Entities 目录

本目录包含所有具体实体实现，按类别组织到子目录中。

## 目录结构

```
entities/
├── passive/          # 被动/中立生物
│   ├── basic/        # 普通动物 (Pig, Cow, Sheep, Chicken, Rabbit, Mooshroom)
│   ├── tamable/      # 可驯服动物 (Wolf, Cat, Ocelot, Parrot + TameableEntity基类)
│   ├── special/      # 特殊动物 (Fox, Panda, PolarBear, Turtle, Bee, Strider)
│   ├── horse/        # 马类 (Horse, Donkey, Mule, SkeletonHorse, ZombieHorse, Llama)
│   ├── fish/         # 鱼类 (Cod, Salmon, Pufferfish, TropicalFish + AbstractFishEntity基类)
│   ├── water/        # 水生生物 (Squid, Dolphin + WaterMobEntity基类)
│   ├── ambient/      # 环境生物 (Bat + AmbientEntity基类)
│   └── golem/        # 傀儡 (IronGolem, SnowGolem + GolemEntity基类)
│
├── monster/          # 敌对生物
│   ├── MonsterEntity.hpp/cpp  # 敌对生物基类
│   ├── undead/       # 亡灵类 (Zombie, Skeleton, Husk, Stray, Drowned, WitherSkeleton, Phantom, ZombieVillager)
│   ├── arthropod/    # 节肢类 (Spider, CaveSpider, Silverfish, Endermite)
│   ├── nether/       # 地狱生物 (Blaze, Ghast, MagmaCube, Piglin, PiglinBrute, Hoglin, Zoglin)
│   ├── end/          # 末地生物 (Enderman, Shulker)
│   ├── basic/        # 基础怪物 (Creeper, Slime, Phantom, Giant)
│   ├── ocean/        # 海洋怪物 (Guardian, ElderGuardian)
│   ├── illager/      # 灾厄村民 (Vindicator, Evoker, Illusioner, Pillager, Ravager, Vex, Witch + AbstractIllagerEntity基类)
│   └── piglin/       # 猪灵相关 (预留)
│
├── boss/             # Boss实体
│   ├── EnderDragonEntity.hpp/cpp  # 末影龙 + EnderDragonPartEntity
│   ├── WitherEntity.hpp/cpp       # 凋灵
│   └── README.md
│
├── villager/         # 村民实体
│   ├── AbstractVillagerEntity.hpp/cpp  # 抽象村民基类
│   ├── VillagerEntity.hpp/cpp          # 村民实体 + VillagerData
│   └── README.md
│
├── projectile/       # 投掷物实体
│   ├── ProjectileEntity.hpp/cpp       # 投掷物基类
│   ├── ThrowableEntity.hpp/cpp        # 可投掷物品基类
│   ├── AbstractArrowEntity.hpp/cpp    # 箭矢基类 + ArrowEntity, SpectralArrowEntity
│   ├── AbstractFireballEntity.hpp/cpp # 火球基类 + FireballEntity, SmallFireballEntity等
│   ├── ProjectileItemEntity.hpp/cpp   # 投掷物品 + SnowballEntity, EggEntity等
│   ├── TridentEntity.hpp/cpp          # 三叉戟实体
│   ├── OtherProjectiles.hpp/cpp       # 其他投掷物
│   └── README.md
│
├── vehicle/          # 交通工具
│   ├── BoatEntity.hpp/cpp        # 船 (6种木材变体)
│   ├── MinecartEntity.hpp/cpp    # 矿车 (7种变体)
│   └── README.md
│
├── hanging/          # 悬挂实体
│   ├── HangingEntity.hpp/cpp     # 画、物品展示框、拴绳结
│   └── README.md
│
├── effect/           # 效果实体
│   ├── EffectEntities.hpp/cpp    # EnderCrystal, Lightning, AreaEffectCloud, ExperienceOrb, ArmorStand
│   └── README.md
│
├── misc/             # 杂项实体
│   ├── MiscEntities.hpp/cpp      # FallingBlock, TNT, EyeOfEnder, Conduit, EvokerFangs
│   └── README.md
│
├── item/             # 物品相关实体
│   └── ItemEntity.hpp/cpp  # 掉落物品实体
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
| passive/horse | 7 | AbstractHorseEntity + Horse, Donkey, Mule, SkeletonHorse, ZombieHorse, Llama |
| monster/undead | 9 | Zombie系列 + Skeleton系列 + Phantom + ZombieVillager |
| monster/arthropod | 4 | Spider, CaveSpider, Silverfish, Endermite |
| monster/nether | 7 | Blaze, Ghast, MagmaCube, Piglin系列, Hoglin系列 |
| monster/end | 2 | Enderman, Shulker |
| monster/basic | 4 | Creeper, Slime, Phantom, Giant |
| monster/ocean | 2 | Guardian, ElderGuardian |
| monster/illager | 8 | AbstractIllagerEntity + Evoker, Illusioner, Ravager, Vex 等 |
| player | 3 | Player, PlayerManager, GameModeUtils |
| item | 1 | ItemEntity |
| **projectile** | **18+** | **ProjectileEntity + Arrow, Snowball, Fireball, Trident等** |
| **villager** | **3** | **AbstractVillagerEntity + VillagerEntity, WanderingTraderEntity** |
| **boss** | **3** | **BossEntity + EnderDragonEntity, WitherEntity** |
| **vehicle** | **8** | **BoatEntity + 7种MinecartEntity** |
| **hanging** | **3** | **PaintingEntity, ItemFrameEntity, LeashKnotEntity** |
| **effect** | **5** | **EnderCrystalEntity, LightningBoltEntity, AreaEffectCloudEntity, ExperienceOrbEntity, ArmorStandEntity** |
| **misc** | **5** | **FallingBlockEntity, TNTEntity, EyeOfEnderEntity, ConduitEntity, EvokerFangsEntity** |

### ⚠️ 框架完成 (TODO需填充)
| 类别 | 说明 |
|------|------|
| vehicle | Boat, Minecart 框架完成，需要实现铁轨逻辑和水上物理 |
| hanging | 画、展示框、拴绳结框架完成，需要实现方块检测 |
| effect | 末影水晶、闪电等框架完成，需要实现效果应用 |
| misc | 下落方块、TNT等框架完成，需要实现爆炸和物理 |

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
│   │       ├── basic/ (Creeper, Slime, Phantom, Giant)
│   │       ├── ocean/ (Guardian, ElderGuardian)
│   │       └── illager/ (Vindicator, Evoker, Illusioner, Pillager, Ravager, Vex, Witch)
│   └── Player (player/Player.hpp)
├── ItemEntity (item/ItemEntity.hpp)
├── VehicleEntity (vehicle/)
│   ├── BoatEntity (+ IRideable)
│   └── AbstractMinecartEntity (+ IRideable)
├── HangingEntity (hanging/)
│   ├── PaintingEntity
│   ├── ItemFrameEntity
│   └── LeashKnotEntity
├── EffectEntity (effect/)
│   ├── EnderCrystalEntity
│   ├── LightningBoltEntity
│   ├── AreaEffectCloudEntity
│   ├── ExperienceOrbEntity
│   └── ArmorStandEntity
└── MiscEntity (misc/)
    ├── FallingBlockEntity
    ├── TNTEntity
    ├── EyeOfEnderEntity
    ├── ConduitEntity
    └── EvokerFangsEntity
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
