# 差分能力：`mth_lerp`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 98 个能力，42 个插值用例全部一致。`Mth` 的插值函数族此前没有被系统性比对过（`mth_rounding`
与 `mth_modulo` 覆盖的是取整与取模）。

## 四个关键点

**1. `clampedLerp` 是「越界直接返回端点」，不是「先钳制再插值」。**

```java
public static double clampedLerp(double d, double start, double end) {
    if (d < 0.0) return start;
    else return d > 1.0 ? end : lerp(d, start, end);
}
```

越界分支返回的是**入参 `start`/`end` 本身，逐位精确**。而 `lerp(clamp(d), start, end)` 会
先做一次乘加——在端点不能精确表示时（例如 `0.1`），结果可能差 1 ulp。

| 单元 | 结果 |
|---|---|
| `clamppd(-0.5, 0.1, 0.7)` | `0.1`（精确端点） |
| `clamppd(1.5, 0.1, 0.7)` | `0.7`（精确端点） |
| `clamppd(-1e-09, 10, 20)` | `10`（极小负值也走端点分支） |
| `clamppd(1.000000001, 10, 20)` | `20` |

**2. `lerpInt` 用 `floor` 而不是截断。**

```java
public static int lerpInt(float d, int start, int end) {
    return start + floor(d * (end - start));
}
```

`d` 允许越界，所以 `d * (end - start)` 为负是合法输入。`floor` 与 `static_cast<i32>`
（向零截断）在负值上差 1：

| 单元 | 计算 | 结果 |
|---|---|---|
| `lerpint(0.55, 10, 0)` | `10 + floor(0.55 * -10)` = `10 + floor(-5.5)` = `10 - 6` | **4** |
| 若用截断 | `10 + (int)(-5.5)` = `10 - 5` | 5（错） |

这条单元是整批里区分力最强的一条：`floor` 与截断的差异在正数区间完全看不出来，必须用
**负增量**才能暴露。

**3. `lerpDiscrete` 的公式与 `lerpInt` 不同。**

```java
public static int lerpDiscrete(float t, int start, int end) {
    int i = end - start;
    return start + floor(t * (i - 1)) + (t > 0.0F ? 1 : 0);
}
```

两个差别：用 `(i - 1)` 而不是 `i`；额外加 `t > 0 ? 1 : 0`。

| 单元 | 结果 | 说明 |
|---|---|---|
| `lerpdisc(0, 0, 10)` | **0** | `t = 0` 时 `t > 0` 为假，**不加 1** |
| `lerpdisc(1, 0, 10)` | **10** | `floor(1 * 9) + 1 = 10`，恰好到端点 |
| `lerpdisc(0.9999, 0, 10)` | **9** | 阶梯状，未到端点 |
| `lerpdisc(0.0001, 0, 10)` | **1** | 刚过 0 就跳到 1 |

**写成 `t >= 0` 会让 `t = 0` 的结果偏 1**，这是最容易犯的错。

**4. `float` 与 `double` 两个版本不能互相代用。** `lerp(float)` 在单精度下算完再提升，
`lerp(double)` 全程双精度。用双精度版算完后转 `float`，结果可能差 1 ulp。探针里两个版本
各自保留独立实现。

## 未覆盖范围

- `lerp3`（三维三线性插值，可由 `lerp2` 组合）
- `lerp(double, Vec3, Vec3)`（逐分量调用 `lerp`，已由 [`vec3_math`](./vec3_math.md) 覆盖
  向量侧）
- 密度函数与噪声烘焙里对 `lerp2` / `lerp3` 的封装