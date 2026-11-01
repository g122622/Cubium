# 差分能力：`direction_nearest`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 137 个能力，29 个用例全部一致。补全了 [`direction_rotate`](./direction_rotate.md)
（`fromYRot` / `toYRot`）之外的「由向量反推方向」路径。

## 核心发现一：全零向量返回 `north`，因为初值是 `Float.MIN_VALUE`

```java
public static Direction getApproximateNearest(float x, float y, float z) {
    Direction direction = NORTH;
    float f = Float.MIN_VALUE;
    for (Direction candidate : VALUES) {
        float dot = x * n.getX() + y * n.getY() + z * n.getZ();
        if (dot > f) {          // 严格大于
            f = dot;
            direction = candidate;
        }
    }
    return direction;
}
```

**`Float.MIN_VALUE` 是最小正规格化正数（约 `1.4e-45`），不是最负的浮点数**（那应该是
`-Float.MAX_VALUE`）。

| 单元 | 结果 |
|---|---|
| **`approxF\|0\|0\|0`** | **`north`** |
| **`approxF\|1e-46\|1e-46\|1e-46`** | **`north`**（低于阈值）|
| `approxF\|1\|0\|0` | `east` |
| `approxF\|0\|0\|-1` | `north` |

**全零向量时所有点积都是 0，`0 > 1.4e-45` 为假，循环一次都不更新**，于是返回默认值 `NORTH`。

这意味着**无法从结果反推「是否为零向量」**——`(0,0,-1)` 也返回 `north`。

若 Cubium 把初值写成 `-Float.MAX_VALUE` 或 `-Infinity`，则全零向量会选中第一个点积为 0 的方向
（`DOWN`，因为 `0 > -Infinity`），**与 `north` 不同**。

## 核心发现二：平局由枚举顺序决定

`VALUES = values()`，枚举声明顺序是 **`DOWN, UP, NORTH, SOUTH, WEST, EAST`**（索引 0–5）。

循环用**严格** `>`，所以**先遍历到的胜出**。

| 单元 | `up` 点积 | `east` 点积 | 胜出 |
|---|---|---|---|
| **`approxF\|1\|1\|0`** | 1 | 1 | **`up`**（索引 1 < 5）|
| **`approxF\|1\|0\|1`** | 0 | `south`=1, `east`=1 | **`south`**（索引 3 < 5）|
| **`approxF\|1\|1\|1`** | 1 | 1 | **`up`** |

`approxF|1|0|1 → south` 尤其隐蔽：`x=1, z=1` 时 `EAST` 与 `SOUTH` 的点积都是 1，`SOUTH` 在枚举中
位于 `WEST`/`EAST` 之前，因此胜出。

**`(1,1,1)` 这种「等角向量」没有返回 `up` 之外的任何方向**——`UP` 索引最早，一旦点积相平就锁定。

## 核心发现三：`double` 版先降精度到 `float`

```java
public static Direction getApproximateNearest(double x, double y, double z) {
    return getApproximateNearest((float) x, (float) y, (float) z);
}
```

| 单元 | 结果 |
|---|---|
| `approxD\|1\|0\|0` | `east` |
| **`approxD\|1e-300\|0\|0`** | **`north`** |
| `approxD\|0\|0\|0` | `north` |

**`1e-300` 是合法 `double`，但降成 `float` 后变成 `0.0F`**，因此走「全零」路径返回 `north`。

**两个重载的行为完全一致**，只是入口不同——不存在「double 版更精确」的情况。

## 核心发现四：`getNearest` 平局回退到 `fallback`，可以是 `null`

```java
int i = Math.abs(x), j = Math.abs(y), k = Math.abs(z);
if (i > k && i > j)      return x < 0 ? WEST : EAST;
else if (k > i && k > j) return z < 0 ? NORTH : SOUTH;
else if (j > i && j > k) return y < 0 ? DOWN : UP;
else                     return fallback;
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `nearest\|5\|1\|1\|north` | `east` | x 主导 |
| `nearest\|-5\|1\|1\|north` | `west` | x 负 |
| `nearest\|1\|5\|1\|north` | `up` | y 主导 |
| `nearest\|1\|-5\|1\|north` | `down` | y 负 |
| `nearest\|1\|1\|5\|north` | `south` | z 主导 |
| `nearest\|1\|1\|-5\|north` | `north` | z 负 |
| **`nearest\|1\|1\|0\|north`** | **`north`** | 平局 → fallback |
| **`nearest\|1\|0\|1\|north`** | **`north`** | 平局 → fallback |
| **`nearest\|1\|1\|1\|north`** | **`north`** | 三轴平局 → fallback |
| **`nearest\|0\|0\|0\|null`** | **`null`** | fallback 为 null |

**`fallback` 为 `null` 是合法路径**，`getNearest` 的返回类型是 `@Nullable Direction`。

**与 `getApproximateNearest` 的关键区别**：`getNearest` 在平局时**完全不猜**，直接交还给调用方；
而 `getApproximateNearest` 用枚举顺序打破平局。

## 本轮情况：一次通过

写代码前完整读取了 `getApproximateNearest` 的两个重载、`getNearest` 的实现，以及 `VALUES` 的
定义（`values()` 按声明顺序）。关键是把 `Float.MIN_VALUE` 正确理解为「最小正数」而非「最小数」。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`direction_rotate`](./direction_rotate.md) | `fromYRot` / `toYRot`（角度反推）|
| [`direction_plane`](./direction_plane.md) | `Direction` 的构造器参数与 `Axis` / `Plane` |
| [`vec3_math`](./vec3_math.md) | 向量点积与归一化 |

## 未覆盖范围

- `Direction.orderedByNearest(Entity)`（需要实体朝向）
- `Direction.rotate(Matrix4fc, Direction)`（需要矩阵）
- `Direction.axisStepOrder(Vec3)`（返回 `ImmutableList<Axis>`）
- `Direction.getFacingAxis(Entity, Axis)`
- `Direction.getRandom(RandomSource)`（需要随机源）
- `getApproximateNearest(Vec3)`（先取 `x/y/z` 再降 float，与 float 重载等价）