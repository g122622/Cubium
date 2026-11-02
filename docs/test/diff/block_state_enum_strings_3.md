# 差分能力：`block_state_enum_strings_3`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 181 个能力，79 个用例全部一致。**探针调用 Cubium 的
`EnumProperty<T>::Traits::toString` / `fromName`**，一次覆盖 9 个方块状态枚举。

**本能力未发现 Cubium 缺陷**，但钉住了 `NoteBlockInstrument` 的 23 项和 `basedrum` 陷阱。

## 核心发现一：`NoteBlockInstrument` 有 23 项，但 `BASEDRUM` 不带下划线

| 常量 | 序列化名 | 含下划线 |
|---|---|---|
| `HARP` | `harp` | 否 |
| **`BASEDRUM`** | **`basedrum`** | **否** |
| `SNARE` | `snare` | 否 |
| `HAT` | `hat` | 否 |
| `BASS` | `bass` | 否 |
| `FLUTE` | `flute` | 否 |
| `BELL` | `bell` | 否 |
| `GUITAR` | `guitar` | 否 |
| `CHIME` | `chime` | 否 |
| `XYLOPHONE` | `xylophone` | 否 |
| **`IRON_XYLOPHONE`** | **`iron_xylophone`** | **是** |
| **`COW_BELL`** | **`cow_bell`** | **是** |
| `DIDGERIDOO` | `didgeridoo` | 否 |
| `BIT` | `bit` | 否 |
| `BANJO` | `banjo` | 否 |
| `PLING` | `pling` | 否 |
| `ZOMBIE` | `zombie` | 否 |
| `SKELETON` | `skeleton` | 否 |
| `CREEPER` | `creeper` | 否 |
| `DRAGON` | `dragon` | 否 |
| **`WITHER_SKELETON`** | **`wither_skeleton`** | **是** |
| `PIGLIN` | `piglin` | 否 |
| **`CUSTOM_HEAD`** | **`custom_head`** | **是** |

**只有 4 项含下划线**（`iron_xylophone` / `cow_bell` / `wither_skeleton` / `custom_head`）。

### `BASEDRUM` 是最容易出错的一项

直觉上「base drum」是两个单词，应该写作 `base_drum`。但原版把常量名写成**一个单词**
`BASEDRUM`，序列化名也是 `basedrum`。

实测：

```
NoteBlockInstrument|toString|BASEDRUM      -> basedrum
NoteBlockInstrument|fromName|basedrum      -> BASEDRUM
NoteBlockInstrument|fromName|base_drum     -> null      ← 不存在
```

**Cubium 实现正确。**

这个陷阱的实际影响：音符盒的 `instrument` 方块状态若写成 `base_drum`，
**整个音符盒状态无法解析**，比林式的底鼓会失效。

## 核心发现二：`SideChainPart` 原版枚举序是 `RIGHT` 在 `CENTER` 之前

```java
public enum SideChainPart implements StringRepresentable {
    UNCONNECTED("unconnected"),
    RIGHT("right"),
    CENTER("center"),
    LEFT("left");
}
```

| 位置 | 原版 | Cubium |
|---|---|---|
| 0 | `UNCONNECTED` | `Unconnected` |
| 1 | **`RIGHT`** | **`Left`** |
| 2 | `CENTER` | `Center` |
| 3 | **`LEFT`** | **`Right`** |

**`RIGHT` 与 `LEFT` 的位置在两边是相反的。**

**这不是缺陷**——本能力的单元用「名字 ↔ 值」成对映射，`toString` 与 `fromName` 都一致。
但它是一个**调色板索引隐患**：若某处按 `static_cast<u8>` 直接映射到索引，`right` 会落到索引 1
（Cubium）而不是索引 3（原版）。

**这条已记录在文档中，但未作为缺陷处理**——因为 `EnumProperty` 的 `getAllValues()`
顺序由 `create("part", {...})` 调用处显式给出，与枚举声明顺序无关。

## 核心发现三：`WallHeight` 对应原版的 `WallSide`

| Cubium | 原版 |
|---|---|
| `WallHeight` | **`WallSide`** |
| `None` / `Low` / `Tall` | `NONE` / `LOW` / `TALL` |

