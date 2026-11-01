# 差分能力：`mth_inverse_lerp`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 132 个能力，22 个用例全部一致。这是 [`mth_lerp`](./mth_lerp.md)（第 98 个能力）的**补充**
——后者覆盖 `clampedLerp` / `lerp2` / `lerp` / `lerpDiscrete` / `lerpInt`，本能力覆盖其文档中明确
列出的未覆盖项 `inverseLerp` 与 `lerp3`。

## 核心发现一：`inverseLerp` 不做任何 clamp，也不保护 `from == to`

```java
public static double inverseLerp(double value, double from, double to) {
    return (value - from) / (to - from);
}
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `invD\|5\|0\|10` | `0.5` | 正常 |
| `invD\|0\|0\|10` | `0` | 下界 |
| `invD\|10\|0\|10` | `1` | 上界 |
| **`invD\|-5\|0\|10`** | **`-0.5`** | **不 clamp** |
| **`invD\|15\|0\|10`** | **`1.5`** | **不 clamp** |
| **`invD\|0\|0\|0`** | **`NaN`** | `0/0` |
| **`invD\|5\|0\|0`** | **`Infinity`** | `5/0` |
| **`invD\|-5\|0\|0`** | **`-Infinity`** | `-5/0` |

**与 `clampedLerp` 形成鲜明对比**：`clampedLerp` 会把结果夹到 `[from, to]`，而 `inverseLerp` 是
「求比例」，不负责夹取。

**退化区间（`from == to`）没有任何保护**，直接产生 `NaN` 或 `±Infinity`。若 Cubium 加上了
`if (from == to) return 0;` 之类的防护，就会与原版不同。

## 核心发现二：分母是 `to - from`，反向区间会翻转符号

```java
return (value - from) / (to - from);
```

| 单元 | 结果 | 推导 |
|---|---|---|
| `invD\|5\|10\|0` | `0.5` | `(5-10)/(0-10) = -5/-10` |
| **`invD\|2.5\|10\|0`** | **`0.75`** | `(2.5-10)/(0-10) = -7.5/-10` |

反向区间（`from > to`）时分母为负，结果符号也随之翻转，但**数值仍是 `[0,1]` 内的比例**。

## 核心发现三：`lerp3` 的外层权重是**第 3 个**参数

```java
public static double lerp3(
    double p1, double p2, double p3, double p4, double p5, double p6,
    double p7, double p8, double p9, double p10, double p11) {
    return lerp(p3, lerp2(p1, p2, p4, p5, p6, p7), lerp2(p1, p2, p8, p9, p10, p11));
}
```

**参数分组**：

- `p1`、`p2`：两个 `lerp2` 共用的**内部权重对**
- **`p3`：外层 `lerp` 的权重**
- `p4..p7`：第一个 `lerp2` 的四个值
- `p8..p11`：第二个 `lerp2` 的四个值
- **`p11` 在实现里完全未被使用**

**注意 `lerp2` 的参数顺序**：

```java
public static double lerp2(double a, double b, double c, double d, double e, double f) {
    return lerp(b, lerp(a, c, d), lerp(a, e, f));
}
```

`a` 是内层权重，**`b` 才是外层权重**。

### 实测

| 单元 | 结果 |
|---|---|
| `lerp3\|0\|0\|0\|0\|0\|0\|10\|10\|10\|10\|0` | `0` |
| `lerp3\|0\|1\|0\|0\|0\|0\|10\|10\|10\|10\|0` | `0` |
| `lerp3\|1\|0\|0\|0\|0\|0\|10\|10\|10\|10\|0` | `0` |
| `lerp3\|1\|1\|0\|0\|0\|0\|10\|10\|10\|10\|0` | `10` |
| `lerp3\|0.5\|0.5\|0\|0\|0\|0\|10\|10\|10\|10\|0` | `2.5` |

## 核心发现四：`lerp3` 的第 11 个参数是死参数

| 单元 | 结果 |
|---|---|
| `lerp3\|0.5\|0.5\|0\|0\|0\|0\|10\|10\|10\|10\|0` | **`2.5`** |
| `lerp3\|0.5\|0.5\|0\|0\|0\|0\|10\|10\|10\|10\|999` | **`2.5`** |

**两次结果完全相同**——`999` 与 `0` 无差别。

这是历史遗留：早期 `lerp3` 可能是真三维插值（需要三个权重），重构后签名未清理。**Cubium 若实现
`lerp3`，必须同样忽略第 11 个参数**；若「善用」它做额外计算，会在特定调用点产生差异。

## 本轮踩坑：三处参数映射错误，逐次被测试抓出

### 错误一：`lerp2` 写成了完全错误的公式

我凭印象写成：

```cpp
return (d1 - d0) * (d2 - d3) * (d4 - d5) + d1;   // 错误
```

正确是：

```java
return lerp(b, lerp(a, c, d), lerp(a, e, f));
```

**这不是「差一点」，而是完全不同的函数。** 读了源码才发现。

### 错误二：`lerp3` 把 `d2` 当作外层权重

我写成 `lerp(d2, ...)`，正确是 **`lerp(d3, ...)`**——外层权重在第 3 个位置，不是第 2 个。

### 错误三：单元格式用了逗号而非竖线

`lerp3` 的 11 个值我写成 `lerp3|0.5,0.5,0,...`（逗号分隔的单个字段），但**单元本身用 `|`
分段**，导致 `parts[1]` 只拿到 `0.5`，oracle 报「需要 11 个值」。

正确格式是 `lerp3|0.5|0.5|0|...`——**每个值独立成段**。

### 教训

**多参数插值函数的参数顺序是最容易抄错的地方**，因为参数名（`p1`..`p11`）毫无语义。必须：

1. **读完整函数体**，把 `p1..pN` 逐个映射到语义角色
2. **手工验算至少一个用例**（如全 0 输入、全 1 输入）
3. **确认单元格式的分隔符与框架的分段方式一致**

本能力的三处错误全部由差分测试捕获——`lerp3|0|1|...` 期望 `0` 而实际 `10`，直接暴露了外层
权重取错。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_lerp`](./mth_lerp.md) | 覆盖 `clampedLerp` / `lerp2` / `lerp` / `lerpDiscrete` / `lerpInt` |
| [`mth_int_div`](./mth_int_div.md) | `Mth` 的整数工具（`floorDiv` / `ceillog2` 等） |
| [`mth_rounding`](./mth_rounding.md) | `Mth` 的取整函数族 |

## 未覆盖范围

- `lerp3` 在密度函数与噪声烘焙里的实际调用点（属 world 型）
- `rotLerp` / `rotLerpRad`（角度插值，需要处理 360 度回绕）
- `catmullrom`（样条插值，5 个参数）
- `lerp(double, Vec3, Vec3)`（已由 [`vec3_math`](./vec3_math.md) 覆盖向量侧）