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
清掉本能力剩余差异中的很大一部分，同时也能解决 `face_sturdy` 剩余的 `bamboo` 与 `azalea`。## 已实现：`offsetFunction`（175 → 162）

上一轮定位到花类差异来自 `offsetType`。本轮补齐了整条链路：

### 1. `Block::getOffset(pos)`

完整照搬原版公式，不做近似：

```cpp
const i64 seed = math::mthGetSeed(pos.x, 0, pos.z);   // y 固定取 0
const auto horizontal = [&](i64 nibble) {
    const f32 raw = (static_cast<f32>(nibble & 15LL) / 15.0f - 0.5f) * 0.5f;
    return static_cast<f64>(math::clamp(raw, -maxHorizontal, maxHorizontal));
};
const f64 offsetX = horizontal(seed);
const f64 offsetZ = horizontal(seed >> 8);
// XYZ 另有竖直分量：(seed >> 4 & 15) / 15 - 1) * maxVertical
```

`getMaxHorizontalOffset()` / `getMaxVerticalOffset()` 做成虚函数（默认 0.25 / 0.2），
与原版一致，供个别方块覆写。

> **关键**：偏移依赖 `pos` 的哈希，**不能硬编码成 `(-0.25, 0, -0.25)`**。
> `BlockPos.ZERO` 处恰好是这个值只是巧合，换个坐标就不成立。

### 2. `CollisionShape::offset(dx, dy, dz)`

新增整体平移方法，逐盒平移后重建形状。

### 3. `BlockState::getShapeAt(pos)`

```cpp
CollisionShape BlockState::getShapeAt(const BlockPos& pos) const
{
    const CollisionShape& base = m_owner->getShape(*this);
    if (m_owner->getOffsetType() == BlockProperties::OffsetType::None) {
        return base;                       // 绝大多数方块，直接返回副本
    }
    const Vector3 offset = m_owner->getOffset(pos);
    return base.offset(offset.x, offset.y, offset.z);
}
```

保留原有 `getShape()`（模型坐标，无偏移）不动——159 处覆写无需修改。

### 4. 注册处补 `offsetType`

发现**注册处从未调用过 `offsetType()`**：`BlockProperties` 有该方法也有 `m_offsetType`
成员，但全部方块都取了默认的 `None`。已给花类补上 `XZ`：

```cpp
BlockProperties flowerProps = BlockProperties(Material::REPLACEABLE_PLANT)
                                  .noCollision()
                                  .notSolid()
                                  .offsetType(BlockProperties::OffsetType::XZ);
```

### 验证

原版实测：

```
allium        offset(ZERO) = (-0.25,  0.0,                  -0.25)
bamboo        offset(ZERO) = (-0.25,  0.0,                  -0.25)
short_grass   offset(ZERO) = (-0.25, -0.20000000298023224,  -0.25)   ← XYZ 类型
```

`block_shape` 从 175 降到 **162**，`allium` 消除。

### TODO

- **其余方块仍需补 `offsetType`**：`bamboo`（XZ）、`short_grass` 等灌木（XYZ）、
  各类作物。原版 `Blocks.java` 里逐个核对最可靠，不要凭经验猜。
- **`Raycast` / `Explosion` 应改用 `getShapeAt(pos)`**：它们有 `pos`，当前用的
  `getShape()` 不含偏移，射线与爆炸对花类的判定会偏。
- **客户端 5 处调用**同样应改，但客户端已停止维护，暂不处理。
## 续：`offsetType` 批量补齐与逻辑接入（162 → 170）

### 1. 射线与爆炸接入 `getShapeAt(pos)`

这是上一轮遗留的**游戏逻辑偏差**，不只是数据差异：

```cpp
// Raycast.cpp
const BlockPos pos(blockX, blockY, blockZ);
const CollisionShape shape = state.getShapeAt(pos);

// Explosion.cpp
const CollisionShape shape = state.getShapeAt(BlockPos(blockX, blockY, blockZ));
```

两处原本都用 `getShape()`（模型坐标，不含偏移），射线与爆炸对花、草类方块的判定会偏离原版。

