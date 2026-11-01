# 差分能力：`style_tristate`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 173 个能力，26 个用例全部一致。**探针直接调用 Cubium 的 `mc::text::Style`**（setter +
`toJson()`），是继 [`text_formatting_color`](./text_formatting_color.md) 之后第三个「对接真实实现」
的能力。

本能力暴露并修复了 **1 处真实缺陷**。

## 方法论：为什么比序列化而不是比 getter

原版 `Style` 的三态字段是**包私有**的，没有公开的三态访问器：

```java
final Boolean bold;                  // 包私有，外部读不到
public boolean isBold() {            // bold == Boolean.TRUE
    return this.bold == Boolean.TRUE;
}
```

**`isBold()` 无法区分「未设置」与「显式 false」**——两者都返回 `false`。

Cubium 则把三态暴露在 getter 上（`bold()` 返回 `std::optional<bool>`）。这是**接口设计差异，
不是缺陷**——Cubium 提供了更丰富的信息。

真正必须一致的是**序列化结果**：原版存档里 `"bold": false` 与「没有 `bold` 键」是两种不同的值，
影响父子样式继承判定与网络字节。因此本能力比对 `toJson()` 的输出。

## 发现的缺陷：颜色序列化成名字，而非 `#RRGGBB`

### 修复前

```cpp
// TextStyle.cpp（修复前）
nlohmann::json Style::toJson() const
{
    if (m_color.has_value()) {
        json["color"] = toName(*m_color);        // ← 写成 "red"
    }
    ...
}

Style Style::fromJson(const nlohmann::json& json)
{
    if (json.contains("color") && json["color"].is_string()) {
        style.m_color = fromName(json["color"].get<std::string>());   // ← 读 "red"
    }
    ...
}
```

### 差异实测

| 单元 | 原版 | Cubium（修复前） |
|---|---|---|
| `colorSet\|RED` | **`{"color":"#FF5555"}`** | `{"color":"red"}` |
| `colorSet\|WHITE` | **`{"color":"#FFFFFF"}`** | `{"color":"white"}` |

### 为什么这是缺陷

原版的 `TextColor.Serializer` 把颜色写成 **`#RRGGBB` 十六进制**，Cubium 写**颜色名**。

Cubium 的 `toJson` / `fromJson` **自洽**（自己写自己读没问题），但与**原版存档格式不兼容**：

- 原版存档里读到 `"color":"#FF5555"`，Cubium 的 `fromName("#FF5555")` **匹配不到任何颜色名**，
  返回 `None`——颜色丢失
- Cubium 写出的 `"color":"red"`，原版 `TextColor.Serializer` **解析失败**
- 与 Java 版客户端共享的任何文本 JSON（`/tellraw`、告示牌、书本、NBT）都会错

这是本项目「仅支持 Java 版客户端连接」前提下的**实质互通缺陷**。

### 修复

```cpp
// toJson：按原版写 #RRGGBB
if (m_color.has_value()) {
    const std::optional<u32> argb = findFormattingARGB(*m_color);
    if (argb.has_value()) {
        char buffer[8];
        std::snprintf(buffer, sizeof(buffer), "#%06X",
                      static_cast<unsigned>(*argb & 0xFFFFFFu));
        json["color"] = buffer;
    }
}

// fromJson：接受 #RRGGBB，也保留颜色名以兼容旧数据与手写 JSON
if (!text.empty() && text.front() == '#' && text.size() == 7) {
    const unsigned rgb = static_cast<unsigned>(std::strtoul(text.c_str() + 1, nullptr, 16));
    style.m_color = std::nullopt;
    for (const auto& entry : FORMATTING_TABLE) {
        if ((entry.color & 0xFFFFFFu) == rgb) {
            style.m_color = entry.formatting;
            break;
        }
    }
    // TODO: Cubium 的 Style 用 TextFormatting 枚举表达能力有限，无法存储任意 RGB
    //（如 "#123456"）。待 Style 改为持有 RGB 后再支持。
} else {
    style.m_color = fromName(text);
}
```

### 修复后

```
[1/1] style_tristate               通过     10.1s  26/26 一致
```

