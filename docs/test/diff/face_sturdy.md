# 差分能力：`face_sturdy`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

**首个系统性暴露 `getBlockSupportShape` 缺失的能力。** 20988 个单元
（1166 个方块 × 6 方向 × 3 种 `SupportType`），初次运行 **625 处差异**。

本轮修复 2 处后降到 **421 处**，仍有 80 个方块受影响——**本能力目前是「部分通过」状态**。

## 测的是什么

```java
// BlockBehaviour.BlockStateBase
public boolean isFaceSturdy(BlockGetter level, BlockPos pos, Direction dir, SupportType type) {
    return this.cache != null
        ? this.cache.isFaceSturdy(dir, type)
        : type.isSupporting(this.asState(), level, pos, dir);
}
```

三种 `SupportType`：

```java
public enum SupportType {
    FULL   { return Block.isFaceFull(state.getBlockSupportShape(level, pos), dir); },
    CENTER { return !Shapes.joinIsNotEmpty(faceShape, Block.column(2.0, 0.0, 10.0), ONLY_SECOND); },
    RIGID  { return !Shapes.joinIsNotEmpty(faceShape,
                 Shapes.join(Shapes.block(), Block.column(12.0, 0.0, 16.0), ONLY_FIRST),
                 ONLY_SECOND); }
}
```

| 类型 | 需要覆盖的区域 | 典型用途 |
|---|---|---|
| `FULL` | 整个 1×1 面 | 火把、灯笼 |
| `CENTER` | 中心 2×2 像素柱（高 0–10）| 钟、悬挂告示牌 |
| `RIGID` | 外环（1×1 去掉中心 12×12）| 铁轨、压力板 |

**关键**：三者都用 `getBlockSupportShape`，**不是** `getCollisionShape`。默认实现让支撑形状等于碰撞形状，
所以**碰撞形状写错会连带污染支撑判定**。

这是所有附着型方块存活判定的公共底层：

```java
// 原版 AmethystClusterBlock
Direction direction = state.getValue(FACING);
BlockPos attached = pos.relative(direction.getOpposite());
return level.getBlockState(attached).isFaceSturdy(level, attached, direction);
```

紫水晶簇、发光地衣、幽匿脉络、珊瑚、告示牌、火把、梯子——它们的 `canSurvive` 最终都落到这里。

## 为什么不用 `block_support` 能力

现有的 [`block_support`](./block_support.md) 测的是 `isValidPosition`（即 `canSurvive`），
它混入了两类语义：

1. `canSurvive`（方块自身能否存活）
2. `canPlace`（放置时的额外约束）

`WallHangingSignBlock` 根本没有覆写 `canSurvive`，它的结果来自 `canPlace`，于是产生假差异。
本能力只测**最底层的面朝支撑判定**，是纯计算、无歧义：

- 只查方块自身形状
- 不需要构造邻域
- 不需要 bootstrap 以外的任何状态

## 本轮修复的 2 处缺陷

### 缺陷一：`LeavesBlock` 缺少 `getBlockSupportShape` 覆写

```java
// 原版 LeavesBlock.java
@Override
protected VoxelShape getBlockSupportShape(BlockState state, BlockGetter level, BlockPos pos) {
    return Shapes.empty();
}
```

**碰撞形状是完整方块，但支撑形状恒为空。**

Cubium 完全没有覆写，于是继承默认实现（支撑形状 = 碰撞形状 = 完整方块），
导致**任何附着型方块都能贴在树叶上**。

修复：

```cpp
// LeavesBlock.hpp
[[nodiscard]] const CollisionShape& getBlockSupportShape(const BlockState& state) const override;

// LeavesBlock.cpp
const CollisionShape& LeavesBlock::getBlockSupportShape(const BlockState& state) const
{
    // 对齐原版：树叶恒返回空支撑形状。碰撞形状虽为完整方块，但不提供任何可附着的面。
    (void)state;
    static const CollisionShape empty = CollisionShape::empty();
    return empty;
}
```

效果：减少 **198 处差异**（427 → 之后又修蜂蜜块，最终 421）。

### 缺陷二：`HoneyBlock` 碰撞箱横向满宽

```java
// 原版 HoneyBlock.java
private static final VoxelShape SHAPE = Block.column(14.0, 0.0, 15.0);
```

`Block.column(radius, yMin, yMax)` 生成**横向 `radius/16`、纵向 `yMin..yMax`** 的柱子：

| 轴 | 范围 |
|---|---|
| `x` | `1/16 .. 15/16`（**各留 1 像素**）|
| `y` | `0 .. 15/16` |
| `z` | `1/16 .. 15/16`（**各留 1 像素**）|

Cubium 修复前：

```cpp
m_collisionShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);
//                                      ^^^^^^^^^^^^^^^^^^^^^^  ^^^^
//                                      横向 0..1 满宽            纵向正确
```

