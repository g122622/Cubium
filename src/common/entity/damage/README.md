# Damage 模块

战斗伤害追踪系统，负责记录和管理实体受到的伤害事件，用于生成死亡消息和统计战斗数据。

## 目录结构

```
src/common/entity/damage/
├── DamageSource.hpp      # 伤害来源定义（核心类型，包含 DamageType 枚举和三类伤害来源类）
├── CombatEntry.hpp       # 战斗条目（记录单次伤害事件）
├── CombatEntry.cpp       # 战斗条目实现
├── CombatTracker.hpp     # 战斗追踪器（记录战斗历史，生成死亡消息）
└── CombatTracker.cpp     # 战斗追踪器实现
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │   LivingEntity  │
                    │  (战斗追踪所有者)  │
                    └────────┬────────┘
                             │ 拥有
                             ▼
┌──────────────────────────────────────────────────────┐
│                    CombatTracker                      │
│              (战斗追踪器 - 记录战斗历史)                 │
└───────────────────────────┬──────────────────────────┘
                            │ 包含多个
                            ▼
┌──────────────────────────────────────────────────────┐
│                    CombatEntry                        │
│              (战斗条目 - 单次伤害记录)                   │
└───────────────────────────┬──────────────────────────┘
                            │ 引用
                            ▼
┌──────────────────────────────────────────────────────┐
│                    DamageSource                       │
│              (伤害来源 - 抽象基类)                      │
├───────────────────────┬──────────────────────────────┤
│                       │                              │
▼                       ▼                              ▼
┌──────────────┐  ┌──────────────────┐  ┌───────────────────────┐
│Environmental │  │ EntityDamage     │  │ IndirectEntity        │
│   Damage     │  │    Source        │  │    DamageSource       │
│ (环境伤害)    │  │  (直接实体伤害)   │  │   (间接实体伤害)        │
└──────────────┘  └──────────────────┘  └───────────────────────┘
```

- **DamageSource**：抽象基类，定义伤害来源接口（类型判断、护甲穿透、死亡消息键等）
- **EnvironmentalDamage**：环境伤害（火焰、摔落、溺水、虚空等）
- **EntityDamageSource**：直接实体伤害（生物攻击、玩家攻击等）
- **IndirectEntityDamageSource**：间接实体伤害（箭矢、三叉戟、火球等投射物）
- **CombatEntry**：记录单次伤害事件（伤害来源、伤害值、时间戳、受伤前生命值、摔落信息）
- **CombatTracker**：战斗追踪器，管理战斗状态、记录伤害、生成死亡消息

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 依赖项 | 用途 |
|--------|------|
| `core/Types.hpp` | 基础类型定义（i8, u8, f32, std::string 等） |
| `Entity.hpp` | 实体前向声明 |
| `LivingEntity.hpp` | 生物实体前向声明 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| Entity / LivingEntity | 受伤和死亡处理 |
| AI Goal (MeleeAttackGoal) | 创建攻击伤害 |
| AttackContext (entity/combat) | 创建 DamageSource 实例 |
| attribute | 伤害计算涉及攻击伤害属性 |
| LootContext | 掉落物生成需要伤害来源信息 |

## 容易踩的坑

### 1. DamageSource 生命周期

**问题**：`DamageSource` 使用 `std::unique_ptr` 存储，所有权转移后原对象不可用。

**解决**：使用 `clone()` 方法复制伤害来源：
```cpp
// 正确
m_combatTracker.recordDamage(source.clone(), amount, timestamp);

// 错误 - source 可能已经被移动
m_combatTracker.recordDamage(std::move(source), amount, timestamp);
```

### 2. 实体指针失效

**问题**：`EntityDamageSource` 和 `IndirectEntityDamageSource` 存储原始指针，如果实体被移除，指针会失效。

**解决**：在使用伤害来源前检查实体是否存活。`CombatTracker` 会在清理过期条目时自动处理。

### 3. 时间戳溢出

**问题**：时间戳使用 `i32` 存储（tick 单位），约 248 天会溢出。

**解决**：使用差值计算时间，不直接比较时间戳大小：
```cpp
// 正确
i32 elapsed = currentTime - entry.timestamp();

// 错误 - 溢出后比较错误
if (entry.timestamp() < currentTime) { ... }
```

### 4. 战斗超时清理

**问题**：`_cleanupOldEntries()` 在每次 `trackDamage()` 时调用，可能导致性能问题。

**解决**：当前实现已优化，增量更新最佳伤害记录只在清理后重新扫描。

### 5. 死亡消息国际化

**问题**：当前 `getDeathMessage()` 返回硬编码英文字符串。

**解决**：后续需要集成国际化系统，使用 `deathMessageKey()` 获取翻译键。

### 6. DamageType 枚举值

**问题**：`DamageType` 定义了 28+ 种伤害类型，但 `deathMessageKey()` 需要为每种类型返回对应的翻译键。

**解决**：新增伤害类型时务必同步更新 `deathMessageKey()` 方法。

### 7. DamageFlags 标志位

**问题**：保护附魔使用 `DamageFlags` 命名空间中的标志位（FIRE, FALL, EXPLOSION, PROJECTILE）进行伤害减免计算。

**解决**：新增伤害类型时需确认是否应设置对应标志位。
