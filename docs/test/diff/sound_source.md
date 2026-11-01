# 差分能力：`sound_source`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 167 个能力，23 个用例全部一致。**声音通道**此前未被覆盖——音效领域此前只有
[`jukebox_song`](./jukebox_song.md) 与 [`block_sound_type`](./block_sound_type.md)。

## 核心发现一：三个复数枚举映射到单数名字

```java
public enum SoundSource {
    MASTER("master"),
    MUSIC("music"),
    RECORDS("record"),      // ← 单数
    WEATHER("weather"),
    BLOCKS("block"),        // ← 单数
    HOSTILE("hostile"),
    NEUTRAL("neutral"),
    PLAYERS("player"),      // ← 单数
    AMBIENT("ambient"),
    VOICE("voice"),
    UI("ui");
```

| 枚举 | `getName()` | 说明 |
|---|---|---|
| `MASTER` | `master` | 小写同名 |
| `MUSIC` | `music` | 小写同名 |
| **`RECORDS`** | **`record`** | **单数** |
| `WEATHER` | `weather` | 小写同名 |
| **`BLOCKS`** | **`block`** | **单数** |
| `HOSTILE` | `hostile` | 小写同名 |
| `NEUTRAL` | `neutral` | 小写同名 |
| **`PLAYERS`** | **`player`** | **单数** |
| `AMBIENT` | `ambient` | 小写同名 |
| `VOICE` | `voice` | 小写同名 |
| `UI` | `ui` | 小写同名 |

**11 项里有 3 项是「复数枚举 → 单数名」**。

若 Cubium 统一用「枚举名小写」（一个极自然的实现），会得到 `records` / `blocks` / `players`——
**三项全错**。

这与 [`team_rule`](./team_rule.md) 的 `pushOtherTeams`（复数）/ `pushOwnTeam`（单数）是**相反方向**
的不一致：那里是名字带复数，这里是枚举名带复数。

## 核心发现二：`SoundSource` 没有实现 `StringRepresentable`

```java
public enum SoundSource {
    ...
    private final String name;
    public String getName() { return this.name; }
}
```

**没有 `getSerializedName()`，也没有 `CODEC`。** 与绝大多数的枚举不同。

| 能力 | `getName()` | `getSerializedName()` | `CODEC` |
|---|---|---|---|
| **`SoundSource`** | **有** | **无** | **无** |
| [`boss_bar`](./boss_bar.md) | 有 | 有 | 有 |
| [`display_slot`](./display_slot.md) | 无 | 有 | 有 |
| [`objective_criteria`](./objective_criteria.md) | 有 | 无 | 有 |

**四种组合都出现过**——不能假设 Minecraft 的枚举有统一接口。

**后果**：`SoundSource` 无法直接走 `StringRepresentable.fromEnum`，需要手写 `Map` 或 switch 才能
序列化。若 Cubium 提供了 `CODEC`，说明是自行添加的，与原版不同。

## 核心发现三：`MASTER` 与 `MUSIC` 是全局音量通道

| 类别 | 通道 |
|---|---|
| **全局** | `master`（总音量）、`music`（背景音乐）|
| **世界** | `record`、`weather`、`block`、`ambient` |
| **生物** | `hostile`、`neutral`、`player`、`voice` |
| **界面** | `ui` |

`master` 不参与 3D 定位——它是「所有声音的总闸」。`music` 同理（背景音乐无空间位置）。

**这一区别不影响 `getName()`**，但解释了顺序：全局通道排在最前。

## 核心发现四：顺序是语义分组，不是字母序

```
master,music,record,weather,block,hostile,neutral,player,ambient,voice,ui
```

- `master` → `music`：全局
- `record` → `weather` → `block` → `ambient`：世界
- `hostile` → `neutral` → `player` → `voice`：生物
- `ui`：界面

**字母序会得到** `ambient,block,hostile,master,music,neutral,player,record,ui,voice,weather`——
与原版完全不同。

`ambient` 在 `voice` 前（语义分组内），字母序会在最前。

## 核心发现五：`byName` 是大小写敏感、且只认单数名

| 单元 | 结果 | 说明 |
|---|---|---|
| `byName\|record` | `RECORDS` | ✓ |
| `byName\|block` | `BLOCKS` | ✓ |
| `byName\|player` | `PLAYERS` | ✓ |
| `byName\|master` | `MASTER` | ✓ |
| `byName\|ui` | `UI` | ✓ |
| **`byName\|records`** | **`null`** | 复数拼写不存在 |
| **`byName\|blocks`** | **`null`** | 复数拼写不存在 |
| **`byName\|players`** | **`null`** | 复数拼写不存在 |
| **`byName\|MASTER`** | **`null`** | **大写不匹配** |
| **`byName\|`**（空串） | **`null`** | 无空名项 |

**`byName` 是本能力定义的查询语义**（原版无此入口）。本能力在探针与 oracle 两侧用**同一规则**实现
（遍历 `values()`，首个 `getName().equals(name)` 胜出），因此这不是「对齐原版行为」，
而是「钉住 `getName()` 的实际取值」。

**这是本能力的一个方法论限制**：`byName` 部分的单元验证的是「两侧实现同一算法」，
真正与原版对齐的是 `name` / `names` / `count` 三组单元。`byName` 的价值在于**把单数名写入单元表**，
使 `records` / `blocks` / `players` 的错误拼写被显式记录为「不存在的名字」。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`jukebox_song`](./jukebox_song.md) | 唱片（`record` 通道的播放源）|
| [`block_sound_type`](./block_sound_type.md) | 方块音效类型 |
| [`chat_formatting`](./chat_formatting.md) | 另一个「命名不一致」的枚举 |
| [`team_rule`](./team_rule.md) | 复数/单数不一致的对照案例 |

## 未覆盖范围

- `SoundSource` 的序列化（原版无入口，客户端已停止维护）
- 各通道的**实际音量控制**（客户端音频系统）
- `SoundEvent` / `SoundEvents` 注册表（需要注册表 bootstrap 且内容庞大）
- `ClientboundSoundPacket` 等网络包中 `SoundSource` 的编码方式
- `SoundSource` 在 `Options` 中的持久化（客户端设置文件）