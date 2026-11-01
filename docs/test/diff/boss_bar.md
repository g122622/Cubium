# 差分能力：`boss_bar`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 166 个能力，25 个用例全部一致。**Boss 栏**此前完全未覆盖——它是计分板之外的另一处「服务端下发
的界面元素」。

## 核心发现一：`BossBarColor` 的 `ChatFormatting` 映射三处名字不匹配

```java
public enum BossBarColor implements StringRepresentable {
    PINK("pink", ChatFormatting.RED),
    BLUE("blue", ChatFormatting.BLUE),
    RED("red", ChatFormatting.DARK_RED),
    GREEN("green", ChatFormatting.GREEN),
    YELLOW("yellow", ChatFormatting.YELLOW),
    PURPLE("purple", ChatFormatting.DARK_BLUE),
    WHITE("white", ChatFormatting.WHITE);
```

| `BossBarColor` | 序列化名 | `ChatFormatting` | 匹配？ |
|---|---|---|---|
| **`PINK`** | `pink` | **`RED`** | ✗ |
| `BLUE` | `blue` | `BLUE` | ✓ |
| **`RED`** | `red` | **`DARK_RED`** | ✗ |
| `GREEN` | `green` | `GREEN` | ✓ |
| `YELLOW` | `yellow` | `YELLOW` | ✓ |
| **`PURPLE`** | `purple` | **`DARK_BLUE`** | ✗ |
| `WHITE` | `white` | `WHITE` | ✓ |

| 单元 | 结果 |
|---|---|
| **`colorFormatting\|PINK`** | **`red`** |
| **`colorFormatting\|RED`** | **`dark_red`** |
| **`colorFormatting\|PURPLE`** | **`dark_blue`** |
| `colorFormatting\|BLUE` | `blue` |
| `colorFormatting\|GREEN` | `green` |
| `colorFormatting\|YELLOW` | `yellow` |
| `colorFormatting\|WHITE` | `white` |

**7 项里有 3 项名字不匹配**。若 Cubium 按名字直接映射（取同名 `ChatFormatting`）：

- `PINK` 会渲染成 `pink`——但 `ChatFormatting` 里**没有** `PINK`（只有 `LIGHT_PURPLE`）
- `RED` 会渲染成 `red`（亮红）而非 `dark_red`（暗红）
- `PURPLE` 会渲染成**不存在**的 `purple`

**正确做法是硬编码映射表，不能靠名字推导。**

## 核心发现二：两个枚举的命名风格不一致

| 枚举 | 风格 | 示例 |
|---|---|---|
| `BossBarColor` | **全小写** | `pink`、`blue`、`red` |
| `BossBarOverlay` | **保留下划线** | `notched_6`、`notched_10` |

| 单元 | 结果 |
|---|---|
| `colorName\|PINK` | `pink` |
| `colorName\|RED` | `red` |
| **`overlayName\|NOTCHED_6`** | **`notched_6`** |
| **`overlayName\|NOTCHED_20`** | **`notched_20`** |
| `overlayName\|PROGRESS` | `progress` |

`NOTCHED_6` 的序列化名是 **`notched_6`**，不是 `notched6`。若 Cubium 统一做「小写化 + 去下划线」，
overlay 会全部错。

**这与 [`team_rule`](./team_rule.md)、[`display_slot`](./display_slot.md)、
[`objective_criteria`](./objective_criteria.md) 一起，构成「命名风格毫无规律」的第四例。**

## 核心发现三：`BossBarOverlay` 的数字是「等分格数」

| 枚举 | 序列化名 | 等分 |
|---|---|---|
| `PROGRESS` | `progress` | 连续（无分格）|
| `NOTCHED_6` | `notched_6` | 6 等分 |
| `NOTCHED_10` | `notched_10` | 10 等分 |
| `NOTCHED_12` | `notched_12` | 12 等分 |
| `NOTCHED_20` | `notched_20` | 20 等分 |

**没有 `notched_8` 或 `notched_16`**——数字不是 2 的幂序列，是「原版用过的几种」。

`overlayCount` 输出 `5`，`colorCount` 输出 `7`。

## 核心发现四：`getName()` 与 `getSerializedName()` 返回同一字段

```java
public String getName() { return this.name; }
@Override
public String getSerializedName() { return this.name; }
```

**没有独立的人类可读名**——两个方法返回完全相同的字符串。

这与 [`objective_criteria`](./objective_criteria.md) 不同（那里 `getName()` 是判据名，
没有 `getSerializedName`），也与 [`display_slot`](./display_slot.md) 不同（那里只有 `getSerializedName`）。

## 本轮情况：一次通过

写代码前用 `javap -p -c` 反编译了两个枚举的常量池，确认了 7 + 5 项的全部字符串对，
再读源码确认 `ChatFormatting` 字段的具体值。

**关键方法是「反编译常量池」**：源码树里 `BossEvent.java` 存在，但用 `javap -c` 交叉验证可以
确保没有看漏（尤其 `PINK` 映 `RED` 这种反直觉项）。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`chat_formatting`](./chat_formatting.md) | `ChatFormatting` 枚举（本能力的渲染色来源）|
| [`team_rule`](./team_rule.md) | 队伍规则枚举 |
| [`display_slot`](./display_slot.md) | 计分板显示槽 |
| [`objective_criteria`](./objective_criteria.md) | 计分判据 |

## 未覆盖范围

- `BossEvent` 实例的 `progress` / `name` / `darkenScreen` 等状态（需要构造 `BossEvent` 子类）
- `BossEvent.CustomBossEvent` / `ServerBossEvent` 的玩家集合管理
- `BossBarColor.CODEC` / `BossBarOverlay.CODEC` 的序列化往返
- Boss 栏的网络包（`ClientboundBossEventPacket`）编解码
- 渲染效果（客户端）