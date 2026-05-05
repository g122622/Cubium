# PortalSize 传送门检测算法对齐报告

## 概述

本报告对比 Minecraft Reborn 项目中的 `PortalSize` 实现与 MC 1.16.5 原版 `PortalSize.java` 的算法差异。

---

## 1. 传送门尺寸常量

| 常量 | MC 1.16.5 | 我们的实现 | 状态 |
|------|-----------|-----------|------|
| MIN_WIDTH | 硬编码 `i >= 2` | `MIN_WIDTH = 2` | 一致 |
| MAX_WIDTH | 硬编码 `i <= 21` | `MAX_WIDTH = 21` | 一致 |
| MIN_HEIGHT | 硬编码 `i >= 3` | `MIN_HEIGHT = 3` | 一致 |
| MAX_HEIGHT | 硬编码 `i <= 21` | `MAX_HEIGHT = 21` | 一致 |
| 向下搜索 | `pos.getY() - 21` | `MAX_SEARCH_DOWN = 21` | 一致 |

**结论：常量定义一致。**

---

## 2. findNetherPortal 搜索顺序

### MC 1.16.5 实现（行 36-44）
```java
public static Optional<PortalSize> func_242965_a(...) {
   Optional<PortalSize> optional = Optional.of(new PortalSize(..., axis)).filter(predicate);
   if (optional.isPresent()) {
      return optional;
   } else {
      Direction.Axis opposite = axis == X ? Z : X;
      return Optional.of(new PortalSize(..., opposite)).filter(predicate);
   }
}
```

### 我们的实现（行 38-58）
```cpp
Direction firstDir = preferXAxis ? Direction::East : Direction::South;
auto result = tryFindPortalOnAxis(world, pos, firstDir);
// ...如果失败，尝试另一个方向
```

### 差异分析

**MC 1.16.5 轴向映射（构造函数行 49）：**
```java
this.rightDir = axisIn == Direction.Axis.X ? Direction.WEST : Direction.SOUTH;
```

| 输入轴 | MC rightDir | 我们 preferXAxis=true 时的 firstDir |
|--------|-------------|-------------------------------------|
| X 轴 | WEST | East |
| Z 轴 | SOUTH | South |

**差异：** 方向语义相反（WEST vs East），但这是对称的，不影响传送门检测正确性。

**结论：搜索顺序逻辑一致。**

---

## 3. findBottomLeft 向下搜索逻辑

### MC 1.16.5 实现（行 65-72）
```java
private BlockPos func_242971_a(BlockPos pos) {
   // 步骤1：向下搜索
   for(int i = Math.max(0, pos.getY() - 21); 
       pos.getY() > i && canConnect(world.getBlockState(pos.down())); 
       pos = pos.down()) {
   }

   // 步骤2：向左搜索框架
   Direction direction = this.rightDir.getOpposite();
   int j = this.func_242972_a(pos, direction) - 1;
   return j < 0 ? null : pos.offset(direction, j);
}
```

### 我们的实现（行 124-158）
```cpp
std::optional<BlockPos> PortalSize::findBottomLeft(...) {
    // 步骤1：向下搜索
    while (currentPos.y > minY) {
        BlockPos belowPos = currentPos.offset(Direction::Down);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !canConnect(*belowState)) break;
        currentPos = belowPos;
    }

    // 步骤2：向左搜索框架
    i32 leftDistance = 0;
    for (i32 i = 0; i <= MAX_WIDTH + 1; ++i) {
        BlockPos checkPos = currentPos.offset(leftDir, i);
        const BlockState* state = world.getBlockState(checkPos);
        if (state != nullptr && isPortalFrame(*state)) {
            leftDistance = i;  // 问题：记录所有框架
        } else {
            break;  // 问题：遇到非框架立即停止
        }
    }
    // ...
}
```

### 关键差异分析

