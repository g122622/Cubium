# 差分能力：`game_type`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 177 个能力，27 个用例全部一致。**探针调用 Cubium 的 `mc::GameMode` 枚举与名字转换**，
是第七个「对接真实实现」的能力。

**本能力未发现 Cubium 缺陷**，但核实了两个极易搞错的布尔方法语义。

## 核心发现一：`DEFAULT_MODE` 是别名，不是第 5 个枚举项

```java
public enum GameType implements StringRepresentable {
    SURVIVAL(0, "survival"),
    CREATIVE(1, "creative"),
    ADVENTURE(2, "adventure"),
    SPECTATOR(3, "spectator");

    public static final GameType DEFAULT_MODE = SURVIVAL;   // ← 别名，不在 values() 中
    private static final int NOT_SET = -1;                  // ← 内部常量，不是枚举项
}
```

`count` 单元输出 **`4`**——`values()` 只有 4 项。

### 与 Cubium 的对应关系

Cubium 的 `GameMode` 有 **5 个枚举值**：

```cpp
enum class GameMode : u8 {
    Survival = 0, Creative = 1, Adventure = 2, Spectator = 3,
    NotSet = 255          // ← 真实的第 5 个枚举值
};
```

原版的 `NOT_SET` 只是**私有 int 常量 `-1`**，从不作为枚举值出现在 `values()` 中；
`DEFAULT_MODE` 也只是指向 `SURVIVAL` 的别名。

**Cubium 用 `NotSet = 255` 建模「未设置」是合理的**（C++ 枚举无法「别名」），
但它不是原版 `values()` 的一部分，因此本能力的 `count` 与 `names` 都排除它。

## 核心发现二：两个布尔方法的语义都不直观

**我最初的推断是错的**，查源码后纠正：

| 模式 | `isCreative()` | `isSurvival()` | `isBlockPlacingRestricted()` |
|---|---|---|---|
| `SURVIVAL` | false | **true** | false |
| `CREATIVE` | **true** | false | **false** |
| `ADVENTURE` | false | **true** | **true** |
| `SPECTATOR` | false | false | **true** |

实测输出（`flags` 依次为 creative / survival / restricted）：

```
flags|SURVIVAL  -> 010
flags|CREATIVE  -> 100
flags|ADVENTURE -> 011
flags|SPECTATOR -> 001
```

### 两条反直觉的结论

```java
public boolean isSurvival() {
    return this == SURVIVAL || this == ADVENTURE;      // ← 冒险模式也算「生存」
}

public boolean isBlockPlacingRestricted() {
    return this == ADVENTURE || this == SPECTATOR;     // ← 创造模式不限制放方块
}
```

1. **`isSurvival()` 对 `ADVENTURE` 返回 `true`**——冒险模式是「受限的生存」，
   共享生存模式的多数行为（生命、饥饿、伤害）。
2. **`isBlockPlacingRestricted()` 对 `CREATIVE` 返回 `false`**——创造模式当然能放方块。
   若按方法名直觉写成「非 Survival 就受限」，会把 `CREATIVE` 判错。

**我最初把 `isBlockPlacingRestricted` 写成 `creative || spectator`**，被源码纠正为
`adventure || spectator`。

## 核心发现三：`BY_ID` 用 `ZERO` 越界策略

```java
private static final IntFunction<GameType> BY_ID =
    ByIdMap.continuous(GameType::getId, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
```

| 输入 | 结果 |
|---|---|
| `0` | `SURVIVAL` |
| `1` | `CREATIVE` |
| `2` | `ADVENTURE` |
| `3` | `SPECTATOR` |
| **`4`** | **`SURVIVAL`** |
| **`-1`** | **`SURVIVAL`** |
| **`100`** | **`SURVIVAL`** |

### 三种策略在 `id = 4` 时的对照

| 策略 | `id=4` 结果 |
|---|---|
| **`ZERO`（本能力）** | **`SURVIVAL`** |
| `WRAP`（[`difficulty`](./difficulty.md)）| `SURVIVAL`（4 % 4 = 0，巧合相同）|
| `CLAMP`（[`permission_level`](./permission_level.md)）| **`SPECTATOR`** |

**`id = -1` 才是 `ZERO` 与 `WRAP` 的区分点**：

| 策略 | `id=-1` 结果 |
|---|---|
| **`ZERO`（本能力）** | **`SURVIVAL`** |
| `WRAP` | `SPECTATOR` |
| `CLAMP` | `SURVIVAL` |

合成两个用例（`byId|4` + `byId|-1`）才能区分三种策略。

## 核心发现四：短名与长名的前缀不同

```java
this.shortName = Component.translatable("selectWorld.gameMode." + name);
this.longName  = Component.translatable("gameMode." + name);
```

| 方法 | 键 |
|---|---|
| `getShortDisplayName()` | **`selectWorld.gameMode.<name>`** |
| `getLongDisplayName()` | **`gameMode.<name>`** |

**短名多一个 `selectWorld.` 前缀**——不是「同一前缀 + `.short` / `.long`」的关系。

实测：

```
shortName|SURVIVAL -> selectWorld.gameMode.survival
longName|SURVIVAL  -> gameMode.survival
```

## 核心发现五：名字是单个小写单词

```
survival,creative,adventure,spectator
```

无下划线、无驼峰。`getName()` 与 `getSerializedName()` 返回同一字段。

## Cubium 侧的两处独立实现

Cubium 有两处 GameMode→字符串的转换，**对 `NotSet` 的处理不同**：

| 文件 | `NotSet` 映射 | 用途 |
|---|---|---|
| `GameModeCommand.cpp:190` `_getGameModeName` | `not_set` | 命令反馈文本 |
| `ScriptSimulatedPlayer.cpp:278` `_gameModeToString` | `default` | 基岩版脚本 API |

**这不是缺陷**——两处服务于不同目标（命令文本 vs 基岩 `GameMode.default`），
注释里都写明了理由。但**是重复实现的隐患**，未来若需要统一的序列化名，
应提取到公共位置。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`difficulty`](./difficulty.md) | `WRAP` 越界策略 |
| [`permission_level`](./permission_level.md) | `CLAMP` 越界策略 |
| [`render_type_strings`](./render_type_strings.md) | 同为「小枚举 + 名字映射」|
| [`gamerule`](./gamerule.md) | 游戏规则（另一个命令参数体系）|

## 未覆盖范围

- `updatePlayerAbilities(Abilities)`（会改动 `mayfly` / `instabuild` / `invulnerable` / `mayBuild`）
- `LEGACY_ID_CODEC`（已 `@Deprecated`，用 `Codec.INT.xmap`）
- `STREAM_CODEC` 的网络字节编码
- 命令参数 `GameModeArgument` 的解析（`GameModeCommand` 里的字面量节点）
- `/gamemode` 命令的完整执行链