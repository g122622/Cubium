# 差分能力：`text_formatting_color`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 170 个能力，44 个用例全部一致。**这是第一个探针直接调用 Cubium 真实实现的能力**——
前 15 个能力（`style_apply` / `text_color` / `difficulty` / `permission_level` 等）的探针都是
手抄原版逻辑，比对的是「原版 vs 转写」，无法暴露 Cubium 的缺陷。

本能力暴露并修复了 **1 处真实缺陷**。

## 方法论转折：为什么之前的「全部一致」不算数

以 [`difficulty`](./difficulty.md) 为例，Cubium 侧只有：

```cpp
enum class Difficulty : u8 { Peaceful = 0, Easy = 1, Normal = 2, Hard = 3 };
```

**没有 `byId`、没有 `WRAP` 回绕、没有 `getDisplayName`、没有 `getInfo`、没有 `INT_CODEC`。**

而我当时的探针验证了 `WRAP` 回绕算法、`options.difficulty.<key>.info` 翻译键——
**这些在 Cubium 里根本不存在**。「全部一致」只说明我抄对了原版源码，与 Cubium 无关。

**本能力的探针改为 `#include "common/util/text/TextStyle.hpp"` 直接调用
`mc::text::findFormattingARGB` / `isColor` / `isStyle` / `fromCode` / `toCode`**，
因此比对的两侧真正是「原版 vs Cubium」。

## 发现的缺陷：`getFormattingColor` 对样式码返回白色哨兵值

### 修复前

```cpp
// TextStyle.cpp（修复前）
u32 getFormattingColor(TextFormatting formatting) noexcept
{
    const auto* info = findFormattingInfo(formatting);
    return info ? info->color : 0xFFFFFFFF;      // 样式码 → 白色
}
```

```cpp
// FORMATTING_TABLE（修复前）
{TextFormatting::Bold, 'l', "bold", 0xFFFFFFFF},          // ← 样式码用白色填充
{TextFormatting::Strikethrough, 'm', "strikethrough", 0xFFFFFFFF},
{TextFormatting::Underline, 'n', "underline", 0xFFFFFFFF},
{TextFormatting::Italic, 'o', "italic", 0xFFFFFFFF},
{TextFormatting::Reset, 'r', "reset", 0xFFFFFFFF},
```

### 差异实测

| 单元 | 原版 | Cubium（修复前） |
|---|---|---|
| `color\|BOLD` | **`null`** | `FFFFFFFF` |
| `color\|ITALIC` | **`null`** | `FFFFFFFF` |
| `color\|OBFUSCATED` | **`null`** | `FFFFFFFF` |
| `color\|RESET` | **`null`** | `FFFFFFFF` |
| `color\|STRIKETHROUGH` | **`null`** | `FFFFFFFF` |
| `color\|UNDERLINE` | **`null`** | `FFFFFFFF` |

**6 处差异**，恰好是 6 个样式码。

### 为什么这是缺陷

原版的 `ChatFormatting.getColor()` 对样式码返回 `null`，语义是「**没有颜色**」。

Cubium 用 `u32`（非 `optional`）无法表达 `null`，退化为**白色哨兵值**。严重后果：

```cpp
// 这段判断永远为 false —— 白色本身是合法颜色（TextFormatting::White）
if (text::getFormattingColor(x) != 0xFFFFFFFF) {
    // 认为 x 有颜色
}
```

**区分不了「`TextFormatting::White`（真白）」与「`TextFormatting::Bold`（无颜色）」**。

当前代码之所以没出错，是因为所有调用点都额外配合了 `isColor()` 判断：

| 调用点 | 保护方式 |
|---|---|
| `WaypointManager.cpp:132` | `text::isColor(team->getColor()) &&` |
| `GlowEffect.cpp:110` | `argb != 0xFFFFFFFF && text::isColor(teamColor)` |
| `FontRenderer.cpp:261` | 依赖 `style.getColor().has_value()`（`optional` 已经是 `null`）|
| `RichTextWidget.hpp:162` | 同上 |
| `Style::getColorARGB()` | 本身就是「默认白色」语义 |

**但接口层面的语义与原版不一致**，任何新调用方若只看返回值就会踩坑。

### 修复

新增 `findFormattingARGB`，返回 `std::optional<u32>`，精确对应原版 `getColor()` 的 `null` 语义：