**MC 的 `func_242972_a`（行 79-98）完整逻辑：**
```java
for(int i = 0; i <= 21; ++i) {
   BlockState state = world.getBlockState(pos.offset(dir, i));
   if (!canConnect(state)) {
      if (isPortalFrame(state)) return i;  // 找到框架，返回距离
      break;  // 非框架非连接，停止
   }
   // 是连接方块，检查底部框架
   BlockState below = world.getBlockState(pos.offset(dir, i).down());
   if (!isPortalFrame(below)) break;  // 底部不是框架，停止
}
return 0;  // 未找到框架
```

**差异对比表：**

| 方面 | MC 1.16.5 | 我们的实现 | 影响 |
|------|-----------|-----------|------|
| 遇到连接方块（空气） | 检查底部框架，继续 | break（停止） | **严重** |
| 遇到框架方块 | 返回当前距离 | 记录距离，继续 | **严重** |
| 底部框架检查 | 每步都检查 | 无 | **严重** |

**问题示例：**

假设传送门内部为空气，左边有黑曜石框架：
```
[黑曜石][空气][空气][当前位置]
   ^
   框架
```

- **MC 行为：**
  - i=0：当前位置是空气 → canConnect=true → 检查底部框架 → 继续下一个 i
  - i=1：空气 → 继续下一个 i
  - i=2：黑曜石 → canConnect=false → isPortalFrame=true → 返回 2
  - bottomLeft = currentPos.offset(left, 1)

- **我们的实现：**
  - i=0：当前位置是空气 → isPortalFrame=false → **break**
  - leftDistance = 0 → 返回 nullopt

**结论：存在严重 bug，会导致传送门检测失败。**

---

## 4. calculateWidth 计算宽度

### MC 1.16.5 实现（行 74-77, 79-98）
```java
private int func_242974_d() {
   int i = this.func_242972_a(this.bottomLeft, this.rightDir);
   return i >= 2 && i <= 21 ? i : 0;
}
```

### 我们的实现（行 160-181）
```cpp
i32 PortalSize::calculateWidth(...) {
    for (i32 i = 0; i <= MAX_WIDTH; ++i) {
        BlockPos checkPos = bottomLeft.offset(rightDir, i);
        const BlockState* state = world.getBlockState(checkPos);

        if (state == nullptr) return 0;

        if (!canConnect(*state)) {
            if (isPortalFrame(*state)) return i;
            return 0;
        }

        BlockPos belowPos = checkPos.offset(Direction::Down);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !isPortalFrame(*belowState)) return 0;
    }
    return 0;
}
```

### 对比分析

| 方面 | MC 1.16.5 | 我们的实现 | 状态 |
|------|-----------|-----------|------|
| 循环范围 | `i <= 21` | `i <= MAX_WIDTH (21)` | 一致 |
| 检查连接方块 | `!canConnect()` 分支 | `!canConnect()` 分支 | 一致 |
| 检查框架方块 | `field_242962_a.test()` | `isPortalFrame()` | 一致 |
| 检查底部框架 | 每步检查 | 每步检查 | 一致 |
| null 检查 | 无 | 有 | 合理扩展 |

**结论：calculateWidth 实现一致。**

---

## 5. calculateHeight 计算高度

### MC 1.16.5 实现（行 101-104, 118-143）
```java
private int func_242975_e() {
   int i = this.func_242969_a(mutable);
   return i >= 3 && i <= 21 && checkTopFrame ? i : 0;
}

private int func_242969_a(BlockPos.Mutable mutable) {
   for(int i = 0; i < 21; ++i) {
      // 检查左边框架
      mutable.setPos(bottomLeft).move(UP, i).move(rightDir, -1);
      if (!isPortalFrame(state)) return i;
      
      // 检查右边框架
      mutable.setPos(bottomLeft).move(UP, i).move(rightDir, width);
      if (!isPortalFrame(state)) return i;
      
      // 检查内部
      for(int j = 0; j < width; ++j) {
         mutable.setPos(bottomLeft).move(UP, i).move(rightDir, j);
         if (!canConnect(state)) return i;
         if (state.isIn(Blocks.NETHER_PORTAL)) ++portalBlockCount;
      }
   }
   return 21;
}
```

