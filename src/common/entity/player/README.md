# 玩家相关模块

本目录包含玩家睡眠系统和物品冷却追踪的核心组件。

## 目录结构

```
src/common/entity/player/
├── CooldownTracker.hpp     # 物品冷却追踪器（紫颂果、盾牌等）
├── CooldownTracker.cpp     # 冷却追踪器实现
├── SleepResult.hpp         # 睡眠结果枚举定义
├── SleepManager.hpp        # 睡眠管理器（静态工具类）
├── SleepManager.cpp        # 睡眠管理器实现
└── README.md               # 本文档
```

## 内部模块关系

```
SleepResult.hpp          ← 独立枚举定义
      ↓
SleepManager.hpp         ← 使用 SleepResult，起床位置委托 BedBlock/DismountHelper

CooldownTracker.hpp      ← 独立冷却追踪
```

各组件相互独立，无内部依赖关系。重生点数据与验证在 `common/world/spawn/`（`RespawnData`/`RespawnConfig`），
落点搜索在 `common/entity/vehicle/DismountHelper`。

## 上下游外部依赖关系

### 上游依赖（本目录依赖的模块）

- `common/core/Types.hpp` - 基础类型定义（i32, f32, u8 等）
- `common/util/Direction.hpp` - 方向枚举（用于床朝向）
- `common/util/math/Vector3.hpp` - 3D 向量（位置计算）
- `common/world/GlobalPos.hpp` - 全局位置（维度 + 坐标）
- `common/world/block/BlockPos.hpp` - 方块位置
- `common/world/IWorld.hpp` - 世界接口（获取方块状态、实体）
- `common/item/Item.hpp` - 物品基类（冷却追踪）

### 下游依赖（依赖本目录的模块）

- `entity/entities/player/Player.hpp` - 玩家实体（使用 CooldownTracker、睡眠功能）
- `server/player/ServerPlayer.hpp` - 服务端玩家（睡眠与重生点设置）
- `server/world/ServerWorld.hpp` - 服务端世界（全员睡眠检测）
- `world/block/blocks/functional/BedBlock.cpp` - 床方块（睡眠检测）
- `network/ir/packets/play/` - 状态与冷却组同步

## 容易踩的坑

### 1. 冷却进度方向

冷却进度值范围是 `0.0 ~ 1.0`，但语义容易混淆：
- **1.0** 表示冷却刚开始（物品刚使用）
- **0.0** 表示冷却结束（物品可用）
- 客户端渲染时使用 `(1.0 - progress)` 显示冷却动画

### 2. 睡眠时间范围

睡眠时间检测遵循 MC 1.16.5 标准：
- 雷暴时：任何时间可睡眠
- 降雨时：12010 - 23991 ticks
- 晴天时：12542 - 23459 ticks

### 3. 床验证距离

玩家必须在床附近才能睡眠，有效范围是**水平 3 格、垂直 2 格**。怪物检测范围是床周围 **8×5×8** 区域。

### 4. 重生点维度限制

- **床**：只在主世界有效（`bedWorks() == true`）
- **重生锚**：只在下界有效（`respawnAnchorWorks() == true`）

维度判定发生在重生时；设置重生点本身不校验维度（与原版一致）。

### 5. 全员睡眠触发

全员睡眠检测需要在 `ServerWorld.tick()` 中调用 `checkSleepStatus()` 触发，睡眠计时器需 >= 100 ticks（5秒）才算完全入睡。

### 6. 重生点有效性在重生时判定

玩家重生点配置（`RespawnConfig`）只保存维度/方块位置/朝向/是否强制，不校验方块是否仍然有效。
床或重生锚被破坏后，配置会保留到下一次重生：届时 `ServerPlayer::findRespawnPositionAndUseSpawnBlock()`
找不到有效落点会返回 `missingRespawnBlock`，调用方清除配置并回退到共享出生点。

### 7. 起床位置算法

起床位置查找委托给 `BedBlock::findStandUpPosition()`：
- 根据床朝向的顺时针方向与实体 yaw 决定优先搜索侧
- 生成 12 个候选位置（10 个周围位置 + 2 个床上位置），每个候选交给
  `DismountHelper::findSafeDismountLocation()` 判定地面高度、碰撞、危险方块与世界边界
- 先安全模式（拒绝火/岩浆/仙人掌等危险方块与悬空），全部失败后允许不安全位置
- 双层床（下方一格也是床）会额外搜索下层
- **找不到任何可行位置时返回空**，不会回退到床内；调用方（起床/重生）需自行处理

### 冷却组与通知

冷却按 ResourceLocation 组名共享，物品默认使用其注册名。ServerPlayer 通过监听器发送 cooldown；到期与主动移除发送 duration=0。计时采用 64 位，避免最大 VarInt 持续时间在累加时溢出。测试物品也必须有独立注册名，不能依赖未注册对象的地址区分冷却。