### 2. 从原版枚举出全部带偏移的方块

用 oracle 侧遍历注册表，按 `getOffset(ZERO).y != 0` 区分类型，得到权威清单：**XZ 32 个 +
XYZ 5 个 = 37 个**。

已补：

| 属性对象 | 类型 | 覆盖方块 |
|---|---|---|
| `flowerProps` | XZ | 蒲公英、虞美人、蓝花美耳草、绒球葱等全部单格花 |
| `grassProps` | **XYZ** | `short_grass`、`fern` |
| `tallGrassProps`（新增） | XZ | `tall_grass` |
| `tallFlowerProps` | XZ | 向日葵、丁香、玫瑰丛、牡丹、大型蕨 |
| `bambooProps` / `bambooSaplingProps` | XZ | `bamboo`、`bamboo_sapling` |
| 下界、洞穴、苍白花园、花园、红树的若干内联属性 | XZ / XYZ | 菌索、下界苗、滴水石锥、垂根、眼眸花、干草、`bush` |

> **关键发现**：`short_grass`/`fern` 是 **XYZ**（有竖直偏移），而 `tall_grass` 是 **XZ**。
> 原版它们共用 `BushBlock` 但注册属性不同，**不能共用同一个 `BlockProperties` 对象**。
> 我最初给 `grassProps` 加 XYZ 后直接影响到 `tall_grass`，后来拆出 `tallGrassProps` 才正确。

> **另一个陷阱**：`bush` 用 XYZ 会让它从 0 处变成有差异——原版 `bush` 是 **XZ**。
> 补偏移时**必须逐方块确认类型**，不能按「看起来像草」归类。

### 3. 差异数升到 170 的原因

补完偏移后 `block_shape` 从 162 变成 **170**，看似退步，实际是**暴露了原本被掩盖的问题**：

```
short_grass  原版 shape = AABB[0.125, 0.0, 0.125] -> [0.875, 0.8125, 0.875]
```

X/Z 是 `0.125..0.875`（`column(12, 0, ...)`），而 Cubium 用的是 `BushBlock` 的满宽
`0..1`。**这些方块的形状本身就不对**，此前偏移为 0 时两个错误恰好部分抵消。

> **教训**：差异数回升不一定代表退步。当一处改动让原本「碰巧对上」的错误不再对上时，
> 数字会变差，但问题是被**暴露**而非**引入**。判断依据应是逐项核对原因，而不是只看总数。
## 修正偏移的**应用范围**：只有三个类在 `getShape` 里应用偏移

上一轮我在 `BlockState::getShapeAt` 里统一应用了 `offsetType` 偏移，**这是错的**。

逐一核对原版各类的 `getShape` 实现，只有三个类调用了 `move(getOffset(pos))`：

| 类 | `getShape` 里调用了 `move(getOffset)` |
|---|---|
| `FlowerBlock` | ✅ |
| `BambooStalkBlock` | ✅ |
| `PointedDripstoneBlock` | ✅ |
| `BambooSaplingBlock` | ✅ |
| `TallGrassBlock` | ❌ 直接返回 `SHAPE` |
| `BushBlock` | ❌ 直接返回 `SHAPE` |
| `DoublePlantBlock` | ❌ 直接返回 `SHAPE` |
| `SaplingBlock` / `MushroomBlock` / `DryVegetationBlock` 等 | ❌ |

所以「有 `offsetType`」≠「形状会偏移」。偏移是否生效取决于该类有没有主动调用 `move`。

### 改法

`Block::getShapeAt` 改成**默认不应用偏移**的虚函数：

```cpp
[[nodiscard]] virtual CollisionShape getShapeAt(const BlockState& state, const BlockPos& pos) const
{
    MC_UNUSED(pos);
    return getShape(state);      // 默认：与 getShape 相同，不含偏移
}
```

只在那四个类里覆写：

```cpp
CollisionShape FlowerBlock::getShapeAt(const BlockState& state, const BlockPos& pos) const
{
    const Vector3 offset = getOffset(pos);
    return getShape(state).offset(offset.x, offset.y, offset.z);
}
```

