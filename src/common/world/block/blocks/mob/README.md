# 生物相关方块模块 (Mob Blocks)

生物相关方块模块提供与生物交互的方块实现。

## 目录结构

```
mob/
├── README.md              # 本文档
├── BeehiveBlock.hpp/cpp   # 蜂巢/蜂箱方块
├── TurtleEggBlock.hpp/cpp # 海龟蛋方块
├── InfestedBlock.hpp/cpp  # 被感染方块（蠹虫方块）
├── SpawnerBlock.hpp/cpp   # 刷怪笼方块
└── DragonBreathBlock.hpp/cpp # 龙息方块
```

## 方块类型

| 类名 | 说明 | 状态属性 | 实现进度 |
|------|------|----------|----------|
| `BeehiveBlock` | 蜂巢/蜂箱 | HONEY_LEVEL_0_5, FACING | 基础框架 |
| `TurtleEggBlock` | 海龟蛋 | EGGS_1_4, HATCH_0_2 | 完整实现 |
| `InfestedBlock` | 被感染方块 | 无 | 完整实现 |
| `SpawnerBlock` | 刷怪笼 | 无 | 基础框架 |
| `DragonBreathBlock` | 龙息 | 无 | **完整实现** |

## 核心机制

### 蜂巢
- 存储蜜蜂
- 收集蜂蜜（需要剪刀）
- 蜂蜜等级 0-5
- 满时可以用玻璃瓶收集

### 海龟蛋 (MC 1.16.5 对齐) - 已完成

#### 状态属性
- `EGGS_1_4`: 蛋数量 (1-4)
- `HATCH_0_2`: 孵化阶段 (0-2)

#### 放置规则
- 只能放置在沙子类方块上 (BlockTags::SAND)
- 放置在已有海龟蛋上时增加蛋数量（最大4个）

#### 孵化逻辑
- 随机 tick 触发孵化检查
- 白天（日光）或 1/500 随机概率时可孵化
- 孵化阶段 0 → 1 → 2 → 破壳
- 破壳时生成小海龟：
  - 调用 `TurtleEntity::setChild(true)` 设置为幼体
  - 调用 `TurtleEntity::setHomePos(pos)` 记住出生位置
  - 每个蛋生成一只小海龟

#### 踩踏机制 (MC 1.16.5 完整实现)
- `onEntityWalk`: 实体走过时有概率踩破蛋
- `onFallenUpon`: 实体摔落时有概率踩破蛋
- 实体类型检查 (`canTrample`):
  - **海龟和蝙蝠**: 不能踩破蛋
  - **非生物实体**: 不能踩破蛋（需要 LivingEntity）
  - **玩家**: 总是可以踩破
  - **其他生物**: 根据 mobGriefing 游戏规则
- 僵尸类实体 (`isZombieType`):
  - Zombie、Husk、Drowned 不会踩破蛋
  - 走过海龟蛋时直接无视

#### 音效
- ENTITY_TURTLE_EGG_CRACK: 孵化进度增加
- ENTITY_TURTLE_EGG_HATCH: 孵化完成
- ENTITY_TURTLE_EGG_BREAK: 蛋被踩破

### 被感染方块 (InfestedBlock) - 已完成

#### 特性
- 外观与普通方块相同
- 被破坏时生成蠹虫
- 更容易被破坏（硬度0.75）

#### 生成蠹虫逻辑 (`onBlockRemoved`)
- 仅在服务端生成（客户端跳过）
- 在方块中心位置生成蠹虫
- 位置计算：`x + 0.5, y, z + 0.5`

### 刷怪笼
- 自动生成生物
- 需要方块实体存储配置
- 创造模式可编辑

### 龙息
- 无碰撞体积
- 实体经过时造成伤害

## 实现状态

