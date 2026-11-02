# 差分能力：`easing_type`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 187 个能力。288 个单元（32 种缓动 × 9 个采样点），**275 一致、13 处差异**。
修复了 **3 处真实缺陷**，剩余 13 处已定位为编译选项导致的浮点语义差异（非代码错误）。

## 测的是什么

原版 32 个具名缓动全部委托给 `net.minecraft.util.Ease`：

```java
EasingType CONSTANT = registerSimple("constant", t -> 0.0F);
EasingType LINEAR   = registerSimple("linear",   t -> t);
EasingType IN_BACK  = registerSimple("in_back",  Ease::inBack);
EasingType OUT_ELASTIC = registerSimple("out_elastic", Ease::outElastic);
// …共 32 个
```

本能力逐个调用 `EasingType.apply(float)`，与 Cubium 的 `EasingType.apply(f32)` 对比。

## 三个关键点

### 一、全程 float 运算，但 `Math.pow` / `Math.sin` / `Math.sqrt` 是 double

原版写法是运算途中把参数提升成 double，算完再截回 float：

```java
(float)(-Math.pow(2.0, 10.0 * t - 10.0) * Math.sin((t * 10.0 - 10.75) * (float)(Math.PI * 2.0 / 3.0)))
```

若在 C++ 里用 `std::pow(float, float)`，会走 float 重载，末位可能不同。

### 二、`inElastic` 的两端有硬编码短路

```java
if (t == 0.0F) return 0.0F;
else if (t == 1.0F) return 1.0F;
else { ... }
```

公式本身在 `t = 1` 时算出来是**负数**，必须靠短路纠正。少了短路的实现会在端点给出完全错误的结果。

### 三、`Mth.sin` 与 `Math.sin` 不能混用

Cubium 的 `MathUtils.hpp` 对此有明确约定：

> 两者差异约为 f32 精度量级（约 1e-7），单独看可忽略；但世界生成中它参与半径、位置等几何量的计算
> 并被反复放大，与 `std::sin` 混用会让生成结果逐位分叉。
> **凡是原版写 `Mth.sin` 的地方都必须用本函数，原版写 `Math.sin` 的地方才用 `std::sin`。**

## 本轮修复的 3 处缺陷

`Ease` 的 elastic 系列用的是 **`Math.sin`**（double 精度），Cubium 误用了 **`math::mthSin`**（查表近似）。

| 函数 | 修复前 | 修复后 |
|---|---|---|
| `_inElastic` | `math::mthSin((f * 10.0f - 10.75f) * C)` | `std::sin((f * 10.0 - 10.75) * C)` |
| `_outElastic` | `math::mthSin((f * 10.0f - 0.75f) * C)` | `std::sin((f * 10.0 - 0.75) * C)` |
| `_inOutElastic` | `const f32 d = math::mthSin(...)` | `const f64 d = std::sin(...)` |

**影响**：查表近似的误差约 1e-7，在弹性缓动的指数放大下体现为**第 4~5 位有效数字**的偏差，
比其它缓动高两个数量级：

| 单元 | 修复前 | 修复后（= 原版）|
|---|---|---|
| `in_elastic\|0.5` | ✗ | ✓ |
| `in_elastic\|0.9` | `-0.249986112` | `-0.250000209` |
| `in_out_elastic\|0.6667` | `1.0028412` | `1.0028369` |

修复后 elastic 系列的 16 处差异**全部消除**（29 → 13）。

## 剩余 13 处：`-ffast-math` 导致的 FMA 收缩

| 单元 | 原版 | Cubium |
|---|---|---|
| `out_quad\|0.1` | `0.190000057` | `0.190000042` |
| `out_cubic\|0.1` | `0.271000087` | `0.271000057` |
| `in_back\|0.6667` | `0.0442548059` | `0.0442548171` |

### 定位过程

**第一步：怀疑实现。** 读 `_outQuad` 的源码：

```cpp
static f32 _outQuad(f32 f) { return 1.0f - math::square(1.0f - f); }
```

与原版 `1.0F - Mth.square(1.0F - t)` 完全一致。

**第二步：手算对照。** 在探针里对同一输入同时计算「手算」与「`apply()`」：

```
DIAG in=0.100000001 inner=0.899999976 sq=0.809999943 manual=0.190000057 applied=0.190000042
```

**同一份代码、同一个函数，手算得到原版值，`apply()` 得到另一个值。**

**第三步：隔离编译选项。** 用最小程序试不同 flag 组合：

| 编译选项 | `1.0f - inner*inner` |
|---|---|
| `-O2` | `0.190000057` |
| `-O2 -ffast-math` | `0.190000057` |
| `-O2 -march=native -mavx2` | `0.190000057` |
| **`-O2 -march=native -mavx2 -ffast-math`** | **`0.190000042`** |

项目在 RelWithDebInfo 下正好启用了最后一组（`CMakeLists.txt` 的 `-march=native` +
`-ffast-math`）。`-ffast-math` 允许编译器把 `1.0f - x * x` 收缩成单条 **FMA** 指令，
舍入点因此改变。

**结论：这不是代码错误，是编译期浮点语义差异。**

```cpp
// 见 tests/diff/probes/easing_type.cpp 头注释
// TODO: 若要彻底消除这类差异，需要为浮点敏感的源文件单独关闭 -ffast-math 与 FMA 收缩
// （-ffp-contract=off）。这是全局编译选项变更，影响面需单独评估。
```

## 踩坑：Java 的 `%g` 不剥尾随零

第一轮跑出 128 处差异，其中绝大多数是**纯格式差异**：

```
原版   -0.0876975060    ← 尾零
Cubium -0.087697506     ← 已剥离
```

C++ 的 `%g` 自动剥尾随零，Java 的 `String.format("%g")` **不剥**。两侧必须显式统一规则，
否则假差异会掩盖真差异（128 里只有 29 是真实的）。

## 单元格式

`<缓动名>|<输入>`

```
in_back|0.5
out_elastic|0.75
```

采样点：`0`、`0.1`、`0.25`、`0.3333`、`0.5`、`0.6667`、`0.75`、`0.9`、`1`——
覆盖端点短路、分段边界与中点。

输出用 `%.9g`（float 的往返位数）：6 位会抹平末位差异，17 位会把 float 的二进制噪声也印出来。

## 未覆盖范围

- **`EasingType.CubicBezier`**：控制点可任意指定，需要额外的参数空间，未纳入。
  见 `TODO: 贝塞尔缓动的 sample() 求解需要与具体控制点组合，待单独设计单元格式。`
- **`EasingType.SIMPLE_REGISTRY`** 的序列化名与 `CODEC`：属于编解码层，不在本能力范围。
- **超出 `[0, 1]` 的输入**：缓动函数在定义域外行为未文档化，未纳入。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_sin`](./mth_sin.md) | `Mth.sin` 的查表实现 |
| [`mth_lerp`](./mth_lerp.md) | `Mth.lerp` 插值 |
| [`mth_approach`](./mth_approach.md) | 趋近运算 |