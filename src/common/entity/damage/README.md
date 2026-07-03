# Damage 模块

战斗伤害追踪系统，负责记录和管理实体受到的伤害事件，用于生成死亡消息和统计战斗数据。
同时包含伤害类型标签（DamageTypeTags）系统，用于将伤害类型分组以支持狼铠吸收判定、伤害分类等。

## 目录结构

```
src/common/entity/damage/
├── DamageSource.hpp      # 伤害来源定义（核心类型，包含 DamageType 枚举和三类伤害来源类）
├── CombatEntry.hpp       # 战斗条目（记录单次伤害事件）
├── CombatEntry.cpp       # 战斗条目实现
├── CombatTracker.hpp     # 战斗追踪器（记录战斗历史，生成死亡消息）
├── CombatTracker.cpp     # 战斗追踪器实现
└── tag/                  # 伤害类型标签系统
    ├── DamageTypeTag.hpp       # 伤害类型标签（包含 DamageType 集合）
    ├── DamageTypeTag.cpp       # 伤害类型标签实现 + DamageTypeNames 映射 + DamageSource::is()
    ├── DamageTypeTags.hpp      # 内置伤害类型标签集合（34 个 MC 1.21.11 标签）
    ├── DamageTypeTags.cpp      # 标签注册表 + 硬编码默认值
    ├── DamageTypeTagLoader.hpp # 数据包 JSON 加载器（两阶段加载）
    └── DamageTypeTagLoader.cpp # 加载器实现
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
- **EntityDamageSource**：直接实体伤害（生物攻击、玩家攻击、实体爆炸等）
- **IndirectEntityDamageSource**：间接实体伤害（箭矢、三叉戟、火球、间接爆炸等）
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

**问题**：`DamageType` 定义了 30+ 种伤害类型，但 `deathMessageKey()` 需要为每种类型返回对应的翻译键。

**解决**：新增伤害类型时务必同步更新 `deathMessageKey()` 方法。

**特别说明**：新增的伤害类型及其语义：
- `Stalagmite`：石笋摔落伤害（踩在朝上的滴石尖端上），属于环境伤害，`isFall()` 返回 `true`（受摔落保护附魔减免），`bypassesArmor()` 返回 `true`（无视护甲）
- `FallingStalactite`：坠落钟乳石伤害（钟乳石从上方掉落砸中实体），属于实体伤害，由 `FallingBlockEntity` 使用
- `Freeze`：冰冻伤害（细雪冰冻），属于环境伤害，`isFreezing()` 返回 `true`，`bypassesArmor()` 返回 `true`（无视护甲），受 `FREEZE_DAMAGE` 游戏规则控制，对 `FREEZE_HURTS_EXTRA_TYPES` 标签中的实体（烈焰人、岩浆怪、炽足兽）造成5倍伤害

### 7. DamageFlags 标志位

**问题**：保护附魔使用 `DamageFlags` 命名空间中的标志位（FIRE, FALL, EXPLOSION, PROJECTILE）进行伤害减免计算。

**解决**：新增伤害类型时需确认是否应设置对应标志位。

## 伤害类型标签系统（DamageTypeTags）

对应 MC 1.21.11 的 `net.minecraft.tags.DamageTypeTags`，提供 34 个内置伤害类型标签，用于狼铠吸收判定、伤害分类、AI 行为等。

### 核心组件

- **DamageTypeTag**：标签类，内部使用 `std::unordered_set<DamageType>` 存储成员（基于枚举，类型安全且性能高）
- **DamageTypeNames**：DamageType 枚举与 MC 资源位置名（如 `"minecraft:drown"`）的双向映射，供数据包加载使用
- **DamageTypeTags**：标签注册表，提供 34 个静态访问器（如 `BYPASSES_WOLF_ARMOR()`）
- **DamageTypeTagLoader**：两阶段 JSON 加载器，路径 `data/<namespace>/tags/damage_type/`

### 用法示例

```cpp
// 判断伤害源是否绕过狼铠（核心消费点：WolfEntity::_canArmorAbsorb）
if (source.is(DamageTypeTags::BYPASSES_WOLF_ARMOR())) {
    // 该伤害绕过狼铠吸收
}

// 判断伤害源是否为火焰伤害
if (source.is(DamageTypeTags::IS_FIRE())) {
    // 应用火焰相关逻辑
}

// 直接检查标签是否包含某伤害类型
if (DamageTypeTags::BYPASSES_ARMOR().contains(DamageType::Fall)) {
    // 摔落伤害绕过护甲
}
```

### 关键标签说明

| 标签 | 用途 |
|------|------|
| `BYPASSES_WOLF_ARMOR` | 狼铠吸收判定（WolfEntity::_canArmorAbsorb） |
| `BYPASSES_ARMOR` | 护甲减免判定 |
| `BYPASSES_INVULNERABILITY` | 无敌穿透（虚空、/kill） |
| `BYPASSES_SHIELD` | 盾牌格挡判定 |
| `IS_FIRE` / `IS_FALL` / `IS_EXPLOSION` / `IS_FREEZING` / `IS_DROWNING` / `IS_LIGHTNING` / `IS_PROJECTILE` / `IS_PLAYER_ATTACK` | 伤害分类 |
| `PANIC_CAUSES` / `PANIC_ENVIRONMENTAL_CAUSES` | 生物恐慌触发 |
| `WITCH_RESISTANT_TO` / `WITHER_IMMUNE_TO` | 特殊生物抗性 |
| `DAMAGES_HELMET` | 头盔损坏判定 |

### 初始化流程

1. `DamageTypeTags::initialize()` — 注册 34 个内置标签并填充硬编码默认值（与 MC 1.21.11 Vanilla 数据包一致）
2. `DamageTypeTagLoader::loadFromDataPackRepository()` — 从数据包加载自定义标签，追加或替换默认值

服务端在 `MinecraftServer::initializeRegistries()` 中调用，客户端在 `ClientApplication::initializeCoreRegistries()` 中调用。

### 新增伤害类型时的注意事项

1. 在 `DamageSource.hpp` 的 `DamageType` 枚举中添加新值
2. 在 `DamageTypeTag.cpp` 的 `DamageTypeNames::kEntries` 数组中添加资源位置映射
3. 在 `EnvironmentalDamage::deathMessageKey()` 或 `EntityDamageSource::deathMessageKey()` 中添加对应翻译键
4. 确认 `bypassesArmor()`、`isFire()`、`isMagic()`、`isExplosion()` 等方法是否需要更新
5. 在 `DamageTypeTags::initialize()` 中将新类型添加到相关标签