### 我们的实现（行 183-217）
```cpp
i32 PortalSize::calculateHeight(...) {
    for (i32 h = 0; h < MAX_HEIGHT; ++h) {
        // 检查左边框架
        BlockPos leftFramePos = bottomLeft.offset(Direction::Up, h).offset(leftDir);
        if (!isPortalFrame(*leftState)) return h;

        // 检查右边框架
        BlockPos rightFramePos = bottomLeft.offset(Direction::Up, h).offset(rightDir, width);
        if (!isPortalFrame(*rightState)) return h;

        // 检查内部
        for (i32 w = 0; w < width; ++w) {
            BlockPos interiorPos = bottomLeft.offset(Direction::Up, h).offset(rightDir, w);
            if (!canConnect(*interiorState)) return h;
            if (interiorState->is(VanillaBlocks::NETHER_PORTAL)) ++outPortalBlockCount;
        }
    }
    return MAX_HEIGHT;
}
```

### 对比分析

| 方面 | MC 1.16.5 | 我们的实现 | 状态 |
|------|-----------|-----------|------|
| 循环范围 | `i < 21` | `h < MAX_HEIGHT (21)` | 一致 |
| 左边框架 | `move(rightDir, -1)` | `offset(leftDir)` | 一致 |
| 右边框架 | `move(rightDir, width)` | `offset(rightDir, width)` | 一致 |
| 内部检查 | 循环 `j < width` | 循环 `w < width` | 一致 |
| 传送门方块统计 | `isIn(Blocks.NETHER_PORTAL)` | `is(VanillaBlocks::NETHER_PORTAL)` | 一致 |

**结论：calculateHeight 实现一致。**

---

## 6. checkTopFrame 顶部框架验证

### MC 1.16.5 实现（行 107-116）
```java
private boolean func_242970_a(BlockPos.Mutable mutable, int height) {
   for(int i = 0; i < this.width; ++i) {
      mutable.setPos(bottomLeft).move(UP, height).move(rightDir, i);
      if (!isPortalFrame(state)) return false;
   }
   return true;
}
```

### 我们的实现（行 219-235）
```cpp
bool PortalSize::checkTopFrame(...) {
    BlockPos topFramePos = bottomLeft.offset(Direction::Up, height);
    for (i32 w = 0; w < width; ++w) {
        BlockPos pos = topFramePos.offset(rightDir, w);
        if (!isPortalFrame(*state)) return false;
    }
    return true;
}
```

**结论：checkTopFrame 实现一致。**

---

## 7. canConnect 内部方块判断

### MC 1.16.5 实现（行 146-148）
```java
private static boolean canConnect(BlockState state) {
   return state.isAir() || state.isIn(BlockTags.FIRE) || state.isIn(Blocks.NETHER_PORTAL);
}
```

### 我们的实现（行 84-89）
```cpp
bool PortalSize::canConnect(const BlockState& state) {
    if (state.isAir()) return true;
    if (VanillaBlocks::FIRE != nullptr && state.is(VanillaBlocks::FIRE)) return true;
    if (VanillaBlocks::NETHER_PORTAL != nullptr && state.is(VanillaBlocks::NETHER_PORTAL)) return true;
    return false;
}
```

### 差异分析

| 方面 | MC 1.16.5 | 我们的实现 | 状态 |
|------|-----------|-----------|------|
| 空气 | `isAir()` | `isAir()` | 一致 |
| 火焰 | `isIn(BlockTags.FIRE)` | `is(VanillaBlocks::FIRE)` | **潜在差异** |
| 传送门 | `isIn(Blocks.NETHER_PORTAL)` | `is(VanillaBlocks::NETHER_PORTAL)` | 一致 |

**潜在问题：** MC 使用 `BlockTags.FIRE` 标签，包含所有火焰类型（普通火、灵魂火等）。我们只检查 `VanillaBlocks::FIRE`。如果项目有灵魂火等，需要扩展。

**建议：** 使用标签检查或检查所有火焰类型。

---

## 8. isPortalFrame 框架方块判断

### MC 1.16.5 实现
```java
// 通过 BlockState.isPortalFrame 方法
// 默认实现：return state.isIn(Blocks.OBSIDIAN);
```