**纵向对了，横向写成了满宽。** 因为 `getBlockSupportShape` 默认返回碰撞形状，
四个侧面被误判为「有坚固面」——火把等附着方块能贴在蜂蜜块侧面。

修复：

```cpp
m_collisionShape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.9375f, 0.9375f);
```

效果：减少 **6 处差异**（蜂蜜块的 6 个方向判定全部纠正）。

## 尚未修复：剩余 421 处差异

仍有 **80 个方块**受影响。差异分布：

| `SupportType` | 差异数 |
|---|---|
| `CENTER` | 132 |
| `FULL` | 34 |
| `RIGID` | 34 |

### 已定位的两个根因

**一、原版有 8 个方块覆写了 `getBlockSupportShape`，Cubium 缺 5 个。**

| 原版类 | Cubium 状态 | 原版实现 |
|---|---|---|
| `LeavesBlock` | ✅ 本轮修复 | `Shapes.empty()` |
| `FenceGateBlock` | ❌ **缺失** | 关闭时返回门板形状，打开时 `Shapes.empty()` |
| `ShulkerBoxBlock` | ❌ **缺失** | 关闭时 `Shapes.block()`，打开时返回倾斜形状 |
| `CeilingHangingSignBlock` | ❌ **缺失** | `this.getShape(...)` |
| `WallHangingSignBlock` | ❌ **缺失** | `this.getShape(...)` |
| `MudBlock` | ✅ 有覆写 | — |
| `SoulSandBlock` | ✅ 有覆写 | — |
| `SnowLayerBlock` | ❌ **文件未找到** | 按层数 |

**二、部分方块的碰撞形状与原版不符。**

例如 `DriedKelpBlock`：原版 `FULL=true`，Cubium `FULL=false`——Cubium 的碰撞形状比完整方块小，
但原版没有覆写（应当是全方块）。

### 下一步方向

1. 补齐 `FenceGateBlock` / `ShulkerBoxBlock` / 悬挂告示牌的 `getBlockSupportShape`；
2. 逐个核对剩余 80 个方块的碰撞形状是否与原版一致；
3. 全部修完后本能力应能跑通。

**本能力维持「部分通过」状态是有意的**——它已经把根因从「625 处差异」收敛到
「5 个缺失覆写 + 若干碰撞形状不符」，后续修复有明确方向。

## 单元格式

`<block>|<Direction>|<SupportType>`

```
minecraft:oak_stairs|UP|FULL
minecraft:oak_stairs|UP|CENTER
minecraft:oak_stairs|UP|RIGID
```

- 方块用注册名（`minecraft:xxx`），两侧都枚举注册表后**按名字排序**
- 方向用 Java 常量名（`DOWN`/`UP`/`NORTH`/`SOUTH`/`WEST`/`EAST`）
- 支撑类型用 `FULL`/`CENTER`/`RIGID`

## 踩坑：`IWorld` 有 22 个纯虚函数

`isFaceSturdy` 只需要 `getBlockState`，但 `IWorld` 要求实现全部 22 个纯虚方法：

```
currentTick  dayTime  difficulty  dimension  getBlockCollisions  getBlockLight
getEntitiesInAABB  getEntitiesInRange  getEntityCollisions  getFluidState
getHeight  getRandom  getSkyLight  hasBlockCollision  hasEntityCollision
isClientSide  isHardcore  isWithinWorldBounds  physicsEngine  seed
setBlockState  tickManager  worldBorder
```

**解决**：直接复用 [`block_support`](./block_support.md) 探针里的 `SupportWorld` 实现并重命名。

另外 `IBlockReader` 是 `IWorld` 的**别名**（`using IWorldReader = IWorld;`），
定义在 `IWorld.hpp` 里，**不存在独立的 `IBlockReader.hpp`**。

## 踩坑：`SupportType` 的包路径

```java
import net.minecraft.world.level.block.SupportType;   // ← 正确
import net.minecraft.world.level.block.state.properties.SupportType;   // ✗ 不存在
```

`SupportType` 在 `block` 包下，**不在** `state.properties` 下——与大部分 `EnumProperty` 的枚举不同。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`block_support`](./block_support.md) | 测 `canSurvive`（含 `canPlace` 噪声）；本能力是它的**底层** |
| [`shapes_boolean`](./shapes_boolean.md) | `VoxelShape` 的布尔运算 |
| [`block_state_names`](./block_state_names.md) | 方块状态名 |
| [`enum_property_order_all`](./enum_property_order_all.md) | 属性值顺序 |

## 未覆盖范围

- 非默认状态的方块（本能力只测 `defaultBlockState()`）
- `getBlockSupportShape` 依赖上下文的方块（如 `ShulkerBoxBlock` 需要查方块实体）
- `canSurvive` / `canPlace` 的完整判定（那些由 `block_support` 覆盖）
- 具体形状的几何数值（本能力只比布尔结果）