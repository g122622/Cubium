# 村民AI目标 (Villager AI Goals)

本目录实现村民特有的AI目标系统。

## 目录结构

```
villager/
├── VillagerGoals.hpp      # 村民AI目标头文件（定义10个目标类）
├── VillagerGoals.cpp      # 村民AI目标实现
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
- `PointOfInterestStorage` - POI系统（查找床位、工作站点）
- `VillageManager` - 村庄管理器（获取POI存储）
- `Brain` / `MemoryModuleType` - 大脑记忆系统（HOME、MEETING_POINT等）
- `ItemEntity` - 物品实体（收集目标）
- `IMob` - 敌对生物接口（逃避检测）
- `BedBlock` - 床方块（验证床位有效性）

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

## 容易踩的坑

1. **SleepAtNightGoal 与 GoToBedGoal 的区别**：两者功能相似但触发条件略有不同。`SleepAtNightGoal` 侧重夜间睡眠逻辑，`GoToBedGoal` 侧重导航到床位。实际使用时注意避免重复注册导致冲突。

2. **床位的 POI 类型遍历**：床有多种颜色，对应多个 POI 类型（`PointOfInterestType::BedRed` 到 `BedYellow`）。查找床位时必须遍历所有床类型，否则可能漏掉某些颜色的床。

3. **GlobalPos 维度检查**：HOME 记忆使用 `GlobalPos` 类型包含维度信息，使用时必须检查维度是否匹配当前世界，否则可能传送到错误维度。

4. **EntityId 有效期**：`m_targetItem`、`m_hostileEntity`、`m_partnerId` 等存储的是 EntityId，使用前必须通过 `world->getEntity()` 验证实体是否仍然存在且存活。

5. **FarmerWorkGoal 继承**：`FarmerWorkGoal` 继承自 `WorkAtJobSiteGoal`，调用 `tick()` 时会先执行父类逻辑（移动到工作站点、增加经验），再执行农民特有行为。如需覆盖父类行为需谨慎处理。

6. **时间判断常量**：夜间时间范围 `12542-23459` tick，工作时间范围 `2000-9000` tick。这些是 MC 1.16.5 的固定值，不要硬编码在其他地方。

7. **Brain 记忆类型**：使用 `MemoryModuleTypes::HOME` 和 `MemoryModuleTypes::MEETING_POINT` 时需确保 Brain 系统已正确初始化这些记忆模块。
