# 差分能力：`difficulty`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 168 个能力，29 个用例全部一致。**难度**此前未覆盖——它是第一个使用
**`OutOfBoundsStrategy.WRAP`** 的枚举能力。

## 核心发现一：`BY_ID` 用 `WRAP` 回绕，不是 `ZERO`

```java
private static final IntFunction<Difficulty> BY_ID =
    ByIdMap.continuous(Difficulty::getId, values(), ByIdMap.OutOfBoundsStrategy.WRAP);
```

| id | 结果 | 计算 |
|---|---|---|
| `0` | `PEACEFUL` | |
| `1` | `EASY` | |
| `2` | `NORMAL` | |
| `3` | `HARD` | |
| **`4`** | **`PEACEFUL`** | 4 % 4 = 0 |
| **`5`** | **`EASY`** | 5 % 4 = 1 |
| **`-1`** | **`HARD`** | -1 → 3 |
| **`-4`** | **`PEACEFUL`** | -4 → 0 |
| **`1000`** | **`PEACEFUL`** | 1000 % 4 = 0 |

### 与前三个能力的策略对比

| 能力 | 越界策略 | `id=5` 的结果 |
|---|---|---|
| [`display_slot`](./display_slot.md) | `ZERO` | `LIST` |
| [`objective_criteria`](./objective_criteria.md) | （无 `BY_ID`）| — |
| [`team_rule`](./team_rule.md) | `ZERO` | `ALWAYS` |
| **`difficulty`** | **`WRAP`** | **`EASY`** |

**`WRAP` 与 `ZERO` 在 `id=4` 时碰巧一致**（都回绕/退化到第一项），**`id=5` 才分道扬镳**。

这是本能力最重要的用例：**只有 `byId|5` 能区分两种策略**。若只测 `byId|4`，错误的 `ZERO`
实现会静默通过。

### 负数回绕需要额外一次修正

Java 的 `%` 对负数结果仍为负（`-1 % 4 = -1`），所以 `ByIdMap` 的 `WRAP` 实现会再补一次：

```cpp
i32 index = id % 4;
if (index < 0) {
    index += 4;
}
```

**C++ 的 `%` 与 Java 一致**（向零截断），所以这段逻辑可以直接照搬。

## 核心发现二：翻译键有两层后缀，且 `.info` 追加在末尾

```java
public Component getDisplayName() {
    return Component.translatable("options.difficulty." + this.key);
}
public Component getInfo() {
    return Component.translatable("options.difficulty." + this.key + ".info");
}
```

| 单元 | 结果 |
|---|---|
| `displayName\|PEACEFUL` | `options.difficulty.peaceful` |
| `displayName\|HARD` | `options.difficulty.hard` |
| **`info\|PEACEFUL`** | **`options.difficulty.peaceful.info`** |
| **`info\|EASY`** | **`options.difficulty.easy.info`** |
| **`info\|HARD`** | **`options.difficulty.hard.info`** |

**`.info` 是在完整键之后追加**，不是 `options.difficulty.<key>.info` 之外的变体。

若 Cubium 写成 `options.<key>.difficulty.info` 或用 `_info`，键会全部错。

## 核心发现三：`byName` 走 `CODEC.byName`，失败返回 `null` 不抛异常

```java
public static @Nullable Difficulty byName(String name) {
    return CODEC.byName(name);
}
```

| 单元 | 结果 |
|---|---|
| `byName\|peaceful` | `PEACEFUL` |
| `byName\|hard` | `HARD` |
| **`byName\|PEACEFUL`** | **`null`**（**大小写敏感**）|
| **`byName\|`**（空串） | **`null`** |

`StringRepresentable.fromEnum` 的 `byName` 对未知名返回 `null`——**不抛异常**。
这与 `Enum.valueOf`（抛 `IllegalArgumentException`）不同。

## 核心发现四：`getKey()` 与 `getSerializedName()` 返回同一字段

```java
public String getKey() { return this.key; }
@Override
public String getSerializedName() { return this.key; }
```

| 单元 | 结果 |
|---|---|
| `serial\|PEACEFUL` | `peaceful` |
| `serial\|EASY` | `easy` |
| `serial\|NORMAL` | `normal` |
| `serial\|HARD` | `hard` |
| `key\|PEACEFUL` | `peaceful` |
| `key\|HARD` | `hard` |

序列化名全小写、无下划线、无驼峰——**与本项目此前遇到的多种命名风格都不同**。

## 核心发现五：`byId` 已标记 `@Deprecated` 但行为有效

```java
@Deprecated
public static Difficulty byId(int id) {
    return BY_ID.apply(id);
}
```

`@Deprecated` 只是「不推荐公开使用」，**逻辑完全可用**。原版的内部代码改为直接用 `BY_ID`
（例如 `STREAM_CODEC = ByteBufCodecs.idMapper(BY_ID, Difficulty::getId)`）。

oracle 为验证越界语义必须调用它——若要避开 `@Deprecated`，需要反射读 `BY_ID`，
本能力选择直接调用并接受 deprecation 警告。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`display_slot`](./display_slot.md) | `ZERO` 越界策略的对照案例 |
| [`team_rule`](./team_rule.md) | `ZERO` 越界策略的对照案例 |
| [`objective_criteria`](./objective_criteria.md) | 计分判据 |
| [`boss_bar`](./boss_bar.md) | 另一个界面枚举 |

## 未覆盖范围

- `Difficulty.CODEC` / `STREAM_CODEC` 的序列化往返
- 难度对**游戏行为**的影响（怪物伤害倍率、饥饿、生成规则，属 world 型）
- `ServerLevel.getDifficulty()` 的传递链
- 难度在 `server.properties` 中的持久化
- `Difficulty` 与 `options.difficulty.*` 语言键的**实际翻译文本**（本能力只验证键）