### TurtleEggBlock (已完成)
- [x] 状态属性: EGGS_1_4 (蛋数量), HATCH_0_2 (孵化阶段)
- [x] 放置检查: 只能在沙子上
- [x] 蛋堆叠: 放置时增加蛋数量
- [x] 孵化逻辑: 随机 tick 增加孵化阶段
- [x] 孵化生成小海龟: 设置幼体状态和出生位置
- [x] 踩踏机制: onEntityWalk, onFallenUpon
- [x] 实体类型检查: canTrample, isZombieType
- [x] 沙子检查: BlockTags::SAND
- [x] 音效: ENTITY_TURTLE_EGG_CRACK, ENTITY_TURTLE_EGG_HATCH, ENTITY_TURTLE_EGG_BREAK

### InfestedBlock (已完成)
- [x] 宿主方块ID存储
- [x] 破坏时生成蠹虫 (SilverfishEntity)
- [x] 服务端生成检查

### BeehiveBlock (基础框架)
- [x] 状态属性: HONEY_LEVEL_0_5 (蜂蜜等级 0-5), HORIZONTAL_FACING (朝向)
- [x] getHoneyLevel()/withHoneyLevel() 方法
- [x] 状态旋转/镜像支持
- [ ] 蜂蜜等级变化
- [ ] 蜜蜂存储/释放
- [ ] 与玻璃瓶/剪刀交互

### DragonBreathBlock (已完成)
- [x] onEntityCollision: 对碰撞的 LivingEntity 造成龙息伤害
- [x] 服务端检查: 仅在服务端执行伤害逻辑
- [x] 伤害类型: DamageSources::dragonBreath() (绕过护甲)
- [x] 单元测试: 实体碰撞伤害、客户端不伤害、多类型实体测试

## 使用方法

```cpp
// 创建蜂巢
auto beehive = std::make_unique<BeehiveBlock>(
    BlockProperties(Materials::WOOD)
        .hardness(0.6f)
);

// 创建海龟蛋
auto turtleEgg = std::make_unique<TurtleEggBlock>(
    BlockProperties(Materials::SAND)
        .hardness(0.5f)
);

// 创建刷怪笼
auto spawner = std::make_unique<SpawnerBlock>(
    BlockProperties(Materials.STONE)
        .hardness(5.0f)
);

// 创建被感染石头
auto infestedStone = std::make_unique<InfestedBlock>(
    stoneBlockId,  // 被感染的方块ID
    BlockProperties(Materials.CLAY)
        .hardness(0.0f)
);
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/block/BlockTags` | 方块标签 (SAND等) |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |
| `entity/core/Entity` | 实体接口 (踩踏检测) |
| `entity/core/LivingEntity` | 生物实体基类 (踩踏检测) |
| `entity/entities/passive/special/TurtleEntity` | 海龟实体 |
| `entity/entities/monster/arthropod/EndermiteEntity` | 蠹虫实体 |

## 测试覆盖

测试文件位于 `tests/common/world/block/blocks/MobBlocksTest.cpp`：

### BeehiveBlock 测试
- 创建和属性验证
- 蜂蜜等级设置和范围限制
- 朝向旋转和镜像

### TurtleEggBlock 测试
- 状态属性验证
- 形状获取
- 随机 tick 标记

### TurtleEggBlock 踩踏测试
- 玩家踩踏验证
- 海龟不能踩破蛋
- 僵尸类不踩破蛋（Zombie, Husk, Drowned）
- 蝙蝠不能踩破蛋
- 非生物实体不能踩破蛋

### TurtleEggBlock 孵化测试
- 孵化进度逻辑
- 无沙子时不孵化
- 客户端不生成实体

### InfestedBlock 测试
- 蠹虫生成验证
- 客户端不生成实体
- 蠹虫位置正确性

## 参考

- MC 1.16.5: net.minecraft.block.TurtleEggBlock
- MC 1.16.5: net.minecraft.block.BeehiveBlock
- MC 1.16.5: net.minecraft.block.InfestedBlock (SilverfishBlock)
- MC 1.16.5: net.minecraft.block.SpawnerBlock