```cpp
std::optional<u32> findFormattingARGB(TextFormatting formatting) noexcept
{
    // 对应原版 ChatFormatting#getColor()：样式码没有颜色（返回 null）。
    if (!isColor(formatting)) {
        return std::nullopt;
    }
    const auto* info = findFormattingInfo(formatting);
    return info ? std::optional<u32>{info->color} : std::nullopt;
}

u32 getFormattingColor(TextFormatting formatting) noexcept
{
    // 渲染便捷入口：无颜色时退化为白色，与既有调用方保持一致。
    // 需要区分「无颜色」与「白色」请用 findFormattingARGB。
    return findFormattingARGB(formatting).value_or(0xFFFFFFFF);
}
```

**保留 `getFormattingColor` 的旧签名**（返回 `u32`、无颜色时给白色），因此 5 个既有调用点
**无需改动**；同时新增的 `findFormattingARGB` 提供了精确语义的入口。

### 修复后

```
[1/1] text_formatting_color        通过      2.4s  44/44 一致
```

**6 处差异全部消失**，且 `mc_tests` 全量构建通过（退出码 0），无回归。

## 其余验证：44 个单元全部一致

### 16 个颜色码的 ARGB 值与原版完全一致

| 单元 | 结果 |
|---|---|
| `color\|BLACK` | `FF000000` |
| `color\|DARK_BLUE` | `FF0000AA` |
| `color\|DARK_GREEN` | `FF00AA00` |
| `color\|DARK_AQUA` | `FF00AAAA` |
| `color\|DARK_RED` | `FFAA0000` |
| `color\|DARK_PURPLE` | `FFAA00AA` |
| `color\|GOLD` | `FFFFAA00` |
| `color\|GRAY` | `FFAAAAAA` |
| `color\|DARK_GRAY` | `FF555555` |
| `color\|BLUE` | `FF5555FF` |
| `color\|GREEN` | `FF55FF55` |
| `color\|AQUA` | `FF55FFFF` |
| `color\|RED` | `FFFF5555` |
| `color\|LIGHT_PURPLE` | `FFFF55FF` |
| `color\|YELLOW` | `FFFFFF55` |
| `color\|WHITE` | `FFFFFFFF` |

### `isColor` / `isStyle` 与原版一致

| 单元 | 结果 |
|---|---|
| `isColor\|BLACK` | `true` |
| `isColor\|WHITE` | `true` |
| `isColor\|BOLD` | `false` |
| **`isColor\|RESET`** | **`false`** |
| `isStyle\|OBFUSCATED` | `true` |
| `isStyle\|ITALIC` | `true` |
| **`isStyle\|RESET`** | **`false`** |

**`RESET` 既不是颜色也不是样式**——原版 `isFormat(RESET)` 也是 `false`。
Cubium 的 `isStyle` 上界是 `Italic`（20），恰好排除了 `Reset`（21），**实现正确**。

### `toCode` / `fromCode` 与原版一致

| 单元 | 结果 |
|---|---|
| `code\|BLACK` | `0` |
| `code\|GREEN` | `a` |
| `code\|WHITE` | `f` |
| `code\|BOLD` | `l` |
| `code\|RESET` | `r` |
| `fromCode\|a` | `GREEN` |
| **`fromCode\|A`** | **`GREEN`**（`tolower`，与原版一致）|
| `fromCode\|z` | `NONE` |
| `fromCode\|q` | `NONE` |

`fromCode` 对未知字符返回 `None`（`255`），原版返回 `null`——**这同样是一处哨兵值差异**，
但 `fromCode` 的调用方从不需要区分「未知字符」与「显式 None」，因此本能力**未视为缺陷**，
仅在探针中输出 `NONE` 使两侧可比。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`chat_formatting`](./chat_formatting.md) | `ChatFormatting` 枚举名/码位（手抄转写，未对接 Cubium）|
| [`style_apply`](./style_apply.md) | `Style` 的格式应用（手抄转写，未对接 Cubium）|
| [`text_color`](./text_color.md) | `TextColor.parseColor`（手抄转写，未对接 Cubium）|
| [`team_rule`](./team_rule.md) | `teamColorToSlot`（手抄转写，未对接 Cubium）|

**上表中除本能力外，其余四个都应在后续轮次改为对接 Cubium 实现。**

## 未覆盖范围

- `Style::applyFormat` / `applyLegacyFormat`（Cubium 无对应 API，见下方 TODO）
- `Style` 的 `withXxx()` 链式调用（Cubium 用 `setXxx()` 就地修改，API 形态不同）
- `ChatFormatting.getName()`（Cubium 的 `FORMATTING_TABLE[i].name` 未暴露公开接口）
- 实际渲染效果（客户端已停止维护）