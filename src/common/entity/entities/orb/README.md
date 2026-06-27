# 经验球实体模块 (Orb Entities)

本目录包含经验球实体的实现。

## 目录结构

```
orb/
├── ExperienceOrbEntity.hpp    # 经验球实体头文件
├── ExperienceOrbEntity.cpp    # 经验球实体实现
└── README.md                  # 本文档
```

## 内部模块关系

```
ExperienceOrbEntity
├── 继承 Entity (core/Entity.hpp)
├── 依赖 EntityUtils::findClosestEntity<Player>() 玩家搜索
├── 依赖 ExperienceConstants 常量
├── 依赖 ExperienceUtils 工具函数
└── 依赖 ExperienceManager 经验管理
```

## 上下游外部依赖关系

**上游依赖（本目录依赖）**：
- `core/Entity.hpp` - 实体基类
- `core/EntityUtils.hpp` - 实体工具函数（玩家搜索）
- `entities/player/Player.hpp` - 玩家实体
- `experience/ExperienceConstants.hpp` - 经验常量
- `experience/ExperienceUtils.hpp` - 经验工具函数
- `experience/ExperienceManager.hpp` - 经验管理器
- `inventory/PlayerInventory.hpp` - 玩家物品栏（经验修补）
- `item/enchantment/EnchantmentHelper.hpp` - 附魔检测（经验修补）
- `world/IWorld.hpp` - 世界接口

**下游依赖（依赖本目录）**：
- `core/EntityRegistry.hpp` - 注册 `minecraft:experience_orb` 实体类型
- 生物死亡时掉落经验 → 经验球生成
- 玩家附魔/修复时消耗经验
- 矿石开采时掉落经验球

## 容易踩的坑

### 1. 拾取延迟默认值为 0

构造函数中 `m_pickupDelay = 0`，与 MC 1.16.5 一致。生成经验球时通常需要设置 `setPickupDelay(10)`。

### 2. 玩家搜索缓存间隔

玩家搜索使用缓存机制：每 `20 + entityId % 100` ticks 搜索一次，不是每 tick 都搜索。

### 3. 地面滑度依赖脚下方块

`_updateMovement()` 使用脚下方块的 `getSlipperiness()` 计算摩擦力，支持史莱姆块、冰块等特殊方块。不要硬编码滑度值。

### 4. 经验修补随机选择

经验修补从所有有损坏且带经验修补附魔的装备中随机选择一件修复，不是固定顺序。

### 5. 经验球合并上限

单个经验球最大经验值 `MAX_ORB_SIZE = 2477`，合并后超过此值则不合并。

### 6. 经验球大小等级

经验球有 11 种大小等级 (0-10)，根据经验值决定，用于渲染纹理选择。使用 `getOrbSize()` 获取。

### 7. 常量来源

所有常量（`MAX_ORB_SIZE`, `MAX_AGE`, `TRACKING_RANGE`, `PICKUP_DISTANCE` 等）都来自 `ExperienceConstants.hpp`，不要硬编码。

### 8. 伤害处理

经验球有 5 点生命值（`m_health = 5`），覆写了 `Entity::hurt()`。受伤时减少生命值并标记 `hurtMarked` 以同步速度到客户端，生命值归零时调用 `discard()` 销毁。对应 MC Java 的 `ExperienceOrb.hurtServer()`。
