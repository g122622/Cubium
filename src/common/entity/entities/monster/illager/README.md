# 灾厄村民模块 (Illager)

灾厄村民是 Minecraft 中的敌对生物类别，包括掠夺者、卫道士、唤魔者、幻术师、恼鬼、女巫和劫掠兽等。

## 目录结构

```
illager/
├── AbstractIllagerEntity.hpp/cpp   # 灾厄村民基类（手臂姿势、RAID参与状态）
├── AbstractRaiderEntity.hpp/cpp     # 袭击者基类（袭击状态、庆祝状态）
├── PatrollerEntity.hpp/cpp          # 巡逻者基类（巡逻目标、队长标记）
├── SpellcastingIllagerEntity.hpp/cpp # 施法灾厄村民基类（法术类型、施法tick）
├── EvokerEntity.hpp/cpp             # 唤魔者（尖牙攻击、召唤恼鬼）
├── IllusionerEntity.hpp/cpp         # 幻术师（失明法术、镜像隐身）
├── IllagerEntities.hpp/cpp          # 掠夺者、卫道士
├── RavagerEntity.hpp/cpp            # 劫掠兽（冲撞、咆哮、破坏树叶）
├── VexEntity.hpp/cpp                # 恼鬼（穿墙飞行、有限生命）
├── WitchEntity.hpp/cpp              # 女巫（药水攻击、喝药水治疗）
└── README.md
```

## 继承层次

```
MonsterEntity (敌对生物基类)
└── PatrollerEntity (巡逻者基类)
    └── AbstractRaiderEntity (袭击者基类)
        ├── AbstractIllagerEntity (灾厄村民基类)
        │   ├── SpellcastingIllagerEntity (施法灾厄村民基类)
        │   │   ├── EvokerEntity (唤魔者)
        │   │   └── IllusionerEntity (幻术师)
        │   ├── VindicatorEntity (卫道士) - 近战斧头攻击
        │   └── PillagerEntity (掠夺者) - 弩远程攻击，实现 ICrossbowUser
        ├── WitchEntity (女巫) - 药水攻击，实现 IRangedAttackMob
        └── RavagerEntity (劫掠兽) - 大型近战，破坏树叶

VexEntity (恼鬼) 独立继承自 MonsterEntity - 穿墙飞行
```

## 内部模块关系

- **PatrollerEntity** 提供巡逻基础设施（巡逻目标、队长标记），被所有袭击者继承
- **AbstractRaiderEntity** 扩展巡逻者，添加袭击参与（Raid关联、波次、庆祝状态）
- **AbstractIllagerEntity** 扩展袭击者，添加手臂姿势状态（用于客户端渲染）
- **SpellcastingIllagerEntity** 扩展灾厄村民，添加施法系统（法术类型、施法tick、粒子颜色）
- **EvokerEntity** / **IllusionerEntity** 继承施法基类，各自实现特定法术
- **PillagerEntity** / **IllusionerEntity** / **WitchEntity** 实现远程攻击接口，但方式不同（弩/弓/药水）
- **VexEntity** 独立实现穿墙飞行，由 EvokerEntity 召唤

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 依赖模块 | 用途 |
|---------|------|
| `entity/core` | EntityId, IWorld, LivingEntity, MonsterEntity 基类 |
| `entity/interfaces/ICrossbowUser` | 弩使用者接口（PillagerEntity） |
| `entity/interfaces/IRangedAttackMob` | 远程攻击接口（IllusionerEntity, WitchEntity） |
| `entity/effect` | 药水效果系统（EffectType, EffectInstance） |
| `entity/damage` | DamageSource 伤害系统 |
| `world/village/raid` | Raid 袭击系统（AbstractRaiderEntity 参与） |
| `world/block` | BlockPos, BlockState, BlockTags（RavagerEntity 破坏树叶） |
| `core/Types` | 基础类型定义 |

### 下游依赖（依赖本模块）

| 依赖方 | 用途 |
|-------|------|
| `entity/registry` | 实体类型注册（EntityTypeRegistry） |
| `world/spawn` | 生物生成系统（灾厄村民生成规则） |
| `world/village/raid` | 袭击系统（灾厄村民参与袭击） |
| `client/renderer/entity` | 实体渲染器（灾厄村民模型渲染、手臂姿势） |
| `server/network` | 实体同步（灾厄村民状态同步到客户端） |

## 容易踩的坑

### 继承链与接口

1. **VexEntity 不继承 AbstractIllagerEntity**：恼鬼独立继承自 MonsterEntity，不能作为灾厄村民处理
2. **ICrossbowUser vs IRangedAttackMob**：PillagerEntity 实现弩接口，IllusionerEntity/WitchEntity 实现远程攻击接口，两者方法签名不同
3. **WitchEntity 不继承 AbstractIllagerEntity**：女巫继承 AbstractRaiderEntity，不是灾厄村民的子类

### 施法系统

4. **施法粒子颜色**：通过 `getSpellParticleColor()` 获取，粒子类型为 `EntityEffect`，颜色通过速度参数传递
5. **施法 tick 管理**：`SpellcastingIllagerEntity` 的 `m_spellTicks` 在 `tick()` 中自动递减，子类只需设置初始值

### 恼鬼特殊行为

6. **穿墙实现**：VexEntity 在 `tick()` 中先 `setNoClip(true)`，调用父类 tick，再 `setNoClip(false)`
7. **有限生命**：恼鬼默认 `m_lifeTime = 2400`（约2分钟），生命结束时使用饥饿伤害（`DamageSources::starve()`）
8. **主人引用**：恼鬼通过 `m_owner` 指针持有唤魔者引用，需注意悬空指针风险

### 劫掠兽

9. **自定义寻路**：RavagerEntity 使用 `RavagerNodeProcessor`，将树叶视为开放区域，可穿过树叶
10. **攻击状态机**：攻击 -> 眩晕(50%概率) -> 咆哮，三个状态通过 tick 计时器管理
11. **咆哮伤害**：咆哮对非掠夺者类实体造成伤害，掠夺者类免疫但会被击退

### 女巫药水逻辑

12. **喝药水时机**：`tick()` 中自动检测是否需要喝药水，喝药水期间 `canRangedAttack()` 返回 false
13. **魔法伤害减免**：女巫对魔法伤害有 85% 减免，免疫自己造成的伤害
14. **攻击药水选择**：根据目标状态选择药水类型，需要正确判断目标是否为掠夺者同伴

### 袭击系统

15. **Raid 指针**：`AbstractRaiderEntity` 持有 `Raid*` 指针，参与袭击时设置，离开时清空
16. **死亡通知**：`die()` 中必须先通知 Raid 再调用父类 die()，顺序很重要
17. **庆祝状态**：庆祝状态通过 `RaiderState::Celebrating` 和 `m_celebrationTime` 管理

## 相关文档

- [敌对生物模块](../README.md)
- [实体系统](../../README.md)
- [实体核心](../../../core/README.md)