**名字不同**（Height vs Side），但语义与序列化名完全一致。

## 核心发现四：`OxidationLevel` 在 1.21.11 没有对应的原版枚举

Cubium 有 `BlockStateProperties::OxidationLevel`（`Unaffected`/`Exposed`/`Weathered`/`Oxidized`），
但原版 **1.21.11 没有** `OxidationLevel` 枚举：

- jar 中只有 `CopperGolemOxidationLevel`（一个 **record**，不是枚举）
- `net/minecraft/world/block/OxidationLevel.class` 不存在

且 Cubium 的 `OxidationLevel` **没有 `toString` 特化**（`EnumProperty.cpp` 中查无此项）。

**因此本能力不覆盖它**——没有可对比的原版实现。

## 9 个枚举的完整清单

| 枚举 | 项数 | 序列化名 |
|---|---|---|
| `NoteBlockInstrument` | **23** | 见上表 |
| `ComparatorMode` | 2 | `compare` / `subtract` |
| `StructureMode` | 4 | `save` / `load` / `corner` / `data` |
| `TestBlockMode` | 4 | `start` / `log` / `fail` / `accept` |
| `TrialSpawnerState` | 6 | `inactive` / `waiting_for_players` / `active` / `waiting_for_reward_ejection` / `ejecting_reward` / `cooldown` |
| `VaultState` | 4 | `inactive` / `active` / `unlocking` / `ejecting` |
| `CreakingHeartState` | 3 | `uprooted` / `dormant` / `awake` |
| `SideChainPart` | 4 | `unconnected` / `right` / `center` / `left` |
| `WallHeight`（原版 `WallSide`）| 3 | `none` / `low` / `tall` |

## 踩坑一：`NoteBlockInstrument` 需要 bootstrap（第五次踩坑）

`NoteBlockInstrument` 的静态初始化引用了 `SoundEvents`（注册表条目）：

```
Exception in thread "main" java.lang.ExceptionInInitializerError
Caused by: java.lang.IllegalArgumentException: Not bootstrapped (called from registry minecraft:game_event)
    at net.minecraft.server.Bootstrap.createBootstrapException(Bootstrap.java:124)
```

修复：在 `main` 开头加

```java
net.minecraft.SharedConstants.tryDetectVersion();
net.minecraft.server.Bootstrap.bootStrap();
```

**这是本会话第五次踩到同一个坑。** 凡是 oracle 触碰任何**枚举类的静态初始化**（哪怕只读
`Enum.values()`），都要先确认该枚举的 `static {}` 或字段初始化是否引用注册表。

## 踩坑二：PowerShell 编辑大文件导致括号失衡

用 PowerShell 逐行删除 `OxidationLevel` 相关行时，跳过了「空行」判断，留下了一个孤立的 `};`，
导致括号计数 **开 106 / 闭 107**，编译报：

```
error: unknown type name 'Probe'
error: use of undeclared identifier 'writeList'; did you mean 'mc::diff::writeList'?
error: extraneous closing brace ('}')
```

**定位方法**：逐字符扫描括号深度，输出第一个出现负深度（或最终不平衡）的行号——比逐段读代码快得多。

**教训**：用脚本删除代码块后，务必检查 `{` 与 `}` 计数是否相等。本能力因此多花了约 20 分钟。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`block_state_enum_strings`](./block_state_enum_strings.md) | 第一组（5 个枚举）|
| [`block_state_enum_strings_2`](./block_state_enum_strings_2.md) | 第二组（11 个枚举）|
| [`note_block_tuning`](./note_block_tuning.md) | 音符盒调音（`NoteBlockInstrument` 的消费者）|
| [`trial_spawner_config`](./trial_spawner_config.md) | 试炼刷怪笼（`TrialSpawnerState` 的消费者）|

## 未覆盖范围

- `CopperGolemPose`（Cubium 有，原版无对应枚举）
- `OxidationLevel`（原版无对应枚举）
- 其余已覆盖的 16 个枚举见前两个能力
- 各枚举在 `block_state_ids` 调色板中的实际索引