# 玩家实体模块

玩家实体模块负责封装玩家的移动、能力、背包、经验、脚步声和游泳声等核心状态，是客户端主循环和服务端玩家逻辑之间的共同抽象。

## 目录结构

```text
src/common/entity/entities/player/
├── Player.hpp         # 玩家实体声明，包含状态、移动和网络同步接口
├── Player.cpp         # 玩家实体实现，包含物理、脚步声、游泳声和序列化
├── PlayerManager.hpp  # 玩家管理器声明
├── PlayerManager.cpp  # 玩家管理器实现
├── GameModeUtils.hpp  # 游戏模式能力映射工具
├── GameModeUtils.cpp  # 游戏模式能力映射实现
└── README.md          # 本文档
```

## 文件介绍

### Player.hpp

声明 `Player` 类以及玩家专用的状态字段和访问接口，包括：

- 游戏模式和能力标志
- 生命值、饥饿、经验和吸收值
- 物理移动入口、跳跃、游泳和步距统计
- 脚步声和游泳声的触发标志
- 网络同步所需的玩家位置封装

### Player.cpp

实现玩家的核心行为：

- 处理移动输入并写入速度
- 执行物理更新、碰撞和跳跃
- 统计移动距离并生成步脚声/游泳声触发信号
- 序列化和反序列化玩家状态

### PlayerManager.hpp / PlayerManager.cpp

管理玩家对象生命周期、查找和基础集合操作。服务端和客户端都会通过它拿到玩家实体。

### GameModeUtils.hpp / GameModeUtils.cpp

把游戏模式映射成玩家能力配置，避免把创造、旁观、生存等模式逻辑散落在各处。

## 模块关系

- `Player` 继承自 `Entity`，复用通用的位置、旋转、碰撞和数据管理能力。
- `ClientApplication` 使用 `Player` 的移动距离累计值来驱动视野晃动，并读取步脚声/游泳声标志来播放本地音效。
- `NetworkClient` 和玩家序列化逻辑负责把服务器传来的传送、位置和状态同步到本地玩家。
- `PlayerManager` 负责在世界层管理玩家集合，服务端和单机集成都依赖它。
- `GameModeUtils` 负责把游戏模式转换为玩家能力，避免重复实现。

## 整体职责

这个模块的职责是把“玩家”从通用实体里单独抽出来，统一管理和玩家强相关的行为：

1. 处理输入到速度的映射
2. 执行玩家专有的移动与跳跃逻辑
3. 维护脚步声、游泳声和视野晃动所需的统计量
4. 提供背包、经验、游戏模式和能力状态
5. 支持网络同步和传送复位

## 输入 / 输出

### 输入

- 键盘和鼠标输入，驱动 `handleMovementInput()`
- 物理引擎的碰撞和重力结果，驱动 `updatePhysics()`
- 服务器同步的传送、位置和旋转数据
- 游戏模式切换和能力更新
- 背包、经验和状态变化

### 输出

- 玩家位置、速度、旋转和姿态变化
- 步脚声和游泳声触发标志
- 视野晃动累计值
- `network::PlayerPosition` 同步数据
- 序列化后的玩家状态数据

## 依赖项

### 内部依赖

- `entity/core/Entity.hpp`
- `physics/PhysicsEngine.hpp`
- `physics/PhysicsConstants.hpp`
- `inventory/PlayerInventory.hpp`
- `experience/ExperienceManager.hpp`
- `movement/AutoJump.hpp`
- `world/block/BlockPos.hpp`
- `network/packet/ProtocolPackets.hpp`

### 外部依赖

- `spdlog`，用于少量日志输出
- 标准库的 `memory`、`array`、`vector`、`cmath`

## 使用方法

```cpp
using namespace mc;

auto player = std::make_unique<Player>(static_cast<EntityId>(1), "Steve");
player->setGameMode(GameMode::Survival);
player->setOnGround(true);

player->handleMovementInput(1.0f, 0.0f, false, false);
player->updatePhysics();

if (player->shouldPlayStepSound()) {
    // 播放脚步声
}
```

外部改坐标时要通过 `Player::setPosition()`，它会同步重置步距采样和脚步声状态，避免把传送或出生位置当成走路距离。

## 容易踩的坑

- 不要把 `Entity::prevPosition()` 当成脚步声采样位置，它是插值/帧历史状态，不是步距累计基准。
- 不要在外部直接修改玩家位置后继续沿用旧的步距计数，传送和出生都应该重置采样。
- `updateMoveDistance()` 可以在同一帧里被多次调用，但每次都必须只统计“上次采样之后”的增量。
- 视野晃动和脚步声共用同一套移动距离统计，统计语义错了会同时污染音效和镜头。

## 测试用例

- [tests/common/entity/PlayerMovementTest.cpp](../../../../../tests/common/entity/PlayerMovementTest.cpp)
- `UpdateMoveDistance_ResamplesCurrentPosition` 覆盖重复采样和坐标重置的回归场景

## Mermaid 图表

```mermaid
flowchart TD
    Input[移动输入] --> Physics[Player::updatePhysics()]
    Physics --> Sample[Player::updateMoveDistance()]
    Sample --> StepFlag[脚步声/游泳声标志]
    Sample --> Bob[视野晃动累计]

    Teleport[Player::setPosition()] --> Reset[重置步距采样]
    Reset --> Sample

    StepFlag --> App[ClientApplication 播放音效]
    Bob --> Camera[Camera 视角偏移]

    style Input fill:#ffd166,stroke:#b7791f,color:#111
    style Physics fill:#8ecae6,stroke:#1d4ed8,color:#111
    style Sample fill:#90be6d,stroke:#2f6f3e,color:#111
    style StepFlag fill:#f4a261,stroke:#b45309,color:#111
    style Bob fill:#cdb4db,stroke:#6d28d9,color:#111
    style Teleport fill:#f28482,stroke:#b91c1c,color:#111
    style Reset fill:#bde0fe,stroke:#2563eb,color:#111
    style App fill:#e9c46a,stroke:#a16207,color:#111
    style Camera fill:#a8dadc,stroke:#0f766e,color:#111
```