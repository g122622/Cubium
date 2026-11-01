# 差分能力：`mth_degrees`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 140 个能力，29 个用例全部一致。补全了 [`mth_approach`](./mth_approach.md)（`approach`）之外的
角度差与朝向平滑函数族。

## 核心发现一：`degreesDifference(from, to)` 的参数是「起点, 终点」

```java
public static float degreesDifference(float from, float to) {
    return wrapDegrees(to - from);
}
```

返回值是「从 `from` 转到 `to` 需要的最短转角」，符号表示方向。

| 单元 | 结果 | 说明 |
|---|---|---|
| `diff\|0\|90` | `90` | 正转 |
| `diff\|90\|0` | `-90` | 反转 |
| **`diff\|170\|-170`** | **`20`** | 只需 20°（不是 -340°）|
| **`diff\|-170\|170`** | **`-20`** | 镜像 |
| **`diff\|0\|180`** | **`-180`** | 180° 被判为上界 → `-180` |
| `diff\|180\|0` | `-180` | |

**`diff|0|180` 得 `-180`**：`wrapDegrees(180)` 返回 `-180`（输出区间 `(-180, 180]`），所以
「转 180°」被记为 `-180`。

`degreesDifferenceAbs` 取绝对值：`diffAbs|170|-170` 与 `diffAbs|-170|170` 都得 `20`。

## 核心发现二：`rotateIfNecessary` 从 `target` 反推，不是从 `current` 前进

```java
public static float rotateIfNecessary(float current, float target, float maxStep) {
    float diff = degreesDifference(current, target);
    float clamped = clamp(diff, -maxStep, maxStep);
    return target - clamped;                    // 从 target 反推
}
```

直觉写法是 `current + clampedStep`，但原版是 **`target - clampedDiff`**。

| 单元 | 结果 |
|---|---|
| `rotate\|0\|90\|10` | `80` |
| `rotate\|0\|90\|100` | `0` |
| `rotate\|90\|0\|10` | `10` |
| **`rotate\|170\|-170\|5`** | **`-175`** |
| **`rotate\|-170\|170\|5`** | **`175`** |
| **`rotate\|170\|-170\|100`** | **`-190`** |

### 语义：返回值是「转过之后的新角度」

`rotateIfNecessary` 返回的不是「转过的步长」，而是**「应用这一步之后 `current` 应变成的
角度」**。

`rotate|0|90|10`：`diff = degreesDifference(0, 90) = 90`，`clamp(90, -10, 10) = 10`，
`90 - 10 = 80`。

含义：目标 90°，这一 tick 最多转 10°，所以**停在 80°**（还差 10°）。下一 tick 再调用会继续
逼近 90°。

若把返回值误当成「步长」并写成 `current + 返回值`，会得到 `0 + 80 = 80`——巧合正确；
但下一 tick 传入 `current = 80` 时，`diff = 10`，`clamp(10,-10,10) = 10`，返回
`90 - 10 = 80`，**永远停在 80°**。

**`rotate|0|90|100` 得 `0`**：`clamp(90, -100, 100) = 90`，`90 - 90 = 0`。步长足够
走完全程时，返回值是 **`0`**（即「已经到达目标，无需再转」的约定值），不是 `90`。

### 跨边界时从 target 反推是必需的

`rotate|170|-170|5`：

- `diff = wrapDegrees(-170 - 170) = wrapDegrees(-340) = 20`
- `clamp(20, -5, 5) = 5`
- `-170 - 5 = **-175**`

若从 `current` 前进（`170 + 5 = 175`），虽然角度等价（差 360），但**数值不同，会写进实体数据**。

## 核心发现三：`approachDegrees` 与 `rotateIfNecessary` 方向相反

```java
public static float approachDegrees(float current, float target, float maxStep) {
    float diff = degreesDifference(current, target);
    return approach(current, current + diff, maxStep);
}
```

`approachDegrees` 从 **`current` 前进**，`rotateIfNecessary` 从 **`target` 反推**。

