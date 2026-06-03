# Damage 模块

战斗伤害追踪系统，负责记录和管理实体受到的伤害事件，用于生成死亡消息和统计战斗数据。

## 目录结构

```
src/common/entity/damage/
├── DamageSource.hpp      # 伤害来源定义（核心类型）
├── CombatEntry.hpp       # 战斗条目头文件
├── CombatEntry.cpp       # 战斗条目实现
├── CombatTracker.hpp     # 战斗追踪器头文件
└── CombatTracker.cpp     # 战斗追踪器实现
```

## 文件详解

### DamageSource.hpp

**职责**: 定义伤害来源类型系统，是整个伤害模块的核心类型定义。

**主要内容**:

1. **DamageType 枚举** - 定义 28 种伤害类型：
   - 环境伤害：`InFire`, `OnFire`, `Lava`, `HotFloor`, `Drown`, `Starve`, `Cactus`, `Fall`, `FlyIntoWall`, `OutOfWorld`, `Generic`, `Magic`, `Wither`, `Anvil`, `FallingBlock`, `DragonBreath`, `Fireworks`
   - 实体伤害：`MobAttack`, `PlayerAttack`, `Arrow`, `Trident`, `MobProjectile`, `Fireball`, `Thorns`, `Explosion`, `ExplosionPlayer`

2. **DamageSource 基类** - 抽象基类，定义伤害来源接口：
   - `clone()` - 克隆伤害来源
   - `type()` - 获取伤害类型
   - `source()` / `directSource()` / `getEntity()` - 获取伤害来源实体
   - `bypassesArmor()` - 是否绕过护甲
   - `bypassesInvulnerability()` - 是否绕过无敌
   - `canDamageCreative()` - 是否伤害创造模式
   - `isFire()` / `isProjectile()` / `isMagic()` / `isExplosion()` - 伤害类型判断
   - `deathMessageKey()` - 获取死亡消息键

3. **EnvironmentalDamage** - 环境伤害来源类：
   - 处理火焰、摔落、溺水、虚空等非实体造成的伤害
   - 自动判断是否绕过护甲（摔落、溺水、饥饿、虚空）
   - `isExplosion()` 实现：检查 `DamageType::Explosion` 或 `DamageType::ExplosionPlayer`

4. **EntityDamageSource** - 直接实体伤害来源类：
   - 处理生物攻击、玩家攻击等直接伤害
   - `isExplosion()` 返回 false（非爆炸类型）

5. **IndirectEntityDamageSource** - 间接实体伤害来源类：
   - 处理箭矢、三叉戟、火球等投射物伤害
   - 区分来源实体（射手）和直接来源实体（箭矢）
   - 火球伤害 `isFire()` 返回 true

6. **DamageSources 命名空间** - 工厂函数集合：
   - `inFire()`, `onFire()`, `lava()`, `drown()`, `starve()`, `cactus()`, `fall()`, `flyIntoWall()`, `outOfWorld()`, `generic()`, `magic()`, `wither()`
   - `mobAttack(Entity*)`, `playerAttack(Entity*)`, `arrow(Entity*, Entity*, bool)`, `trident(Entity*, Entity*, bool)`, `thorns(Entity*)`, `explosion(Entity*)`

### CombatEntry.hpp / CombatEntry.cpp

**职责**: 记录单次伤害事件的详细信息。

**主要成员**:
- `m_source` - 伤害来源（`std::unique_ptr<DamageSource>`）
- `m_damage` - 伤害值（`f32`）
- `m_timestamp` - 发生时间，以 tick 为单位（`i32`）
- `m_health` - 受伤前生命值（`f32`）
- `m_fallSuffix` - 摔落后缀（如 "fall", "ladder"）
- `m_fallDistance` - 摔落距离（`f32`）

