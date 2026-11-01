# 差分能力：`string_util`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 143 个能力，39 个用例全部一致。这是**首个字符串处理能力**——此前
[`text_encode`](./text_encode.md) 与 [`chat_formatting`](./chat_formatting.md) 覆盖的是
`Component` 与网络编码，不涉及 `StringUtil` 的纯字符串工具。

## 核心发现一：`truncateStringIfNecessary` 的 `maxLength > 3` 是独立条件

```java
if (s.length() <= maxLength) return s;
return addEllipsis && maxLength > 3
    ? s.substring(0, maxLength - 3) + "..."
    : s.substring(0, maxLength);
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `trunc\|abcdefgh\|5\|true` | `ab...` | 截 2 个 + `...` |
| `trunc\|abcdefgh\|4\|true` | `a...` | 截 1 个 + `...` |
| **`trunc\|abcdefgh\|3\|true`** | **`abc`** | **不加省略号** |
| `trunc\|abcdefgh\|5\|false` | `abcde` | 直接截断 |
| `trunc\|abc\|5\|true` | `abc` | 未超长，原样返回 |

**`maxLength = 3` 时 `addEllipsis` 完全失效**——因为 `3 - 3 = 0`，`"" + "..."` 会超长。

**`maxLength <= 0` 时 `substring(0, maxLength)` 会抛 `StringIndexOutOfBoundsException`**，
原版没有保护。

## 核心发现二：`lineCount` 的 `\v` 是「垂直空白字符类」，会匹配 `\n`

```java
private static final Pattern LINE_PATTERN = Pattern.compile("\r\n|\v");
```

**关键**：Java 正则在**默认（非 `UNIX_LINES`）模式**下，`\v` 表示**垂直空白字符类**，
等价于 `[\n\u000B\f\r\u0085\u2028\u2029]`——**包含 `\n`**。

| 单元 | 结果 | 说明 |
|---|---|---|
| **`lines\|`** | **`0`** | 空串特判 |
| `lines\|abc` | `1` | 无换行 |
| **`lines\|a\nb`** | **`2`** | **`\n` 算换行** |
| `lines\|a\r\nb` | `2` | `\r\n` 算**一次**（不是两次）|
| `lines\|a\v b` | `2` | 垂直制表 |

**我最初在文档草稿里写「`\n` 不算换行」是错的**——实测推翻了这个假设。

`\r\n` 得 `2` 而不是 `3`，因为正则在 `\r\n` 处匹配一次后整体跳过。

## 核心发现三：`stripColor` 的字母表是 `[0-9A-FK-OR]`，其中 `K-OR` 是范围

```java
private static final Pattern STRIP_COLOR_PATTERN = Pattern.compile("(?i)\u00A7[0-9A-FK-OR]");
```

**`K-OR` 不是「K、O、R 三个字母」，而是「`K` 到 `O` 的范围 + `R`」**，即：
**K、L、M、N、O、R**。

| 单元 | 结果 |
|---|---|
| `strip\|\u00A7ahello` | `hello` |
| **`strip\|\u00A7a\u00A7lhello`** | **`hello`**（`§l` 也剥离）|
| `strip\|\u00A7A\u00A7B\u00A7C` | ``（空串）|
| **`strip\|\u00A7zhello`** | **`§zhello`**（`z` 不在表里）|
| `strip\|hello` | `hello` |

**`§l`（粗体）也是合法颜色码**——`L` 在 `K-O` 范围内。我最初误把 `K-OR` 读成「K、O、R」三个
离散字母，漏了 `L`、`M`、`N`，**被测试抓出**。

`(?i)` 让 `a-f` 与 `A-F` 都能匹配。`\u00A7` 是 `§`。

## 核心发现四：`isAllowedChatCharacter` 的三个排除项

```java
return c != 167 && c >= 32 && c != 127;
```

`167` 是 `§`，`127` 是 `DEL`。

| 单元 | 结果 |
|---|---|
| `allowedChat\|65` | `True`（`A`）|
| `allowedChat\|31` | `False`（控制字符）|
| `allowedChat\|127` | `False`（DEL）|
| **`allowedChat\|167`** | **`False`**（`§`）|

**`§` 本身不是合法聊天字符**——这防止玩家注入颜色码。

**注意 `>= 32` 意味着所有非 ASCII 码点（`>= 128`）都合法**，只有 `167` 被单独排除。

### `filterText`

| 单元 | 结果 |
|---|---|
| `filter\|ab\u0001cd\|false` | `abcd`（`\u0001` 被过滤）|
| `filter\|ab\ncd\|false` | `abcd`（`\n` 被过滤）|
| **`filter\|ab\ncd\|true`** | **`ab\ncd`**（`keepNewlines` 保留 `\n`）|

**`keepNewlines = true` 只保留 `\n`，不保留 `\r` 或其他空白**。

## 其余实测

### `formatTickDuration`

| 单元 | 结果 |
|---|---|
| `duration\|0\|20` | `00:00` |
| `duration\|119\|20` | `00:05`（`119/20 = 5.95` → `floor = 5`）|
| `duration\|1200\|20` | `01:00` |
| `duration\|3600\|20` | `03:00` |
| **`duration\|72000\|20`** | **`01:00:00`**（≥1 小时用三段格式）|

`ticks / ticksPerSecond` 是 **`int / float` 的浮点除法**，再 `Mth.floor`。

### `isValidPlayerName`

| 单元 | 结果 |
|---|---|
| `playerName\|Steve` | `True` |
| `playerName\|Steve123` | `True` |
| `playerName\|abcdefghijklmnopq` | `False`（17 字符 > 16）|
| `playerName\|abc def` | `False`（空格 `= 32`）|
| `playerName\|abc\u00A7def` | `False`（`§`）|

### `isBlank` / `isWhitespace`

| 单元 | 结果 |
|---|---|
| `blank\|` | `True`（空串）|
| `blank\|   ` | `True` |
| `blank\|abc` | `False` |
| `whitespace\|32` | `True` |
| `whitespace\|9` | `True` |
| `whitespace\|65` | `False` |

## 本轮踩坑：Java 源码中任何 `\u` 都是 Unicode 转义

Java 编译报错：

```
error: illegal unicode escape
```

**Java 在词法分析之前就把源码中所有 `\uXXXX` 当成 Unicode 转义处理**，包括：

- **注释里的 `\uXXXX`**（即使 `XXXX` 不是合法十六进制也会报错）
- **字符串字面量里的 `\\u`**（`\\` 不阻止 `\u` 转义）

修正方式：**用常量拼接**绕开——

```java
private static final String UNICODE_ESC = "\\" + "u";   // 编译时拼出字面 "\u" 两字符
```

然后单元表写成 `"strip|" + UNICODE_ESC + "00A7ahello"`。

**探针侧对应地**：C++ 的 `"\\u00A7"` 是合法的字面两字符，直接用即可。

这是 `_patterns.md`「表示差异」的一个新实例：**两门语言的转义规则不同，必须各自用本地合法写法
构造同一字节序列**。

### 另一个教训：多次行编辑破坏文件结构

本能力开发过程中，我用行索引插入/删除的方式修改 `string_util.cpp`，先后破坏了：

1. `decodeEscapes` 的 `switch` 结构
2. `stripColor` 的 `i += 2; continue;`
3. `lineCount` 的闭合花括号

每次都要重新构建才能发现。**教训**：对结构紧凑的函数（嵌套 `switch`/`if`），
**优先整段替换或用 `write` 重写整个文件**，不要用行索引编辑——这与 `_patterns.md` 里已有的
「批量改索引文件先备份」条目同源。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`chat_formatting`](./chat_formatting.md) | `Component` 的格式化（本能力处理原始字符串）|
| [`text_encode`](./text_encode.md) | `Component` 的网络编码 |
| [`mth_rounding`](./mth_rounding.md) | `Mth.floor`（`formatTickDuration` 依赖）|

## 未覆盖范围

- `StringUtil.isNullOrEmpty`（依赖 Guava，行为是 `null` 或空串）
- `isBlank(null)` 的 `null` 分支（Java API 允许 `null`，探针协议无 `null` 概念）
- 代理对（辅助平面字符）在 `isValidPlayerName` 中的行为——已按「UTF-16 码元 > 127 即非法」实现，
  但未纳入用例
- `truncateStringIfNecessary` 的 `maxLength <= 0` 异常路径（原版会抛异常，探针不做错误输出）