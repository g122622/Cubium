# 差分能力：`permission_level`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 169 个能力，26 个用例全部一致。这是**第三种越界策略 `CLAMP` 的首次覆盖**——
前两个是 [`display_slot`](./display_slot.md) / [`team_rule`](./team_rule.md) 的 `ZERO`
与 [`difficulty`](./difficulty.md) 的 `WRAP`。

## 核心发现一：三种越界策略的完整对照

```java
private static final IntFunction<PermissionLevel> BY_ID =
    ByIdMap.continuous(level -> level.id, values(), ByIdMap.OutOfBoundsStrategy.CLAMP);
```

| id | 结果 |
|---|---|
| `0` | `ALL` |
| `4` | `OWNERS` |
| **`-1`** | **`ALL`**（钳到下界）|
| **`-100`** | **`ALL`** |
| **`5`** | **`OWNERS`**（钳到上界）|
| **`100`** | **`OWNERS`** |

### 三种策略在越界时的完整行为

设枚举有 `n` 项、输入 `id`：

| 策略 | `id < 0` | `id >= n` | `-1`（n=5） | `5`（n=5） |
|---|---|---|---|---|
| **`ZERO`** | 第 0 项 | 第 0 项 | `ALL` | `ALL` |
| **`WRAP`** | `(id mod n + n) mod n` | `id mod n` | `OWNERS` | `MODERATORS` |
| **`CLAMP`** | 第 0 项 | 第 `n-1` 项 | `ALL` | `OWNERS` |

**本能力用一个 `byId|5` 就区分了全部三种策略**：

| 策略 | `byId\|5` 的结果 |
|---|---|
| `ZERO` | `ALL` |
| `WRAP` | `MODERATORS` |
| **`CLAMP`（本能力）** | **`OWNERS`** |

而 `byId|-1` 区分 `CLAMP`/`WRAP` 与 `ZERO`：

| 策略 | `byId\|-1` 的结果 |
|---|---|
| `ZERO` | `ALL` |
| `WRAP` | `OWNERS` |
| **`CLAMP`（本能力）** | **`ALL`** |

**两个用例的组合完整钉住了三种策略。**

## 核心发现二：`isEqualOrHigherThan` 是 `>=` 不是 `>`

```java
public boolean isEqualOrHigherThan(PermissionLevel other) {
    return this.id >= other.id;
}
```

| 单元 | 结果 |
|---|---|
| `higher\|OWNERS\|ALL` | `true` |
| `higher\|ALL\|OWNERS` | `false` |
| **`higher\|ALL\|ALL`** | **`true`**（**相等时为真**）|
| `higher\|ADMINS\|GAMEMASTERS` | `true` |
| `higher\|GAMEMASTERS\|ADMINS` | `false` |

**数值越大权限越高**：`ALL`(0) < `MODERATORS`(1) < `GAMEMASTERS`(2) < `ADMINS`(3) < `OWNERS`(4)。

若 Cubium 用 `>`，`higher|ALL|ALL` 会返回 `false`——**同权限判定会失败**，
这在实际命令权限检查中会导致「自己不能执行自己的命令」。

## 核心发现三：序列化名是复数，只有 `ALL` 是单数

| 枚举 | 序列化名 |
|---|---|
| `ALL` | `all` |
| **`MODERATORS`** | `moderators` |
| **`GAMEMASTERS`** | `gamemasters` |
| **`ADMINS`** | `admins` |
| **`OWNERS`** | `owners` |

`names` 输出：`all,moderators,gamemasters,admins,owners`

### 与 `sound_source` 的方向对比

| 能力 | 枚举名 | 序列化名 | 关系 |
|---|---|---|---|
| [`sound_source`](./sound_source.md) | `RECORDS`（复数）| `record`（单数）| **枚举复数、名字单数** |
| **`permission_level`** | **`MODERATORS`（复数）** | **`moderators`（复数）** | **两者都复数** |

**同一个项目里，两处「权限/通道」枚举对复数的处理完全相反**——不能靠「一律单数」或「一律复数」
推导，必须逐个核对。

## 核心发现四：只有 `getSerializedName()`，没有 `getName()`

| 能力 | `getName()` | `getSerializedName()` | `getKey()` |
|---|---|---|---|
| [`sound_source`](./sound_source.md) | 有 | 无 | 无 |
| [`difficulty`](./difficulty.md) | 无 | 有 | 有 |
| **`permission_level`** | **无** | **有** | **无** |
| [`boss_bar`](./boss_bar.md) | 有 | 有 | 无 |

**四种组合又出现了新的一个**——`permission_level` 是最「标准」的 `StringRepresentable` 实现
（只有 `getSerializedName()`）。

## 核心发现五：`INT_CODEC` 用 `BY_ID` 做双向映射

```java
public static final Codec<PermissionLevel> INT_CODEC =
    Codec.INT.xmap(BY_ID::apply, level -> level.id);
```

`INT_CODEC` 的解码侧直接调用 `BY_ID.apply`——**所以越界整数会静默钳到边界**，
不会报错。这是 `INT_CODEC` 与「严格范围校验」的关键区别。

（本能力未直接验证 `INT_CODEC` 的 codec 行为，但 `byId` 的单元已覆盖 `BY_ID` 本身。）

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`difficulty`](./difficulty.md) | `WRAP` 策略的对照案例 |
| [`display_slot`](./display_slot.md) | `ZERO` 策略的对照案例 |
| [`team_rule`](./team_rule.md) | `ZERO` 策略的对照案例 |
| [`sound_source`](./sound_source.md) | 复数/单数命名方向的对照案例 |
| [`command_argument`](./command_argument.md) | 命令权限检查的消费者 |

## 未覆盖范围

- `PermissionLevel.CODEC` / `INT_CODEC` 的 codec 往返（依赖 `DynamicOps`）
- `PermissionLevel` 在命令权限检查中的实际判定链（`CommandSourceStack.hasPermission`）
- `PermissionLevel` 与 `ops.json` 中 `level` 字段的对应关系
- 各等级对应的**具体命令白名单**（`Commands` 注册时的 `requires` 谓词）
- `PermissionLevel` 在 `/permission` 命令（若存在）中的用法