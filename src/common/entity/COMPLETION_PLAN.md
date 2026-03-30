# 实体系统补全计划

基于 2026-03-30 的全面检查结果，本文档列出了需要补全的工作项。

## 一、目录结构状态

### ✅ 已完成
| 目录 | 状态 | 说明 |
|------|------|------|
| `core/` | 完成 | 所有核心基类已实现 |
| `entities/passive/basic/` | 完成 | 6种动物实体 |
| `entities/passive/tamable/` | 完成 | 4种可驯服动物 + 基类 |
| `entities/passive/special/` | 完成 | 6种特殊动物 |
| `entities/passive/horse/` | 部分 | 仅有AbstractHorseEntity |
| `entities/passive/fish/` | 完成 | 5种鱼类 + 基类 |
| `entities/passive/water/` | 完成 | 3种水生生物 + 基类 |
| `entities/passive/ambient/` | 完成 | AmbientEntity + BatEntity |
| `entities/passive/golem/` | 完成 | 3种傀儡 + 基类 |
| `entities/monster/undead/` | 完成 | 6种亡灵 |
| `entities/monster/arthropod/` | 完成 | 4种节肢类 |
| `entities/monster/nether/` | 完成 | 7种地狱生物 |
| `entities/monster/end/` | 完成 | 2种末地生物 |
| `entities/monster/basic/` | 完成 | Creeper, Slime, Phantom |
| `entities/monster/ocean/` | 完成 | Guardian, ElderGuardian |
| `entities/monster/illager/` | 完成 | 8种灾厄村民 |
| `entities/player/` | 完成 | Player + PlayerManager |
| `entities/item/` | 部分 | ItemEntity完成 |
| `interfaces/` | 完成 | 8个接口 |
| `ai/goal/` | 完成 | 目标系统框架 + 多种Goal |
| `ai/brain/` | 框架完成 | Brain系统框架 |
| `ai/pathfinding/` | 完成 | 寻路系统 |
| `attribute/` | 完成 | 属性系统 |
| `combat/` | 完成 | 战斗系统 |
| `damage/` | 完成 | 伤害系统 |
| `inventory/` | 完成 | 背包系统 |
| `loot/` | 完成 | 掉落表系统 |
| `movement/` | 完成 | 移动系统 |

### ❌ 未完成
| 目录 | 状态 | 缺失实体 |
|------|------|----------|
| `entities/projectile/` | 空目录 | 15+投掷物实体 |
| `entities/boss/` | 空目录 | WitherEntity, EnderDragonEntity |
| `entities/villager/` | 空目录 | 5种村民相关实体 |
| `entities/vehicle/` | 空目录 | BoatEntity, 8种矿车 |
| `entities/hanging/` | 空目录 | 4种悬挂实体 |
| `entities/effect/` | 空目录 | LightningBoltEntity, EndCrystalEntity |
| `entities/equipment/` | 空目录 | ArmorStandEntity |
| `entities/passive/horse/` | 缺失 | HorseEntity, DonkeyEntity, MuleEntity等 |
| `ai/goal/goals/special/` | 空目录 | CreeperSwellGoal, EatGrassGoal等 |
| `ai/brain/sensor/sensors/` | 空目录 | 传感器实现 |
| `ai/brain/task/tasks/` | 空目录 | 任务实现 |

---

## 二、高优先级任务

### 2.1 投掷物实体系统 (核心游戏体验)
**目录**: `entities/projectile/`

需要实现：
1. **ProjectileEntity** - 投掷物基类
   - 位置、速度、加速度
   - 碰撞检测
   - 穿透、弹射逻辑
   - 所有者追踪

2. **AbstractArrowEntity** - 箭矢基类
   - 穿透深度
   - 伤害计算
   - 捡起逻辑

3. **ArrowEntity** - 普通箭
4. **SpectralArrowEntity** - 光灵箭（发光效果）
5. **ThrowableEntity** - 可投掷物基类
6. **SnowballEntity** - 雪球
7. **EggEntity** - 鸡蛋
8. **EnderPearlEntity** - 末影珍珠（传送）
9. **PotionEntity** - 喷溅药水
10. **ExperienceBottleEntity** - 附魔之瓶
11. **FireballEntity** - 火球（恶魂）
12. **SmallFireballEntity** - 小火球（烈焰人）
13. **DragonFireballEntity** - 龙息火球
14. **WitherSkullEntity** - 凋灵之首
15. **TridentEntity** - 三叉戟
16. **ShulkerBulletEntity** - 潜影贝子弹
17. **LlamaSpitEntity** - 羊驼唾液
18. **FishingBobberEntity** - 钓鱼浮漂

### 2.2 村民系统 (核心交易系统)
**目录**: `entities/villager/`

需要实现：
1. **AbstractVillagerEntity** - 村民基类
   - 交易系统
   - 背包管理
   - 经验值

2. **VillagerEntity** - 普通村民
   - VillagerData（职业、等级、经验）
   - VillagerProfession（15种职业）
   - Brain系统（日程、工作、休息）

3. **WanderingTraderEntity** - 流浪商人
   - 独特的交易表
   - 消失机制

### 2.3 Boss实体
**目录**: `entities/boss/`

需要实现：
1. **WitherEntity** - 凋灵
   - 三头攻击
   - 凋灵效果
   - 盾牌阶段

2. **EnderDragonEntity** - 末影龙
   - 多阶段AI
   - 龙息攻击
   - 破坏方块
   - EndCrystal关联

3. **EnderDragonPartEntity** - 末影龙部件（多碰撞箱）

---

## 三、中优先级任务

