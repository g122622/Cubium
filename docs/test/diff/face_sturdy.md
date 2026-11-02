# 差分能力：`face_sturdy`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

**首个系统性暴露 `getBlockSupportShape` 缺失的能力。** 20988 个单元
（1166 个方块 × 6 方向 × 3 种 `SupportType`），初次运行 **625 处差异**。

| 阶段 | 差异数 |
|---|---|
| 初次运行 | **625** |
| 修复 `LeavesBlock` | 427 |
| 修复 `HoneyBlock` | 421 |
| 修复 `FenceGateBlock` | 397 |
| 修复 `CarpetBlock` / `LadderBlock` / `NetherWartBlock` | **344** |

> **注意**：早期版本本文档曾判断剩余差异的主因是 `VoxelShape` 的归一化缺口。
> 那个判断是**错的**——深入排查后发现真正原因是若干方块把碰撞形状错误地设成了空。
> 详见下方「地毯/梯子/下界疣：碰撞形状被错误设成空」。

仍有 81 个方块受影响——**本能力目前是「部分通过」状态**。
其中 `CarpetBlock` / `LadderBlock` / `NetherWartBlock` 的空碰撞形状已修复，
剩余差异见下方「尚未修复」。

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

## 地毯 / 梯子 / 下界疣：碰撞形状被错误设成空

**这是本轮最有价值的发现，也推翻了本文档早先的一个错误判断。**

### 症状

地毯的 51 处差异（17 种地毯 × 3 种 SupportType）：

```
minecraft:black_carpet|DOWN|FULL:   原版=[true]  Cubium=[false]
minecraft:black_carpet|DOWN|CENTER: 原版=[true]  Cubium=[false]
minecraft:black_carpet|DOWN|RIGID:  原版=[true]  Cubium=[false]
```

### 排查过程

第一步：比较形状定义，发现**两边完全一致**（`(0,0,0)-(1,1/16,1)`），于是怀疑
是 `VoxelShape` 的坐标归一化（`VoxelShape.cpp:66` 的 TODO）导致 `getFaceShape` 切错层。

第二步：写 Java 诊断程序，打印原版实际坐标集：

```
carpet.support = VoxelShape[AABB[0.0, 0.0, 0.0] -> [1.0, 0.0625, 1.0]]
   x=[0.0, 1.0] y=[0.0, 0.0625] z=[0.0, 1.0]
faceShape(DOWN) = VoxelShape[AABB[0.0, 0.0, 0.0] -> [1.0, 1.0, 1.0]]
isFaceFull(DOWN) = true
```

原版每个轴**只有 2 个坐标点**（`[min, max]`），因此 `isCubeLikeAlong` 判定为真、
`getFaceShape` 直接返回自身。而 `findBits` 的循环上限是 `i <= 3`（最多 8 分格），
`1/16 × 8 = 0.5` 不是整数，所以**返回 -1，走 `ArrayVoxelShape` 分支**，坐标集正是 `[min, max]`。

第三步：写 C++ 诊断分支，打印 Cubium 侧的实际坐标点数：

```
minecraft:black_carpet|DOWN|DIAG -> counts:1,1,1 empty=T full=F boxes=0
minecraft:stone|DOWN|DIAG        -> counts:2,2,2 empty=F full=T boxes=1
```

**地毯的支撑形状是 `empty`（空形状），只有 1 个坐标点。** 这与归一化无关——
空形状根本没有几何体。

### 根因

```cpp
// src/common/world/block/blocks/decorative/CarpetBlock.cpp（修复前）
const CollisionShape& CarpetBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 地毯没有碰撞箱（可以穿过）      ← 注释是错的
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}
```

原版 `CarpetBlock` **只覆写 `getShape`，不覆写 `getCollisionShape`**，因此碰撞形状等于
`SHAPE = Block.column(16.0, 0.0, 1.0)`——**1/16 厚的完整薄片**。玩家能站在地毯上。

Cubium 额外覆写并返回空，注释理由「地毯没有碰撞箱（可以穿过）」与事实不符。

**连带后果**：`Block::getBlockSupportShape` 的默认实现转调 `getCollisionShape`，于是地毯的
支撑形状也变成空，`isFaceSturdy` 对地毯的任何面都判 false。

### 同类缺陷还有两个

| 方块 | Cubium（修复前）| 原版 | 本能力差异 |
|---|---|---|---|
| `CarpetBlock` | 返回空 | 不覆写（=getShape）| 51 处 |
| `LadderBlock` | 返回空，注释「梯子没有碰撞箱」| 不覆写（=getShape）| 3 处 |
| `NetherWartBlock` | 返回空 | 不覆写（=getShape）| 0 处（形状极小，两种判定恰好都是 false）|

`NetherWartBlock` 虽然没被本能力的默认状态用例检出，但语义同样是错的，一并修复。

