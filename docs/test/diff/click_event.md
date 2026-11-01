# 差分能力：`click_event`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 158 个能力，23 个用例全部一致。这是 [`style_apply`](./style_apply.md) / [`text_color`](./text_color.md)
的延续——覆盖 `Style` 里的 `clickEvent` 字段所属的 `ClickEvent.Action`。

## 核心发现一：`OPEN_FILE` 是唯一 `allowFromServer = false` 的动作

```java
public static enum Action implements StringRepresentable {
    OPEN_URL("open_url", true, ...),
    OPEN_FILE("open_file", false, ...),        // 唯一一个 false
    RUN_COMMAND("run_command", true, ...),
    SUGGEST_COMMAND("suggest_command", true, ...),
    SHOW_DIALOG("show_dialog", true, ...),
    CHANGE_PAGE("change_page", true, ...),
    COPY_TO_CLIPBOARD("copy_to_clipboard", true, ...),
    CUSTOM("custom", true, ...);
```

| 单元 | 结果 |
|---|---|
| **`allowed\|OPEN_FILE`** | **`False`** |
| `allowed\|OPEN_URL` | `True` |
| `allowed\|RUN_COMMAND` | `True` |
| `allowed\|CUSTOM` | `True` |

**语义**：`OPEN_FILE` 会打开**服务器本地**的文件路径。若允许服务端下发，恶意服务器可以让客户端
打开任意文件——这是安全边界。

## 核心发现二：`filterForSerialization` 只对 `OPEN_FILE` 报错

```java
public static DataResult<Action> filterForSerialization(Action action) {
    return !action.isAllowedFromServer()
        ? DataResult.error(() -> "Click event type not allowed: " + action)
        : DataResult.success(action, Lifecycle.stable());
}
```

| 单元 | 结果 |
|---|---|
| **`filter\|OPEN_FILE`** | **`False`** |
| `filter\|OPEN_URL` | `True` |
| `filter\|RUN_COMMAND` | `True` |
| `filter\|CHANGE_PAGE` | `True` |

**注意有两个 codec**：

```java
public static final Codec<Action> UNSAFE_CODEC = StringRepresentable.fromEnum(Action::values);
public static final Codec<Action> CODEC = UNSAFE_CODEC.validate(Action::filterForSerialization);
```

**`UNSAFE_CODEC` 不做过滤**——它接受 `OPEN_FILE`。只有 `CODEC` 拒绝。

命名很直白：用 `UNSAFE_CODEC` 就是在绕过这个安全检查。

## 核心发现三：`ChangePage` 的页码是 `POSITIVE_INT`，从 1 开始

```java
public static final Codec<Integer> POSITIVE_INT =
    intRangeWithMessage(1, Integer.MAX_VALUE, v -> "Value must be positive: " + v);
```

| 单元 | 结果 |
|---|---|
| **`page\|0`** | **`False`** |
| **`page\|1`** | **`True`** |
| `page\|2` | `True` |
| `page\|-1` | `False` |
| `page\|-2147483648` | `False` |
| `page\|2147483647` | `True` |

**`0` 非法**——书页从 `1` 开始编号。`POSITIVE_INT` 与 `NON_NEGATIVE_INT`（`[0, MAX]`）是两个不同的
codec，写错就会接受 `0`。

## 核心发现四：动作名是纯 `snake_case`，无前缀

| 枚举 | 序列化名 |
|---|---|
| `OPEN_URL` | `open_url` |
| `OPEN_FILE` | `open_file` |
| `RUN_COMMAND` | `run_command` |
| `SUGGEST_COMMAND` | `suggest_command` |
| `SHOW_DIALOG` | `show_dialog` |
| `CHANGE_PAGE` | `change_page` |
| `COPY_TO_CLIPBOARD` | `copy_to_clipboard` |
| `CUSTOM` | `custom` |

**`names` 单元的完整输出**：

```
open_url,open_file,run_command,suggest_command,show_dialog,change_page,copy_to_clipboard,custom
```

**没有命名空间前缀**——与 `Identifier` 不同。

## 本轮踩坑：`Action` 的静态初始化需要 bootstrap

首次运行：

```
Exception in InitializerError
```

`ClickEvent.Action` 的静态字段 `UNSAFE_CODEC` / `CODEC` 会经 `StringRepresentable.fromEnum` 与
注册表交互，必须先行 `Bootstrap.bootStrap()`。

修复（与 [`chunk_region`](./chunk_region.md) 同一模式）：

```java
net.minecraft.SharedConstants.tryDetectVersion();
net.minecraft.server.Bootstrap.bootStrap();
```

**顺序不能反**——`bootStrap()` 要求先设置游戏版本。

### 附带发现：oracle 的 `page` 校验需要防溢出

我最初写成：

```java
(int) Long.parseLong(parts[1]) >= POSITIVE_INT_MIN
```

`Long.parseLong("2147483648")` 得 `2147483648L`，强转 `int` 会溢出成 `-2147483648`，于是
「超出范围的合法正数」被误判为非法。

正确写法是用 `Integer.parseInt` 并捕获 `NumberFormatException`：

```java
try {
    int page = Integer.parseInt(parts[1]);
    out.addProperty("value", page >= POSITIVE_INT_MIN);
} catch (NumberFormatException exception) {
    out.addProperty("value", false);
}
```

**这是 `_patterns.md` 中「Java 的 `(int)` 转换对超界值饱和 / 溢出」条目的一个实例**——
本能力没有踩到实际的测试失败，因为在写第一版后就自查发现并修正了。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`style_apply`](./style_apply.md) | `Style` 的格式应用（`clickEvent` 是其字段）|
| [`text_color`](./text_color.md) | `Style` 的颜色值 |
| [`chat_formatting`](./chat_formatting.md) | `ChatFormatting` 枚举 |
| [`command_argument`](./command_argument.md) | 命令参数类型（`RUN_COMMAND` 的目标）|

## 未覆盖范围

- `ClickEvent` 的各个 record 类型（`OpenUrl` / `RunCommand` / `ChangePage` / …）的 codec 往返
- `ClickEvent.CODEC` 的完整派发（`dispatch("action", ...)`）
- `HoverEvent.Action`（结构类似，可另立能力）
- `ClickEvent.Action` 在客户端处理中的实际效果（客户端已停止维护）