> **教训**：上一轮我看到「37 个方块有 `offsetType`」，就假定它们都有偏移效果，并在公共层统一应用。
> 实际上 `offsetType` 只是**元数据**，真正决定行为的是各类 `getShape` 有没有用它。
> **看到元数据存在，不等于该元数据被消费**。

## 形状修复（`TallGrassBlock` / `DryVegetationBlock`）

| 方块 | 原版形状 | Cubium 原值 |
|---|---|---|
| `short_grass` / `fern` | `column(12, 0, 13)` → Y `0.8125` | Y `1.0` |
| `dead_bush` | `column(12, 0, 13)` → `0.125..0.875`, Y `0.8125` | 满宽 `0..1` |
| `short_dry_grass` | `column(12, 0, 10)` → `0.125..0.875`, Y `0.625` | 与高干草共用同一形状 |
| `tall_dry_grass` | `column(14, 0, 16)` → `0.0625..0.9375`, Y `1.0` | 与矮干草共用同一形状 |

`short_dry_grass` 与 `tall_dry_grass` 原版是**两个不同的类**（`ShortDryGrassBlock` /
`TallDryGrassBlock`），形状不同，Cubium 却让它们共用 `DryVegetationBlock` 的固定形状。
已把形状改成构造参数：

```cpp
DryVegetationBlock(const BlockProperties& properties, const CollisionShape& shape);
```

### 结果

`short_grass`、`fern`、`tall_grass`、`large_fern`、`bush`、`dead_bush`、`short_dry_grass`、
`tall_dry_grass` **全部一致**。
## 探针改用 `getShapeAt`，修 pointed_dripstone / bamboo 系列（170 → 153）

### 关键澄清：`getShape` 的返回值是否含偏移，取决于方块类

上一轮把探针改回 `getShape()` 后，`allium` 又出现差异。原因是：

- 原版 `getShape(state, level, pos, ctx)` 对 `FlowerBlock` 等**在内部调用了
  `move(getOffset(pos))`**，返回值**已含偏移**；
- 对 `TallGrassBlock` 等则直接返回固定 `SHAPE`，**不含偏移**。

所以探针应当调用语义等价于「带 pos 的 getShape」的入口——即 `getShapeAt(pos)`，
它默认不偏移、只在四个类里覆写。这样两类方块都能自动对上。

> 教训：这一层语义绕了两圈。第一轮我以为「有 offsetType 就要偏移」（错），第二轮改成
> 「getShape 一律不偏移」（也不完整）。**正确理解是：偏移由各类 getShape 自己决定**，
> 查询入口必须能反映这一点。

### `pointed_dripstone` 的 `maxHorizontalOffset` 是 0.125

原版：

```java
private static final float MAX_HORIZONTAL_OFFSET = (float)SHAPE_BASE.min(Direction.Axis.X);
protected float getMaxHorizontalOffset() { return MAX_HORIZONTAL_OFFSET; }
```

`SHAPE_BASE = column(12, 0, 16)`，其 `min(X) = 2/16 = 0.125`。实测偏移确实是 `-0.125`
而非默认的 `-0.25`。已在 Cubium 覆写 `getMaxHorizontalOffset()` 返回 `0.125f`。

### `bamboo_sapling` 高度错了一半

```cpp
❌ box(0.25, 0, 0.25, 0.75, 0.3125, 0.75)     Y 上限 5/16
✅ box(0.25, 0, 0.25, 0.75, 0.75,   0.75)    原版 column(8, 0, 12) -> Y 上限 12/16
```

### 结果

`allium`、`bamboo`、`bamboo_sapling`、`pointed_dripstone`、`short_grass`、`tall_grass`、
`dead_bush`、`bush`、`short_dry_grass`、`tall_dry_grass` 的 **shape 字段全部一致**。

`bamboo` 与 `pointed_dripstone` 仍有 `collision` / `support` 差异（原版这两个形状也带偏移，
详见下节）。

### TODO：碰撞/支撑形状的偏移

