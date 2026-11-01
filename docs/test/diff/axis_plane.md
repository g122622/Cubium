# 差分能力：`axis_plane`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 97 个能力，30 个轴/平面判定用例全部一致。补齐 [`direction_rotate`](./direction_rotate.md)
未覆盖的部分。

## 三个关键点

**1. `Axis.choose(x, y, z)` 是按轴取对应分量，不是「选最大/最小」。**

```java
X("x") { public int choose(int x, int y, int z) { return x; } },
Y("y") { public int choose(int x, int y, int z) { return y; } },
Z("z") { public int choose(int x, int y, int z) { return z; } },
```

用固定三分量 `(10, 20, 30)` 实测：`X` 返回 10、`Y` 返回 20、`Z` 返回 30。

`int` / `double` / `boolean` 三个重载结构完全一致。这个函数在坐标分量按轴投影时被广泛
调用（`BlockPos` 距离、`AABB` 分轴比较等），把轴和分量对应错是典型的静默 bug。

**2. `getPositive` / `getNegative` 里 Z 轴是反的。**

| 轴 | `getPositive` | `getNegative` |
|---|---|---|
| X | `EAST` | `WEST` |
| Y | `UP` | `DOWN` |
| **Z** | **`SOUTH`** | `NORTH` |

**`+Z` 指向南，不是北。** 这一处最容易写反——因为 X 轴是「正东」、Y 轴是「正上」，直觉
会顺推出 Z 轴「正北」，而 Minecraft 的坐标系里 `NORTH` 是 `-Z`。

**3. `Plane.test(null)` 返回 `false`，不抛异常。**

```java
public boolean test(@Nullable Direction d) {
    return d != null && d.getAxis().getPlane() == this;
}
```

`AxisDirection` 与 `Direction` 都可能为空，这个 `null` 守卫在渲染、光照更新等链路上被
依赖。把它写成 `d.getAxis()` 直接解引用会在原版容忍的场景下崩溃。

## 平面与轴的对应

| 平面 | 轴集合 | 方向数组（迭代顺序） |
|---|---|---|
| `HORIZONTAL` | `{X, Z}` | `NORTH, EAST, SOUTH, WEST` |
| `VERTICAL` | `{Y}` | `UP, DOWN` |

注意 `HORIZONTAL` 的**方向数组顺序是 `NORTH, EAST, SOUTH, WEST`**——这是顺时针对角。
依赖遍历下标的代码（例如 `faces[(index + 1) % 4]` 取下一个方向）必须用这个顺序，用
`N, S, E, W` 或枚举顺序都会错位。

轴集合的顺序是 `{X, Z}` 而不是 `{Z, X}`，`Plane.getRandomAxis` 依赖它。

## 未覆盖范围

- `Plane.getRandomDirection` / `getRandomAxis`（依赖随机源）
- `Plane.iterator` / `stream`（迭代顺序已在上文记录，但未用单元钉住）