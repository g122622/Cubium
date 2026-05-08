# 玩家系统模块

本目录包含玩家睡眠系统、重生点验证和物品冷却追踪的核心组件。

## 目录结构

```
src/common/entity/player/
├── CooldownTracker.hpp     # 物品冷却追踪器
├── CooldownTracker.cpp     # 冷却追踪器实现
├── SleepResult.hpp         # 睡眠结果枚举定义
├── SleepManager.hpp        # 睡眠管理器（静态工具类）
├── SleepManager.cpp        # 睡眠管理器实现
├── SpawnPointValidator.hpp # 重生点验证器（静态工具类）
├── SpawnPointValidator.cpp # 重生点验证器实现
└── README.md               # 本文档
```

## 文件说明

### CooldownTracker.hpp/cpp

物品冷却追踪器，用于追踪物品的冷却时间。参考 MC 1.16.5 `net.minecraft.util.CooldownTracker`。

#### 冷却进度说明

- 冷却进度值范围：`0.0` ~ `1.0`
- `1.0` 表示冷却刚开始
- `0.0` 表示冷却结束，物品可用
- 客户端渲染时使用 `(1.0 - progress)` 显示冷却动画

#### 核心方法

```cpp
class CooldownTracker {
public:
    // 每游戏 tick 调用，更新冷却状态
    void tick();
    
    // 设置物品冷却（ticks）
    void setCooldown(const Item* item, i32 ticks);
    
    // 移除物品冷却
    void removeCooldown(const Item* item);
    
    // 获取冷却进度（0.0-1.0），支持 partialTicks 插值
    f32 getCooldownProgress(const Item* item, f32 partialTicks = 0.0f) const;
    
    // 检查物品是否在冷却中
    bool hasCooldown(const Item* item) const;
    
    // 获取冷却剩余 tick 数
    i32 getCooldownTicks(const Item* item) const;
    
    // 辅助方法
    i32 currentTick() const;
    bool isEmpty() const;
    size_t cooldownCount() const;

protected:
    // 子类可重写的回调
    virtual void notifyOnSet(const Item* item, i32 ticks);
    virtual void notifyOnRemove(const Item* item);
};
```

#### 使用示例

```cpp
// 设置冷却（紫颂果 20 ticks = 1 秒）
player.cooldownTracker().setCooldown(item, 20);

// 检查冷却
if (!player.cooldownTracker().hasCooldown(item)) {
    // 物品可用
    useItem();
    player.cooldownTracker().setCooldown(item, 20);
}

// 获取冷却进度（用于渲染）
float progress = player.cooldownTracker().getCooldownProgress(item, partialTicks);
// progress = 0 表示冷却结束，1 表示刚开始
```

#### 典型应用场景

- **紫颂果**：20 ticks（1秒）冷却
- **末影珍珠**：20 ticks（1秒）冷却
- **盾牌**：被斧击中后 100 ticks（5秒）冷却
- **山羊角**：未实现，预期约 7 秒

### SleepResult.hpp

定义睡眠尝试的结果枚举：

- `OK` - 成功入睡
- `NOT_POSSIBLE_HERE` - 此维度不能睡眠（下界/末地）
- `NOT_POSSIBLE_NOW` - 现在不是睡眠时间
- `TOO_FAR_AWAY` - 离床太远
- `OBSTRUCTED` - 床被阻挡
- `OTHER_PROBLEM` - 其他问题
- `NOT_SAFE` - 周围有怪物

提供辅助函数：
- `getSleepResultMessage()` - 获取对应的翻译键
- `isSleepSuccess()` - 检查是否成功

### SleepManager.hpp/cpp

睡眠系统的静态工具类，提供以下功能：

#### 时间检测

```cpp
static bool canSleepAtTime(i64 dayTime, bool isThundering, bool isRaining);
```

根据游戏时间和天气判断是否可以睡眠：
- 雷暴时：任何时间
- 降雨时：12010 - 23991 ticks
- 晴天时：12542 - 23459 ticks

#### 位置检测

```cpp
static bool isPlayerNearBed(const Vector3& playerPos, const BlockPos& bedPos);
```

检测玩家是否在床附近（水平 3 格，垂直 2 格）。

#### 床状态检测

```cpp
static bool isBedObstructed(const IWorld& world, const BlockPos& bedPos, Direction bedFacing);
```

检测床上方是否有足够空间。

```cpp
static bool isBedSurroundedByMonsters(IWorld& world, const BlockPos& bedPos, const Player& player);
```

检测床周围是否有怪物（8x5x8 范围）。

#### 起床位置计算

```cpp
static std::optional<Vector3> findWakeUpPosition(const IWorld& world, const BlockPos& bedPos, Direction bedFacing);
```

计算玩家醒来时的站立位置。

### SpawnPointValidator.hpp/cpp

重生点验证器的静态工具类，用于验证玩家的重生点（床/重生锚）是否有效。

#### 重生点验证结果

```cpp
enum class SpawnPointValidationResult : u8 {
    Valid,                      // 重生点有效
    BedMissing,                 // 床不存在或被破坏
    BedWrongDimension,          // 床在错误的维度
    BedObstructed,              // 床被阻挡
    RespawnAnchorMissing,       // 重生锚不存在或被破坏
    RespawnAnchorNoCharge,      // 重生锚无能量
    RespawnAnchorWrongDimension,// 重生锚在错误的维度
    RespawnAnchorNoSafePosition,// 重生锚周围无安全位置
    BlockCannotSpawnIn,         // 方块不允许在内部生成
    DimensionNotFound,          // 维度不存在
    WorldNotFound               // 世界不存在
};
```

#### 验证方法