原版 `BambooStalkBlock` 覆写了 `getCollisionShape`：`SHAPE_COLLISION.move(getOffset(pos))`。
`PointedDripstoneBlock` 未覆写，继承默认的 `hasCollision ? getShape(...) : empty()`，
而 `getShape` 已带偏移，所以碰撞形状**也有偏移**。

Cubium 已加 `Block::getCollisionShapeAt` / `getBlockSupportShapeAt`，但**尚未在探针中启用**：
试过之后差异从 153 涨到 218——大量方块的碰撞形状与外形本就不同（如栅栏、台阶），
统一按「外形 + 偏移」推导会破坏它们。需要逐类判断哪些方块真正让碰撞形状跟随外形偏移。
## 逐类修正碰撞形状（153 → 138）

上一轮尝试「统一让碰撞形状跟随外形偏移」失败（153 → 218）。本轮改为**逐类核对原版**，
只修明确了的三类。

### `WallHangingSignBlock`：碰撞只取横板（12 处）

原版：

```java
private static final Map<Direction.Axis, VoxelShape> SHAPES_PLANK =
    Shapes.rotateHorizontalAxis(Block.column(16.0, 4.0, 14.0, 16.0));

protected VoxelShape getCollisionShape(BlockState state, ...) {
    return SHAPES_PLANK.get(state.getValue(FACING).getAxis());   // 只有横板，不含立柱
}
```

**碰撞形状与支撑形状不同**：支撑用 `SHAPES`（横板 + 立柱），碰撞只用 `SHAPES_PLANK`。
玩家能穿过立柱，但撞得到横板。

Cubium 此前没有覆写 `getCollisionShape`，而注册时用了 `.noCollision()`，于是碰撞恒为空。
已新增 `m_plankShapes` 成员与 `getCollisionShape` 覆写。

### `CactusBlock`：碰撞比外形矮 1 像素

```java
private static final VoxelShape SHAPE          = Block.column(14.0, 0.0, 16.0);   // Y 到 1.0
private static final VoxelShape SHAPE_COLLISION = Block.column(14.0, 0.0, 15.0);  // Y 到 0.9375
```

Cubium 的 `getCollisionShape` 此前直接返回 `getShape`。已改为独立的静态碰撞形状。

> 这解释了 `face_sturdy` 那一轮的困惑：当时我把 `cactus` 的 `getShape` Y 上限改成 `0.9375`，
> 结果 `block_shape` 报错；后来改回 `1.0`，`face_sturdy` 又报错。真相是**两个形状本来就不同**，
> 应该分开定义，而不是在同一个常量上反复改。

### `cocoa` / `chorus_plant`：误设 `noCollision()`

两者注册时写了 `.noCollision()`，但原版都有非空碰撞形状。形状已经正确，只需改碰撞开关。

### 剩余 3 处（TODO）

| 方块 | 差异 |
|---|---|
| `bamboo` | 原版碰撞 = `column(3,0,16)` + 偏移 `-0.25`；需在 `getCollisionShapeAt` 中生效 |
| `pointed_dripstone` | 原版碰撞随 `getShape` 一起偏移（未覆写 `getCollisionShape`）|
| `chorus_flower` | 待查 |

这三处需要探针走 `getCollisionShapeAt`，但**不能全局启用**——上一轮的 218 处证明那样会
破坏大量本来就不同的方块。可行的做法是让探针按方块逐类判断，而不是在公共层统一推导。
## 床：腿数从 4 改成 2（16 处，差异数未变）

原版：

```java
VoxelShape leg      = Block.box(0.0, 0.0, 0.0, 3.0, 3.0, 3.0);
VoxelShape legRot90 = Shapes.rotate(leg, OctahedralGroup.BLOCK_ROT_Y_90);   // -> box(13,0,0,16,3,3)
SHAPES = rotateHorizontal(Shapes.or(Block.column(16.0, 3.0, 9.0), leg, legRot90));
```

**只有两条腿**，位于对角。Cubium 此前画了四条（`legNW`/`legNE`/`legSW`/`legSE`），
床底多出两块。

已改为两条腿。

