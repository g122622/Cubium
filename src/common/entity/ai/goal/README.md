# AI 目标系统 (Goal System)

## 目录结构

```
goal/
├── Goal.hpp                  # AI目标基类
├── GoalConstants.hpp         # 目标系统常量定义
├── GoalFlag.hpp              # 互斥标志枚举
├── GoalSelector.hpp          # 目标选择器
├── PrioritizedGoal.hpp       # 带优先级的目标包装器
└── goals/                    # 具体目标实现
    ├── AvoidEntityGoal.hpp/cpp   # 避开实体目标
    ├── AvoidBlockGoal.hpp/cpp   # 避开方块目标（主动远离指定标签的方块，如排斥物）
    ├── BreedGoal.hpp/cpp         # 繁殖目标
    ├── EatGrassGoal.hpp/cpp      # 吃草目标（羊等草食动物）
    ├── FollowParentGoal.hpp/cpp  # 跟随父母目标
    ├── LookAtGoal.hpp/cpp        # 看向目标/随机看向
    ├── MeleeAttackGoal.hpp/cpp   # 近战攻击目标
    ├── PanicGoal.hpp/cpp         # 恐慌逃跑目标
    ├── RandomWalkingGoal.hpp/cpp # 随机漫步目标
    ├── SwimGoal.hpp/cpp          # 游泳目标
    ├── TemptGoal.hpp/cpp         # 食物诱惑目标
    ├── RandomSwimmingGoal.hpp/cpp # 随机游泳目标（水生生物）
    ├── FishSwimGoal.hpp/cpp      # 鱼类游泳目标（检查 canRandomSwim）
    ├── FindWaterGoal.hpp/cpp     # 寻找水源目标（水生生物）
    ├── SwimUpGoal.hpp/cpp        # 向上游目标（水生生物）
    ├── AdditionalGoals.hpp/cpp   # 已迁移：所有目标已移至独立文件（见下方）
    ├── FleeSunGoal.hpp/cpp       # 躲避阳光目标（亡灵生物日间避阳，主动寻找阴影）
    ├── RestrictSunGoal.hpp/cpp   # 限制阳光目标（配置导航器避阳路径，骷髅优先使用）
    ├── FindShelterGoal.hpp/cpp   # 寻找庇护所目标（日间寻找阴影位置）
    ├── FlyGoal.hpp/cpp           # 飞行目标（蝙蝠等飞行生物随机飞行）
    ├── movement/                 # 移动类目标
    │   └── MovementGoals.hpp/cpp # WaterAvoidingRandomWalkingGoal, LeapAtTargetGoal, MoveTowardsTargetGoal, MoveTowardsRestrictionGoal
    │   └── FollowSchoolLeaderGoal.hpp/cpp # 跟随群体领导者（群游鱼类）
    ├── attack/                   # 攻击类目标
    │   └── RangedAttackGoals.hpp/cpp # RangedAttackGoal, RangedBowAttackGoal, RangedCrossbowAttackGoal
    ├── target/                   # 目标选择目标
    │   ├── TargetGoals.hpp/cpp   # TargetGoal, NearestAttackableTargetGoal, HurtByTargetGoal等
    │   └── README.md             # 目标选择器详细文档
    ├── interact/                 # 交互类目标
    │   └── TameableGoals.hpp/cpp # FollowOwnerGoal, SitGoal, BegGoal
    ├── villager/                  # 村民目标（每个类独立文件）
    │   ├── VillagerGoalUtils.hpp/cpp # 村民目标共享辅助函数
    │   ├── SleepAtNightGoal.hpp/cpp  # 夜间睡眠目标
    │   ├── WorkAtJobSiteGoal.hpp/cpp # 工作站点目标（FarmerWorkGoal的基类）
    │   ├── FarmerWorkGoal.hpp/cpp    # 农民工作目标（继承WorkAtJobSiteGoal）
    │   ├── LookForJobSiteGoal.hpp/cpp # 寻找工作站点目标
    │   ├── GatherItemsGoal.hpp/cpp   # 收集地面物品目标
    │   ├── AvoidHostileGoal.hpp/cpp  # 逃避敌对生物目标
    │   ├── GoToBedGoal.hpp/cpp       # 前往床位目标
    │   ├── VillagerBreedGoal.hpp/cpp # 繁殖目标
    │   ├── CongregateGoal.hpp/cpp    # 聚集互动目标
    │   ├── ShareItemsGoal.hpp/cpp    # 分享物品目标
    │   ├── LookAtEntitiesGoal.hpp/cpp # 看向实体目标
    │   └── README.md                 # 村民目标详细文档
    └── special/                  # 特殊目标
        ├── SpecialGoals.hpp/cpp  # CreeperSwellGoal, RunAroundLikeCrazyGoal等
        ├── RavagerGoals.hpp/cpp  # RavagerAttackGoal 劫掠兽近战攻击目标
        ├── GuardianAttackGoal.hpp/cpp # 守卫者激光攻击目标
        └── MoveToLavaGoal.hpp/cpp # 移动到方块目标基类、炽足兽寻找熔岩目标
```

