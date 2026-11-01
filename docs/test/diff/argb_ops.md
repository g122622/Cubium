# 差分能力：`argb_ops`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 136 个能力，37 个用例全部一致。这是 [`mth_color`](./mth_color.md) 文档中明确列出的未覆盖项
「`ARGB.alpha`/`red`/`green`/`blue` 的反解作为独立能力」的补全，同时覆盖 `ARGB` 的其余通道运算。

## 核心发现一：`alpha` 用 `>>>`（无符号右移），其余通道用 `>>`

```java
public static int alpha(int c) { return c >>> 24; }        // 无符号
public static int red(int c)   { return c >> 16 & 0xFF; }  // 有符号 + 掩码
public static int green(int c) { return c >> 8 & 0xFF; }
public static int blue(int c)  { return c & 0xFF; }
```

| 单元 | 结果 |
|---|---|
| **`ch\|-1`** | **`255,255,255,255`** |
| `ch\|-65536` | `255,255,0,0` |
| `ch\|0x80FF8040` | `128,255,128,64` |
| `ch\|0x00000000` | `0,0,0,0` |
| `ch\|0x0000FF` | `0,0,0,255`（缺省 alpha 被当作 0）|

**`alpha` 必须用无符号右移**：颜色 `-1`（即 `0xFFFFFFFF`）若是 `c >> 24` 会得 `-1`，而正确结果
是 `255`。

其余通道因为后面有 `& 0xFF`，`>>` 与 `>>>` 结果相同——但**写 `alpha` 时不能用 `>>`**。

## 核心发现二：三参 `color` 的 alpha 是 255，与 `hsvToRgb` 相反

```java
public static int color(int r, int g, int b) { return color(255, r, g, b); }
```

| 单元 | 结果 | 十六进制 |
|---|---|---|
| `pack3\|255\|0\|0` | `-65536` | `0xFFFF0000` |
| `pack3\|0\|0\|255` | `-16776961` | `0xFF0000FF` |
| `pack\|0\|255\|0\|0` | `16711680` | `0x00FF0000`（显式 alpha 0）|

**这与 [`mth_hsv`](./mth_hsv.md) 的 `hsvToRgb`（alpha 恒为 0）完全相反**：

| 方法 | alpha |
|---|---|
| `ARGB.color(r, g, b)` | **255（不透明）** |
| `Mth.hsvToRgb(h, s, v)` | **0（全透明）** |

两套 API 对「省略 alpha」的默认值态度截然相反，混用会产生完全错误的渲染结果。

## 核心发现三：`scaleRGB` 的 `float` 版与 `int` 版算法不同，结果差 1

```java
// float 版：先乘后截断
scaleRGB(int c, float s)      -> (int)(channel * s)
scaleRGB(int c, float r,s,b)  -> (int)(channel * 系数)

// int 版：long 乘法后整除 255
scaleRGB(int c, int s)        -> (long)channel * s / 255L
```

| 单元 | 结果 | 十六进制 |
|---|---|---|
| **`scaleF\|0x00FF8040\|0.5`** | **`8339488`** | **`0x007F4020`** |
| **`scaleI\|0x00FF8040\|127`** | **`8339231`** | **`0x007F3F1F`** |
| `scaleF\|0x00FF8040\|2` | `16777088` | `0x00FFFF80`（clamp 饱和）|
| `scaleI\|0x00FF8040\|255` | `16744512` | `0x00FF8040`（原值）|
| `scaleI\|0x00FF8040\|0` | `0` | `0x00000000` |

**`0.5` 与 `127` 在语义上都表示「缩放一半」，但每个通道都差 1**：

- `float` 版：`0x80 * 0.5 = 64.0` → `64`（`0x40`），`0x40 * 0.5 = 32.0` → `32`（`0x20`）
- `int` 版：`128 * 127 / 255 = 63`（`0x3F`），`64 * 127 / 255 = 31`（`0x1F`）

**若 Cubium 只实现一种重载，另一种的调用点会产生偏差。**

`scaleF|0x00FF8040|2` 得 `0x00FFFF80`：`0x80 * 2 = 256` 被 clamp 到 `255`，而 `0x40 * 2 = 128`
正好翻倍——**放大时会饱和**。

## 核心发现四：`multiply` 对 `-1` 短路，但不影响数值

```java
if (a == -1) return b;
if (b == -1) return a;
```

