# 村民AI目标 (Villager AI Goals)

本目录实现村民特有的AI目标系统。

## 目录结构

```
villager/
├── VillagerGoals.hpp      # 村民AI目标头文件（定义10个目标类）
├── VillagerGoals.cpp      # 村民AI目标实现（FarmerWorkGoal含收获/种植/堆肥逻辑）
└── README.md              # 本文档
```

## 内部模块关系

本目录包含10个村民专用目标类，继承自 `Goal` 基类：

```
Goal (基类)
├── SleepAtNightGoal      ─┐
├── GoToBedGoal            │ 夜间睡眠相关
├── WorkAtJobSiteGoal     ─┼─ 工作相关
│   └── FarmerWorkGoal    ─┘    (继承WorkAtJobSiteGoal)
│       ├── _tryHarvest()        收获成熟作物（3x3x3蓄水池抽样）
│       ├── _tryPlant()          在空耕地上种植种子
│       ├── _tryCompost()        多余种子堆肥/取出骨粉
│       ├── _harvestCrop()       收获单个作物（生成掉落物+onBlockRemoved+设为空气）
│       ├── _hasFarmSeeds()      检查背包是否有可种植种子
│       ├── _isCropMatureAt()    检查位置是否为成熟作物
│       ├── _canPlantAt()        检查位置是否可种植（空气+下方耕地）
│       ├── _isValidFarmPos()    判断是否为有效农田位置
│       └── _pickValidFarmland() 蓄水池抽样选取有效位置
├── LookForJobSiteGoal    ─── 就业相关
├── GatherItemsGoal       ─── 物品收集
├── AvoidHostileGoal      ─── 安全逃避
├── VillagerBreedGoal     ─── 繁殖
├── CongregateGoal        ─┐
├── ShareItemsGoal        ─┼─ 社交互动
└── LookAtEntitiesGoal    ─┘
```

## 上下游外部依赖关系

**被以下模块依赖：**
- `VillagerEntity::registerGoals()` - 注册村民AI目标

**依赖以下模块：**
- `Goal` / `GoalSelector` - AI目标基类和选择器
- `VillagerEntity` - 村民实体，提供状态查询和行为接口
- `IWorld` - 世界接口，获取时间、方块状态
- `PathNavigator` - 寻路导航（通过Entity间接使用）
- `EntityUtils` - 实体工具类（查找附近实体）
- `PointOfInterestStorage` - POI系统（查找床位、工作站点、堆肥桶）
- `VillageManager` - 村庄管理器（获取POI存储）
- `Brain` / `MemoryModuleType` - 大脑记忆系统（HOME、MEETING_POINT等）
- `ItemEntity` - 物品实体（收集目标）
- `IMob` - 敌对生物接口（逃避检测）
- `BedBlock` - 床方块（验证床位有效性）
- `CropBlock` - 作物方块（收获逻辑：getCropItem/getSeedItem/isMaxAge/withAge）
- `FarmlandBlock` - 耕地方块（种植条件判断）
- `ComposterBlock` - 堆肥桶方块（attemptCompost/empty/getLevel）
- `BlockItemRegistry` - 方块物品注册表（种子→作物方块映射）
- `ItemDropHelper` - 物品掉落工具（生成掉落物实体）
- `BlockRegistry` - 方块注册表（获取空气方块状态）
- `BlockTags` - 方块标签系统（可替换方块判断）

## 互斥标志

| 目标 | 互斥标志 |
|------|----------|
| SleepAtNightGoal | Move, Look |
| WorkAtJobSiteGoal / FarmerWorkGoal | Move, Look |
| LookForJobSiteGoal | Move |
| GatherItemsGoal | Move |
| AvoidHostileGoal | Move |
| GoToBedGoal | Move |
| VillagerBreedGoal | Move, Look |
| CongregateGoal | Move, Look |
| LookAtEntitiesGoal | Look |
| ShareItemsGoal | Move, Look |

## 食物分享机制

ShareItemsGoal 和 CongregateGoal::_shareItems() 使用相同的食物分享逻辑：

**分享条件**（优先级从高到低）：
1. 食物分享：村民有食物过剩（`hasExcessFood()`，食物点数 >= 24）时抛出一半食物给目标
2. 小麦分享：农民有超过半组小麦时（>32）抛出一半小麦给目标

**抛出数量规则**（throwHalfStackToTarget）：
- 库存中某物品 > maxStackSize/2（通常 >32）：抛出 count/2 个
- 库存中某物品 > 24 但不超过半组：保留24个，抛出剩余

**物品实体生成**：创建 ItemEntity 抛向目标方向，设置 40 tick 拾取延迟和所有者标识（防止村民捡回自己扔出的物品）。

## 容易踩的坑

1. **SleepAtNightGoal 与 GoToBedGoal 的区别**：两者功能相似但触发条件略有不同。`SleepAtNightGoal` 侧重夜间睡眠逻辑，`GoToBedGoal` 侧重导航到床位。实际使用时注意避免重复注册导致冲突。

2. **床位的 POI 类型遍历**：床有多种颜色，对应多个 POI 类型（`PointOfInterestType::BedRed` 到 `BedYellow`）。查找床位时必须遍历所有床类型，否则可能漏掉某些颜色的床。

3. **GlobalPos 维度检查**：HOME 记忆使用 `GlobalPos` 类型包含维度信息，使用时必须检查维度是否匹配当前世界，否则可能传送到错误维度。

4. **EntityId 有效期**：`m_targetItem`、`m_hostileEntity`、`m_partnerId` 等存储的是 EntityId，使用前必须通过 `world->getEntity()` 验证实体是否仍然存在且存活。

5. **FarmerWorkGoal 继承**：`FarmerWorkGoal` 继承自 `WorkAtJobSiteGoal`，调用 `tick()` 时会先执行父类逻辑（移动到工作站点、增加经验），再执行农民特有行为。如需覆盖父类行为需谨慎处理。

6. **FarmerWorkGoal 收获逻辑**：收获作物时不使用 `destroyBlock`（需要 `ServerWorld`），而是手动生成掉落物（通过 `CropBlock::getCropItem()/getSeedItem()` 获取物品ID，放入背包或丢在地上），然后调用 `onBlockRemoved()` 通知方块移除回调，最后将方块设为空气。

7. **FarmerWorkGoal 种植逻辑**：种植时通过 `_getCropBlockForSeed()` 将种子物品映射为作物方块（小麦种子→minecraft:wheat，胡萝卜→minecraft:carrots，马铃薯→minecraft:potatoes，甜菜种子→minecraft:beetroots），然后放置默认状态（age=0）。`_hasFarmSeeds()` 检查小麦种子、胡萝卜、马铃薯、甜菜种子。**注意**：当前作物方块尚未注册到 `BlockItemRegistry`，种植功能依赖作物方块在 `BlockRegistry` 中的注册，需要后续完成 `AgriculturalBlocks` 注册。

8. **FarmerWorkGoal 堆肥逻辑**：堆肥只处理小麦种子和甜菜种子（保留10个，多余的最多20个用于堆肥）。使用 `ComposterBlock::attemptCompost()` 逐个尝试堆肥。满桶时使用 `ComposterBlock::empty()` 取出骨粉。

9. **时间判断常量**：夜间时间范围 `12542-23459` tick，工作时间范围 `2000-9000` tick。这些是 MC 1.16.5 的固定值，不要硬编码在其他地方。

10. **Brain 记忆类型**：使用 `MemoryModuleTypes::HOME` 和 `MemoryModuleTypes::MEETING_POINT` 时需确保 Brain 系统已正确初始化这些记忆模块。
