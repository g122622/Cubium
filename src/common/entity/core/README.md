# Entity Core Module

实体系统的核心框架，包含所有实体的基类和基础设施。

## 目录结构树

```
src/common/entity/core/
├── Entity.hpp/cpp                  # 所有实体的基类（位置、运动、碰撞、火焰等）
├── LivingEntity.hpp/cpp            # 有生命值的生物实体基类（生命值、吸收值、装备、药水效果）
├── MobEntity.hpp/cpp               # 有AI的生物实体基类（AI系统、目标选择）
├── CreatureEntity.hpp/cpp          # 陆地生物基类（寻路、步进）
├── FlyingEntity.hpp/cpp            # 飞行生物基类
├── AgeableEntity.hpp/cpp           # 成长系统基类（幼体→成体）
├── EntityType.hpp/cpp              # 实体类型定义
├── EntityRegistry.hpp              # 实体注册表（工厂模式创建实体）
├── EntityTypeIdNumber.hpp/cpp      # 实体类型ID常量（网络同步用）
├── EntityDataManager.hpp           # 实体数据同步管理（客户端-服务端数据同步）
├── EntityPose.hpp                  # 实体姿态枚举（站立、潜行、游泳、睡眠等）
├── EntitySize.hpp                  # 实体尺寸定义（宽度、高度、眼睛高度）
├── EntityClassification.hpp/cpp    # 实体分类（怪物、动物、环境等）
├── EntitySpawnPlacementRegistry.hpp/cpp  # 生成位置规则、SpawnReason枚举
├── EntityUtils.hpp                 # 模板型实体工具函数（搜索、距离）
├── DataParameter.hpp               # 数据参数定义（网络同步用）
├── MoverType.hpp                   # 移动类型枚举（自移、活塞、玩家、弹射物等）
├── BoostHelper.hpp                 # 可骑乘实体的鞍和加速管理（猪、炽足兽等）
├── VanillaEntities.hpp             # 原版实体类型注册声明
└── README.md
```

## 内部模块关系

```
继承层次：
Entity（基类：位置、运动、碰撞、火焰、流体检测）
├── LivingEntity（生命值、装备、药水效果、击退、空气供应）
│   ├── MobEntity（AI系统、目标选择、控制器、日光检测）
│   │   ├── CreatureEntity（陆地移动、寻路）
│   │   │   ├── AgeableEntity（成长系统）
│   │   │   │   └── AnimalEntity（繁殖系统，在../passive/）
│   │   │   └── MonsterEntity（敌对行为，在../monster/）
│   │   └── FlyingEntity（飞行移动）
│   └── Player（玩家特有功能，在../../player/）
└── ItemEntity（掉落物，在../）

辅助类依赖关系：
- Entity → EntityDataManager → DataParameter（数据同步）
- Entity → EntitySize → AxisAlignedBB（碰撞箱）
- EntityRegistry → EntityType → EntityTypeIdNumber（类型注册）
- BoostHelper → EntityDataManager（鞍和加速状态同步）
- EntitySpawnPlacementRegistry → SpawnReason（生成规则判断）
  - EntitySpawnPlacementRegistry → ISpawnWorldReader（世界状态查询接口）
  - EntitySpawnPlacementRegistry → BiomeTags（生物群系标签查询，如地表史莱姆生成）
  - EntitySpawnPlacementRegistry → InternalLightUtils（月相、光照计算）
  - EntitySpawnPlacementRegistry → SlimeChunkChecker（史莱姆区块判断）
```

## 上下游外部依赖关系

**上游依赖（本目录依赖的外部模块）**：
- `core/Types.hpp` - 基础类型定义（EntityId、Vector3等）
- `entity/attribute/` - 属性系统（generic.max_health等）
- `entity/damage/` - 伤害系统（DamageSource、DamageSources）
- `world/IWorld.hpp` - 世界级声音和位置查询入口
- `world/block/Block.hpp` - 方块交互回调
- `physics/PhysicsEngine.hpp` - 物理引擎（移动、碰撞）
- `util/math/random/Random.hpp` - 随机数生成器
- `util/nbt/` - NBT序列化
- `scoreboard/Team.hpp` - 队伍系统

**下游依赖（依赖本目录的外部模块）**：
- `entity/passive/` - 被动生物（动物）
- `entity/monster/` - 敌对生物
- `entity/projectile/` - 弹射物实体
- `entity/item/` - 物品实体
- `entity/vehicle/` - 载具实体
- `player/` - 玩家实体
- `world/World.hpp` - 世界实体管理
- `server/` - 服务端实体调度
- `client/renderer/entity/` - 客户端实体渲染