**4 处差异全部消失**，`mc_tests` 全量构建通过（退出码 0），无回归。

## 三态序列化的核心验证

| 单元 | 结果 | 说明 |
|---|---|---|
| `empty` | `{}` | 默认构造不输出任何键 |
| `set\|bold\|true` | `{"bold":true}` | 显式 true 写出 |
| **`set\|bold\|false`** | **`{"bold":false}`** | **显式 false 必须写出** |
| `set\|bold\|unset` | `{}` | 清除后不输出键 |
| `setThenClear\|bold\|true` | `{}` | 设值再清除回到空 |
| `setThenClear\|italic\|false` | `{}` | 同上 |
| `setAll\|t` | 五个键全 `true` | |
| `setAll\|f` | 五个键全 `false` | |

**`set|bold|false` 是本能力最关键的单标志用例**：若 Cubium 用 `if (*value)` 判断再写，
显式 `false` 会被**静默丢弃**，父子样式继承判定就会出错。

Cubium 的实现是正确的：

```cpp
// 三态：显式 false 也要写出来（`"italic": false`）。原版存档里它与「没写」是两种值。
if (m_bold.has_value()) {
    json["bold"] = *m_bold;
}
```

## `isXxx()` 的行为与原版一致

| 单元 | 结果 |
|---|---|
| `asBool\|bold\|unset` | `false` |
| `asBool\|bold\|false` | `false` |
| `asBool\|bold\|true` | `true` |
| `asBool\|italic\|unset` | `false` |
| `asBool\|italic\|false` | `false` |

Cubium 的 `isBold()` 实现是 `m_bold.value_or(false)`，与原版 `bold == Boolean.TRUE` 等价。

## 踩坑：JSON 键序导致假差异

修复颜色后仍有 2 处差异，全部是 `setAll` 的键序不同：

```
oracle: {"bold":false,"italic":false,"underlined":false,"strikethrough":false,"obfuscated":false}
probe : {"bold":false,"italic":false,"obfuscated":false,"strikethrough":false,"underlined":false}
```

**JSON 对象本无序**，语义完全相同。但 `nlohmann::json::object()` 底层是 `std::map`，
**会按键名字典序重排**——即使按自定义顺序赋值，`dump()` 时仍按字母序输出。

修复：手工拼接字符串保留顺序。

```cpp
std::string out = "{";
bool first = true;
auto append = [&](const std::string& key, const json& item) {
    if (!first) out.push_back(',');
    first = false;
    out += json(key).dump();
    out.push_back(':');
    out += item.dump();
};
```

**教训**：用 `nlohmann::json` 做「字段顺序敏感」的比对时，必须自己控制序列化顺序，
不能依赖 `dump()`。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`text_formatting_color`](./text_formatting_color.md) | 同族：颜色 ARGB 查询（修复了 `findFormattingARGB`）|
| [`text_formatting_name`](./text_formatting_name.md) | 同族：名字双向映射 |
| [`style_apply`](./style_apply.md) | 手抄转写的旧版（`applyFormat` 在 Cubium 无对应 API）|
| [`text_color`](./text_color.md) | `TextColor.parseColor`（手抄转写，应改造）|
| [`chat_formatting`](./chat_formatting.md) | 手抄转写（`getName`/`getChar` 已被前两个能力覆盖）|

**`style_apply` 与 `text_color` 已可判定为「对 Cubium 无验证价值」**：
Cubium 的 `Style` 是可变 `setXxx()` 风格，没有原版的 `applyFormat` / `withXxx()` 链式 API；
`TextColor` 也不是独立类型而是 `TextFormatting` 枚举。这两个能力应在后续轮次标注为「原版快照」。

## 未覆盖范围

- `Style` 的 `clickEvent` / `hoverEvent` / `insertion` 序列化
- 任意 RGB 颜色（`"#123456"`）——**Cubium 当前无法表达**，已在 `fromJson` 中留 TODO
- `Style` 的父子继承合并（`result.m_bold = pick(m_bold, parent.m_bold)`）
- `Style` 的网络字节编码（`TRUSTED_STREAM_CODEC`）