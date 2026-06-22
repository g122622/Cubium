# AI工具模块 (entity/ai/util)

本目录包含AI系统的工具类和辅助功能。

## 目录结构

```
util/
├── PiglinAi.hpp             # 猪灵AI工具类（愤怒传播、金盔甲检测等）
├── PiglinAi.cpp             # 猪灵AI工具类实现
├── RandomPositionGenerator.hpp  # 随机位置生成器（为AI目标生成智能随机位置）
├── RandomPositionGenerator.cpp  # 随机位置生成器实现
└── README.md                    # 本文档
```

## 内部模块关系

- `PiglinAi` 独立于 `RandomPositionGenerator`，两者无依赖关系。

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

| 模块 | 依赖内容 |
|------|----------|
| `entity/core` | `EntityUtils` - 实体查询、`Entity` - canSee方法 |
| `entity/entities/monster/nether` | `PiglinEntity` - 猪灵实体类型 |
| `entity/interfaces` | `IAngerable` - 愤怒接口 |
| `world` | `IWorld` - 世界查询接口、`GameRules` - UNIVERSAL_ANGER规则 |
| `entity/entities/player` | `Player` - 玩家实体 |

### 下游依赖（依赖本模块的外部模块）

| 模块 | 使用方式 |
|------|----------|
| `world/block/Block.cpp` | `PiglinAi::angerNearbyPiglins()` - 破坏GUARDED_BY_PIGLINS方块时激怒猪灵 |
| `world/block/blocks/functional/BarrelBlock.cpp` | `PiglinAi::angerNearbyPiglins()` - 打开木桶时激怒猪灵 |
| `world/block/blocks/ChestBlock.cpp` | `PiglinAi::angerNearbyPiglins()` - 打开箱子时激怒猪灵 |
| `world/block/blocks/nether/EnderChestBlock.cpp` | `PiglinAi::angerNearbyPiglins()` - 打开末影箱时激怒猪灵 |
| `world/block/blocks/ShulkerBoxBlock.cpp` | `PiglinAi::angerNearbyPiglins()` - 打开潜影盒时激怒猪灵 |

## 容易踩的坑

### 1. findRandomTargetBlock vs findRandomTarget 的区别

**问题**：混淆这两个方法导致飞行实体或水生生物行为异常。

**区别**：
- `findRandomTarget` - 要求位置可行走，适用于陆地生物
- `findRandomTargetBlock` - 不要求可行走，适用于飞行实体和水生生物

### 2. getGroundHeight 返回值

**问题**：返回 -1 表示找不到地面，但调用者未检查。

**注意**：返回值使用 `world::MIN_BUILD_HEIGHT` 作为无效标记，检查时应用 `>= MIN_BUILD_HEIGHT` 判断有效性。

### 3. calculatePositionScore 返回负值

**问题**：返回 -1000.0f 表示危险位置（如岩浆附近），但调用者可能误以为是评分计算错误。

**原因**：位置评分使用 `creature->getPathWeight()` 加上危险检测，岩浆位置直接返回 -1000.0f。

### 4. MAX_ATTEMPTS 限制

**问题**：最多尝试 10 次，复杂地形可能找不到有效位置。

**解决**：调用者需要处理返回 false 的情况，不应假设一定能找到位置。

### 5. directionBias 参数使用

**问题**：`findRandomTargetTowards` 的 targetPos 是零向量时方向偏好无效。

**注意**：传入零向量或非常短的向量会被视为无方向偏好，等同于 `findRandomTarget`。
