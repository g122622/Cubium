# 差分能力：`mth_rounding`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 91 个能力，21 个取整输入。19 个一致，2 个记为已知差异。`Mth` 的取整函数被全项目
广泛调用，但从未被系统性比对过。

#### 原版实现

```java
public static int floor(float f)  { int i = (int) f; return f < i ? i - 1 : i; }
public static int floor(double d) { int i = (int) d; return d < i ? i - 1 : i; }
public static int ceil(float f)   { int i = (int) f; return f > i ? i + 1 : i; }
public static int ceil(double d)  { int i = (int) d; return d > i ? i + 1 : i; }
```

**不是 `Math.floor`**，而是「先向零截断成 int，再比较」。在射程内两者结果相同，但：

#### 负数的取整方向

| 输入 | `floor` | `ceil` |
|---|---|---|
| `-0.5` | **-1** | **0** |
| `-1.5` | **-2** | **-1** |
| `-2.9` | **-3** | **-2** |

常见错误写法是 `static_cast<i32>(x)`——它对负数**向零截断**，等价于 `ceil` 而不是
`floor`。把 `floor` 写成它就是错的。

#### 超界输入：语言层面的差异，不是实现缺陷

| 输入 | 原版 | 探针 |
|---|---|---|
| `3000000000` | `2147483647` | `-2147483648` |
| `-3000000000` | `2147483647` | `-2147483648` |

- **Java** 的 `(int)` 转换对超界值是**饱和**的（一律 `Integer.MAX_VALUE`）
- **C++** 的 `static_cast<i32>(double)` 对超界值是**未定义行为**（本平台实测得到 `MIN_VALUE`）

这不是 Cubium 的实现缺陷——C++ 标准就没有定义这个行为。因此登记为**已知差异**而非待修
问题。（注意 `-3000000000` 在 Java 侧也得到 `MAX_VALUE`，这是 `(int)` 饱和的规则，与符号
无关。）

#### 这条能力为什么值得存在

`Mth.floor` / `ceil` 的正确性影响面极广（区块坐标转换、方块位置计算、渲染插值）。把
21 个代表性输入的取整结果固定下来，可以在有人把它「优化」成 `std::floor` 时立刻发现。
`std::floor(-0.5)` 得 `-1`（与 `Mth.floor` 相同），但 `std::floor` 的返回类型是浮点，
`static_cast<i32>(std::floor(2.9))` 与 `Mth.floor(2.9f)` 在边界值上仍可能因浮点精度差异
而不同。