**主要方法**:
- `source()` - 获取伤害来源
- `damage()` - 获取伤害值
- `timestamp()` - 获取发生时间
- `health()` - 获取受伤前生命值
- `fallSuffix()` - 获取摔落后缀
- `fallDistance()` - 获取摔落距离
- `isLivingSource()` - 是否来自生物
- `isPlayerSource()` - 是否来自玩家
- `getDamageAmount()` - 获取伤害量（虚空伤害返回最大值）

**设计说明**:
- 使用 `std::unique_ptr<DamageSource>` 存储多态伤害来源
- 不可变对象，创建后不可修改
- 作为 `CombatTracker` 的条目存储
- 支持 MC 1.16.5 的摔落后缀系统（用于生成"试图逃离 xxx"死亡消息）

### CombatTracker.hpp / CombatTracker.cpp

**职责**: 战斗追踪器，记录实体的战斗历史，生成死亡消息。

**MC 1.16.5 对齐状态**: ✅ 完整实现战斗状态管理、伤害记录、死亡消息生成

**主要成员**:
- `m_owner` - 拥有此追踪器的生物（`LivingEntity*`）
- `m_entries` - 战斗记录列表（`std::vector<CombatEntry>`）
- `m_totalDamage` - 总承受伤害（`f32`）
- `m_bestEntryIndex` - 最佳伤害记录索引（`size_t`）
- `m_lastDamageTime` - 最后受伤时间（`i32`）
- `m_combatStartTime` - 战斗开始时间（`i32`）
- `m_combatEndTime` - 战斗结束时间（`i32`）
- `m_inCombat` - 是否在战斗中（`bool`）
- `m_takingDamage` - 是否正在受到伤害（`bool`）
- `m_fallSuffix` - 当前摔落后缀（`std::string`）

**主要方法**:
- `trackDamage(source, health, damage)` - 记录伤害事件
- `reset()` - 重置追踪器（通常在重生时调用）
- `getLastEntry()` - 获取最近的伤害记录
- `getBestEntry()` - 获取造成最多伤害的记录
- `getLastAttacker()` - 获取最近的攻击者
- `getBestAttacker()` - 获取造成最多伤害的攻击者
- `getBestAttackerLiving()` - 获取最佳攻击者（LivingEntity 类型）
- `getDeathMessage()` - 生成死亡消息
- `hasCombat()` - 是否有战斗记录
- `getTotalDamage()` - 获取总承受伤害
- `getCombatDuration()` - 获取战斗时长
- `inCombat()` - 检查是否在战斗中
- `combatStartTime()` / `combatEndTime()` - 战斗时间查询
- `_calculateFallSuffix()` - 计算摔落后缀（私有方法）

**关键常量**:
- `COMBAT_TIMEOUT = 100` - 战斗超时时间（100 tick = 5 秒）

**战斗状态管理**:
- 进入战斗：受到实体伤害且当前不在战斗中
- 战斗超时：战斗中 300 tick 无新伤害，非战斗中 100 tick
- 战斗结束：实体死亡或超时
- 自动发送 `sendEnterCombat()` 和 `sendEndCombat()` 通知

**死亡消息生成**:
- 支持摔落+攻击组合消息（"xxx fell from a high place whilst trying to escape yyy"）
- 根据伤害类型生成不同的死亡消息
- 支持环境伤害消息（燃烧、溺水、摔落等）
- 支持实体伤害消息（被击杀）

**摔落后缀系统**:
- `_calculateFallSuffix()` - 根据攀爬方块类型确定摔落后缀
- 后缀类型：
  - `ladder` - 从梯子或打开的活板门摔落
  - `vines` - 从藤蔓摔落
  - `weeping_vines` - 从垂泪藤摔落
  - `twisting_vines` - 从扭曲藤摔落
  - `scaffolding` - 从脚手架摔落
  - `other_climbable` - 从其他可攀爬方块摔落
  - `water` - 在水中摔落
  - 空 - 普通摔落
- 攀爬位置由 `Entity::getLastClimbPos()` 提供
- 落地时自动清空攀爬位置