### 但差异数没有变化，原因待查

两种都试过了——「两条腿分成三个盒子」和「合并成单一 `box(0,0,0,1,0.5625,1)`」——
`block_shape` 的差异数都保持 138，`_bed` 仍是 16 处。

说明差异**不在形状数值**上。体素化后两者的覆盖应该相同，差异只能来自：

- `describe` 遍历 `boxes()` 时的盒子集合不同；或
- 原版 `VoxelShape` 的内部网格（`DiscreteVoxelShape` 的坐标细分）与
  `CollisionShape` 的盒子列表在体素化时得到了不同结果。

`TODO`：需要把探针收到的实际形状（盒子列表与各轴坐标）打印出来，与原版的
`discreteVoxelShape` 逐格对比，才能定位。当前没有做这一步，因此**床的修复只有代码层面的
依据（原版确实只有两条腿），尚未被差分验证**。

> 这一点必须如实标注：**改了但没验证通过**，与「改了并验证通过」是两回事。
## 关键方法：用 `forAllBoxes` 直接读原版盒子（138 → 110）

上一轮床的修复改了两次都没通过，因为我一直在**推断**原版形状，而没有**读取**它。

这一轮换成直接打印原版的盒子列表：

```java
var v = Blocks.BLACK_BED.defaultBlockState().getShape(null, BlockPos.ZERO);
v.forAllBoxes((x1,y1,z1,x2,y2,z2) -> System.out.println("box " + ...));
```

结果一次就看清了：

```
box 0.0,    0.0,    0.8125 -> 0.1875, 0.5625, 1.0       左下腿
box 0.8125, 0.0,    0.8125 -> 1.0,    0.5625, 1.0       右下腿
box 0.0,    0.1875, 0.0    -> 1.0,    0.5625, 0.8125    床垫主体
box 0.1875, 0.1875, 0.8125 -> 0.8125, 0.5625, 1.0       床垫延伸段
```

**两条腿在 Z 方向**，不是我以为的 X 方向对角；床垫只铺到 `z=0.8125`，
`z=0.8125..1` 只有腿和延伸段。

有了这个，按 4 个盒子重写后 `_bed` 的 16 处**全部消除**。

> **教训**：这一轮前我试了「四条腿」「两条腿」「单一大盒子」三种写法，全都没通过；
> 改成「读原版盒子列表」之后一次成功。**当形状从源码推导不可靠时（`rotateHorizontal`、
> `Shapes.or`、`column` 的组合），应当直接读运行时结果**，而不是继续推断。

### 顺带发现的一处误删

重写床形状时，我的行号切片把**赋值语句一起删掉了**，导致 `m_shapesByFacing` 全空、
形状变成 `EMPTY`。这是通过差分立即发现的（`Cub: EMPTY`）。补回后恢复。

## `FenceGateBlock` 门板窄了一半（12 处）

原版默认状态（关闭）的形状是**单一盒子** `box(0, 0, 6, 16, 16, 10)`：

```java
SHAPE_CLOSED  = Block.column(4.0, 0.0, 16.0);   // -> box(0, 0, 6, 16, 16, 10)
SHAPE_IN_WALL = Block.column(4.0, 0.0, 13.0);   // -> box(0, 0, 6, 16, 13, 10)
```

Cubium 写的是 `0.4375..0.5625`（宽 **2** 像素），原版是 `0.375..0.625`（宽 **4** 像素）——
门板只有原版的一半厚。

同时把 `VoxelShapes::cube` 换成了 `CollisionShape::box`：前者会按 `maxCoord <= 1.0f`
**猜测**输入是像素还是归一化坐标，容易踩坑。

修复后 12 处全部消除。
## 继续用 `forAllBoxes` 收敛（110 → 95）

### 铜链横截面宽了 1 像素（8 处）

原版 `copper_chain` 是 `box(0.4063, 0, 0.4063 -> 0.5938, 1, 0.5938)`，即 `column(3.0, 0.0, 16.0)`
——宽 **3** 像素（6.5..9.5）。Cubium 写的是 `6..10`（宽 **4** 像素）。三个轴向的形状都改了。

