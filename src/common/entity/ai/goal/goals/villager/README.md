# 村民AI目标 (Villager AI Goals)

本目录实现村民特有的AI目标系统。每个目标类独立一个文件，共享辅助函数放在 `VillagerGoalUtils` 中。

## 目录结构

```
villager/
├── AvoidHostileGoal.hpp/cpp      # 逃离敌对生物（僵尸、掠夺者等）
├── CongregateGoal.hpp/cpp        # 聚集互动（流言传播、物品分享）
├── FarmerWorkGoal.hpp/cpp        # 农民工作（种植、收获、堆肥），继承WorkAtJobSiteGoal
├── GatherItemsGoal.hpp/cpp       # 收集地面物品
├── GoToBedGoal.hpp/cpp           # 前往床位导航
├── LookAtEntitiesGoal.hpp/cpp    # 随机看向附近实体
├── LookForJobSiteGoal.hpp/cpp    # 无职业村民寻找工作站点
├── ShareItemsGoal.hpp/cpp        # 农民分享食物给其他村民
├── SleepAtNightGoal.hpp/cpp      # 夜间寻找床位并睡眠
├── VillagerBreedGoal.hpp/cpp     # 村民繁殖
├── VillagerGoalUtils.hpp/cpp     # 共享辅助函数（距离计算、物品抛出等）
├── WorkAtJobSiteGoal.hpp/cpp     # 工作站点工作（基类，FarmerWorkGoal继承此类）
└── README.md                     # 本文档
```

## 内部模块关系

本目录包含11个村民专用目标类，继承自 `Goal` 基类：

```
Goal (基类)
├── SleepAtNightGoal      ─┐ 夜间睡眠相关
├── GoToBedGoal            │
├── WorkAtJobSiteGoal     ─┼─ 工作相关（含补货逻辑，300tick冷却检查）
│   └── FarmerWorkGoal    ─┘    (继承WorkAtJobSiteGoal)
├── LookForJobSiteGoal    ─── 就业相关
├── GatherItemsGoal       ─── 物品收集
├── AvoidHostileGoal      ─── 安全逃避
├── VillagerBreedGoal     ─── 繁殖（无床位时双亲广播VillagerAngry粒子+重置伙伴繁殖意愿）
├── CongregateGoal        ─┐
├── ShareItemsGoal        ─┼─ 社交互动
└── LookAtEntitiesGoal    ─┘
```

## 上下游外部依赖关系

**被以下模块依赖：**
- `VillagerEntity::registerGoals()` - 注册村民AI目标

**依赖以下模块：**
- `Goal` / `GoalSelector` - AI目标基类和选择器
- `VillagerEntity` - 村民实体，提供状态查询和行为接口（含 `isNightTime()`、`isWorkTime()`）
- `VillagerGoalUtils` - 村民目标共享辅助函数（`isWithinDistance`、`distanceToBlockCenter`、`throwHalfStackToTarget`）
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
- `VanillaBlocks` - 原版方块静态引用（AgriculturalBlocks::WHEAT/CARROTS/POTATOES/BEETROOTS 用于种子→作物映射）
- `ItemDropHelper` - 物品掉落工具（生成掉落物实体）
- `BlockRegistry` - 方块注册表（获取空气方块状态）
- `BlockTags` - 方块标签系统（可替换方块判断）
- `broadcastEntityStatus` - 实体状态广播（`VillagerBreedGoal` 无床位时广播 `VillagerAngry` 粒子，经 IR `ir::play::EntityEvent` 传输，状态枚举见 `network/protocol/EntityEvents.hpp`）

## 容易踩的坑

1. **SleepAtNightGoal 与 GoToBedGoal 的区别**：两者功能相似但触发条件略有不同。`SleepAtNightGoal` 侧重夜间睡眠逻辑，`GoToBedGoal` 侧重导航到床位。实际使用时注意避免重复注册导致冲突。

2. **床位的 POI 类型遍历**：床有多种颜色，对应多个 POI 类型（`PointOfInterestType::BedRed` 到 `BedYellow`）。查找床位时必须遍历所有床类型，否则可能漏掉某些颜色的床。

