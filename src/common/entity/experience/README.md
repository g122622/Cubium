# 经验系统模块 (Experience System)

本目录包含 Minecraft 经验系统的核心实现。

## 目录结构

```
experience/
├── ExperienceConstants.hpp    # 经验系统常量（经验球参数、分割表、矿石/生物经验掉落）
├── ExperienceManager.hpp/cpp  # 经验管理器（等级、进度、升级、附魔消耗）
├── ExperienceUtils.hpp        # 经验工具函数（分割、颜色计算、死亡掉落）
└── README.md                  # 本文档
```

## 内部模块关系

```
ExperienceManager（经验管理器）
    ├── 依赖 ExperienceConstants（常量）
    ├── 依赖 ExperienceUtils（工具函数）
    └── 关联 Player（玩家实体）

ExperienceUtils（工具函数）
    └── 依赖 ExperienceConstants（常量）

ExperienceConstants（常量）
    └── 独立模块，无依赖
```

**职责划分：**
- **ExperienceConstants**：定义经验球存活时间、追踪范围、拾取延迟、分割值表、矿石/生物经验掉落范围等常量
- **ExperienceManager**：管理玩家经验状态（等级、进度、总经验），处理升级/降级、附魔消耗、死亡掉落计算
- **ExperienceUtils**：提供经验分割、经验球大小/颜色计算、矿石/生物经验随机生成等工具函数

## 上下游外部依赖关系

### 本模块依赖的外部模块

- `common/entity/entities/player/Player.hpp` - 玩家实体（升级音效播放）
- `common/core/Types.hpp` - 基础类型定义
- `common/sound/SoundEvents.hpp` - 音效事件常量
- `common/util/math/random/Random.hpp` - 随机数生成器
- `common/util/math/MathConstants.hpp` - 数学常量

### 依赖本模块的外部模块

- `entities/player/Player.hpp/cpp` - 玩家实体持有 ExperienceManager 实例
- `entities/orb/ExperienceOrbEntity.hpp/cpp` - 经验球实体使用 ExperienceUtils 分割经验、计算颜色
- `entities/boss/EnderDragonEntity.cpp` - 末影龙死亡时使用 ExperienceUtils 分割大量经验
- `entities/core/MobEntity.cpp` - 生物死亡时掉落经验
- `server/world/drop/BlockDropHandler.cpp` - 方块掉落时生成经验
- `world/storage/player/PlayerDataManager.cpp` - 玩家数据序列化/反序列化经验状态
- `client/renderer/trident/entity/renderer/projectile/ExperienceOrbRenderer.cpp` - 经验球渲染使用颜色计算

## 容易踩的坑

### 1. 经验进度条容量公式

经验进度条容量（升级所需经验）随等级变化：
- 等级 0-14：`7 + level * 2`（范围 7-35）
- 等级 15-29：`37 + (level - 15) * 5`（范围 37-107）
- 等级 30+：`112 + (level - 30) * 9`（范围 112-382）

**注意**：容量随等级变化，升级时必须用"乘旧容量、除新容量"算法保持进度，否则会丢失或溢出经验。

### 2. 负经验处理

`addExperience()` 支持负值，会触发降级逻辑。但等级为 0 时进度归零，不会出现负等级。`onEnchant()` 直接消耗等级不检查是否足够，等级变负时会重置为 0。

### 3. 升级音效触发条件

升级音效仅在等级是 5 的倍数（5, 10, 15...）且距离上次播放至少 100 tick（5 秒）时播放。音量根据等级计算，等级 > 30 时固定最大值。

### 4. 经验球分割值

经验分割使用固定的 11 档值表：`{2477, 1237, 617, 307, 149, 73, 37, 17, 7, 3, 1}`。单个经验球最大值为 2477，大量经验会被分割成多个经验球。

### 5. 死亡掉落经验上限

玩家死亡掉落经验 = `min(level * 7, 100)`，上限 100 点。这意味着等级 15 以上的玩家死亡都只掉落 100 点经验。

### 6. 经验修补比例

经验修补附魔：每 2 点经验修复 1 点耐久。`durabilityToXp()` 和 `xpToDurability()` 进行双向转换。

### 7. 附魔种子重置

每次附魔后必须调用 `resetXpSeed()` 生成新的随机种子，用于随机化附魔选项。`onEnchant()` 会自动处理。

### 8. 同步状态

`ExperienceManager` 维护 `m_dirty` 标志，经验变化后需要同步到客户端。使用 `isDirty()` 检查，同步后调用 `clearDirty()`。

## 参考

- Minecraft 1.16.5: `net.minecraft.entity.player.PlayerEntity`（经验相关逻辑）
- Minecraft 1.16.5: `net.minecraft.entity.item.ExperienceOrbEntity`（经验球实体）
- Minecraft Wiki: [Experience](https://minecraft.fandom.com/wiki/Experience)
