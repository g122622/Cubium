# 差分能力：`mth_approach`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 102 个能力，33 个逼近/钳制用例全部一致。实体朝向平滑的核心逻辑此前没有被比对过。

## 四个关键点

**1. `approach` 第一行就把 `step` 取绝对值。**

```java
public static float approach(float value, float limit, float step) {
    step = abs(step);
    return value < limit ? clamp(value + step, value, limit)
                         : clamp(value - step, limit, value);
}
```

传负步长**不会让逼近反向**——它变成正步长照常逼近：

| 单元 | 结果 |
|---|---|
| `approach(0, 100, -10)` | **10**（仍是 +10） |
| `approach(100, 0, -10)` | **90**（仍是 -10） |

这处「防御性 abs」静默吞掉了调用方的符号错误。想反向只能交换 `value` 与 `limit`。

**2. `clamp` 不校验 `min <= max`。**

```java
public static float clamp(float value, float min, float max) {
    return value < min ? min : Math.min(value, max);
}
```

实现是「先看下界，再用上界裁剪」，**不是** `max(min, min(value, max))` 那种对称写法。
当 `min > max` 时：

| 单元 | 结果 | 说明 |
|---|---|---|
| `clamp(5, 10, 0)` | **10** | `5 < 10`，直接返回 `min` = 10 |
| `clamp(0, 10, 0)` | **10** | 同上 |
| `clamp(15, 10, 0)` | **0** | `15 >= 10`，被 `max` 裁到 0 |

结果是「下界优先，上界只做裁剪」——**并不在 `[min, max]` 之间**（那个区间本身是空的）。
调用方若指望「夹在两者之间」，在 `min > max` 时会拿到超出预期的值。

**3. `approachDegrees` 先算差值再逼近，所以走短弧。**

```java
public static float approachDegrees(float from, float to, float step) {
    float f = degreesDifference(from, to);      // wrapDegrees(to - from)
    return approach(from, from + f, step);      // 注意目标是 from + f，不是 to
}
```

它把目标换成 `from + wrapDegrees(to - from)`——一个与 `from` 相差不超过 180 度的值，再对这个
目标做普通逼近。这样跨 0/360 度时走短弧：

| 单元 | `degreesDifference` | 结果 |
|---|---|---|
| `approachDegrees(350, 10, 5)` | `wrapDegrees(-340)` = **+20** | **355** |
| `approachDegrees(10, 350, 5)` | `wrapDegrees(340)` = **-20** | **5** |
| `approachDegrees(350, 10, 100)` | +20 | **370** |

**注意 `approachDegrees(350, 10, 100)` 的结果是 370，超出 `[0, 360)`**——这个函数只保证
「朝最短方向逼近」，不保证输出归一化。与 [`mth_rot`](./mth_rot.md) 里的 `rotLerp` 是同一类
行为。

**4. `degreesDifference` 端点不对称。**

`wrapDegrees` 的规则是 `>= 180` 就减 360，所以：

| 单元 | 结果 |
|---|---|
| `degreesDifference(0, 180)` | **-180** |
| `degreesDifference(0, 540)` | **-180** |
| `degreesDifference(0, 270)` | -90 |
| `degreesDifference(350, 10)` | 20 |

差 180 度时返回 -180 而不是 +180，方向选择偏向负向。详见 [`mth_modulo`](./mth_modulo.md)。

## 未覆盖范围

- `Mth.approach` 的 double 重载（原版只有 float 版用于角度，double 版结构相同）
- 调用方（`LivingEntity` 的朝向平滑、`Mob` 的转头）的完整 tick 流程