3. **GlobalPos 维度检查**：HOME 记忆使用 `GlobalPos` 类型包含维度信息，使用时必须检查维度是否匹配当前世界，否则可能传送到错误维度。

4. **EntityId 有效期**：`m_targetItem`、`m_hostileEntity`、`m_partnerId` 等存储的是 EntityId，使用前必须通过 `world->getEntity()` 验证实体是否仍然存在且存活。

5. **FarmerWorkGoal 继承**：`FarmerWorkGoal` 继承自 `WorkAtJobSiteGoal`，调用 `tick()` 时会先执行父类逻辑（移动到工作站点、增加经验），再执行农民特有行为。如需覆盖父类行为需谨慎处理。

6. **FarmerWorkGoal 收获逻辑**：收获作物时不使用 `destroyBlock`（需要 `ServerWorld`），而是手动生成掉落物（通过 `CropBlock::getCropItem()/getSeedItem()` 获取物品ID，放入背包或丢在地上），然后调用 `onBlockRemoved()` 通知方块移除回调，最后将方块设为空气。

7. **FarmerWorkGoal 种植逻辑**：种植时通过 `ItemTags::VILLAGER_PLANTABLE_SEEDS` 标签判断可种植物品（小麦种子、胡萝卜、马铃薯、甜菜种子、火把花种子、瓶草荚果），再通过 `_getCropBlockForSeed()` 将种子物品映射为作物方块（小麦种子→`VanillaBlocks::WHEAT`，胡萝卜→`VanillaBlocks::CARROTS`，马铃薯→`VanillaBlocks::POTATOES`，甜菜种子→`VanillaBlocks::BEETROOTS`，火把花种子→`VanillaBlocks::TORCHFLOWER_CROP`，瓶草荚果→`VanillaBlocks::PITCHER_CROP`），然后放置默认状态（age=0）。种植后播放 `SoundEvents::ITEM_CROP_PLANT` 音效并触发 `GameEvent::BLOCK_PLACE` 事件。注意：瓶草作物（PitcherCropBlock）继承自 DoublePlantBlock 而非 CropBlock，村民可以种植但不能收获（与 MC 1.21.11 原版行为一致）。

8. **FarmerWorkGoal 堆肥逻辑**：堆肥只处理小麦种子和甜菜种子（保留10个，多余的最多20个用于堆肥）。使用 `ComposterBlock::attemptCompost()` 逐个尝试堆肥。满桶时使用 `ComposterBlock::empty()` 取出骨粉。

9. **时间判断**：夜间时间范围 `12542-23459` tick，工作时间范围 `2000-9000` tick。这些常量已统一到 `VillagerEntity::isNightTime()` 和 `VillagerEntity::isWorkTime()` 方法中。

10. **Brain 记忆类型**：使用 `MemoryModuleTypes::HOME` 和 `MemoryModuleTypes::MEETING_POINT` 时需确保 Brain 系统已正确初始化这些记忆模块。

11. **VillagerGoalUtils 共享函数**：`isWithinDistance()` 和 `distanceToBlockCenter()` 被4个目标类共享（SleepAtNightGoal、WorkAtJobSiteGoal、GoToBedGoal、LookForJobSiteGoal），`throwHalfStackToTarget()` 被2个目标类共享（CongregateGoal、ShareItemsGoal）。修改这些函数时需注意影响范围。

12. **VillagerBreedGoal 无床位失败处理**：当繁殖条件满足但没有可用床位时：
    - **双亲愤怒粒子**：对**两个**亲代村民都广播 `VillagerAngry` 粒子（status 13），而非仅广播一个。这是因为繁殖失败对双方都是负面反馈。
    - **伙伴繁殖意愿重置**：在无床位分支中，除了重置当前村民的繁殖意愿外，还必须重置伙伴的繁殖意愿（调用 `setWillingToMate(false)`），否则伙伴会持续尝试繁殖导致 AI 循环。
    - 依赖 `broadcastEntityStatus()` 发送粒子状态，客户端通过 `ClientPlayVisitor` 的 `onEntityStatus` 回调接收并渲染。
