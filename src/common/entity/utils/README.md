# Entity Utils Module

实体系统的非模板工具函数模块，提供物品掉落相关的工具功能。

## 目录结构

```text
src/common/entity/utils/
├── ItemDropHelper.hpp   # 物品掉落工具类（随机速度计算、物品实体生成）
├── ItemDropHelper.cpp   # 物品掉落工具实现
├── EntityUtils.hpp      # 非模板工具声明（已废弃，仅保留命名空间框架）
├── EntityUtils.cpp      # 空实现
└── README.md            # 模块说明
```

## 内部模块关系

```
ItemDropHelper（物品掉落工具）
    ├── getBlockDropVelocity()      # 方块掉落式随机速度
    ├── getSimpleDropVelocity()     # 简单随机速度
    ├── getPlayerDropVelocity()     # 玩家丢弃物品速度
    ├── getGaussianVelocity()       # 高斯分布速度（发射器）
    ├── spawnItemEntity()           # 生成单个物品实体
    ├── spawnItemAtEntity()         # 在实体位置生成物品
    └── spawnItemEntities()         # 批量生成物品实体

EntityUtils（已废弃）
    └── 空命名空间，原 legacyTypeToTypeId 已迁移到 VanillaEntityTypeKeys
```

## 上下游依赖关系

### 上游依赖（本模块依赖的）

- `src/common/entity/core/Entity.hpp` - Entity 基类
- `src/common/entity/entities/item/ItemEntity.hpp` - ItemEntity 类
- `src/common/item/core/ItemStack.hpp` - ItemStack 类
- `src/common/world/IWorld.hpp` - IWorld 接口
- `src/common/util/math/random/Random.hpp` - 随机数生成器
- `src/common/util/math/MathConstants.hpp` - 数学常量（TWO_PI, DEG_TO_RAD）
- `src/common/world/block/BlockPos.hpp` - BlockPos 类

### 下游依赖（依赖本模块的）

- `src/common/entity/core/EntityUtils.hpp` - 模板型搜索、距离工具（core 目录，非本目录）
- 方块掉落处理器 - 调用 `spawnItemEntities()`
- 剪刀物品交互 - 调用 `spawnItemEntity()`（剪羊毛等场景）
- 玩家丢弃物品 - 调用 `getPlayerDropVelocity()`
- 发射器 - 调用 `getGaussianVelocity()`

## 容易踩的坑

### 1. 随机数生成器来源

`ItemDropHelper` 的方法需要调用方提供 `math::Random&` 引用。通常从 `IWorld::getRandom()` 或 `Entity::getRandom()` 获取。

### 2. 物品实体指针的有效性

`spawnItemEntity()` 返回的指针可能在后续操作后失效。如需后续操作，应保存 `EntityId` 而非原始指针，通过 `world->getEntity(id)` 获取实体。

### 3. 拾取延迟设置

新生成的物品若无延迟会立即被拾取。使用 `DEFAULT_PICKUP_DELAY = 10` ticks（0.5秒），或根据场景调整。

### 4. 不要把模板型搜索函数迁到这里

`findClosestEntity(...)` 等模板函数属于 `core/EntityUtils.hpp`（core 目录），不是本目录。

### 5. EntityUtils 已废弃

`EntityUtils` 命名空间已清空，`legacyTypeToTypeId()` 已删除。实体类型ID现在通过 `Entity::getTypeId()` 获取，由 EntityRegistry 在创建实体时设置。