```cpp
static SpawnPointValidationResult validate(
    IWorld& world,
    const GlobalPos& spawnPoint,
    bool spawnForced,
    bool consumeCharge);
```

执行完整的重生点验证流程：
1. 检查世界和维度是否存在
2. 检查方块是否存在
3. 根据方块类型（床/重生锚/其他）执行特定验证
4. 查找安全生成位置

#### 安全位置查找

```cpp
static std::optional<Vector3> findSafeSpawnPosition(
    IWorld& world,
    const GlobalPos& spawnPoint,
    bool spawnForced,
    bool consumeCharge);
```

如果重生点有效，返回安全的生成位置。

#### 床验证

```cpp
static bool validateBedSpawn(IWorld& world, const BlockPos& bedPos);
static std::optional<Vector3> findBedSpawnPosition(IWorld& world, const BlockPos& bedPos);
```

验证床的重生点有效性，查找床周围的安全生成位置。

#### 重生锚验证

```cpp
static bool validateRespawnAnchorSpawn(IWorld& world, const BlockPos& anchorPos);
static std::optional<Vector3> findRespawnAnchorSpawnPosition(
    IWorld& world, const BlockPos& anchorPos, bool consumeCharge);
```

验证重生锚的重生点有效性，查找重生锚周围的安全生成位置。

#### 工具方法

```cpp
static bool isBed(const BlockState& state);              // 检查方块是否为床
static bool isRespawnAnchor(const BlockState& state);     // 检查方块是否为重生锚
static i32 getRespawnAnchorCharges(const BlockState& state); // 获取重生锚充能等级
```

## ServerWorld 睡眠管理

ServerWorld 提供以下睡眠管理功能：

- `updateAllPlayersSleepingFlag()` - 更新全员睡眠标志
- `checkSleepStatus()` - 检查并处理全员睡眠
- `wakeUpAllPlayers()` - 唤醒所有玩家
- `skipToMorning()` - 跳到早晨

当所有非观察者玩家完全入睡（睡眠计时器 >= 100 ticks）时，自动跳过夜晚。

## ServerPlayer 重生系统

ServerPlayer 提供以下重生相关功能：

- `determineRespawnPosition()` - 确定重生位置
- `determineRespawnDimension()` - 确定重生维度

重生逻辑：
1. 检查玩家个人重生点（床/重生锚设置）
2. 验证重生点是否有效（使用 SpawnPointValidator）
3. 如果无效，清除重生点并回退到世界出生点
4. 使用世界出生点作为最终备选

## 网络同步

SleepPacket 用于同步玩家的睡眠状态：
- 睡眠时发送带有床位置的包
- 唤醒时发送不带床位置的包

## 依赖关系

- `IWorld` - 世界接口，用于获取方块状态和实体
- `BlockPos` - 方块位置
- `Direction` - 方向枚举
- `WeatherConstants` - 天气时间常量
- `MonsterEntity` - 怪物实体基类
- `DimensionType` - 维度类型
- `BlockState` - 方块状态
- `Properties` - 方块属性（BED_PART, CHARGES_0_4 等）

## 使用示例

```cpp
#include "entity/player/SleepManager.hpp"
#include "entity/player/SleepResult.hpp"

// 检查是否可以睡眠
bool canSleep = SleepManager::canSleepAtTime(dayTime, isThundering, isRaining);

// 检查玩家是否在床附近
if (SleepManager::isPlayerNearBed(playerPos, bedPos)) {
    // 尝试让玩家睡眠
    SleepResult result = player.trySleep(bedPos);
    if (result != SleepResult::OK) {
        player.sendSystemMessage(getSleepResultMessage(result));
    }
}
```

```cpp
#include "entity/player/SpawnPointValidator.hpp"

// 验证重生点
auto spawnPoint = player.getSpawnPoint();
if (spawnPoint.has_value()) {
    SpawnPointValidationResult result = SpawnPointValidator::validate(
        *world, spawnPoint.value(), player.isSpawnForced(), true);
    
    if (result == SpawnPointValidationResult::Valid) {
        // 查找安全生成位置
        auto safePos = SpawnPointValidator::findSafeSpawnPosition(
            *world, spawnPoint.value(), player.isSpawnForced(), true);
        if (safePos.has_value()) {
            // 在安全位置重生
        }
    } else {
        // 重生点无效，回退到世界出生点
        player.clearSpawnPoint();
    }
}
```

## 注意事项

1. 时间范围遵循 MC 1.16.5 标准
2. 怪物检测范围是床周围 8x5x8 区域
3. 起床位置优先级：床头前方 > 床尾前方 > 床两侧 > 床上方
4. 全员睡眠需要在 ServerWorld.tick() 中通过 checkSleepStatus() 触发
5. 重生点验证需要访问对应维度的世界，如果世界不存在则回退到世界出生点
6. 床只在主世界有效（bedWorks() == true）
7. 重生锚只在下界有效（respawnAnchorWorks() == true）

## 测试

测试文件位于 `tests/common/entity/player/`：
- `CooldownTrackerTest.cpp` - 物品冷却追踪器测试（20个测试用例）
- `SleepManagerTest.cpp` - 睡眠管理器测试
- `SpawnPointValidatorTest.cpp` - 重生点验证器测试

覆盖：
- 冷却追踪器：设置/移除冷却、tick更新、进度计算、多物品独立冷却、覆盖设置、典型场景（紫颂果、盾牌）
- 时间检测逻辑
- 距离检测逻辑
- 睡眠结果消息映射
- 重生点验证结果枚举
- 床/重生锚属性检测
- 维度类型验证
- GlobalPos 和 BlockPos 操作
