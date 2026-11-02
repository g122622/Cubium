# 差分能力：`text_formatting_name`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 171 个能力，37 个用例全部一致。**探针直接调用 Cubium 的 `mc::text::toName` /
`mc::text::fromName`**，是继 [`text_formatting_color`](./text_formatting_color.md) 之后
第二个「对接真实实现」的能力。

**本能力未发现 Cubium 缺陷**——`toName` / `fromName` 实现正确。

## 核心发现一：原版 `getName()` 与 `getSerializedName()` 返回同一值

```java
public String getName() { return this.name.toLowerCase(Locale.ROOT); }
public String getSerializedName() { return this.getName(); }   // 直接委托
```

两者都是 `name.toLowerCase(Locale.ROOT)`。

**与 [`display_slot`](./display_slot.md) 的 `sidebar.team.black` 不同**（那里大小写+点分隔可变），
这里的名字是**严格的小写 snake_case**。

## 核心发现二：22 个名字的完整列表

```
black,dark_blue,dark_green,dark_aqua,dark_red,dark_purple,gold,gray,dark_gray,
blue,green,aqua,red,light_purple,yellow,white,
obfuscated,bold,strikethrough,underline,italic,reset
```

| 特征 | 说明 |
|---|---|
| 全小写 | `dark_blue` 不是 `DARK_BLUE` |
| snake_case | `dark_blue` 不是 `darkBlue` |
| 顺序 | 16 个颜色在前，6 个格式在后，与枚举序一致 |

`names` 单元通过原版 `ChatFormatting.getNames(true, true)` 获取——
它的实现是 `for (values()) if ((isColor && !colors) || (isFormat && !formats)) continue; add(getName())`。

**Cubium 侧 `text::toName` 逐项输出完全相同。**

## 核心发现三：`fromName` 大小写不敏感

Cubium 的 `fromName` 先转小写再查表：

```cpp
std::transform(name.begin(), name.end(), std::back_inserter(lowerName),
    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
```

| 单元 | 结果 |
|---|---|
| `fromName\|dark_blue` | `DARK_BLUE` |
| **`fromName\|DARK_BLUE`** | **`DARK_BLUE`** |
| **`fromName\|Dark_Blue`** | **`DARK_BLUE`** |
| `fromName\|bold` | `BOLD` |
| `fromName\|reset` | `RESET` |

三种大小写写法全部命中。

**原版没有 `fromName` 这个入口**（只有 `getByCode(char)`），所以本能力用两个方式验证：

1. **`names`**：`toName` 的输出必须与原版 `getNames(true, true)` 逐字相同
2. **`roundTrip`**：`fromName(toName(x)) == x`，验证正向/反向映射能互相还原

## 核心发现四：`darkBlue` 未命中——snake_case 才是规范形式

| 单元 | 结果 |
|---|---|
| **`fromName\|darkBlue`** | **`NONE`** |
| `fromName\|not_a_color` | `NONE` |
| `fromName\|`（空串） | `NONE` |

**`fromName` 只做大小写归一，不做下划线插入**——`darkBlue`（驼峰）匹配不到 `dark_blue`。

这是正确的：`fromName` 只需能还原 `getName()` 的输出，而 `getName()` 从不产生驼峰形式。

## 核心发现五：`roundTrip` 全部为真

| 单元 | 结果 |
|---|---|
| `roundTrip\|BLACK` | `true` |
| `roundTrip\|DARK_BLUE` | `true` |
| `roundTrip\|LIGHT_PURPLE` | `true` |
| `roundTrip\|BOLD` | `true` |
| `roundTrip\|RESET` | `true` |

覆盖了「单词名」（`black`）、「双词下划线名」（`dark_blue`）、「格式码」（`bold`）、
「特殊项」（`reset`）四类——**往返性成立**。

## 核心发现六：`TextFormatting::None` 不在原版枚举中

Cubium 的 `TextFormatting` 有 23 个值（22 真实 + `None = 255` 哨兵），原版只有 22 个。

本能力的 `count` 单元输出 `22`（排除 `None`），`names` 也不包含 `None`。

`toName(None)` 返回空串——与原版无对应项，故未纳入单元。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`text_formatting_color`](./text_formatting_color.md) | 同族：颜色查询（已修复 `findFormattingARGB`）|
| [`chat_formatting`](./chat_formatting.md) | 手抄转写的旧版（应改造）|
| [`style_apply`](./style_apply.md) | `Style` 格式应用（手抄转写，应改造）|
| [`display_slot`](./display_slot.md) | 另一种命名风格（点分隔）|

## 未覆盖范围

- `ChatFormatting.stripFormatting(String)`（正则剥离，Cubium 有 `stripColor` 之类的实现，
  但接口名与语义需单独核对）
- `ChatFormatting.getByCode(char)`（已在 [`text_formatting_color`](./text_formatting_color.md) 覆盖）
- `ChatFormatting.getNames(boolean, boolean)` 的**过滤参数组合**（本能力只用 `(true, true)`）
- `TextFormatting::None` 的对外行为（原版无对应项）