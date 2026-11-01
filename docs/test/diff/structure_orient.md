# 差分能力：`structure_orient`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 150 个能力，14 个用例全部一致。补全了 [`bounding_box`](./bounding_box.md) 文档中列出的
`orientBox`（结构旋转与镜像）缺口。

## 核心发现一：`UP` / `DOWN` 走 `default` 分支，结果与 `SOUTH` 完全相同

```java
switch (direction) {
    case SOUTH:
    default:
        return new BoundingBox(...);   // 不旋转
    case NORTH: ...
    case WEST: ...
    case EAST: ...
}
```

**`switch` 里没有 `UP` / `DOWN` 的 case**——它们落到 `default`，与 `SOUTH` 走同一段代码。

| 单元 | 结果 |
|---|---|
| `0\|0\|0\|0\|0\|0\|3\|2\|4\|south` | `0,0,0,2,1,3` |
| **`0\|0\|0\|0\|0\|0\|3\|2\|4\|up`** | **`0,0,0,2,1,3`** |
| **`0\|0\|0\|0\|0\|0\|3\|2\|4\|down`** | **`0,0,0,2,1,3`** |

结构生成中 `orientBox` 只处理四个**水平**方向；垂直方向等价于「不旋转」。

## 核心发现二：只有 `NORTH` 让 Z 轴反向

```java
case NORTH:
    return new BoundingBox(
        x + offsetX,                 y + offsetY,                 z - sizeZ + 1 + offsetZ,
        x + sizeX - 1 + offsetX,     y + sizeY - 1 + offsetY,     z + offsetZ);
```

| 单元 | 结果 | `minZ` |
|---|---|---|
| `0\|0\|0\|0\|0\|0\|3\|2\|4\|north` | **`0,0,-3,2,1,0`** | `z - sizeZ + 1 = -3` |
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|north` | `11,66,-20,13,67,-17` | `-20 - 4 + 1 + 3 = -20` |

**`north` 的 Z 区间是 `[-3, 0]`，而 `south` 是 `[0, 3]`**——整体镜像到原点另一侧。

`maxZ` 只是 `z + offsetZ`（不含 `sizeZ`），`minZ` 才含 `-sizeZ + 1`。

## 核心发现三：`WEST` / `EAST` 交换 X 与 Z 的角色

```java
case WEST:
    return new BoundingBox(
        x - sizeZ + 1 + offsetZ,  ..., z + offsetX,
        x + offsetZ,              ..., z + sizeX - 1 + offsetX);
case EAST:
    return new BoundingBox(
        x + offsetZ,              ..., z + offsetX,
        x + sizeZ - 1 + offsetZ,  ..., z + sizeX - 1 + offsetX);
```

**`WEST`/`EAST` 的 X 范围用 `sizeZ`，Z 范围用 `sizeX`**——因为结构转了 90°，X/Z 尺寸互换。

| 方向 | `minX` | `maxX` | `minZ` | `maxZ` |
|---|---|---|---|---|
| `SOUTH` | `x + offsetX` | `x + sizeX - 1 + offsetX` | `z + offsetZ` | `z + sizeZ - 1 + offsetZ` |
| `NORTH` | `x + offsetX` | `x + sizeX - 1 + offsetX` | `z - sizeZ + 1 + offsetZ` | `z + offsetZ` |
| **`WEST`** | **`x - sizeZ + 1 + offsetZ`** | **`x + offsetZ`** | **`z + offsetX`** | **`z + sizeX - 1 + offsetX`** |
| **`EAST`** | **`x + offsetZ`** | **`x + sizeZ - 1 + offsetZ`** | `z + offsetX` | `z + sizeX - 1 + offsetX` |

| 单元 | 结果 |
|---|---|
| `0\|0\|0\|0\|0\|0\|3\|2\|4\|west` | **`-3,0,0,0,1,2`** |
| `0\|0\|0\|0\|0\|0\|3\|2\|4\|east` | **`0,0,0,3,1,2`** |

两者**共享 Z 区间 `[0, 2]`**（`sizeX - 1 = 2`），差别只在 X：
`west` 是 `[-3, 0]`（反向），`east` 是 `[0, 3]`（正向）。

## 核心发现四：Y 轴在所有方向上都不变

所有四个分支的 Y 部分都是：

```java
y + offsetY,   y + sizeY - 1 + offsetY
```

**没有任何分支对 Y 做镜像或交换**——结构生成只绕 Y 轴旋转。

| 单元 | `minY` / `maxY` |
|---|---|
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|south` | `66` / `67` |
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|north` | `66` / `67` |
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|west` | `66` / `67` |
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|east` | `66` / `67` |

## 其余实测

### 带偏移的完整用例

| 单元 | 结果 |
|---|---|
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|south` | `11,66,-17,13,67,-14` |
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|north` | `11,66,-20,13,67,-17` |
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|west` | `10,66,-19,13,67,-17` |
| `10\|64\|-20\|1\|2\|3\|3\|2\|4\|east` | `13,66,-19,16,67,-17` |

### 单方块退化

| 单元 | 结果 |
|---|---|
| `5\|5\|5\|0\|0\|0\|1\|1\|1\|north` | `5,5,5,5,5,5` |
| `5\|5\|5\|0\|0\|0\|1\|1\|1\|west` | `5,5,5,5,5,5` |

尺寸为 1 时所有方向都退化成一个点——**这掩盖了 X/Z 交换的 bug**。

### 正方体尺寸也掩盖 bug

| 单元 | 结果 |
|---|---|
| `0\|0\|0\|1\|1\|1\|2\|2\|2\|west` | `0,1,1,1,2,2` |
| `0\|0\|0\|1\|1\|1\|2\|2\|2\|east` | `1,1,1,2,2,2` |

`sizeX == sizeZ` 时交换看不出来。**用例必须包含非立方尺寸**（本能力用了 `3×2×4`）才能暴露
X/Z 混淆。

## 本轮情况：一次通过

`orientBox` 是 10 个参数的纯算术函数。写代码前逐行抄录了四个 `case` 的九个坐标表达式。

关键是把 **`WEST` 的 `minX = x - sizeZ + 1 + offsetZ`** 与 **`EAST` 的 `maxX = x + sizeZ - 1 + offsetZ`**
里的 `sizeZ`（而不是 `sizeX`）抄对——这是最容易写错的地方，因为直觉上 `minX` 应该用 `sizeX`。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`bounding_box`](./bounding_box.md) | 包围盒判定（本能力补全它的 `orientBox` 缺口）|
| [`block_transform`](./block_transform.md) | `Rotation` / `Mirror` 的坐标变换 |
| [`front_and_top`](./front_and_top.md) | `FrontAndTop` 的朝向组合 |
| [`rotation_info`](./rotation_info.md) | `Rotation` 枚举信息 |

## 未覆盖范围

- `BoundingBox.fromCorners`（两个 `Vec3i` 的包围盒，已由 `bounding_box` 覆盖）
- `BoundingBox.encapsulatingPositions` / `encapsulatingBoxes`（`Iterable` 的封装）
- `BoundingBox.intersectingChunks`（区块相交，依赖 `ChunkPos`）
- `BoundingBox.CODEC` / `STREAM_CODEC`（序列化）
- `orientBox` 在 `Template` / `StructureTemplate` 中的实际调用链（属 world 型）