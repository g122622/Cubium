# 差分能力：`mth_rot`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 100 个能力，33 个旋转插值用例全部一致。旋转插值此前完全空白。

## 四个关键点

**1. `rotLerp` 走最短弧，但结果不归一化。**

```java
public static float rotLerp(float delta, float start, float end) {
    return start + delta * wrapDegrees(end - start);
}
```

它先把 `end - start` 用 `wrapDegrees` 折到 `[-180, 180)`，再插值。**但插值结果本身不被
折回 `[0, 360)`**：

| 单元 | 增量 | 结果 |
|---|---|---|
| `rotLerp(0.5, 350, 10)` | `wrapDegrees(-340)` = **20** | **360** |
| `rotLerp(0.5, 10, 350)` | `wrapDegrees(340)` = **-20** | **0** |
| `rotLerp(1, 350, 10)` | 20 | **370** |
| `rotLerp(0, 350, 10)` | 20 | 350 |

`delta = 1` 时结果可以是 370——**调用方不能假设返回值在 `[0, 360)` 内**。这个函数只保证
「插值路径是最短弧」，不保证「输出已归一化」。

**2. 差值恰好 180 时走负方向。**

`rotLerp(0.5, 0, 180)` 的增量是 `wrapDegrees(180)`。由于 `wrapDegrees` 的规则是
`>= 180` 就减 360，`180` 被折成 `-180`，所以结果是 **-90** 而不是 90。
反向的 `rotLerp(0.5, 180, 0)` 增量是 `wrapDegrees(-180) = -180`（不减），结果是 90。

**两个方向的增量都是 -180**，都朝负方向插值——这是 `wrapDegrees` 端点不对称的直接后果
（见 [`mth_modulo`](./mth_modulo.md)）。

**3. `rotLerpRad` 用 `while` 循环，不是 `wrapDegrees`。**

```java
float f = end - start;
while (f < (float) -Math.PI) { f += (float) (Math.PI * 2); }
while (f >= (float) Math.PI)  { f -= (float) (Math.PI * 2); }
return start + delta * f;
```

| 单元 | 结果 |
|---|---|
| `rotLerpRad(0.5, 0, 12.566370)`（4π） | **-4.77e-07** |
| `rotLerpRad(0.5, 0, -12.566370)` | **4.77e-07** |

**多圈差值的回绕会留下浮点残差，结果不是精确的 0。** 双精度 π 转成 `float` 后每加/减一次
`2π` 就引入一点误差，多次循环后累积成 `~5e-7`。这个量级在大多数场景可忽略，但对比时
必须逐位相同。

边界用 `(float) Math.PI`，不是双精度 `PI`。`f >= PI` 才减，所以恰好等于 `PI` 会被折成
`-PI`——区间是 `[-PI, PI)`。

**4. `triangleWave` 对负输入超出 `[-1, 1]`。**

```java
public static float triangleWave(float input, float period) {
    return (Math.abs(input % period - period * 0.5F) - period * 0.25F) / (period * 0.25F);
}
```

Java 的 `%` 符号跟随被除数，负输入的 `input % period` 是负数：

| 单元 | `input % period` | 结果 |
|---|---|---|
| `triangleWave(0, 1)` | 0 | **1** |
| `triangleWave(0.25, 1)` | 0.25 | 0 |
| `triangleWave(0.5, 1)` | 0.5 | **-1** |
| `triangleWave(-0.25, 1)` | **-0.25** | **2** |
| `triangleWave(-0.5, 1)` | **-0.5** | **3** |
| `triangleWave(-2.5, 10)` | **-2.5** | **2** |

**负输入的结果可以是 2 或 3，完全在 `[-1, 1]` 之外。** 我在写能力注释时曾经写成「输出范围
是 `[-1, 1]`」，实测证明那是错的——只有非负输入才落在 `[-1, 1]`。原版调用方（如云层高度、
附魔光效）都保证传入非负值，但工具函数本身不做这个保证。

## 未覆盖范围

- `Mth.sin` / `Mth.cos`（大角度查表近似，需单独能力）
- `Mth.approachDegrees`（带步长的角度逼近，逻辑与 `rotLerp` 不同）