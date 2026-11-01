# 差分能力：`team_rule`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 165 个能力，27 个用例全部一致。这是 [`display_slot`](./display_slot.md) /
[`objective_criteria`](./objective_criteria.md) 的第三个计分板同伴——补上**队伍**这一层。

## 核心发现一：两个枚举结构相同，但翻译键前缀不同

`net.minecraft.world.scores.Team` 里有两个内部枚举，成员与 `id` 完全对称：

| `CollisionRule` | `Visibility` | id |
|---|---|---|
| `ALWAYS("always")` | `ALWAYS("always")` | 0 |
| `NEVER("never")` | `NEVER("never")` | 1 |
| `PUSH_OTHER_TEAMS("pushOtherTeams")` | `HIDE_FOR_OTHER_TEAMS("hideForOtherTeams")` | 2 |
| `PUSH_OWN_TEAM("pushOwnTeam")` | `HIDE_FOR_OWN_TEAM("hideForOwnTeam")` | 3 |

但 `getDisplayName()` 的前缀不同：

```java
// CollisionRule
public Component getDisplayName() { return Component.translatable("team.collision." + this.name); }

// Visibility
public Component getDisplayName() { return Component.translatable("team.visibility." + this.name); }
```

| 单元 | 结果 |
|---|---|
| **`collisionDisplay\|ALWAYS`** | **`team.collision.always`** |
| **`visibilityDisplay\|ALWAYS`** | **`team.visibility.always`** |
| `collisionDisplay\|PUSH_OTHER_TEAMS` | `team.collision.pushOtherTeams` |
| `visibilityDisplay\|HIDE_FOR_OWN_TEAM` | `team.visibility.hideForOwnTeam` |

**同名字、不同键**——若 Cubium 共用一个翻译键生成函数并漏传前缀，两个枚举会撞键。

## 核心发现二：序列化名混合全小写与 camelCase

| 枚举 | 序列化名 |
|---|---|
| `ALWAYS` | **`always`** |
| `NEVER` | **`never`** |
| `PUSH_OTHER_TEAMS` | **`pushOtherTeams`** |
| `PUSH_OWN_TEAM` | **`pushOwnTeam`** |
| `HIDE_FOR_OTHER_TEAMS` | **`hideForOtherTeams`** |
| `HIDE_FOR_OWN_TEAM` | **`hideForOwnTeam`** |

**`always` / `never` 是全小写，其余四个是 camelCase**——同一枚举内两种风格并存。

这与 [`objective_criteria`](./objective_criteria.md)（全 camelCase）、
[`display_slot`](./display_slot.md)（全下划线）三种风格并存，说明**计分板领域的命名风格完全没有统一**。

## 核心发现三：复数与单数的不对称

| 枚举 | 序列化名 | 复数形态 |
|---|---|---|
| `PUSH_OTHER_TEAMS` | `pushOtherTeams` | **Teams（复数）** |
| `PUSH_OWN_TEAM` | `pushOwnTeam` | **Team（单数）** |
| `HIDE_FOR_OTHER_TEAMS` | `hideForOtherTeams` | **Teams（复数）** |
| `HIDE_FOR_OWN_TEAM` | `hideForOwnTeam` | **Team（单数）** |

**「其他队伍」用复数，「自己的队伍」用单数**——语义上说得通（其他队伍有多个，自己的只有一个），
但**手写单元表时极易把 `pushOwnTeams` 写错**。

## 核心发现四：`BY_ID` 越界返回 `ALWAYS`

```java
private static final IntFunction<CollisionRule> BY_ID =
    ByIdMap.continuous(rule -> rule.id, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
```

| 单元 | 结果 |
|---|---|
| `collisionById\|0` | `ALWAYS` |
| `collisionById\|3` | `PUSH_OWN_TEAM` |
| **`collisionById\|4`** | **`ALWAYS`**（越界）|
| **`collisionById\|-1`** | **`ALWAYS`**（越界）|
| **`collisionById\|1000`** | **`ALWAYS`**（越界）|
| `visibilityById\|3` | `HIDE_FOR_OWN_TEAM` |
| **`visibilityById\|4`** | **`ALWAYS`** |

与 [`display_slot`](./display_slot.md) 的 `OutOfBoundsStrategy.ZERO` 完全一致。

## 核心发现五：`getDisplayName()` 返回 `TranslatableContents`，可取其键

oracle 通过类型转换取翻译键：

```java
private static String translationKey(Component component) {
    if (component.getContents() instanceof TranslatableContents translatable) {
        return translatable.getKey();
    }
    throw new IllegalStateException(...);
}
```

**这是验证「翻译键构造」的正确方式**——比直接调 `getString()` 好（后者依赖 `Language` 加载，
属 world 型）。若类型不是 `TranslatableContents` 会立刻抛异常，暴露实现变化。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`display_slot`](./display_slot.md) | 计分板显示槽（含 `sidebar.team.*`）|
| [`objective_criteria`](./objective_criteria.md) | 计分判据 |
| [`chat_formatting`](./chat_formatting.md) | 队伍颜色 |
| [`plain_text_contents`](./plain_text_contents.md) | `TranslatableContents` 的兄弟类型 |

## 未覆盖范围

- `Team.isAlliedTo`（`this == other` 的 identity 比较，需要真实 `Team` 实例）
- `Team.CollisionRule.CODEC` / `STREAM_CODEC` 的序列化往返
- `PlayerTeam.getFormattedName`（依赖队伍颜色与 `Component` 拼接）
- 碰撞规则的**实际物理效果**（实体互相推挤，属 world 型）
- 可见性规则的**实际渲染效果**（客户端）