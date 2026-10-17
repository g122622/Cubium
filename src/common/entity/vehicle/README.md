# 载具与落点工具（entity/vehicle）

为"实体从某个方块柱附近落地"提供统一判定：下马、床与重生锚的起身位置、重生锚与共享出生点的安全落点。

## 目录结构

```
entity/vehicle/
├── DismountHelper.hpp/cpp   # 地面高度计算、危险方块判定、安全落点搜索
└── README.md
```

## 内部模块关系

`DismountHelper` 是纯静态工具，不持有状态，被方块层的落点搜索（`BedBlock::findStandUpPosition`、
`RespawnAnchorBlock::findStandUpPosition`）、睡眠起床、服务端重生与出生点搜索复用。

## 上下游外部依赖关系

上游：`IWorld`（方块状态、碰撞、世界边界）、`BlockTags`（`CLIMBABLE`/`FIRE`/`INVALID_SPAWN_INSIDE`）、
`CampfireBlock::isLitCampfire`、`TrapDoorBlock::OPEN`、`VanillaBlocks`。

下游：`world/block/blocks/functional/BedBlock`、`world/block/blocks/functional/RespawnAnchorBlock`、
`entity/player/SleepManager`、`entity/entities/villager/VillagerEntity`、
`server/player/ServerPlayer`、`server/world/player/PlayerSpawnFinder`、`server/world/ServerWorld`。

## 容易踩的坑

1. **地面高度不是"下方是否实心"**：方块自身的可穿过形状（可攀爬方块、开启的活板门）被忽略后，
   取下一格形状的最高点减 1；两格都无可穿过形状时返回正无穷（不可站立）。
2. **安全模式两层含义**：拒绝危险方块（火/岩浆/岩浆块/点燃的营火/炼药锅岩浆/凋灵玫瑰/甜浆果/仙人掌/细雪），
   以及"地面高度 <= 0 且下方危险"的悬空；非安全模式允许这些位置。
3. **安全模式先、非安全模式后**：调用方必须两轮都搜完，不能一轮失败就放弃。
4. **玩家额外禁入 `INVALID_SPAWN_INSIDE`**：末地传送门/末地折跃门内部不允许重生。
5. **边界判定用 `WorldBorder::isWithinBounds(AABB)`**，不是 `intersects`：碰撞箱必须完全在边界内。
6. **标签系统未初始化时**（单元测试夹具）`BlockTags` 访问器返回空指针，需先查 `BlockTags::isInitialized()`。
