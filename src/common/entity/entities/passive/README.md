# 被动/中立生物模块

包含所有被动生物和中立生物的实现。

## 目录结构

```
passive/
├── basic/          # 普通动物（猪、牛、羊、鸡等）
├── tamable/        # 可驯服动物（狼、猫、鹦鹉等）
├── special/        # 特殊动物（狐狸、熊猫、北极熊等）
├── horse/          # 马类（马、驴、骡、羊驼等）
├── fish/           # 鱼类（鳕鱼、鲑鱼、河豚、热带鱼）
├── water/          # 水生生物（鱿鱼、海豚）
├── ambient/        # 环境生物（蝙蝠）
└── golem/          # 傀儡（铁傀儡、雪傀儡）
```

## 继承层次

```
MobEntity
└── CreatureEntity
    └── AgeableEntity
        └── AnimalEntity
            ├── TameableEntity (可驯服)
            │   ├── WolfEntity (狼)
            │   ├── CatEntity (猫)
            │   ├── OcelotEntity (豹猫)
            │   └── ParrotEntity (鹦鹉)
            ├── PigEntity (猪)
            ├── CowEntity (牛)
            ├── SheepEntity (羊)
            ├── ChickenEntity (鸡)
            └── ...
```

## 动物行为

### 基础行为优先级
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | PanicGoal | 受伤/着火时逃跑 |
| 2 | BreedGoal | 繁殖 |
| 3 | TemptGoal | 被食物诱惑 |
| 4 | FollowParentGoal | 幼体跟随父母 |
| 5 | WaterAvoidingRandomWalkingGoal | 避开水随机行走 |
| 6 | LookAtGoal | 看向玩家 |
| 7 | LookRandomlyGoal | 随机看向 |

### 可驯服动物额外行为
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SitGoal | 坐下（当被命令时） |
| 3 | FollowOwnerGoal | 跟随主人 |

## 子目录详细说明

### basic/ - 普通动物
| 实体 | 说明 | 繁殖物品 |
|------|------|----------|
| AnimalEntity | 动物基类 | - |
| PigEntity | 猪 | 胡萝卜 |
| CowEntity | 牛 | 小麦 |
| SheepEntity | 羊 | 小麦 |
| ChickenEntity | 鸡 | 种子 |
| RabbitEntity | 兔子 | 胡萝卜/蒲公英 |
| MooshroomEntity | 哞菇 | 小麦 |

### tamable/ - 可驯服动物
| 实体 | 说明 | 驯服物品 |
|------|------|----------|
| TameableEntity | 可驯服基类 | - |
| WolfEntity | 狼 | 骨头 |
| CatEntity | 猫 | 生鱼 |
| OcelotEntity | 豹猫 | 生鱼 |
| ParrotEntity | 鹦鹉 | 种子 |

### special/ - 特殊动物
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| FoxEntity | 狐狸 | 叼物品、信任机制 |
| PandaEntity | 熊猫 | 7种性格基因 |
| PolarBearEntity | 北极熊 | 保护幼崽 |
| TurtleEntity | 海龟 | 出生地记忆、产卵 |
| BeeEntity | 蜜蜂 | 授粉、蜂巢记忆 |
| StriderEntity | 炽足兽 | 熔岩行走、可骑乘 |

### horse/ - 马类
| 实体 | 说明 | 状态 |
|------|------|------|
| AbstractHorseEntity | 马类基类 | ✅ 已实现 |
| HorseEntity | 马 | ❌ 未实现 |
| DonkeyEntity | 驴 | ❌ 未实现 |
| MuleEntity | 骡 | ❌ 未实现 |
| SkeletonHorseEntity | 骷髅马 | ❌ 未实现 |
| ZombieHorseEntity | 僵尸马 | ❌ 未实现 |
| LlamaEntity | 羊驼 | ❌ 未实现 |

### fish/ - 鱼类
| 实体 | 说明 |
|------|------|
| AbstractFishEntity | 鱼类基类 |
| CodEntity | 鳕鱼 |
| SalmonEntity | 鲑鱼 |
| PufferfishEntity | 河豚 |
| TropicalFishEntity | 热带鱼 |

### water/ - 水生生物
| 实体 | 说明 |
|------|------|
| WaterMobEntity | 水生生物基类 |
| SquidEntity | 鱿鱼 |
| DolphinEntity | 海豚 |

### ambient/ - 环境生物
| 实体 | 说明 |
|------|------|
| AmbientEntity | 环境生物基类 |
| BatEntity | 蝙蝠 |

### golem/ - 傀儡
| 实体 | 说明 |
|------|------|
| GolemEntity | 傀儡基类 |
| IronGolemEntity | 铁傀儡 |
| SnowGolemEntity | 雪傀儡 |