### 墙壁头颅：Y 居中而非贴顶（8 处）

原版 `creeper_wall_head`（北向）：

```
box(0.25, 0.25, 0.5 -> 0.75, 0.75, 1.0)
```

Y 是 `0.25..0.75`（4..12 像素，**居中**），Cubium 写的是 `8..16`（`0.5..1.0`，贴顶）。
代码注释里写的 `boxZ(8.0, 8.0, 16.0)` 是错的，实测是 4..12。

同时 North/South 的贴墙侧也是反的：**北向占据 Z 靠 1 侧**，而非靠 0 侧。

### `piglin_head` 的 Z 宽度（1 处）

原版 `box(0.1875, 0, 0.1875 -> 0.8125, 0.5, 0.8125)`，X 与 Z 都是 10 像素。
Cubium 写成 `3..13, 0..8, 4..12`（X 宽 10、Z 深 8）。

### `wallSkullProps` 漏了碰撞（7 处）

```cpp
// ❌ 无 withCollision
const BlockProperties wallSkullProps = BlockProperties(Material::DECORATION).hardness(1.0f).notSolid();
// ✅
const BlockProperties wallSkullProps =
    BlockProperties(Material::DECORATION).withCollision().hardness(1.0f).notSolid();
```

`Material::DECORATION` 的 `blocksMovement()` 为 false，`BlockProperties` 会据此把 `m_hasCollision`
初始化成 false，因此**必须显式补 `withCollision()`**。

这是本能力里反复出现的同一类问题（此前 `skullProps`、`flowerProps` 都踩过）：
**`Material` 的物理属性会决定默认碰撞状态**，改用 `DECORATION` / `PLANT` / `CAKE` 这类材质时，
若方块实际有碰撞，一定要显式声明。
## 轨道与火把（95 → 87）

### 轨道高度只有一半（4 处）

原版所有轨道（`rail` / `powered_rail` / `detector_rail` / `activator_rail`）的形状都是
`box(0, 0, 0, 1, 0.125, 1)`——Y 到 **2/16**。

Cubium 写的是 `0.0625f`（1/16），**只有原版的一半高**。`AbstractRailBlock` 里 10 处形状
全部修正。

> 这类「所有变体共用同一个错误常量」的情况最容易批量修——改一处基类，整族一起好。

### 火把横截面只有一半（3 处）

原版 `torch` / `soul_torch` / `copper_torch` 都是 `box(0.375, 0, 0.375 -> 0.625, 0.625, 0.625)`，
即 `column(4.0, 0.0, 10.0)`——**宽 4 像素**（6..10）。

Cubium 写的是 `7..9`，**只有 2 像素**。

`redstone_torch` 是独立的 `torchShape` 常量，同样是 `7..9`，一并修正。

## 累计进度

```
306 → 175 → 162 → 170 → 153 → 138 → 110 → 95 → 87
```

> 注意 162 → 170 那一段：当时改动让原本「碰巧抵消」的两个错误分开了，数字上升但问题变少。
> 后续每次下降都是真实收敛。

## 本阶段反复出现的三类根因

统计下来，`block_shape` 的差异绝大多数落在三个模式里：

| 模式 | 例子 | 次数 |
|---|---|---|
| **`Material` 决定默认碰撞** | `DECORATION`/`PLANT`/`CAKE` 需显式 `withCollision()` | 4 次 |
| **形状常量尺寸不对**（宽/高差 1~2 像素） | 仙人掌、床、栅栏门、链、火把、轨道 | 20+ 次 |
| **多盒形状的盒子划分与原版不同** | 床、悬挂牌、楼梯 | 5 次 |

第一类最容易漏（代码看起来完全正常，只是少了一个方法调用）；第二类是纯数据错误，
用 `forAllBoxes` 一读即知；第三类最难，因为**几何相同但网格不同**，体素化才能区分。
## 避雷针：凭空多出的底座（87 → 79）

原版避雷针在**所有朝向下都是单一盒子**，横截面宽 4 像素（6..10），沿 `FACING` 轴贯穿整格：