### 3.1 载具系统
**目录**: `entities/vehicle/`

需要实现：
1. **BoatEntity** - 船（6种木材类型）
2. **AbstractMinecartEntity** - 矿车基类
3. **MinecartEntity** - 普通矿车
4. **ChestMinecartEntity** - 运输矿车
5. **FurnaceMinecartEntity** - 动力矿车
6. **HopperMinecartEntity** - 漏斗矿车
7. **TNTMinecartEntity** - TNT矿车
8. **SpawnerMinecartEntity** - 刷怪笼矿车
9. **CommandBlockMinecartEntity** - 命令方块矿车

### 3.2 马类实体
**目录**: `entities/passive/horse/`

需要实现（现有AbstractHorseEntity）：
1. **HorseEntity** - 马（35种变体）
2. **DonkeyEntity** - 驴
3. **MuleEntity** - 骡
4. **SkeletonHorseEntity** - 骷髅马
5. **ZombieHorseEntity** - 僵尸马
6. **LlamaEntity** - 羊驼
7. **TraderLlamaEntity** - 商队羊驼

### 3.3 特殊Goal
**目录**: `ai/goal/goals/special/`

需要实现：
1. **CreeperSwellGoal** - 苦力怕膨胀
2. **EndermanTeleportGoal** - 末影人瞬移
3. **EatGrassGoal** - 羊吃草
4. **DolphinJumpGoal** - 海豚跳跃
5. **FoxEatBerriesGoal** - 狐狸吃浆果
6. **TurtleLayEggGoal** - 海龟下蛋
7. **OcelotAttackGoal** - 豹猫攻击

### 3.4 Brain传感器和任务
**目录**: `ai/brain/sensor/sensors/` 和 `ai/brain/task/tasks/`

需要实现：
- **传感器**: NearestPlayersSensor, NearestLivingEntitiesSensor, HurtBySensor, VillagerHostilesSensor
- **任务**: MoveToTargetTask, SleepTask, WorkTask, PlayTask, VillagerTask系列

---

## 四、低优先级任务

### 4.1 悬挂实体
**目录**: `entities/hanging/`

1. **HangingEntity** - 悬挂基类
2. **ItemFrameEntity** - 物品展示框
3. **PaintingEntity** - 画
4. **LeashKnotEntity** - 拴绳结

### 4.2 效果实体
**目录**: `entities/effect/`

1. **LightningBoltEntity** - 闪电
2. **EndCrystalEntity** - 末地水晶

### 4.3 装备实体
**目录**: `entities/equipment/`

1. **ArmorStandEntity** - 盔甲架

### 4.4 其他物品实体
**目录**: `entities/item/`

1. **FallingBlockEntity** - 下落方块
2. **TNTEntity** - TNT
3. **ExperienceOrbEntity** - 经验球
4. **AreaEffectCloudEntity** - 区域效果云

---

## 五、接口实现问题

### 5.1 已有方法但未实现接口的实体

| 实体 | 缺失接口 | 现有方法 |
|------|----------|----------|
| PigEntity | IRideable | hasSaddle(), setSaddle(), boost() |
| SheepEntity | IShearable | shear(), hasWool() |
| MooshroomEntity | IShearable | shear(), isShearable() |
| SnowGolemEntity | IShearable | shearPumpkin() |
| BeeEntity | IFlyingAnimal | isFlying(), setFlying() |
| ParrotEntity | IFlyingAnimal | isFlying(), setFlying() |

### 5.2 TODO汇总（约128处）

按优先级分类：

**高优先级** (影响核心功能):
- 世界查询接口实现（getEntitiesWithinAABB, getNearestPlayer等）
- 玩家交互逻辑（isBreedingItem检测等）
- 繁殖系统完善（spawnBaby创建幼体）

**中优先级** (影响特定功能):
- 粒子/音效系统集成
- 水方块检测
- 远程攻击生成投掷物

**低优先级** (优化项):
- 快速逆平方根近似
- 更精确的碰撞检测

---

## 六、README更新需求

### 需要更新的README文件

1. `src/common/entity/README.md` - ✅ 已更新（删除过时的living/mob目录）
2. `src/common/entity/entities/README.md` - 需要补充新实体
3. `src/common/entity/entities/passive/README.md` - 需要补充马类子实体
4. `src/common/entity/entities/monster/README.md` - 需要检查完整性
5. `src/common/entity/ai/brain/README.md` - 需要补充传感器/任务

---

## 七、实施顺序

### 第一阶段：核心功能
1. 实现ProjectileEntity基类和ArrowEntity
2. 实现ThrowableEntity和基础投掷物（Snowball, Egg, EnderPearl）
3. 实现FireballEntity和SmallFireballEntity

### 第二阶段：Boss和村民
1. 实现WitherEntity
2. 实现VillagerEntity和交易系统
3. 实现EnderDragonEntity（多阶段AI复杂）

### 第三阶段：载具和马类
1. 实现BoatEntity
2. 实现AbstractMinecartEntity和矿车变种
3. 实现HorseEntity等马类子实体

### 第四阶段：完善AI
1. 实现special/目录下的Goal
2. 实现Brain传感器和任务
3. 解决TODO项

### 第五阶段：其他实体
1. 悬挂实体
2. 效果实体
3. 装备实体

---

## 八、测试计划

每个新实体需要测试：
1. 基本属性（生命值、速度、攻击力等）
2. AI行为（Goal选择、优先级）
3. 交互逻辑（繁殖、驯服、骑乘等）
4. 边界条件

运行测试：
```powershell
cmake --build build --config Release
./build/bin/Release/mc_tests.exe
```
