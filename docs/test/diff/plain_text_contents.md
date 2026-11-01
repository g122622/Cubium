# 差分能力：`plain_text_contents`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 161 个能力，18 个用例全部一致。这是 [`style_apply`](./style_apply.md) /
[`text_color`](./text_color.md) / [`click_event`](./click_event.md) / [`hover_event`](./hover_event.md)
一族的收尾——补上 `Component` 的**文本内容**这一层。

## 核心发现一：空串折叠为 `EMPTY` 单例，`toString()` 是裸词 `empty`

```java
PlainTextContents EMPTY = new PlainTextContents() {
    @Override public String toString() { return "empty"; }
    @Override public String text() { return ""; }
};

static PlainTextContents create(String text) {
    return text.isEmpty() ? EMPTY : new PlainTextContents.LiteralContents(text);
}

public record LiteralContents(String text) implements PlainTextContents {
    @Override public String toString() { return "literal{" + this.text + "}"; }
}
```

| 单元 | 结果 |
|---|---|
| **`create\|`**（空串） | **`empty`** |
| **`create\|abc`** | **`literal{abc}`** |

**格式不对称**：`EMPTY` 是裸词 `empty`，**没有 `literal{}` 包裹**。

若 Cubium 统一用 `literal{}` 包裹，空串会输出 `literal{}`——与原版不同。

## 核心发现二：空串是单例，`create("") == create("")` 为 `true`

```java
return text.isEmpty() ? EMPTY : new PlainTextContents.LiteralContents(text);
```

| 单元 | 结果 | 原因 |
|---|---|---|
| **`identity\|`** | **`true`** | 返回同一 `EMPTY` 实例 |
| **`identity\|abc`** | **`false`** | 每次 `new LiteralContents(...)` |

**这对依赖 `==` 的代码路径有实际影响**：`Component` 的 intern 化、缓存键的 identity 比较。

若 Cubium 每次 `new` 一个空内容对象，identity 比较会失败。

## 核心发现三：`equals` 的两条路径不同

- `EMPTY` 是**匿名类**，走 `Object.equals`（**只有 identity 相等**）
- `LiteralContents` 是 **record**，按 `text` 值比较

| 单元 | 结果 | 原因 |
|---|---|---|
| **`eq\|\|`** | **`true`** | 两个都是 `EMPTY` 单例 → 同一实例 |
| **`eq\|abc\|abc`** | **`true`** | record 值相等 |
| `eq\|abc\|abd` | `false` | 值不等 |
| **`eq\|\|abc`** | **`false`** | 不同类型 |

## 核心发现四：折叠不影响文本内容

`EMPTY.text()` 返回 `""`，与输入一致。

| 单元 | 结果 |
|---|---|
| `text\|` | `` （空串）|
| `text\|abc` | `abc` |
| `string\|` | `` （`Component.literal("").getString()`）|
| `string\|abc` | `abc` |

所以 `create(t).text().equals(t)` **恒真**。折叠只影响 `toString` 与 identity。

## 核心发现五：只有**真正的空串**才折叠，空白串不折叠

| 单元 | 结果 |
|---|---|
| **`create\| `**（单个空格） | **`literal{ }`** |
| `create\|a b` | `literal{a b}` |

`String.isEmpty()` 只判断长度是否为 0，**不做 trim**。

若 Cubium 用 `text.trim().isEmpty()` 判断，`" "` 会被错误折叠成 `empty`。

## 核心发现六：`toString` 不转义花括号

| 单元 | 结果 |
|---|---|
| **`create\|{}`** | **`literal{{}}`** |
| **`create\|literal{x}`** | **`literal{literal{x}}`** |

直接拼接，**没有转义、没有转义层**。这是调试用的 `toString`，不用于序列化。

## 本轮踩坑：oracle 输出布尔，探针输出字符串

首次运行：

```
oracle: {"value":true}
probe : {"value":"true"}
```

`JsonObject.addProperty("value", boolean)` 生成 JSON 布尔，而探针用 `json{{"value", "true"}}` 生成字符串。

修复：oracle 侧用 `String.valueOf(...)` 统一为字符串。

**统一约定**：本能力的 `identity` / `eq` 输出**字符串形式的布尔**，与探针的 `std::string` 类型对齐。
（对比 [`click_event`](./click_event.md) 的 `allowed` 输出原生布尔——那里探针用 `bool`，两侧一致。）

**这是一个可复用的判断依据**：JSON 比对是**严格类型**的，`true` 与 `"true"` 不等。
新建能力时先确定每个字段的 JSON 类型，不要在探针与 oracle 之间混用。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`style_apply`](./style_apply.md) | `Style` 的格式应用 |
| [`text_color`](./text_color.md) | `Style` 的颜色值 |
| [`click_event`](./click_event.md) | `Style` 的点击事件 |
| [`hover_event`](./hover_event.md) | `Style` 的悬停事件 |
| [`text_encode`](./text_encode.md) | 方块状态的文本表示（不同概念）|

## 未覆盖范围

- `PlainTextContents.MAP_CODEC`（`Codec.STRING.fieldOf("text")` 的往返）
- 其它 `ComponentContents` 实现：
  - `TranslatableContents`（依赖 `Language`，属 world 型）
  - `ScoreContents` / `SelectorContents`（依赖计分板 / 实体选择器）
  - `KeybindContents`（依赖客户端按键绑定）
  - `NbtContents`（依赖 NBT 数据源）
  - `ObjectContents`（依赖对象类型存储）
- `MutableComponent.append` / `withStyle` 的链式结构
- `Component.getString()` 的多层拼接（本能力只覆盖单层 literal）