## 整体职责

AI 目标系统负责管理实体的智能行为决策。每个 AI 行为（如游泳、漫步、攻击、繁殖等）都被封装为一个独立的 Goal 对象，通过 GoalSelector 根据优先级和互斥标志协调多个目标的执行。

### 核心设计理念

1. **模块化**: 每个 AI 行为是独立的目标类，易于扩展和维护
2. **优先级系统**: 数值越小优先级越高，高优先级目标可以抢占低优先级目标
3. **互斥机制**: 通过 GoalFlag 标志防止冲突行为同时执行
4. **生命周期管理**: 统一的执行流程 (`shouldExecute` → `startExecuting` → `tick` → `resetTask`)

## 内部模块关系

```
Goal (基类)
    ↓ 继承
PrioritizedGoal (包装器，添加优先级)
    ↓ 管理
GoalSelector (选择器，协调目标执行)
    ↓ 使用
GoalFlag (互斥标志枚举)
```

**核心组件依赖关系：**
- `Goal.hpp` - 所有目标的抽象基类，定义生命周期接口
- `GoalFlag.hpp` - 定义互斥标志（Move, Look, Jump, Target）
- `GoalConstants.hpp` - 定义系统常量（距离、时间、概率等）
- `PrioritizedGoal.hpp` - 包装 Goal 并添加优先级和抢占逻辑
- `GoalSelector.hpp` - 管理所有目标，负责选择和执行

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

```cpp
// 实体系统
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"

// 控制器
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/controller/JumpController.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"

// 工具
#include "common/util/math/random/Random.hpp"
#include "common/core/EnumSet.hpp"
```

### 下游依赖（依赖本模块）

```cpp
// 动物实体
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/basic/CowEntity.hpp"
// ...其他动物

// 敌对生物
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
// ...其他生物
```

## 容易踩的坑

### 1. 优先级数值混淆

**问题**: 以为数值越大优先级越高。

**正确**: 数值越小优先级越高。

```cpp
// 正确：高优先级用小数值
m_goalSelector.addGoal(0, swimGoal);      // 最高优先级
m_goalSelector.addGoal(10, wanderGoal);   // 低优先级
```

### 2. 忘记设置互斥标志

**问题**: 两个目标同时修改实体状态导致冲突。

**解决**: 始终设置正确的互斥标志。

```cpp
// 正确：设置互斥标志
class MyGoal : public Goal {
    MyGoal() {
        setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
    }
};
```

### 3. `shouldContinueExecuting` 默认行为

**问题**: 依赖默认实现 `return shouldExecute()`，导致目标过早结束。

**解决**: 根据需要重写 `shouldContinueExecuting()`。

### 4. 空指针检查缺失

**问题**: 实体指针在目标执行期间可能失效。

**解决**: 在每个方法开始检查空指针。

```cpp
void tick() override {
    if (!m_mob) return;  // 防御性检查
    if (!m_target || !m_target->isAlive()) return;
    // 正常逻辑...
}
```

### 5. 目标状态未清理

**问题**: `resetTask()` 未清理所有状态。

**解决**: 确保 `resetTask()` 清理所有临时状态，包括导航路径。

### 6. 距离比较使用 sqrt

**问题**: 频繁调用 `sqrt()` 影响性能。

**解决**: 使用距离平方比较，或使用 `GoalConstants.hpp` 中预定义的平方常量。

### 7. 目标间竞争条件

**问题**: 多个相同优先级的目标可能竞争执行。

**解决**: 使用不同的优先级或互斥标志。

### 8. LookAtGoal 的类型过滤

LookAtGoal 支持通过 `TypeFilter<T>{}` 或自定义谓词过滤目标类型。使用 `TypeFilter<Player>{}` 可看向特定类型实体。

### 9. RangedCrossbowAttackGoal 需要 ICrossbowUser 接口

使用此目标的实体必须实现 `ICrossbowUser` 接口，包括 `setChargingCrossbow()`、`isChargingCrossbow()`、`onCrossbowLoadComplete()`、`shootCrossbow()` 等方法。

### 10. FishSwimGoal 与 FollowSchoolLeaderGoal 的协作

群游鱼类使用 `canRandomSwim()` 条件检查：
- 群首鱼（无 leader）: `canRandomSwim() = true`，执行 FishSwimGoal
- 跟随鱼（有 leader）: `canRandomSwim() = false`，执行 FollowSchoolLeaderGoal

### 11. BreedGoal 需要触发成就事件

