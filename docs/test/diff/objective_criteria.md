# 差分能力：`objective_criteria`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 164 个能力，28 个用例全部一致。这是 [`display_slot`](./display_slot.md) 的配套——
两者共同撑起计分板领域的枚举层。

## 核心发现一：判据名是 camelCase，且 `EXPERIENCE` 叫 `xp`

```java
public static final ObjectiveCriteria DUMMY              = registerCustom("dummy");
public static final ObjectiveCriteria TRIGGER            = registerCustom("trigger");
public static final ObjectiveCriteria DEATH_COUNT        = registerCustom("deathCount");
public static final ObjectiveCriteria KILL_COUNT_PLAYERS = registerCustom("playerKillCount");
public static final ObjectiveCriteria KILL_COUNT_ALL     = registerCustom("totalKillCount");
public static final ObjectiveCriteria HEALTH             = registerCustom("health", true, HEARTS);
public static final ObjectiveCriteria FOOD               = registerCustom("food", true, INTEGER);
public static final ObjectiveCriteria AIR                = registerCustom("air", true, INTEGER);
public static final ObjectiveCriteria ARMOR              = registerCustom("armor", true, INTEGER);
public static final ObjectiveCriteria EXPERIENCE         = registerCustom("xp", true, INTEGER);
public static final ObjectiveCriteria LEVEL              = registerCustom("level", true, INTEGER);
```

| 常量 | 判据名 |
|---|---|
| `DUMMY` | `dummy` |
| `TRIGGER` | `trigger` |
| **`DEATH_COUNT`** | **`deathCount`** |
| **`KILL_COUNT_PLAYERS`** | **`playerKillCount`** |
| **`KILL_COUNT_ALL`** | **`totalKillCount`** |
| `HEALTH` | `health` |
| `FOOD` | `food` |
| `AIR` | `air` |
| `ARMOR` | `armor` |
| **`EXPERIENCE`** | **`xp`** |
| `LEVEL` | `level` |

**两处陷阱**：

1. **camelCase 而非 snake_case**——`deathCount`，不是 `death_count`。
   这与 [`display_slot`](./display_slot.md) 的下划线风格相反，说明**同一领域内命名风格也不统一**。
2. **`EXPERIENCE` 的判据名是 `xp`**——与常量名风马牛不相及。若按常量名推导会得到
   `experience`，玩家输入 `/scoreboard objectives add obj experience` 会被拒绝。

## 核心发现二：只有 `HEALTH` 用 `RenderType.HEARTS`

| 单元 | 结果 |
|---|---|
| **`renderType\|HEALTH`** | **`hearts`** |
| `renderType\|DUMMY` | `integer` |
| `renderType\|FOOD` | `integer` |
| `renderType\|LEVEL` | `integer` |

`RenderType` 只有 2 项：`INTEGER("integer")` 与 `HEARTS("hearts")`。
**`HEALTH` 是唯一的心形渲染**——这符合直觉（血量显示为爱心）。

## 核心发现三：`readOnly` 只对 6 个「世界状态派生」判据为 `true`

```java
DUMMY              = registerCustom("dummy");                    // false
TRIGGER            = registerCustom("trigger");                  // false
DEATH_COUNT        = registerCustom("deathCount");               // false
KILL_COUNT_PLAYERS = registerCustom("playerKillCount");          // false
KILL_COUNT_ALL     = registerCustom("totalKillCount");           // false
HEALTH             = registerCustom("health", true, HEARTS);     // true
FOOD               = registerCustom("food", true, INTEGER);      // true
AIR                = registerCustom("air", true, INTEGER);       // true
ARMOR              = registerCustom("armor", true, INTEGER);     // true
EXPERIENCE         = registerCustom("xp", true, INTEGER);        // true
LEVEL              = registerCustom("level", true, INTEGER);     // true
```

| 类别 | `readOnly` |
|---|---|
| `dummy` / `trigger` / `deathCount` / `playerKillCount` / `totalKillCount` | `false`（可被 `/scoreboard` 直接改）|
| **`health` / `food` / `air` / `armor` / `xp` / `level`** | **`true`**（由游戏状态自动派生）|
| `teamkill.*` / `killedByTeam.*` | `false` |

**注意 `deathCount` / `playerKillCount` / `totalKillCount` 也是 `false`**——尽管它们是「事件触发型」，
但因为是「累加计数」而非「即时状态」，所以仍可手动改。

## 核心发现四：`TEAM_KILL` 与 `KILLED_BY_TEAM` 的前缀风格不一致

```java
public static final ObjectiveCriteria[] TEAM_KILL = new ObjectiveCriteria[]{
    registerCustom("teamkill." + ChatFormatting.BLACK.getName()),   // teamkill.black
    ... 共 16 个颜色
};
public static final ObjectiveCriteria[] KILLED_BY_TEAM = new ObjectiveCriteria[]{
    registerCustom("killedByTeam." + ChatFormatting.BLACK.getName()), // killedByTeam.black
    ... 共 16 个颜色
};
```

| 数组 | 前缀 | 示例 |
|---|---|---|
| `TEAM_KILL` | **`teamkill.`**（全小写）| `teamkill.black` |
| `KILLED_BY_TEAM` | **`killedByTeam.`**（camelCase）| `killedByTeam.black` |

| 单元 | 结果 |
|---|---|
| `teamKill\|0` | `teamkill.black` |
| `teamKill\|1` | `teamkill.dark_blue` |
| `teamKill\|15` | `teamkill.white` |
| `killedByTeam\|0` | `killedByTeam.black` |
| `killedByTeam\|1` | `killedByTeam.dark_blue` |
| `killedByTeam\|15` | `killedByTeam.white` |
| `teamKillCount` | `16` |
| `killedByTeamCount` | `16` |

**`teamkill` 无驼峰，`killedByTeam` 有驼峰**——同一文件里两种风格并存。

## 本轮情况：一次通过

写代码前完整读取了 11 个静态常量的定义、16×2 个团队判据数组、`RenderType` 枚举，
并核实 `registerCustom` 只操作 `CUSTOM_CRITERIA` 映射表（**不接触注册表**，因此不需要 bootstrap）。

### 一个实现细节：反射读取常量

oracle 通过反射按名字读取 `ObjectiveCriteria` 的静态字段，避免手抄 11 个常量引用：

```java
Field field = ObjectiveCriteria.class.getField(constant.toUpperCase(Locale.ROOT));
return (ObjectiveCriteria) field.get(null);
```

这样即使常量列表变化，oracle 也能自适应（单元表变化时会立刻暴露）。**优于硬编码常量引用**
——后者在常量重命名时会编译失败而非运行期暴露。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`display_slot`](./display_slot.md) | 计分板显示槽（同领域）|
| [`advancement`](./advancement.md) | 进度系统 |
| [`chat_formatting`](./chat_formatting.md) | `ChatFormatting.getName()`（团队判据的后缀来源）|
| [`identifier_resolve`](./identifier_resolve.md) | 命名风格与解析 |

## 未覆盖范围

- `ObjectiveCriteria.CODEC`（序列化往返）
- `ObjectiveCriteria.byName` / `getByName`（当前版本无此公开方法）
- `ObjectiveCriteria.RenderType.byId`（`CODEC.byName(id, INTEGER)` 的默认回退）
- `CUSTOM_CRITERIA` 映射表的注册与查询
- `Objective` / `PlayerScores` 的完整计分逻辑（依赖世界状态）