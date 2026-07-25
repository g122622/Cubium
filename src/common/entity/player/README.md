# 玩家相关模块

本目录包含玩家睡眠系统、重生点验证和物品冷却追踪的核心组件。

## 目录结构

```
src/common/entity/player/
├── CooldownTracker.hpp     # 物品冷却追踪器（紫颂果、盾牌等）
├── CooldownTracker.cpp     # 冷却追踪器实现
├── SleepResult.hpp         # 睡眠结果枚举定义
├── SleepManager.hpp        # 睡眠管理器（静态工具类）
├── SleepManager.cpp        # 睡眠管理器实现
├── SpawnPointValidator.hpp # 重生点验证器（静态工具类）
├── SpawnPointValidator.cpp # 重生点验证器实现
└── README.md               # 本文档
```

## 内部模块关系

```
SleepResult.hpp          ← 独立枚举定义
      ↓
SleepManager.hpp         ← 使用 SleepResult
      ↓
SpawnPointValidator.hpp  ← 独立验证工具

CooldownTracker.hpp      ← 独立冷却追踪
```

各组件相互独立，无内部依赖关系。

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
- `server/player/ServerPlayer.hpp` - 服务端玩家（重生点验证）
- `server/world/ServerWorld.hpp` - 服务端世界（全员睡眠检测）
- `world/block/blocks/functional/BedBlock.cpp` - 床方块（睡眠检测）
- `network/codec/Packet.hpp` - 睡眠状态网络同步（Sleep 包 ID 231，S→C）

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

### 5. 全员睡眠触发

全员睡眠检测需要在 `ServerWorld.tick()` 中调用 `checkSleepStatus()` 触发，睡眠计时器需 >= 100 ticks（5秒）才算完全入睡。

### 6. 重生点验证的 World 参数

`SpawnPointValidator::validate()` 需要传入对应维度的世界对象，如果世界不存在会返回 `WorldNotFound`，调用方需处理回退到世界出生点的逻辑。

### 7. 起床位置算法

起床位置查找委托给 `BedBlock::findStandUpPosition()`，算法与 MC 原版一致：
- 根据床朝向计算顺时针方向，再根据实体 yaw 偏航角决定优先搜索左侧还是右侧
- 生成 12 个候选位置（10 个周围位置 + 2 个床上方位置）
- 支持双层床检测（床下方一格也有床时搜索下层）
- 优先搜索安全位置（避开液体），然后回退到不安全位置
- 最终回退到床头正上方

`SleepManager::findWakeUpPosition()` 内部委托给 `BedBlock::findStandUpPosition()`，保持向后兼容。
