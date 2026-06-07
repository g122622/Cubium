# 傀儡实体模块

本目录包含傀儡实体的实现，包括铁傀儡和雪傀儡。

## 目录结构

```
golem/
├── GolemEntity.hpp       # 傀儡基类（实现 IAngerable 接口）
├── GolemEntity.cpp       # 傀儡基类实现
├── SnowGolemEntity.hpp   # 雪傀儡实体（IShearable, IRangedAttackMob）
├── SnowGolemEntity.cpp   # 雪傀儡实现
├── IronGolemEntity.hpp   # 铁傀儡实体
├── IronGolemEntity.cpp   # 铁傀儡实现
└── README.md             # 本文件
```

## 内部模块关系

```
                ┌─────────────────┐
                │   CreatureEntity │
                └────────┬────────┘
                         │
                ┌────────▼────────┐
                │   GolemEntity    │
                │  (IAngerable)    │
                └────────┬────────┘
                         │
       ┌─────────────────┼─────────────────┐
       │                 │                 │
┌──────▼──────┐   ┌──────▼──────┐   ┌──────▼──────┐
│SnowGolemEntity│  │IronGolemEntity│  │ (其他傀儡) │
│(IShearable)  │   │              │   │             │
│(IRangedAttack)│  │              │   │             │
└──────────────┘   └──────────────┘   └─────────────┘
```

- **GolemEntity**：傀儡基类，继承 `CreatureEntity` 并实现 `IAngerable` 接口，提供愤怒系统
- **SnowGolemEntity**：雪傀儡，实现远程攻击（雪球）和剪切（南瓜头）功能
- **IronGolemEntity**：铁傀儡，实现近战攻击和村民保护功能

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `CreatureEntity` - 生物基类
- `IAngerable` - 愤怒接口
- `IRangedAttackMob` - 远程攻击接口
- `IShearable` - 可剪切接口
- `BiomeRegistry` - 生物群系注册表（雪傀儡温度检查）
- `VanillaBlocks` / `BlockItemRegistry` - 方块和物品注册表
- `SoundEvents` - 声音事件定义
- `GameRules` - 游戏规则系统（mobGriefing）
- AI 目标系统（RangedAttackGoal、MeleeAttackGoal 等）

### 下游依赖（依赖本模块）

- 实体注册系统 - 注册傀儡实体类型
- 世界生成系统 - 村庄铁傀儡生成
- 玩家交互系统 - 铁傀儡建造检测
- 实体 AI 系统 - 使用傀儡特定的 AI 目标

## 容易踩的坑

1. **继承链顺序**：`GolemEntity` 继承自 `CreatureEntity` 而非 `MobEntity`，与 MC 1.16.5 保持一致。

2. **温度检查**：`Biome::getTemperature(y)` 会考虑高度因素，不是简单的生物群系基础温度。雪傀儡的融化检查和雪层放置都依赖此方法。

3. **雪层放置条件**：需要同时满足：
   - `mobGriefing` 游戏规则为 true
   - 实体存活
   - 不在客户端
   - 生物群系温度 < 0.8

4. **剪切物品获取**：通过 `BlockItemRegistry::getBlockItem()` 获取 CARVED_PUMPKIN 对应的物品，确保物品系统已初始化后再调用。

5. **远程攻击命名空间**：雪球实体使用 `entity::SnowballEntity`（在 `mc::entity` 命名空间），需正确使用命名空间。

6. **玩家创建标记**：铁傀儡有 `m_playerCreated` 标记，玩家创建的铁傀儡不攻击玩家，需要在生成时正确设置。

7. **苦力怕排除**：铁傀儡不攻击苦力怕，`canAttackEntity()` 和 AI 目标选择器都有相关过滤逻辑。