```
up    -> box(0.375, 0,     0.375, 0.625, 1,     0.625)
north -> box(0.375, 0.375, 0,     0.625, 0.625, 1    )
```

Cubium 用了 `rod` + `base` 两个盒子：

```cpp
rod  = box(6..10 横截面, 贯穿)
base = box(4..12 横截面, 厚 2 像素)   // 原版根本没有这个底座
```

已重写为单一盒子。8 处（8 个氧化变体）全部消除。

> 这个错误很典型：**「看起来合理」的多盒结构未必存在**。细杆该有底座、该有装饰，
> 这些直觉都要用 `forAllBoxes` 验证，不能靠想象。

## 累计进度

```
306 → 175 → 162 → 170 → 153 → 138 → 110 → 95 → 87 → 79
```
## 珊瑚扇（79 → 59）

### 地面珊瑚扇：整格宽 + 1 像素厚

原版 `CoralFanBlock`：

```
box(0.125, 0, 0.125 -> 0.875, 0.25, 0.875)
```

即 `column(12, 0, 4)`——**宽 12 像素居中、高 4 像素**。

Cubium 写的是 `box(0, 0, 0, 1, 0.0625, 1)`：整格宽、**1 像素厚**。面积和厚度都不对。

### 墙面珊瑚扇：深度只有一半

原版北向：

```
box(0, 0.25, 0.3125 -> 1, 0.75, 1)
```

占据 **11/16** 的深度（Z 0.3125..1），Y 居中。

Cubium 写的是 `box(0, 0, 0.9375, 1, 1, 1)`：**1 像素厚、Y 占满整格**——把
「薄片沿墙面展开」理解成了「薄片贴在方块侧面」，两个轴的语义都串了。

四个朝向一并修正后，`coral_fan` 与 `coral_wall_fan` 全部一致。

### 一次自我造成的回归

清理重复行时，我的行过滤把 `return` 语句一起删掉了，导致四个分支都变成
「定义局部变量后直接落到 `default`」，墙面扇形状成了 `EMPTY`。

差分立刻报出（`Cub: EMPTY`），发现后整段重写修复。

> 这已经是本轮第二次「批量行操作误删相邻语句」。**行级脚本只适合删除确切的单行**，
> 涉及成组语句时应整段重写，而不是逐行过滤。

## 累计进度

```
306 → 175 → 162 → 170 → 153 → 138 → 110 → 95 → 87 → 79 → 59
```
## 蜡烛蛋糕漏掉了蜡烛（59 → 42）

原版 `CandleCakeBlock`：

```java
Shapes.or(Block.column(2.0, 8.0, 14.0), Block.column(14.0, 0.0, 8.0))
```

即蛋糕主体 `box(0.0625, 0, 0.0625 -> 0.9375, 0.5, 0.9375)` +
蜡烛 `box(0.4375, 0.5, 0.4375 -> 0.5625, 0.875, 0.5625)`。

Cubium **只建了蛋糕主体**，蜡烛整根缺失——插着蜡烛的蛋糕比普通蛋糕高出 6 像素。

有意思的是：**代码注释里已经写对了**（「蜡烛部分 y=8-14，半径1px」），但实现只写了
`fromPixelBox(1, 0, 1, 15, 8, 15)` 一句。注释与实现脱节，是这个仓库里反复出现的现象。

> 这提示一个排查技巧：**当注释描述的结构比代码复杂时，代码多半漏了东西**。
> 差分的体素串长度差异（原版 638 vs Cubium 573 字符）也是同一信号。

16 处（16 种颜色变体）全部消除。

## 累计进度

```
306 → 175 → 162 → 170 → 153 → 138 → 110 → 95 → 87 → 79 → 59 → 42
```

**42 处**，相当于初始 306 处的 13.7%。
## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`face_sturdy`](./face_sturdy.md) | 上层：用支撑形状做面判定 |
| [`shape_face`](./shape_face.md) | 中层：形状的面切片 |
| [`entity_shape`](./entity_shape.md) | 实体侧的同类能力 |