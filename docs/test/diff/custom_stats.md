# 差分能力：`custom_stats`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 191 个能力。77 个单元，**全部一致**——过程中查出并修复了 **14 处缺失的统计常量**。

## 测的是什么

原版 77 个自定义统计都由 `Stats.makeCustomStat(name, formatter)` 声明：

```java
public static final Identifier LEAVE_GAME = makeCustomStat("leave_game", StatFormatter.DEFAULT);
public static final Identifier WALK_ONE_CM = makeCustomStat("walk_one_cm", StatFormatter.DISTANCE);
public static final Identifier DAMAGE_DEALT = makeCustomStat("damage_dealt", StatFormatter.DIVIDE_BY_TEN);
```

本能力比对**统计 ID 全集**与**格式化语义**两件事。

## 为什么统计 ID 必须全量比对

统计 ID 是**存档里的持久键**——`stats/<uuid>.json` 直接用这些字符串作字段名：

```json
{
  "minecraft:walk_one_cm": 12345,
  "minecraft:jump": 88
}
```

少一个常量意味着：

1. 对应行为**无法被记录**（该计数永远为 0）；
2. 更糟的是，**玩家存档里已有的该项统计在读取时会被静默丢弃**。

多一个或拼错则会在存档里写进原版读不懂的键。这类错误不会崩溃、不会报错，
只会让数据悄悄消失。

## 查出的 14 处缺失

Cubium 的 `src/common/stats/Stats.hpp` 只有 **63** 个常量，原版有 **77** 个：

| 分类 | 缺失的统计 |
|---|---|
| 动作计数 | `jump`、`drop` |
| 伤害（7 个）| `damage_dealt`、`damage_dealt_absorbed`、`damage_dealt_resisted`、`damage_taken`、`damage_blocked_by_shield`、`damage_absorbed`、`damage_resisted` |
| 交互与世界 | `animals_bred`、`fish_caught`、`talked_to_villager`、`traded_with_villager`、`eat_cake_slice` |

**伤害类整整缺了一整个分组**（7 项），说明当初是成批遗漏的。

补齐后 77/77 一致。

## 三个关键点

### 一、用 `format(12345)` 而不是 formatter 名字

`Stat` 类**不暴露** formatter（只有 `format(int)`）。因此本能力比对的是固定输入下
格式化后的**实际字符串**，这比比对枚举名更有意义——它同时验证了「统计存在」与
「格式化语义正确」。

不同 formatter 对 `12345` 的输出差异明显：

| formatter | `format(12345)` |
|---|---|
| `DEFAULT` | `12,345` |
| `TIME` | `10.29 min` |
| `DISTANCE` | `123.45 m` |
| `DIVIDE_BY_TEN` | `1234.50` |

### 二、oracle 用反射枚举，不硬编码

```java
for (Field field : Stats.class.getDeclaredFields()) {
    if (field.getType() == Identifier.class) { ... }
}
```

**原版新增统计时 oracle 自动跟上**，不会因为写死列表而漏掉。这是一条通用做法：
当参照实现提供反射可达的常量集合时，优先枚举而不是抄写。

### 三、探针表必须是 Cubium 的**真实状态**

这是本能力最关键的设计点，也是我中途改过一次的地方。

最初把探针表填成了原版 77 项（`format` 的输出逐项抄写）。这样探针和 oracle
输出完全相同，**差异恒为 0**——能力表面上「通过」，但实际上什么都测不到。

正确做法：探针表只列 **Cubium 真实存在的 63 项**，命中不到就返回 `MISSING`：

```cpp
for (const auto& entry : CUBIUM_STATS) {
    if (line == entry.name) {
        return json{{"unit", line}, {"result", entry.formatted}}.dump();
    }
}
return json{{"unit", line}, {"result", "MISSING"}}.dump();
```

于是 14 个缺失项各自以东版 `12,345` / Cubium `MISSING` 的形式暴露。

> **教训**：把参照实现的输出抄进被测侧的探针表，会让能力变成「自己跟自己比」。
> 探针必须读**被测实现**的真实状态。

## 单元格式

单个统计名（不含 `minecraft:` 前缀），例如：

```
walk_one_cm
damage_dealt
jump
```

单元表由探针的 `list` 输出，内容为原版 77 项全集——这样 Cubium 缺项才能被探测到。

## 未覆盖范围

- **`Stats` 的注册表统计类型**（`BLOCK_MINED`、`ITEM_CRAFTED` 等 9 个 `StatType`）：
  它们的 ID 由方块/物品注册表派生，已由 `block_state_ids` / `item_summary` 等能力覆盖。
- **`StatFormatter` 的完整格式化行为**：本能力只用 `12345` 一个采样点。
  `TODO: 时间格式在超过 1 小时、超过 1 天时的边界行为（`10.29 min` 之外的分支）未覆盖。`
- **`ServerStatsCounter` 的存档读写**：属于有状态能力，尚未覆盖。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`objective_criteria`](./objective_criteria.md) | 计分板准则，与统计共享 `Stat` 抽象 |
| [`advancement`](./advancement.md) | 成就系统的进度触发大量统计 |