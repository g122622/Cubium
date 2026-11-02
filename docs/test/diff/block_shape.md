# 差分能力：`block_shape`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 196 个能力。1166 个单元，首次运行报出 **306 处差异**（146 collision + 54 support，其余为
报告上限截断）。

## 测什么

每个方块默认状态的**三种形状**，逐盒输出坐标：

| 形状 | 原版接口 | 用途 |
|---|---|---|
| 外形 | `getShape` | 碰撞箱、视觉、选中框 |
| 碰撞 | `getCollisionShape` | 实体移动阻挡 |
| 支撑 | `getBlockSupportShape` | 附着型方块的存活判定 |

三者在原版是**各自独立**的：

- 很多方块外形与碰撞不同（空气、水、火）；
- 很多方块碰撞为空而支撑非空（灯笼、悬挂告示牌）；
- 也有反过来（下界疣：外形非空、碰撞与支撑皆空）。

## 为什么需要这一层

`face_sturdy` 只能看到支撑判定的**布尔结论**。一个方块报差异时，无法区分是外形写错了、
碰撞开关错了、还是支撑形状被覆写覆盖了——只能靠人肉逐个试，`face_sturdy` 收敛到 8 处
时剩下的全是这种「改了反而更糟」的情况。

本能力把三种形状的**坐标**直接摊开。首次运行的效果立竿见影：

```
acacia_fence
  原版    collision = 0.375,0,0.375 -> 0.625,1.5,0.625      Y 到 1.5
  Cubium  collision = 0.375,0,0.375 -> 0.625,1.0,0.625      Y 到 1.0

acacia_sapling
  原版    shape = 0.125,0,0.125 -> 0.875,0.75,0.875
  Cubium  shape = 0.25,0,0.25 -> 0.75,0.5,0.75              整体偏小一半
```

这些差异在 `face_sturdy` 里**完全不可见**（树苗的支撑形状双方都为空），但它们确实是缺陷——
影响碰撞手感与选中框。

## 关键点一：逐盒输出，不要用坐标列表

`CollisionShape` 的多盒形状（楼梯、栅栏、告示牌）如果用「各轴坐标列表」描述，会丢失盒子归属：

```
X[0,1]Y[0,0.5]Z[0,1] + X[0,0.5]Y[0.5,1]Z[0,1]
```

这串坐标无法还原成「两个盒子还是一个盒子」。改为逐个盒子输出：

```
0,0,0,1,0.5,1;0,0.5,0,0.5,1,1
```

既精确又便于对比。

## 关键点二：oracle 不能传 `null` 作为 BlockGetter

首次运行报：

```
NullPointerException: Cannot invoke "BlockGetter.getBlockEntity" ...
```

部分方块的形状实现会查询世界（例如需要 BlockEntity 的方块）。传 `null` 直接崩溃。

改用最小的只读实现：

```java
private static final BlockGetter EMPTY_LEVEL = new BlockGetter() {
    public BlockEntity getBlockEntity(BlockPos pos) { return null; }
    public BlockState getBlockState(BlockPos pos) { return Blocks.AIR.defaultBlockState(); }
    public FluidState getFluidState(BlockPos pos) { return Fluids.EMPTY.defaultFluidState(); }
    public int getMinY() { return 0; }
    public int getHeight() { return 1; }
};
```

> **踩坑**：`BlockGetter` 只声明了 `getBlockEntity` / `getBlockState` / `getFluidState` 三个
> 抽象方法（外加继承自 `LevelHeightAccessor` 的 `getMinY` / `getHeight`）。
> 我一开始多写了 `getHeight(Heightmap.Types, int, int)`，编译报
> `method does not override or implement a method from a supertype`。

## 输出格式

```
shape=<盒子列表>|collision=<盒子列表>|support=<盒子列表>
```

空形状记作 `EMPTY`，多盒用 `;` 分隔，每盒 6 个坐标（`%.6f`）。

## 未覆盖范围

- **非默认状态**：只取 `defaultBlockState()`。带朝向/开合/含水状态的方块，其它状态的形状
  未覆盖（`face_sturdy` 同样只测默认状态，两者口径一致）。
- **`getOcclusionShape` / `getInteractionShape`**：另外两种形状，未纳入。
- **方块实体的渲染形状**：不在本能力范围。

## 后续工作（TODO）

首次运行报出 306 处差异，本轮**只建立了能力，尚未修复**。已确认的典型缺陷：

| 类别 | 例子 |
|---|---|
| 碰撞 Y 上限少 | `acacia_fence` 等栅栏：原版 `1.5`，Cubium `1.0` |
| 外形整体偏小 | `acacia_sapling` 等树苗：原版 `0.125..0.875`，Cubium `0.25..0.75` |
| 支撑形状缺失 | `scaffolding`、`azalea` 等（与 `face_sturdy` 剩余 8 处重合）|

`TODO`：按 collision / support 两类分批收敛。这批差异比 `face_sturdy` 剩余的 8 处更有价值——
它们中的大多数在支撑判定里完全不可见，却直接影响碰撞与选中框。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`face_sturdy`](./face_sturdy.md) | 上层：用支撑形状做面判定 |
| [`shape_face`](./shape_face.md) | 中层：形状的面切片 |
| [`entity_shape`](./entity_shape.md) | 实体侧的同类能力 |