繁殖成功时会调用 `IWorld::onBredAnimals()` 触发 `BredAnimalsEvent`，用于 `minecraft:bred_animals` 成就进度。事件包含繁殖发起者玩家 ID、子代实体、父母实体。

### 12. EatGrassGoal 需要 GameRule 检查

吃草时必须检查 `GameRuleKeys::MOB_GRIEFING` 游戏规则：
- `mobGriefing=true`：改变方块（草方块→泥土，移除草丛）
- `mobGriefing=false`：不改变方块
- 无论规则如何，都调用 `eatGrassBonus` 回调

### 13. TargetPredicate 自定义筛选

`NearestAttackableTargetGoal` 的 `TargetPredicate` 参数允许自定义目标筛选逻辑，如只攻击玩家、排除队友等。

### 14. TemptGoal 的虚方法扩展

`TemptGoal::isScaredByPlayerMovement()` 是虚方法，子类（如 `OcelotTemptGoal`）可重写以实现自定义行为。豹猫在未信任时会被玩家快速移动吓跑。

### 15. BegGoal 与 TemptGoal 的区别

| 特性 | BegGoal | TemptGoal |
|------|---------|-----------|
| 行为 | 只看向玩家，不移动 | 跟随玩家移动 |
| 适用动物 | 狼 | 牛、猪、羊等 |
| 互斥标志 | `Look` | `Move`, `Look` |
| 驯服物品支持 | 已驯服动物对驯服物品乞求 | 不检查驯服状态 |

### 16. AvoidBlockGoal 方块回避目标

`AvoidBlockGoal` 使生物主动远离指定标签的方块，对应 MC 1.21.11 的 `SetWalkTargetAwayFrom.pos(NEAREST_REPELLENT)` 行为。

**核心逻辑**：
1. `shouldExecute()`：扫描附近区域寻找匹配标签的排斥方块，找到后使用 `RandomPositionGenerator` 计算远离该方块的位置
2. `shouldContinueExecuting()`：检查是否仍在排斥范围内、导航路径是否完成
3. `tick()`：导航完成后若仍靠近排斥方块，尝试重新计算逃跑位置

**使用场景**：
- 疣猪兽远离 `HOGLIN_REPELLENTS` 方块（诡异菌、诡异菌岩、下界传送门、重生锚）
- 猪灵远离 `PIGLIN_REPELLENTS` 方块（灵魂火、灵魂火把、灵魂灯笼、灵魂营火、诡异菌）

**BlockValidator 回调**：某些方块需要额外状态检查（如灵魂营火需点燃才排斥猪灵），通过 `BlockValidator` 函数实现：

```cpp
// 带验证函数的构造
m_goalSelector.addGoal(4,
    std::make_unique<AvoidBlockGoal>(
        this, BlockTags::PIGLIN_REPELLENTS(), 1.0, 8, 4,
        [](const BlockState& state) {
            if (state.is(block_registry::NetherBlocks::SOUL_CAMPFIRE)) {
                return blocks::CampfireBlock::isLitCampfire(state);
            }
            return true;
        }));
```

**与 `getPathWeight()` 的协同**：`AvoidBlockGoal` 提供主动逃跑行为，而 `getPathWeight()` 返回 `-1.0f` 阻止寻路穿过排斥区域。两者共同实现了 MC 原版 `isPosNearNearestRepellent` + `SetWalkTargetAwayFrom` 的完整行为。

**互斥标志**：`GoalFlag::Move`（与 `AvoidEntityGoal`、`FleeSunGoal` 等一致）

### 17. MeleeAttackGoal 在 resetTask 中清除激怒状态

`MeleeAttackGoal::resetTask()` 对应 MC 1.21.11 `MeleeAttackGoal#stop`，在目标丢失或目标死亡时调用 `m_mob->setAggroed(false)`（委托 `setAggressive(false)`）清除激怒状态。

**重要**：`resetTask()` **不再** 标记为 `noexcept`。原因：`setAggroed → setAggressive → m_dataManager.set` 涉及互斥锁，理论上可抛异常。继承 `MeleeAttackGoal` 的子类（如 `PolarBearMeleeAttackGoal`）重写 `resetTask` 时也必须移除 `noexcept`，否则在异常传播时会导致 `std::terminate`。

**数据流**：`resetTask` 调用 `setAggroed(false)` → `DATA_MOB_FLAGS_PARAM` 位 2 清除 → `ir::play::SetEntityData` 广播 → 客户端 `ClientEntity::syncMetadataFromDataManager` 读取 → `m_isAggressive=false` → `EntityRendererManager::_applyZombieState` 推送到 `ZombieModel::setAggressive(false)` → 手臂从 `-PI/1.5`（攻击抬臂）切换到 `-PI/2.25`（自然站立）。