## MobEntity 生成初始化系统

MobEntity 提供 `finalizeSpawn()` 方法，在实体被创建并设置好位置后调用，用于根据难度和生成原因进行初始化。

### finalizeSpawn 调用链

```
finalizeSpawn(world, difficulty, spawnReason)
├── 设置 canPickUpLoot（概率 = 0.55 * specialMultiplier）
├── populateDefaultEquipmentSlots(random, difficulty)
│   └── 护甲生成概率 = 0.15 * specialMultiplier
│       └── getEquipmentForSlot(slot, armorLevel) → 护甲物品映射
└── populateDefaultEquipmentEnchantments(random, difficulty)
    ├── 武器附魔概率 = 0.25 * specialMultiplier
    └── 护甲附魔概率 = 0.5 * specialMultiplier（每个槽位独立）
```

### 子类覆写示例

- **ZombieEntity**：覆写 `populateDefaultEquipmentSlots()` 添加铁剑/铁锹
- **MonsterEntity**：调用 `finalizeSpawn()` 设置 `isDespawnPeaceful()`

### finalizeSpawn 调用位置

所有 MobEntity 生成路径必须在 `spawnEntity()` 之前调用 `finalizeSpawn()`：

| 生成路径 | SpawnReason | 文件 |
|---------|-------------|------|
| 自然生成 | Natural | NaturalSpawner.cpp |
| 村庄围攻 | Event | VillageSiege.cpp |
| 刷怪蛋 | SpawnEgg | SpawnEggItem.cpp |
| /summon 命令 | Command | SummonCommand.cpp |
| 陷阱骷髅马 | Trigger | SkeletonHorseEntity.cpp |
| 袭击 | Event | Raid.cpp |
| 繁殖 | Breeding | BreedGoal.cpp |
| 史莱姆分裂 | Reinforcement | SlimeEntity.cpp |
| 恼鬼召唤 | MobSummons | EvokerEntity.cpp |
| 蠹虫生成 | Event | InfestedBlock.cpp |
| 雪傀儡/铁傀儡 | Event | MelonPumpkinBlocks.cpp |
| 海龟孵化 | Natural | TurtleEggBlock.cpp |
| 远古守卫者 | Structure | OceanMonumentPieces.cpp |
| 结构模板 | Structure | Template.cpp |
| 区块生成 | ChunkGeneration | ServerWorld.cpp, MinecraftServer.cpp |
| 僵尸村民治愈 | Conversion | ZombieVillagerEntity.cpp |

### getEquipmentForSlot 护甲等级映射

| armorLevel | 材质 | Head | Chest | Legs | Feet |
|-----------|------|------|-------|------|------|
| 0 | 皮革 | LEATHER_HELMET | LEATHER_CHESTPLATE | LEATHER_LEGGINGS | LEATHER_BOOTS |
| 1 | 铜(暂铁) | IRON_HELMET | IRON_CHESTPLATE | IRON_LEGGINGS | IRON_BOOTS |
| 2 | 金 | GOLDEN_HELMET | GOLDEN_CHESTPLATE | GOLDEN_LEGGINGS | GOLDEN_BOOTS |
| 3 | 锁链 | CHAINMAIL_HELMET | CHAINMAIL_CHESTPLATE | CHAINMAIL_LEGGINGS | CHAINMAIL_BOOTS |
| 4 | 铁 | IRON_HELMET | IRON_CHESTPLATE | IRON_LEGGINGS | IRON_BOOTS |
| 5 | 钻石 | DIAMOND_HELMET | DIAMOND_CHESTPLATE | DIAMOND_LEGGINGS | DIAMOND_BOOTS |

注意：armorLevel=1 对应铜材质，但铜护甲尚未实现，当前回退为铁护甲。

## 容易踩的坑

### 区块高度常量混淆
- `CHUNK_HEIGHT` 和 `MAX_BUILD_HEIGHT` 目前值相同但语义不同
- `CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT`
- 未来 `MIN_BUILD_HEIGHT` 可能从 0 改为 -64，届时 `CHUNK_HEIGHT` 将不等于 `MAX_BUILD_HEIGHT`
- 务必根据语义选择正确常量，不要硬编码

### 步进高度（stepHeight）设置
- Entity 基类默认 stepHeight = 0.0f（无步进能力）
- LivingEntity 默认 stepHeight = 0.6f（可走台阶）
- 铁傀儡、马、末影人、溺尸、劫掠兽、海龟等为 1.0f（可走完整方块）
- 盔甲架为 0.0f（无法步进）
- 骑乘时步高会动态变化（IRideable）

