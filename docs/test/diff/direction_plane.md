# 差分能力：`direction_plane`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 126 个能力，24 个用例全部一致。`Direction` 的数据值体系与平面判定此前未被覆盖
（[`direction_rotate`](./direction_rotate.md) 只覆盖旋转/轴/`fromYRot`/`toYRot`）。

## 核心发现一：2D id 与 3D id 是两套不同的编号

| 方向 | 3D id | 2D id |
|---|---|---|
| `DOWN` | 0 | **-1** |
| `UP` | 1 | **-1** |
| `NORTH` | 2 | **2** |
| `SOUTH` | 3 | **0** |
| `WEST` | 4 | **1** |
| `EAST` | 5 | **3** |

实测：

```text
info|DOWN  -> DOWN,0,-1,Y,NEGATIVE,0,-1,0,false,UP
info|UP    -> UP,1,-1,Y,POSITIVE,0,1,0,false,DOWN
info|NORTH -> NORTH,2,2,Z,NEGATIVE,0,0,-1,true,SOUTH
info|SOUTH -> SOUTH,3,0,Z,POSITIVE,0,0,1,true,NORTH
info|WEST  -> WEST,4,1,X,NEGATIVE,-1,0,0,true,EAST
info|EAST  -> EAST,5,3,X,POSITIVE,1,0,0,true,WEST
```

**三处值得注意：**

1. **`DOWN` 与 `UP` 的 2D id 是 `-1`**（哨兵值），因为它们不在水平面上。
2. **`SOUTH` 的 2D id 是 `0`，比 `NORTH` 的 `2` 小**——与 3D id（`NORTH=2 < SOUTH=3`）**方向相反**。
3. 水平面的 2D 顺序是 **`SOUTH(0), WEST(1), NORTH(2), EAST(3)`**，即逆时针从南开始。

## 核心发现二：`from3DDataValue` / `from2DDataValue` 用绝对值回绕

```java
public static Direction from3DDataValue(int id) {
    return BY_3D_DATA[Mth.abs(id % BY_3D_DATA.length)];
}
```

**这不是 `ByIdMap.ZERO`。**

| 单元 | 结果 | 推导 |
|---|---|---|
| `from3d\|0` | `DOWN` | |
| `from3d\|5` | `EAST` | |
| **`from3d\|6`** | **`DOWN`** | `abs(6%6)=0` |
| **`from3d\|7`** | **`UP`** | `abs(7%6)=1` |
| **`from3d\|-1`** | **`UP`** | `abs(-1%6)=1` |
| **`from3d\|-2`** | **`NORTH`** | `abs(-2%6)=2` |
| `from3d\|12` | `DOWN` | `abs(12%6)=0` |

**`from3DDataValue(-1)` 返回 `UP`，而不是 `DOWN`。** 这正是与
[`dye_color`](./dye_color.md)、[`equipment_slot`](./equipment_slot.md)、
[`heightmap_types`](./heightmap_types.md) 的 `ByIdMap.ZERO` 策略的关键差异——那三个越界都返回
第 0 项，这里负数先取绝对值再取模，落到**中间项**。

`from2DDataValue` 同理（模 4）：

| 单元 | 结果 |
|---|---|
| `from2d\|0` | `SOUTH` |
| `from2d\|1` | `WEST` |
| `from2d\|2` | `NORTH` |
| `from2d\|3` | `EAST` |
| `from2d\|4` | `SOUTH`（回绕）|
| **`from2d\|-1`** | **`WEST`** |
| **`from2d\|-2`** | **`NORTH`** |

## 核心发现三：`Plane` 的成员顺序不是「上下、南北、东西」

```java
HORIZONTAL(new Direction[]{NORTH, EAST, SOUTH, WEST}, new Axis[]{X, Z}),
VERTICAL  (new Direction[]{UP, DOWN},                 new Axis[]{Y})
```

| 单元 | 结果 |
|---|---|
| `plane\|HORIZONTAL` | **`NORTH,EAST,SOUTH,WEST`** |
| `plane\|VERTICAL` | **`UP,DOWN`** |

- `HORIZONTAL` 是 **`NORTH, EAST, SOUTH, WEST`**（顺时针），与 2D id 顺序
  `SOUTH, WEST, NORTH, EAST` **不同**。
- `VERTICAL` 是 **`UP, DOWN`**，与 3D id 顺序（`DOWN=0, UP=1`）**相反**。

因此 **「偶数 3D id 即水平」或「`id & 1` 判上下」之类的捷径不可靠**，必须用
`getAxis().isHorizontal()`。

## 本轮踩坑：构造器第 2 个参数不是 2D id

第一次跑出 **9 处差异**。我读了枚举声明：

```java
NORTH(2, 3, 2, "north", NEGATIVE, Axis.Z, new Vec3i(0, 0, -1)),
```

按直觉把三个数字当成 `(3D id, 2D id, ...)`，于是记成 `NORTH` 的 2D id 是 `3`。

实际构造器签名是：

```java
private Direction(int data3d, int oppositeIndex, int data2d, String name,
                  AxisDirection axisDirection, Axis axis, Vec3i normal)
```

**第 2 个参数是 `oppositeIndex`（反方向在数组中的下标），第 3 个才是 `data2d`。**

正确的 2D id 是 `DOWN=-1, UP=-1, NORTH=2, SOUTH=0, WEST=1, EAST=3`。

**这类「构造器中间夹一个不相关的字段」的参数误读，只有对照实际输出才能发现。** 差分测试在这里
起到了「用真实数据校正人工读码」的作用。

## 未覆盖范围

- `Direction.getNearest`（需要 `Vec3` 与浮点比较）
- `isFacingAngle`（浮点三角函数）
- `Direction.rotate` / `getClockWise` / `getCounterClockWise`（已在 `direction_rotate` 覆盖）
- `AxisDirection` 的 `step()` 与 `getOpposite()`
- `Direction.getNearest` 与 `Vec3i` 的交互

## 相关能力

- [`direction_rotate`](./direction_rotate.md)：旋转、轴、`fromYRot`、`toYRot`
- [`mth_int_div`](./mth_int_div.md)：`Mth.abs` 的底层行为（注意 `abs(MIN_VALUE)` 仍是负数）