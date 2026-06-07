# 被动/中立生物模块

包含所有被动生物和中立生物的实现。

## 目录结构

```
passive/
├── basic/                    # 普通动物（猪、牛、羊、鸡、兔、哞菇）
│   ├── AnimalEntity.hpp/cpp  # 动物基类，支持繁殖、喂食、跟随父母
│   ├── PigEntity.hpp/cpp     # 猪（可骑乘）
│   ├── CowEntity.hpp/cpp     # 牛
│   ├── SheepEntity.hpp/cpp   # 羊（可剪毛）
│   ├── ChickenEntity.hpp/cpp # 鸡（下蛋）
│   ├── RabbitEntity.hpp/cpp  # 兔子（多种毛色）
│   └── MooshroomEntity.hpp/cpp # 哞菇（可碗取蘑菇汤）
├── tamable/                  # 可驯服动物（狼、猫、豹猫、鹦鹉）
│   ├── TameableEntity.hpp/cpp    # 可驯服基类（IAngerable 接口）
│   ├── ShoulderRidingEntity.hpp  # 肩膀乘坐基类
│   ├── WolfEntity.hpp/cpp    # 狼
│   ├── CatEntity.hpp/cpp     # 猫（11种皮肤）
│   ├── OcelotEntity.hpp/cpp  # 豹猫（信任机制，非驯服）
│   └── ParrotEntity.hpp/cpp  # 鹦鹉（可站肩膀，不可繁殖）
├── special/                  # 特殊动物（狐狸、熊猫、北极熊、海龟、蜜蜂、炽足兽）
│   ├── FoxEntity.hpp/cpp     # 狐狸（信任机制、叼物品）
│   ├── PandaEntity.hpp/cpp   # 熊猫（7种性格基因）
│   ├── PolarBearEntity.hpp/cpp # 北极熊（保护幼崽）
│   ├── TurtleEntity.hpp/cpp  # 海龟（出生地记忆、产卵）
│   ├── BeeEntity.hpp/cpp     # 蜜蜂（授粉、蜂巢记忆、螫刺后死亡）
│   └── StriderEntity.hpp/cpp # 炽足兽（熔岩行走、可骑乘）
├── horse/                    # 马类（马、驴、骡、羊驼等）
│   ├── AbstractHorseEntity.hpp/cpp    # 马类基类（IJumpingMount, IEquipable）
│   ├── AbstractChestedHorseEntity.hpp # 箱子马基类
│   ├── CoatColors.hpp        # 马匹毛色
│   ├── CoatTypes.hpp         # 马匹花纹
│   ├── HorseEntity.hpp/cpp   # 马
│   ├── DonkeyEntity.hpp/cpp  # 驴
│   ├── MuleEntity.hpp/cpp    # 骡
│   ├── SkeletonHorseEntity.hpp/cpp # 骷髅马
│   ├── ZombieHorseEntity.hpp/cpp   # 僵尸马
│   ├── LlamaEntity.hpp/cpp   # 羊驼（可 spit 攻击）
│   └── TraderLlamaEntity.hpp # 流浪商人的羊驼
├── fish/                     # 鱼类（鳕鱼、鲑鱼、河豚、热带鱼）
│   ├── AbstractFishEntity.hpp/cpp    # 鱼类基类（桶装鱼支持）
│   ├── AbstractGroupFishEntity.hpp/cpp # 群游鱼类基类
│   ├── CodEntity.hpp/cpp     # 鳕鱼
│   ├── SalmonEntity.hpp/cpp  # 鲑鱼
│   ├── PufferfishEntity.hpp/cpp # 河豚（膨胀防御）
│   └── TropicalFishEntity.hpp/cpp # 热带鱼（多种变体）
├── water/                    # 水生生物（鱿鱼、海豚、美西螈）
│   ├── WaterMobEntity.hpp/cpp # 水生生物基类（反逻辑溺水：陆地消耗空气）
│   ├── SquidEntity.hpp/cpp   # 鱿鱼（喷墨）
│   ├── DolphinEntity.hpp/cpp # 海豚（宝藏寻找、海豚恩惠）
│   └── AxolotlEntity.hpp/cpp # 美西螈（装死、支援效果）
├── ambient/                  # 环境生物
│   ├── AmbientEntity.hpp/cpp # 环境生物基类
│   └── BatEntity.hpp/cpp     # 蝙蝠（昼夜检测、倒挂休息）
└── golem/                    # 傀儡
    ├── GolemEntity.hpp/cpp   # 傀儡基类（IAngerable）
    ├── IronGolemEntity.hpp/cpp # 铁傀儡（保护村民）
    └── SnowGolemEntity.hpp/cpp # 雪傀儡（发射雪球、留下雪径）
```

