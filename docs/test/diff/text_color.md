# 差分能力：`text_color`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 157 个能力，27 个用例全部一致。这是 [`style_apply`](./style_apply.md) 的延续——
后者覆盖 `Style` 的格式应用，本能力覆盖 `Style` 里的**颜色值** `TextColor`。

## 核心发现一：`#f00` 不是 RGB 缩写，而是十六进制 `0x000F00`

```java
if (text.startsWith("#")) {
    try {
        int i = Integer.parseInt(text.substring(1), 16);
        return i >= 0 && i <= 16777215 ? ... : error;
    } catch (NumberFormatException exception) { ... }
}
```

**只有 `Integer.parseInt(..., 16)`，没有位数检查。**

| 单元 | 结果 | 说明 |
|---|---|---|
| **`parse\|#f00`** | **`000F00\|#000F00`** | **暗绿，不是红** |
| `parse\|#0` | `000000` | 单字符合法 |
| `parse\|#ff0000` | `FF0000\|#FF0000` | 标准 6 位 |
| `parse\|#FF0000` | `FF0000\|#FF0000` | 大小写不敏感 |

**`#f00` 是最容易踩的坑**：CSS 语义下它是红色简写（`0xFF0000`），而这里它是
**`0x000F00`（暗绿）**。

若 Cubium 实现成「3 位则每位数重复一次」的 CSS 语义，`#f00` 会得到 `FF0000`——**完全错误**。

## 核心发现二：范围检查在 `parseInt` 之后，`#-1` 与 `#1000000` 都失败

```java
int i = Integer.parseInt(text.substring(1), 16);
return i >= 0 && i <= 16777215 ? success : error;
```

| 单元 | 结果 | 原因 |
|---|---|---|
| **`parse\|#1000000`** | **`<error>`** | `16777216 > 0xFFFFFF` |
| `parse\|#ffffff` | `FFFFFF` | 正好上界 |
| **`parse\|#-1`** | **`<error>`** | `parseInt` 得 `-1`，被 `>= 0` 拒绝 |
| `parse\|#` | `<error>` | 空串，`parseInt` 抛异常 |
| `parse\|#gggggg` | `<error>` | 非十六进制字符 |
| **`parse\|#+ff`** | **`0000FF`** | **`parseInt` 接受前导 `+`** |

**`#+ff` 被接受**是 Java `Integer.parseInt` 的细节——C++ 的 `std::stoi` 也接受前导 `+`，
但手写解析器容易漏掉。

## 核心发现三：`NAMED_COLORS` 只有 16 个传统颜色，且不含格式名

| 名字 | 值 |
|---|---|
| `black` | `000000` |
| `dark_blue` | `0000AA` |
| `dark_green` | `00AA00` |
| `dark_aqua` | `00AAAA` |
| `dark_red` | `AA0000` |
| `dark_purple` | `AA00AA` |
| `gold` | `FFAA00` |
| `gray` | `AAAAAA` |
| `dark_gray` | `555555` |
| `blue` | `5555FF` |
| `green` | `55FF55` |
| `aqua` | `55FFFF` |
| `red` | `FF5555` |
| `light_purple` | `FF55FF` |
| `yellow` | `FFFF55` |
| `white` | `FFFFFF` |

| 单元 | 结果 |
|---|---|
| `parse\|red` | `FF5555\|red` |
| `parse\|light_purple` | `FF55FF\|light_purple` |
| **`parse\|RED`** | **`<error>`**（**大小写敏感**）|
| **`parse\|bold`** | **`<error>`**（格式名无颜色值）|
| **`parse\|reset`** | **`<error>`** |
| `parse\|` | `<error>`（空串不是名字）|

**注意 `red` 是 `FF5555`（亮红），不是 `FF0000`**——这是传统 16 色的浅色调色板。

## 核心发现四：`serialize()` 对具名色返回名字，对 `fromRgb` 返回大写十六进制

```java
public String serialize() { return this.name != null ? this.name : this.formatValue(); }
private String formatValue() { return String.format(Locale.ROOT, "#%06X", this.value); }
```

| 单元 | 结果 |
|---|---|
| **`parse\|red`** | `FF5555\|`**`red`** |
| **`parse\|#ff0000`** | `FF0000\|`**`#FF0000`** |
| **`fromRgb\|16711680`** | `FF0000\|`**`#FF0000`** |

**`parseColor(x).serialize()` 不等于 `x`**：

- `"#ff0000"` → `"#FF0000"`（**变成大写**）
- `"red"` → `"red"`（保持名字）

## 核心发现五：`equals` 只比 `value`，忽略名字

```java
public boolean equals(Object other) {
    ...
    return this.value == textcolor.value;
}
```

| 单元 | 结果 |
|---|---|
| **`match\|red\|#ff5555`** | **`True`**（`FF5555 == FF5555`）|
| **`match\|red\|#ff0000`** | **`False`**（`FF5555 != FF0000`）|
| `match\|red\|red` | `True` |

**具名色与同值十六进制色相等**——`name` 不参与比较。

## 核心发现六：`fromRgb` 用 `& 0xFFFFFF` 截断，不做范围检查

```java
private TextColor(int value) { this.value = value & 16777215; }
```

| 单元 | 结果 |
|---|---|
| **`fromRgb\|-1`** | **`FFFFFF`**（`-1 & 0xFFFFFF = 0xFFFFFF`）|
| **`fromRgb\|16777216`** | **`000000`**（`0x1000000 & 0xFFFFFF = 0`）|
| **`fromRgb\|4294967295`** | `FFFFFF`（`0xFFFFFFFF & 0xFFFFFF = 0xFFFFFF`）|

**`fromRgb` 与 `parseColor` 的行为不对称**：前者静默截断（丢弃高位），后者报错。

若 Cubium 在 `fromRgb` 里加了范围检查并抛异常，就会与原版不同。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`style_apply`](./style_apply.md) | `Style` 的格式应用（颜色是其字段之一）|
| [`chat_formatting`](./chat_formatting.md) | `ChatFormatting` 枚举（`fromLegacyFormat` 的数据源）|
| [`mth_color`](./mth_color.md) | RGB 分量的位运算 |
| [`argb_ops`](./argb_ops.md) | ARGB 打包与拆解 |

## 未覆盖范围

- `TextColor.CODEC`（序列化 codec，依赖 `DynamicOps`）
- `TextColor.fromLegacyFormat`（已被 [`style_apply`](./style_apply.md) 间接覆盖）
- `TextColor.HASH_STRATEGY` / `collect`（批量哈希）
- `TextColor.parseColor` 返回的 `DataResult` 的**错误消息原文**（本能力只区分成功/失败）