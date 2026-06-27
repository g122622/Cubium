# 坐标参数系统 (Coordinate Arguments)

本目录实现了 Minecraft 命令系统中的坐标参数类型，支持三种坐标格式：

- **绝对坐标**: `100 64 -200` — 直接使用输入值
- **相对坐标**: `~ ~ ~` / `~1 ~-2 ~5` — 以执行者位置为基准的偏移
- **局部坐标**: `^ ^ ^` / `^1 ^2 ^3` — 以执行者视线方向为基准的偏移

对应 MC Java 的 `net.minecraft.commands.arguments.coordinates` 包。

## 目录结构

```
coordinates/
├── Coordinates.hpp       - 坐标接口（统一抽象）
├── WorldCoordinate.hpp   - 单坐标分量（绝对/相对）
├── WorldCoordinates.hpp  - 世界坐标容器（绝对/相对坐标）
├── LocalCoordinates.hpp  - 局部坐标容器（^ ^ ^ 坐标）
└── README.md             - 本文档
```

## 设计

### Coordinates 接口

所有坐标类型实现 `Coordinates` 接口，提供两个核心方法：
- `getPosition(source)` — 根据命令源上下文计算最终世界坐标
- `getRotation(source)` — 根据命令源上下文计算最终旋转角

### WorldCoordinate（单分量）

存储单个坐标分量的值和类型标志：
- `isRelative()` — 是否为相对坐标（`~` 前缀）
- `getValue()` — 获取原始偏移值
- `get(baseValue)` — 相对时返回 `value + baseValue`，绝对时返回 `value`

### WorldCoordinates（绝对/相对坐标）

三个 `WorldCoordinate` 分量 (x, y, z) 组成。`getPosition()` 将各分量与命令源位置相加（相对时）或直接使用（绝对时）。

### LocalCoordinates（局部坐标）

三个 `f64` 分量 (left, up, forwards) 组成。`getPosition()` 基于 MC Java 的旋转变换公式将局部坐标转换为世界坐标：
1. 从执行者的 yaw/pitch 构建三个基向量（forward, up, left）
2. 将局部坐标的三个分量作为权重加权组合三个基向量
3. 加上锚点位置（feet/eyes）

## 与 MC Java 的对应关系

| MC Java 类 | 本项目类 | 说明 |
|-----------|---------|------|
| `Coordinates` | `Coordinates` | 接口，定义 getPosition/getRotation |
| `WorldCoordinate` | `WorldCoordinate` | 单坐标分量 |
| `WorldCoordinates` | `WorldCoordinates` | 绝对/相对坐标 |
| `LocalCoordinates` | `LocalCoordinates` | 局部坐标 (^ ^ ^) |
| `EntityAnchorArgument.Anchor` | `EntityAnchorType` | 锚点类型 (Feet/Eyes) |

## 使用方式

命令参数类型（Vec3ArgumentType、BlockPosArgumentType 等）解析时返回 `Coordinates` 对象，命令执行时调用 `getPosition(source)` 获取最终坐标：

```cpp
// 命令回调中
auto coords = context.getArgument<Coordinates::Ptr>("pos");
Vector3d position = coords->getPosition(source);
Vector3i blockPos = BlockPos(position.x, position.y, position.z);
```

## 坐标类型混合规则

MC Java 不允许混合使用不同类型的坐标前缀：
- `~10 ~5 30` — 允许（相对 + 相对 + 绝对）
- `^ ^ ^5` — 允许（局部 + 局部 + 局部）
- `~10 ^5 30` — 禁止（不允许混合相对和局部坐标）

解析时如果检测到混合类型，会抛出 `ERROR_MIXED_TYPE` 异常。

## centerCorrect 行为

Vec3ArgumentType 的 `centerCorrect` 选项（默认 true）控制绝对整数坐标是否加 0.5 偏移到方块中心：
- `10` → `10.5`（centerCorrect=true 时绝对整数加 0.5）
- `10.0` → `10.0`（含小数点时不加偏移）
- `~10` → `sourcePos.x + 10`（相对坐标不受影响）
