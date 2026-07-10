# 傀儡实体模块

本目录包含傀儡实体的实现，包括铁傀儡、雪傀儡和铜傀儡。

## 目录结构

```
golem/
├── GolemEntity.hpp         # 傀儡基类（实现 IAngerable 接口）
├── GolemEntity.cpp         # 傀儡基类实现
├── SnowGolemEntity.hpp     # 雪傀儡实体（IShearable, IRangedAttackMob）
├── SnowGolemEntity.cpp     # 雪傀儡实现
├── IronGolemEntity.hpp     # 铁傀儡实体
├── IronGolemEntity.cpp     # 铁傀儡实现
├── CopperGolemTypes.hpp    # 铜傀儡类型定义（氧化等级、行为状态、工具集）
├── CopperGolemEntity.hpp   # 铜傀儡实体（IShearable，氧化与转雕像）
├── CopperGolemEntity.cpp   # 铜傀儡实现
└── README.md               # 本文件
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
│SnowGolemEntity│  │IronGolemEntity│  │CopperGolemEntity│
│(IShearable)  │   │              │   │(IShearable)     │
│(IRangedAttack)│  │              │   │(氧化/转雕像)     │
└──────────────┘   └──────────────┘   └─────────────────┘
```

- **GolemEntity**：傀儡基类，继承 `CreatureEntity` 并实现 `IAngerable` 接口，提供愤怒系统
- **SnowGolemEntity**：雪傀儡，实现远程攻击（雪球）和剪切（南瓜头）功能
- **IronGolemEntity**：铁傀儡，实现近战攻击和村民保护功能
- **CopperGolemEntity**：铜傀儡，实现氧化等级系统、斧头敲击雕像生成、涂蜡阻止氧化、剪刀剪切天线（罂粟花）、氧化到顶后转化为雕像

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
   - 目标位置雪层可以存活（通过 `SnowBlock::canSurviveAt` 检查，受 `SNOW_LAYER_CANNOT_SURVIVE_ON` 和 `SNOW_LAYER_CAN_SURVIVE_ON` 标签约束）

4. **剪切物品获取**：通过 `BlockItemRegistry::getBlockItem()` 获取 CARVED_PUMPKIN 对应的物品，确保物品系统已初始化后再调用。

5. **远程攻击命名空间**：雪球实体使用 `entity::SnowballEntity`（在 `mc::entity` 命名空间），需正确使用命名空间。

6. **玩家创建标记**：铁傀儡有 `m_playerCreated` 标记，玩家创建的铁傀儡不攻击玩家，需要在生成时正确设置。

7. **苦力怕排除**：铁傀儡不攻击苦力怕，通过重写 `canAttackType()` 实现（对应 MC 原版 `IronGolem.canAttackType()`），`TargetGoal::isSuitableTarget()` 自动调用此方法过滤目标类型。

8. **canAttackType 重写**：`IronGolemEntity::canAttackType()` 替代了之前的 `canAttackEntity()`，现在是 `MobEntity::canAttackType()` 的虚方法重写。修改铁傀儡可攻击类型时只需修改此方法，所有目标选择器自动继承。

9. **ATTACK_DAMAGE 属性注册**：铁傀儡的攻击伤害属性值为 15.0（MC 1.21.11 原版），需要在 `registerAttributes()` 中先调用 `registerAttribute(*Attributes::attackDamage())` 再 `setBaseValue()`，因为 `GolemEntity` 继承链（不同于 `MonsterEntity`）未注册此属性。

10. **伤害随机化**：`attackEntityAsMob()` 中的伤害计算 `damage/2.0f + nextInt((int)damage)` 对应 MC 原版 `f/2.0F + this.random.nextInt((int)f)`，其中 `(int)f` 是截断取整。

11. **铜傀儡氧化时序**：`m_nextWeatheringTick` 使用三个特殊值：`-2`（IGNORE_WEATHERING_TICK，已涂蜡）、`-1`（UNSET_WEATHERING_TICK，未设置）、`>=0`（绝对 tick）。涂蜡后永远不氧化，刮削后不重置 `m_nextWeatheringTick`（与 MC 一致）。

12. **铜傀儡转雕像条件**：`canTurnToStatue()` 同时检查脚下为空气且随机数 `<= 0.0058F`，对应 MC `level.getBlockState(blockPosition()).isAir() && random.nextFloat() <= 0.0058F`。`turnToStatue()` 在 `Oxidized` 等级且满足条件时由 `updateWeathering()` 调用。

13. **铜傀儡 NBT 持久化**：仅持久化 `next_weather_age`（i64）与 `weather_state`（string），`behaviorState` 为运行时动画状态不持久化（与 MC 1.21.11 一致）。NBT 键名常量定义在 `EntityNbtKeys.hpp` 的 `NEXT_WEATHER_AGE` 与 `WEATHER_STATE`。

14. **铜傀儡雕像架构差异**：本项目基础 `copper_golem_statue` 注册为 `CopperGolemStatueBlock`（不实现 `IOxidizableBlock`），与 MC 原版使用 `WeatheringCopperGolemStatueBlock(UNAFFECTED)` 不同。因此斧头生成铜傀儡的逻辑在 `CopperGolemStatueBlock::onBlockActivated` 中实现，并通过 `HoneycombItem::getWaxedOff(state)` 区分涂蜡变体（返回 Pass 交由 AxeItem 除蜡）与基础变体（生成铜傀儡）。

15. **Direction 转 yaw**：MC 的 `Direction.toYRot()` 在本项目无对应工具函数，`CopperGolemStatueBlockEntity::removeStatue` 与 `CopperGolemEntity::turnToStatue` 中手写转换：South=0, West=90, North=180, East=270。

16. **铜傀儡天线槽设计**：对应 MC 1.21.11 `CopperGolem.EQUIPMENT_SLOT_ANTENNA = EquipmentSlot.SADDLE`。铜傀儡头顶"天线"并非独立物品，而是 `EquipmentSlot::Saddle` 槽中持有的罂粟花（`minecraft:poppy`），由铁傀儡 `OfferFlowerGoal` 赠予。剪切时通过 `ItemTags::SHEARABLE_FROM_COPPER_GOLEM` 判断可剪性，取出 Saddle 槽物品并掉落。转雕像时由 `MobEntity::dropPreservedEquipment()` 自动掉落 Saddle 槽物品（需先 `setGuaranteedDrop(Saddle)` 标记保留，由 `OfferFlowerGoal` 调用）。

17. **EquipmentSlot::Saddle 扩展**：为支持铜傀儡天线槽，`EquipmentSlot` 枚举扩展了 `Saddle = 7`（`Count = 8`）。所有基于 `EquipmentSlot::Count` 的 `std::array` 自动扩展。`EquipmentSlotNames` 提供 `saddle` 名称映射，`ItemSlotArgument` 的 `saddle` 命名槽位映射到索引 106 → `EquipmentSlot` 索引 7。`Player::getEquipment` 的 `default` 分支返回空 `ItemStack`，因此玩家访问 Saddle 槽安全。
