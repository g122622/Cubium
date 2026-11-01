# 差分能力：`chat_formatting`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 121 个能力，39 个用例全部一致。`ChatFormatting` 此前完全未被覆盖。

## 核心发现：`stripFormatting` 的字符集是有限的

```java
private static final Pattern STRIP_FORMATTING_PATTERN =
    Pattern.compile("(?i)\u00a7[0-9A-FK-OR]");
```

这个正则**只认**：

| 范围 | 含义 |
|---|---|
| `0-9` | 16 个颜色码 |
| `A-F` | 16 个颜色码（小写形式的等价写法）|
| `K-O` | 扁平化/粗体/删除线/下划线/斜体 |
| `R` | 重置 |

**不认 `G-J`、`P-Q`、`S-Z`。** 所以 `"§g"`、`"§z"`、`"§ hello"`、孤立的 `"§"` 都**不会被剥离**，
原样保留。

| 单元 | 结果 |
|---|---|
| `strip\|§ahello` | `hello` |
| `strip\|§0hello` | `hello` |
| `strip\|§lhello` | `hello` |
| `strip\|§rhello` | `hello` |
| **`strip\|§ghello`** | **`§ghello`**（不剥离） |
| **`strip\|§zhello`** | **`§zhello`**（不剥离） |
| **`strip\|§ hello`** | **`§ hello`**（不剥离） |
| **`strip\|§`** | **`§`**（不剥离） |
| `strip\|§a§lBold§rPlain` | `BoldPlain` |

这与「剥掉所有 `§` 后面的字符」的直觉不同，是数据校验的重要依据。

## 其余三个关键点

**1. 正则带 `(?i)`，大小写不敏感。**

`strip|§Ahello → hello`、`strip|§Lhello → hello`，与对应小写形式结果相同。

**2. `getByCode` 先 `toLowerCase` 再匹配。**

| 单元 | 结果 |
|---|---|
| `byCode\|a` | `GREEN` |
| `byCode\|A` | `GREEN` |
| `byCode\|0` | `BLACK` |
| `byCode\|f` | `WHITE` |
| `byCode\|r` | `RESET` |
| **`byCode\|g`** | **`null`**（无对应枚举） |

找不到时返回 `null`，**不是 `RESET`**。

**3. `getById` 对负数一律返回 `RESET`，不是 `null`。**

```java
public static ChatFormatting getById(int id) {
    if (id < 0) { return RESET; }
    for (ChatFormatting f : values()) { if (f.getId() == id) { return f; } }
    return null;
}
```

| 单元 | 结果 |
|---|---|
| `byId\|0` | `BLACK` |
| `byId\|15` | `WHITE` |
| **`byId\|-1`** | `RESET` |
| **`byId\|-2`** | **`RESET`**（不是 `null`！） |
| **`byId\|16`** | **`null`** |
| **`byId\|100`** | **`null`** |

`RESET` 的 `id` 本身是 `-1`，所以「负数 → RESET」与「id == -1 → RESET」在这里重合。但
**`getById(-2)` 也返回 `RESET`**，这是容易误以为返回 `null` 的地方。

正数找不到时才返回 `null`。

**4. `isColor()` 排除 `RESET`。**

```java
public boolean isColor() { return !this.isFormat && this != RESET; }
```

| 单元 | 结果 | 说明 |
|---|---|---|
| `info\|BLACK` | `0,0,true,false,§0` | 颜色 |
| `info\|WHITE` | `f,15,true,false,§f` | 颜色 |
| `info\|BOLD` | `l,-1,false,true,§l` | 格式 |
| **`info\|RESET`** | **`r,-1,false,false,§r`** | **既非颜色也非格式** |

字段顺序为 `code,id,isColor,isFormat,toString`。

## `getByName` 的 `cleanName`

```java
private static String cleanName(String name) {
    return name.toLowerCase(Locale.ROOT).replaceAll("[^a-z]", "");
}
```

| 单元 | 结果 |
|---|---|
| `byName\|black` | `BLACK` |
| `byName\|BLACK` | `BLACK` |
| `byName\|dark_blue` | `DARK_BLUE` |
| **`byName\|Dark-Blue`** | **`DARK_BLUE`**（去掉连字符） |
| `byName\|not_a_color` | `null` |

**所有非 `a-z` 字符都被剔除**，所以下划线、连字符、空格都会被忽略。

## 本轮踩坑：C++ 十六进制转义会贪婪吞掉后续字符

最初把用例写成：

```cpp
"strip|\xc2\xa7ahello",   // 错误！
```

`\xa7a` 被 C++ 当成**一个**十六进制转义序列（读到非十六进制字符为止），值超出 `char` 范围，
编译报 `hex escape sequence out of range`。

正确写法是用相邻字符串字面量拼接，把转义与后续字母隔开：

```cpp
"strip|\xc2\xa7" "ahello",
```

这个坑对任何「`§` 后面紧跟 `0-9`、`a-f`」的用例都成立——正好是本能力的主要用例形态。

## 关于 `§` 的字节表示

`§`（U+00A7）在 UTF-8 下是两字节 `0xC2 0xA7`。探针按**字节**扫描 `stripFormatting`：

- 遇到 `0xC2 0xA7` 且后面是合法字符 → 跳过 3 字节
- 否则保留 `0xC2`，继续扫描

这保证了对非法序列（如 `§g`）的 `§` 本身被原样输出。

## 未覆盖范围

- `getNames(boolean, boolean)`（按 isColor/isFormat 过滤枚举名）
- `getName()` 的 Locale 行为
- `CODEC` / `COLOR_CODEC` / `CHARACTER_AND_FORMATTING_CODEC` 的序列化形式
- `getColor()` 的 RGB 值与 `TextColor` 的互转