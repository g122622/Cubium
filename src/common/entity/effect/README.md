# 状态效果系统 (Effect System)

## 目录结构

```
effect/
├── EffectType.hpp/cpp           # 效果类型枚举和工具函数（ID转换、资源位置映射）
├── EffectInstance.hpp/cpp       # 效果实例（持续时间、等级、tick逻辑）
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
- `WitchEntity`、`PufferfishEntity`、`DolphinGoals` 等实体 AI

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
瞬间治疗、瞬间伤害、饱和效果在 `tick()` 中处理，MC 原版在添加时立即执行。目前实现与 MC 有差异，需要后续对齐。

### 4. 属性修改器的 UUID
效果使用固定 UUID 来标识属性修改器，在 `apply()` 时添加，`remove()` 时移除。同一个效果不能重复叠加属性修改器。

### 5. 效果合并规则
当添加同名效果时，根据持续时间和等级决定合并策略：
- 新效果等级更高 → 覆盖
- 等级相同且新效果持续时间更长 → 合并持续时间
- 否则 → 忽略

### 6. 新增效果类型注意
MC 1.21 新增了 TrialOmen、WindCharged、RaidOmen 三个试炼密室效果，数值 ID 为 33-35。如需新增效果，需同时更新：`EffectType` 枚举、`getEffectById()`、`getEffectResourceLocation()`、`getEffectResourceName()`。

### 7. 效果 tick 间隔
部分效果有特殊的 tick 间隔计算：
- 生命恢复：每 `50/(level+1)` tick 治疗 1 HP
- 中毒：每 `25/(level+1)` tick 造成 1 HP 伤害
- 凋零：每 `40/(level+1)` tick 造成 1 HP 伤害
- 饥饿：每 tick 增加 `exhaustion += 0.005 * (level+1)`