**清理机制**:
- `_cleanupOldEntries()` - 清理超过战斗超时时间的条目
- `_updateBestEntry()` - 重新计算最佳伤害记录
- `_getBestCombatEntry()` - 优先选择玩家造成的伤害

## 模块关系图

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

## 整体职责

1. **伤害类型定义** - 定义游戏中所有伤害类型及其属性
2. **伤害来源追踪** - 区分环境伤害、直接实体伤害、间接实体伤害
3. **战斗历史记录** - 记录实体受到的所有伤害事件
4. **死亡消息生成** - 根据战斗历史生成合适的死亡消息
5. **战斗状态判断** - 判断实体是否处于战斗中

## 输入和输出

### 输入

- **伤害事件**: `DamageSource`（伤害来源）+ `f32`（伤害值）+ `u32`（时间戳）
- **时间查询**: 当前游戏时间（tick）

### 输出

- **战斗记录**: 最近伤害、最佳伤害、攻击者信息
- **死亡消息**: 根据伤害类型生成的字符串
- **战斗状态**: 是否在战斗中、战斗时长、总承受伤害

## 依赖项

| 依赖项 | 用途 |
|--------|------|
| `core/Types.hpp` | 基础类型定义（`i8`, `u8`, `f32`, `std::string` 等） |
| `Entity.hpp` | 实体前向声明 |
| `LivingEntity.hpp` | 生物实体前向声明 |

## 使用方法

### 1. 创建伤害来源

```cpp
#include "damage/DamageSource.hpp"

// 环境伤害
EnvironmentalDamage fireDamage = DamageSources::inFire();
EnvironmentalDamage fallDamage = DamageSources::fall();

// 实体伤害
EntityDamageSource attack = DamageSources::playerAttack(player);
EntityDamageSource mobAttack = DamageSources::mobAttack(mob);

// 间接实体伤害
IndirectEntityDamageSource arrowDamage = DamageSources::arrow(arrowEntity, shooter, isPlayer);
```

### 2. 应用伤害

```cpp
// 在 LivingEntity 中
void LivingEntity::hurt(DamageSource& source, f32 amount) {
    // 记录伤害到战斗追踪器
    m_combatTracker.recordDamage(source.clone(), amount, ticksExisted());
}
```

### 3. 生成死亡消息

```cpp
// 实体死亡时
void LivingEntity::die(DamageSource& cause) {
    std::string message = m_combatTracker.getDeathMessage();
    // 广播死亡消息到聊天
}
```

### 4. 检查战斗状态

```cpp
// 判断实体是否在战斗中
bool inCombat = m_combatTracker.isInCombat(currentTick);

// 获取最近的攻击者
Entity* attacker = m_combatTracker.getLastAttacker();

// 获取造成最多伤害的攻击者
Entity* bestAttacker = m_combatTracker.getBestAttacker();
```

### 5. 重置追踪器

```cpp
// 玩家重生时
void Player::respawn() {
    m_combatTracker.reset();
}
```

## 容易踩的坑

### 1. DamageSource 生命周期

**问题**: `DamageSource` 使用 `std::unique_ptr` 存储，所有权转移后原对象不可用。

**解决**: 使用 `clone()` 方法复制伤害来源：
```cpp
// 正确
m_combatTracker.recordDamage(source.clone(), amount, timestamp);

// 错误 - source 可能已经被移动
m_combatTracker.recordDamage(std::move(source), amount, timestamp);
```

### 2. 实体指针失效

**问题**: `EntityDamageSource` 和 `IndirectEntityDamageSource` 存储原始指针，如果实体被移除，指针会失效。

**解决**:
- 在使用伤害来源前检查实体是否存活
- `CombatTracker` 会在清理过期条目时自动处理

### 3. 时间戳溢出

**问题**: 时间戳使用 `u32` 存储，约 248 天会溢出。

