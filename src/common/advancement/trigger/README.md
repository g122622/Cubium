# 触发器系统 (Trigger System)

## 目录结构树

```
trigger/
├── CriterionTrigger.hpp           # 触发器接口和基类（ICriterionTriggerBase、ICriterionTrigger、AbstractCriterionTrigger）
├── CriterionTrigger.cpp           # 触发器基类实现
├── CriterionTriggers.hpp          # 触发器注册表（单例，管理所有触发器实例，registerBuiltinTriggers 在服务器启动时调用）
├── CriterionTriggers.cpp          # 注册表实现，包含内置触发器注册
│
├── conditions/                    # 条件谓词（用于匹配特定游戏状态）
│   ├── ItemPredicate.hpp/cpp      # 物品匹配（ID、数量、耐久、药水、附魔、NBT）
│   ├── EntityPredicate.hpp/cpp    # 实体匹配（类型、位置、效果、NBT、装备、标志）+ DamageSourcePredicate
│   ├── EntityFlagsPredicate.hpp/cpp   # 实体标志匹配（燃烧、潜行、疾跑、游泳、幼年）
│   ├── EntityEquipmentPredicate.hpp/cpp   # 装备匹配（头盔、胸甲、护腿、靴子、主手、副手）
│   ├── NBTPredicate.hpp/cpp       # NBT 数据匹配（递归比较，期望标签是实际标签的子集）
│   ├── LocationPredicate.hpp/cpp  # 位置匹配（坐标、维度、生物群系、结构、流体）
│   ├── BlockPredicate.hpp/cpp     # 方块匹配 + FluidPredicate（流体匹配）
│   └── MobEffectsPredicate.hpp/cpp    # 效果匹配（效果类型、等级、持续时间、环境标志）
│
└── impl/                          # 触发器实现
    ├── ImpossibleTrigger.hpp      # 不可能触发器（只能手动授予）
    ├── TickTrigger.hpp/cpp        # Tick 触发器（每 tick 触发，由 AdvancementEventHandler 订阅 ServerTickEvent 驱动）
    ├── InventoryChangedTrigger.hpp/cpp  # 物品栏变化触发器
    ├── LocationTrigger.hpp/cpp          # 位置触发器（维度、生物群系检测）
    ├── PlayerKilledEntityTrigger.hpp/cpp # 玩家击杀实体触发器
    ├── BlockTriggers.hpp/cpp           # 方块触发器（放置、进入、滑落、蜂巢破坏、目标击中）
    ├── ItemTriggers.hpp/cpp            # 物品触发器（消耗、耐久变化、附魔、装桶）
    ├── EntityTriggers.hpp/cpp          # 实体触发器（驯服、繁殖、交易、治愈、召唤、互动）
    ├── EffectTriggers.hpp/cpp          # 效果触发器（效果变化、酿造药水）
    ├── ChanneledLightningTrigger.hpp/cpp # 引雷附魔触发器
    └── AvoidVibrationTrigger.hpp/cpp   # 避免振动触发器（潜行避开幽匿感测体振动时触发）
```

## 内部模块关系

```
CriterionTriggers（注册表，服务器启动时调用 registerBuiltinTriggers）
    │ 管理
    ▼
ICriterionTriggerBase（类型擦除接口，供 PlayerAdvancements 使用）
    │  提供 addListenerForCriterion / removeListenerForCriterion / removeAllListenersForPlayer
    │ 派生
    ▼
ICriterionTrigger<T>（带类型参数的触发器接口）
    │ 派生
    ▼
AbstractCriterionTrigger<T>（监听器管理基类）
    │ 实现类型擦除方法（内部使用 std::dynamic_pointer_cast<T> 转型）
    │ 派生
    ▼
具体触发器实现（InventoryChangedTrigger、PlayerKilledEntityTrigger 等）
    │ 使用
    ▼
条件谓词（ItemPredicate、EntityPredicate、BlockPredicate 等）
```

## 上下游外部依赖关系

### 本模块依赖的外部模块

- `common/core/ResourceLocation.hpp` - 资源位置（成就 ID、触发器 ID）
- `common/util/nbt/` - NBT 系统（NBTPredicate）
- `common/entity/` - 实体系统（EntityPredicate）
- `common/entity/effect/` - 效果系统（MobEffectsPredicate）
- `common/entity/damage/` - 伤害系统（DamageSourcePredicate）
- `common/world/` - 世界接口（LocationPredicate、BlockPredicate）
- `common/world/biome/` - 生物群系注册表（LocationPredicate）
- `common/item/` - 物品系统（ItemPredicate）
- `common/item/loot/StatePropertiesPredicate.hpp` - 状态属性谓词（BlockPredicate 复用）
- `nlohmann/json.hpp` - JSON 解析