| 单元 | 结果 |
|---|---|
| `approachDeg|0|90|10` | `10`（从 0 前进到 10）|
| `approachDeg|0|90|100` | `90`（直达目标）|
| `approachDeg|90|0|10` | `80`（从 90 后退到 80）|
| **`approachDeg|170|-170|10`** | **`180`** |
| **`approachDeg|-170|170|10`** | **`-180`** |

**`approachDeg|170|-170|10` 得 `180`**：`diff = 20`，`approach(170, 190, 10)` →
`clamp(170 + 10, 170, 190) = 180`。结果是 `180` 而不是 `-180`（虽然等价）。

与 `rotate|170|-170|5` 的 `-175` 对比：**两个函数在同样的输入下给出了符号相反的结果**。

## 核心发现四：`absMax` 在 `Integer.MIN_VALUE` 上溢出，但结果仍是 0

```java
public static int absMax(int a, int b) {
    return Math.max(Math.abs(a), Math.abs(b));
}
```

`Math.abs(Integer.MIN_VALUE)` 返回 **`Integer.MIN_VALUE`**（仍是负数，因为 `-MIN_VALUE` 在补码里
溢出回自己）。

| 单元 | 结果 | 说明 |
|---|---|---|
| `absMaxI\|5\|-10` | `10` | 正常 |
| **`absMaxI\|-2147483648\|0`** | **`0`** | `max(MIN_VALUE, 0) = 0` |
| **`absMaxI\|-2147483648\|5`** | **`5`** | `max(MIN_VALUE, 5) = 5` |

**`absMaxI|-2147483648|0` 得 `0` 而不是负数**——因为 `Math.max(负数, 0)` 正确地返回了 `0`。

**这个溢出在实践中被 `Math.max` 掩盖了**：只有当**两个参数都是 `MIN_VALUE`** 时才会返回负数
（`max(MIN_VALUE, MIN_VALUE) = MIN_VALUE`）。

`absMaxF` / `absMaxD` 无此问题：`absMaxF|-3.5|2` 与 `absMaxD|-3.5|2` 都得 `3.5`。

## 核心发现五：`sign` 把 `NaN` 判为 `-1`

```java
public static int sign(double value) {
    if (value == 0.0) return 0;
    return value > 0.0 ? 1 : -1;
}
```

| 单元 | 结果 |
|---|---|
| `sign|0` | `0` |
| `sign|5` | `1` |
| `sign|-5` | `-1` |
| `sign|0.0001` | `1` |
| `sign|-0.0001` | `-1` |

`NaN == 0.0` 为 `false`，`NaN > 0.0` 也为 `false`，因此 **`sign(NaN)` 走 `-1`**。
这与 `Math.signum(NaN)` 返回 `NaN` 不同。

## 本轮踩坑：我的预测与实际不符，但探针是对的

写文档时我最初预测「`absMaxI|-2147483648|0` 返回负数」，实测得到 `0`——因为 `Math.max` 把溢出
结果掩盖了。**探针照抄了 `Math.max(Math.abs(a), Math.abs(b))`，所以一侧是对的，是我的推理错了。**

**教训**：即使理解了底层溢出的机制，也**不能跳过「照抄表达式 + 实测」这一步**。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_approach`](./mth_approach.md) | `approach`（本能力的 `approachDegrees` 内部依赖）|
| [`mth_angle_lerp`](./mth_angle_lerp.md) | `wrapDegrees` / `rotLerp`（本能力的 `degreesDifference` 依赖）|
| [`mth_pack_degrees`](./mth_pack_degrees.md) | 角度单字节压缩 |
| [`mth_inverse_lerp`](./mth_inverse_lerp.md) | 插值族 |

## 未覆盖范围

- `Mth.rotateIfNecessary` 在实体朝向 tick 中的完整调用链（属世界型）
- `Mth.approach` 的 `double` 重载（原版只有 `float` 版）
- `wrapDegrees(long)` 重载（`mth_angle_lerp` 已覆盖 `int`、`float` 版）
- `absMax` 两个参数都为 `Integer.MIN_VALUE` 的边界（未纳入用例）