**梯子的碰撞箱虽薄但确实存在**——玩家撞上去会被挡住，只是能贴着它攀爬。
「可以攀爬」不等于「没有碰撞」。

### 如何避免同类错误

判断某个方块在 Cubium 里是否应当返回空碰撞形状时，**不能凭直觉**，要回到原版源码确认：

```
grep "protected VoxelShape getCollisionShape" <Block>.java
```

- **有覆写** → 按原版分支实现；
- **无覆写** → 碰撞形状 = `getShape` 的结果，**永远不要返回空**（除非 `getShape` 本身为空，
  例如打开状态的活板门）。

## 修正：早期对「归一化缺口」的误判

本文档早期版本曾判断剩余差异的主因是 `VoxelShape` 的坐标归一化。**那个判断是错的。**

误判的来源是把 `fence_gate` 的现象当成了通例：它的支撑形状 `y` 上界确实超出方块顶部
（`24/16`），看起来像归一化问题。但对地毯做定量诊断后发现，**差异方块里绝大多数根本
不是坐标精度问题，而是整个形状为空**。

教训：**「看起来像某个已知缺口」不等于「就是那个缺口」。** 应当先写最小诊断、打印实际
中间值（这里就是一个坐标点数 `1,1,1` vs `2,2,2`），再下结论。

`VoxelShape` 的归一化缺口确实存在（`VoxelShape.cpp:66` 的 TODO），但它影响的是
**非空形状的切片精度**，与「形状为空」是两回事。

仍有 **80 个方块**受影响。差异分布：

| `SupportType` | 差异数 |
|---|---|
| `CENTER` | 132 |
| `FULL` | 34 |
| `RIGID` | 34 |

### 已定位的两个根因

**一、原版有 8 个方块覆写了 `getBlockSupportShape`，Cubium 缺 4 个。**

| 原版类 | Cubium 状态 | 原版实现 |
|---|---|---|
| `LeavesBlock` | ✅ 已修复 | `Shapes.empty()` |
| `FenceGateBlock` | ✅ 已修复 | 关闭时 `SHAPE_SUPPORT`，打开时 `Shapes.empty()` |
| `ShulkerBoxBlock` | ❌ **缺失** | 关闭时 `Shapes.block()`，打开时返回倾斜形状 |
| `CeilingHangingSignBlock` | ❌ **缺失** | `this.getShape(...)` |
| `WallHangingSignBlock` | ❌ **缺失** | `this.getShape(...)` |
| `MudBlock` | ✅ 有覆写 | — |
| `SoulSandBlock` | ✅ 有覆写 | — |
| `SnowLayerBlock` | ❌ **文件未找到** | 按层数 |

### `FenceGateBlock` 的支撑形状与碰撞形状只差纵向起点

这是本轮最有价值的发现：

```java
private static final Map<Direction.Axis, VoxelShape> SHAPE_COLLISION =
    Shapes.rotateHorizontalAxis(Block.column(16.0, 4.0, 0.0, 24.0));
private static final Map<Direction.Axis, VoxelShape> SHAPE_SUPPORT =
    Shapes.rotateHorizontalAxis(Block.column(16.0, 4.0, 5.0, 24.0));
//                                                        ^ 只有这一个数字不同
```

`Block.column(xzWidth, xzDepth, yMin, yMax)` 的参数含义：

| 参数 | 值 | 含义 |
|---|---|---|
| `xzWidth` | `16.0` | x 向满宽 |
| `xzDepth` | `4.0` | z 向厚 4/16（居中，即 `6/16..10/16`）|
| `yMin` | `0` / **`5`** | **纵向起点：碰撞 0，支撑 5/16** |
| `yMax` | `24.0` | 纵向终点 24/16（超出方块顶）|

**支撑面比碰撞面高 5/16。** 若把支撑形状也写成从 `y=0` 起，下方 `y=0` 处的火把会被误判为「有支撑」。

修复后减少 **24 处差异**（421 → 397）。

**二、剩余 344 处差异**：

| `SupportType` | 差异数 |
|---|---|
| `CENTER` | 136 |
| `FULL` | 32 |
| `RIGID` | 32 |

### 下一步方向

1. 继续用「打印实际中间值」的方式定位剩余差异（本轮正是靠坐标点数 `1,1,1` vs `2,2,2`
   才找到空形状这个真因）；
2. 补齐 `ShulkerBoxBlock` / `CeilingHangingSignBlock` / `WallHangingSignBlock` 的
   `getBlockSupportShape`；
3. 定位 `SnowLayerBlock` 的 Cubium 实现（当前按文件名找不到）。

**本能力维持「部分通过」状态是有意的**——它已经把根因从「625 处差异」收敛到
「**4 处空形状/缺失覆写 + 1 个待确认的形状精度缺口**」，后续修复有明确方向。

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
