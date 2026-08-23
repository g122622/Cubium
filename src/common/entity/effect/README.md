# 状态效果系统 (Effect System)

## 目录结构

```
effect/
├── EffectType.hpp/cpp           # 效果类型枚举和工具函数（ID转换、资源位置映射）
├── EffectInstance.hpp/cpp       # 效果实例（持续时间、等级、tick逻辑、到期查询）
├── EffectManager.hpp/cpp        # 效果管理器（实体身上的效果集合）
├── EffectAttributeModifiers.hpp/cpp # 效果属性修改器定义（UUID、修改量计算）
└── README.md                    # 本文档
```

## 内部模块关系

```
EffectType ←── EffectInstance ←── EffectManager
     ↑              ↑                  ↑
     └──────────────┴──────────────────┘
                    │
        EffectAttributeModifiers
```

- **EffectType**：底层枚举和工具函数，被所有其他模块依赖
- **EffectInstance**：表示单个效果实例，依赖 EffectType，持有效果的具体数据
- **EffectManager**：管理实体的所有效果实例，依赖 EffectInstance
- **EffectAttributeModifiers**：定义效果对属性的修改，被 EffectInstance 的 `apply()`/`remove()` 调用

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `mc::nbt::tags::compound_tag` - NBT 序列化
- `mc::entity::attribute` - 属性系统（Attribute、AttributeModifier、Attributes）
- `mc::resource::ResourceLocation` - 资源位置
- `mc::LivingEntity` - 生物实体（前向声明）

**下游依赖（被谁依赖）：**
- `LivingEntity` - 生物实体基类，持有 EffectManager
- `PotionItem`、`PotionUtils`、`PotionRegistry` - 药水系统
- `FoodItem`、`FoodStats` - 食物系统（饥饿效果、食物效果）
- `BeaconEntity`、`ConduitEntity` - 方块实体（信标、潮涌核心效果）
- `EffectCommand`、`EffectResolver` - 命令系统
- `MobEffectsPredicate` - 进度系统（效果条件判定）
- `PlayerSaveData`、`ServerPlayerData` - 玩家数据存储
- `WitchEntity`、`PufferfishEntity`、`DolphinGoals`、`AxolotlEntity` 等实体 AI

## 容易踩的坑

### 1. amplifier 与等级显示的区别
- `amplifier` 是 **0-based**（0 = I级，1 = II级）
- `getEffectLevel()` 是 **1-based**（1 = I级，2 = II级）
- 属性修改量计算使用 `amplifier + 1`

### 2. 持续时间的特殊值
- `duration > 0`：正常倒计时
- `duration == 0`：已过期
- `duration < 0`：永久效果（如信标效果）

### 3. 瞬间效果的 tick 处理
瞬间治疗、瞬间伤害、饱和效果在 `EffectManager::addEffect()` 中立即执行效果逻辑后丢弃，不保存到效果列表。这与 MC 原版行为一致：`InstantenousMobEffect.shouldApplyEffectTickThisTick()` 在 duration >= 1 时返回 true，效果在第一个 tick 触发一次后过期。

饱和效果（仅对玩家有效）通过 `Player::foodStats().addStats()` 恢复饥饿值和饱和度，公式为 `addStats(amplifier + 1, 1.0)`，即每级增加 1 点饥饿值和 2 点饱和度。

### 4. 属性修改器的 UUID
效果使用固定 UUID 来标识属性修改器，在 `apply()` 时添加，`remove()` 时移除。同一个效果不能重复叠加属性修改器。

### 5. 效果合并规则
当添加同名效果时，根据持续时间和等级决定合并策略：
- 新效果等级更高 → 覆盖（移除旧属性修改器，应用新属性修改器）
- 等级相同且新效果持续时间更长 → 合并持续时间（属性修改器不变）
- 否则 → 忽略（不修改现有效果和属性修改器）

注意：`EffectManager::addEffect()` 在合并时，只有当新效果的 amplifier 严格大于现有效果时，
才会先调用 `remove()` 移除旧属性修改器，再调用 `apply()` 应用新属性修改器。
同级延长时间不会重新应用属性修改器，更弱的效果会被完全忽略。

### 6. 属性修改器映射
效果到属性修改器的映射定义在 `EffectAttributeModifiers` 中：

| 效果 | 属性 | 修改量/级 | 操作类型 | UUID |
|------|------|-----------|----------|------|
| Speed | MOVEMENT_SPEED | +0.2 | MultiplyTotal | SPEED_UUID |
| Slowness | MOVEMENT_SPEED | -0.15 | MultiplyTotal | SLOWNESS_UUID |
| Haste | ATTACK_SPEED | +0.1 | MultiplyTotal | HASTE_UUID |
| MiningFatigue | ATTACK_SPEED | -0.1 | MultiplyTotal | MINING_FATIGUE_UUID |
| Strength | ATTACK_DAMAGE | +3.0 | Addition | STRENGTH_UUID |
| Weakness | ATTACK_DAMAGE | -4.0 | Addition | WEAKNESS_UUID |
| JumpBoost | SAFE_FALL_DISTANCE | +1.0 | Addition | JUMP_BOOST_SAFE_FALL_UUID |
| HealthBoost | MAX_HEALTH | +4.0 | Addition | HEALTH_BOOST_UUID |
| Absorption | MAX_ABSORPTION | +4.0 | Addition | ABSORPTION_UUID |
| Luck | LUCK | +1.0 | Addition | LUCK_UUID |
| BadLuck | LUCK | -1.0 | Addition | BAD_LUCK_UUID |

修改器名称格式：`effect.minecraft.<resource_name>.<level>`（MC 原版格式）

### 7. 新增效果类型注意
MC 1.21 新增了 TrialOmen、WindCharged、RaidOmen 三个试炼密室效果，数值 ID 为 33-35。如需新增效果，需同时更新：`EffectType` 枚举、`getEffectById()`、`getEffectResourceLocation()`、`getEffectResourceName()`。如果新效果有属性修改器，还需在 `EffectAttributeModifiers` 中添加映射条目，并在 `AttributeModifierUUIDs.hpp` 中定义对应 UUID 常量。

### 8. 效果 tick 间隔
部分效果有特殊的 tick 间隔计算：
- 生命恢复：每 `50/(level+1)` tick 治疗 1 HP
- 中毒：每 `25/(level+1)` tick 造成 1 HP 伤害
- 凋零：每 `40/(level+1)` tick 造成 1 HP 伤害
- 饥饿：每 tick 增加 `exhaustion += 0.005 * (level+1)`

### 9. endsWithin() 的语义陷阱
`EffectInstance::endsWithin(maxDuration)` 在以下情况返回 **false**：
- 永久效果（`duration < 0`）：即使 maxDuration 很大也返回 false，永久效果不应被"刷新"
- 已过期效果（`duration == 0`）：返回 true（0 <= maxDuration），但通常此时效果已被移除

典型用途：美西螈的 `applySupportingEffects()` 使用 `endsWithin(2399)` 判断玩家现有的再生效果是否需要刷新——如果现有再生效果剩余时间超过 2399 tick（即已达上限 2400），则不刷新。
