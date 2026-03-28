# 生物相关方块模块 (Mob Blocks)

生物相关方块模块提供与生物交互的方块实现。

## 目录结构

```
mob/
├── README.md           # 本文档
├── MobBlocks.hpp/cpp   # 所有生物相关方块
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `BeehiveBlock` | 蜂巢/蜂箱 | HONEY_LEVEL_0_5, FACING |
| `TurtleEggBlock` | 海龟蛋 | EGGS_1_4, HATCH_0_2 |
| `InfestedBlock` | 被感染方块 | 无 |
| `SpawnerBlock` | 刷怪笼 | 无 |
| `DragonBreathBlock` | 龙息 | 无 |

## 核心机制

### 蜂巢
- 存储蜜蜂
- 收集蜂蜜（需要剪刀）
- 蜂蜜等级 0-5
- 满时可以用玻璃瓶收集

### 海龟蛋
- 放置在沙滩上
- 随机孵化（3个阶段）
- 可堆叠1-4个
- 玩家踩踏可能破坏

### 被感染方块
- 外观与普通方块相同
- 破坏时生成蠹虫
- 更容易被破坏

### 刷怪笼
- 自动生成生物
- 需要方块实体存储配置
- 创造模式可编辑

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
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |
