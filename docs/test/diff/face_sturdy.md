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
| 修复 `CarpetBlock` / `LadderBlock` / `NetherWartBlock` | 344 |
| 修复 `dried_kelp_block` / `ender_chest` / `daylight_detector` | 270 |
| 修复 `piston_head` / `enchanting_table` | 255 |
| 修复 `moving_piston` / `azalea` 系列 | 249 |
| 补全 `CombineOp::AND` / `NOT`，对齐 3 个炼药锅类 | 189 |
| 旗帜补 `noCollision()`（支撑形状应为空）| 141 |
| 悬挂告示牌补 `getBlockSupportShape` 覆写 + 修正形状 | 129 |
| 蜡烛补 `withCollision()`（支撑形状应非空）| 95 |
| 吊挂告示牌按 `rotation` 区分形状 | 83 |
| 墙面悬挂告示牌按原版方式合成细分网格 | 71 |
| 中继器/比较器补 `withCollision()` | 65 |
| 修复 6 处「像素值传给归一化 `box()`」+ 4 处方块碰撞状态 | 39 |
| 栅栏门支撑形状改用 `box()` 绕开坐标系启发式 | 27 |
| 5 个方块补 `withCollision()` | **22** |

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

## 后续批次修复（344 → 249）

| 方块 | 问题 | 差异 |
|---|---|---|
| `dried_kelp_block` | `Material::PLANT` 推导出「无碰撞」，形状整个为空 | 18 |
| `ender_chest` | 碰撞箱横向满宽（应为 14×14×14）| 17 |
| `daylight_detector` | **完全无形状定义**（应为 16×6×16 薄板）| 15 |
| `moving_piston` | 静态形状应为空（移动中无稳定形状）| 18 |
| `piston_head` | **完全无形状定义**（平台 + 推杆，长杆到 20/16）| 15 |
| `enchanting_table` | 两层拼合，与原文实心矮块完全不符 | 6 |
| `azalea` / `flowering_azalea` | 同干海带块，材质短路了自定义形状 | 8 |

### 关键教训：`Material` 不等于形状

```cpp
// BlockProperties 的构造
m_hasCollision(material.blocksMovement())
// Material::PLANT → makePlantMaterial().solid(false) → blocksMovement() == false
```

干海带块与杜鹃花借用植物材质**只为挖掘手感与可燃性**，自身却是实心方块：

- 干海带块在原版注册成普通 `Block`，玩家能站上去；
- 杜鹃花有上半叶 + 细杆的实际碰撞箱。

但材质推导出的「无碰撞」会把 `m_hasCollision` 置为 false，`Block::getCollisionShape` 于是
直接返回空——**连方块自己 `getShape` 定义的形状也会被整个短路**。

为此新增 `BlockProperties::withCollision()` 显式覆盖材质推导值。

### 教训：诊断必须打印实际中间值

本轮所有定位都靠临时 `DIAG` 分支输出，而不是靠阅读代码猜测：

```
minecraft:dried_kelp_block|DOWN|DIAG -> col:empty sup:empty          ← 形状为空
minecraft:cauldron|DOWN|DIAG          -> supBoxes=5 voxCoordsY=5     ← 坐标集偏少
minecraft:stone|DOWN|DIAG             -> col:full sup:full          ← 正常
```

上一轮之所以误判成「归一化缺口」，就是因为只读代码没有打印实际值。

## 炼药锅系列：45 处已收敛（249 → 189）

> **本节保留原始分析以供追溯，但结论已被下一节推翻。**

剩余差异中最大的一类是炼药锅：`cauldron` 15 + `lava_cauldron` 15 +
`powder_snow_cauldron` 15 = **45 处**，占剩余量的绝大部分。

### 原版的构造方式

```java
private static final VoxelShape SHAPE_INSIDE = Block.column(12.0, 4.0, 16.0);
protected static final VoxelShape SHAPE =
    Shapes.join(Shapes.block(), SHAPE_INSIDE, BooleanOp.ONLY_FIRST);   // 完整方块减去空腔
```

### 实际原因：漏掉了底部的十字凹槽

上面引用的 `Shapes.join(Shapes.block(), SHAPE_INSIDE, ONLY_FIRST)` 是**旧版**写法。
1.21.11 的 `AbstractCauldronBlock` 已经在被减集合里**多加了三块**：

```java
return Shapes.join(
    Shapes.block(),
    Shapes.or(Block.column(16.0, 8.0, 0.0, 3.0),   // 底部 X 向凹槽
              Block.column(8.0, 16.0, 0.0, 3.0),   // 底部 Z 向凹槽
              Block.column(12.0, 0.0, 3.0),        // 锅腔下段
              SHAPE_INSIDE),                       // 锅腔上段
    BooleanOp.ONLY_FIRST);
```

`Block.column(xzW, xzD, yMin, yMax)` 以方块中心为原点，展开成像素坐标是：

| 调用 | 展开 |
|---|---|
| `column(16.0, 8.0, 0.0, 3.0)` | `box(0, 0, 4, 16, 3, 12)` |
| `column(8.0, 16.0, 0.0, 3.0)` | `box(4, 0, 0, 12, 3, 16)` |
| `column(12.0, 0.0, 3.0)` | `box(2, 0, 2, 14, 3, 14)` |
| `SHAPE_INSIDE` | `box(2, 4, 2, 14, 16, 14)` |

Cubium 三个炼药锅类都手工拼「底座 + 四壁」的并集，**漏掉了前两个十字凹槽**——
它们在 Y 只到 3、X/Z 未铺满，是锅底向外的两条凹槽。

### 根因：`CombineOp::AND` / `NOT` 静默返回空形状

```cpp
// 修复前 src/common/physics/collision/CollisionShape.hpp
if (op == CombineOp::OR) {
    ...
}
// AND 和 NOT 操作暂不实现（需要更复杂的几何运算）
return result;   // ← 空形状，不报错
```

只要传 `AND` 或 `NOT` 就拿到一个空形状，**既不报错也没有任何提示**。因此在表达
「完整方块减去某集合」时无法使用 `Shapes.join`，只能手工拼并集，于是漏项。

本轮补全了这两个操作：

- **`AND`**：两个盒列表两两求交，交集非空才保留。
- **`NOT`**：逐个把 a 的盒子按 b 的盒子切分（X / Y / Z 三向各切两侧，最多 6 个碎块）。

修改后三个炼药锅类都改为与原版同构的 `combine(fullBlock(), carved, NOT)` 写法，
45 处差异**全部消除**。

```cpp
// TODO: CombineOp::NOT 的结果盒数可能显著增加（1 个盒被 n 个盒切分后最多 6n 个碎块）。
// 当前按点是否落在任一盒内判定，语义正确但会放大遍历开销。若后续出现性能问题，
// 可考虑引入 VoxelShape 式的离散网格存储。
```

> **教训**：把「未实现」写成静默返回空值，会把一个立即暴露的缺失变成难以察觉的数据错误。
> 这类地方应当断言或显式标注，而不是悄悄返回一个合法但错误的值。

### 为什么 5 个盒子拼不出等价形状

直觉上「完整方块减去中心空腔」= 底座 + 四壁，5 个盒子就够了。但把原版形状
`toAabbs()` 打印出来，得到的是 **15 个盒子**：

```
AABB[0.0, 0.0, 0.0] -> [0.125, 1.0, 0.25]
AABB[0.0, 0.0, 0.75] -> [0.125, 1.0, 1.0]
...
AABB[0.0, 0.1875, 0.25] -> [1.0, 0.25, 0.75]     ← 注意 0.1875
...
```

原因是**离散网格把边界切碎了**：`Shapes.join` 会合并两个形状的坐标集，
`column(12,4,16)` 的 y 坐标是 `[0, 0.25, 0.5, 0.75, 1.0]`，与 `block` 的 `[0, 1]` 合并后
生成 `[0, 0.1875, 0.25, 0.5, 0.75, 1.0]`——多出一个 `0.1875`。

而 `isFaceFull` 判的是**离散格子是否全填满**（原版 `isShapeFullBlock` → `isFullWide`），
不是 AABB 覆盖。网格切分不同，判定结果就不同。

### 无法在本层修复

```cpp
// src/common/physics/collision/CollisionShape.hpp
enum class CombineOp : u8 { OR, AND, NOT };
// AND 和 NOT 操作暂不实现（需要更复杂的几何运算）
// TODO: 补全 CombineOp::AND / CombineOp::NOT。当前传这两个值会**静默返回空形状**
```

`CollisionShape` 只存**轴对齐盒的并集**，无法表达「带内部空洞的拓扑」，也存不下
离散网格的坐标集。要真正收敛这 45 处，需要把 `CollisionShape` 的底层存储换成
`VoxelShape`（或补全 `AND`/`NOT` 并保留精确坐标集）——这是架构级改造，
会影响光照遮挡、碰撞检测、`shapes_boolean`、`collide_sweep` 等所有使用方。

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

## 蜡烛系列：32 处已收敛（129 → 95）

### 现象

`candle` / `candle_cake` 各 16 处，全部是 `DOWN|CENTER`：**原版 true、Cubium false**。

### 排查

第一步实测原版蜡烛的支撑形状：

```
support bounds = AABB[0.4375, 0.0, 0.4375] -> [0.5625, 0.375, 0.5625]
shape bounds   = AABB[0.4375, 0.0, 0.4375] -> [0.5625, 0.375, 0.5625]
```

**支撑形状非空**，而且 x/z 范围 `0.4375..0.5625` 与 `CENTER_SUPPORT_SHAPE`
（`column(2.0, 0.0, 10.0)` → `box(7,0,7,9,10,9)` = `0.4375..0.5625`）**完全吻合**。
所以 `DOWN|CENTER` 判 true 是正确的。

Cubium 的蜡烛形状本身是对的：

```cpp
m_shapesByCount[0] = CollisionShape::fromPixelBox(7.0f, 0.0f, 7.0f, 9.0f, 6.0f, 9.0f);
```

### 根因：`Material::DECORATION.solid(false)` 让碰撞被关掉

Cubium 的 `BlockProperties` 用 **Material 的 `blocksMovement`** 初始化 `m_hasCollision`：

```cpp
m_hasCollision(material.blocksMovement())
```

而 `makeDecorationMaterial()` 是 `.solid(false)`，于是 `m_hasCollision = false`，
`getCollisionShape` 走 `!m_hasCollision → VoxelShapes::empty()`，
支撑形状因此为空。

**这是架构差异**：原版 `hasCollision` 是独立的方块属性（默认 true，用 `.noCollision()`
关闭）；Cubium 从 Material 的 `solid` 派生。

### 弯路：先删了 `noCollision()`，没用

第一次尝试是删掉 `CandleBlocks.cpp` 里的 `.noCollision()`。结果差异从 32 **升到 34**——
因为 `DECORATION` 的 `solid(false)` 已经让 `m_hasCollision` 为 false，显式删不删都无所谓。

### 正确修复：显式 `withCollision()`

```cpp
BlockProperties(Material::DECORATION).withCollision()
BlockProperties(Material::CAKE).withCollision()
```

`withCollision()` 是本次会话早先为 `dried_kelp_block` / `azalea` 新增的，
用来在 Material 派生之外**显式打开**碰撞。

蜡烛本体用 `Material::DECORATION`（17 处）、蜡烛蛋糕用 `Material::CAKE`（17 处），
两类分别补上后 **32 处差异全部消除**。

> **教训**：Cubium 的 `m_hasCollision` 有两个来源——Material 派生与显式 `withCollision()`/
> `noCollision()`。判断某个方块应当有/无碰撞时，必须同时看 Material 的 `solid` 与显式设置，
> 只看一侧会得出错误结论。
## 吊挂告示牌：按 rotation 区分形状（95 → 83）

### 缺陷

`oak_hanging_sign` 等 12 个变体，全部是 `UP|CENTER`：**原版 false、Cubium true**。

### 根因：原版形状随 rotation 变化，Cubium 恒定

原版 `CeilingHangingSignBlock`：

```java
SHAPE_DEFAULT = Block.column(10.0, 0.0, 16.0);
SHAPES        = Shapes.rotateHorizontal(Block.column(14.0, 2.0, 0.0, 10.0))...;
getShape(...) { return SHAPES.getOrDefault(state.getValue(ROTATION), SHAPE_DEFAULT); }
```

`rotateHorizontal` 只产生 8 个**旋转段**（每个 22.5°），映射到 `rotation` 值后，
只有 0/8 与 4/12 落在表内，其余 12 个走 `SHAPE_DEFAULT`。

实测原版 16 个 rotation 的支撑形状：

| rotation | 形状 | 来源 |
|---|---|---|
| 0, 8 | `box(1,0,7,15,10,9)` 长边沿 X，**Y 只到 10** | `SHAPES` |
| 4, 12 | `box(7,0,1,9,10,15)` 长边沿 Z，**Y 只到 10** | `SHAPES` |
| 其余 12 个 | `box(3,0,3,13,16,13)` **Y 到 16** | `SHAPE_DEFAULT` |

Cubium 此前恒定返回 `box(0.25,0,0.25,0.75,1,0.75)`（即 `box(4,0,4,12,16,12)`），
**三种都不匹配**：X/Z 范围比 `SHAPE_DEFAULT` 窄，Y 范围又比 `SHAPES` 高。

**Y 范围（10 vs 16）的差异直接影响 `UP|CENTER`**：面切片取的 VoxelShape 不同，
中心柱是否被覆盖的结论就不同。

已改为按 rotation 查表的 `m_shapesByRotation` + `m_defaultShape`，12 处差异全部消除。

## 墙面悬挂告示牌：网格细分必须按原版方式合成（83 → 71）

`*_wall_hanging_sign` 仍剩 12 处。这一块的难点**不是形状外轮廓，而是离散网格的细分**。

### 外轮廓已经查清

```java
getBlockSupportShape(...) { return this.getShape(state, level, pos, CollisionContext.empty()); }
```

实测原版（`getShape` 与支撑形状相同）：

| FACING | 外轮廓 |
|---|---|
| NORTH / SOUTH | `box(0, 0, 6, 16, 16, 10)` X 满宽、Z 居中 2 像素、**Y 满高** |
| EAST / WEST | `box(6, 0, 0, 10, 16, 16)` Z 满宽、X 居中 2 像素、**Y 满高** |

### 但网格不是 2 个坐标

原版形状来自 `Shapes.rotateHorizontalAxis(or(SHAPES_PLANK.get(Axis.Z), column(14.0, 2.0, 0.0, 10.0)))`，
合并后的坐标列表被细分：

```
EAST 朝向  X = [0, 0.125, 0.25, 0.375, 0.4375, 0.5, 0.5625, 0.625, 0.75, 0.875, 1]
           Z = [0, 0.0625, 0.9375, 1]
```

于是 `getFaceShape(EAST)` 取到的切片落在**没有体素的格子**里，返回空形状，
原版的 `EAST|CENTER` 因此是 false。

Cubium 用 `fromPixelBox(6,0,0,10,16,16)` 只有 2 个坐标，切片非空，误判为 true。

### 尝试过的两条路（都不对）

1. **按 `Block.column` 参数直接代入像素坐标**推出「横板 + 中心立柱」——
   与实测外轮廓完全不符。`rotateHorizontalAxis` 的结果**不能**这样推断。
2. **改用实测外轮廓的 2 坐标盒子**——外轮廓对了，但网格太粗，
   `EAST|WEST|CENTER` 从 false 变 true，差异从 12 升到 24。

本轮已回退到路径 1 的状态（保留 12 处差异），并把外轮廓与网格数据记入本文档。

### 最终解法（已实施）

复刻原版的合成方式：先用 `Block.column` 的展开公式算出两个分量盒，再用 `combine(OR)`
取坐标并集。

```cpp
column(16.0, 4.0, 14.0, 16.0)  ->  box(0, 14, 6, 16, 16, 10)   顶部横板
column(14.0, 2.0,  0.0, 10.0)  ->  box(1,  0, 7, 15, 10,  9)   中心立柱

// Z 轴朝向：直接 OR
alongZ = combine(plank, pillar, OR);
// X 轴朝向：两个分量盒各绕 Y 轴转 90 度后再 OR
alongX = combine(plankRot, pillarRot, OR);
```

得到的网格与原版完全一致：

```
X = [0, 0.0625, 0.9375, 1]                                       (3 格，中间那格大部分是空的)
Z = [0, 0.125, 0.25, 0.375, 0.4375, 0.5, 0.5625, 0.625, 0.75, 0.875, 1]
```

**12 处差异全部消除。**

> 教训：构造 VoxelShape 时**不能只对齐外轮廓**。原始形状由多个盒子 OR 而成时，
> 其离散网格（坐标并集）才是决定面切片行为的关键，必须按同样的分量盒与顺序合成。

## 中继器 / 比较器：补 `withCollision()`（71 → 65）

`repeater` 与 `comparator` 各 3 处，全部是 `DOWN|CENTER` / `DOWN|FULL` / `DOWN|RIGID`：
原版 true、Cubium false。

这与蜡烛系列是**同一个根因**：两者都注册在 `Material::DECORATION` 上，而
`makeDecorationMaterial()` 是 `.solid(false)`，`m_hasCollision = false`，
支撑形状因此为空。

```cpp
BlockProperties(Material::DECORATION).withCollision().notSolid()
```

6 处差异全部消除。

> 注意 `redstone_wire` 也用了 `Material::DECORATION`，但它同时显式写了 `.noCollision()`
> ——后者覆盖前者，红石线确实没有碰撞形状，原版也是如此。判断这类方块时必须看
> **最终生效的显式设置**，不能只看 Material。

## 脚手架：尝试失败已回退（仍剩 3 处）

`scaffolding` 的 3 处是 `UP|CENTER` / `UP|FULL` / `UP|RIGID`：原版 true、Cubium false。

### 实测

```java
getShape(empty ctx) = VoxelShape[AABB[0.0, 0.0, 0.0] -> [1.0, 1.0, 1.0]]
support             = VoxelShape[AABB[0.0, 0.0, 0.0] -> [1.0, 1.0, 1.0]]
cshape(empty)       = VoxelShape[AABB[0.0, 0.0, 0.0] -> [1.0, 1.0, 1.0]]
holdingItem(empty)  = false
```

原版脚手架**没有覆写** `getBlockSupportShape`，走基类链路
`getCollisionShape(..., CollisionContext.empty())`。而 `CollisionContext.empty()` 让
`isAbove(...)` 恒为 true、`isDescending()` 恒为 false，于是命中
`isAbove(Shapes.block(), pos, true) && !isDescending()` 分支返回 `SHAPE_STABLE`。

### 失败原因

我给 Cubium 覆写 `getBlockSupportShape` 直接返回满方块，结果差异从 3 **升到 15**——
说明并非所有状态都返回满方块。`SHAPE_STABLE` 名义上是「顶部平台 + 四角柱」，
实测却给出满方块，两者不一致，说明我对该分支的实际走向判断有误。

已回退。`TODO`：需要逐状态测出原版脚手架 8×`distance`×`bottom` 组合下的支撑形状，
再决定 Cubium 的覆写策略，而不是一刀切返回满方块。
## 像素坐标误传给归一化 `box()`：一次清掉 17 处（65 → 48）

### 发现方式

按「`Material` 的 `solid` 决定碰撞」这条线索，扫描所有 `solid(false)` 材质的方块，
但发现 `lantern` 用的是 `Material::IRON`（solid = true）也报差异——说明根因不止一处。

于是改为扫描 `CollisionShape::box(...)` 的**数值范围**：归一化接口的参数必须落在 0..1，
任何超过 1 的参数都是错误。

### 查出的 6 处

| 文件 | 错误代码 | 影响方块 |
|---|---|---|
| `LanternBlock.cpp:69,71` | `box(5.0f, 0.0f, 5.0f, 11.0f, 7.0f, 11.0f)` | `lantern` / `soul_lantern` |
| `WeatheringCopperLanternBlock.cpp:73,75,191,192` | 同上 | 9 个 `*_copper_lantern` |
| `CopperGolemStatueBlock.cpp:72` | `box(3.0f, 0.0f, 3.0f, 13.0f, 14.0f, 13.0f)` | 8 个 `*_golem_statue` |
| `ConduitBlock.cpp:58` | `box(5.0f, 0.0f, 5.0f, 11.0f, 6.0f, 11.0f)` | `conduit` |

```cpp
// ❌ box() 收归一化坐标，传入像素值 5..11 会得到远超方块的形状
m_standingShape = CollisionShape::box(5.0f, 0.0f, 5.0f, 11.0f, 7.0f, 11.0f);

// ✅ 像素坐标要用 fromPixelBox
m_standingShape = CollisionShape::combine(
    CollisionShape::fromPixelBox(6.0f, 7.0f, 6.0f, 10.0f, 9.0f, 10.0f),   // 顶部小柱
    CollisionShape::fromPixelBox(5.0f, 0.0f, 5.0f, 11.0f, 7.0f, 11.0f),    // 灯体
    CollisionShape::CombineOp::OR);
```

同时按原版 `SHAPE_STANDING = or(column(4.0, 7.0, 9.0), column(6.0, 0.0, 7.0))` 补全了
灯笼的「顶部小柱 + 灯体」两段结构（此前只画了灯体一段，Y 上限也写错成 7，应为 9）。

**17 处差异一次性消除。**

> 教训：`CollisionShape::box` 与 `fromPixelBox` 的坐标系完全不同，参数却都是 6 个 float，
> 编译器无法区分。**建议在 review 时对所有 `box(...)` 调用检查参数是否 ≤ 1.0**——
> 这条静态规则能直接抓出这一类缺陷。

## 方块碰撞状态按实测逐个纠正（48 → 39）

| 方块 | Cubium 原设置 | 原版实测 | 修正 |
|---|---|---|---|
| `small/medium/large_amethyst_bud` | `.noCollision()` | support 非空、collision 非空 | 改 `.withCollision()` |
| `end_rod` | `.noCollision()` | support `box(6,0,6,10,16,10)` 非空 | 改 `.withCollision()` |
| `pale_moss_carpet` | `.noCollision()` | support `box(0,0,0,16,1,16)` 非空 | 改 `.withCollision()` |
| `nether_wart` | `getCollisionShape` 返回 `getShape` | **collision 与 support 均为空** | 改回返回空形状 |

`nether_wart` 这一处尤其值得记录：原版 `getShape` 非空（`box(0,0,0,16,5,16)`，随 age 变化），
但 `getCollisionShape` 与 `getBlockSupportShape` **都是空**。此前代码的注释断言
「碰撞形状等于 getShape」，与本能力实测的 `nether_wart|DOWN|CENTER` 原版 false / Cubium true
直接矛盾。**注释中的推断不能替代实测。**
## 栅栏门：坐标系启发式误判（39 → 27）

### 现象

12 个 `*_fence_gate`，全部是 `UP|CENTER`：原版 true、Cubium false。

### 排查

实测原版支撑形状：

```
north open=false -> AABB[0.0, 0.3125, 0.375] -> [1.0, 1.5, 0.625]
north open=true  -> EMPTY
```

Cubium 的 `m_supportShapes` 写的正是 `box(0, 0.3125, 0.375, 1, 1.5, 0.625)`，**数值完全一致**。

### 根因：`VoxelShapes::cube` 的坐标系启发式

`VoxelShapes::cube` 为了同时兼容「像素坐标」和「归一化坐标」两种写法，用参数的最大值猜测：

```cpp
CollisionShape VoxelShapes::cube(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2)
{
    const f32 maxCoord = std::max({std::abs(x1), ..., std::abs(z2)});
    if (maxCoord <= 1.0f) {
        return CollisionShape::box(...);        // 当作归一化
    }
    // 否则当作像素，整体除以 16
}
```

栅栏门支撑形状的 Y 上限是 **1.5**（原版 `SHAPE_SUPPORT = column(16.0, 4.0, 5.0, 24.0)`，
Y 范围 5/16..**24/16**，故意超出方块顶部一格），`maxCoord = 1.5 > 1.0`，
于是被当成像素坐标除以 16，实际形状缩成 `0..0.09375` —— `UP` 面自然覆盖不到中心。

改用 `CollisionShape::box`（明确归一化）后 12 处全部消除。

### 这个启发式的适用范围

全项目扫描后，24 处归一化调用中只有栅栏门这一处越界失效——其余都用
`14.0f / 16.0f` 这类表达式，结果 ≤ 1.0，能被正确识别。

> **建议**：`VoxelShapes::cube` 的坐标系猜测是脆弱的接口设计。任何「归一化但某个分量
> 故意超出 1.0」的形状（跨方块的支撑形状、合并后的复合形状）都会静默出错。
> 新代码应直接用 `CollisionShape::box` 或 `fromPixelBox`，不要依赖这个猜测。
> **TODO**：后续应考虑废除 `VoxelShapes::cube` 的双坐标系兼容，统一到两个显式接口。

## 未收敛：脚手架（3 处，已回退两次）

`scaffolding` 的 3 处是 `UP|CENTER` / `UP|FULL` / `UP|RIGID`：原版 true、Cubium false。

### 实测数据

```
bottom=false -> support = AABB[0.0, 0.0, 0.0] -> [1.0, 1.0, 1.0]   实心
bottom=true  -> support = AABB[0.0, 0.0, 0.0] -> [1.0, 1.0, 1.0]   实心

isFaceSturdy(null, ZERO, UP):
  up    FULL=true  CENTER=true  RIGID=true
  down  FULL=false CENTER=false RIGID=false
  north FULL=false CENTER=false RIGID=false
```

### 矛盾点

支撑形状明明是**实心 1×1×1**，`UP` 三型全 true 符合预期，但 `DOWN` / `NORTH` **全为 false**
—— 满方块在 `DOWN` 面和 `NORTH` 面本应同样判 true。

这说明 `isFaceSturdy` 走的不是 `getBlockSupportShape` 的实时计算，而是
`BlockStateBase.Cache` 里**方块初始化时预算好的缓存值**，而缓存计算时形状很可能为空
（脚手架注册带 `noCollision()`，`hasCollision = false`）。

### 两次失败的尝试

1. 覆写 `getBlockSupportShape` 返回 `getShape`（平台 + 四角柱）→ 差异 3 → 15
2. 覆写返回 `fullBlock()` → 差异 3 → 15，且 `DOWN`/`NORTH`/`EAST`/`WEST` 全部误判为 true

### TODO

需要先搞清楚 Cubium 的 `BlockStateBase.Cache` 等价物在什么时机、用什么 level 计算
`isFaceSturdy` 缓存，再决定脚手架的支撑形状。直接覆写 `getBlockSupportShape` 会与
缓存路径脱节，必然失败。
## 补 5 个方块的碰撞（27 → 22）

按「原版支撑形状非空，但 Cubium 判 false」这条判据，扫出仍带 `noCollision()` 的方块：

| 方块 | 原版支撑形状 |
|---|---|
| `sea_pickle` | `box(6,0,6,10,6,10)` |
| `turtle_egg` | `box(3,0,3,12,7,12)` |
| `lily_pad` | `box(1,0,1,15,1.5,15)` |
| `decorated_pot` | `box(1,0,1,15,16,15)` |

全部改为 `withCollision()`，4 处差异（含 `decorated_pot` 的两面）消除。

### `chorus_flower` 单独失败已回退

`chorus_flower` 原版支撑形状是 `box(1,0,1,15,15,15)`，Cubium 判 false，看起来该改。
但改成 `withCollision()` 后差异从 1 **涨到 17**——说明它的碰撞形状本身有 bug，
一旦启用碰撞就会把错误暴露到所有面上。

已回退为 `noCollision()`。`TODO`：先修 `ChorusFlowerBlock` 的形状，再开启碰撞。

> 这是本能力里第二次遇到「补碰撞反而恶化」。判据是：**补碰撞后若差异从 1 变成十几处，
> 说明该方块自身的形状实现有误**，碰撞开关只是把它暴露出来。修复顺序应当是
> 「先修形状，再开碰撞」。
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
