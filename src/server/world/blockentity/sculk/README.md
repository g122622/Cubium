# 幽匿方块实体服务端逻辑 (Sculk Server Logic)

服务端专属的幽匿方块实体逻辑，包括振动系统集成和幽匿尖啸体激活流程。

## 文件结构

```
sculk/
├── README.md                    # 本文档
├── SculkVibrationSystem.hpp     # 幽匿振动系统附件（SculkSensorVibrationUser、SculkShriekerVibrationUser、SculkVibrationSystem、SculkVibrationManager）
├── SculkVibrationSystem.cpp     # 振动系统集成实现
├── SculkShriekerHelper.hpp      # 幽匿尖啸体服务端逻辑辅助类
└── SculkShriekerHelper.cpp      # 幽匿尖啸体服务端逻辑实现
```

## 架构概览

### SculkVibrationSystem

由于方块实体位于 mc_common（不能依赖 mc_server），而 VibrationSystem::Listener 的 `handleGameEvent()` 需要依赖 mc_server 中的代码，因此 VibrationSystem 的集成采用"附件"模式：

- **mc_common 层**：SculkSensorBlockEntity / SculkShriekerBlockEntity 持有 VibrationSystem::Data，提供序列化/反序列化，但不持有 Listener 或 User
- **mc_server 层**：SculkVibrationSystem（VibrationSystem 子类）持有 User 和 Listener，通过引用访问方块实体的 Data；SculkVibrationManager 管理附件的生命周期

#### SculkSensorVibrationUser vs SculkShriekerVibrationUser

| 特性 | SculkSensorVibrationUser | SculkShriekerVibrationUser |
|------|--------------------------|---------------------------|
| 检测半径 | 8 格 | 8 格 |
| 响应事件 | 所有频率 > 0 的事件 | 仅 SHRIEK 事件 |
| canTriggerAvoidVibration | true（潜行可触发规避成就） | false |
| isSculkShrieker | false | true |
| onReceiveVibration | 更新频率、计算红石信号、调用 SculkSensorBlock::activate() | 调用 SculkShriekerHelper::tryShriek() |

#### SculkVibrationManager 生命周期

在 ServerWorld 中的三个关键时机介入：
1. `ServerWorld::setBlockEntity()` → 创建 VibrationSystem 附件，注册 Listener 到 GameEventListenerRegistry
2. `ServerWorld::tickBlockEntities()` → 驱动 VibrationSystem::Ticker::tick()，检查尖啸体 SHRIEKING 结束标志
3. `ServerWorld::removeBlockEntity()` → 注销 Listener，销毁附件

### SculkShriekerHelper

幽匿尖啸体的服务端逻辑辅助类，所有方法均为静态方法。由于这些逻辑依赖 ServerWorld（玩家查找、实体搜索、效果应用等），不能放在 mc_common 层。

#### 核心流程

```
实体踩上尖啸体 / 接收到 SHRIEK 振动
    ↓
SculkShriekerBlock::onEntityWalk() → 发出 SHRIEK 游戏事件
    ↓
振动传播到附近尖啸体
    ↓
SculkShriekerVibrationUser::onReceiveVibration() → SculkShriekerHelper::tryShriek()
    ↓
tryShriek():
    1. 解析触发实体为玩家（tryGetPlayer）
    2. 检查 SHRIEKING 状态（不能重复激活）
    3. 检查旁观者模式
    4. 重置警告等级
    5. _canRespond() 为 true 时调用 _tryWarn() 递增警告等级
    6. 执行 shriek()（设置 SHRIEKING 状态、播放粒子、发出 SHRIEK 事件）
    ↓
90 tick 后 SHRIEKING 状态到期
    ↓
SculkShriekerBlock::tick() → 设置 shriekingFinished 标志
    ↓
SculkVibrationManager::tickAll() → SculkShriekerHelper::checkShriekingFinished()
    ↓
SculkShriekerHelper::tryRespond():
    1. _canRespond() 检查（CAN_SUMMON + 非和平 + 游戏规则）
    2. 警告等级 >= 4 时尝试 _trySummonWarden()
    3. 未召唤监守者时播放 _playWardenReplySound()
    4. _applyDarknessAround() 对附近玩家施加黑暗效果
```

#### 方法说明

| 方法 | 说明 |
|------|------|
| tryShriek() | 尝试激活幽匿尖啸体 |
| tryRespond() | 尖啸结束后的响应 |
| checkShriekingFinished() | 检查尖啸结束标志 |
| _canRespond() | 检查 CAN_SUMMON + 非和平 + 游戏规则 |
| _trySummonWarden() | 在附近寻找有效位置召唤监守者 |
| _hasNearbyWarden() | 48 格范围内搜索监守者 |
| _tryWarn() | 递增附近 16 格内玩家的警告等级 |
| _playWardenReplySound() | 根据警告等级播放监守者回应声音 |
| _applyDarknessAround() | 对 40 格内玩家施加 260 tick 黑暗效果 |
| tryGetPlayer() | 将触发实体解析为玩家 |

#### 关键常量

| 常量 | 值 | 说明 |
|------|-----|------|
| WARDEN_SEARCH_RADIUS | 48.0 | 附近监守者搜索半径 |
| PLAYER_SEARCH_RADIUS | 16.0 | 附近玩家搜索半径（_tryWarn） |
| DARKNESS_RADIUS | 40.0 | 黑暗效果应用半径 |
| DARKNESS_DURATION | 260 | 黑暗效果持续时间（tick） |
| DARKNESS_COOLDOWN | 200 | 黑暗效果冷却（tick） |
| SUMMON_ATTEMPTS | 20 | 监守者生成尝试次数 |
| SUMMON_HORIZONTAL_RANGE | 5 | 生成水平偏移范围 |
| SUMMON_VERTICAL_RANGE | 6 | 生成垂直偏移范围 |

#### 警告等级声音映射

| 等级 | 声音事件 |
|------|---------|
| 0 | 无声音 |
| 1 | entity.warden.nearby_close |
| 2 | entity.warden.nearby_closer |
| 3 | entity.warden.nearby_closest |
| 4 | entity.warden.listening_angry |

## 待实现

- tryGetPlayer() 中载具乘客、投射物主人、物品主人解析（依赖未来子系统完善）
- _trySummonWarden() 中监守者实体类型未注册时的优雅跳过（依赖 WardenEntity 实现）