## 内部模块关系

```
Entity
└── LivingEntity
    └── MobEntity
        └── CreatureEntity
            ├── AgeableEntity
            │   └── AnimalEntity
            │       ├── basic/ (Pig, Cow, Sheep, Chicken, Rabbit, Mooshroom)
            │       ├── TameableEntity (+ IAngerable)
            │       │   ├── WolfEntity
            │       │   ├── CatEntity
            │       │   ├── OcelotEntity
            │       │   └── ParrotEntity (+ IFlyingAnimal, ShoulderRidingEntity)
            │       ├── special/ (Fox, Panda, PolarBear, Turtle, Bee, Strider)
            │       └── AbstractHorseEntity (+ IJumpingMount, IEquipable)
            │           ├── HorseEntity
            │           ├── DonkeyEntity / MuleEntity
            │           ├── SkeletonHorseEntity / ZombieHorseEntity
            │           └── LlamaEntity
            ├── WaterMobEntity
            │   ├── DolphinEntity
            │   ├── AxolotlEntity
            │   ├── SquidEntity
            │   └── AbstractFishEntity
            │       └── fish/ (Cod, Salmon, Pufferfish, TropicalFish)
            ├── AmbientEntity
            │   └── BatEntity
            └── GolemEntity (+ IAngerable)
                ├── IronGolemEntity
                └── SnowGolemEntity
```

## 上下游外部依赖关系

### 依赖的上游模块
- `entity/core/` - Entity, LivingEntity, MobEntity, CreatureEntity, AgeableEntity 基类
- `entity/interfaces/` - IAngerable, IRideable, IJumpingMount, IEquipable, IShearable, IFlyingAnimal 接口
- `entity/ai/` - Goal 系统（SwimGoal, PanicGoal, BreedGoal, TemptGoal, FollowParentGoal 等）
- `entity/attributes/` - 属性系统（MAX_HEALTH, MOVEMENT_SPEED 等）
- `world/IWorld.hpp` - 世界接口
- `item/Items.hpp` - 物品定义
- `block/Blocks.hpp` - 方块定义

### 被下游模块依赖
- `server/` - 服务器端实体生成、AI 调度
- `client/` - 客户端实体渲染
- `world/spawn/` - 生物群系生成时的实体放置
- `entity/VanillaEntities.hpp` - 实体类型注册

## 容易踩的坑

### 继承层次陷阱
1. **AbstractHorseEntity 不实现 IRideable**：MC 1.16.5 中马的控制逻辑通过 MobEntity 的乘客系统实现，不是像猪/炽足兽那样通过 IRideable::ride() 方法。马只实现 IJumpingMount 接口。

2. **WaterMobEntity 的反逻辑溺水**：水生生物在陆地上消耗空气、在水中恢复空气，与陆地生物相反。实现新的水生生物时务必正确调用基类的 `updateAirSupply()`。

3. **TameableEntity vs 信任机制**：豹猫使用信任机制（`isTrusting()`）而非 TameableEntity 的驯服系统，不继承 TameableEntity。狐狸也使用独立的信任系统。

### AI 目标注册顺序
1. **优先级数字越小越优先**：注册 Goal 时优先级参数是整数，0 最高优先。
2. **子类必须调用父类 registerGoals()**：否则会丢失基础行为（如 SwimGoal, PanicGoal）。
3. **动态 AI 管理**：部分实体（猫、豹猫）需要根据驯服/信任状态动态添加移除 Goal，参考 `setupTamedAI()` 模式。

### 数据参数同步
1. **DataParameter 必须在类内静态声明**：如 `static DataParameter<bool> DATA_TAMED;`
2. **数据参数注册顺序**：子类注册时必须先调用父类的 `registerDataParameters()`，再注册自己的参数。

### 繁殖物品检查
1. **使用 isBreedingItem() 而非硬编码**：繁殖物品判断必须重写 `isBreedingItem()` 方法，不要在交互逻辑中硬编码物品类型。
2. **狼的特殊食物**：狼可以吃腐肉繁殖和治疗，且不会获得饥饿效果。

### 消失条件
1. **被动生物默认不消失**：`canDespawn()` 返回 false。
2. **桶装鱼不消失**：从桶放出的鱼 `preventDespawn()` 返回 true。
3. **有名称的实体不消失**：检查 `hasCustomName()`。

### 马类实现注意事项
1. **AbstractChestedHorseEntity 只有头文件**：目前只有声明无实现，需要在 .cpp 文件中补充。
2. **TraderLlamaEntity 只有头文件**：目前只有声明无实现，需要在 .cpp 文件中补充。
3. **ShoulderRidingEntity 只有头文件**：目前只有声明无实现，需要在 .cpp 文件中补充。
