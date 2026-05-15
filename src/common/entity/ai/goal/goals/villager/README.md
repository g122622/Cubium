# 村民AI目标 (Villager AI Goals)

本目录实现村民特有的AI目标系统。

## 目录结构

```
villager/
├── VillagerGoals.hpp      # 村民AI目标头文件
├── VillagerGoals.cpp      # 村民AI目标实现
└── README.md              # 本文档
```

## AI目标类

### SleepAtNightGoal - 夜间睡眠目标

村民在夜间寻找床位并睡眠。

**执行条件**：
- 是夜间时间（12542-23459 tick）
- 有绑定的床位

**行为**：
1. 移动到床位位置
2. 到达后进入睡眠状态
3. 天亮后自动醒来

### WorkAtJobSiteGoal - 工作目标

村民在工作时间前往工作站点工作。

**执行条件**：
- 不是傻子村民
- 是工作时间（2000-9000 tick）
- 有绑定的工作站点

**行为**：
1. 移动到工作站点
2. 执行工作（增加经验）
3. 定期补货

### LookForJobSiteGoal - 寻找工作站点目标

无职业村民寻找可用的工作站点。

**执行条件**：
- 没有工作站点
- 不是傻子村民
- 搜索冷却结束

**行为**：
1. 搜索附近的工作站点
2. 移动到目标位置
3. 绑定工作站点

### GatherItemsGoal - 收集物品目标

村民收集地上的食物或种子等物品。

**执行条件**：
- 附近有可拾取的物品（32格范围内）
- 物品类型在允许列表中：面包、土豆、胡萝卜、小麦、小麦种子、甜菜根、甜菜根种子
- 村民库存有空间

**行为**：
1. `findNearbyItems()`: 使用 EntityUtils::findClosestEntity<ItemEntity> 搜索附近可拾取物品
2. `moveToItem()`: 获取物品实体并移动到物品位置（速度 0.5）
3. `pickupItem()`: 在 1.5 格范围内拾取物品，添加到村民库存
4. `shouldContinueExecuting()`: 检查物品实体是否有效、是否仍在范围内

**参考**：MC 1.16.5 `VillagerEntity.updateEquipmentIfNeeded()`

### FarmerWorkGoal - 农民工作目标

农民特有的工作行为：种植、收获、堆肥。

**继承自**：WorkAtJobSiteGoal

**额外行为**：
- 收获成熟作物
- 种植作物
- 使用堆肥桶

### AvoidHostileGoal - 逃避敌对目标

村民逃离僵尸、掠夺者等敌对生物。

**执行条件**：
- 附近有敌对生物（8格范围内）
- 敌对生物实现了 IMob 接口

**行为**：
1. `findNearestHostile()`: 使用 EntityUtils::findClosestEntity<LivingEntity> 搜索附近的敌对生物
2. `fleeFromHostile()`: 计算远离敌对生物的方向，移动到安全位置（16格逃跑距离）
3. `shouldContinueExecuting()`: 检查敌对生物是否仍然存在、距离是否超过逃跑距离的2倍

**常量**：
- `FLEE_RANGE = 8.0f`: 敌对生物触发距离
- `FLEE_DISTANCE = 16.0f`: 逃跑距离
- `FLEE_SPEED = 0.6f`: 逃跑速度倍率

### GoToBedGoal - 前往床位目标

夜间前往床上睡觉的导航目标。

**执行条件**：
- 是夜间时间
- 有绑定的床位

**行为**：
1. 导航到床位
2. 到达后开始睡眠

### VillagerBreedGoal - 繁殖目标

村民繁殖行为。

**执行条件**：
- 愿意繁殖（isWillingToBreed()）
- 有足够的床位
- 找到配偶（8格范围内）

**行为**：
1. `findPartner()`: 使用 EntityUtils::findClosestEntity<VillagerEntity> 搜索附近愿意繁殖的村民
2. `moveToPartner()`: 获取配偶实体并移动到其位置（速度 0.5）
3. `tick()`: 检查距离，距离 <= 2格 且 繁殖计时器 >= 60 tick 时生成幼年村民
4. `spawnChild()`: 创建幼年村民实体并生成到世界

**常量**：
- `BREED_TICKS = 60`: 繁殖动画时长
- `BREED_DISTANCE = 2.0f`: 繁殖触发距离

## 互斥标志

| 目标 | 互斥标志 |
|------|----------|
| SleepAtNightGoal | Move, Look |
| WorkAtJobSiteGoal | Move, Look |
| LookForJobSiteGoal | Move |
| GatherItemsGoal | Move |
| AvoidHostileGoal | Move |
| GoToBedGoal | Move |
| VillagerBreedGoal | Move, Look |

## 依赖关系

```
VillagerGoals
    ├── VillagerEntity (村民实体)
    ├── IWorld (世界接口)
    ├── BlockPos (方块位置)
    ├── PathNavigator (寻路导航)
    ├── GoalSelector (目标选择器)
    └── POI系统 (兴趣点，待集成)
```

## 使用方法

```cpp
// 在VillagerEntity::registerGoals()中注册
void VillagerEntity::registerGoals() {
    // 高优先级 - 逃避敌对
    goalSelector().addGoal(1, std::make_unique<AvoidHostileGoal>(this));

    // 中优先级 - 繁殖
    goalSelector().addGoal(2, std::make_unique<VillagerBreedGoal>(this));

    // 正常优先级 - 工作、睡眠
    goalSelector().addGoal(3, std::make_unique<WorkAtJobSiteGoal>(this));
    goalSelector().addGoal(3, std::make_unique<SleepAtNightGoal>(this));

    // 低优先级 - 其他行为
    goalSelector().addGoal(4, std::make_unique<LookForJobSiteGoal>(this));
    goalSelector().addGoal(5, std::make_unique<GatherItemsGoal>(this));
}
```

## 时间系统

MC 1.16.5 时间参考：
- **夜间**：12542-23459 tick（黄昏到黎明）
- **工作时间**：2000-9000 tick
- **一天**：24000 tick（20分钟）

## TODO

- [ ] 集成POI系统查找床位和工作站点
- [ ] 实现完整的睡眠状态管理
- [ ] 实现农民的作物种植和收获
- [x] 实现敌对生物检测（AvoidHostileGoal.findNearestHostile）
- [x] 实现繁殖时的配偶查找（VillagerBreedGoal.findPartner）
- [x] 实现物品拾取逻辑（GatherItemsGoal）

## 参考

- MC 1.16.5 `net.minecraft.entity.ai.brain.task.villager.*`
- MC 1.16.5 `net.minecraft.entity.ai.goal.SleepAtNightGoal`
- MC 1.16.5 `net.minecraft.entity.ai.goal.WorkAtJobSiteGoal`
