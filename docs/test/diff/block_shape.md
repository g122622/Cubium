# 差分能力：`block_shape`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 196 个能力。1166 个单元。首次运行报出 306 处差异；改为**体素化表示**后降到 **175 处**
——其中约 130 处原本是「同一几何的冗余盒子拆分」，属于误报。

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

## 关键点一：必须体素化，不能逐盒输出

**这是本能力最重要的设计决定，也是我第一版做错的地方。**

第一版逐盒输出坐标（每盒 6 个数，多盒用 `;` 分隔）。运行后 306 处差异，看似很多问题。
但抽样核对后发现绝大多数是**同一几何的不同拆分方式**：

```
acacia_stairs
  原版  (2 盒): 0,0,0,1,0.5,1 ; 0,0.5,0,1,1,0.5
  Cubium(3 盒): 0,0,0,1,0.5,1 ; 0,0.5,0,0.5,1,0.5 ; 0.5,0.5,0,1,1,0.5
```

后两个盒子左右相邻，合并后恰好等于原版那一个盒子——**几何完全相同**。

写脚本把两侧形状都体素化到 16³ 网格逐点比对，200 条样本里 **199 条几何等价，只有 1 条真不同**。

形状的语义是「覆盖了方块内的哪些点」，不是「由几个盒子拼出来」。改为体素化后：

```cpp
Cardinal16 voxelize(const CollisionShape& shape)  // 每体素取中心点做包含判断
```

差异从 294 降到 **175**。这 175 处才是真正的覆盖差异。

> **踩坑**：逐盒输出把「表示差异」和「语义差异」混在一起，会淹没真问题。
> 判断一个比较维度是否合适，标准是：**该维度上的差异是否都对应真实的行为差异**。
> 盒子拆分方式不满足这条，体素覆盖满足。

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

## 本轮修复的形状缺陷

体素化后的差异里，已修复 4 处：

| 方块 | 原版 | Cubium 原值 |
|---|---|---|
| `FlowerBlock` | `column(6, 0, 10)` → Y 上限 `0.625` | Y 上限 `0.375` |
| `SaplingBlock` | `column(12, 0, 12)` → `0.125..0.875`, Y `0.75` | `0.25..0.75`, Y `0.5` |
| `MushroomBlock` | `column(6, 0, 6)` → `0.3125..0.6875`, Y `0.375` | `0.25..0.75`, Y `0.5` |
| `BushBlock` | `column(16, 0, 13)` → 满宽, Y `0.8125` | `fullBlock()` |

> **注意 `cactus`**：本能力显示原版 `getShape` 的 Y 上限是 `1.0`（`column(14, 0, 16)`），
> 而 `face_sturdy` 那轮我把 Cubium 改成了 `0.9375`。两者并不矛盾——**外形与支撑形状在原版
> 是两回事**。已把 `cactus` 的 `getShape` 改回 `1.0`。这也说明单一能力容易得出片面结论。

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
shape=<体素串>|collision=<体素串>|support=<体素串>
```

空形状记作 `EMPTY`。体素串是 16×16×16 网格按 `y→z→x` 展开的 4096 个 `0/1`，
再做**游程压缩**（`<字符><次数>`，如 `0316`）以控制体积。

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

## 根因定位：`offsetFunction` 未实现（影响花类、竹类等多处方块）

`allium` 在修好 `FlowerBlock` 形状后**仍然报差异**。解码体素串对比：

```
原版   : 0×17, 1, 0×6, ...       首个覆盖在索引 17
Cubium : 0×85, 1, 0×6, ...       首个覆盖在索引 85
```

按 `y→z→x` 展开，索引 17 = `(x=1, y=0, z=1)`，索引 85 = `(x=5, y=0, z=5)`。
即原版形状**从像素 (1,1) 开始**，Cubium 从 (5,5) 开始——整体偏移了 4 像素。

原版实测：

```
allium       offsetFn=true  bounds=AABB[0.0625, 0.0, 0.0625] -> [0.4375, 0.625, 0.4375]
poppy        offsetFn=true  bounds=AABB[0.0625, 0.0, 0.0625] -> [0.4375, 0.625, 0.4375]
azure_bluet  offsetFn=true  bounds=AABB[0.0625, 0.0, 0.0625] -> [0.4375, 0.625, 0.4375]
```

`hasOffsetFunction() == true`：**所有花类都带 `offsetType`**。`FlowerBlock.SHAPE` 本身是
`column(6.0, 0.0, 10.0)`（居中，`0.3125..0.6875`），但 `getShape` 里做了
`voxelshape.move(getOffset(pos))`，在 `BlockPos.ZERO` 处偏移量为 `(-0.25, 0, -0.25)`。

这解释了至少三类现象：

| 现象 | 出处 |
|---|---|
| 花类 `0.0625..0.4375` vs `0.3125..0.6875` | 本能力（143 处「other」里的一大类）|
| `bamboo` 的 `-0.25` 偏移 | `face_sturdy` 那一轮的未解之谜 |
| `azalea` 等灌木的偏移 | `face_sturdy` 剩余差异 |

Cubium 有 `BlockProperties::getOffsetType()`，但**没有 `BlockState::getOffset(pos)`，
也没有把偏移应用到 `getShape`**。`TODO`：需要实现

```cpp
Vec3 BlockState::getOffset(const BlockPos& pos) const;   // 按 offsetType 与 pos 计算
VoxelShape Block::getShape(...)                          // 叠加 getOffset(pos)
```

这是一处**系统性缺失**，一次性影响花、作物、竹子、灌木等大量方块的形状位置。修好它预计能
清掉本能力剩余差异中的很大一部分，同时也能解决 `face_sturdy` 剩余的 `bamboo` 与 `azalea`。## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`face_sturdy`](./face_sturdy.md) | 上层：用支撑形状做面判定 |
| [`shape_face`](./shape_face.md) | 中层：形状的面切片 |
| [`entity_shape`](./entity_shape.md) | 实体侧的同类能力 |