| 单元 | 结果 |
|---|---|
| `mul\|0xFFFFFFFF\|0x8000FF00` | `0x8000FF00`（原值）|
| `mul\|0x8000FF00\|0xFFFFFFFF` | `0x8000FF00`（原值）|
| **`mul\|0x80808080\|0x80808080`** | **`0x40404040`** |

**`0x80 * 0x80 / 255 = 64 = 0x40`**——两个半透明灰相乘后每个通道都减半，**颜色变暗**。

`-1`（`0xFFFFFFFF`）是「纯白不透明」的**乘法单位元**，短路只为性能，数值上 `x * 255 / 255 == x`。

## 其余实测

### `greyscale`

| 单元 | 结果 | 推导 |
|---|---|---|
| `grey\|0x00FF0000` | `0x004C4C4C` | `255*0.3 = 76.5 → 76 = 0x4C` |
| `grey\|0x0000FF00` | `0x00969696` | `255*0.59 = 150.45 → 150 = 0x96` |
| `grey\|0x000000FF` | `0x001C1C1C` | `255*0.11 = 28.05 → 28 = 0x1C` |
| `grey\|0x00123456` | `0x002D2D2D` | `18*0.3 + 52*0.59 + 86*0.11 = 45.34 → 45` |

**权重是 `0.3 / 0.59 / 0.11`（不是标准的 `0.299 / 0.587 / 0.114`）**，且用 `(int)` 截断。

### `addRgb`

| 单元 | 结果 |
|---|---|
| `add\|0x00FF0000\|0x0000FF00` | `0x00FFFF00` |
| `add\|0x00FF0000\|0x00FF0000` | `0x00FF0000`（**饱和到 255**）|
| `add\|0x80000000\|0x00123456` | `0x80123456`（alpha 取第一个）|

**alpha 取自第一个参数**，两色的 alpha 不参与运算。

### `multiplyAlpha`

| 单元 | 结果 |
|---|---|
| `mulAlpha\|0xFF0000FF\|0.5` | `0x7F0000FF`（alpha `255*0.5 = 127 = 0x7F`）|
| `mulAlpha\|0xFF0000FF\|0` | `0`（**短路**）|
| `mulAlpha\|0xFF0000FF\|1` | `0xFF0000FF`（**短路**，原值）|

### `alphaBlend`

| 单元 | 结果 |
|---|---|
| `blend\|0x00333333\|0x80FF0000` | `0x80FF0000`（overlay 几乎完全覆盖）|
| `blend\|0x00333333\|0xFFFFFFFF` | `0xFFFFFFFF`（**短路**，alpha 255 直接返回 overlay）|
| `blend\|0x00333333\|0x00FFFFFF` | `0x00333333`（**短路**，alpha 0 直接返回 base）|

通道混合公式是独立的 `alphaBlendChannel`：`(overlay*oAlpha + base*(outAlpha-oAlpha)) / outAlpha`。

## 本轮踩坑：`multiplyAlpha` 的内部调用

我最初把 `multiplyAlpha` 的最后一步写成 `pack(newAlpha, color)`——以为存在一个「用新 alpha 替换」
的两参重载。编译失败：

```
error: no matching function for call to 'pack'
```

原版那里调用的是 **`ARGB.color(float alpha, int rgb)`**（float alpha 的重载），语义是「保留 RGB、
只换 alpha」。我改写成 `pack(newAlpha, red(c), green(c), blue(c))`，等价。

**教训**：`ARGB` 有多个 `color` 重载（`(a,r,g,b)` / `(r,g,b)` / `(float, int)` / `(Vec3)`），
写探针时要看清是哪一个。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`mth_color`](./mth_color.md) | HSV 转换与 murmur3；本文档补全了它列出的 `ARGB` 通道缺口 |
| [`mth_hsv`](./mth_hsv.md) | `hsvToRgb`（alpha 恒 0）——**与 `ARGB.color(r,g,b)` 的 alpha 255 相反** |
| [`dye_color`](./dye_color.md) | `DyeColor` 的颜色表（另一套体系）|

## 未覆盖范围

- `srgbToLinearChannel` / `linearToSrgbChannel`（sRGB 伽马曲线，涉及查表与浮点）
- `meanLinear` / `srgbLerp`（线性空间混合）
- `subtractRgb`
- `color(Vec3)`（依赖 `as8BitChannel` = `Mth.floor(x*255)`）
- `toVector4` / `redFloat` / `greenFloat` / `blueFloat` / `alphaFloat`（浮点通道）