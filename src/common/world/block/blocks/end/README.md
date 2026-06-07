# 末地方块模块 (End Blocks)

末地方块模块提供末地维度相关方块的实现，包括末地传送门、紫颂植物、龙蛋等。

## 目录结构

```
end/
├── EndPortalBlock.hpp/cpp      # 末地传送门方块（实体碰撞触发传送）
├── EndPortalFrameBlock.hpp/cpp # 末地传送门框架（放置末影之眼）
├── EndGatewayBlock.hpp/cpp     # 末地折跃门（关联方块实体传送）
├── ChorusPlantBlock.hpp/cpp    # 紫颂植物（六方向连接形状）
├── ChorusFlowerBlock.hpp/cpp   # 紫颂花（生长阶段0-5）
├── DragonEggBlock.hpp/cpp      # 龙蛋（点击传送、下落延迟5tick）
└── README.md                   # 本文档
```

## 方块类型

| 类名 | 说明 | 状态属性 | 参考 MC 源码 |
|------|------|----------|--------------|
| `EndPortalBlock` | 末地传送门方块 | 无 | `EndPortalBlock` |
| `EndPortalFrameBlock` | 末地传送门框架 | EYE, HORIZONTAL_FACING | `EndPortalFrameBlock` |
| `EndGatewayBlock` | 末地折跃门 | 无（关联 EndGatewayEntity） | `EndGatewayBlock` |
| `ChorusPlantBlock` | 紫颂植物 | NORTH/SOUTH/EAST/WEST/DOWN/UP | `ChorusPlantBlock` |
| `ChorusFlowerBlock` | 紫颂花 | AGE_0_5 | `ChorusFlowerBlock` |
| `DragonEggBlock` | 龙蛋 | 无（继承 FallingBlock） | `DragonEggBlock` |

## 内部模块关系

```
Block (基类)
├── EndPortalBlock      ─┐
├── EndPortalFrameBlock  │
├── EndGatewayBlock      ├─ 直接继承 Block
├── ChorusPlantBlock     │
├── ChorusFlowerBlock   ─┘
└── FallingBlock
    └── DragonEggBlock      ─ 继承 FallingBlock（下落行为）
```

**方块间连接关系**：
- `ChorusPlantBlock` 和 `ChorusFlowerBlock` 相互连接
- `ChorusPlantBlock` 向下可连接到末地石（作为生长基底）

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 依赖 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/blocks/FallingBlock` | 龙蛋下落行为基类 |
| `physics/collision/CollisionShape` | 碰撞形状 |
| `util/property/Properties` | 方块属性定义 |
| `util/Direction` | 方向枚举 |
| `util/math/random/Random` | 随机数生成 |
| `world/IWorld`、`world/IBlockReader` | 世界接口 |
| `world/block/registry/VanillaBlocks` | 原版方块引用（末地石、紫颂花等） |
| `world/blockentity/BlockEntityType` | 折跃门方块实体类型 |
| `client/renderer/trident/particle/ParticleTypes` | 龙蛋传送粒子 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/block/registry/VanillaBlocks.cpp` | 方块注册 |
| `world/dimension/teleport/Teleporter` | 传送逻辑（传送门方块） |
| `world/gen/structure/structures/StrongholdPieces` | 要塞结构生成（传送门框架） |
| `entity/entities/projectile` | 投射物（末影珍珠触发传送门） |

## 容易踩的坑

### 1. 紫颂植物形状索引位顺序

Direction 枚举顺序：Down=0, Up=1, North=2, South=3, West=4, East=5。形状索引必须按此顺序计算位掩码，否则碰撞形状会错乱。

### 2. 紫颂植物连接检查使用指针比较

`_canConnect` 使用 `adjState->is(this)` 检查相邻方块是否是紫颂植物。测试时必须使用 `VanillaBlocks::CHORUS_PLANT` 指针，而不是创建新的实例。

### 3. 传送门传送只设置标志

`EndPortalBlock::onEntityCollision` 只设置传送请求标志，实际传送由服务端的 `ServerDimensionManager` 处理，不要在方块代码中直接执行传送。

### 4. 龙蛋下落延迟

龙蛋的下落延迟是 5 tick（`FALL_DELAY_TICKS = 5`），比普通下落方块的 2 tick 更长，这是 MC 原版行为。

### 5. 龙蛋传送范围和尝试次数

传送范围：X/Z 方向 -15 ~ +15，Y 方向 -7 ~ +7。最多尝试 1000 次寻找有效位置。如果全部失败，龙蛋不会传送。

### 6. 紫颂花 isValidPosition 空气检查

当紫颂花下方是空气时，需要检查水平四个方向是否恰好有一个紫颂植物，且其他三个水平方向必须是空气。这个逻辑比较复杂，修改时需谨慎。

## 参考资料

- MC 1.16.5 `net.minecraft.block.EndPortalBlock`
- MC 1.16.5 `net.minecraft.block.EndPortalFrameBlock`
- MC 1.16.5 `net.minecraft.block.EndGatewayBlock`
- MC 1.16.5 `net.minecraft.block.ChorusPlantBlock`（SixWayBlock 实现）
- MC 1.16.5 `net.minecraft.block.ChorusFlowerBlock`
- MC 1.16.5 `net.minecraft.block.DragonEggBlock`