### 依赖本模块的外部模块

- `server/advancement/` - 服务端成就系统
  - `PlayerAdvancements` - 通过 `ICriterionTriggerBase` 类型擦除接口注册/注销监听器
  - `AdvancementEventHandler` - 订阅 ServerTickEvent 驱动 TickTrigger
  - `TriggerInstantiation` - 触发器实例化工具
- `server/application/MinecraftServer.cpp` - 启动时调用 `registerBuiltinTriggers()`
- `common/network/ir/packets/play/PlayPacketsExtended.hpp` - 成就 IR 网络包（`SelectAdvancementTab`/`SeenAdvancements`；`UpdateAdvancements` 完整进度树同步尚未实现，旧 `common/network/packet/AdvancementPackets.hpp` 已删除）
- `common/item/loot/conditions/` - 战利品条件复用谓词（EntityPropertiesCondition、LocationCheckCondition、DamageSourcePropertiesCondition）

## 容易踩的坑

1. **触发器注册**：所有触发器必须在 `CriterionTriggers::registerBuiltinTriggers()` 中注册，否则无法从 JSON 加载。`registerBuiltinTriggers()` 在服务器启动时自动调用。新增触发器后记得在此方法中添加注册代码。

2. **谓词为空语义**：大多数谓词的默认构造（空状态）表示"匹配任意"，而非"不匹配"。检查谓词时需注意 `isAny()` 的语义。

3. **实体类型检查**：`EntityEquipmentPredicate` 和 `MobEffectsPredicate` 只对 `LivingEntity` 有效，非 `LivingEntity` 对这些谓词返回 `false`（除非谓词为空）。

4. **伤害源标志**：`EnvironmentalDamage` 的 `isProjectile()` 和 `isExplosion()` 始终返回 `false`，投射物和爆炸伤害需使用 `EntityDamageSource` 或 `IndirectEntityDamageSource`。

5. **维度名称**：维度检查使用 `ResourceLocation` 的路径部分（如 `overworld`），不是完整字符串。

6. **效果类型解析**：使用 `getEffectByResourceLocation()` 解析效果类型，未知效果会被跳过并输出警告日志。

7. **流体等效性**：`FluidPredicate` 使用 `Fluid::isEquivalentTo()` 比较，`minecraft:water` 同时匹配水源和流动水。

8. **状态属性复用**：`BlockPredicate` 复用 `mc::StatePropertiesPredicate`（位于 `common/entity/loot/`），而非重新实现。

9. **触发器模板模式**：创建新触发器需继承 `AbstractCriterionTrigger<T>`，其中 `T` 是触发器实例类型。`CriterionInstance<T>` 提供条件检测的 `test()` 方法。

10. **服务端触发路径**：服务端触发成就需通过 `AdvancementEventHandler` 订阅事件，然后调用 `trigger->trigger(*advancements, predicate)`。直接调用触发器不会生效，因为没有监听器上下文。特殊例外：`AvoidVibrationTrigger` 在 `VibrationSystemServer.cpp` 的 `isValidVibration()` 中直接调用基类模板方法 `AbstractCriterionTrigger<AvoidVibrationTriggerInstance>::trigger(*advancements, predicate)`，因为其触发时机不在事件系统中，而是在振动验证逻辑中。

11. **命名空间注意**：`mc::advancement::PlayerAdvancements` 是前向声明，实际定义在 `mc::server::PlayerAdvancements`。触发器接口使用 `mc::server::PlayerAdvancements&` 作为参数类型。

12. **触发器 ID 常量**：所有触发器 ID 常量定义在 `mc::advancement::triggers` 命名空间中（如 `triggers::INVENTORY_CHANGED`），新增触发器时需同步添加常量。

13. **类型擦除接口与模板接口的选择**：`PlayerAdvancements` 在注册/注销监听器时使用 `ICriterionTriggerBase` 的类型擦除方法（`addListenerForCriterion` 等），因为此时只知道 `ICriterionInstance` 基类指针；而 `AdvancementEventHandler` 触发检测时使用 `ICriterionTrigger<T>` 的模板方法（`trigger`、`hasListeners` 等），因为触发器类型已知。不要混用两条路径。

14. **dynamic_pointer_cast 失败**：`addListenerForCriterion()` 内部使用 `std::dynamic_pointer_cast<T>` 将 `ICriterionInstance` 向下转型。如果触发器 ID 与实例类型不匹配（例如注册了错误的实例类型），转型会失败并输出警告日志但不会崩溃。确保 `fromJson()` 返回的实例类型与触发器模板参数 `T` 一致。