**解决**: 使用差值计算时间，不直接比较时间戳大小：
```cpp
// 正确
u32 elapsed = currentTime - entry.timestamp();

// 错误 - 溢出后比较错误
if (entry.timestamp() < currentTime) { ... }
```

### 4. 战斗超时清理

**问题**: `_cleanupOldEntries()` 会在每次 `trackDamage()` 时调用，可能导致性能问题。

**解决**: 当前实现已经优化，增量更新最佳伤害记录只在清理后重新扫描。

### 5. 死亡消息国际化

**问题**: 当前 `getDeathMessage()` 返回硬编码英文字符串。

**解决**: 后续需要集成国际化系统，使用 `deathMessageKey()` 获取翻译键。

## 涉及的测试用例

测试文件位于 `tests/entity/LivingEntityTests.cpp`，包含以下伤害相关测试：

| 测试用例 | 描述 |
|----------|------|
| `DamageSourceTest.EnvironmentalDamage` | 测试环境伤害来源的属性判断 |
| `DamageSourceTest.EntityDamage` | 测试实体伤害来源的属性判断 |
| `DamageSourceTest.IndirectEntityDamage` | 测试间接实体伤害来源的属性判断 |
| `DamageSourceTest.DamageTypes` | 测试各种伤害类型判断（摔落、溺水、饥饿） |
| `DamageSourceTest.BypassesArmor` | 测试护甲穿透逻辑 |
| `DamageSourceTest.DeathMessageKeys` | 测试死亡消息键获取 |
| `DamageSourceTest.DamageSourcesFactory` | 测试工厂函数 |
| `LivingEntityTest.Hurt` | 测试受伤逻辑 |
| `LivingEntityTest.HurtInvulnerability` | 测试受伤无敌帧 |
| `LivingEntityTest.Death` | 测试死亡逻辑 |

新增测试文件 `tests/common/entity/damage/DamageSourceExplosionTest.cpp`：

| 测试用例 | 描述 |
|----------|------|
| `DamageSourceExplosionTest.EnvironmentalDamage_ExplosionType_ReturnsTrue` | 测试 Explosion/ExplosionPlayer 类型的 isExplosion() 返回 true |
| `DamageSourceExplosionTest.EnvironmentalDamage_NonExplosionType_ReturnsFalse` | 测试非爆炸类型的 isExplosion() 返回 false |
| `DamageSourceExplosionTest.EnvironmentalDamage_FireType_ReturnsCorrectBool` | 测试火焰类型的 isFire() 返回值 |
| `DamageSourceExplosionTest.DamageSources_Explosion_CreatesExplosionDamage` | 测试工厂函数创建爆炸伤害 |
| `DamageSourceExplosionTest.CombinedCheck_FireAndExplosion` | 测试火焰+爆炸组合检测（用于矿车逻辑） |
| `DamageSourceExplosionTest.NullptrSource_HandledSafely` | 测试 nullptr 伤害源的安全处理 |
| `DamageSourceExplosionTest.AllDamageTypes_HaveCorrectClassification` | 测试所有伤害类型的 isFire/isExplosion 分类 |

## 与其他模块的关系

### 上游模块

- **Entity / LivingEntity**: 使用伤害系统进行受伤和死亡处理
- **AI Goal (MeleeAttackGoal)**: 使用 `EntityDamageSource` 创建攻击伤害

### 下游模块

- **CombatTracker**: 依赖 `DamageSource` 和 `CombatEntry`
- **AttackContext (entity/combat)**: 创建 `DamageSource` 实例

### 协作模块

- **attribute**: 伤害计算涉及攻击伤害属性
- **LootContext**: 掉落物生成需要伤害来源信息

## 参考

本模块参考 MC 1.16.5 的 `DamageSource`、`CombatEntry`、`CombatTracker` 实现：
- `net.minecraft.util.DamageSource`
- `net.minecraft.util.CombatEntry`
- `net.minecraft.util.CombatTracker`
- `net.minecraft.entity.LivingEntity` 中的战斗追踪逻辑
