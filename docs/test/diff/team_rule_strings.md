# 差分能力：`team_rule_strings`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 174 个能力，24 个用例全部一致。**探针直接调用 Cubium 的 `mc::scoreboard::teamVisibilityParse`
等**，是继 [`text_formatting_color`](./text_formatting_color.md) / [`style_tristate`](./style_tristate.md)
之后第四个「对接真实实现」的能力。

本能力暴露并修复了 **1 处真实缺陷**。

## 发现的缺陷：`fromString` 对未知输入静默回退到 `Always`

### 修复前

```cpp
// TeamEnums.hpp（修复前）
[[nodiscard]] inline TeamVisibility teamVisibilityFromString(const std::string& str) noexcept
{
    if (str == "never") { return TeamVisibility::Never; }
    else if (str == "hideForOtherTeams") { return TeamVisibility::HideForOtherTeams; }
    else if (str == "hideForOwnTeam") { return TeamVisibility::HideForOwnTeam; }
    return TeamVisibility::Always;          // ← 未知输入静默变成 Always
}
```

### 差异实测（7 处）

| 单元 | 原版 | Cubium（修复前） |
|---|---|---|
| `visibilityFromString\|dunno` | **`null`** | `ALWAYS` |
| `visibilityFromString\|`（空串） | **`null`** | `ALWAYS` |
| `visibilityFromString\|ALWAYS` | **`null`** | `ALWAYS` |
| `visibilityFromString\|hide_for_other_teams` | **`null`** | `ALWAYS` |
| `collisionFromString\|dunno` | **`null`** | `ALWAYS` |
| `collisionFromString\|`（空串） | **`null`** | `ALWAYS` |
| `collisionFromString\|push_other_teams` | **`null`** | `ALWAYS` |

原版 `StringRepresentable.fromEnum` 的 `CODEC.byName` 对未知输入返回 `null`，
**不抛异常、不回退**。

### 为什么这是缺陷

静默回退会导致**真实的玩法错误**。最严重的是存档读取路径：

```cpp
// ScoreboardSaveData.cpp（修复前）
team->setNameTagVisibility(teamVisibilityFromString(teamData.nameTagVisibility));
team->setDeathMessageVisibility(teamVisibilityFromString(teamData.deathMessageVisibility));
team->setCollisionRule(teamCollisionRuleFromString(teamData.collisionRule));
```

存档里若是 `"nameTagVisibility": "hide_for_other_teams"`（下划线，或任何手改/未来版本的值），
Cubium 会**静默变成 `always`**——名称标签全部显示出来。原版则会读取失败并保留原值。

命令路径 `TeamCommand.cpp` 早已发现这个坑，用**反向核对**绕过：

```cpp
const auto visibility = scoreboard::teamVisibilityFromString(visStr);
// teamVisibilityFromString 对未知值回退 Always，用原字符串核对以拒绝非法输入。
if (scoreboard::teamVisibilityToString(visibility) != visStr) { /* 报错 */ }
```

**这种写法虽然能用，但说明底层接口语义错了**——每个调用方都得记得核对。

### 修复

新增严格解析函数，返回 `std::optional`，与原版 `CODEC.byName` 语义一致：

```cpp
[[nodiscard]] inline std::optional<TeamVisibility> teamVisibilityParse(const std::string& str) noexcept
{
    if (str == "always") { return TeamVisibility::Always; }
    else if (str == "never") { return TeamVisibility::Never; }
    else if (str == "hideForOtherTeams") { return TeamVisibility::HideForOtherTeams; }
    else if (str == "hideForOwnTeam") { return TeamVisibility::HideForOwnTeam; }
    return std::nullopt;
}

// 旧的宽松入口保留为薄包装，避免破坏既有调用方
[[nodiscard]] inline TeamVisibility teamVisibilityFromString(const std::string& str) noexcept
{
    return teamVisibilityParse(str).value_or(TeamVisibility::Always);
}
```

`TeamCollisionRule` 同构。

### 三个调用方全部改造

| 文件 | 改造前 | 改造后 |
|---|---|---|
| `TeamCommand.cpp`（nameTagVisibility）| 宽松 + 反向核对 | **严格解析** |
| `TeamCommand.cpp`（deathMessageVisibility）| 宽松 + 反向核对 | **严格解析** |
| `TeamCommand.cpp`（collisionRule）| 宽松 + 反向核对 | **严格解析** |
| `ScoreboardSaveData.cpp`（3 处）| 宽松（**无核对**）| **严格解析，失败保留默认值** |

