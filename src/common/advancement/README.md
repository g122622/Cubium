# 成就系统 (Advancement System)

## 目录结构树

```
advancement/
├── Advancement.hpp / cpp            # 成就定义（不可变对象，Builder 模式构建）
├── AdvancementDisplay.hpp / cpp     # 显示信息（图标、标题、描述、框架类型）
├── AdvancementFrame.hpp             # 框架类型枚举（Task / Challenge / Goal）
├── AdvancementList.hpp / cpp        # 成就列表管理（父子关系维护）
├── AdvancementLoader.hpp / cpp      # JSON 加载器（从数据包或目录加载成就）
├── AdvancementManager.hpp / cpp     # 成就注册表（单例）
├── AdvancementProgress.hpp / cpp    # 进度追踪（条件完成状态、序列化）
├── AdvancementRewards.hpp / cpp     # 奖励定义（经验、战利品、功能、配方）
├── AdvancementVisibilityEvaluator.hpp / cpp  # 成就可见性评估器（MC 原版递归算法）
├── Criterion.hpp / cpp              # 条件定义（触发器实例引用）
├── MinMaxBounds.hpp                 # 范围谓词（IntBounds、DoubleBounds 等）
├── README.md                        # 本文件
│
└── trigger/                         # 触发器系统
    ├── CriterionTrigger.hpp / cpp   # 触发器接口（ICriterionTrigger、AbstractCriterionTrigger）
    ├── CriterionTriggers.hpp / cpp  # 触发器注册表（单例，管理所有触发器）
    │
    ├── conditions/                  # 触发器条件谓词（用于匹配特定游戏状态）
    │   ├── ItemPredicate.hpp / cpp          # 物品匹配（ID、数量、耐久、药水等）
    │   ├── EntityPredicate.hpp / cpp        # 实体匹配 + DamageSourcePredicate
    │   ├── EntityFlagsPredicate.hpp / cpp   # 实体标志匹配（燃烧、潜行、疾跑等）
    │   ├── EntityEquipmentPredicate.hpp / cpp  # 装备匹配（头盔、胸甲、护腿、靴子、主手、副手）
    │   ├── NBTPredicate.hpp / cpp           # NBT 数据匹配（递归比较）
    │   ├── LocationPredicate.hpp / cpp      # 位置匹配（坐标、维度、生物群系）
    │   ├── BlockPredicate.hpp / cpp         # 方块匹配 + FluidPredicate
    │   ├── MobEffectsPredicate.hpp / cpp    # 效果匹配（效果类型、等级、持续时间）
    │   └── README.md
    │
    └── impl/                        # 触发器实现
        ├── ImpossibleTrigger.hpp / cpp           # 不可能触发器（手动授予）
        ├── TickTrigger.hpp / cpp                 # Tick 触发器（每 tick 触发）
        ├── InventoryChangedTrigger.hpp / cpp     # 物品栏变化触发器
        ├── LocationTrigger.hpp / cpp             # 位置触发器（维度、生物群系检测）
        ├── PlayerKilledEntityTrigger.hpp / cpp   # 玩家击杀实体触发器
        ├── BlockTriggers.hpp / cpp               # 方块相关触发器（放置、进入、滑落、蜂巢破坏）
        ├── ItemTriggers.hpp / cpp                # 物品相关触发器（消耗、耐久变化、附魔、装桶）
        ├── EntityTriggers.hpp / cpp              # 实体相关触发器（驯服、繁殖、交易、治愈等）
        ├── EffectTriggers.hpp / cpp              # 效果相关触发器（效果变化）
        └── ChanneledLightningTrigger.hpp / cpp   # 引雷附魔触发器
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────┐
│ AdvancementManager                                                  │
│                     （成就注册表，单例）                              │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ 管理
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│ AdvancementList                                                     │
│                    （成就列表，父子关系）                             │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ 包含
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│ Advancement                                                         │
│                （成就定义，不可变对象）                               │
│  ┌─────────────────┬─────────────────┬─────────────────┐           │
│  │ AdvancementDisplay│ AdvancementRewards│ Criterion     │           │
│  │   （显示信息）    │    （奖励）      │  （条件定义）   │           │
│  └─────────────────┴─────────────────┴────────┬────────┘           │
└────────────────────────────────────────────────┼────────────────────┘
                                                 │ 引用
                                                 ▼
┌─────────────────────────────────────────────────────────────────────┐
│ CriterionTriggers                                                   │
│                    （触发器注册表，单例）                             │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ 管理
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│ ICriterionTrigger<T>                                                │
│                         （触发器接口）                               │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ 实现
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│ AbstractCriterionTrigger<T>                                         │
│                    （监听器管理基类）                                │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ 派生
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 具体触发器实现                                                       │
│ InventoryChangedTrigger │ LocationTrigger │ PlayerKilledEntityTrigger │ ...
└───────────────────────────┬─────────────────────────────────────────┘
                            │ 使用
                            ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 条件谓词                                                             │
│ ItemPredicate │ EntityPredicate │ BlockPredicate │ LocationPredicate │ ...
└─────────────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 本模块依赖的外部模块

- `common/core/ResourceLocation.hpp` - 资源位置（成就 ID、触发器 ID）
- `common/util/DateTimeUtils.hpp` - 日期时间格式化/解析（CriterionProgress 序列化）
- `common/util/nbt/` - NBT 系统（NBTPredicate）
- `common/entity/` - 实体系统（EntityPredicate、EntityFlagsPredicate、EntityEquipmentPredicate）
- `common/entity/effect/` - 效果系统（MobEffectsPredicate）
- `common/entity/damage/` - 伤害系统（DamageSourcePredicate）
- `common/world/` - 世界接口（LocationPredicate、BlockPredicate）
- `common/world/biome/` - 生物群系注册表（LocationPredicate）
- `common/item/` - 物品系统（ItemPredicate）
- `common/item/loot/StatePropertiesPredicate.hpp` - 状态属性谓词（BlockPredicate 复用）
- `common/resource/repository/DataPackRepository.hpp` - 数据包仓库（AdvancementLoader 加载数据包用）
- `nlohmann/json.hpp` - JSON 解析

### 依赖本模块的外部模块

- `server/advancement/` - 服务端成就系统
  - `PlayerAdvancements` - 玩家进度管理
  - `AdvancementEventHandler` - 事件处理器
  - `TriggerInstantiation` - 触发器实例化工具
- `common/network/ir/packets/play/PlayPacketsExtended.hpp` - 成就 IR 网络包（`UpdateAdvancements`/`SelectAdvancementTab`/`SeenAdvancements`，旧 `common/network/packet/AdvancementPackets.hpp` 已删除）
- `common/item/loot/conditions/` - 战利品条件复用谓词
  - `EntityPropertiesCondition` - 复用 EntityPredicate
  - `LocationCheckCondition` - 复用 LocationPredicate
  - `DamageSourcePropertiesCondition` - 复用 DamageSourcePredicate
- `server/command/commands/ReloadCommand` - /reload 命令重新加载进度
- `server/application/MinecraftServer` - 启动时通过 AdvancementLoader 加载进度

## 容易踩的坑

1. **触发器注册**：所有触发器必须在 `CriterionTriggers::registerBuiltinTriggers()` 中注册，否则无法从 JSON 加载。新增触发器后记得在此方法中添加注册代码。

2. **谓词为空语义**：大多数谓词的默认构造（空状态）表示"匹配任意"，而非"不匹配"。检查谓词时需注意 `isAny()` 的语义。

3. **实体类型检查**：`EntityEquipmentPredicate` 和 `MobEffectsPredicate` 只对 `LivingEntity` 有效，非 `LivingEntity` 对这些谓词返回 `false`（除非谓词为空）。

4. **伤害源标志**：`EnvironmentalDamage` 的 `isProjectile()` 和 `isExplosion()` 始终返回 `false`，投射物和爆炸伤害需使用 `EntityDamageSource` 或 `IndirectEntityDamageSource`。

5. **维度名称**：维度检查使用 `ResourceLocation` 的路径部分（如 `overworld`），不是完整字符串。

6. **效果类型解析**：使用 `getEffectByResourceLocation()` 解析效果类型，未知效果会被跳过并输出警告日志。

7. **流体等效性**：`FluidPredicate` 使用 `Fluid::isEquivalentTo()` 比较，`minecraft:water` 同时匹配水源和流动水。

8. **状态属性复用**：`BlockPredicate` 复用 `mc::StatePropertiesPredicate`（位于 `common/item/loot/`），而非重新实现。

9. **触发器模板模式**：创建新触发器需继承 `AbstractCriterionTrigger<T>`，其中 `T` 是触发器实例类型。`CriterionInstance<T>` 提供条件检测的 `test()` 方法。

10. **服务端触发路径**：服务端触发成就需通过 `AdvancementEventHandler` 订阅事件，然后调用 `trigger->trigger(*advancements, predicate)`。直接调用触发器不会生效，因为没有监听器上下文。

11. **可见性判定仅使用 isDone**：`AdvancementVisibilityEvaluator` 的 `isDone` 谓词只检查成就是否完成，不使用 `hasProgress`（部分完成）。这与 MC Java 原版一致——部分完成不影响可见性。`PlayerAdvancements._shouldShow` 也仅使用 `isDone`。

12. **无 display 成就的 anyChildDone 行为**：当无 display 的成就有已完成子成就时，`anyChildDone = true` 会在算法层面将其标记为"可见"。这与 MC Java 一致，客户端/UI 层负责过滤不渲染无 display 的成就。

13. **数据包目录名兼容**：`AdvancementLoader` 同时支持 MC 1.21+ 的单数目录名 `advancement/` 和 MC 1.16.5 的复数目录名 `advancements/`，`loadFromDataPackRepository` 和 `pathToAdvancementId` 均可处理两种格式。`pathToAdvancementId` 同时支持两种路径形式：含 `data/` 前缀（`data/minecraft/advancements/...`）和相对于 `data/` 根（`minecraft/advancements/...`，后者是 `DataPackRepository::listResources` 返回的形式）。

14. **服务器启动加载**：`MinecraftServer::initializeRegistries()` 会在启动时通过 `AdvancementLoader::loadFromDataPackRepository()` 从数据包加载进度。`/reload` 命令也会重新加载进度。

15. **AdvancementManager 是单例**：`AdvancementManager::instance()` 是全局单例，`AdvancementLoader` 通过该单例注册成就，无需构造函数注入管理器引用。

16. **CriterionProgress 序列化格式兼容 MC Java 版**：`CriterionProgress::toJson()` 输出 MC Java 版兼容的日期时间字符串格式（`"yyyy-MM-dd HH:mm:ss Z"`，如 `"2024-06-15 14:30:00 +0800"`），与 Java 版 `AdvancementProgress.OBTAINED_TIME_FORMAT` 完全对应。`CriterionProgress::fromJson()` 支持三种格式：MC Java 字符串格式、数字格式（项目内部毫秒时间戳）和对象格式（`{"obtainedTime": 毫秒}`），确保与 Java 版存档的完整互操作。格式化/解析由 `common/util/DateTimeUtils.hpp` 统一提供。
