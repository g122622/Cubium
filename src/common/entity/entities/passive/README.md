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
| 实体 | 说明 | 繁殖物品 | 接口 |
|------|------|----------|------|
| AnimalEntity | 动物基类 | - | - |
| PigEntity | 猪 | 胡萝卜 | IRideable |
| CowEntity | 牛 | 小麦 | - |
| SheepEntity | 羊 | 小麦 | IShearable |
| ChickenEntity | 鸡 | 种子 | - |
| RabbitEntity | 兔子 | 胡萝卜/蒲公英 | - |
| MooshroomEntity | 哞菇 | 小麦 | IShearable |

### tamable/ - 可驯服动物
| 实体 | 说明 | 驯服物品 | 繁殖物品 | 接口 |
|------|------|----------|----------|------|
| TameableEntity | 可驯服基类 | - | - | IAngerable |
| WolfEntity | 狼 | 骨头 | 肉类 | - |
| CatEntity | 猫 | 生鱼 | 生鱼 | - |
| OcelotEntity | 豹猫 | 生鱼(信任) | 生鳕鱼/生鲑鱼 | - |
| ParrotEntity | 鹦鹉 | 种子 | 不可繁殖 | IFlyingAnimal |

**注意**：豹猫使用信任机制而非传统驯服，详见 `tamable/README.md`。

### special/ - 特殊动物
| 实体 | 说明 | 繁殖物品 | 特殊行为 | 接口 |
|------|------|----------|----------|------|
| FoxEntity | 狐狸 | 甜浆果 | 叼物品、信任机制 | - |
| PandaEntity | 熊猫 | 竹子 | 7种性格基因 | - |
| PolarBearEntity | 北极熊 | 不可繁殖 | 保护幼崽 | - |
| TurtleEntity | 海龟 | 海草 | 出生地记忆、产卵 | - |
| BeeEntity | 蜜蜂 | 花朵 | 授粉、蜂巢记忆、螫刺后死亡 | IFlyingAnimal |
| StriderEntity | 炽足兽 | 诡异菌 | 熔岩行走、可骑乘 | IRideable |

### horse/ - 马类
| 实体 | 说明 | 状态 |
|------|------|------|
| AbstractHorseEntity | 马类基类 | ✅ 已实现 |
| HorseEntity | 马 | ✅ 已实现 |
| DonkeyEntity | 驴 | ✅ 已实现 |
| MuleEntity | 骡 | ✅ 已实现 |
| SkeletonHorseEntity | 骷髅马 | ✅ 已实现 |
| ZombieHorseEntity | 僵尸马 | ✅ 已实现 |
| LlamaEntity | 羊驼 | ✅ 已实现 |

### 马类源码缺口

1. 仍缺 `AbstractChestedHorseEntity`、`TraderLlamaEntity`、`ShoulderRidingEntity`、`CoatColors`、`CoatTypes` 这些 1.16.5 侧的支撑类型。
2. 羊驼商队和箱子马类的语义需要补到和源码一致。
3. 当前父目录 README 对马类状态的描述应以本节为准。

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

#### SquidEntity 行为详情

鱿鱼是水生生物，具有喷墨和游泳行为。

**喷墨行为**
- 触发：`sprayInk()` 方法
- 持续时间：`SPRAY_INK_DURATION` tick
- 粒子效果：生成 30 个 `ParticleTypeId::SquidInk` 粒子
- 粒子分布：在鱿鱼周围随机分布，形成云状墨汁效果
- 粒子速度：向外扩散，轻微向上飘动

```cpp
void sprayInk() {
    if (!m_sprayingInk) {
        m_sprayingInk = true;
        m_sprayTimer = SPRAY_INK_DURATION;
        // 生成墨汁粒子...
    }
}
```

| 方法 | MC 1.16.5 | 项目 | 状态 |
|------|-----------|------|------|
| sprayInk() | 喷墨粒子效果 | 30个SquidInk粒子 | ✅ 已实现 |
| tick() | 游泳状态更新 | 方向改变、推进 | ✅ 已实现 |
| isInWater() | 检查是否在水中 | 父类方法 | ✅ 已实现 |

### ambient/ - 环境生物
| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| AmbientEntity | 环境生物基类 | - | ✅ 框架完成 |
| BatEntity | 蝙蝠 | 昼夜检测、倒挂休息 | ✅ 已实现 |

#### BatEntity 行为详情

蝙蝠是唯一的环境生物，具有独特的行为模式：

**昼夜检测**
```cpp
i64 timeOfDay = world->dayTime() % 24000;
bool isDay = timeOfDay < 12000;  // 0-12000: 白天, 12000-24000: 夜晚
```

**倒挂休息**
- 蝙蝠在白天时寻找上方有固体方块的位置倒挂休息
- `canRest()` 方法检查上方方块是否为固体
- 休息时蝙蝠保持静止，不进行飞行

**夜间飞行**
- 夜晚蝙蝠会苏醒并开始飞行
- 使用随机飞行目标进行移动

| 方法 | MC 1.16.5 | 项目 | 状态 |
|------|-----------|------|------|
| canRest() | 检查上方固体方块 | 上方方块检测 | ✅ 已实现 |
| tick() | 昼夜检测切换状态 | dayTime() % 24000 | ✅ 已实现 |
| restState | 倒挂休息状态 | m_resting | ✅ 已实现 |

### golem/ - 傀儡
| 实体 | 说明 | 接口 |
|------|------|------|
| GolemEntity | 傀儡基类 | IAngerable |
| IronGolemEntity | 铁傀儡 | - |
| SnowGolemEntity | 雪傀儡 | IShearable |
