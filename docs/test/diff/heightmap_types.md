# 差分能力：`heightmap_types`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 124 个能力，33 个用例全部一致。高度图类型判定此前完全未被覆盖。

## 只有 6 种类型，三种谓词

| 类型 | id | Usage | 谓词 |
|---|---|---|---|
| `WORLD_SURFACE_WG` | 0 | `WORLDGEN` | `NOT_AIR` |
| `WORLD_SURFACE` | 1 | `CLIENT` | `NOT_AIR` |
| `OCEAN_FLOOR_WG` | 2 | `WORLDGEN` | `MATERIAL_MOTION_BLOCKING` |
| `OCEAN_FLOOR` | 3 | `LIVE_WORLD` | `MATERIAL_MOTION_BLOCKING` |
| `MOTION_BLOCKING` | 4 | `CLIENT` | `blocksMotion \|\| hasFluid` |
| `MOTION_BLOCKING_NO_LEAVES` | 5 | `CLIENT` | `(blocksMotion \|\| hasFluid) && !isLeaves` |

**id 连续 `0..5`，没有空洞**（与 [`equipment_slot`](./equipment_slot.md) 的 id 有空洞形成对比）。

六种类型其实是**三种谓词 × 生命周期**的组合：`WORLD_SURFACE_WG` 与 `WORLD_SURFACE` 谓词相同、
只是 `Usage` 不同；`OCEAN_FLOOR_WG` 与 `OCEAN_FLOOR` 同理。

## 核心差异一：液体

```java
MOTION_BLOCKING:      s -> s.blocksMotion() || !s.getFluidState().isEmpty()
MATERIAL_MOTION_BLOCKING (OCEAN_FLOOR): s -> s.blocksMotion()
```

| 单元 | 结果 | 说明 |
|---|---|---|
| **`test\|OCEAN_FLOOR\|0\|0\|1\|0`** | **`false`** | **水不算海底高度** |
| **`test\|MOTION_BLOCKING\|0\|0\|1\|0`** | **`true`** | **水算运动阻挡** |

水、岩浆这类**不阻挡移动但有流体**的方块，在 `MOTION_BLOCKING` 里算「高」，在 `OCEAN_FLOOR`
里**不算**。这是海洋高度图与运动阻挡高度图的核心差异——海底要找的是**固体底**，而不是水面。

## 核心差异二：树叶（本轮最重要的发现）

**原版 `LeavesBlock` 的 `blocksMotion()` 返回 `true`。** 树叶是阻挡移动的。

| 单元 | 结果 |
|---|---|
| `test\|OCEAN_FLOOR\|0\|1\|0\|1` | `true` |
| **`test\|MOTION_BLOCKING\|0\|1\|0\|1`** | **`true`** |
| **`test\|MOTION_BLOCKING_NO_LEAVES\|0\|1\|0\|1`** | **`false`** |

**这是 `MOTION_BLOCKING_NO_LEAVES` 存在的唯一理由**：因为树叶本身阻挡移动，`MOTION_BLOCKING`
会把树冠算进高度；`_NO_LEAVES` 变体把它排除。

我最初假设「树叶不阻挡移动，所以 `_NO_LEAVES` 是冗余的」，**这个假设被 oracle 的属性校验直接
捕获**——见下方「本轮踩坑」。

## 其余实测

| 单元 | 结果 |
|---|---|
| `test\|WORLD_SURFACE\|1\|0\|0\|0` | `false`（空气不是高度） |
| `test\|WORLD_SURFACE\|0\|0\|0\|0` | `true`（高草：非空气即算） |
| `test\|OCEAN_FLOOR\|0\|0\|0\|0` | `false`（高草不阻挡） |
| `test\|MOTION_BLOCKING\|0\|0\|0\|0` | `false` |
| `test\|MOTION_BLOCKING\|0\|1\|0\|0` | `true`（固体） |

`WORLD_SURFACE` 只看「非空气」，所以**高草也算高度**；`OCEAN_FLOOR` 只看「阻挡」，高草不算。
这是地表高度图与海底高度图的另一处差异。

## `byId` 越界返回 `WORLD_SURFACE_WG`

`OutOfBoundsStrategy.ZERO`，与 [`dye_color`](./dye_color.md)、[`equipment_slot`](./equipment_slot.md)
同一策略：

| 单元 | 结果 |
|---|---|
| `byId\|0` | `WORLD_SURFACE_WG` |
| `byId\|5` | `MOTION_BLOCKING_NO_LEAVES` |
| **`byId\|6`** | **`WORLD_SURFACE_WG`**（越界） |
| **`byId\|-1`** | **`WORLD_SURFACE_WG`** |
| **`byId\|999`** | **`WORLD_SURFACE_WG`** |

## 本轮踩坑：oracle 的属性校验捕获了我的错误假设

探针用 `<air>|<blocksMotion>|<hasFluid>|<isLeaves>` 四元组描述方块，oracle 必须找到属性
**完全匹配**的真实 `BlockState`。我在 `stateFor` 里写了一段**自校验**：

```java
boolean actualMotion = state.blocksMotion();
boolean actualLeaves = block instanceof LeavesBlock;
if (actualAir != air || actualMotion != blocksMotion || ...) {
    throw new IllegalStateException("方块的属性与四元组不符：" + ...);
}
```

第一次运行时报错：

```text
IllegalStateException: heightmap_types: 方块的属性与四元组不符：0001 -> minecraft:oak_leaves
```

**`0001` 是我写错的四元组**（`air=0, motion=0, fluid=0, leaves=1`）——我假设树叶不阻挡移动。
实际 `OAK_LEAVES` 的 `blocksMotion()` 是 `true`。

**这段自校验是本能力最有价值的设计**：它不是验证 Cubium，而是验证**我的测试数据本身**是否
对应真实方块。若没有它，两侧会「一致地」使用我编造的错误数据，测试通过但毫无意义。

**已沉淀为 `_patterns.md` 第十七条。**

## 局限：两处原版无对应方块的组合已移除

原计划包含 `0|1|0|1` 之外的「带流体树叶」（`0|0|1|1`）等组合，但**原版没有这类方块**，
oracle 无法找到真实状态来验证。这些用例已移除，而非登记为「已知缺口」——因为没有真实方块
意味着它们描述的是**不存在的情况**，测它没有意义。

## 未覆盖范围

- `Heightmap.getFirstAvailable`（需要真实 `ChunkAccess` 与逐 Y 扫描）
- `primeHeightmaps` 的批量更新
- `Usage` 在渲染/存档中的具体作用（只验证了三个枚举值与它们的公开布尔投影）
- `Heightmap.Types.getId()` 与 `usage()`（**均为私有**，oracle 用 `ordinal()` 与
  `sendToClient()`/`keepAfterWorldgen()` 反推）