存档路径的改造：

```cpp
// 严格解析：原版 CODEC.byName 对未知值返回 null。这里只在解析成功时设置，
// 失败则保留队伍默认值——避免未知字符串被静默写成 Always。
// TODO: 原版在解析失败时会让整个存档读取报错；Cubium 目前静默跳过，
// 待存档读取的错误上报链路补齐后改为返回错误。
if (const auto visibility = teamVisibilityParse(teamData.nameTagVisibility)) {
    team->setNameTagVisibility(*visibility);
}
```

### 修复后

```
[1/1] team_rule_strings            通过      2.4s  24/24 一致
```

**7 处差异全部消失**，`mc_tests` 全量构建通过（退出码 0），无回归。

## 核心验证：序列化名是驼峰，不是下划线

| 枚举 | `getSerializedName()` |
|---|---|
| `ALWAYS` | `always` |
| `NEVER` | `never` |
| **`HIDE_FOR_OTHER_TEAMS`** | **`hideForOtherTeams`** |
| **`HIDE_FOR_OWN_TEAM`** | **`hideForOwnTeam`** |
| **`PUSH_OTHER_TEAMS`** | **`pushOtherTeams`** |
| **`PUSH_OWN_TEAM`** | **`pushOwnTeam`** |

**枚举名是 `SNAKE_CASE`，序列化名却是 `camelCase`。**

读源码时我曾误以为序列化名是下划线形式，用 `javap` 查常量池后确认是驼峰——**Cubium 的实现是正确的**。

### 单复数的不对称

| 枚举 | 序列化名 | 单复数 |
|---|---|---|
| `HIDE_FOR_OTHER_TEAMS` | `hideForOtherTeams` | **复数**（other team**s**）|
| `HIDE_FOR_OWN_TEAM` | `hideForOwnTeam` | **单数**（own team）|
| `PUSH_OTHER_TEAMS` | `pushOtherTeams` | **复数** |
| `PUSH_OWN_TEAM` | `pushOwnTeam` | **单数** |

「其他队伍」是复数、「自己队伍」是单数——这个不对称在枚举名里已体现，序列化名只是照搬。

**与 [`permission_level`](./permission_level.md) 的「两者都复数」、[`sound_source`](./sound_source.md)
的「枚举复数、名字单数」并列为第三种单复数模式。**

## 大小写与下划线均不被接受

| 单元 | 结果 | 说明 |
|---|---|---|
| `visibilityFromString\|always` | `ALWAYS` | ✓ |
| `visibilityFromString\|never` | `NEVER` | ✓ |
| `visibilityFromString\|hideForOtherTeams` | `HIDE_FOR_OTHER_TEAMS` | ✓ |
| `visibilityFromString\|hideForOwnTeam` | `HIDE_FOR_OWN_TEAM` | ✓ |
| **`visibilityFromString\|ALWAYS`** | **`null`** | **大小写敏感** |
| **`visibilityFromString\|hide_for_other_teams`** | **`null`** | **下划线不是合法形式** |

Cubium 用 `str == "hideForOtherTeams"` 精确比较，没有 `tolower`——与原版一致。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`team_rule`](./team_rule.md) | **手抄转写**的旧版（验证了同样的名字，但未对接 Cubium）|
| [`permission_level`](./permission_level.md) | 单复数「都复数」的对照 |
| [`sound_source`](./sound_source.md) | 单复数「枚举复数、名字单数」的对照 |
| [`style_tristate`](./style_tristate.md) | 同批「对接真实实现」的能力 |

**`team_rule` 已被本能力取代**：`team_rule` 验证的是同样的字符串映射，但探针是手抄转写，
无法暴露 Cubium 的缺陷。本能力用真实实现重验，发现了 `fromString` 的回退问题。
`team_rule` 应在后续轮次标注为「原版快照」或删除。

## 未覆盖范围

- 队伍颜色（`ScorePlayerTeam::setColor` 的 `TextFormatting` 语义）
- `allowFriendlyFire` / `seeFriendlyInvisibles` 的行为
- 队伍成员的添加/移除与 `addPlayerToTeam` 的冲突检查
- 死亡消息可见性的实际渲染
- 队伍名称/前缀/后缀的组件解析