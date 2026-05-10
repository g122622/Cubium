# 生物相关方块模块 (Mob Blocks)

生物相关方块模块提供与生物交互的方块实现。

## 目录结构

```
mob/
├── README.md           # 本文档
├── MobBlocks.hpp/cpp   # 所有生物相关方块
```

## 方块类型

| 类名 | 说明 | 状态属性 | 实现进度 |
|------|------|----------|----------|
| `BeehiveBlock` | 蜂巢/蜂箱 | HONEY_LEVEL_0_5, FACING | 基础框架 |
| `TurtleEggBlock` | 海龟蛋 | EGGS_1_4, HATCH_0_2 | 孵化/踩踏完成 |
| `InfestedBlock` | 被感染方块 | 无 | 基础框架 |
| `SpawnerBlock` | 刷怪笼 | 无 | 基础框架 |
| `DragonBreathBlock` | 龙息 | 无 | 基础框架 |

## 核心机制

### 蜂巢
- 存储蜜蜂
- 收集蜂蜜（需要剪刀）
- 蜂蜜等级 0-5
- 满时可以用玻璃瓶收集

### 海龟蛋 (MC 1.16.5 对齐)
- 只能放置在沙子类方块上 (BlockTags::SAND)
- 支持蛋堆叠 (1-4个蛋，放置时叠加)
- 随机孵化 (3个阶段: HATCH 0-2)
- 孵化条件: 白天或 1/500 随机概率
- 踩踏机制:
  - 实体走过: 1/100 概率破坏
  - 实体摔落: 1/3 概率破坏 (僵尸类除外)
  - 海龟和蝙蝠不会踩破蛋
- 孵化完成后生成小海龟 (TODO: 实体生成)

### 被感染方块
- 外观与普通方块相同
- 破坏时生成蠹虫
- 更容易被破坏

### 刷怪笼
- 自动生成生物
- 需要方块实体存储配置
- 创造模式可编辑

## 实现状态

### TurtleEggBlock (已完成)
- [x] 状态属性: EGGS_1_4 (蛋数量), HATCH_0_2 (孵化阶段)
- [x] 放置检查: 只能在沙子上
- [x] 蛋堆叠: 放置时增加蛋数量
- [x] 孵化逻辑: 随机 tick 增加孵化阶段
- [x] 踩踏机制: onEntityWalk, onFallenUpon
- [x] 沙子检查: BlockTags::SAND
- [ ] 声音效果: ENTITY_TURTLE_EGG_CRACK, ENTITY_TURTLE_EGG_HATCH, ENTITY_TURTLE_EGG_BREAK
- [ ] 孵化时生成小海龟

### BeehiveBlock (基础框架)
- [x] 状态属性: HONEY_LEVEL_0_5 (蜂蜜等级 0-5), HORIZONTAL_FACING (朝向)
- [x] getHoneyLevel()/withHoneyLevel() 方法
- [x] 状态旋转/镜像支持
- [ ] 蜂蜜等级变化
- [ ] 蜜蜂存储/释放
- [ ] 与玻璃瓶/剪刀交互

## 使用方法

```cpp
// 创建蜂巢
auto beehive = std::make_unique<BeehiveBlock>(
    BlockProperties(Materials::WOOD)
        .hardness(0.6f)
);

// 创建海龟蛋
auto turtleEgg = std::make_unique<TurtleEggBlock>(
    BlockProperties(Materials::EGG)
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

## 参考

- MC 1.16.5: net.minecraft.block.TurtleEggBlock
- MC 1.16.5: net.minecraft.block.BeehiveBlock
- MC 1.16.5: net.minecraft.block.InfestedBlock
- MC 1.16.5: net.minecraft.block.SpawnerBlock