### 火焰系统注意事项
- `setFire(seconds)` 将秒转换为 tick（×20），只在当前值较小时更新
- `forceFireTicks(ticks)` 直接设置值，用于增减火焰时间
- 火焰免疫由 `EntityType::immuneToFire()` 标志决定
- 烈焰人、恶魂、岩浆怪、猪灵系、疣猪兽、潜影贝、Boss实体免疫火焰

### 空气供应与溺水
- 空气值可从正数变成负数（用于溺水计时）
- 当空气值降到 -20 时重置为 0 并触发溺水伤害
- 亡灵生物 `canBreatheUnderwater()` 返回 true，不会溺水
- WaterMobEntity 使用反逻辑：水中恢复，陆地上消耗

### 队伍关系判断
- 使用**指针相等性**比较，而非队伍名称比较
- 两个 Team 对象即使名称相同，指针不同也不算同一队伍
- 没有队伍的实体（`getTeam()` 返回 nullptr）不会与任何队伍匹配

### 传送系统使用
- `attemptTeleport(x, y, z)` - 安全传送，自动查找地面
- `randomTeleport(range, playEffects, avoidFluid)` - 随机传送
- 传送会自动重置运动向量

### 类型标识符获取
- `typeId()` 返回 `EntityTypeIdNumber` 命名空间中的常量
- `legacyType()` 返回 `LegacyEntityType` 枚举（旧版，仅用于兼容）
- 新代码应使用 `typeId()`

### ISpawnWorldReader 接口
- 定义在 `EntitySpawnPlacementRegistry.hpp` 中，用于生成检查的最小世界读取接口
- 主要方法：
  - `getBlockState(x, y, z)` - 获取方块状态
  - `isInWorldBounds(x, y, z)` - 检查位置是否在世界范围内
  - `getHeight(type, x, z)` - 获取高度图值
  - `getBiome(x, y, z)` - 获取生物群系ID
  - `seed()` - 获取世界种子（史莱姆区块判断等确定性生成条件）
  - `difficulty()` - 获取世界难度（和平难度拒绝怪物生成）
  - `dayTime()` - 获取游戏日时间（月相计算等基于时间的生成条件）
  - `getMaxLocalRawBrightness(x, y, z)` - 获取最大原始亮度（光照等级生成条件）
- 适配器实现：`NaturalSpawner::ServerWorldAdapter`（服务端）、`WorldGenRegionAdapter`（世界生成）

### 数据参数注册
- 数据参数必须在 `registerData()` 中注册
- 参数 ID 在实体类继承链中必须唯一
- 客户端通过 `EntityDataManager` 同步数据

### 鞍与加速系统（BoostHelper）
- `BoostHelper` 的加速状态不保存到 NBT（MC 1.16.5 行为）
- 只有鞍状态会持久化
- 客户端通过 `syncFromDataManager()` 同步状态

### 玩家交互
- `processInitialInteract()` - 无位置信息的交互（骑乘、驯服）
- `applyPlayerInteraction()` - 有位置信息的交互（盔甲架装备槽）
- 基类默认返回 `ActionResultType::Pass`

### 水花溅射效果
- 速度因子 f1 决定水花强度和声音选择
- f1 < 0.25 用普通溅水声，f1 >= 0.25 用高速溅水声
- 观察者模式玩家不产生水花效果

### 发光效果判断
- `isGlowing()` 客户端检查数据参数标志位，服务端检查 m_glowing 字段
- 发光效果来源：发光药水、Entity发光标志、团队发光规则

### 箭矢计数与脱落
- 箭矢数量越多，脱落越快
- 脱落计时器公式：`20 * (30 - arrowCount)` ticks
- 箭矢计数仅用于渲染，不影响游戏逻辑

### 吸收值（金苹果额外生命）
- 使用 `absorptionAmount()` 获取吸收值，`setAbsorptionAmount(f32)` 设置吸收值
- `setAbsorptionAmount` 会将值限制在 `[0, maxAbsorption]` 范围内（与 MC 原版一致）
- `maxAbsorption` 来自 `generic.max_absorption` 属性，默认值为 0.0
- Player 不再需要单独定义吸收值方法，统一使用 LivingEntity 基类的实现
- 吸收值在 `actuallyHurt` 中通过 `setAbsorptionAmount` 消耗，确保限制逻辑生效
- NBT 序列化键为 `"AbsorptionAmount"`，反序列化也使用 `setAbsorptionAmount`