### 我们的实现（行 91-95）
```cpp
bool PortalSize::isPortalFrame(const BlockState& state) {
    if (VanillaBlocks::OBSIDIAN != nullptr && state.is(VanillaBlocks::OBSIDIAN)) return true;
    return false;
}
```

**结论：isPortalFrame 实现一致。**

---

## 发现的问题汇总

### 问题 1：findBottomLeft 中向左搜索逻辑错误 [严重]

**问题描述：**
- 我们的实现：遇到非框架方块立即 break
- MC 实现：遇到连接方块（空气）时检查底部框架后继续

**影响：**
- 当前位置是传送门内部空气时，向左搜索会立即失败
- 导致传送门检测完全失败

**修复建议：**
```cpp
std::optional<BlockPos> PortalSize::findBottomLeft(
    IWorld& world,
    const BlockPos& pos,
    Direction rightDir)
{
    Direction leftDir = Directions::opposite(rightDir);

    // 步骤1：向下搜索
    BlockPos currentPos = pos;
    i32 minY = std::max(world::MIN_BUILD_HEIGHT, pos.y - MAX_SEARCH_DOWN);
    while (currentPos.y > minY) {
        BlockPos belowPos = currentPos.offset(Direction::Down);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !canConnect(*belowState)) break;
        currentPos = belowPos;
    }

    // 步骤2：向左搜索框架（修复后）
    i32 leftDistance = 0;
    for (i32 i = 0; i <= MAX_WIDTH; ++i) {
        BlockPos checkPos = currentPos.offset(leftDir, i);
        const BlockState* state = world.getBlockState(checkPos);

        if (state == nullptr) {
            return std::nullopt;
        }

        if (!canConnect(*state)) {
            if (isPortalFrame(*state)) {
                leftDistance = i;
                break;  // 找到框架，停止
            }
            return std::nullopt;  // 非框架非连接，无效
        }

        // 是连接方块，检查底部框架
        BlockPos belowPos = checkPos.offset(Direction::Down);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !isPortalFrame(*belowState)) {
            return std::nullopt;  // 底部不是框架，无效
        }
    }

    if (leftDistance <= 0) return std::nullopt;
    return currentPos.offset(leftDir, leftDistance - 1);
}
```

### 问题 2：canConnect 火焰检查不完整 [轻微]

**问题描述：**
- MC 使用 `BlockTags.FIRE` 标签检查所有火焰
- 我们只检查 `VanillaBlocks::FIRE`

**影响：** 如果有灵魂火等其他火焰类型，可能导致检测失败。

**修复建议：**
```cpp
bool PortalSize::canConnect(const BlockState& state) {
    if (state.isAir()) return true;
    // 使用标签检查或检查所有火焰类型
    if (VanillaBlocks::FIRE != nullptr && state.is(VanillaBlocks::FIRE)) return true;
    if (VanillaBlocks::SOUL_FIRE != nullptr && state.is(VanillaBlocks::SOUL_FIRE)) return true;
    if (VanillaBlocks::NETHER_PORTAL != nullptr && state.is(VanillaBlocks::NETHER_PORTAL)) return true;
    return false;
}
```

### 问题 3：向下搜索下限使用 MIN_BUILD_HEIGHT [轻微]

**MC 1.16.5：** `Math.max(0, pos.getY() - 21)`
**我们：** `std::max(world::MIN_BUILD_HEIGHT, pos.y - MAX_SEARCH_DOWN)`

如果 `MIN_BUILD_HEIGHT = 0`，行为一致。未来如果支持负高度建筑，需要确认 MC 行为。

---

## 结论

| 方法 | 状态 | 说明 |
|------|------|------|
| 尺寸常量 | 一致 | |
| findNetherPortal | 一致 | 方向语义差异但不影响正确性 |
| findBottomLeft | **严重 Bug** | 向左搜索逻辑错误 |
| calculateWidth | 一致 | |
| calculateHeight | 一致 | |
| checkTopFrame | 一致 | |
| canConnect | 轻微差异 | 火焰检查不完整 |
| isPortalFrame | 一致 | |

**优先修复：** `findBottomLeft` 中的向左搜索逻辑错误。
