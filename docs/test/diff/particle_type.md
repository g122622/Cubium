# 差分能力：`particle_type`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 193 个能力。115 个单元，**110 一致、5 处已登记的命名差异**。

## 测的是什么

`ParticleTypeId` 的枚举值**直接就是网络协议 ID**：

```cpp
enum class ParticleTypeId : u16 {
    AngryVillager = 0,
    Block = 1,
    BlockMarker = 2,
    // ...
};
```

服务端发的 `ClientboundLevelParticlesPacket` 里带的就是这个数值，客户端按下标查表。

本能力比对 **协议 ID → 粒子名** 的映射，与 `BuiltInRegistries.PARTICLE_TYPE` 的迭代顺序
（即原版的协议 ID 顺序）对齐。

## 为什么这个能力重要

**索引只要错一位，后面所有粒子都会错位**，表现为「放烟花却出末影粒子」这类完全不讲
道理的现象，且**不会报任何错误**——协议层只看到一串数字。

这是本会话里少数「错了也完全不报错」的类型，差分是唯一可靠的防线。

## 结果：索引完全对齐

115 个协议 ID 的**顺序与原版逐一吻合**。这是本项目做得相当扎实的一处。

差异只有 5 处，全是**命名差异**（索引正确，只是枚举名不同）：

| 协议 ID | 原版注册表 ID | Cubium 枚举名 | 性质 |
|---|---|---|---|
| 16 | `effect` | `Spell` | 旧版 MC 命名 |
| 22 | `explosion_emitter` | `HugeExplosion` | 旧版 MC 命名 |
| 46 | `instant_effect` | `InstantSpell` | 旧版 MC 命名 |
| 75 | `campfire_cosy_smoke` | `CampfireCozy` | `cosy` / `cozy` 拼写差异 |
| 76 | `campfire_signal_smoke` | `CampfireSignal` | 省略了 `_smoke` |

这 5 项已登记为 `knownDifferences`。**不是缺陷**：枚举名只在本项目内部引用，
不参与网络序列化，也不参与资源路径查找。

## Cubium 的内部扩展类型

Cubium 在 0..114 之外还定义了 10 个**本项目内部扩展**的粒子：

```
Breaking(115)  Barrier(116)  Light(117)  Redstone(118)  LargeExplosion(119)
ItemPickup(120)  DrippingCherryLeaves(121)  FallingCherryLeaves(122)
LandingCherryLeaves(123)  Count(124)
```

代码里有明确的界定：

```cpp
inline constexpr u16 PROTOCOL_PARTICLE_TYPE_COUNT = 115;
// 协议 ID 范围为 0~114，超出此范围的为本项目内部扩展类型
```

**这是合理的设计**——扩展类型只在 Cubium 内部流通，不发给原版客户端。
本能力只比对 0..114，扩展部分不参与。

> 注意 `DrippingCherryLeaves` / `FallingCherryLeaves` / `LandingCherryLeaves` 是 1.20
> 时期的旧名字。原版现在把它们统一成了 `cherry_leaves` 一张粒子表。因属于内部扩展
> 类型，未登记为缺口，但**若将来要把这几个扩展类型同步给原版客户端，名字需要先对齐**。

## 关键点：命名归一化

Cubium 用驼峰枚举名（`AngryVillager`），原版用 snake_case 注册表 ID（`angry_villager`）。
探针在输出前做归一化：

```cpp
std::string toRegistryName(std::string_view camel)
{
    // AngryVillager -> angry_villager
    // 每个大写字母前插下划线，然后整体转小写
}
```

**不做归一化，115 个单元会全部报差异**，把一个「索引完全正确」的能力伪装成全面失败。

## 单元格式

`<协议ID>|<原版注册表ID>`，例如：

```
0|angry_villager
16|effect
```

单元表由探针按原版注册表顺序枚举（`%zu|%s`），oracle 用同一顺序按下标查。

## 未覆盖范围

- **粒子的载荷类型**（`BlockParticleOption`、`DustParticleOptions` 等）：不同粒子的
  网络包结构不同，本能力只比对 ID。
- **内部扩展类型（115..124）**：见上文，不发给原版客户端，不参与比对。
- **粒子的渲染参数**（速度、颜色、数量）：属于包内容，不在本能力范围。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`sound_source`](./sound_source.md) | 同样是「枚举 ↔ 协议」的映射 |
| [`render_type_strings`](./render_type_strings.md) | 渲染类型枚举的序列化名 |