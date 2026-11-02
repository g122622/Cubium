# 差分能力：`sound_events`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 192 个能力。1637 个单元，**全部一致**——过程中查出并补齐了 **549 个缺失的音效 ID**。

## 测的是什么

原版每个音效都由 `SoundEvents.register(name)` 声明：

```java
public static final SoundEvent ALLAY_AMBIENT_WITH_ITEM = register("entity.allay.ambient_with_item");
public static final SoundEvent BLOCK_STONE_BREAK = register("block.stone.break");
```

本能力比对**音效 ID 全集**。

## 为什么音效 ID 必须全量比对

音效 ID 同时是两样东西：

1. **资源路径键**——`assets/minecraft/sounds.json` 用这些字符串作字段名；
2. **网络包内容**——`ClientboundSoundPacket` 携带的是 SoundEvent 的注册表 ID。

少一个音效意味着：

- 对应行为**完全无声**（播放请求找不到注册项）；
- 客户端收到未知 ID 时只能静默丢弃或回退到默认音效。

这类缺失不会崩溃、不会报错，只会让玩家「觉得哪里没声音」，极难排查。

## 查出的 549 处缺失

Cubium 的 `SoundEvents` 此前只有 **1254** 个 ID，原版有 **1637** 个：

| 大类 | 缺失数 |
|---|---|
| `block.*` | **416** |
| `entity.*` | **117** |
| `item.*` | 10 |
| `event.*` | 3 |
| `intentionally_empty` / `ui.*` / `weather.*` | 各 1 |

`block` 类占了 76%。举例：`block.amethyst_block.{break,fall,hit,place,step}` 整组缺失
（Cubium 只有 `chime` 与 `resonate` 两个特殊音）。这类「方块交互五连音」
（break / fall / hit / place / step）是成组缺失的主要形态。

补齐后 1637/1637 一致。

## 两个关键点

### 一、命名的机械映射

Cubium 的常量命名是 ID 的直接变换：

```
block.amethyst_block.break   →   BLOCK_AMETHYST_BLOCK_BREAK
```

即「`.` 换成 `_`，整体转大写」。因此 549 个常量可以脚本批量生成，
不需要逐个人工命名——**这类「命名规则确定」的常量表应优先脚本生成**，
既快又不会手抖写错。

### 二、`list` 必须确定性

oracle 的 `list` 通过反射枚举 `SoundEvents` 的字段。反射的**字段顺序不保证**跨 JVM
一致，直接用 `HashSet` 迭代会让单元表顺序漂移，破坏差分的可比性。

```java
private static List<String> collectSorted() throws IllegalAccessException {
    List<String> list = new ArrayList<>(collect());
    Collections.sort(list);
    return list;
}
```

同一份单元表用 `sort` 固定顺序，多次运行结果完全一致。

## 一个值得注意的观察：Cubium 多出 166 项

补齐后 Cubium 共有 1803 个唯一 ID，比原版的 1637 多 **166** 个。这些多出的项全是：

```
ambient.cave
ambient.basalt_deltas.{additions,loop,mood}
ambient.crimson_forest.{additions,loop,mood}
ambient.nether_wastes.{additions,loop,mood}
ambient.soul_sand_valley.{additions,loop,mood}
...
```

**这不是缺陷。** 原版这些环境音由 `AmbientSoundHandler` 通过各种 `AmbientAdditionsSound`
/ `AmbientMoodSound` / `AmbientLoopSoundEvent` 直接引用，**没有**在 `SoundEvents` 类里
声明为静态字段。Cubium 把它们统一收进了 `SoundEvents`，属于合理的组织方式差异。

> 教训：全量比对的差异**不一定是缺陷**。判定时要回到原版的使用路径，确认该 ID
> 在原版是否以另一种形式存在。

## 单元格式

单个音效 ID 路径（不含 `minecraft:` 前缀），例如：

```
block.stone.break
entity.allay.death
```

单元表由探针输出原版 1637 项全集，Cubium 缺项以 `MISSING` 暴露。

## 未覆盖范围

- **音效的 `SoundEvent` 元数据**（`fixedRange` 等）：本能力只比对 ID 存在性。
- **`sounds.json` 的实际内容**：文件是否真的存在、采样文件是否齐全，属于资源包验证，
  不在差分能力范围。
- **`ambient.*` 那 166 项的归类**：见上文，属于合理的组织差异，未登记为缺口。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`sound_source`](./sound_source.md) | `SoundSource` 枚举与序列化名 |
| [`block_sound_type`](./block_sound_type.md) | 方块的 `SoundType` 五元组 |
| [`jukebox_song`](./jukebox_song.md) | 唱片机曲目 |