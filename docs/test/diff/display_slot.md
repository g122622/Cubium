# 差分能力：`display_slot`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 163 个能力，31 个用例全部一致。补上了**计分板**这一覆盖最薄弱的领域——
此前该领域只有 [`advancement`](./advancement.md) 一个能力。

## 核心发现一：序列化名有两种命名风格混用

```java
LIST(0, "list"),
SIDEBAR(1, "sidebar"),
BELOW_NAME(2, "below_name"),
TEAM_BLACK(3, "sidebar.team.black"),
TEAM_DARK_BLUE(4, "sidebar.team.dark_blue"),
...
TEAM_WHITE(18, "sidebar.team.white");
```

| 枚举 | 序列化名 |
|---|---|
| `LIST` | `list` |
| `SIDEBAR` | `sidebar` |
| `BELOW_NAME` | `below_name` |
| **`TEAM_BLACK`** | **`sidebar.team.black`** |
| **`TEAM_DARK_BLUE`** | **`sidebar.team.dark_blue`** |
| **`TEAM_LIGHT_PURPLE`** | **`sidebar.team.light_purple`** |

**团队槽用点 `.` 分隔，前三个用下划线 `_`**——同一枚举内风格不统一。

`TEAM_DARK_BLUE` 的枚举名是下划线（`TEAM_DARK_BLUE`），序列化名却是
`sidebar.team.dark_blue`（点在层级、下划线在词内）。**若 Cubium 用「枚举名小写」生成序列化名，
会得到 `team_dark_blue`——完全错误。**

`names` 单元的完整输出：

```
list,sidebar,below_name,sidebar.team.black,sidebar.team.dark_blue,sidebar.team.dark_green,
sidebar.team.dark_aqua,sidebar.team.dark_red,sidebar.team.dark_purple,sidebar.team.gold,
sidebar.team.gray,sidebar.team.dark_gray,sidebar.team.blue,sidebar.team.green,
sidebar.team.aqua,sidebar.team.red,sidebar.team.light_purple,sidebar.team.yellow,
sidebar.team.white
```

## 核心发现二：`BY_ID` 越界策略是 `ZERO`，返回 `LIST` 而非 `null`

```java
public static final IntFunction<DisplaySlot> BY_ID =
    ByIdMap.continuous(DisplaySlot::id, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
```

| 单元 | 结果 |
|---|---|
| `byId\|0` | `LIST` |
| `byId\|1` | `SIDEBAR` |
| `byId\|2` | `BELOW_NAME` |
| `byId\|18` | `TEAM_WHITE` |
| **`byId\|19`** | **`LIST`**（越界）|
| **`byId\|-1`** | **`LIST`**（越界）|
| **`byId\|1000`** | **`LIST`**（越界）|

**`ZERO` 策略的语义是「返回 `values()[0]`」，即 `LIST`**——不是抛异常，也不是返回 `null`。

若 Cubium 用 `std::vector::at` 或返回 `std::optional`，越界行为会不同。这是**网络包解析的容错点**：
非法 id 会被静默当成 `LIST`。

## 核心发现三：`teamColorToSlot` 只接受 16 个颜色，6 个格式码返回 `null`

```java
public static @Nullable DisplaySlot teamColorToSlot(ChatFormatting formatting) {
    return switch (formatting) {
        case BLACK -> TEAM_BLACK;
        ...
        case WHITE -> TEAM_WHITE;
        case BOLD, ITALIC, UNDERLINE, RESET, OBFUSCATED, STRIKETHROUGH -> null;
    };
}
```

| 单元 | 结果 |
|---|---|
| `teamColor\|BLACK` | `TEAM_BLACK` |
| `teamColor\|DARK_BLUE` | `TEAM_DARK_BLUE` |
| `teamColor\|GOLD` | `TEAM_GOLD` |
| `teamColor\|LIGHT_PURPLE` | `TEAM_LIGHT_PURPLE` |
| `teamColor\|WHITE` | `TEAM_WHITE` |
| **`teamColor\|BOLD`** | **`null`** |
| **`teamColor\|ITALIC`** | **`null`** |
| **`teamColor\|UNDERLINE`** | **`null`** |
| **`teamColor\|STRIKETHROUGH`** | **`null`** |
| **`teamColor\|OBFUSCATED`** | **`null`** |
| **`teamColor\|RESET`** | **`null`** |

**`RESET` 返回 `null`**——它在 `ChatFormatting` 里属于「格式」而非「颜色」，尽管语义上「重置」看起来像颜色。

`ChatFormatting` 共 22 项：16 个颜色 + `RESET` + 5 个格式（`BOLD` / `ITALIC` / `UNDERLINE` /
`STRIKETHROUGH` / `OBFUSCATED`）。**格式码 6 个全部返回 `null`**。

若 Cubium 只判断 `isColor()` 而不特别处理 `RESET`，可能误给 `RESET` 返回一个槽位。

## 核心发现四：id 与序数一致，但不能依赖

| 单元 | 结果 |
|---|---|
| `id\|LIST` | `0` |
| `id\|BELOW_NAME` | `2` |
| `id\|TEAM_BLACK` | `3` |
| `id\|TEAM_WHITE` | `18` |
| `count` | `19` |

构造器显式传入 `id`，恰好等于 `ordinal()`。但 `BY_ID` 走的是 `DisplaySlot::id` 方法引用，
**不是序数**——若未来插入一个 id 不连续的枚举项，依赖序数的实现会静默出错。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`advancement`](./advancement.md) | 进度系统（计分板领域的另一部分）|
| [`chat_formatting`](./chat_formatting.md) | `ChatFormatting` 枚举（本能力的 `teamColor` 输入）|
| [`text_color`](./text_color.md) | 颜色解析 |
| [`style_apply`](./style_apply.md) | `Style` 的格式应用 |

## 未覆盖范围

- `DisplaySlot.CODEC`（`StringRepresentable.fromEnum` 的序列化往返）
- `Scoreboard` / `Objective` / `PlayerTeam` 的完整逻辑（依赖世界状态，属 world 型）
- `ScoreboardSaveData` 的 NBT 往返
- `PlayerScores` 的分数存取与锁定
- `Scoreboard.getNameForSlot` / `DisplaySlot` 的实际渲染效果（客户端）